use clap::{Parser, ValueEnum};
use container::Container;
use tracing::info;
use syslog_tracing;
use std::ffi::CString;
mod container;

// Mimic python container script `FAILURE = -1`
const FAILURE: i32 = -1;

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum)]
enum Action {
    Start,
    Stop,
    Kill,
    Wait,
}

#[derive(Parser)]
#[command(version, about, long_about = None)]
struct Cli {
    #[arg(value_enum)]
    /// The action to take for the container
    action: Action,

    /// The name of the container
    name: String,

    /// Timeout for the action to occur
    #[arg(short, long)]
    timeout: Option<i64>,
}

#[tokio::main]
async fn main() -> Result<(), container::Error> {
    let identity = CString::new("docker-rs")
        .map_err(|e| container::Error::Syslog(format!("invalid identity string: {}", e)))?;
    let syslog = syslog_tracing::Syslog::new(
        identity,
        syslog_tracing::Options::LOG_PID,
        syslog_tracing::Facility::Daemon
    ).ok_or_else(|| container::Error::Syslog("failed to initialize syslog".to_string()))?;
    tracing_subscriber::fmt()
        .with_writer(syslog)
        .with_ansi(false)
        .with_target(false)
        .with_level(false)
        .without_time()
        .init();

    let cli = Cli::parse();

    let container = Container::new(&cli.name);

    let result = match cli.action {
        Action::Start => container
            .start()
            .await
            .inspect_err(|e| info!("Unable to start container: {e}")),
        Action::Wait => container
            .wait()
            .await
            .inspect_err(|e| info!("Unable to wait on container: {e}")),
        Action::Stop => container
            .stop(cli.timeout)
            .await
            .inspect_err(|e| info!("Unable to stop container: {e}")),
        Action::Kill => container
            .kill()
            .await
            .inspect_err(|e| info!("Unable to kill container: {e}")),
    };

    if let Err(e) = result {
        // Don't exit with failure for wait operations when container was killed (exit code 137)
        if matches!(cli.action, Action::Wait) {
            if let container::Error::Docker(bollard::errors::Error::DockerContainerWaitError { code: 137, .. }) = e {
                return Ok(());
            }
        }
        std::process::exit(FAILURE);
    }
    Ok(())
}
