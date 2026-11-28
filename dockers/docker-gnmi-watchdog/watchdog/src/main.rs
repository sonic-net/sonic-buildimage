use std::collections::{BTreeSet, HashMap};
use std::env;
use std::fs;
use std::io::{BufRead, BufReader, Read, Write};
use std::net::TcpListener;
use std::os::unix::fs::FileTypeExt;
use std::os::unix::net::UnixStream;
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

use redis::Commands;
use serde::Serialize;

const WATCHDOG_PORT: u16 = 51000;

// Same default as gnmi-native.sh when GNMI|gnmi.port is not configured.
const DEFAULT_GNMI_PORT: u16 = 8080;

#[derive(Serialize)]
struct HealthStatus {
    gnmi_status: String,
    // Set when the watchdog cannot authenticate to the server, so gNMI probes were not run.
    #[serde(skip_serializing_if = "Option::is_none")]
    probes_skipped: Option<String>,
    // Set when the SHOW probe ran but no xpath succeeded.
    #[serde(skip_serializing_if = "Option::is_none")]
    show_probe_error: Option<String>,
    xpath_commands: Vec<CommandResult>,
    xpath_load_errors: Vec<String>,
}

#[derive(Serialize, Clone, Debug)]
struct CommandResult {
    xpath: String,
    success: bool,
    error: Option<String>,
    // The server answered gRPC NotFound: this gnmi build doesn't implement the xpath.
    unsupported: bool,
    duration_ms: u128,
    // The server answered gRPC Unauthenticated.
    #[serde(skip)]
    unauthenticated: bool,
}

const CMD_LIST_JSON: &str = "/cmd_list.json"; // absolute path inside container
const XPATH_ENV_VAR: &str = "GNMI_WATCHDOG_XPATHS"; // comma-separated list
const XPATH_ENV_BLACKLIST: &str = "GNMI_WATCHDOG_XPATHS_BLACKLIST"; // comma-separated list to exclude
const CMD_TIMEOUT_ENV_VAR: &str = "GNMI_WATCHDOG_CMD_TIMEOUT_SECS"; // per-command timeout seconds
const GNMI_BASE_CMD: &str = "gnmi_get"; // assumed in PATH
// gnmi serves this socket next to TCP; mounted from the host.
const DEFAULT_UNIX_SOCKET: &str = "/var/run/gnmi/gnmi.sock";
const UNIX_SOCKET_ENV_VAR: &str = "GNMI_WATCHDOG_UNIX_SOCKET";
// With --pid=host, PID 1's view is the host network namespace whatever the container's own is.
const PROC_NET_DIRS: [&str; 2] = ["/proc/1/net", "/proc/net"];
const TCP_LISTEN_STATE: &str = "0A";
// optional: set to "false" to skip show api (gnmi xpath) probing
const SHOW_API_PROBE_ENV_VAR: &str = "GNMI_WATCHDOG_SHOW_API_PROBE_ENABLED";
// optional: set to "true" to enable serial number probing
const SERIALNUMBER_PROBE_ENV_VAR: &str = "GNMI_WATCHDOG_SERIALNUMBER_PROBE_ENABLED";
// optional: set to "true" to enable the client certificate and config apply-patch probes
const CERT_PROBE_ENV_VAR: &str = "GNMI_WATCHDOG_CERT_PROBE_ENABLED";
const TARGET_NAME_ENV_VAR: &str = "GNMI_WATCHDOG_TARGET_NAME";
const CA_CRT_ENV_VAR: &str = "GNMI_WATCHDOG_CA_CRT";
const SERVER_CRT_ENV_VAR: &str = "GNMI_WATCHDOG_SERVER_CRT";
const SERVER_KEY_ENV_VAR: &str = "GNMI_WATCHDOG_SERVER_KEY";
const BAD_CA_ENV_VAR: &str = "GNMI_WATCHDOG_BAD_CA";
const BAD_CERT_ENV_VAR: &str = "GNMI_WATCHDOG_BAD_CERT";
const BAD_KEY_ENV_VAR: &str = "GNMI_WATCHDOG_BAD_KEY";
const BAD_CNAME_ENV_VAR: &str = "GNMI_WATCHDOG_BAD_CNAME";
const GOOD_CA_ENV_VAR: &str = "GNMI_WATCHDOG_GOOD_CA";
const GOOD_CERT_ENV_VAR: &str = "GNMI_WATCHDOG_GOOD_CERT";
const GOOD_KEY_ENV_VAR: &str = "GNMI_WATCHDOG_GOOD_KEY";
const GOOD_CNAME_ENV_VAR: &str = "GNMI_WATCHDOG_GOOD_CNAME";

const DEFAULT_TARGET_NAME: &str = "default-target-name";
const DEFAULT_CA_CRT: &str = "/path/to/ca.crt";
const DEFAULT_SERVER_CRT: &str = "/path/to/server.crt";
const DEFAULT_SERVER_KEY: &str = "/path/to/server.key";
// Max stderr we keep per probe (bytes) before truncation.
const STDERR_TRUNCATE_LIMIT: usize = 16 * 1024; // 16KB

// BAD (expected fail) probe
const DEFAULT_BAD_CA: &str = "/path/to/bad-ca.crt";
const DEFAULT_BAD_CERT: &str = "/path/to/bad-cert.crt";
const DEFAULT_BAD_KEY: &str = "/path/to/bad-cert.key";
const DEFAULT_BAD_CNAME: &str = "bad-cname.example.com";

// GOOD (expected success) probe
const DEFAULT_GOOD_CA: &str = "/path/to/good-ca.crt";
const DEFAULT_GOOD_CERT: &str = "/path/to/good-cert.crt";
const DEFAULT_GOOD_KEY: &str = "/path/to/good-cert.key";
const DEFAULT_GOOD_CNAME: &str = "good-cname.example.com";

const APPLY_PATCH_PROBE_NAME: &str = "config-apply-patch-probe";
const SERIAL_NUMBER_XPATH: &str = "DEVICE_METADATA/localhost/chassis_serial_number";
const CERT_PROBE_XPATH: &str = "reboot-cause/history";

const HTTP_OK: &str = "HTTP/1.1 200 OK";
const HTTP_ERROR: &str = "HTTP/1.1 500 Internal Server Error";

// Configuration:
// 1. JSON file (/cmd_list.json) optional. Format:
//    {
//        "xpaths": [
//            "reboot-cause/history"
//        ]
//    }
// 2. Environment variable GNMI_WATCHDOG_XPATHS optional. Comma-separated list of xpaths.
// Both sources are merged; duplicates removed.
// During probe: for each xpath (after the listener check) run command:
//   gnmi_get -xpath_target SHOW -xpath <XPATH> -target_addr unix://<socket> -logtostderr -notls
// gnmi skips authentication for unix-socket peers, so no TLS or client cert is needed. If the
// socket is missing, or the server rejects it as Unauthenticated (gnmi builds that authenticate
// unix peers), fall back to TCP:
//   gnmi_get ... -target_addr 127.0.0.1:<port> -logtostderr \
//     [ -ca <ca> -cert <cert> -key <key> -target_name <name> | -insecure true | -notls ]
// The TCP TLS mode mirrors gnmi-native.sh (see server_tls_mode). Port, cert paths and user_auth
// come from CONFIG_DB (GNMI|gnmi, GNMI|certs, DEVICE_METADATA|x509, DEVICE_METADATA|localhost).
// Any failure (spawn error / non-zero exit / timeout) sets HTTP 500; body lists per-xpath results.
// A SHOW xpath the server reports as gRPC NotFound is marked unsupported and does not fail the
// check, but at least one SHOW xpath must succeed.

#[derive(Debug, PartialEq, Clone, Copy)]
enum TlsMode {
    NoTls,
    Tls { client_cert: bool },
}

struct GnmiSecurityConfig {
    tls: TlsMode,
    ca_crt: String,
    server_crt: String,
    server_key: String,
}

type Hash = HashMap<String, String>;

fn field<'a>(hash: &'a Hash, name: &str) -> Option<&'a str> {
    hash.get(name).map(String::as_str).filter(|v| !v.is_empty())
}

// The TLS config gnmi-native.sh hands the server: GNMI|certs if present, else DEVICE_METADATA|x509.
fn server_tls_source<'a>(certs: &'a Hash, x509: &'a Hash) -> Option<&'a Hash> {
    if !certs.is_empty() {
        Some(certs)
    } else if !x509.is_empty() {
        Some(x509)
    } else {
        None
    }
}

// gnmi-native.sh passes --client_auth $USER_AUTH, defaulting an unset or "null" user_auth to "cert".
fn effective_user_auth(gnmi: &Hash) -> &str {
    match field(gnmi, "user_auth") {
        Some(v) if v != "null" => v,
        _ => "cert",
    }
}

// Mirrors gnmi-native.sh:
// - SmartSwitch DPU (DEVICE_METADATA|localhost type=SmartSwitchDPU or switch_type=dpu) without a
//   TLS source carrying server_crt and server_key: ephemeral TLS (--insecure), probed without a
//   client cert.
// - Otherwise GNMI|certs or DEVICE_METADATA|x509 present: TLS. client_auth only toggles
//   --allow_no_client_auth at the TLS layer; gRPC authentication follows user_auth, so the
//   watchdog presents a client cert when user_auth includes "cert". Any other user_auth
//   (e.g. password, jwt) needs credentials the watchdog doesn't have: Err(reason).
// - Neither present: --noTLS; the server answers -notls requests without a client cert.
fn server_tls_mode(certs: &Hash, x509: &Hash, gnmi: &Hash, localhost: &Hash) -> Result<TlsMode, String> {
    let is_dpu = field(localhost, "type") == Some("SmartSwitchDPU")
        || field(localhost, "switch_type") == Some("dpu");
    let source = server_tls_source(certs, x509);
    if is_dpu {
        let has_keypair = source
            .map(|s| field(s, "server_crt").is_some() && field(s, "server_key").is_some())
            .unwrap_or(false);
        if !has_keypair {
            return Ok(TlsMode::Tls { client_cert: false });
        }
    }
    if source.is_none() {
        return Ok(TlsMode::NoTls);
    }
    let user_auth = effective_user_auth(gnmi);
    if user_auth.split(',').any(|m| m.trim() == "cert") {
        Ok(TlsMode::Tls { client_cert: true })
    } else {
        Err(format!("user_auth={user_auth} not supported by watchdog"))
    }
}

fn non_empty(v: Option<String>) -> Option<String> {
    v.map(|s| s.trim().to_string()).filter(|s| !s.is_empty())
}

fn env_or_default(env_var: &str, default: &str) -> String {
    non_empty(env::var(env_var).ok()).unwrap_or_else(|| default.to_string())
}

fn env_flag(value: Option<&str>, default: bool) -> bool {
    match value.map(str::trim) {
        Some(v) if v.eq_ignore_ascii_case("true") => true,
        Some(v) if v.eq_ignore_ascii_case("false") => false,
        _ => default,
    }
}

fn env_flag_var(env_var: &str, default: bool) -> bool {
    env_flag(env::var(env_var).ok().as_deref(), default)
}

/// Truncate `s` to at most `max_bytes`, walking back to the nearest UTF-8
/// char boundary so we never panic on multi-byte sequences.
fn truncate_on_char_boundary(s: &str, max_bytes: usize) -> &str {
    if s.len() <= max_bytes {
        return s;
    }
    let mut end = max_bytes;
    while end > 0 && !s.is_char_boundary(end) {
        end -= 1;
    }
    &s[..end]
}

fn summarize_stderr(stderr: &str) -> String {
    let trimmed = stderr.trim();
    if trimmed.len() > STDERR_TRUNCATE_LIMIT {
        let head = truncate_on_char_boundary(trimmed, STDERR_TRUNCATE_LIMIT);
        format!("{}...[truncated {} bytes]", head, trimmed.len() - head.len())
    } else {
        trimmed.to_string()
    }
}

fn merge_xpaths(
    json_content: Result<String, String>,
    extra: Option<&str>,
    blacklist: Option<&str>,
) -> (Vec<String>, Vec<String>) {
    let mut set: BTreeSet<String> = BTreeSet::new();
    let mut errors: Vec<String> = Vec::new();
    match json_content {
        Ok(content) => {
            #[derive(serde::Deserialize)]
            struct JsonCfg {
                xpaths: Option<Vec<String>>,
            }
            match serde_json::from_str::<JsonCfg>(&content) {
                Ok(cfg) => {
                    for x in cfg.xpaths.unwrap_or_default() {
                        if !x.is_empty() {
                            set.insert(x);
                        }
                    }
                }
                Err(e) => errors.push(format!("Failed to parse {CMD_LIST_JSON}: {e}")),
            }
        }
        Err(e) => errors.push(e),
    }

    for part in extra.unwrap_or("").split(',') {
        let t = part.trim();
        if !t.is_empty() {
            set.insert(t.to_string());
        }
    }

    for part in blacklist.unwrap_or("").split(',') {
        set.remove(part.trim());
    }

    (set.into_iter().collect(), errors)
}

fn load_xpath_list() -> (Vec<String>, Vec<String>) {
    let json_content = fs::read_to_string(CMD_LIST_JSON).map_err(|e| {
        format!("Could not read {CMD_LIST_JSON}: {e} (will continue with env var only)")
    });
    merge_xpaths(
        json_content,
        env::var(XPATH_ENV_VAR).ok().as_deref(),
        env::var(XPATH_ENV_BLACKLIST).ok().as_deref(),
    )
}

// Open a Redis connection to CONFIG_DB (DB 4) and fetch one hash field.
// Returns None on any error (client creation, connection, or HGET failure).
fn redis_hget(hash: &str, field: &str) -> Option<String> {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error {hash}.{field}: {e}");
            return None;
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error {hash}.{field}: {e}");
            return None;
        }
    };
    match conn.hget::<_, _, Option<String>>(hash, field) {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Redis HGET error {hash}.{field}: {e}");
            None
        }
    }
}

fn redis_hgetall(hash: &str) -> Hash {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error {hash}: {e}");
            return Hash::new();
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error {hash}: {e}");
            return Hash::new();
        }
    };
    conn.hgetall::<_, Hash>(hash).unwrap_or_else(|e| {
        eprintln!("Redis HGETALL error {hash}: {e}");
        Hash::new()
    })
}

fn is_grpc_not_found(stderr: &str) -> bool {
    stderr.contains("code = NotFound") || stderr.contains("not found in clientTrie")
}

// Run `cmd` with an optional stdin payload, killing it after `timeout`.
// stderr is drained on a separate thread so a chatty child cannot block on a full pipe.
// With `allow_unsupported`, a gRPC NotFound failure is flagged as unsupported.
fn run_probe(
    name: &str,
    mut cmd: Command,
    stdin_data: Option<&[u8]>,
    timeout: Duration,
    allow_unsupported: bool,
) -> CommandResult {
    let start = Instant::now();
    let result = |success: bool, error: Option<String>| CommandResult {
        xpath: name.to_string(),
        success,
        error,
        unsupported: false,
        duration_ms: start.elapsed().as_millis(),
        unauthenticated: false,
    };

    cmd.stdin(if stdin_data.is_some() { Stdio::piped() } else { Stdio::null() })
        // stdout is ignored so a large response cannot block the child.
        .stdout(Stdio::null())
        .stderr(Stdio::piped());

    let mut child = match cmd.spawn() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Failed to spawn probe {name}: {e}");
            return result(false, Some(e.to_string()));
        }
    };

    if let (Some(data), Some(mut stdin_pipe)) = (stdin_data, child.stdin.take()) {
        if let Err(e) = stdin_pipe.write_all(data) {
            eprintln!("Failed to write stdin for probe {name}: {e}");
            let _ = child.kill();
            let _ = child.wait();
            return result(false, Some(format!("stdin write failed: {e}")));
        }
        // stdin_pipe is dropped here, sending EOF
    }

    let stderr_handle = child.stderr.take().map(|mut pipe| {
        std::thread::spawn(move || {
            let mut buf = Vec::new();
            let _ = pipe.read_to_end(&mut buf);
            String::from_utf8_lossy(&buf).to_string()
        })
    });

    let wait_result = loop {
        match child.try_wait() {
            Ok(Some(status)) => break Ok(status),
            Ok(None) => {
                if start.elapsed() >= timeout {
                    let _ = child.kill();
                    let _ = child.wait();
                    break Err(std::io::Error::new(
                        std::io::ErrorKind::TimedOut,
                        format!("{name} timed out"),
                    ));
                }
                std::thread::sleep(Duration::from_millis(50));
            }
            Err(e) => break Err(e),
        }
    };

    // After a timeout kill, a grandchild may still hold stderr open; don't block on it.
    let raw_stderr = stderr_handle
        .filter(|h| wait_result.is_ok() || h.is_finished())
        .and_then(|h| h.join().ok())
        .unwrap_or_default();
    let stderr = summarize_stderr(&raw_stderr);

    match wait_result {
        Ok(status) if status.success() => result(true, None),
        Ok(status) => {
            let exit_code = status
                .code()
                .map(|c| c.to_string())
                .unwrap_or_else(|| "unknown".to_string());
            let mut msg = String::new();
            if !stderr.is_empty() {
                msg.push_str(&format!("stderr: {stderr}; "));
            }
            msg.push_str(&format!("exit code: {exit_code}"));
            let mut res = result(false, Some(msg));
            res.unsupported = allow_unsupported && is_grpc_not_found(&raw_stderr);
            res.unauthenticated = raw_stderr.contains("code = Unauthenticated");
            res
        }
        Err(e) => {
            eprintln!("Probe {name} wait/timeout failed: {e}");
            let mut msg = format!("error: {e}");
            if !stderr.is_empty() {
                msg.push_str(&format!("; stderr: {stderr}"));
            }
            result(false, Some(msg))
        }
    }
}

fn gnmi_get_unix_command(program: &str, socket: &str, xpath: &str, xpath_target: &str) -> Command {
    let mut cmd = Command::new(program);
    cmd.arg("-xpath_target").arg(xpath_target)
        .arg("-xpath").arg(xpath)
        .arg("-target_addr").arg(format!("unix://{socket}"))
        .arg("-logtostderr")
        .arg("-notls");
    cmd
}

fn gnmi_get_command(
    program: &str,
    xpath: &str,
    port: u16,
    sec: &GnmiSecurityConfig,
    target_name: &str,
    xpath_target: &str,
) -> Command {
    let mut cmd = Command::new(program);
    cmd.arg("-xpath_target").arg(xpath_target)
        .arg("-xpath").arg(xpath)
        .arg("-target_addr").arg(format!("127.0.0.1:{port}"))
        .arg("-logtostderr");
    match sec.tls {
        TlsMode::Tls { client_cert: true } => {
            cmd.arg("-ca").arg(&sec.ca_crt)
                .arg("-cert").arg(&sec.server_crt)
                .arg("-key").arg(&sec.server_key)
                .arg("-target_name").arg(target_name);
        }
        TlsMode::Tls { client_cert: false } => {
            cmd.arg("-insecure").arg("true");
        }
        TlsMode::NoTls => {
            cmd.arg("-notls");
        }
    }
    cmd
}

fn run_gnmi_for_xpath(
    xpath: &str,
    port: u16,
    sec: &GnmiSecurityConfig,
    target_name: &str,
    timeout: Duration,
    xpath_target: &str,
    allow_unsupported: bool,
) -> CommandResult {
    let cmd = gnmi_get_command(GNMI_BASE_CMD, xpath, port, sec, target_name, xpath_target);
    run_probe(xpath, cmd, None, timeout, allow_unsupported)
}

fn usable_socket(path: &str) -> Option<String> {
    let is_socket = fs::metadata(path).map(|m| m.file_type().is_socket()).unwrap_or(false);
    is_socket.then(|| path.to_string())
}

// A unix-socket probe the server rejected as Unauthenticated: retry over TCP.
fn unix_rejected(res: &CommandResult) -> bool {
    !res.success && res.unauthenticated
}

// Runs gnmi_get over the unix socket while it is usable, else over TCP.
struct Prober {
    program: String,
    socket: Option<String>,
    // Err(reason) when the watchdog has no credentials the TCP listener accepts.
    tcp: Result<(u16, GnmiSecurityConfig), String>,
    target_name: String,
    timeout: Duration,
}

impl Prober {
    fn get(&mut self, xpath: &str, xpath_target: &str, allow_unsupported: bool) -> Result<CommandResult, String> {
        if let Some(socket) = &self.socket {
            let cmd = gnmi_get_unix_command(&self.program, socket, xpath, xpath_target);
            let res = run_probe(xpath, cmd, None, self.timeout, allow_unsupported);
            if !unix_rejected(&res) {
                return Ok(res);
            }
            eprintln!("gNMI unix socket {socket} requires authentication; falling back to TCP");
            self.socket = None;
        }
        match &self.tcp {
            Ok((port, sec)) => {
                let cmd = gnmi_get_command(&self.program, xpath, *port, sec, &self.target_name, xpath_target);
                Ok(run_probe(xpath, cmd, None, self.timeout, allow_unsupported))
            }
            Err(reason) => Err(reason.clone()),
        }
    }
}

// Validates ConfigDB YANG integrity: `echo '[]' | sudo config apply-patch /dev/stdin`.
fn run_yang_config_apply_patch_probe(timeout: Duration) -> CommandResult {
    let mut cmd = Command::new("sudo");
    cmd.arg("-n").arg("config").arg("apply-patch").arg("/dev/stdin");
    run_probe(APPLY_PATCH_PROBE_NAME, cmd, Some(b"[]\n"), timeout, false)
}

// Client cert paths are the server's own TLS source (GNMI|certs, else DEVICE_METADATA|x509),
// falling back to env vars.
fn security_config(certs: &Hash, x509: &Hash, gnmi: &Hash, localhost: &Hash) -> Result<GnmiSecurityConfig, String> {
    let empty = Hash::new();
    let source = server_tls_source(certs, x509).unwrap_or(&empty);
    let cert = |name: &str, env_var: &str, default: &str| {
        non_empty(field(source, name).map(str::to_string)).unwrap_or_else(|| env_or_default(env_var, default))
    };
    Ok(GnmiSecurityConfig {
        tls: server_tls_mode(certs, x509, gnmi, localhost)?,
        ca_crt: cert("ca_crt", CA_CRT_ENV_VAR, DEFAULT_CA_CRT),
        server_crt: cert("server_crt", SERVER_CRT_ENV_VAR, DEFAULT_SERVER_CRT),
        server_key: cert("server_key", SERVER_KEY_ENV_VAR, DEFAULT_SERVER_KEY),
    })
}

fn read_timeout(value: Option<&str>) -> Duration {
    const DEFAULT_SECS: u64 = 10;
    match value.map(|v| v.trim().parse::<u64>()) {
        Some(Ok(secs)) if secs > 0 => Duration::from_secs(secs),
        _ => Duration::from_secs(DEFAULT_SECS),
    }
}

fn parse_gnmi_port(value: Option<&str>) -> u16 {
    match value {
        Some(p) => p.trim().parse::<u16>().unwrap_or_else(|_| {
            eprintln!("CONFIG_DB: GNMI|gnmi.port not a valid u16: {p}");
            DEFAULT_GNMI_PORT
        }),
        None => {
            eprintln!("CONFIG_DB: GNMI|gnmi.port missing; defaulting to {DEFAULT_GNMI_PORT}");
            DEFAULT_GNMI_PORT
        }
    }
}

// Fail-open: a missing or unreadable FEATURE|gnmi.state counts as enabled.
fn feature_enabled(state: Option<&str>) -> bool {
    match state {
        Some(s) => {
            let s = s.trim();
            !s.eq_ignore_ascii_case("disabled") && !s.eq_ignore_ascii_case("always_disabled")
        }
        None => true,
    }
}

// True if a /proc/net/tcp or tcp6 table has a LISTEN socket on `port`, on any address. A
// listener bound in a VRF (l3mdev, same namespace) is listed too, unlike a 127.0.0.1 connect.
fn tcp_listening(table: &str, port: u16) -> bool {
    table.lines().skip(1).any(|line| {
        let cols: Vec<&str> = line.split_whitespace().collect();
        let local_port = cols
            .get(1)
            .and_then(|local| local.rsplit(':').next())
            .and_then(|hex| u16::from_str_radix(hex, 16).ok());
        local_port == Some(port) && cols.get(3) == Some(&TCP_LISTEN_STATE)
    })
}

fn read_tcp_tables() -> Result<String, String> {
    let mut last_err = String::new();
    for dir in PROC_NET_DIRS {
        match fs::read_to_string(format!("{dir}/tcp")) {
            // tcp6 is absent when IPv6 is disabled.
            Ok(v4) => return Ok(v4 + &fs::read_to_string(format!("{dir}/tcp6")).unwrap_or_default()),
            Err(e) => last_err = format!("cannot read {dir}/tcp: {e}"),
        }
    }
    Err(last_err)
}

// In uds-only mode (GNMI_LISTENER_MODE=uds-only) gnmi has no TCP listener; with no port
// configured, a live unix socket counts as the listener being up.
fn listener_status(
    tables: Result<String, String>,
    port: u16,
    port_configured: bool,
    socket_live: impl FnOnce() -> bool,
) -> String {
    let tables = match tables {
        Ok(t) => t,
        Err(e) => return format!("ERROR: {e}"),
    };
    if tcp_listening(&tables, port) {
        "OK".to_string()
    } else if !port_configured && socket_live() {
        "OK (unix socket)".to_string()
    } else {
        format!("ERROR: no TCP listener on port {port}")
    }
}

// The BAD-cert probe passes when the server rejects the request and fails when it is accepted.
fn expect_rejected(mut res: CommandResult) -> CommandResult {
    if res.success {
        res.success = false;
        res.error = Some("Expected FAILURE with BAD cert but command SUCCEEDED".to_string());
    } else {
        res.success = true;
    }
    res
}

fn show_probe_error(results: &[CommandResult]) -> Option<String> {
    if results.is_empty() || results.iter().any(|r| r.success) {
        return None;
    }
    let unsupported = results.iter().filter(|r| r.unsupported).count();
    Some(format!(
        "no supported SHOW xpaths: {unsupported} unsupported, {} failed",
        results.len() - unsupported
    ))
}

fn run_health_checks() -> (bool, HealthStatus) {
    let mut ok = true;
    let mut cmd_results: Vec<CommandResult> = Vec::new();
    let mut load_errors: Vec<String> = Vec::new();

    let state = redis_hget("FEATURE|gnmi", "state");
    if state.is_none() {
        eprintln!("CONFIG_DB: FEATURE|gnmi.state missing; defaulting to enabled");
    }
    if !feature_enabled(state.as_deref()) {
        let status = HealthStatus {
            gnmi_status: "SKIPPED: feature disabled".to_string(),
            probes_skipped: None,
            show_probe_error: None,
            xpath_commands: cmd_results,
            xpath_load_errors: load_errors,
        };
        return (true, status);
    }

    let gnmi = redis_hgetall("GNMI|gnmi");
    let port = parse_gnmi_port(field(&gnmi, "port"));
    let socket = usable_socket(&env_or_default(UNIX_SOCKET_ENV_VAR, DEFAULT_UNIX_SOCKET));
    let gnmi_status = listener_status(read_tcp_tables(), port, field(&gnmi, "port").is_some(), || {
        socket.as_deref().map(|s| UnixStream::connect(s).is_ok()).unwrap_or(false)
    });
    // Only run xpath probes if the listener is up
    if !gnmi_status.starts_with("OK") {
        let status = HealthStatus {
            gnmi_status,
            probes_skipped: None,
            show_probe_error: None,
            xpath_commands: cmd_results,
            xpath_load_errors: load_errors,
        };
        return (false, status);
    }

    let sec_cfg = security_config(
        &redis_hgetall("GNMI|certs"),
        &redis_hgetall("DEVICE_METADATA|x509"),
        &gnmi,
        &redis_hgetall("DEVICE_METADATA|localhost"),
    );
    let timeout = read_timeout(env::var(CMD_TIMEOUT_ENV_VAR).ok().as_deref());
    let target_name = env_or_default(TARGET_NAME_ENV_VAR, DEFAULT_TARGET_NAME);
    let cert_probe_enabled = env_flag_var(CERT_PROBE_ENV_VAR, false);

    if cert_probe_enabled {
        let res_patch = run_yang_config_apply_patch_probe(Duration::from_secs(30));
        ok &= res_patch.success;
        cmd_results.push(res_patch);
    }

    // Certificate probes: a BAD client cert must be rejected and a GOOD one accepted.
    // They exercise TCP cert auth, so only run when the TCP listener expects a client cert.
    if cert_probe_enabled && matches!(&sec_cfg, Ok(cfg) if cfg.tls == TlsMode::Tls { client_cert: true }) {
        let bad_sec = GnmiSecurityConfig {
            tls: TlsMode::Tls { client_cert: true },
            ca_crt: env_or_default(BAD_CA_ENV_VAR, DEFAULT_BAD_CA),
            server_crt: env_or_default(BAD_CERT_ENV_VAR, DEFAULT_BAD_CERT),
            server_key: env_or_default(BAD_KEY_ENV_VAR, DEFAULT_BAD_KEY),
        };
        let bad_cname = env_or_default(BAD_CNAME_ENV_VAR, DEFAULT_BAD_CNAME);
        let res_bad = expect_rejected(run_gnmi_for_xpath(CERT_PROBE_XPATH, port, &bad_sec, &bad_cname, timeout, "SHOW", false));
        ok &= res_bad.success;
        cmd_results.push(res_bad);

        let good_sec = GnmiSecurityConfig {
            tls: TlsMode::Tls { client_cert: true },
            ca_crt: env_or_default(GOOD_CA_ENV_VAR, DEFAULT_GOOD_CA),
            server_crt: env_or_default(GOOD_CERT_ENV_VAR, DEFAULT_GOOD_CERT),
            server_key: env_or_default(GOOD_KEY_ENV_VAR, DEFAULT_GOOD_KEY),
        };
        let good_cname = env_or_default(GOOD_CNAME_ENV_VAR, DEFAULT_GOOD_CNAME);
        let res_good = run_gnmi_for_xpath(CERT_PROBE_XPATH, port, &good_sec, &good_cname, timeout, "SHOW", false);
        ok &= res_good.success;
        cmd_results.push(res_good);
    }

    let mut prober = Prober {
        program: GNMI_BASE_CMD.to_string(),
        socket,
        tcp: sec_cfg.map(|cfg| (port, cfg)),
        target_name,
        timeout,
    };
    // Without usable credentials every gNMI probe would be rejected by a healthy server, and a
    // false 500 would get it restarted; report the gap instead of failing.
    let mut probes_skipped = None;
    let mut show_error = None;

    if env_flag_var(SERIALNUMBER_PROBE_ENV_VAR, false) {
        match prober.get(SERIAL_NUMBER_XPATH, "STATE_DB", false) {
            Ok(res_sn) => {
                ok &= res_sn.success;
                cmd_results.push(res_sn);
            }
            Err(reason) => probes_skipped = Some(reason),
        }
    }

    if probes_skipped.is_none() && env_flag_var(SHOW_API_PROBE_ENV_VAR, true) {
        let (xpaths, xpath_load_errors) = load_xpath_list();
        if !xpath_load_errors.is_empty() {
            load_errors.extend(xpath_load_errors);
            ok = false;
        }
        let mut show_results = Vec::new();
        for xp in xpaths {
            match prober.get(&xp, "SHOW", true) {
                Ok(res) => {
                    ok &= res.success || res.unsupported;
                    show_results.push(res);
                }
                Err(reason) => {
                    probes_skipped = Some(reason);
                    break;
                }
            }
        }
        if probes_skipped.is_none() {
            show_error = show_probe_error(&show_results);
            ok &= show_error.is_none();
        }
        cmd_results.extend(show_results);
    }
    if let Some(reason) = &probes_skipped {
        eprintln!("Skipping gNMI probes: {reason}");
    }

    let status = HealthStatus {
        gnmi_status,
        probes_skipped,
        show_probe_error: show_error,
        xpath_commands: cmd_results,
        xpath_load_errors: load_errors,
    };
    (ok, status)
}

fn http_response(ok: bool, status: &HealthStatus) -> String {
    let status_line = if ok { HTTP_OK } else { HTTP_ERROR };
    let json_body = serde_json::to_string(status).unwrap();
    format!(
        "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
        json_body.len(),
        json_body
    )
}

fn main() {
    let listener = TcpListener::bind(("127.0.0.1", WATCHDOG_PORT))
        .unwrap_or_else(|e| panic!("Failed to bind to 127.0.0.1:{WATCHDOG_PORT}: {e}"));
    println!("Watchdog HTTP server running on http://127.0.0.1:{WATCHDOG_PORT}");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut request_line = String::new();
                if let Ok(_) = BufReader::new(&stream).read_line(&mut request_line) {
                    println!("Received request: {}", request_line.trim_end());
                }

                let (ok, status) = run_health_checks();
                if let Err(e) = stream.write_all(http_response(ok, &status).as_bytes()) {
                    eprintln!("Failed to write response: {}", e);
                }
            }
            Err(e) => eprintln!("Error accepting connection: {}", e),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Writing a stub script while another thread forks lets the child inherit the write fd until
    // it execs, so exec'ing the stub can fail with ETXTBSY. Serialize every test that writes a stub
    // or spawns a process.
    static STUB_LOCK: std::sync::Mutex<()> = std::sync::Mutex::new(());

    fn stub_lock() -> std::sync::MutexGuard<'static, ()> {
        STUB_LOCK.lock().unwrap_or_else(|e| e.into_inner())
    }

    fn sh(script: &str) -> Command {
        let mut cmd = Command::new("sh");
        cmd.arg("-c").arg(script);
        cmd
    }

    fn args(cmd: &Command) -> Vec<String> {
        cmd.get_args().map(|a| a.to_string_lossy().to_string()).collect()
    }

    #[test]
    fn shipped_cmd_list_parses() {
        let (xpaths, errors) = merge_xpaths(Ok(include_str!("cmd_list.json").to_string()), None, None);
        assert!(errors.is_empty(), "{errors:?}");
        assert!(xpaths.contains(&"reboot-cause/history".to_string()));
    }

    #[test]
    fn merge_xpaths_merges_dedups_and_blacklists() {
        let json = r#"{"xpaths": ["a", "b", ""]}"#.to_string();
        let (xpaths, errors) = merge_xpaths(Ok(json), Some(" b, c ,,"), Some("a"));
        assert!(errors.is_empty());
        assert_eq!(xpaths, vec!["b", "c"]);
    }

    #[test]
    fn merge_xpaths_reports_load_errors() {
        let (xpaths, errors) = merge_xpaths(Err("missing".to_string()), Some("x"), None);
        assert_eq!(xpaths, vec!["x"]);
        assert_eq!(errors, vec!["missing"]);

        let (xpaths, errors) = merge_xpaths(Ok("not json".to_string()), None, None);
        assert!(xpaths.is_empty());
        assert_eq!(errors.len(), 1);
        assert!(errors[0].starts_with("Failed to parse"));
    }

    #[test]
    fn port_parsing_falls_back_to_gnmi_default() {
        assert_eq!(parse_gnmi_port(Some("50052")), 50052);
        assert_eq!(parse_gnmi_port(Some(" 50051 ")), 50051);
        assert_eq!(parse_gnmi_port(Some("abc")), DEFAULT_GNMI_PORT);
        assert_eq!(parse_gnmi_port(Some("70000")), DEFAULT_GNMI_PORT);
        assert_eq!(parse_gnmi_port(None), DEFAULT_GNMI_PORT);
    }

    fn hash(pairs: &[(&str, &str)]) -> Hash {
        pairs.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect()
    }

    const NO_TLS: TlsMode = TlsMode::NoTls;
    const CLIENT_CERT: TlsMode = TlsMode::Tls { client_cert: true };
    const NO_CLIENT_CERT: TlsMode = TlsMode::Tls { client_cert: false };

    fn gnmi_table(client_auth: Option<&str>, user_auth: Option<&str>) -> Hash {
        let mut h = hash(&[("port", "50052")]);
        if let Some(v) = client_auth {
            h.insert("client_auth".into(), v.into());
        }
        if let Some(v) = user_auth {
            h.insert("user_auth".into(), v.into());
        }
        h
    }

    const CLIENT_AUTHS: [Option<&str>; 3] = [None, Some("true"), Some("false")];

    #[test]
    fn tls_mode_without_certs_is_notls() {
        let none = Hash::new();
        for ca in CLIENT_AUTHS {
            for ua in [None, Some("cert"), Some("password"), Some("password,cert")] {
                let gnmi = gnmi_table(ca, ua);
                assert_eq!(server_tls_mode(&none, &none, &gnmi, &none), Ok(NO_TLS), "{ca:?} {ua:?}");
            }
        }
    }

    #[test]
    fn tls_mode_follows_user_auth_not_client_auth() {
        let none = Hash::new();
        let certs = hash(&[("server_crt", "/c"), ("server_key", "/k"), ("ca_crt", "/ca")]);
        for ca in CLIENT_AUTHS {
            for ua in [None, Some("cert"), Some("password,cert"), Some("null"), Some("")] {
                let gnmi = gnmi_table(ca, ua);
                assert_eq!(server_tls_mode(&certs, &none, &gnmi, &none), Ok(CLIENT_CERT), "{ca:?} {ua:?}");
            }
            let gnmi = gnmi_table(ca, Some("password"));
            assert_eq!(
                server_tls_mode(&certs, &none, &gnmi, &none),
                Err("user_auth=password not supported by watchdog".to_string()),
                "{ca:?}"
            );
        }
        let jwt = gnmi_table(None, Some("jwt,password"));
        assert!(server_tls_mode(&certs, &none, &jwt, &none).is_err());
        assert_eq!(server_tls_mode(&certs, &none, &none, &none), Ok(CLIENT_CERT));
    }

    #[test]
    fn tls_mode_falls_back_to_x509() {
        let none = Hash::new();
        let x509 = hash(&[("server_crt", "/x"), ("server_key", "/xk")]);
        assert_eq!(server_tls_mode(&none, &x509, &none, &none), Ok(CLIENT_CERT));
    }

    #[test]
    fn tls_mode_dpu_ephemeral() {
        let none = Hash::new();
        let dpu = hash(&[("type", "SmartSwitchDPU")]);
        let dpu_switch_type = hash(&[("switch_type", "dpu")]);
        assert_eq!(server_tls_mode(&none, &none, &none, &dpu), Ok(NO_CLIENT_CERT));
        assert_eq!(server_tls_mode(&none, &none, &none, &dpu_switch_type), Ok(NO_CLIENT_CERT));
        let partial = hash(&[("server_crt", "/c")]);
        assert_eq!(server_tls_mode(&partial, &none, &none, &dpu), Ok(NO_CLIENT_CERT));
        let full = hash(&[("server_crt", "/c"), ("server_key", "/k")]);
        assert_eq!(server_tls_mode(&full, &none, &none, &dpu), Ok(CLIENT_CERT));
        let empty_key = hash(&[("server_crt", "/c"), ("server_key", "")]);
        assert_eq!(server_tls_mode(&empty_key, &none, &none, &dpu), Ok(NO_CLIENT_CERT));
    }

    #[test]
    fn security_config_uses_server_tls_source_paths() {
        let none = Hash::new();
        let x509 = hash(&[("server_crt", "/x.crt"), ("server_key", "/x.key"), ("ca_crt", "/xca.crt")]);
        let sec = security_config(&none, &x509, &none, &none).unwrap();
        assert_eq!(sec.tls, CLIENT_CERT);
        assert_eq!((sec.ca_crt.as_str(), sec.server_crt.as_str(), sec.server_key.as_str()),
                   ("/xca.crt", "/x.crt", "/x.key"));

        let certs = hash(&[("server_crt", "/g.crt"), ("server_key", "/g.key")]);
        let sec = security_config(&certs, &x509, &none, &none).unwrap();
        assert_eq!(sec.tls, CLIENT_CERT);
        assert_eq!(sec.server_crt, "/g.crt");
        assert_eq!(sec.ca_crt, DEFAULT_CA_CRT);
    }

    #[test]
    fn grpc_not_found_detection() {
        assert!(is_grpc_not_found("F0101 Get failed: rpc error: code = NotFound desc = foo not found in clientTrie tree"));
        assert!(is_grpc_not_found("show/foo not found in clientTrie tree"));
        assert!(!is_grpc_not_found("rpc error: code = Unavailable desc = connection refused"));
        assert!(!is_grpc_not_found("rpc error: code = Unauthenticated desc = bad cert"));
    }

    #[test]
    fn feature_state_fails_open() {
        assert!(feature_enabled(Some("enabled")));
        assert!(feature_enabled(Some("always_enabled")));
        assert!(feature_enabled(None));
        assert!(!feature_enabled(Some("disabled")));
        assert!(!feature_enabled(Some("Disabled")));
        assert!(!feature_enabled(Some("always_disabled")));
    }

    #[test]
    fn env_flag_uses_default_for_unknown_values() {
        assert!(env_flag(Some("true"), false));
        assert!(!env_flag(Some("False"), true));
        assert!(env_flag(Some("yes"), true));
        assert!(!env_flag(Some("yes"), false));
        assert!(env_flag(None, true));
    }

    #[test]
    fn timeout_parsing() {
        assert_eq!(read_timeout(Some("3")), Duration::from_secs(3));
        assert_eq!(read_timeout(Some("0")), Duration::from_secs(10));
        assert_eq!(read_timeout(Some("x")), Duration::from_secs(10));
        assert_eq!(read_timeout(None), Duration::from_secs(10));
    }

    #[test]
    fn truncation_respects_char_boundaries() {
        assert_eq!(truncate_on_char_boundary("abc", 5), "abc");
        assert_eq!(truncate_on_char_boundary("héllo", 2), "h");
        let long = "é".repeat(STDERR_TRUNCATE_LIMIT);
        let summary = summarize_stderr(&long);
        assert!(summary.ends_with(&format!("...[truncated {} bytes]", STDERR_TRUNCATE_LIMIT)));
    }

    #[test]
    fn gnmi_get_args_insecure_without_client_auth() {
        let sec = GnmiSecurityConfig {
            tls: NO_CLIENT_CERT,
            ca_crt: "ca".into(),
            server_crt: "crt".into(),
            server_key: "key".into(),
        };
        let cmd = gnmi_get_command(GNMI_BASE_CMD, "uptime", 50052, &sec, "tgt", "SHOW");
        assert_eq!(cmd.get_program(), GNMI_BASE_CMD);
        assert_eq!(
            args(&cmd),
            vec!["-xpath_target", "SHOW", "-xpath", "uptime", "-target_addr", "127.0.0.1:50052",
                 "-logtostderr", "-insecure", "true"]
        );
    }

    #[test]
    fn gnmi_get_args_notls() {
        let sec = GnmiSecurityConfig {
            tls: NO_TLS,
            ca_crt: "ca".into(),
            server_crt: "crt".into(),
            server_key: "key".into(),
        };
        let cmd = gnmi_get_command(GNMI_BASE_CMD, "uptime", 8080, &sec, "tgt", "SHOW");
        assert_eq!(
            args(&cmd),
            vec!["-xpath_target", "SHOW", "-xpath", "uptime", "-target_addr", "127.0.0.1:8080",
                 "-logtostderr", "-notls"]
        );
    }

    #[test]
    fn gnmi_get_args_with_client_auth() {
        let sec = GnmiSecurityConfig {
            tls: CLIENT_CERT,
            ca_crt: "ca".into(),
            server_crt: "crt".into(),
            server_key: "key".into(),
        };
        let cmd = gnmi_get_command(GNMI_BASE_CMD, SERIAL_NUMBER_XPATH, 8080, &sec, "tgt", "STATE_DB");
        assert_eq!(
            args(&cmd),
            vec!["-xpath_target", "STATE_DB", "-xpath", SERIAL_NUMBER_XPATH, "-target_addr",
                 "127.0.0.1:8080", "-logtostderr", "-ca", "ca", "-cert", "crt", "-key", "key",
                 "-target_name", "tgt"]
        );
    }

    #[test]
    fn run_probe_success() {
        let _guard = stub_lock();
        let res = run_probe("p", sh("exit 0"), None, Duration::from_secs(5), true);
        assert!(res.success);
        assert_eq!(res.xpath, "p");
        assert!(res.error.is_none());
    }

    #[test]
    fn run_probe_reports_exit_code_and_stderr() {
        let _guard = stub_lock();
        let res = run_probe("p", sh("echo boom >&2; exit 3"), None, Duration::from_secs(5), true);
        assert!(!res.success);
        assert!(!res.unsupported);
        assert_eq!(res.error.as_deref(), Some("stderr: boom; exit code: 3"));
    }

    const NOT_FOUND_SCRIPT: &str =
        "echo 'Get failed: rpc error: code = NotFound desc = show/foo not found in clientTrie tree' >&2; exit 1";

    #[test]
    fn run_probe_flags_not_found_as_unsupported() {
        let _guard = stub_lock();
        let res = run_probe("p", sh(NOT_FOUND_SCRIPT), None, Duration::from_secs(5), true);
        assert!(!res.success);
        assert!(res.unsupported);
        assert!(res.error.unwrap().contains("NotFound"));
    }

    #[test]
    fn run_probe_not_found_is_failure_when_unsupported_not_allowed() {
        let _guard = stub_lock();
        let res = run_probe("p", sh(NOT_FOUND_SCRIPT), None, Duration::from_secs(5), false);
        assert!(!res.success);
        assert!(!res.unsupported);
    }

    #[test]
    fn run_probe_not_found_detected_past_stderr_truncation() {
        let _guard = stub_lock();
        let script = format!("head -c 200000 /dev/zero | tr '\\0' x >&2; {NOT_FOUND_SCRIPT}");
        let res = run_probe("p", sh(&script), None, Duration::from_secs(5), true);
        assert!(res.unsupported);
    }

    #[test]
    fn run_probe_times_out() {
        let _guard = stub_lock();
        let res = run_probe("p", sh("sleep 5"), None, Duration::from_millis(200), true);
        assert!(!res.success);
        assert!(res.error.unwrap().contains("timed out"));
        assert!(res.duration_ms < 5000);
    }

    #[test]
    fn run_probe_spawn_failure() {
        let _guard = stub_lock();
        let res = run_probe("p", Command::new("/nonexistent/gnmi_get"), None, Duration::from_secs(1), true);
        assert!(!res.success);
        assert!(res.error.is_some());
    }

    #[test]
    fn run_probe_feeds_stdin() {
        let _guard = stub_lock();
        let res = run_probe("p", sh(r#"read l; [ "$l" = "[]" ]"#), Some(b"[]\n"), Duration::from_secs(5), false);
        assert!(res.success, "{:?}", res.error);
    }

    #[test]
    fn run_probe_drains_large_stderr() {
        let _guard = stub_lock();
        // More than a pipe buffer of stderr must not wedge the child until timeout.
        let res = run_probe("p", sh("head -c 200000 /dev/zero | tr '\\0' x >&2; exit 1"), None, Duration::from_secs(5), true);
        assert!(!res.success);
        assert!(!res.unsupported);
        let err = res.error.unwrap();
        assert!(err.contains("[truncated"), "{}", &err[err.len().saturating_sub(80)..]);
        assert!(err.ends_with("exit code: 1"));
    }

    #[test]
    fn tcp_table_parsing() {
        // 0xC3B4 = 50100, 0x1F90 = 8080, 0xC3B5 = 50101
        let v4 = "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode
   0: 00000000:C3B4 00000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 1 1 0000000000000000 100 0 0 10 0
   1: 0100007F:1F90 0100007F:D2F0 01 00000000:00000000 00:00000000 00000000     0        0 2 1 0000000000000000 20 4 30 10 -1
   2: 0100007F:C3B5 00000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 3 1 0000000000000000 100 0 0 10 0
";
        assert!(tcp_listening(v4, 50100));
        assert!(tcp_listening(v4, 50101));
        // ESTABLISHED (01), not LISTEN
        assert!(!tcp_listening(v4, 8080));
        assert!(!tcp_listening(v4, 0xD2F0));
        assert!(!tcp_listening("", 50100));

        let v6 = "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode
   0: 00000000000000000000000000000000:1F90 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 4 1 0000000000000000 100 0 0 10 0
   1: 00000000000000000000000001000000:C3B4 00000000000000000000000001000000:9C40 06 00000000:00000000 03:00000F4F 00000000     0        0 0 3 0000000000000000
";
        assert!(tcp_listening(v6, 8080));
        // TIME_WAIT (06)
        assert!(!tcp_listening(v6, 50100));
    }

    const LISTEN_8080: &str = "header
   0: 00000000:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 4 1
";

    #[test]
    fn listener_status_rules() {
        let never = || -> bool { panic!("socket consulted") };
        assert_eq!(listener_status(Ok(LISTEN_8080.into()), 8080, true, never), "OK");
        assert_eq!(listener_status(Ok(LISTEN_8080.into()), 50052, true, || true),
                   "ERROR: no TCP listener on port 50052");
        // uds-only: no port configured, no TCP listener, live socket.
        assert_eq!(listener_status(Ok("header\n".into()), 8080, false, || true), "OK (unix socket)");
        assert_eq!(listener_status(Ok("header\n".into()), 8080, false, || false),
                   "ERROR: no TCP listener on port 8080");
        assert_eq!(listener_status(Err("cannot read".into()), 8080, false, || true), "ERROR: cannot read");
    }

    #[test]
    fn proc_tcp_tables_readable() {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let port = listener.local_addr().unwrap().port();
        assert!(tcp_listening(&read_tcp_tables().unwrap(), port));
        drop(listener);
        assert!(!tcp_listening(&read_tcp_tables().unwrap(), port));
    }

    fn result(success: bool, unsupported: bool) -> CommandResult {
        CommandResult {
            xpath: "x".into(),
            success,
            error: None,
            unsupported,
            duration_ms: 0,
            unauthenticated: false,
        }
    }

    #[test]
    fn bad_cert_probe_passes_only_when_rejected() {
        let mut rejected = result(false, false);
        rejected.error = Some("exit code: 1".into());
        let res = expect_rejected(rejected);
        assert!(res.success);
        assert_eq!(res.error.as_deref(), Some("exit code: 1"));
        let res = expect_rejected(result(true, false));
        assert!(!res.success);
        assert!(res.error.unwrap().contains("Expected FAILURE"));
    }

    #[test]
    fn show_probe_needs_one_success() {
        assert_eq!(show_probe_error(&[]), None);
        assert_eq!(show_probe_error(&[result(false, true), result(true, false)]), None);
        assert_eq!(
            show_probe_error(&[result(false, true), result(false, true)]),
            Some("no supported SHOW xpaths: 2 unsupported, 0 failed".to_string())
        );
        assert_eq!(
            show_probe_error(&[result(false, true), result(false, false)]),
            Some("no supported SHOW xpaths: 1 unsupported, 1 failed".to_string())
        );
    }

    struct Stub {
        dir: std::path::PathBuf,
    }

    impl Stub {
        // A fake gnmi_get that logs its args and runs `body`.
        fn new(name: &str, body: &str) -> Stub {
            use std::os::unix::fs::PermissionsExt;
            let dir = env::temp_dir().join(format!("gnmi-watchdog-test-{}-{name}", std::process::id()));
            let _ = fs::remove_dir_all(&dir);
            fs::create_dir_all(&dir).unwrap();
            let script = dir.join("gnmi_get");
            let log = dir.join("calls");
            fs::write(&script, format!("#!/bin/sh\necho \"$*\" >> '{}'\n{body}\n", log.display())).unwrap();
            fs::set_permissions(&script, fs::Permissions::from_mode(0o755)).unwrap();
            Stub { dir }
        }

        fn program(&self) -> String {
            self.dir.join("gnmi_get").display().to_string()
        }

        fn calls(&self) -> Vec<String> {
            fs::read_to_string(self.dir.join("calls")).unwrap_or_default().lines().map(String::from).collect()
        }

        fn prober(&self, socket: Option<&str>, tcp: Result<(u16, GnmiSecurityConfig), String>) -> Prober {
            Prober {
                program: self.program(),
                socket: socket.map(String::from),
                tcp,
                target_name: "tgt".into(),
                timeout: Duration::from_secs(5),
            }
        }
    }

    impl Drop for Stub {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.dir);
        }
    }

    fn notls_tcp() -> Result<(u16, GnmiSecurityConfig), String> {
        Ok((8080, GnmiSecurityConfig {
            tls: NO_TLS,
            ca_crt: "ca".into(),
            server_crt: "crt".into(),
            server_key: "key".into(),
        }))
    }

    const UNAUTH: &str = r#"case "$*" in *unix://*) echo 'rpc error: code = Unauthenticated desc = no cert' >&2; exit 1;; esac"#;

    #[test]
    fn prober_uses_unix_socket_when_present() {
        let _guard = stub_lock();
        let stub = Stub::new("unix", "exit 0");
        let mut prober = stub.prober(Some("/run/g.sock"), notls_tcp());
        assert!(prober.get("clock", "SHOW", true).unwrap().success);
        assert_eq!(stub.calls(), vec!["-xpath_target SHOW -xpath clock -target_addr unix:///run/g.sock -logtostderr -notls"]);
    }

    #[test]
    fn prober_uses_tcp_without_socket() {
        let _guard = stub_lock();
        let stub = Stub::new("tcp", "exit 0");
        let mut prober = stub.prober(None, notls_tcp());
        assert!(prober.get("clock", "SHOW", true).unwrap().success);
        assert_eq!(stub.calls(), vec!["-xpath_target SHOW -xpath clock -target_addr 127.0.0.1:8080 -logtostderr -notls"]);
    }

    #[test]
    fn prober_falls_back_to_tcp_on_unauthenticated() {
        let _guard = stub_lock();
        let stub = Stub::new("fallback", UNAUTH);
        let mut prober = stub.prober(Some("/run/g.sock"), notls_tcp());
        assert!(prober.get("clock", "SHOW", true).unwrap().success);
        assert!(prober.get("uptime", "SHOW", true).unwrap().success);
        let calls = stub.calls();
        assert_eq!(calls.len(), 3, "{calls:?}");
        assert!(calls[0].contains("unix:///run/g.sock"));
        assert!(calls[1].contains("-xpath clock -target_addr 127.0.0.1:8080"));
        // The socket is not retried after it rejected us.
        assert!(calls[2].contains("-xpath uptime -target_addr 127.0.0.1:8080"));
    }

    #[test]
    fn prober_keeps_unix_result_on_other_errors() {
        let _guard = stub_lock();
        let stub = Stub::new("notfound", "echo 'rpc error: code = NotFound desc = not found in clientTrie' >&2; exit 1");
        let mut prober = stub.prober(Some("/run/g.sock"), notls_tcp());
        let res = prober.get("clock", "SHOW", true).unwrap();
        assert!(res.unsupported);
        assert_eq!(stub.calls().len(), 1);
    }

    #[test]
    fn prober_reports_skip_when_tcp_unusable() {
        let _guard = stub_lock();
        let reason = "user_auth=password not supported by watchdog".to_string();
        let stub = Stub::new("skip", UNAUTH);
        let mut prober = stub.prober(Some("/run/g.sock"), Err(reason.clone()));
        assert_eq!(prober.get("clock", "SHOW", true).unwrap_err(), reason);
        let mut prober = stub.prober(None, Err(reason.clone()));
        assert_eq!(prober.get("clock", "SHOW", true).unwrap_err(), reason);
    }

    #[test]
    fn usable_socket_requires_a_socket() {
        let dir = env::temp_dir().join(format!("gnmi-watchdog-test-{}-sock", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).unwrap();
        let sock = dir.join("gnmi.sock").display().to_string();
        let file = dir.join("file").display().to_string();
        fs::write(&file, "").unwrap();
        let _listener = std::os::unix::net::UnixListener::bind(&sock).unwrap();
        assert_eq!(usable_socket(&sock), Some(sock.clone()));
        assert_eq!(usable_socket(&file), None);
        assert_eq!(usable_socket(&dir.join("missing").display().to_string()), None);
        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn http_response_status_and_body() {
        let mut status = HealthStatus {
            gnmi_status: "OK".to_string(),
            probes_skipped: None,
            show_probe_error: None,
            xpath_commands: vec![],
            xpath_load_errors: vec![],
        };
        let body = r#"{"gnmi_status":"OK","xpath_commands":[],"xpath_load_errors":[]}"#;
        assert_eq!(
            http_response(true, &status),
            format!("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{body}", body.len())
        );
        assert!(http_response(false, &status).starts_with("HTTP/1.1 500 Internal Server Error\r\n"));

        status.probes_skipped = Some("user_auth=password not supported by watchdog".to_string());
        assert!(http_response(true, &status).ends_with(
            r#"{"gnmi_status":"OK","probes_skipped":"user_auth=password not supported by watchdog","xpath_commands":[],"xpath_load_errors":[]}"#
        ));
    }
}
