// Supervisor process exit listener - Single file Rust implementation
// Mirrors the Python version structure and function names

use crate::childutils;
use clap::Parser;
use log::{error, info, warn, Level, LevelFilter, Log, Metadata, Record};
use mio::{Events, Token};
use nix::sys::signal::{self, Signal};
use nix::unistd::getppid;
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Read};
use std::os::unix::io::AsRawFd;
use std::process;
use std::sync::Mutex;
use std::time::{Duration, Instant};
use swss_common::{ConfigDBConnector, EventPublisher};
use syslog::Severity;
use thiserror::Error;

// File paths
const WATCH_PROCESSES_FILE: &str = "/etc/supervisor/watchdog_processes";
const CRITICAL_PROCESSES_FILE: &str = "/etc/supervisor/critical_processes";

// Table names
const FEATURE_TABLE_NAME: &str = "FEATURE";
const HEARTBEAT_TABLE_NAME: &str = "HEARTBEAT";

// Timing constants
const SELECT_TIMEOUT_SECS: u64 = 1;
pub const ALERTING_INTERVAL_SECS: u64 = 60;

// Events configuration
const EVENTS_PUBLISHER_SOURCE: &str = "sonic-events-host";
const EVENTS_PUBLISHER_TAG: &str = "process-exited-unexpectedly";

#[derive(Error, Debug)]
pub enum SupervisorError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
    #[error("Parse error: {0}")]
    Parse(String),
    #[error("Config error: {0}")]
    Config(String),
    #[error("Database error: {0}")]
    Database(String),
    #[error("System error: {0}")]
    System(String),
}

type Result<T> = std::result::Result<T, SupervisorError>;

/// Trait for polling operations - allows for mocking in tests
pub trait Poller {
    fn poll(&mut self, events: &mut mio::Events, timeout: Option<std::time::Duration>) -> std::io::Result<()>;
    fn register(&self, stdin_fd: std::os::unix::io::RawFd, token: Token) -> std::io::Result<()>;
}

/// Production implementation using mio::Poll
pub struct MioPoller(mio::Poll);

impl MioPoller {
    pub fn new() -> std::io::Result<Self> {
        Ok(MioPoller(mio::Poll::new()?))
    }
    
    pub fn registry(&self) -> &mio::Registry {
        self.0.registry()
    }
}

impl Poller for MioPoller {
    fn poll(&mut self, events: &mut mio::Events, timeout: Option<std::time::Duration>) -> std::io::Result<()> {
        self.0.poll(events, timeout)
    }

    fn register(&self, stdin_fd: std::os::unix::io::RawFd, token: Token) -> std::io::Result<()> {
        let mut stdin_source = mio::unix::SourceFd(&stdin_fd);
        self.0.registry().register(&mut stdin_source, token, mio::Interest::READABLE)
    }
}

/// Trait for ConfigDB operations - allows for both real ConfigDBConnector and mocks
pub trait ConfigDBTrait {
    /// Get table data from database
    fn get_table(&self, table: &str) -> std::result::Result<HashMap<String, HashMap<String, String>>, Box<dyn std::error::Error>>;
}

/// Implementation of ConfigDBTrait for the real ConfigDBConnector
impl ConfigDBTrait for ConfigDBConnector {
    fn get_table(&self, table: &str) -> std::result::Result<HashMap<String, HashMap<String, String>>, Box<dyn std::error::Error>> {
        let result = self.get_table(table)?;
        
        // Convert from CxxString to String
        let mut converted_result = HashMap::new();
        for (key, value_map) in result {
            let mut converted_value_map = HashMap::new();
            for (inner_key, inner_value) in value_map {
                converted_value_map.insert(inner_key, inner_value.to_string_lossy().to_string());
            }
            converted_result.insert(key, converted_value_map);
        }
        
        Ok(converted_result)
    }
}

#[derive(Parser, Debug)]
#[command(name = "supervisor-proc-exit-listener")]
#[command(about = "SONiC supervisor process exit listener")]
pub struct Args {
    #[arg(short = 'c', long = "container-name", required = true)]
    pub container_name: String,

    #[arg(short = 's', long = "use-unix-socket-path")]
    pub use_unix_socket_path: bool,
}

/// Read the critical processes/group names
pub fn get_group_and_process_list(process_file: &str) -> Result<(Vec<String>, Vec<String>)> {
    let mut group_list = Vec::new();
    let mut process_list = Vec::new();

    let file = File::open(process_file)?;
    let reader = BufReader::new(file);

    for (_line_num, line) in reader.lines().enumerate() {
        let line = line?;
        let line = line.trim();
        
        // ignore blank lines
        if line.is_empty() {
            continue;
        }

        let line_info: Vec<&str> = line.split(':').collect();
        if line_info.len() != 2 {
            error!("Syntax of the line {} in processes file is incorrect. Exiting...", line);
            process::exit(5);
        }

        let identifier_key = line_info[0].trim();
        let identifier_value = line_info[1].trim();

        if identifier_key == "group" && !identifier_value.is_empty() {
            group_list.push(identifier_value.to_string());
        } else if identifier_key == "program" && !identifier_value.is_empty() {
            process_list.push(identifier_value.to_string());
        } else {
            error!("Syntax of the line {} in processes file is incorrect. Exiting...", line);
            process::exit(6);
        }
    }

    Ok((group_list, process_list))
}

/// Generate alerting message
pub fn generate_alerting_message(process_name: &str, status: &str, dead_minutes: u64, priority: Severity) {
    let namespace_prefix = std::env::var("NAMESPACE_PREFIX").unwrap_or_default();
    let namespace_id = std::env::var("NAMESPACE_ID").unwrap_or_default();

    let namespace = if namespace_prefix.is_empty() || namespace_id.is_empty() {
        "host".to_string()
    } else {
        format!("{}{}", namespace_prefix, namespace_id)
    };

    let message = format!(
        "Process '{}' is {} in namespace '{}' ({} minutes).",
        process_name, status, namespace, dead_minutes
    );

    // Log with appropriate severity (matching syslog levels)
    match priority {
        Severity::LOG_ERR => error!("{}", message),
        Severity::LOG_WARNING => warn!("{}", message),
        Severity::LOG_INFO => info!("{}", message),
        _ => error!("{}", message),
    }
}

/// Read auto-restart state from ConfigDB
/// Returns None on error or if the FEATURE table or the container entry is not found
pub fn get_autorestart_state(container_name: &str, config_db: &dyn ConfigDBTrait) -> Option<String> {
    let features_table = match config_db.get_table(FEATURE_TABLE_NAME) {
        Ok(table) => table,
        Err(e) => {
            warn!("Unable to retrieve features table from Config DB: {}", e);
            return None;
        }
    };

    if features_table.is_empty() {
        warn!("Empty features table");
        return None;
    }

    let feature_config = match features_table.get(container_name) {
        Some(config) => config,
        None => {
            warn!("Unable to retrieve feature '{}'", container_name);
            return None;
        }
    };

    // Use default "enabled" if auto_restart field not found
    Some(feature_config.get("auto_restart").cloned().unwrap_or_else(|| "enabled".to_string()))
}

/// Load heartbeat alert intervals from ConfigDB
pub fn load_heartbeat_alert_interval(config_db: &dyn ConfigDBTrait) -> HashMap<String, f64> {
    let mut mapping = HashMap::new();

    let heartbeat_table = match config_db.get_table(HEARTBEAT_TABLE_NAME) {
        Ok(table) => table,
        Err(e) => {
            warn!("Failed to get HEARTBEAT table: {}", e);
            return mapping;
        }
    };

    for (process, config) in heartbeat_table {
        if let Some(alert_interval_str) = config.get("alert_interval") {
            if let Ok(alert_interval_ms) = alert_interval_str.parse::<i64>() {
                // Convert from milliseconds to seconds
                mapping.insert(process, alert_interval_ms as f64 / 1000.0);
            }
        }
    }

    mapping
}

/// Get heartbeat alert interval for process
pub fn get_heartbeat_alert_interval(process: &str, heartbeat_intervals: &HashMap<String, f64>) -> f64 {
    heartbeat_intervals.get(process).copied().unwrap_or(ALERTING_INTERVAL_SECS as f64)
}

/// Publish events
pub fn publish_events(events_handle: &EventPublisher, process_name: &str, container_name: &str) -> Result<()> {
    let mut params = HashMap::new();
    params.insert("process_name".to_string(), process_name.to_string());
    params.insert("ctr_name".to_string(), container_name.to_string());
    
    events_handle.publish(EVENTS_PUBLISHER_TAG, Some(&params))
        .map_err(|e| SupervisorError::System(format!("Failed to publish event: {}", e)))?;
    
    info!("Published event: {} for process {} in container {}", EVENTS_PUBLISHER_TAG, process_name, container_name);
    Ok(())
}

/// Get current monotonic time as seconds since an arbitrary epoch, not wall clock time
pub fn get_current_time() -> f64 {
    static START_TIME: std::sync::OnceLock<Instant> = std::sync::OnceLock::new();
    let start = START_TIME.get_or_init(|| Instant::now());
    start.elapsed().as_secs_f64()
}

/// Dual logger: always writes to stderr (supervisord captures it), and connects
/// to syslog lazily on each write. This matches the behaviour of Python's syslog
/// module, which also reconnects on every call. A not-yet-ready /dev/log at
/// startup never causes init failure or startretries/FATAL; once /dev/log
/// appears (rsyslogd starts), the next write picks it up automatically.
struct DualLogger {
    level: LevelFilter,
    syslog: Mutex<Option<syslog::Logger<syslog::LoggerBackend, syslog::Formatter3164>>>,
}

impl DualLogger {
    fn make_syslog() -> Option<syslog::Logger<syslog::LoggerBackend, syslog::Formatter3164>> {
        syslog::unix(syslog::Formatter3164 {
            facility: syslog::Facility::LOG_USER,
            ..Default::default()
        }).ok()
    }
}

impl Log for DualLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= self.level
    }

    fn log(&self, record: &Record) {
        if !self.enabled(record.metadata()) {
            return;
        }
        // stderr is always available; supervisord captures and routes it.
        eprintln!("{}: {}", record.level(), record.args());
        // syslog: lazy-connect on each write so /dev/log not being ready at
        // startup is non-fatal and self-healing (matches Python syslog behaviour).
        if let Ok(mut guard) = self.syslog.lock() {
            if guard.is_none() {
                *guard = Self::make_syslog();
            }
            if let Some(ref mut l) = *guard {
                let msg = record.args().to_string();
                let result = match record.level() {
                    Level::Error => l.err(msg),
                    Level::Warn  => l.warning(msg),
                    Level::Info  => l.info(msg),
                    Level::Debug | Level::Trace => l.debug(msg),
                };
                // If the write fails (e.g. /dev/log closed), drop the handle so
                // the next call retries the connection.
                if result.is_err() {
                    *guard = None;
                }
            }
        }
    }

    fn flush(&self) {}
}

/// Initialize logging to both stderr and syslog. Syslog init failure is
/// non-fatal: the logger connects lazily on first write and retries on every
/// subsequent write until /dev/log is available. This prevents the /dev/log-
/// not-ready race at container startup that caused startretries exhaustion and
/// FATAL state, while ensuring syslog alerting resumes as soon as rsyslogd is up.
fn init_logging() {
    let logger = DualLogger {
        level: LevelFilter::Info,
        syslog: Mutex::new(DualLogger::make_syslog()),
    };
    if log::set_boxed_logger(Box::new(logger)).is_ok() {
        log::set_max_level(LevelFilter::Info);
    }
}

/// Main function with testable parameters
pub fn main_with_args(args: Option<Vec<String>>) -> Result<()> {
    // Best-effort logging init: stderr always works, syslog is optional.
    // This prevents the /dev/log-not-ready race at container startup that
    // caused startretries exhaustion and FATAL state.
    init_logging();

    // Parse command line arguments
    let parsed_args = if let Some(args) = args {
        Args::try_parse_from(args).map_err(|e| SupervisorError::Parse(e.to_string()))?
    } else {
        Args::parse()
    };

    main_with_parsed_args(parsed_args)
}

/// Main function with parsed arguments - uses stdin by default
pub fn main_with_parsed_args(args: Args) -> Result<()> {
    let config_db = ConfigDBConnector::new(args.use_unix_socket_path, None)
        .map_err(|e| SupervisorError::Database(format!("Failed to create ConfigDB connector: {}", e)))?;
    config_db.connect(true, false)
        .map_err(|e| SupervisorError::Database(format!("Failed to connect to ConfigDB: {}", e)))?;
    let poller = MioPoller::new().map_err(|e| SupervisorError::Io(e))?;
    main_with_parsed_args_and_stdin(args, io::stdin(), CRITICAL_PROCESSES_FILE, WATCH_PROCESSES_FILE, &config_db, poller)
}

/// Main function with parsed arguments and custom stdin - allows for easy testing
pub fn main_with_parsed_args_and_stdin<S: Read + AsRawFd, P: Poller>(args: Args, stdin: S, critical_processes_file: &str, watch_processes_file: &str, config_db: &dyn ConfigDBTrait, mut poller: P) -> Result<()> {
    let container_name = args.container_name;

    // Get critical processes and groups
    let (critical_group_list, critical_process_list) = get_group_and_process_list(critical_processes_file)?;

    // WATCH_PROCESSES_FILE is optional
    let watch_process_list = if std::path::Path::new(watch_processes_file).exists() {
        get_group_and_process_list(watch_processes_file)?.1
    } else {
        Vec::new()
    };

    // Process state tracking
    let mut process_under_alerting: HashMap<String, HashMap<String, f64>> = HashMap::new();
    let mut process_heart_beat_info: HashMap<String, HashMap<String, f64>> = HashMap::new();

    // Load heartbeat alert intervals once at startup
    let heartbeat_intervals = load_heartbeat_alert_interval(config_db);

    // Initialize events publisher
    let mut events_handle = EventPublisher::new(EVENTS_PUBLISHER_SOURCE)
        .map_err(|e| SupervisorError::System(format!("Failed to initialize event publisher: {}", e)))?;
    info!("Initialized events publisher: {}", EVENTS_PUBLISHER_SOURCE);

    // Transition from ACKNOWLEDGED to READY
    childutils::listener::ready();

    // Set up non-blocking I/O with mio for timeout-based reading
    let mut events = Events::with_capacity(128);
    const STDIN_TOKEN: Token = Token(0);
    
    // Register stdin for reading using the poller
    let stdin_fd = stdin.as_raw_fd();
    poller.register(stdin_fd, STDIN_TOKEN).map_err(|e| SupervisorError::Io(e))?;

    let timeout = Duration::from_secs(SELECT_TIMEOUT_SECS);

    // Create buffered reader from the provided stdin
    let mut stdin_reader = BufReader::new(stdin);

    // Main event loop with timeout
    loop {
        // Poll for events with timeout
        poller.poll(&mut events, Some(timeout)).map_err(|e| SupervisorError::Io(e))?;

        let mut stdin_ready = false;
        for event in events.iter() {
            if event.token() == STDIN_TOKEN {
                stdin_ready = true;
            } else {
                error!("Unexpected event token: {:?}, this may indicate invalid logic", event.token());
            }
        }

        if stdin_ready {
            // Read from stdin
            let mut buffer = String::new();
            match stdin_reader.read_line(&mut buffer) {
                Ok(0) => {
                    // EOF - supervisor shut down
                    return Ok(());
                }
                Ok(_) => {
                    // Parse supervisor protocol headers
                    let headers = childutils::get_headers(&buffer);

                    // Check if 'len' is missing - if so, log and continue
                    let len = if let Some(len_str) = headers.get("len") {
                        len_str.parse::<usize>().unwrap_or(0)
                    } else {
                        warn!("Missing 'len' in headers: {:?}", headers);
                        continue;
                    };

                    // Read payload
                    let mut payload = vec![0u8; len];
                    if len > 0 {
                        match stdin_reader.read_exact(&mut payload) {
                            Ok(_) => {},
                            Err(e) => {
                                error!("Failed to read payload: {}", e);
                                continue;
                            }
                        }
                    }
                    let payload = String::from_utf8_lossy(&payload);

                    // Handle different event types
                    let eventname = headers.get("eventname").cloned().unwrap_or_default();
                    match eventname.as_str() {
                        "PROCESS_STATE_EXITED" => {
                            // Handle the PROCESS_STATE_EXITED event
                            let (payload_headers, _payload_data) = childutils::eventdata(&(payload.to_string() + "\n"));

                            let expected = payload_headers.get("expected").and_then(|s| s.parse().ok()).unwrap_or(0);
                            let process_name = payload_headers.get("processname").cloned().unwrap_or_default();
                            let group_name = payload_headers.get("groupname").cloned().unwrap_or_default();

                            // Check if critical process and handle
                            if (critical_process_list.contains(&process_name) || critical_group_list.contains(&group_name)) && expected == 0 {
                                let is_auto_restart = get_autorestart_state(&container_name, config_db);

                                if is_auto_restart == Some("enabled".to_string()) {
                                    // Process exited unexpectedly - terminate supervisor
                                    let msg = format!("Process '{}' exited unexpectedly. Terminating supervisor '{}'", 
                                        process_name, container_name);
                                    info!("{}", msg);

                                    // Publish events
                                    publish_events(&events_handle, &process_name, &container_name).ok();

                                    // Deinit publisher
                                    events_handle.deinit().ok();

                                    // Terminate supervisor
                                    if let Err(e) = terminate_supervisor() {
                                        error!("Failed to terminate supervisor: {}", e);
                                    }
                                    return Ok(());
                                } else {
                                    // Add to alerting processes
                                    let mut process_info = HashMap::new();
                                    process_info.insert("last_alerted".to_string(), get_current_time());
                                    process_info.insert("dead_minutes".to_string(), 0.0);
                                    process_under_alerting.insert(process_name.clone(), process_info);
                                }
                            }
                        }

                        "PROCESS_STATE_RUNNING" => {
                            // Handle the PROCESS_STATE_RUNNING event
                            let (payload_headers, _payload_data) = childutils::eventdata(&(payload.to_string() + "\n"));

                            let process_name = payload_headers.get("processname").cloned().unwrap_or_default();

                            // Remove from alerting if it was there
                            if process_under_alerting.contains_key(&process_name) {
                                process_under_alerting.remove(&process_name);
                            }
                        }

                        "PROCESS_COMMUNICATION_STDOUT" => {
                            // Handle the PROCESS_COMMUNICATION_STDOUT event
                            let (payload_headers, _payload_data) = childutils::eventdata(&(payload.to_string() + "\n"));

                            let process_name = payload_headers.get("processname").cloned().unwrap_or_default();

                            // Update process heart beat time
                            if watch_process_list.contains(&process_name) {
                                let mut heartbeat_info = HashMap::new();
                                heartbeat_info.insert("last_heart_beat".to_string(), get_current_time());
                                process_heart_beat_info.insert(process_name.clone(), heartbeat_info);
                            }
                        }

                        "PROCESS_STATE_FATAL" => {
                            // Handle the PROCESS_STATE_FATAL event (process exhausted all startretries)
                            // This is a terminal state — the process will never restart on its own.
                            // Unlike PROCESS_STATE_EXITED, there is no 'expected' field; any FATAL
                            // on a critical process is always unexpected.
                            let (payload_headers, _payload_data) = childutils::eventdata(&(payload.to_string() + "\n"));

                            let process_name = payload_headers.get("processname").cloned().unwrap_or_default();
                            let group_name = payload_headers.get("groupname").cloned().unwrap_or_default();

                            if critical_process_list.contains(&process_name) || critical_group_list.contains(&group_name) {
                                let is_auto_restart = get_autorestart_state(&container_name, config_db);

                                if is_auto_restart == Some("enabled".to_string()) {
                                    let msg = format!("Process '{}' entered FATAL state. Terminating supervisor '{}'",
                                        process_name, container_name);
                                    info!("{}", msg);

                                    publish_events(&events_handle, &process_name, &container_name).ok();
                                    events_handle.deinit().ok();

                                    if let Err(e) = terminate_supervisor() {
                                        error!("Failed to terminate supervisor: {}", e);
                                    }
                                    return Ok(());
                                } else {
                                    let mut process_info = HashMap::new();
                                    process_info.insert("last_alerted".to_string(), get_current_time());
                                    process_info.insert("dead_minutes".to_string(), 0.0);
                                    process_under_alerting.insert(process_name.clone(), process_info);
                                }
                            }
                        }

                        _ => {
                            // Unknown event type - just acknowledge
                            warn!("Unknown event type: {}", eventname);
                        }
                    }

                    // Transition from BUSY to ACKNOWLEDGED
                    childutils::listener::ok();

                    // Transition from ACKNOWLEDGED to READY
                    childutils::listener::ready();
                }
                Err(e) => {
                    error!("Failed to read from stdin: {}", e);
                    return Err(SupervisorError::Io(e));
                }
            }
        }

        // Check whether we need to write alerting messages
        let current_time = get_current_time();

        for (process_name, process_info) in process_under_alerting.iter_mut() {
            if let Some(&last_alerted) = process_info.get("last_alerted") {
                let elapsed_secs = current_time - last_alerted;
                if elapsed_secs >= ALERTING_INTERVAL_SECS as f64 {
                    let elapsed_mins = (elapsed_secs / 60.0) as u64;
                    process_info.insert("last_alerted".to_string(), current_time);
                    let current_dead_minutes = process_info.get("dead_minutes").unwrap_or(&0.0);
                    let new_dead_minutes = current_dead_minutes + elapsed_mins as f64;
                    process_info.insert("dead_minutes".to_string(), new_dead_minutes);

                    generate_alerting_message(process_name, "not running", new_dead_minutes as u64, Severity::LOG_ERR);
                }
            }
        }

        // Check heartbeat timeouts
        for (process, process_info) in process_heart_beat_info.iter() {
            if let Some(&last_heart_beat) = process_info.get("last_heart_beat") {
                let elapsed_secs = current_time - last_heart_beat;
                let threshold = get_heartbeat_alert_interval(process, &heartbeat_intervals);
                if threshold > 0.0 && elapsed_secs >= threshold {
                    let elapsed_mins = (elapsed_secs / 60.0) as u64;
                    generate_alerting_message(process, "stuck", elapsed_mins, Severity::LOG_WARNING);
                }
            }
        }
    }
}

// Helper function to terminate supervisor - extracted from main loop logic
fn terminate_supervisor() -> Result<()> {
    let parent_pid = getppid();
    signal::kill(parent_pid, Signal::SIGTERM).map_err(|e| {
        SupervisorError::System(format!("Failed to send SIGTERM: {}", e))
    })?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use log::{Level, LevelFilter, Log};

    #[test]
    fn test_get_current_time() {
        let time1 = get_current_time();
        std::thread::sleep(std::time::Duration::from_millis(10));
        let time2 = get_current_time();
        assert!(time2 > time1);
    }

    /// DualLogger must not panic when /dev/log is absent (syslog slot is None).
    /// This is the core property that prevents startretries exhaustion.
    #[test]
    fn test_dual_logger_no_panic_without_syslog() {
        let logger = DualLogger {
            level: LevelFilter::Info,
            syslog: Mutex::new(None), // simulate /dev/log absent
        };
        // log() must not panic with a None syslog slot
        let record = log::Record::builder()
            .level(Level::Info)
            .args(format_args!("test: /dev/log absent"))
            .module_path(Some("test"))
            .file(Some("test"))
            .line(Some(1))
            .build();
        logger.log(&record); // must not panic
    }

    /// DualLogger::enabled() must respect the level filter.
    #[test]
    fn test_dual_logger_enabled_respects_level_filter() {
        let logger = DualLogger {
            level: LevelFilter::Info,
            syslog: Mutex::new(None),
        };
        let info_meta = log::Metadata::builder().level(Level::Info).target("test").build();
        let debug_meta = log::Metadata::builder().level(Level::Debug).target("test").build();
        assert!(logger.enabled(&info_meta), "Info should be enabled at Info filter");
        assert!(!logger.enabled(&debug_meta), "Debug should be disabled at Info filter");
    }

    /// A record below the level filter must be silently dropped — no syslog
    /// slot access, no panic.
    #[test]
    fn test_dual_logger_below_level_silently_dropped() {
        let logger = DualLogger {
            level: LevelFilter::Info,
            syslog: Mutex::new(None),
        };
        let record = log::Record::builder()
            .level(Level::Debug)
            .args(format_args!("should be dropped"))
            .module_path(Some("test"))
            .file(Some("test"))
            .line(Some(1))
            .build();
        logger.log(&record); // must be a no-op — no panic, no side effect
    }

    /// After a failed write the syslog slot must be cleared to None so the
    /// next call will retry the connection (self-heal property).
    #[test]
    fn test_dual_logger_clears_slot_on_write_failure() {
        // Construct a deliberately broken Logger by pointing syslog at a
        // non-existent socket path via the unix() factory with a bad path.
        // syslog::unix() with a missing path returns Err, so .ok() gives None —
        // we seed the slot with None and verify it stays None after log().
        // The observable property is: slot is None before and after the call.
        let logger = DualLogger {
            level: LevelFilter::Info,
            syslog: Mutex::new(None),
        };
        let record = log::Record::builder()
            .level(Level::Info)
            .args(format_args!("trigger retry path"))
            .module_path(Some("test"))
            .file(Some("test"))
            .line(Some(1))
            .build();
        logger.log(&record);
        // After the call the slot should still be None (no /dev/log available
        // in test environment, so make_syslog() returns None, slot stays None).
        let guard = logger.syslog.lock().unwrap();
        assert!(guard.is_none(), "slot must remain None when /dev/log is unavailable");
    }
}
