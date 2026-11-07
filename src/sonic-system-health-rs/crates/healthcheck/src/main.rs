//! One system health check cycle for `show system-health`, printed as JSON with --json.
//!
//! Prints {"led": ..., "stat": {category: {object: {type, message, status}}}, "ignore": {"services", "devices"}}.
//! Exit codes: 0 done, 1 configuration file missing, 2 not root or internal error. Sets the LED; writes no DB and
//! sends no events.
//! Only the JSON goes to stdout. What the checks print (the vendor's Python, the LED messages the Python manager
//! printed) goes to stderr, which `show system-health` prints before its report; this binary's own messages go to
//! syslog, and to stderr only when it fails.

use std::fs::File;
use std::io::Write;
use std::os::fd::AsFd;
use std::path::PathBuf;
use std::process::ExitCode;

use anyhow::Result;
use serde::Serialize;
use sonic_rs_common::DeviceInfo;
use system_health::command::TokioCommandRunner;
use system_health::config::{platform_name, Config};
use system_health::context::CheckContext;
use system_health::db::{ConfigDb, SwssConfigDb, SwssStateDb};
use system_health::docker::BollardRuntime;
use system_health::events::NoEvents;
use system_health::led::{PlatformLed, StatusLed};
use system_health::manager::HealthCheckManager;
use system_health::status::Stats;

const DEVICE_PATH: &str = "/usr/share/sonic/device";

#[derive(Serialize)]
struct Ignore {
    services: Vec<String>,
    devices: Vec<String>,
}

#[derive(Serialize)]
struct Output {
    led: String,
    stat: Stats,
    ignore: Ignore,
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args != ["--json"] {
        eprintln!("usage: {} --json", env!("CARGO_PKG_NAME"));
        return ExitCode::from(2);
    }
    // As `show system-health`: as another user the vendor's LED writes fail and log errors.
    if !nix::unistd::geteuid().is_root() {
        eprintln!("Root privileges are required for this operation");
        return ExitCode::from(2);
    }
    let mut json_out = match stdout_for_json() {
        Ok(out) => out,
        Err(e) => {
            eprintln!("{e}");
            return ExitCode::from(2);
        }
    };
    let device = DeviceInfo::new();
    let platform = platform_name(&device, || {
        SwssConfigDb::connect().ok()?.get_table("DEVICE_METADATA").ok()
    });
    let config = Config::for_platform(&PathBuf::from(DEVICE_PATH), &platform);
    if !config.config_file_exists() {
        eprintln!("System health configuration file not found, exit...");
        return ExitCode::from(1);
    }
    let led = match PlatformLed::open(env!("CARGO_PKG_NAME")) {
        Ok(led) => led,
        Err(e) => {
            eprintln!("Failed to open the platform API: {e}");
            return ExitCode::from(2);
        }
    };
    let runtime = match tokio::runtime::Builder::new_current_thread().enable_all().build() {
        Ok(runtime) => runtime,
        Err(e) => {
            eprintln!("{e}");
            return ExitCode::from(2);
        }
    };
    match runtime.block_on(run(&device, config, led)) {
        Ok(output) => match writeln!(json_out, "{output}") {
            Ok(()) => ExitCode::SUCCESS,
            Err(e) => {
                eprintln!("{e}");
                ExitCode::from(2)
            }
        },
        Err(e) => {
            eprintln!("{e:#}");
            ExitCode::from(2)
        }
    }
}

/// A copy of stdout for the JSON; stdout itself then goes to stderr.
fn stdout_for_json() -> std::io::Result<File> {
    let json = std::io::stdout().as_fd().try_clone_to_owned()?;
    nix::unistd::dup2(2, 1)?;
    Ok(File::from(json))
}

/// The check, then the vendor's exit handlers, however the check ended: Python runs them at any exit.
async fn run(device: &DeviceInfo, config: Config, mut led: PlatformLed) -> Result<String> {
    let output = check(device, config, &mut led).await;
    led.close().await;
    output
}

async fn check(device: &DeviceInfo, config: Config, led: &mut PlatformLed) -> Result<String> {
    let state_db = SwssStateDb::connect()?;
    let config_db = SwssConfigDb::connect()?;
    let docker = BollardRuntime::default();
    let ctx = CheckContext {
        state_db: &state_db,
        config_db: &config_db,
        docker: &docker,
        commands: &TokioCommandRunner,
        events: &NoEvents,
        device,
        proc_root: PathBuf::from("/proc"),
    };
    let mut manager = HealthCheckManager::new(config);
    let result = manager.check(&ctx, led).await;
    // As the Python CLI: the LED is initialized after the check, then read.
    led.initialize().await;
    let output = Output {
        led: led.get_status().await,
        stat: result.stats,
        ignore: Ignore {
            services: manager.config.ignore_services.clone().unwrap_or_default(),
            devices: manager.config.ignore_devices.clone().unwrap_or_default(),
        },
    };
    Ok(serde_json::to_string(&output)?)
}
