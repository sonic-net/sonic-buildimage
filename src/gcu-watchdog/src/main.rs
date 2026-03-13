// main.rs — GCU Watchdog gRPC server
//
// Purpose
// -------
// Monitor the integrity of the GCU Python venv installed inside the
// docker-gcu container at /opt/gcu-venv.  Specifically, watch all files
// under:
//     /opt/gcu-venv/lib/python*/site-packages/generic_config_updater/
//
// At startup the watchdog computes a baseline MD5 over those files.  A
// background Tokio task re-computes the checksum periodically and stores the
// result.  A foreground gRPC server (tonic) exposes GetStatus so that
// monitoring agents can query health at any time.
//
// Algorithm
// ---------
// 1. Walk the target directory recursively and collect (path, md5-of-file)
//    pairs, sorted deterministically by relative path.
// 2. Concatenate all per-file hex hashes into a single string and compute the
//    MD5 of that string — this is the "directory checksum".
// 3. Compare to the baseline recorded at container start.  If they differ,
//    report unhealthy with a detail message.
//
// gRPC schema — see proto/watchdog.proto:
//   service WatchdogService { rpc GetStatus(StatusRequest) -> StatusResponse }
//   StatusResponse { bool healthy; string detail; }

use std::sync::Arc;
use std::time::Duration;

use tokio::sync::RwLock;
use tonic::{transport::Server, Request, Response, Status};
use tracing::{error, info};
use walkdir::WalkDir;

// Signal handling: tokio::signal is included in the "full" feature set.
// SIGTERM is only available on Unix; on other platforms we only handle Ctrl-C.
#[cfg(unix)]
use tokio::signal::unix::{signal, SignalKind};

// Include the tonic-generated gRPC stubs (produced from proto/watchdog.proto
// by tonic-build in build.rs at compile time).
pub mod watchdog_proto {
    tonic::include_proto!("watchdog");
}

use watchdog_proto::watchdog_service_server::{WatchdogService, WatchdogServiceServer};
use watchdog_proto::{StatusRequest, StatusResponse};

// ---------------------------------------------------------------------------
// Checksum logic
// ---------------------------------------------------------------------------

/// Walk `dir_glob` (a shell glob such as
/// `/opt/gcu-venv/lib/python*/site-packages/generic_config_updater`)
/// and compute a deterministic MD5 over all regular files.
///
/// Files are sorted by their path relative to the first matching directory so
/// that the result is reproducible across runs regardless of filesystem order.
fn compute_venv_checksum(dir_glob: &str) -> Result<String, String> {
    // Expand the glob (handles the `python*` wildcard in the venv lib path).
    let paths: Vec<std::path::PathBuf> = glob::glob(dir_glob)
        .map_err(|e| format!("glob pattern error: {}", e))?
        .filter_map(|r| r.ok())
        .collect();

    if paths.is_empty() {
        return Err(format!(
            "No directories matched glob '{}' — GCU package may not be installed",
            dir_glob
        ));
    }

    // Collect all (relative_path, file_md5) pairs from each matched directory.
    let mut entries: Vec<(String, String)> = Vec::new();

    for base in &paths {
        for entry in WalkDir::new(base)
            .sort_by_file_name()
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_type().is_file())
        {
            let abs_path = entry.path();

            // MD5 of the individual file's contents
            let bytes = std::fs::read(abs_path)
                .map_err(|e| format!("Failed to read {}: {}", abs_path.display(), e))?;
            let file_hash = format!("{:x}", md5::compute(&bytes));

            // Store path relative to the venv root so the baseline is stable
            // even if the mount point changes.
            let rel = abs_path
                .strip_prefix(base)
                .unwrap_or(abs_path)
                .to_string_lossy()
                .to_string();
            entries.push((rel, file_hash));
        }
    }

    // Sort deterministically (WalkDir already sorts by name, but sort again
    // across multiple matching base directories).
    entries.sort_by(|a, b| a.0.cmp(&b.0));

    // Build a combined string and hash it.
    let mut combined = String::with_capacity(entries.len() * 64);
    for (p, h) in &entries {
        combined.push_str(p);
        combined.push(':');
        combined.push_str(h);
        combined.push('\n');
    }
    let dir_hash = format!("{:x}", md5::compute(combined.as_bytes()));

    Ok(dir_hash)
}

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------

#[derive(Debug, Clone)]
struct HealthState {
    healthy: bool,
    detail: String,
}

// ---------------------------------------------------------------------------
// gRPC service implementation
// ---------------------------------------------------------------------------

struct GcuWatchdogService {
    state: Arc<RwLock<HealthState>>,
}

#[tonic::async_trait]
impl WatchdogService for GcuWatchdogService {
    async fn get_status(
        &self,
        _request: Request<StatusRequest>,
    ) -> Result<Response<StatusResponse>, Status> {
        let s = self.state.read().await;
        Ok(Response::new(StatusResponse {
            healthy: s.healthy,
            detail: s.detail.clone(),
        }))
    }
}

// ---------------------------------------------------------------------------
// Background check loop
// ---------------------------------------------------------------------------

async fn run_check_loop(
    state: Arc<RwLock<HealthState>>,
    baseline: String,
    dir_glob: String,
    interval_secs: u64,
) {
    loop {
        tokio::time::sleep(Duration::from_secs(interval_secs)).await;

        match tokio::task::spawn_blocking({
            let g = dir_glob.clone();
            move || compute_venv_checksum(&g)
        })
        .await
        {
            Ok(Ok(current)) => {
                let mut s = state.write().await;
                if current == baseline {
                    s.healthy = true;
                    s.detail = "OK".to_string();
                    info!("GCU venv checksum OK ({})", current);
                } else {
                    s.healthy = false;
                    s.detail = format!(
                        "checksum mismatch: expected {}, got {}",
                        baseline, current
                    );
                    error!("{}", s.detail);
                }
            }
            Ok(Err(e)) => {
                let mut s = state.write().await;
                s.healthy = false;
                s.detail = format!("checksum error: {}", e);
                error!("{}", s.detail);
            }
            Err(e) => {
                error!("spawn_blocking panicked: {}", e);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialise structured logging; respect RUST_LOG env var.
    tracing_subscriber::fmt::init();

    let dir_glob =
        "/opt/gcu-venv/lib/python*/site-packages/generic_config_updater".to_string();
    let grpc_addr = "[::]:50051".parse()?;
    // Default to 60 seconds; override with GCU_WATCHDOG_INTERVAL_SECS env var.
    let check_interval_secs: u64 = std::env::var("GCU_WATCHDOG_INTERVAL_SECS")
        .unwrap_or_else(|_| "60".to_string())
        .parse()
        .unwrap_or(60);

    // Compute baseline checksum at startup.
    info!("Computing baseline GCU venv checksum from '{}'", dir_glob);
    let baseline = {
        let g = dir_glob.clone();
        tokio::task::spawn_blocking(move || compute_venv_checksum(&g))
            .await??
    };
    info!("Baseline checksum: {}", baseline);

    let initial_state = HealthState {
        healthy: true,
        detail: "OK".to_string(),
    };
    let state = Arc::new(RwLock::new(initial_state));

    // Spawn the background check loop.
    tokio::spawn(run_check_loop(
        Arc::clone(&state),
        baseline,
        dir_glob,
        check_interval_secs,
    ));

    // Serve gRPC on the foreground thread.
    let svc = GcuWatchdogService {
        state: Arc::clone(&state),
    };

    info!("GCU watchdog gRPC server listening on {}", grpc_addr);

    // Register a SIGTERM handler so supervisord can stop the process cleanly.
    // tokio::signal::unix requires the "signal" feature (included in "full").
    // Gated to Unix because the API is unavailable on other platforms.
    #[cfg(unix)]
    let mut sigterm = signal(SignalKind::terminate())
        .expect("Failed to install SIGTERM handler");

    let server_future = Server::builder()
        .add_service(WatchdogServiceServer::new(svc))
        .serve(grpc_addr);

    // Run the gRPC server until it completes or until SIGTERM/SIGINT is received.
    #[cfg(unix)]
    tokio::select! {
        result = server_future => {
            if let Err(e) = result {
                error!("gRPC server error: {}", e);
            }
        }
        _ = sigterm.recv() => {
            info!("Received SIGTERM, shutting down GCU watchdog gracefully");
        }
        _ = tokio::signal::ctrl_c() => {
            info!("Received SIGINT, shutting down GCU watchdog gracefully");
        }
    }

    #[cfg(not(unix))]
    tokio::select! {
        result = server_future => {
            if let Err(e) = result {
                error!("gRPC server error: {}", e);
            }
        }
        _ = tokio::signal::ctrl_c() => {
            info!("Received SIGINT, shutting down GCU watchdog gracefully");
        }
    }

    Ok(())
}
