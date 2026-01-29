use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};

// Check restapi program status
fn check_restapi_status() -> String {
    let restapi_https_port = 8081;
    let addr = format!("127.0.0.1:{}", restapi_https_port);
    match TcpStream::connect(&addr) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn main() {
    let watchdog_port = 50100;
    // Start a HTTP server listening on port 50100
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

                let restapi_result = check_restapi_status();

                // Build a JSON object
                let json_body = format!(
                    r#"{{"restapi_status":"{}"}}"#,
                    restapi_result
                );

                // Determine overall status
                let all_results = vec![
                    &restapi_result
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
