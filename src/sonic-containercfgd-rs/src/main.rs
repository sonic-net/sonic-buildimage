mod db_listener;
mod syslog_config_updater;
mod timezone_updater;

use std::{env, io::ErrorKind, os::fd::AsRawFd};

use anyhow::Context;
use clap::Parser;
use epoll::{Event, Events};
use libc::EPOLLIN;
#[cfg(debug_assertions)]
use tracing::Level;

use crate::{
    db_listener::SonicDatabaseListener, syslog_config_updater::SyslogConfigUpdater,
    timezone_updater::TimezoneUpdater,
};

/// Modify the container configuration based on changes to CONFIG_DB
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Name of this service/container. Defaults to basing it off of NAMESPACE_ID and CONTAINER_NAME
    /// if not specified
    #[arg(long)]
    service_name: Option<String>,
}

fn main() -> anyhow::Result<()> {
    // Get the binary name from the command line
    let bin_path = std::env::args().next().unwrap();
    // In case this is a full path (which it will be when executed by supervisord), only get the
    // file name from the path.
    let mut bin_name = bin_path.rsplit('/').next().unwrap();
    if bin_name.is_empty() {
        // Use a default value if it's going to be an empty string otherwise
        bin_name = "sonic-containercfgd-rs";
    }

    // Initialize syslog using a crate that uses the libc APIs. We need something that can handle
    // /dev/log being removed and recreated as rsyslogd gets restarted.
    let syslog = syslog_tracing::Syslog::new(
        std::ffi::CString::new(bin_name)?,
        syslog_tracing::Options::LOG_PID,
        syslog_tracing::Facility::Daemon,
    )
    .context("Failed to initialize syslog")?;
    let subscriber_builder = tracing_subscriber::fmt()
        .with_writer(syslog)
        .with_ansi(false)
        .with_target(false)
        .with_level(false)
        .without_time();
    #[cfg(debug_assertions)]
    subscriber_builder.with_max_level(Level::DEBUG).init();
    #[cfg(not(debug_assertions))]
    subscriber_builder.init();

    let args = Args::parse();
    let service_name = args
        .service_name
        .or_else(|| {
            let container_name = env::var("CONTAINER_NAME");
            match container_name {
                Ok(container_name) => {
                    let namespace_id = env::var("NAMESPACE_ID").ok();
                    match namespace_id {
                        Some(namespace_id) => {
                            if namespace_id.is_empty() {
                                Some(container_name)
                            } else {
                                Some(
                                    container_name
                                        .rsplit(&namespace_id)
                                        .next()
                                        .unwrap()
                                        .to_owned(),
                                )
                            }
                        }
                        None => Some(container_name),
                    }
                }
                Err(_) => None,
            }
        })
        .context(
            "Service name not set on command line and not available through environment variable",
        )?;

    let epoll_fd = epoll::create(true).context("Unable to create epoll instance")?;

    let syslog_config_updater = SyslogConfigUpdater::new(service_name)?;
    let mut syslog_db_listener =
        SonicDatabaseListener::new("CONFIG_DB", "SYSLOG_CONFIG_FEATURE", syslog_config_updater)?;
    syslog_db_listener.process_existing_data()?;
    epoll::ctl(
        epoll_fd,
        epoll::ControlOptions::EPOLL_CTL_ADD,
        syslog_db_listener.as_raw_fd(),
        epoll::Event {
            events: EPOLLIN as u32,
            data: 0,
        },
    )
    .context("Unable to add syslog config listener to epoll instance")?;

    let timezone_updater = TimezoneUpdater::new()?;
    let mut timezone_db_listener =
        SonicDatabaseListener::new("CONFIG_DB", "DEVICE_METADATA", timezone_updater)?;
    timezone_db_listener.process_existing_data()?;
    epoll::ctl(
        epoll_fd,
        epoll::ControlOptions::EPOLL_CTL_ADD,
        timezone_db_listener.as_raw_fd(),
        epoll::Event {
            events: EPOLLIN as u32,
            data: 1,
        },
    )
    .context("Unable to add timezone listener to epoll instance")?;

    let mut events: [Event; 4] = [Event::new(Events::empty(), 0); 4];

    loop {
        let epoll_wait_result = epoll::wait(epoll_fd, -1, &mut events);
        match epoll_wait_result {
            Ok(events_count) => {
                for event in &events[0..events_count] {
                    match event.data {
                        0 => {
                            syslog_db_listener.read_data()?;
                        }
                        1 => {
                            timezone_db_listener.read_data()?;
                        }
                        token => {
                            unreachable!("Unknown data token {} received!", token)
                        }
                    }
                }
            }
            Err(err) => {
                anyhow::ensure!(
                    err.kind() == ErrorKind::Interrupted,
                    "Unexpected error waiting on epoll events"
                )
            }
        }
    }
}
