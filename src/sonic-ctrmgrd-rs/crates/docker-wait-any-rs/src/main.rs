use clap::Parser;
use docker_wait_any_rs::{run_main, BollardDockerApi, Error};
use std::process;
use std::sync::Arc;
use tracing_subscriber::prelude::*;
use syslog_tracing;
use std::ffi::CString;

#[derive(Parser)]
#[command(
    name = "docker-wait-any-rs",
    version,
    about = "Wait for dependent docker services"
)]
struct Cli {
    #[arg(
        short = 's',
        long = "service",
        num_args = 1..,
        help = "name of the service"
    )]
    service: Option<Vec<String>>,

    #[arg(
        short = 'd', 
        long = "dependent",
        num_args = 0..,
        help = "other dependent services"
    )]
    dependent: Option<Vec<String>>,
}


#[tokio::main]
async fn main() -> Result<(), Error> {
    let identity = CString::new("docker-wait-any-rs")
        .map_err(|e| Error::Syslog(format!("invalid identity string: {}", e)))?;
    let syslog = syslog_tracing::Syslog::new(
        identity,
        syslog_tracing::Options::LOG_PID,
        syslog_tracing::Facility::Daemon
    ).ok_or_else(|| Error::Syslog("failed to initialize syslog".to_string()))?;
    tracing_subscriber::fmt()
        .with_writer(syslog)
        .with_ansi(false)
        .with_target(false)
        .with_level(false)
        .without_time()
        .init();

    let cli = Cli::parse();
    let docker_client = Arc::new(BollardDockerApi::new()?);
    let exit_code = run_main(docker_client, cli.service, cli.dependent).await?;
    process::exit(exit_code);
}