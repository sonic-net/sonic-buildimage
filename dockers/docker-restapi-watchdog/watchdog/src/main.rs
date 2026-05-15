use std::io::{BufRead, BufReader, Write};
use std::net::TcpListener;
use std::time::Duration;
use std::path::Path;

use serde::Serialize;
use redis::{Commands, Connection};

const CONFIG_DB: i32 = 4;
const REDIS_PORT: i32 = 6379;
const WATCHDOG_PORT: i32 = 50100;
const RESTAPI_HTTPS_PORT: i32 = 8081;
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

struct CertPaths {
    ca_crt: String,
    server_crt: String,
    server_key: String,
}

// Reads certificate paths from Redis and returns them as a CertPaths struct.
// If any of the paths are missing or if there's a Redis error, returns None.
fn read_cert_paths_from_redis() -> Option<CertPaths> {
    // Connect to Redis
    let mut conn = match redis_connect() {
        Some(c) => c,
        None => { eprintln!("Failed to connect to Redis. Assuming certificates do not exist."); return None; }
    };
    // Read the certificate and key paths from Redis
    let root_cert_path = match redis_hget(&mut conn, RESTAPI_CERTS, "ca_crt") {
        Some(path) => path,
        None => { eprintln!("Root certificate path not found in Redis. Assuming the cert does not exist."); return None; }
    };
    let server_cert_path = match redis_hget(&mut conn, RESTAPI_CERTS, "server_crt") {
        Some(path) => path,
        None => { eprintln!("Server certificate path not found in Redis. Assuming the cert does not exist."); return None; }
    };
    let server_key_path = match redis_hget(&mut conn, RESTAPI_CERTS, "server_key") {
        Some(path) => path,
        None => { eprintln!("Server key path not found in Redis. Assuming the key does not exist."); return None; }
    };
    Some(CertPaths { ca_crt: root_cert_path, server_crt: server_cert_path, server_key: server_key_path })
}

// Check if root cert, server cert, and server key exist
fn check_certificates(cert_paths_opt: Option<CertPaths>) -> bool {
    let cert_paths = match cert_paths_opt {
        Some(paths) => paths,
        None => return false,
    };
    let paths = [
        cert_paths.ca_crt.as_str(),
        cert_paths.server_crt.as_str(),
        cert_paths.server_key.as_str(),
    ];

    paths.iter().all(|path|
        if path.starts_with(DEFAULT_RESTAPI_CERT_DIR) {
            Path::new(path).exists()
        } else {
            println!("The path {path} is outside the default directory.");
            false
        }
    )
}

// Checks restapi status by sending a GET request to the restapi HTTPS server.
// Uses the root CA cert to authenticate the server, and sends the server cert and key as
// client identity to the server.
// Pre-condition: All cert paths start with DEFAULT_RESTAPI_CERT_DIR and point to existing files.
fn check_restapi_status(cert_paths: CertPaths) -> String {
    let url = format!("https://127.0.0.1:{}/v1/state/heartbeat", RESTAPI_HTTPS_PORT);
    let timeout = Duration::from_secs(5);

    let ca_pem = match std::fs::read(&cert_paths.ca_crt) {
        Ok(b) => b,
        Err(e) => return format!("ERROR: failed to read CA cert {}: {}", cert_paths.ca_crt, e),
    };
    let ca_cert = match reqwest::Certificate::from_pem(&ca_pem) {
        Ok(c) => c,
        Err(e) => return format!("ERROR: failed to parse CA cert: {}", e),
    };

    let client_cert_pem = match std::fs::read(&cert_paths.server_crt) {
        Ok(b) => b,
        Err(e) => return format!("ERROR: failed to read client cert {}: {}", cert_paths.server_crt, e),
    };
    let client_key_pem = match std::fs::read(&cert_paths.server_key) {
        Ok(b) => b,
        Err(e) => return format!("ERROR: failed to read client key {}: {}", cert_paths.server_key, e),
    };
    let mut identity_pem = client_cert_pem;
    identity_pem.extend_from_slice(b"\n");
    identity_pem.extend_from_slice(&client_key_pem);
    let identity = match reqwest::Identity::from_pem(&identity_pem) {
        Ok(i) => i,
        Err(e) => return format!("ERROR: failed to build client identity: {}", e),
    };

    let client = match reqwest::blocking::Client::builder()
        .timeout(timeout)
        .add_root_certificate(ca_cert)
        .identity(identity)
        .build()
    {
        Ok(c) => c,
        Err(e) => return format!("ERROR: failed to build HTTPS client: {}", e),
    };

    match client.get(&url).send() {
        Ok(resp) => {
            let status = resp.status();
            if status.as_u16() == 200 {
                "OK".to_string()
            } else {
                format!("ERROR: unexpected HTTP status {}", status)
            }
        },
        Err(e) => format!("ERROR: {}", e),
    }
}

fn main() {
    // Start a HTTP server listening on port 50100
    let listener = TcpListener::bind(format!("127.0.0.1:{}", WATCHDOG_PORT))
        .expect(&format!("Failed to bind to 127.0.0.1:{}", WATCHDOG_PORT));

    println!("Watchdog HTTP server running on http://127.0.0.1:{}", WATCHDOG_PORT);

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

                    let cert_paths = read_cert_paths_from_redis();
                    let certs_exist = check_certificates(cert_paths);
                    let restapi_result = if !certs_exist {
                        println!("Skipping restapi connectivity check.");
                        "OK".to_string()
                    } else {
                        check_restapi_status(cert_paths.unwrap())
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
