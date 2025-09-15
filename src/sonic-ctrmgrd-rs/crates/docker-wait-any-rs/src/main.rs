use clap::Parser;
use docker_wait_any_rs::{run_main, BollardDockerApi, Error};
use std::process;
use std::sync::Arc;

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
    let cli = Cli::parse();
    let docker_client = Arc::new(BollardDockerApi::new()?);
    let exit_code = run_main(docker_client, cli.service, cli.dependent).await?;
    process::exit(exit_code);
}