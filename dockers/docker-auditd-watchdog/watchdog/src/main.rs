use std::io::{Read, Write};
use std::net::TcpListener;
use std::process::Command;
use regex::Regex;

static NSENTER_CMD: &str = "nsenter --target 1 --pid --mount --uts --ipc --net";

// Expected hash values
static AUDITD_CONF_HASH: &str = "7cdbd1450570c7c12bdc67115b46d9ae778cbd76";
static AUDITD_RULES_HASH_64BIT: &str = "1c532e73fdd3f7366d9c516eb712102d3063bd5a";
static AUDITD_RULES_HASH_32BIT: &str = "ac45b13d45de02f08e12918e38b4122206859555";

// Helper to run commands
fn run_command(cmd: &str) -> Result<String, String> {
    println!("Running command: {}", cmd);
    let output = Command::new("sh")
        .arg("-c")
        .arg(cmd)
        .output()
        .map_err(|e| format!("Failed to spawn command '{}': {}", cmd, e))?;

    if !output.status.success() {
        let error = format!(
            "Command '{}' failed with status {}: {}",
            cmd,
            output.status.code().map_or("unknown".to_string(), |c| c.to_string()),
            String::from_utf8_lossy(&output.stderr)
        );
        eprintln!("{}", error);
        return Err(error);
    }
    Ok(String::from_utf8_lossy(&output.stdout).to_string())
}

// Check auditd.conf sha1sum
fn check_auditd_conf() -> String {
    let cmd = format!(r#"{NSENTER_CMD} cat /etc/audit/auditd.conf | sha1sum"#);
    match run_command(&cmd) {
        Ok(s) => {
            if s.contains(AUDITD_CONF_HASH) {
                "OK".to_string()
            } else {
                format!("FAIL (sha1 = {}, expected = {})", s.trim(), AUDITD_CONF_HASH)
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
fn check_auditd_rules() -> String {
    // Get HW SKU
    let cmd = format!(r#"{NSENTER_CMD} file -L /bin/sh"#);
    let bitness = match run_command(&cmd) {
        Ok(s) => s.trim().to_string(),
        Err(e) => return format!("FAIL (could not get bitness: {})", e),
    };

    let expected = if bitness.contains("32-bit") {
        AUDITD_RULES_HASH_32BIT
    } else if bitness.contains("64-bit") {
        AUDITD_RULES_HASH_64BIT
    } else {
        return format!("FAIL (unknown bitness: {})", bitness);
    };

    let cmd = format!(
        r#"{NSENTER_CMD} sh -c 'find /etc/audit/rules.d/ -name '*.rules' -type f | sort | xargs cat 2>/dev/null | sha1sum'"#
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


// Check auditd rate limit
fn check_auditd_rate_limit_status() -> String {
    // read auditd rules config file
    let cmd = format!(r#"{NSENTER_CMD} cat /etc/audit/rules.d/audit.rules"#);
    match run_command(&cmd) {
        Ok(file_config) => {
            let confix_file_regex = Regex::new(r"-r (?<rate>\d+)").unwrap();
            match confix_file_regex.captures(&file_config) {
                Some(config_file_caps) => {
                    let config_file_rate_limit = &config_file_caps["rate"];

                    let cmd = format!(r#"{NSENTER_CMD} auditctl -s"#);
                    match run_command(&cmd) {
                        Ok(running_config) => {
                            let running_config_regex = Regex::new(r"rate_limit (?<rate>\d+)").unwrap();
                            match running_config_regex.captures(&running_config) {
                                Some(running_config_caps) => {
                                    if &running_config_caps["rate"] == config_file_rate_limit {
                                        "OK".to_string()
                                    } else {
                                        format!("FAIL (rate_limit: {} mismatch with config file setting: {})", running_config, config_file_rate_limit)
                                    }
                                }
                                None => {
                                    format!("FAIL (rate_limit not set = {}, config file setting: {})", running_config, config_file_rate_limit)
                                }
                            }
                        }
                        Err(e) => format!("FAIL (error message = {})", e),
                    }
                }
                None => {
                    // rate limit disabled when -r missing in config file
                    "OK".to_string()
                }
            }
        }
        Err(e) => {
            // config file missing
            format!("FAIL (open config file failed, error message = {})", e)
        }
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

                let conf_result      = serde_json::to_string(&check_auditd_conf()).unwrap();
                let syslog_result    = serde_json::to_string(&check_syslog_conf()).unwrap();
                let rules_result     = serde_json::to_string(&check_auditd_rules()).unwrap();
                let srvc_result      = serde_json::to_string(&check_auditd_service()).unwrap();
                let srvc_active      = serde_json::to_string(&check_auditd_active()).unwrap();
                let rate_limit_result = serde_json::to_string(&check_auditd_rate_limit_status()).unwrap();

                // Build JSON response
                let json_body = format!(
                    r#"{{
  "auditd_conf":{},
  "syslog_conf":{},
  "auditd_rules":{},
  "auditd_service":{},
  "auditd_active":{},
  "rate_limit":{}
}}"#,
                    conf_result,
                    syslog_result,
                    rules_result,
                    srvc_result,
                    srvc_active,
                    rate_limit_result
                );

                // Determine overall status
                let all_results = vec![
                    &conf_result,
                    &syslog_result,
                    &rules_result,
                    &srvc_result,
                    &srvc_active,
                    &rate_limit_result
                ];
                let all_passed = all_results.iter().all(|r| r.trim_matches('"').starts_with("OK"));

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
