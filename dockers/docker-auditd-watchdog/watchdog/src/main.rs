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

// Check syslog.conf
fn check_syslog_conf() -> String {
    let cmd = format!(r#"{NSENTER_CMD} grep '^active = yes' /etc/audit/plugins.d/syslog.conf"#);
    match run_command(&cmd) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("FAIL (syslog.conf does not contain 'active = yes': {})", e),
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
        "bd574779fb4e1116838d18346187bb7f7bd089c9"
    } else {
        "f88174f901ec8709bacaf325158f10ec62909d13"
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

// Check auditd.service
fn check_auditd_service() -> String {
    let cmd = format!(r#"{NSENTER_CMD} grep '^CPUQuota=10%' /lib/systemd/system/auditd.service"#);
    match run_command(&cmd) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("FAIL (auditd.service does not contain 'CPUQuota=10%': {})", e),
    }
}

// Check that auditd is active
fn check_auditd_active() -> String {
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

// Check reload auditd rules
fn check_auditd_reload_status() -> String {
    let cmd = format!(r#"{NSENTER_CMD} auditctl -R /etc/audit/audit.rules"#);
    match run_command(&cmd) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("FAIL (error message = {})", e),
    }
}

fn main() {
    // Start a HTTP server listening on port 50058
    let listener = TcpListener::bind("127.0.0.1:50058")
        .expect("Failed to bind to 127.0.0.1:50058");

    println!("Watchdog HTTP server running on http://127.0.0.1:50058");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut buffer = [0_u8; 512];
                if let Ok(bytes_read) = stream.read(&mut buffer) {
                    let req_str = String::from_utf8_lossy(&buffer[..bytes_read]);
                    println!("Received request: {}", req_str);
                }

                let conf_result      = check_auditd_conf();
                let syslog_result    = check_syslog_conf();
                let rules_result     = check_auditd_rules();
                let srvc_result      = check_auditd_service();
                let srvc_active      = check_auditd_active();
                let reload_result    = check_auditd_reload_status();

                // Build a JSON object
                let json_body = format!(
                    r#"{{
  "auditd_conf":"{}",
  "syslog_conf":"{}",
  "auditd_rules":"{}",
  "auditd_service":"{}",
  "auditd_active":"{}",
  "auditd_reload":"{}"
}}"#,
                    conf_result,
                    syslog_result,
                    rules_result,
                    srvc_result,
                    srvc_active,
                    reload_result
                );

                // Determine overall status
                let all_results = vec![
                    &conf_result,
                    &syslog_result,
                    &rules_result,
                    &srvc_result,
                    &srvc_active,
                    &reload_result
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
