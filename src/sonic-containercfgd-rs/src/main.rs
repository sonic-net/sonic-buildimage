mod db_listener;
mod syslog_config_updater;
mod timezone_updater;

use std::{io::ErrorKind, os::fd::AsRawFd};

#[cfg(not(test))]
use std::env;

#[cfg(test)]
use tests::env;

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

fn get_syslog_name() -> String {
    // Get the binary name from the command line
    let bin_path = env::args().next().unwrap();
    // In case this is a full path (which it will be when executed by supervisord), only get the
    // file name from the path.
    let mut bin_name = bin_path.rsplit('/').next().unwrap();
    if bin_name.is_empty() {
        // Use a default value if it's going to be an empty string otherwise
        bin_name = "sonic-containercfgd-rs";
    }
    bin_name.to_owned()
}

fn setup_syslog() -> anyhow::Result<()> {
    let bin_name = get_syslog_name();

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
    Ok(())
}

fn listen_for_changes(service_name: &str, epoll_fd: i32) -> anyhow::Result<()> {
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

fn get_service_name(args: &Args) -> anyhow::Result<String> {
    args.service_name
        .clone()
        .or_else(|| {
            let container_name = env::var("CONTAINER_NAME");
            match container_name {
                Ok(container_name) => {
                    let namespace_id = env::var("NAMESPACE_ID");
                    match namespace_id {
                        Ok(namespace_id) => {
                            if namespace_id.is_empty() {
                                Some(container_name)
                            } else if container_name.ends_with(&namespace_id) {
                                let mut container_name_chars = container_name.chars();
                                container_name_chars.nth_back(namespace_id.len() - 1);
                                Some(container_name_chars.collect())
                            } else {
                                Some(container_name)
                            }
                        }
                        Err(_) => Some(container_name),
                    }
                }
                Err(_) => None,
            }
        })
        .context(
            "Service name not set on command line and not available through environment variable",
        )
}

fn main() -> anyhow::Result<()> {
    setup_syslog()?;

    let args = Args::parse();
    let service_name = get_service_name(&args)?;

    let epoll_fd = epoll::create(true).context("Unable to create epoll instance")?;

    listen_for_changes(&service_name, epoll_fd)
}

#[cfg(test)]
mod tests {
    use std::{cell::RefCell, collections::HashMap, ffi::OsString, str::FromStr};

    use super::*;

    thread_local! {
        static ENVIRONMENT_VARIABLES: RefCell<Option<HashMap<&str, String>>> = const { RefCell::new(None) };
        static PROCESS_ARGS: RefCell<Vec<OsString>> = const { RefCell::new(vec![]) };
    }

    #[test]
    fn autodetect_service_name_for_single_asic() {
        ENVIRONMENT_VARIABLES.replace(Some(HashMap::from([("CONTAINER_NAME", "bgp".to_owned())])));
        let args = Args { service_name: None };
        assert_eq!(get_service_name(&args).unwrap(), "bgp");
    }

    #[test]
    fn autodetect_service_name_for_multi_asic() {
        ENVIRONMENT_VARIABLES.replace(Some(HashMap::from([
            ("CONTAINER_NAME", "bgp0".to_owned()),
            ("NAMESPACE_ID", "0".to_owned()),
        ])));
        let args = Args { service_name: None };
        assert_eq!(get_service_name(&args).unwrap(), "bgp");
    }

    #[test]
    fn manual_service_name_for_single_asic() {
        ENVIRONMENT_VARIABLES.replace(Some(HashMap::from([("CONTAINER_NAME", "bgp".to_owned())])));
        let args = Args {
            service_name: Some("bgp".to_owned()),
        };
        assert_eq!(get_service_name(&args).unwrap(), "bgp");
    }

    #[test]
    fn manual_service_name_for_multi_asic() {
        ENVIRONMENT_VARIABLES.replace(Some(HashMap::from([
            ("CONTAINER_NAME", "bgp0".to_owned()),
            ("NAMESPACE_ID", "0".to_owned()),
        ])));
        let args = Args {
            service_name: Some("bgp".to_owned()),
        };
        assert_eq!(get_service_name(&args).unwrap(), "bgp");
    }

    #[test]
    fn syslog_name_full_path() {
        PROCESS_ARGS.replace(vec![
            OsString::from_str("/usr/bin/sonic-containercfgd-rs").unwrap(),
        ]);
        assert_eq!(get_syslog_name(), "sonic-containercfgd-rs");
    }

    #[test]
    fn syslog_name_command_name() {
        PROCESS_ARGS.replace(vec![OsString::from_str("sonic-containercfgd-rs").unwrap()]);
        assert_eq!(get_syslog_name(), "sonic-containercfgd-rs");
    }

    #[test]
    fn syslog_name_alias_name() {
        PROCESS_ARGS.replace(vec![OsString::from_str("container-config").unwrap()]);
        assert_eq!(get_syslog_name(), "container-config");
    }

    #[test]
    fn syslog_name_alias_path() {
        PROCESS_ARGS.replace(vec![
            OsString::from_str("/usr/bin/container-config").unwrap(),
        ]);
        assert_eq!(get_syslog_name(), "container-config");
    }

    #[test]
    fn syslog_name_path_is_a_directory() {
        PROCESS_ARGS.replace(vec![OsString::from_str("/usr/bin/").unwrap()]);
        assert_eq!(get_syslog_name(), "sonic-containercfgd-rs");
    }

    #[test]
    fn syslog_name_empty_command() {
        // This might not actually be possible, but testing anyways
        PROCESS_ARGS.replace(vec![OsString::from_str("").unwrap()]);
        assert_eq!(get_syslog_name(), "sonic-containercfgd-rs");
    }

    pub mod env {

        use std::{
            env::VarError,
            ffi::{OsStr, OsString},
            vec,
        };

        use super::*;

        pub struct Args {
            inner: vec::IntoIter<OsString>,
        }

        impl Iterator for Args {
            type Item = String;

            fn next(&mut self) -> Option<String> {
                self.inner.next().map(|s| s.into_string().unwrap())
            }

            #[inline]
            fn size_hint(&self) -> (usize, Option<usize>) {
                self.inner.size_hint()
            }
        }

        pub fn args() -> Args {
            PROCESS_ARGS.with_borrow(|process_args| Args {
                inner: process_args.clone().into_iter(),
            })
        }

        pub fn var<K: AsRef<OsStr>>(key: K) -> Result<String, VarError> {
            ENVIRONMENT_VARIABLES.with_borrow(|env_variables| {
                if let Some(environment_variables) = env_variables {
                    environment_variables
                        .get(key.as_ref().to_str().unwrap())
                        .cloned()
                        .ok_or(VarError::NotPresent)
                } else {
                    Err(VarError::NotPresent)
                }
            })
        }
    }
}
