//! Syslog with the identifiers the Python daemon logged with: its checkers and sysmonitor as `healthd`
//! (the daemon opened the log last), the utilities as `healthd#utils` with facility DAEMON.

use std::collections::{BTreeSet, HashMap};
use std::sync::{Mutex, OnceLock};
use std::time::{Duration, Instant};

use sonic_rs_common::Logger;

pub const HEALTHD: &str = "healthd";
pub const SERVICE_CHECKER: &str = HEALTHD;
pub const SYSMONITOR: &str = HEALTHD;
pub const UTILS: &str = "healthd#utils";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Level {
    Error,
    Warning,
    Notice,
    Info,
    Debug,
}

static LOGGERS: OnceLock<Mutex<HashMap<&'static str, Option<Logger>>>> = OnceLock::new();

pub fn log(identifier: &'static str, level: Level, msg: &str) {
    let loggers = LOGGERS.get_or_init(Default::default);
    let mut loggers = loggers.lock().unwrap_or_else(|e| e.into_inner());
    let Some(logger) = loggers
        .entry(identifier)
        .or_insert_with(|| new_logger(identifier))
        .as_mut()
    else {
        return;
    };
    let _ = match level {
        Level::Error => logger.log_error(msg, false),
        Level::Warning => logger.log_warning(msg, false),
        Level::Notice => logger.log_notice(msg, false),
        Level::Info => logger.log_info(msg, false),
        Level::Debug => logger.log_debug(msg, false),
    };
}

fn new_logger(identifier: &'static str) -> Option<Logger> {
    if identifier == UTILS {
        let mut logger = Logger::new_with_options(Some(identifier.to_string()), Logger::LOG_FACILITY_DAEMON).ok()?;
        logger.set_min_log_priority_info();
        return Some(logger);
    }
    Logger::new(Some(identifier.to_string())).ok()
}

pub fn error(identifier: &'static str, msg: &str) {
    log(identifier, Level::Error, msg)
}

pub fn warning(identifier: &'static str, msg: &str) {
    log(identifier, Level::Warning, msg)
}

pub fn notice(identifier: &'static str, msg: &str) {
    log(identifier, Level::Notice, msg)
}

pub fn info(identifier: &'static str, msg: &str) {
    log(identifier, Level::Info, msg)
}

pub fn debug(identifier: &'static str, msg: &str) {
    log(identifier, Level::Debug, msg)
}

/// A failure that repeats every few seconds, as a monitor retrying: ERR at most once per interval, DEBUG between.
pub struct RepeatedFailure {
    interval: Duration,
    last_error: Option<Instant>,
}

impl RepeatedFailure {
    pub fn new(interval: Duration) -> Self {
        Self {
            interval,
            last_error: None,
        }
    }

    pub fn log(&mut self, identifier: &'static str, msg: &str) {
        log(identifier, self.level_at(Instant::now()), msg)
    }

    /// The level of a failure at `now`; an ERR starts a new interval.
    fn level_at(&mut self, now: Instant) -> Level {
        if self
            .last_error
            .is_some_and(|last| now.duration_since(last) < self.interval)
        {
            return Level::Debug;
        }
        self.last_error = Some(now);
        Level::Error
    }
}

/// A condition retried every cycle, per key: WARNING when it starts for a key, DEBUG while it lasts.
#[derive(Default)]
pub struct WarnOnce {
    warned: BTreeSet<String>,
}

impl WarnOnce {
    pub fn log(&mut self, identifier: &'static str, key: &str, msg: &str) {
        log(identifier, self.level(key), msg)
    }

    fn level(&mut self, key: &str) -> Level {
        if self.warned.insert(key.to_string()) {
            Level::Warning
        } else {
            Level::Debug
        }
    }

    /// The condition ended for `key`; when it comes back, it is a WARNING again.
    pub fn clear(&mut self, key: &str) {
        self.warned.remove(key);
    }

    /// Keeps the keys for which `keep` holds; the others are cleared.
    pub fn retain(&mut self, keep: impl FnMut(&String) -> bool) {
        self.warned.retain(keep);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_repeated_failure_is_an_error_once_per_interval() {
        let mut failure = RepeatedFailure::new(Duration::from_secs(600));
        let start = Instant::now();
        let at = |secs| start + Duration::from_secs(secs);
        assert_eq!(failure.level_at(at(0)), Level::Error);
        assert_eq!(failure.level_at(at(5)), Level::Debug);
        assert_eq!(failure.level_at(at(599)), Level::Debug);
        assert_eq!(failure.level_at(at(600)), Level::Error);
        assert_eq!(failure.level_at(at(605)), Level::Debug);
        assert_eq!(failure.level_at(at(1200)), Level::Error);
    }

    #[test]
    fn a_retried_condition_is_a_warning_once_per_key_until_it_ends() {
        let mut once = WarnOnce::default();
        assert_eq!(once.level("dhcp_relay"), Level::Warning);
        assert_eq!(once.level("dhcp_relay"), Level::Debug);
        assert_eq!(once.level("dhcp_server"), Level::Warning);
        once.clear("dhcp_relay");
        assert_eq!(once.level("dhcp_relay"), Level::Warning);
        once.retain(|key| key != "dhcp_server");
        assert_eq!(once.level("dhcp_server"), Level::Warning);
        assert_eq!(once.level("dhcp_relay"), Level::Debug);
    }
}
