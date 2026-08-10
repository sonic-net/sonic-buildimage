use log::{Record, Level, Metadata};
use crate::logger::{Severity, Facility, Logger};
use std::sync::Mutex;

pub struct SimpleLogger {
    logger: Mutex<Logger>
}

impl SimpleLogger {
    pub fn new(log_facility: Facility) -> Result<SimpleLogger, crate::logger::LoggerError> {
        let mut logger = Logger::new_with_options(None, log_facility)?;
        logger.set_min_log_priority(Severity::Debug);
        Ok(SimpleLogger { logger: Mutex::new(logger) })
    }

    pub fn init(log_facility: Facility, level: log::LevelFilter) -> Result<(), String> {
        let logger = Self::new(log_facility).map_err(|e| format!("Failed to create SimpleLogger: {:?}", e))?;
        log::set_boxed_logger(Box::new(logger))
            .map(|()| log::set_max_level(level))
            .map_err(|e| format!("Failed to initialize logger: {}", e))
    }
}

impl log::Log for SimpleLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= log::max_level()
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
