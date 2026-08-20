use std::io::{Read, Write};
use std::net::TcpListener;
use std::path::Path;

// Check GCU venv integrity
fn check_gcu_status() -> String {
    let venv_path = Path::new("/opt/sonic/gcu/venv");
    if !venv_path.exists() {
        return format!("ERROR: GCU venv not found at {}", venv_path.display());
    }

    "OK".to_string()
}

fn main() {
    let watchdog_port = 51200;
    // Start a HTTP server listening on port 51200
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

                let gcu_result = check_gcu_status();

                // Build a JSON object
                let json_body = format!(
                    r#"{{"gcu_status":"{}"}}"#,
                    gcu_result
                );

                let all_passed = gcu_result.starts_with("OK");

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
