//! Everything a check cycle talks to, so tests can replace each part.

use std::path::{Path, PathBuf};

use sonic_rs_common::DeviceInfo;

use crate::command::CommandRunner;
use crate::db::{ConfigDb, RedisErrors, StateDb};
use crate::docker::ContainerRuntime;
use crate::events::EventSink;

pub struct CheckContext<'a> {
    pub state_db: &'a dyn StateDb,
    pub config_db: &'a dyn ConfigDb,
    pub docker: &'a dyn ContainerRuntime,
    pub commands: &'a dyn CommandRunner,
    pub events: &'a dyn EventSink,
    pub device: &'a DeviceInfo,
    /// "/proc" on a device.
    pub proc_root: PathBuf,
    /// healthd's Redis connection errors: after one, a cycle sets no LED, and healthd exits. None in the CLI, which
    /// reports the error in its results, as the Python CLI did.
    pub redis_errors: Option<&'a RedisErrors>,
}

impl CheckContext<'_> {
    /// Whether healthd's Redis connections have failed.
    pub fn redis_failed(&self) -> bool {
        self.redis_errors.is_some_and(|errors| errors.first().is_some())
    }

    /// System uptime in seconds.
    pub fn uptime(&self) -> f64 {
        read_uptime(&self.proc_root.join("uptime")).unwrap_or(0.0)
    }
}

fn read_uptime(path: &Path) -> Option<f64> {
    std::fs::read_to_string(path)
        .ok()?
        .split_whitespace()
        .next()?
        .parse()
        .ok()
}
