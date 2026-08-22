use std::io::{BufRead, BufReader, Write};
use std::net::TcpListener;
use std::process::Command;

use serde::Serialize;
use redis::Commands;

#[derive(Serialize)]
struct HealthStatus {
    check_bmp_db: String,
    check_bmp_port: String,
}

// Connects to Redis DB 4 and returns true if the bmp feature is enabled.
// If Redis is unavailable or the field is missing, default to enabled (fail-open).
fn is_bmp_enabled() -> bool {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error (feature): {e}");
            return true;
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error (feature): {e}");
            return true;
        }
    };

    let res: redis::RedisResult<Option<String>> = conn.hget("FEATURE|bmp", "state");
    match res {
        Ok(Some(state)) => !state.eq_ignore_ascii_case("disabled"),
        Ok(None) => {
            eprintln!("Redis: FEATURE|bmp.state missing; defaulting to enabled");
            true
        }
        Err(e) => {
            eprintln!("Redis HGET error (feature): {e}");
            true
        }
    }
}

fn check_bmp_db() -> String {
    let output = Command::new("docker")
        .args(["exec", "-i", "database", "supervisorctl", "status"])
        .output();

    match output {
        Ok(output) => {
            if !output.status.success() {
                return format!("ERROR: Command failed with status {}", output.status);
            }

            let stdout = String::from_utf8_lossy(&output.stdout);

            let has_redis_bmp = stdout.lines().any(|line| {
                line.starts_with("redis_bmp") && line.contains("RUNNING")
            });

            if has_redis_bmp {
                "OK".to_string()
            } else {
                "ERROR: redis_bmp not running".to_string()
            }
        }
        Err(e) => format!("ERROR: Failed to run command - {}", e),
    }
}

fn check_bmp_port() -> String {
    match std::net::TcpStream::connect("127.0.0.1:5000") {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn main() {
    // Start a HTTP server listening on port 50060
    let listener = TcpListener::bind("127.0.0.1:50060")
        .expect("Failed to bind to 127.0.0.1:50060");

    println!("Watchdog HTTP server running on http://127.0.0.1:50060");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                // Read the first request line in a short scope so the borrow ends before any writes.
                let mut request_line = String::new();
                {
                    let mut reader = BufReader::new(&mut stream);
                    let _ = reader.read_line(&mut request_line);
                }

                println!("Received request: {}", request_line.trim_end());

                if !request_line.starts_with("GET /") {
                    let _ = stream.write_all(b"HTTP/1.1 405 Method Not Allowed\r\n\r\n");
                    continue;
                }

                // Default to “feature disabled” status; override if BMP is enabled.
                let mut db_result = "OK: feature disabled".to_string();
                let mut port_result = "OK: feature disabled".to_string();

                if is_bmp_enabled() {
                    db_result = check_bmp_db();
                    port_result = check_bmp_port();
                }

                let status = HealthStatus {
                    check_bmp_db: db_result.clone(),
                    check_bmp_port: port_result.clone(),
                };

                let all_passed = [db_result, port_result]
                    .iter()
                    .all(|s| s.starts_with("OK"));

                let status_line = if all_passed {
                    "HTTP/1.1 200 OK"
                } else {
                    "HTTP/1.1 500 Internal Server Error"
                };

                let json_body = serde_json::to_string(&status).unwrap();
                let response = format!(
                    "{status_line}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                    json_body.len(),
                    json_body
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
