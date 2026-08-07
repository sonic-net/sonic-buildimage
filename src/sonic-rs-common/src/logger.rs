use std::ffi::CString;
use std::sync::Mutex;

#[derive(thiserror::Error, Debug)]
pub enum LoggerError {
    #[error("Only one instance is allowed")]
    MultipleInstances,
    #[error("Invalid log identifier: {0}")]
    InvalidIdentifier(String),
}

#[repr(i32)]
#[derive(Copy, Clone)]
pub enum Facility {
    Daemon = libc::LOG_DAEMON,
    User = libc::LOG_USER,
}

#[repr(i32)]
#[derive(Copy, Clone)]
pub enum Severity {
    Emergency = libc::LOG_EMERG,
    Alert = libc::LOG_ALERT,
    Critical = libc::LOG_CRIT,
    Error = libc::LOG_ERR,
    Warning = libc::LOG_WARNING,
    Notice = libc::LOG_NOTICE,
    Info = libc::LOG_INFO,
    Debug = libc::LOG_DEBUG,
}

pub type LoggerResult<T> = std::result::Result<T, LoggerError>;

pub struct Logger {
    log_facility: Facility,
    min_log_priority: Severity,
    _ident: Option<CString>,
}

// Due to how libc::openlog() behaves, we can only have one instance of this class at any given time
// This mutex ensures that
static LOGGER_GUARD: Mutex<bool> = Mutex::new(false);

impl Logger {
    const DEFAULT_LOG_FACILITY: Facility = Facility::User;
    // Format specifier for libc::syslog()
    const SYSLOG_FMT: &'static core::ffi::CStr = cstr::cstr!(b"%s");

    pub fn new(log_identifier: Option<String>) -> LoggerResult<Self> {
        Self::new_with_options(log_identifier, Self::DEFAULT_LOG_FACILITY)
    }

    pub fn new_with_options(log_identifier: Option<String>, log_facility: Facility) -> LoggerResult<Self> {
        let mut guard = match LOGGER_GUARD.lock () {
            Ok(guard) => guard,
            Err(poisoned) => {
                let mut guard = poisoned.into_inner();
                // This lock will only get poisoned if a panic occurs when creating or dropping the logger.
                // It is safe to assume that if we panic when creating it, it was never created in the first place,
                // thus we can call openlog() again.
                *guard = false;
                guard
            },
        };

        if *guard {
            return Err(LoggerError::MultipleInstances);
        }

        let ident = match log_identifier {
            Some(id) => {
                let c_ident = CString::new(id)
                    .map_err(|e| LoggerError::InvalidIdentifier(e.to_string()))?;

                unsafe {
                    libc::openlog(c_ident.as_ptr(), 0, log_facility as i32);
                }

                Some(c_ident)
            }
            None => None,
        };

        *guard = true;

        Ok(Logger {
            log_facility,
            min_log_priority: Severity::Notice,
            _ident: ident,
        })
    }

    pub fn set_min_log_priority(&mut self, priority: Severity) {
        self.min_log_priority = priority;
    }

    pub fn set_min_log_priority_error(&mut self) {
        self.set_min_log_priority(Severity::Error);
    }

    pub fn set_min_log_priority_warning(&mut self) {
        self.set_min_log_priority(Severity::Warning);
    }

    pub fn set_min_log_priority_notice(&mut self) {
        self.set_min_log_priority(Severity::Notice);
    }

    pub fn set_min_log_priority_info(&mut self) {
        self.set_min_log_priority(Severity::Info);
    }

    pub fn set_min_log_priority_debug(&mut self) {
        self.set_min_log_priority(Severity::Debug);
    }

    pub fn log(&mut self, priority: Severity, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        if self.min_log_priority as i32 >= priority as i32 {
            let c_msg = CString::new(msg).unwrap_or_default();

            unsafe {
                libc::syslog(self.log_facility as i32 | priority as i32, Self::SYSLOG_FMT.as_ptr(), c_msg.as_ptr());
            }

            if also_print_to_console {
                println!("{}", msg);
            }
        }

        Ok(())
    }

    pub fn log_error(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Severity::Error, msg, also_print_to_console)
    }

    pub fn log_warning(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Severity::Warning, msg, also_print_to_console)
    }

    pub fn log_notice(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Severity::Notice, msg, also_print_to_console)
    }

    pub fn log_info(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Severity::Info, msg, also_print_to_console)
    }

    pub fn log_debug(&mut self, msg: &str, also_print_to_console: bool) -> LoggerResult<()> {
        self.log(Severity::Debug, msg, also_print_to_console)
    }
}

impl Drop for Logger {
    fn drop(&mut self) {
        unsafe {
            libc::closelog();
        }

        if let Ok(mut guard) = LOGGER_GUARD.lock() {
            *guard = false;
        }
    }
}
