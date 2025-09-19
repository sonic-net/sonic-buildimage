use std::io::{BufRead, BufReader, Write};
use std::net::{TcpListener, TcpStream};
use std::time::{Duration, Instant};
use std::process::{Command, Stdio};
use std::fs;
use std::env;

use serde::Serialize;
use redis::Commands;

// Fail-open: if Redis is down or field is missing/invalid, default to 50051
const DEFAULT_TELEMETRY_SERVICE_PORT: u16 = 50051;

#[derive(Serialize)]
struct HealthStatus {
    check_telemetry_port: String,
    xpath_commands: Vec<CommandResult>,
}

#[derive(Serialize, Clone)]
struct CommandResult {
    xpath: String,
    success: bool,
    error: Option<String>,
    duration_ms: u128,
}

const CMD_LIST_JSON: &str = "/cmd_list.json"; // absolute path inside container
const XPATH_ENV_VAR: &str = "TELEMETRY_WATCHDOG_XPATHS"; // comma-separated list
const XPATH_ENV_BLACKLIST: &str = "TELEMETRY_WATCHDOG_XPATHS_BLACKLIST"; // comma-separated list to exclude
const CMD_TIMEOUT_ENV_VAR: &str = "TELEMETRY_WATCHDOG_CMD_TIMEOUT_SECS"; // per-command timeout seconds
const GNMI_BASE_CMD: &str = "gnmi_get"; // assumed in PATH
const TARGET_NAME_ENV_VAR: &str = "TELEMETRY_WATCHDOG_TARGET_NAME"; // optional override for target_name
const DEFAULT_TARGET_NAME: &str = "server.ndastreaming.ap.gbl";
const DEFAULT_CA_CRT: &str = "/etc/sonic/telemetry/dsmsroot.cer";
const DEFAULT_SERVER_CRT: &str = "/etc/sonic/telemetry/streamingtelemetryserver.cer";
const DEFAULT_SERVER_KEY: &str = "/etc/sonic/telemetry/streamingtelemetryserver.key";

// Configuration:
// 1. JSON file (/cmd_list.json) optional. Format:
//    {
//        "xpaths": [
//            "reboot-cause/history"
//        ]
//    }
// 2. Environment variable TELEMETRY_WATCHDOG_XPATHS optional. Comma-separated list of xpaths.
// Both sources are merged; duplicates removed (first occurrence kept).
// During the probe request, after verifying the GNMI port is reachable, each xpath results in a command:
//   gnmi_get -xpath_target SHOW -xpath <XPATH> -target_addr 127.0.0.1:<port> -logtostderr [ -ca <ca> -cert <cert> -key <key> -target_name <name> | -insecure true ]
// Cert paths and client_auth/target_name are pulled from Redis hashes (TELEMETRY|certs, TELEMETRY|gnmi).
// client_auth now: ONLY explicit Redis value "true" (case-insensitive) enables TLS client auth; anything else (missing/other value) -> insecure.
// Any failure (spawn error or non-zero exit status) causes overall HTTP 500 with per-xpath results in JSON body.

fn load_xpath_list() -> Vec<String> {
    let mut list: Vec<String> = Vec::new();

    // JSON file expected format: { "xpaths": ["reboot-cause/history", "lldp/neighbors"] }
    match fs::read_to_string(CMD_LIST_JSON) {
        Ok(content) => {
            #[derive(serde::Deserialize)]
            struct JsonCfg { xpaths: Option<Vec<String>> }
            match serde_json::from_str::<JsonCfg>(&content) {
                Ok(cfg) => {
                    if let Some(mut xs) = cfg.xpaths { list.append(&mut xs); }
                },
                Err(e) => eprintln!("Failed to parse {}: {}", CMD_LIST_JSON, e),
            }
        },
    Err(e) => eprintln!("Could not read {}: {} (will continue with env var only)", CMD_LIST_JSON, e),
    }

    if let Ok(env_val) = env::var(XPATH_ENV_VAR) {
        for part in env_val.split(',') { let trimmed = part.trim(); if !trimmed.is_empty() { list.push(trimmed.to_string()); } }
    }

    // dedupe while preserving order
    let mut seen = std::collections::HashSet::new();
    list.retain(|x| seen.insert(x.clone()));
    // apply blacklist from env
    if let Ok(blacklist) = env::var(XPATH_ENV_BLACKLIST) {
        let mut blk = std::collections::HashSet::new();
        for part in blacklist.split(',') {
            let trimmed = part.trim();
            if !trimmed.is_empty() { blk.insert(trimmed.to_string()); }
        }
        if !blk.is_empty() {
            list.retain(|x| !blk.contains(x));
        }
    }
    list
}

struct TelemetrySecurityConfig {
    use_client_auth: bool,
    ca_crt: String,
    server_crt: String,
    server_key: String,
}

fn run_gnmi_for_xpath(xpath: &str, port: u16, sec: &TelemetrySecurityConfig, target_name: &str, timeout: Duration) -> CommandResult {
    // Build full command: gnmi_get -xpath_target SHOW -xpath <xpath> -target_addr 127.0.0.1:<port> -logtostderr -insecure true
    let addr = format!("127.0.0.1:{port}");
    let start = Instant::now();
    let mut cmd = Command::new(GNMI_BASE_CMD);
    cmd.arg("-xpath_target").arg("SHOW")
        .arg("-xpath").arg(xpath)
        .arg("-target_addr").arg(addr)
        .arg("-logtostderr")
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    if sec.use_client_auth {
        cmd.arg("-ca").arg(&sec.ca_crt)
            .arg("-cert").arg(&sec.server_crt)
            .arg("-key").arg(&sec.server_key)
            .arg("-target_name").arg(target_name);
    } else {
        // no client auth -> insecure mode
        cmd.arg("-insecure").arg("true");
    }
    // Enforce timeout
    let mut child = match cmd.spawn() {
        Ok(c) => c,
        Err(e) => {
            let dur = start.elapsed().as_millis();
            eprintln!("Failed to spawn gnmi_get for {}: {}", xpath, e);
            return CommandResult { xpath: xpath.to_string(), success: false, error: Some(e.to_string()), duration_ms: dur };
        }
    };

    let output = {
        let start_wait = Instant::now();
        loop {
            match child.try_wait() {
                Ok(Some(_status)) => {
                    // Process exited; collect output
                    match child.wait_with_output() {
                        Ok(out) => break Ok(out),
                        Err(e) => break Err(e),
                    }
                }
                Ok(None) => {
                    if start_wait.elapsed() >= timeout {
                        // kill on timeout
                        let _ = child.kill();
                        let _ = child.wait();
                        break Err(std::io::Error::new(std::io::ErrorKind::TimedOut, "gnmi_get timed out"));
                    }
                    std::thread::sleep(Duration::from_millis(50));
                }
                Err(e) => {
                    break Err(e);
                }
            }
        }
    };
    let dur = start.elapsed().as_millis();

    match output {
        Ok(out) => {
            if out.status.success() {
                println!("gnmi_get success xpath={}", xpath);
                CommandResult { xpath: xpath.to_string(), success: true, error: None, duration_ms: dur }
            } else {
                let stderr = String::from_utf8_lossy(&out.stderr).to_string();
                eprintln!("gnmi_get failed xpath={} status={:?} err={}", xpath, out.status.code(), stderr);
                CommandResult { xpath: xpath.to_string(), success: false, error: Some(stderr), duration_ms: dur }
            }
        },
        Err(e) => {
            eprintln!("Failed to spawn gnmi_get for {}: {}", xpath, e);
            CommandResult { xpath: xpath.to_string(), success: false, error: Some(e.to_string()), duration_ms: dur }
        }
    }
}

fn get_security_config() -> TelemetrySecurityConfig {
    // Redis DB 4 hashes:
    // TELEMETRY|certs: ca_crt, server_crt, server_key
    // TELEMETRY|gnmi: client_auth, target_name (target_name new; if absent we still proceed)
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error (security): {e}");
            return TelemetrySecurityConfig { use_client_auth: false, ca_crt: DEFAULT_CA_CRT.to_string(), server_crt: DEFAULT_SERVER_CRT.to_string(), server_key: DEFAULT_SERVER_KEY.to_string() };
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error (security): {e}");
            return TelemetrySecurityConfig { use_client_auth: false, ca_crt: DEFAULT_CA_CRT.to_string(), server_crt: DEFAULT_SERVER_CRT.to_string(), server_key: DEFAULT_SERVER_KEY.to_string() };
        }
    };

    let mut get_field = |hash: &str, field: &str| -> Option<String> {
        let r: redis::RedisResult<Option<String>> = conn.hget(hash, field);
        match r { Ok(v) => v, Err(e) => { eprintln!("Redis HGET error {hash}.{field}: {e}"); None } }
    };

    let ca_crt = get_field("TELEMETRY|certs", "ca_crt")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_CA_CRT.to_string());
    let server_crt = get_field("TELEMETRY|certs", "server_crt")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_SERVER_CRT.to_string());
    let server_key = get_field("TELEMETRY|certs", "server_key")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_SERVER_KEY.to_string());
    let client_auth_opt = get_field("TELEMETRY|gnmi", "client_auth");
    // Only explicit "true" turns on client auth; everything else (including None) -> false
    let use_client_auth = matches!(client_auth_opt.as_ref(), Some(v) if v.eq_ignore_ascii_case("true"));
    TelemetrySecurityConfig { use_client_auth, ca_crt, server_crt, server_key }
}

fn get_target_name() -> String {
    match env::var(TARGET_NAME_ENV_VAR) {
        Ok(v) if !v.trim().is_empty() => v.trim().to_string(),
        _ => DEFAULT_TARGET_NAME.to_string(),
    }
}

fn read_timeout() -> Duration {
    const DEFAULT_SECS: u64 = 5;
    match env::var(CMD_TIMEOUT_ENV_VAR) {
        Ok(val) => match val.trim().parse::<u64>() {
            Ok(secs) if secs > 0 => Duration::from_secs(secs),
            _ => Duration::from_secs(DEFAULT_SECS),
        },
        Err(_) => Duration::from_secs(DEFAULT_SECS),
    }
}

fn get_gnmi_port() -> u16 {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error (port): {e}");
            return DEFAULT_TELEMETRY_SERVICE_PORT;
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error (port): {e}");
            return DEFAULT_TELEMETRY_SERVICE_PORT;
        }
    };

    let res: redis::RedisResult<Option<String>> = conn.hget("TELEMETRY|gnmi", "port");
    match res {
        Ok(Some(p)) => p.parse::<u16>().unwrap_or_else(|_| {
            eprintln!("Redis: TELEMETRY|gnmi.port not a valid u16: {p}");
            DEFAULT_TELEMETRY_SERVICE_PORT
        }),
        Ok(None) => {
            eprintln!("Redis: TELEMETRY|gnmi.port missing; defaulting to {}", DEFAULT_TELEMETRY_SERVICE_PORT);
            DEFAULT_TELEMETRY_SERVICE_PORT
        }
        Err(e) => {
            eprintln!("Redis HGET error (port): {e}");
            DEFAULT_TELEMETRY_SERVICE_PORT
        }
    }
}

// Connects to Redis DB 4 and returns true if the telemetry feature is enabled.
// If Redis is unavailable or the field is missing, default to enabled (fail-open).
fn is_telemetry_enabled() -> bool {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error (feature): {e}");
            return true;
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error (feature): {e}");
            return true;
        }
    };

    let res: redis::RedisResult<Option<String>> = conn.hget("FEATURE|telemetry", "state");
    match res {
        Ok(Some(state)) => !state.eq_ignore_ascii_case("disabled"),
        Ok(None) => {
            eprintln!("Redis: FEATURE|telemetry.state missing; defaulting to enabled");
            true
        }
        Err(e) => {
            eprintln!("Redis HGET error (feature): {e}");
            true
        }
    }
}

fn check_telemetry_port() -> String {
    let port = get_gnmi_port();
    let addr = format!("127.0.0.1:{port}");
    match TcpStream::connect(&addr) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn main() {
    let listener = TcpListener::bind("127.0.0.1:50080")
        .expect("Failed to bind to 127.0.0.1:50080");
    println!("Watchdog HTTP server running on http://127.0.0.1:50080");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut reader = BufReader::new(&stream);
                let mut request_line = String::new();

                if let Ok(_) = reader.read_line(&mut request_line) {
                    println!("Received request: {}", request_line.trim_end());

                    if !request_line.starts_with("GET /") {
                        let response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
                        let _ = stream.write_all(response.as_bytes());
                        continue;
                    }

                    let telemetry_enabled = is_telemetry_enabled();

                    let mut http_status = "HTTP/1.1 200 OK";
                    let check_port_result;
                    let mut cmd_results: Vec<CommandResult> = Vec::new();

                    if !telemetry_enabled {
                        check_port_result = "SKIPPED: feature disabled".to_string();
                    } else {
                        check_port_result = check_telemetry_port();
                        if !check_port_result.starts_with("OK") { http_status = "HTTP/1.1 500 Internal Server Error"; }

                        // Only run xpath commands if port is OK
                        if http_status == "HTTP/1.1 200 OK" {
                            let xpaths = load_xpath_list();
                            let port = get_gnmi_port();
                            let sec_cfg = get_security_config();
                            let timeout = read_timeout();
                            let target_name = get_target_name();
                            for xp in xpaths {
                                let res = run_gnmi_for_xpath(&xp, port, &sec_cfg, &target_name, timeout);
                                if !res.success { http_status = "HTTP/1.1 500 Internal Server Error"; }
                                cmd_results.push(res);
                            }
                        }
                    }

                    let status = HealthStatus { check_telemetry_port: check_port_result, xpath_commands: cmd_results };

                    let json_body = serde_json::to_string(&status).unwrap();
                    let response = format!(
                        "{http_status}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                        json_body.len(),
                        json_body
                    );

                    if let Err(e) = stream.write_all(response.as_bytes()) {
                        eprintln!("Failed to write response: {}", e);
                    }
                }
            }
            Err(e) => eprintln!("Error accepting connection: {}", e),
        }
    }
}
