use std::io::{Read, Write};
use std::net::TcpListener;
use std::process::Command;

static NSENTER_CMD: &str = "nsenter --target 1 --pid --mount --uts --ipc --net";

// Helper to run commands
fn run_command(cmd: &str) -> Result<String, String> {
    let output = Command::new("sh")
        .arg("-c")
        .arg(cmd)
        .output()
        .map_err(|e| format!("Failed to spawn command '{}': {}", cmd, e))?;

    if !output.status.success() {
        return Err(format!(
            "Command '{}' failed with status {}: {}",
            cmd,
            output.status.code().map_or("unknown".to_string(), |c| c.to_string()),
            String::from_utf8_lossy(&output.stderr)
        ));
    }
    Ok(String::from_utf8_lossy(&output.stdout).to_string())
}

// Check CPU usage for auditd
fn check_cpu_usage() -> String {
    let cmd = format!(
        r#"{NSENTER_CMD} top -b -n1 -p $(pidof auditd) | grep auditd | awk '{{print $9}}'"#
    );
    match run_command(&cmd) {
        Ok(s) => {
            if let Ok(value) = s.trim().parse::<f32>() {
                if value <= 0.8 {
                    "OK".to_string()
                } else {
                    format!("FAIL (CPU usage {:.2} > 0.8)", value)
                }
            } else {
                format!("FAIL (could not parse CPU usage: {})", s.trim())
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check memory usage for auditd
fn check_mem_usage() -> String {
    let cmd = format!(
        r#"{NSENTER_CMD} top -b -n1 -p $(pidof auditd) | grep auditd | awk '{{print $10}}'"#
    );
    match run_command(&cmd) {
        Ok(s) => {
            if let Ok(value) = s.trim().parse::<f32>() {
                if value <= 0.8 {
                    "OK".to_string()
                } else {
                    format!("FAIL (MEM usage {:.2} > 0.8)", value)
                }
            } else {
                format!("FAIL (could not parse MEM usage: {})", s.trim())
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check auditd.conf sha1sum
fn check_auditd_conf() -> String {
    let cmd = format!(r#"{NSENTER_CMD} cat /etc/audit/auditd.conf | sha1sum"#);
    match run_command(&cmd) {
        Ok(s) => {
            if s.contains("7cdbd1450570c7c12bdc67115b46d9ae778cbd76") {
                "OK".to_string()
            } else {
                format!("FAIL (sha1 = {})", s.trim())
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check syslog.conf sha1sum
fn check_syslog_conf() -> String {
    let cmd = format!(r#"{NSENTER_CMD} cat /etc/audit/plugins.d/syslog.conf | sha1sum"#);
    match run_command(&cmd) {
        Ok(s) => {
            if s.contains("9939d57c7a895b3a12f5334d191b786b89ecd022") {
                "OK".to_string()
            } else {
                format!("FAIL (syslog.conf sha1 = {})", s.trim())
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check auditd rules sha1, depends on HW SKU
// - If HW SKU contains "Nokia-7215" or "Nokia-M0-7215", expect 65a4379b1401159cf2699f34a2a014f1b50c021d
// - Otherwise expect 317040ff8516bd74f97e5f5570834955f52c28b6
fn check_auditd_rules() -> String {
    // Get HW SKU
    let hwsku_cmd = format!(r#"{NSENTER_CMD} sonic-cfggen -d -v DEVICE_METADATA.localhost.hwsku"#);
    let hwsku = match run_command(&hwsku_cmd) {
        Ok(s) => s.trim().to_string(),
        Err(e) => return format!("FAIL (could not get HW SKU: {})", e),
    };

    let expected = if hwsku.contains("Nokia-7215") || hwsku.contains("Nokia-M0-7215") {
        "65a4379b1401159cf2699f34a2a014f1b50c021d"
    } else {
        "317040ff8516bd74f97e5f5570834955f52c28b6"
    };

    let cmd = format!(
        r#"{NSENTER_CMD} find /etc/audit/rules.d -type f -name "[0-9][0-9]-*.rules" \
            ! -name "30-audisp-tacplus.rules" -exec cat {{}} + | sort | sha1sum"#
    );

    match run_command(&cmd) {
        Ok(s) => {
            if s.contains(expected) {
                "OK".to_string()
            } else {
                format!("FAIL (rules sha1 = {}, expected {})", s.trim(), expected)
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check auditd.service file sha1sum
fn check_auditd_service_sha1() -> String {
    let cmd = format!(r#"{NSENTER_CMD} cat /lib/systemd/system/auditd.service | sha1sum"#);
    match run_command(&cmd) {
        Ok(s) => {
            if s.contains("86ad795f89cda625220472965c1d880088796621") {
                "OK".to_string()
            } else {
                format!("FAIL (auditd.service sha1 = {})", s.trim())
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

// Check that auditd is active
fn check_auditd_service_status() -> String {
    let cmd = format!(r#"{NSENTER_CMD} systemctl is-active auditd"#);
    match run_command(&cmd) {
        Ok(s) => {
            let trimmed = s.trim();
            if trimmed == "active" {
                "OK".to_string()
            } else {
                format!("FAIL (auditd status = {})", trimmed)
            }
        }
        Err(e) => format!("FAIL ({})", e),
    }
}

fn main() {
    // Start a HTTP server listening on port 8080
    let listener = TcpListener::bind("0.0.0.0:8080")
        .expect("Failed to bind to 0.0.0.0:8080");

    println!("Watchdog HTTP server running on http://0.0.0.0:8080");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut buffer = [0_u8; 512];
                if let Ok(bytes_read) = stream.read(&mut buffer) {
                    let req_str = String::from_utf8_lossy(&buffer[..bytes_read]);
                    println!("Received request: {}", req_str);
                }

                let cpu_result       = check_cpu_usage();
                let mem_result       = check_mem_usage();
                let conf_result      = check_auditd_conf();
                let syslog_result    = check_syslog_conf();
                let rules_result     = check_auditd_rules();
                let svc_file_sha1    = check_auditd_service_sha1();
                let svc_status       = check_auditd_service_status();

                // Build a JSON object
                let json_body = format!(
                    r#"{{
  "cpu_usage":"{}",
  "mem_usage":"{}",
  "auditd_conf":"{}",
  "syslog_conf_sha1":"{}",
  "auditd_rules_sha1":"{}",
  "auditd_service_sha1":"{}",
  "auditd_service_status":"{}"
}}"#,
                    cpu_result,
                    mem_result,
                    conf_result,
                    syslog_result,
                    rules_result,
                    svc_file_sha1,
                    svc_status
                );

                // Determine overall status
                let all_results = vec![
                    &cpu_result,
                    &mem_result,
                    &conf_result,
                    &syslog_result,
                    &rules_result,
                    &svc_file_sha1,
                    &svc_status
                ];
                let all_passed = all_results.iter().all(|r| r.starts_with("OK"));

                let (status_line, content_length) = if all_passed {
                    ("HTTP/1.1 200 OK", json_body.len())
                } else {
                    ("HTTP/1.1 500 Internal Server Error", json_body.len())
                };

                let response = format!(
                    "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {content_length}\r\n\r\n{json_body}"
                );

                if let Err(e) = stream.write_all(response.as_bytes()) {
                    eprintln!("Failed to write response: {}", e);
                }
            }
            Err(e) => {
                eprintln!("Error accepting connection: {}", e);
            }
        }
    }
}
