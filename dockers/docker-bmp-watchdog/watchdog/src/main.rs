use std::io::{BufRead, BufReader, Write};
use std::net::TcpListener;
use std::process::Command;
 
use serde::Serialize;
 
#[derive(Serialize)]
struct HealthStatus {
    check_bmp_supervisorctl: String,
    check_bmp_db: String,
    check_bmp_port: String,
}
 
fn check_bmp_supervisorctl() -> String {
    // TODO: Replace with real health check
    "OK".to_string()
}
 
fn check_bmp_db() -> String {
    // TODO: Replace with real health check
    "OK".to_string()
}
 
fn check_bmp_port() -> String {
    // TODO: Replace with real health check
    "OK".to_string()
}
 
fn main() {
    // Start a HTTP server listening on port 50058
    let listener = TcpListener::bind("127.0.0.1:50058")
    .expect("Failed to bind to 127.0.0.1:50058");

    println!("Watchdog HTTP server running on http://127.0.0.1:50058");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut reader = BufReader::new(&stream);
                let mut request_line = String::new();
 
                if let Ok(_) = reader.read_line(&mut request_line) {
                    println!("Received request: {}", request_line.trim_end());
 
                    if !request_line.starts_with("GET /") {
                        let response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
                        stream.write_all(response.as_bytes()).ok();
                        continue;
                    }
 
                    let supervisorctl_result = check_bmp_supervisorctl();
                    let db_result = check_bmp_db();
                    let port_result = check_bmp_port();
 
                    let status = HealthStatus {
                        check_bmp_supervisorctl: supervisorctl_result.clone(),
                        check_bmp_db: db_result.clone(),
                        check_bmp_port: port_result.clone(),
                    };
 
                    let json_body = serde_json::to_string(&status).unwrap();
                    let all_passed = [supervisorctl_result, db_result, port_result]
                        .iter()
                        .all(|s| s.starts_with("OK"));
 
                    let status_line = if all_passed {
                        "HTTP/1.1 200 OK"
                    } else {
                        "HTTP/1.1 500 Internal Server Error"
                    };
 
                    let response = format!(
                        "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                        json_body.len(),
                        json_body
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