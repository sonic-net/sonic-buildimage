use std::collections::HashSet;
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
    xpath_load_errors: Vec<String>,
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
// optional: set to "false" to skip show api (gnmi xpath) probing
const SHOW_API_PROBE_ENV_VAR: &str = "TELEMETRY_WATCHDOG_SHOW_API_PROBE_ENABLED";
// optional: set to "true" to enable serial number probing
const SERIALNUMBER_PROBE_ENV_VAR: &str = "TELEMETRY_WATCHDOG_SERIALNUMBER_PROBE_ENABLED";
const TARGET_NAME_ENV_VAR: &str = "TELEMETRY_WATCHDOG_TARGET_NAME"; // optional override for target_name
const DEFAULT_TARGET_NAME: &str = "server.ndastreaming.ap.gbl";
const DEFAULT_CA_CRT: &str = "/etc/sonic/telemetry/dsmsroot.cer";
const DEFAULT_SERVER_CRT: &str = "/etc/sonic/telemetry/streamingtelemetryserver.cer";
const DEFAULT_SERVER_KEY: &str = "/etc/sonic/telemetry/streamingtelemetryserver.key";
// Max stderr we keep per gnmi_get (bytes) before truncation.
const STDERR_TRUNCATE_LIMIT: usize = 16 * 1024; // 16KB

// Configuration:
// 1. JSON file (/cmd_list.json) optional. Format:
//    {
//        "xpaths": [
//            "reboot-cause/history"
//        ]
//    }
// 2. Environment variable TELEMETRY_WATCHDOG_XPATHS optional. Comma-separated list of xpaths.
// Both sources are merged; duplicates removed (first occurrence kept).
// During probe: for each xpath (after port OK) run command:
//   gnmi_get -xpath_target SHOW -xpath <XPATH> -target_addr 127.0.0.1:<port> -logtostderr \
//     [ -ca <ca> -cert <cert> -key <key> -target_name <name> | -insecure true ]
// Cert paths + client_auth come from Redis (TELEMETRY|certs, TELEMETRY|gnmi).
// client_auth: ONLY explicit Redis value "true" (case-insensitive) enables TLS; anything else -> insecure.
// Any failure (spawn error / non-zero exit) sets HTTP 500; body lists per-xpath results.
// SHOW probe control: env TELEMETRY_WATCHDOG_SHOW_API_PROBE="disable" skips gnmi_get xpaths (default enabled).

fn load_xpath_list() -> (Vec<String>, Vec<String>) {
    let mut set: HashSet<String> = HashSet::new();
    let mut errors: Vec<String> = Vec::new();
    // JSON file format example:
    //   { "xpaths": ["reboot-cause/history", "lldp/neighbors"] }
    match fs::read_to_string(CMD_LIST_JSON) {
        Ok(content) => {
            #[derive(serde::Deserialize)]
            struct JsonCfg { xpaths: Option<Vec<String>> }
            match serde_json::from_str::<JsonCfg>(&content) {
                Ok(cfg) => {
                    if let Some(xs) = cfg.xpaths { for x in xs { if !x.is_empty() { set.insert(x); } } }
                },
                Err(e) => {
                    let msg = format!("Failed to parse {CMD_LIST_JSON}: {e}");
                    errors.push(msg);
                },
            }
        }
        Err(e) => {
            let msg = format!(
                "Could not read {CMD_LIST_JSON}: {e} (will continue with env var only)"
            );
            errors.push(msg);
        }
    }

    if let Ok(env_val) = env::var(XPATH_ENV_VAR) {
        for part in env_val.split(',') {
            let t = part.trim();
            if !t.is_empty() { set.insert(t.to_string()); }
        }
    }

    // Apply blacklist: remove those entries
    if let Ok(blacklist) = env::var(XPATH_ENV_BLACKLIST) {
        if !blacklist.trim().is_empty() {
            for part in blacklist.split(',') {
                let t = part.trim();
                if !t.is_empty() { set.remove(t); }
            }
        }
    }

    (set.into_iter().collect(), errors)
}

struct TelemetrySecurityConfig {
    use_client_auth: bool,
    ca_crt: String,
    server_crt: String,
    server_key: String,
}

// Unified helper: open a Redis connection to DB 4 and fetch one hash field.
// Returns None on any error (client creation, connection, or HGET failure).
fn redis_hget(hash: &str, field: &str) -> Option<String> {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => { eprintln!("Redis client error {hash}.{field}: {e}"); return None; }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => { eprintln!("Redis connection error {hash}.{field}: {e}"); return None; }
    };
    match conn.hget::<_, _, Option<String>>(hash, field) {
        Ok(v) => v,
        Err(e) => { eprintln!("Redis HGET error {hash}.{field}: {e}"); None }
    }
}

fn run_gnmi_for_xpath(
    xpath: &str,
    port: u16,
    sec: &TelemetrySecurityConfig,
    target_name: &str,
    timeout: Duration,
    xpath_target: &str,
) -> CommandResult {
    let addr = format!("127.0.0.1:{port}");
    let start = Instant::now();
    let mut cmd = Command::new(GNMI_BASE_CMD);
    cmd.arg("-xpath_target").arg(xpath_target)
        .arg("-xpath").arg(xpath)
        .arg("-target_addr").arg(addr)
        .arg("-logtostderr")
        .stdin(Stdio::null())
        // We ignore stdout to avoid blocking if gnmi_get prints a lot.
        .stdout(Stdio::null())
        // Keep stderr for error diagnostics.
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
            return CommandResult {
                xpath: xpath.to_string(),
                success: false,
                error: Some(e.to_string()),
                duration_ms: dur,
            };
        }
    };

    let wait_result = {
        let start_wait = Instant::now();
        loop {
            match child.try_wait() {
                Ok(Some(status)) => break Ok(status),
                Ok(None) => {
                    if start_wait.elapsed() >= timeout {
                        let _ = child.kill();
                        let _ = child.wait();
                        break Err(std::io::Error::new(
                            std::io::ErrorKind::TimedOut,
                            "gnmi_get timed out",
                        ));
                    }
                    std::thread::sleep(Duration::from_millis(50));
                }
                Err(e) => break Err(e),
            }
        }
    };
    // After status known, pull stderr (if any).
    let stderr_string = match child.stderr.take() {
        Some(mut stderr_pipe) => {
            use std::io::Read;
            let mut buf = Vec::new();
            // Best-effort read; ignore read errors.
            if let Err(e) = stderr_pipe.read_to_end(&mut buf) {
                eprintln!("Failed to read gnmi_get stderr: {e}");
            }
            String::from_utf8_lossy(&buf).to_string()
        }
        None => String::new(),
    };
    let dur = start.elapsed().as_millis();

    match wait_result {
        Ok(status) => {
            if status.success() {
                CommandResult {
                    xpath: xpath.to_string(),
                    success: true,
                    error: None,
                    duration_ms: dur,
                }
            } else {
                // Possibly large stderr; truncate if huge (optional threshold 16KB).
                let mut truncated = if stderr_string.len() > STDERR_TRUNCATE_LIMIT {
                    format!(
                        "{}...[truncated {} bytes]",
                        &stderr_string[..STDERR_TRUNCATE_LIMIT],
                        stderr_string.len() - STDERR_TRUNCATE_LIMIT
                    )
                } else {
                    stderr_string.clone()
                };
                let exit_code = status.code().map(|c| c.to_string()).unwrap_or_else(|| "unknown".to_string());
                truncated.push_str(&format!(" (exit code: {exit_code})"));
                CommandResult {
                    xpath: xpath.to_string(),
                    success: false,
                    error: Some(truncated),
                    duration_ms: dur,
                }
            }
        },
        Err(e) => {
            eprintln!("Failed to spawn gnmi_get for {}: {}", xpath, e);
            CommandResult {
                xpath: xpath.to_string(),
                success: false,
                error: Some(e.to_string()),
                duration_ms: dur,
            }
        }
    }
}

fn get_security_config() -> TelemetrySecurityConfig {
    // Redis DB 4 hashes:
    // TELEMETRY|certs: ca_crt, server_crt, server_key
    // TELEMETRY|gnmi: client_auth
    let ca_crt = redis_hget("TELEMETRY|certs", "ca_crt")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_CA_CRT.to_string());
    let server_crt = redis_hget("TELEMETRY|certs", "server_crt")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_SERVER_CRT.to_string());
    let server_key = redis_hget("TELEMETRY|certs", "server_key")
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| DEFAULT_SERVER_KEY.to_string());
    let client_auth_opt = redis_hget("TELEMETRY|gnmi", "client_auth");
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
    const DEFAULT_SECS: u64 = 10;
    match env::var(CMD_TIMEOUT_ENV_VAR) {
        Ok(val) => match val.trim().parse::<u64>() {
            Ok(secs) if secs > 0 => Duration::from_secs(secs),
            _ => Duration::from_secs(DEFAULT_SECS),
        },
        Err(_) => Duration::from_secs(DEFAULT_SECS),
    }
}

fn is_show_api_probe_enabled() -> bool {
    match env::var(SHOW_API_PROBE_ENV_VAR) {
        Ok(v) if v.eq_ignore_ascii_case("false") => false,
        _ => true, // default enabled
    }
}

fn is_serialnumber_probe_enabled() -> bool {
    match env::var(SERIALNUMBER_PROBE_ENV_VAR) {
        Ok(v) if v.eq_ignore_ascii_case("true") => true,
        _ => false, // default disabled
    }
}

fn get_gnmi_port() -> u16 {
    match redis_hget("TELEMETRY|gnmi", "port") {
        Some(p) => p.parse::<u16>().unwrap_or_else(|_| {
            eprintln!(
                "Redis: TELEMETRY|gnmi.port not a valid u16: {p}"
            );
            DEFAULT_TELEMETRY_SERVICE_PORT
        }),
        None => {
            eprintln!(
                "Redis: TELEMETRY|gnmi.port missing; defaulting to {}",
                DEFAULT_TELEMETRY_SERVICE_PORT
            );
            DEFAULT_TELEMETRY_SERVICE_PORT
        }
    }
}

// Connects to Redis DB 4 and returns true if the telemetry feature is enabled.
// If Redis is unavailable or the field is missing, default to enabled (fail-open).
fn is_telemetry_enabled() -> bool {
    match redis_hget("FEATURE|telemetry", "state") {
        Some(state) => !state.eq_ignore_ascii_case("disabled"),
        None => {
            eprintln!("Redis: FEATURE|telemetry.state missing; defaulting to enabled");
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
                    let mut load_json_errors: Vec<String> = Vec::new();

                    if !telemetry_enabled {
                        check_port_result = "SKIPPED: feature disabled".to_string();
                    } else {
                        check_port_result = check_telemetry_port();
                        if !check_port_result.starts_with("OK") { http_status = "HTTP/1.1 500 Internal Server Error"; }

                        // Only run xpath commands if port is OK
                        if http_status == "HTTP/1.1 200 OK" {
                            let port = get_gnmi_port();
                            let sec_cfg = get_security_config();
                            let timeout = read_timeout();
                            let target_name = get_target_name();

                            // Check Serial Number
                            if is_serialnumber_probe_enabled() {
                                let xpath_sn = "DEVICE_METADATA/localhost/chassis_serial_number";
                                let res_sn = run_gnmi_for_xpath(&xpath_sn, port, &sec_cfg, &target_name, timeout, "STATE_DB");
                                if !res_sn.success { http_status = "HTTP/1.1 500 Internal Server Error"; }
                                cmd_results.push(res_sn);
                            }

                            // Check SHOW API xpaths
                            if is_show_api_probe_enabled() {
                                let (xpaths, xpath_load_errors) = load_xpath_list();
                                if !xpath_load_errors.is_empty() {
                                    load_json_errors.extend(xpath_load_errors);
                                    http_status = "HTTP/1.1 500 Internal Server Error";
                                }
                                for xp in xpaths {
                                    let res = run_gnmi_for_xpath(&xp, port, &sec_cfg, &target_name, timeout, "SHOW");
                                    if !res.success { http_status = "HTTP/1.1 500 Internal Server Error"; }
                                    cmd_results.push(res);
                                }
                            }
                        }
                    }

                    let status = HealthStatus {
                        check_telemetry_port: check_port_result,
                        xpath_commands: cmd_results,
                        xpath_load_errors: load_json_errors
                    };

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
