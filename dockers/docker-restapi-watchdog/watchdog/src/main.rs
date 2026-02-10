use std::io::{BufRead, BufReader, Write};
use std::net::{TcpListener, TcpStream};
use std::time::Duration;

use serde::Serialize;

#[derive(serde::Serialize)]
struct HealthStatus {
    restapi_status: String,
}

// Check restapi program status
fn check_restapi_status() -> String {
    let restapi_https_port = 8081;
    let addr = format!("127.0.0.1:{}", restapi_https_port);
    let timeout = Duration::from_secs(5);

    match TcpStream::connect_timeout(&addr.parse().unwrap(), timeout) {
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
                let mut reader = BufReader::new(&stream);
                let mut request_line = String::new();

                if let Ok(_) = reader.read_line(&mut request_line) {
                    println!("Received request: {}", request_line.trim_end());

                    if !request_line.starts_with("GET /") {
                        let response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                        if let Err(e) = stream.write_all(response.as_bytes()) {
                            eprintln!("Failed to write response: {}", e);
                        }
                        continue;
                    }

                    let restapi_result = check_restapi_status();

                    let status = HealthStatus {
                        restapi_status: restapi_result,
                    };

                    // Build a JSON object
                    let json_body = serde_json::to_string(&status).unwrap();

                    let (status_line, content_length) = if status.restapi_status == "OK" {
                        ("HTTP/1.1 200 OK", json_body.len())
                    } else {
                        ("HTTP/1.1 500 Internal Server Error", json_body.len())
                    };

                    let response = format!(
                        "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {content_length}\
                        \r\nConnection: close\r\n\r\n{json_body}"
                    );

                    if let Err(e) = stream.write_all(response.as_bytes()) {
                        eprintln!("Failed to write response: {}", e);
                    }
                }
            }
            Err(e) => {
                eprintln!("Error accepting connection: {}", e);
            }
        }
    }
}
