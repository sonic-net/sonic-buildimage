use std::io::{BufRead, BufReader, Write};
use std::net::{TcpListener, TcpStream};

use serde::Serialize;
use redis::Commands;

// Fail-open: if Redis is down or field is missing/invalid, default to 50051
const DEFAULT_TELEMETRY_SERVICE_PORT: u16 = 50051;

#[derive(Serialize)]
struct HealthStatus {
    check_telemetry_port: String,
}

fn get_gnmi_port() -> u16 {
    let client = match redis::Client::open("redis://127.0.0.1:6379/4") {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis client error (port): {e}");
            return DEFAULT_TELEMETRY_SERVICE_PORT;
        }
    };
    let mut conn = match client.get_connection() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Redis connection error (port): {e}");
            return DEFAULT_TELEMETRY_SERVICE_PORT;
        }
    };

    let res: redis::RedisResult<Option<String>> = conn.hget("TELEMETRY|gnmi", "port");
    match res {
        Ok(Some(p)) => p.parse::<u16>().unwrap_or_else(|_| {
            eprintln!("Redis: TELEMETRY|gnmi.port not a valid u16: {p}");
            DEFAULT_TELEMETRY_SERVICE_PORT
        }),
        Ok(None) => {
            eprintln!("Redis: TELEMETRY|gnmi.port missing; defaulting to {}", DEFAULT_TELEMETRY_SERVICE_PORT);
            DEFAULT_TELEMETRY_SERVICE_PORT
        }
        Err(e) => {
            eprintln!("Redis HGET error (port): {e}");
            DEFAULT_TELEMETRY_SERVICE_PORT
        }
    }
}

// Connects to Redis DB 4 and returns true if the telemetry feature is enabled.
// If Redis is unavailable or the field is missing, default to enabled (fail-open).
fn is_telemetry_enabled() -> bool {
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

    let res: redis::RedisResult<Option<String>> = conn.hget("FEATURE|telemetry", "state");
    match res {
        Ok(Some(state)) => !state.eq_ignore_ascii_case("disabled"),
        Ok(None) => {
            eprintln!("Redis: FEATURE|telemetry.state missing; defaulting to enabled");
            true
        }
        Err(e) => {
            eprintln!("Redis HGET error (feature): {e}");
            true
        }
    }
}

fn check_telemetry_port() -> String {
    let port = get_gnmi_port();
    let addr = format!("127.0.0.1:{port}");
    match TcpStream::connect(&addr) {
        Ok(_) => "OK".to_string(),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn main() {
    let listener = TcpListener::bind("127.0.0.1:50080")
        .expect("Failed to bind to 127.0.0.1:50080");
    println!("Watchdog HTTP server running on http://127.0.0.1:50080");

    for stream_result in listener.incoming() {
        match stream_result {
            Ok(mut stream) => {
                let mut reader = BufReader::new(&stream);
                let mut request_line = String::new();

                if let Ok(_) = reader.read_line(&mut request_line) {
                    println!("Received request: {}", request_line.trim_end());

                    if !request_line.starts_with("GET /") {
                        let response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
                        let _ = stream.write_all(response.as_bytes());
                        continue;
                    }

                    let telemetry_enabled = is_telemetry_enabled();

                    let (result_string, http_status) = if !telemetry_enabled {
                        ("SKIPPED: feature disabled".to_string(), "HTTP/1.1 200 OK")
                    } else {
                        let port_result = check_telemetry_port();
                        let ok = port_result.starts_with("OK");
                        let status_line = if ok {
                            "HTTP/1.1 200 OK"
                        } else {
                            "HTTP/1.1 500 Internal Server Error"
                        };
                        (port_result, status_line)
                    };

                    let status = HealthStatus {
                        check_telemetry_port: result_string,
                    };

                    let json_body = serde_json::to_string(&status).unwrap();
                    let response = format!(
                        "{http_status}\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
                        json_body.len(),
                        json_body
                    );

                    if let Err(e) = stream.write_all(response.as_bytes()) {
                        eprintln!("Failed to write response: {}", e);
                    }
                }
            }
            Err(e) => eprintln!("Error accepting connection: {}", e),
        }
    }
}
