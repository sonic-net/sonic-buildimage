use std::io::{Read, Write};
use std::net::TcpListener;
use std::process::Command;
use chrono::{TimeZone};

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
            String::from_utf8_lossy(&output.stderr).to_string() // Clone the error message
        ));
    }

    Ok(String::from_utf8_lossy(&output.stdout).to_string()) // Clone the output
}

// Check gnmi program status
fn check_gnmi_status() -> String {

    "OK".to_string()
}

fn main() {
    let watchdog_port = 51000;
    // Start a HTTP server listening on port 51000
    let listener = TcpListener::bind(format!("127.0.0.1:{}", watchdog_port))
        .expect(&format!("Failed to bind to 127.0.0.1:{}", watchdog_port));

    println!("Watchdog HTTP server running on http://127.0.0.1:{}", watchdog_port);

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut buffer = [0_u8; 512];
                if let Ok(bytes_read) = stream.read(&mut buffer) {
                    let req_str = String::from_utf8_lossy(&buffer[..bytes_read]);
                    println!("Received request: {}", req_str);
                }

                let gnmi_result      = check_gnmi_status();

                // Build a JSON object
                let json_body = format!(
                    r#"{{"gnmi_status":"{}"}}"#,
                    gnmi_result
                );

                // Determine overall status
                let all_results = vec![
                    &gnmi_result
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
