use log::{Record, Level, Metadata};
use crate::logger::{Severity, Facility, Logger};
use std::sync::Mutex;

pub struct SimpleLogger {
    logger: Mutex<Logger>
}

impl SimpleLogger {
    pub fn new(log_facility: Facility) -> SimpleLogger {
        let mut logger = Logger::new_with_options(None, log_facility).unwrap();
        logger.set_min_log_priority(Severity::Debug);
        SimpleLogger { logger: Mutex::new(logger) }
    }
}

impl log::Log for SimpleLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= Level::Info
    }

    fn log(&self, record: &Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        let severity = match record.metadata().level() {
            Level::Error => Severity::Error,
            Level::Warn => Severity::Warning,
            Level::Info => Severity::Info,
            Level::Debug => Severity::Debug,
            Level::Trace => Severity::Debug,
        };

        let mut logger = self.logger.lock().unwrap();
        logger.log(severity, record.args().to_string().as_ref(), false).unwrap();
    }

    fn flush(&self) {}
}
