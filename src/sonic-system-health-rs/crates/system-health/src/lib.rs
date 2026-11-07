//! SONiC system health: the checks behind healthd and `show system-health`.

pub mod command;
pub mod config;
pub mod context;
pub mod db;
pub mod docker;
pub mod events;
pub mod hardware;
pub mod led;
pub mod log;
pub mod manager;
pub mod monit;
pub mod natsort;
pub mod publisher;
pub mod pyfmt;
pub mod service;
pub mod status;
pub mod supervisor;
pub mod sysmonitor;
pub mod systemd;
pub mod template;
pub mod user_defined;

/// Timeout of every external call except user-defined checks.
pub const CALL_TIMEOUT: std::time::Duration = std::time::Duration::from_secs(15);
