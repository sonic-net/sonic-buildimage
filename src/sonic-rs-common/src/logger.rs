use syslog::{Facility, Severity};
use std::env;
use std::path::Path;

#[derive(thiserror::Error, Debug)]
pub enum LoggerError {
    #[error("Syslog error")]
    Syslog(#[from] syslog::Error),
}

pub type LoggerResult<T> = std::result::Result<T, LoggerError>;

pub struct Logger {
    writer: Option<syslog::Logger<syslog::LoggerBackend, syslog::Formatter3164>>,
    min_log_priority: Severity,
}

impl Logger {
    pub const LOG_FACILITY_DAEMON: Facility = Facility::LOG_DAEMON;
    pub const LOG_FACILITY_USER: Facility = Facility::LOG_USER;

    pub const LOG_PRIORITY_ERROR: Severity = Severity::LOG_ERR;
    pub const LOG_PRIORITY_WARNING: Severity = Severity::LOG_WARNING;
    pub const LOG_PRIORITY_NOTICE: Severity = Severity::LOG_NOTICE;
    pub const LOG_PRIORITY_INFO: Severity = Severity::LOG_INFO;
    pub const LOG_PRIORITY_DEBUG: Severity = Severity::LOG_DEBUG;

    pub const DEFAULT_LOG_FACILITY: Facility = Self::LOG_FACILITY_USER;

    pub fn new(log_identifier: Option<String>) -> LoggerResult<Self> {
        Self::new_with_options(log_identifier, Self::DEFAULT_LOG_FACILITY)
    }

    pub fn new_with_options(log_identifier: Option<String>, log_facility: Facility) -> LoggerResult<Self> {
        let identifier = match log_identifier {
            Some(id) => id,
            None => {
                let args: Vec<String> = env::args().collect();
                Path::new(args.first().unwrap_or(&String::new()))
                    .file_name()
                    .and_then(|name| name.to_str())
                    .unwrap_or("")
                    .to_string()
            }
        };

        let formatter = syslog::Formatter3164 {
            facility: log_facility,
            hostname: None,
            process: identifier,
            pid: std::process::id(),
        };

        // Try to create syslog writer, but don't fail if it doesn't work
        let writer = syslog::unix(formatter).ok();

        Ok(Logger {
            writer,
            min_log_priority: Self::LOG_PRIORITY_NOTICE,
        })
    }

    pub fn set_min_log_priority(&mut self, priority: Severity) {
        self.min_log_priority = priority;
    }

    pub fn set_min_log_priority_error(&mut self) {
        self.set_min_log_priority(Self::LOG_PRIORITY_ERROR);
    }

    pub fn set_min_log_priority_warning(&mut self) {
        self.set_min_log_priority(Self::LOG_PRIORITY_WARNING);
    }

    pub fn set_min_log_priority_notice(&mut self) {
        self.set_min_log_priority(Self::LOG_PRIORITY_NOTICE);
    }

    pub fn set_min_log_priority_info(&mut self) {
        self.set_min_log_priority(Self::LOG_PRIORITY_INFO);
    }

    pub fn set_min_log_priority_debug(&mut self) {
        self.set_min_log_priority(Self::LOG_PRIORITY_DEBUG);
    }

    pub fn log(&mut self, priority: Severity, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        if self.min_log_priority as i32 >= priority as i32 {
            // Only log to syslog if writer is available
            if let Some(ref mut writer) = self.writer {
                match priority {
                    Severity::LOG_ERR => writer.err(msg)?,
                    Severity::LOG_WARNING => writer.warning(msg)?,
                    Severity::LOG_NOTICE => writer.notice(msg)?,
                    Severity::LOG_INFO => writer.info(msg)?,
                    Severity::LOG_DEBUG => writer.debug(msg)?,
                    _ => writer.info(msg)?,
                }
            }

            if also_print_to_console {
                println!("{}", msg);
            }
        }
        Ok(())
    }

    pub fn log_error(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Self::LOG_PRIORITY_ERROR, msg, also_print_to_console)
    }

    pub fn log_warning(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Self::LOG_PRIORITY_WARNING, msg, also_print_to_console)
    }

    pub fn log_notice(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Self::LOG_PRIORITY_NOTICE, msg, also_print_to_console)
    }

    pub fn log_info(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Self::LOG_PRIORITY_INFO, msg, also_print_to_console)
    }

    pub fn log_debug(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Self::LOG_PRIORITY_DEBUG, msg, also_print_to_console)
    }
}