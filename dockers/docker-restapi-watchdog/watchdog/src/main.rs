use std::io::{BufRead, BufReader, Write};
use std::net::{TcpListener, TcpStream};
use std::time::Duration;
use std::path::Path;

use serde::Serialize;
use redis::{Commands, Connection};

const CONFIG_DB: i32 = 4;
const REDIS_PORT: i32 = 6379;
const RESTAPI_CERTS: &str = "RESTAPI|certs";
const DEFAULT_RESTAPI_CERT_DIR: &str = "/etc/sonic/credentials/";

#[derive(Serialize)]
struct HealthStatus {
    restapi_status: String,
}

// Opens a Redis connection to CONFIG DB.
// Returns None on any error (client creation or connection).
fn redis_connect() -> Option<Connection> {
    let client = match redis::Client::open(format!("redis://127.0.0.1:{}/{}", REDIS_PORT, CONFIG_DB)) {
        Ok(c) => c,
        Err(e) => { eprintln!("Redis client error: {e}"); return None; }
    };
    match client.get_connection() {
        Ok(conn) => Some(conn),
        Err(e) => { eprintln!("Redis connection error: {e}"); None }
    }
}

// Fetches a hash field from CONFIG DB.
// Returns None if HGET fails.
fn redis_hget(conn: &mut Connection, hash: &str, field: &str) -> Option<String> {
    match conn.hget::<_, _, Option<String>>(hash, field) {
        Ok(v) => v,
        Err(e) => { eprintln!("Redis HGET error {hash}.{field}: {e}"); None }
    }
}

// Check if root cert, server cert, and server key exist
fn check_certificates() -> bool {
    // Connect to Redis
    let mut conn = match redis_connect() {
        Some(c) => c,
        None => { eprintln!("Failed to connect to Redis. Assuming certificates do not exist."); return false; }
    };
    // Read the certificate and key paths from Redis
    let root_cert_path = match redis_hget(&mut conn, RESTAPI_CERTS, "ca_crt") {
        Some(path) => path,
        None => { eprintln!("Root certificate path not found in Redis. Assuming the cert does not exist."); return false; }
    };
    let server_cert_path = match redis_hget(&mut conn, RESTAPI_CERTS, "server_crt") {
        Some(path) => path,
        None => { eprintln!("Server certificate path not found in Redis. Assuming the cert does not exist."); return false; }
    };
    let server_key_path = match redis_hget(&mut conn, RESTAPI_CERTS, "server_key") {
        Some(path) => path,
        None => { eprintln!("Server key path not found in Redis. Assuming the key does not exist."); return false; }
    };
    let cert_paths = [
        root_cert_path.as_str(),
        server_cert_path.as_str(),
        server_key_path.as_str(),
    ];

    cert_paths.iter().all(|path|
        if path.starts_with(DEFAULT_RESTAPI_CERT_DIR) {
            Path::new(path).exists()
        } else {
            println!("The path {path} is outside the default directory. Assuming it exists.");
            true
        }
    )
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

                    let certs_exist = check_certificates();
                    let restapi_result = if !certs_exist {
                        println!("restapi is waiting for certificates.");
                        "OK".to_string()
                    } else {
                        check_restapi_status()
                    };

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
