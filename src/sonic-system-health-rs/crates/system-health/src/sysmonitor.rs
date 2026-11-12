//! System ready: the status of every SONiC service in ALL_SERVICE_STATUS, and SYSTEM_READY.

use std::collections::BTreeSet;
use std::path::PathBuf;
use std::time::Duration;

use anyhow::{anyhow, Result};
use sonic_rs_common::DeviceInfo;

use crate::config::Config;
use crate::db::{ConfigDb, RedisErrors, StateDb};
use crate::log;
use crate::systemd::Systemd;
use crate::template::{feature_context, render_value, FEATURE_STATES};

/// Full rescan period; it also catches failures that sent no event, e.g. a fast crash and restart.
pub const PERIODIC_POLL_INTERVAL: Duration = Duration::from_secs(15);
const FEATURE_RETRIES: u32 = 3;
const FEATURE_RETRY_DELAY: Duration = Duration::from_secs(1);
const TARGETS: [&str; 2] = ["multi-user.target.wants", "sonic.target.wants"];
const EXCLUDED_SERVICES: [&str; 1] = ["ztp.service"];
const NON_BLOCKING_SERVICES: [&str; 2] = ["database-chassis", "gbsyncd"];
const NON_BLOCKING_INACTIVE_REASONS: [&str; 1] = ["exec-condition"];
const ENABLED_UNIT_FILE_STATES: [&str; 4] = ["enabled", "enabled-runtime", "static", "generated"];

pub trait Console {
    fn write(&self, message: &str);
}

/// Prints to /dev/console with the Python timestamp format.
pub struct DevConsole;

impl Console for DevConsole {
    fn write(&self, message: &str) {
        let line = format!("\n{} {message}\n", chrono::Local::now().format("%b %d %H:%M:%S%.6f"));
        // Best effort, as in Python: a failed write (EIO on a simulated serial console) becomes a NOTICE.
        if std::fs::write("/dev/console", &line).is_err() {
            log::notice(log::SYSMONITOR, &line);
        }
    }
}

pub struct SysmonContext<'a> {
    pub state_db: &'a dyn StateDb,
    pub config_db: &'a dyn ConfigDb,
    pub systemd: &'a dyn Systemd,
    pub device: &'a DeviceInfo,
    pub console: &'a dyn Console,
    /// "/etc/systemd/system" on a device.
    pub systemd_dir: PathBuf,
    /// healthd's Redis connection errors: after one, the system state is not published, and healthd exits.
    pub redis_errors: Option<&'a RedisErrors>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SystemState {
    Up,
    Down,
}

impl SystemState {
    pub fn as_str(self) -> &'static str {
        match self {
            SystemState::Up => "UP",
            SystemState::Down => "DOWN",
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnitStatus {
    Ok,
    NotOk,
    /// Not loaded, or not an enabled unit: not tracked.
    Unmanaged,
    /// systemd or the databases did not answer: the last status stands.
    Unknown,
}

pub struct Sysmonitor {
    config: Config,
    down: BTreeSet<String>,
    state: SystemState,
}

impl Sysmonitor {
    pub fn new(config: Config) -> Self {
        Self {
            config,
            down: BTreeSet::new(),
            // As Python: nothing is published until the first change from DOWN.
            state: SystemState::Down,
        }
    }

    pub fn state(&self) -> SystemState {
        self.state
    }

    pub fn down_services(&self) -> &BTreeSet<String> {
        &self.down
    }

    /// Services to monitor: the units wanted by multi-user.target and sonic.target, plus enabled FEATUREs.
    pub async fn get_all_service_list(&mut self, ctx: &SysmonContext<'_>) -> Vec<String> {
        let mut services: Vec<String> = Vec::new();
        for target in TARGETS {
            let Ok(entries) = std::fs::read_dir(ctx.systemd_dir.join(target)) else {
                continue;
            };
            for entry in entries.flatten() {
                let name = entry.file_name().to_string_lossy().into_owned();
                if name.ends_with(".service") && !name.starts_with('.') && !services.contains(&name) {
                    services.push(name);
                }
            }
        }

        self.add_feature_services(ctx, &mut services).await;

        self.config.load_config();
        services.retain(|s| !self.config.ignores_service(s) && !EXCLUDED_SERVICES.contains(&s.as_str()));

        if ctx.device.is_multi_asic() {
            if let Ok(features) = ctx.config_db.get_table("FEATURE") {
                services.retain(|s| {
                    if s.contains('@') {
                        return true;
                    }
                    let name = s.replace(".service", "");
                    features
                        .get(&name)
                        .and_then(|f| f.get("has_global_scope"))
                        .is_none_or(|scope| scope.to_lowercase() != "false")
                });
            }
        }
        services.sort();
        services
    }

    /// FEATURE entries whose rendered state is not disabled. Retried: `config reload` writes the table in steps.
    async fn add_feature_services(&self, ctx: &SysmonContext<'_>, services: &mut Vec<String>) {
        for attempt in 1..=FEATURE_RETRIES {
            match feature_services(ctx, services) {
                Ok(()) => return,
                Err(e) => {
                    if attempt == FEATURE_RETRIES {
                        log::error(
                            log::SYSMONITOR,
                            &format!("FEATURE table is not fully ready: {e}, max retry reached"),
                        );
                    } else {
                        log::warning(
                            log::SYSMONITOR,
                            &format!("FEATURE table is not fully ready: {e}, retrying"),
                        );
                        tokio::time::sleep(FEATURE_RETRY_DELAY).await;
                    }
                }
            }
        }
    }

    pub async fn get_unit_status(&mut self, ctx: &SysmonContext<'_>, unit: &str) -> UnitStatus {
        let Some((service_name, _)) = unit.rsplit_once('.') else {
            return UnitStatus::Unmanaged;
        };
        let props = match ctx.systemd.unit_properties(unit).await {
            Ok(props) => props,
            Err(e) => {
                log::error(log::SYSMONITOR, &format!("Get unit status {service_name}-{e}"));
                return UnitStatus::Unknown;
            }
        };
        if props.load_state != "loaded" {
            return UnitStatus::Unmanaged;
        }
        log::info(
            log::SYSMONITOR,
            &format!(
                "{unit} service state changed to [{}/{}]",
                props.active_state, props.sub_state
            ),
        );
        if !ENABLED_UNIT_FILE_STATES.contains(&props.unit_file_state.as_str()) {
            return UnitStatus::Unmanaged;
        }

        let mut status = UnitStatus::NotOk;
        let mut service_status = "Down";
        let mut up_status = "Down";
        let mut fail_reason = if props.result == "success" {
            "-".to_string()
        } else {
            props.result.clone()
        };
        let mut update_time = "-".to_string();
        match (props.active_state.as_str(), props.sub_state.as_str()) {
            ("active", "exited") => {
                (service_status, up_status, status) = ("OK", "OK", UnitStatus::Ok);
            }
            ("active", "running") => {
                service_status = "OK";
                let app = match app_ready_status(ctx, service_name) {
                    Ok(app) => app,
                    Err(e) => {
                        log::error(log::SYSMONITOR, &format!("Get unit status {service_name}-{e}"));
                        return UnitStatus::Unknown;
                    }
                };
                update_time = app.update_time;
                if app.up {
                    (up_status, status) = ("OK", UnitStatus::Ok);
                } else {
                    fail_reason = app.fail_reason;
                    if fail_reason == "docker start" {
                        up_status = "Starting";
                        fail_reason = "-".to_string();
                    }
                }
            }
            ("activating", _) => (service_status, up_status) = ("Starting", "Starting"),
            ("deactivating", _) => (service_status, up_status) = ("Stopping", "Stopping"),
            ("inactive", _) => {
                // A nonzero timestamp tells a real condition skip from a stopped static unit that was reloaded.
                let ts = props.condition_timestamp_monotonic.as_str();
                let condition_unmet = props.condition_result == "no" && !ts.is_empty() && ts != "0";
                let non_blocking = props.unit_type == "oneshot"
                    || NON_BLOCKING_SERVICES.contains(&service_name)
                    || NON_BLOCKING_INACTIVE_REASONS.contains(&fail_reason.as_str());
                if non_blocking || condition_unmet {
                    (service_status, up_status, status) = ("OK", "OK", UnitStatus::Ok);
                    if !non_blocking {
                        fail_reason = "condition-unmet".to_string();
                    }
                } else if fail_reason == "-" {
                    fail_reason = "Inactive".to_string();
                }
            }
            _ => {}
        }

        let values = [
            ("service_status", service_status.to_string()),
            ("app_ready_status", up_status.to_string()),
            ("fail_reason", fail_reason),
            ("update_time", update_time),
        ]
        .map(|(f, v)| (f.to_string(), v));
        if let Err(e) = ctx
            .state_db
            .hmset(&format!("ALL_SERVICE_STATUS|{service_name}"), &values)
        {
            log::error(log::SYSMONITOR, &format!("Get unit status {service_name}-{e}"));
            return UnitStatus::Unknown;
        }
        status
    }

    /// Checks every service again and publishes the system state.
    pub async fn update_system_status(&mut self, ctx: &SysmonContext<'_>) {
        let mut fresh_down = BTreeSet::new();
        for service in self.get_all_service_list(ctx).await {
            match self.get_unit_status(ctx, &service).await {
                UnitStatus::NotOk => {
                    fresh_down.insert(service);
                }
                UnitStatus::Unmanaged | UnitStatus::Unknown if self.down.contains(&service) => {
                    fresh_down.insert(service);
                }
                _ => {}
            }
        }
        self.down = fresh_down;
        let state = if self.down.is_empty() {
            SystemState::Up
        } else {
            SystemState::Down
        };
        self.publish_system_status(ctx, state);
    }

    /// Handles one systemd job or FEATURE change.
    pub async fn check_unit_status(&mut self, ctx: &SysmonContext<'_>, unit: &str) {
        let services = self.get_all_service_list(ctx).await;
        if !services.iter().any(|s| s == unit) {
            self.down.remove(unit);
            self.publish_system_status(ctx, self.state_from_down());
            delete_service_status(ctx, unit);
            return;
        }
        let state = match self.get_unit_status(ctx, unit).await {
            UnitStatus::Unmanaged => {
                self.down.remove(unit);
                delete_service_status(ctx, unit);
                self.state_from_down()
            }
            UnitStatus::Unknown => return,
            UnitStatus::Ok if self.state == SystemState::Up => SystemState::Up,
            UnitStatus::Ok => {
                if self.down.remove(unit) {
                    self.state_from_down()
                } else {
                    SystemState::Down
                }
            }
            UnitStatus::NotOk => {
                self.down.insert(unit.to_string());
                SystemState::Down
            }
        };
        self.publish_system_status(ctx, state);
    }

    fn state_from_down(&self) -> SystemState {
        if self.down.is_empty() {
            SystemState::Up
        } else {
            SystemState::Down
        }
    }

    /// Logs, prints and stores the state when it changes.
    pub fn publish_system_status(&mut self, ctx: &SysmonContext<'_>, state: SystemState) {
        // A scan that lost its Redis connection read a partial service list; healthd exits and scans again.
        if self.state == state || ctx.redis_errors.is_some_and(|errors| errors.first().is_some()) {
            return;
        }
        self.state = state;
        let msg = match state {
            SystemState::Down => "System is not ready - one or more services are not up",
            SystemState::Up => "System is ready",
        };
        log::notice(log::SYSMONITOR, msg);
        ctx.console.write(msg);
        let values = [("Status".to_string(), state.as_str().to_string())];
        match ctx.state_db.hmset("SYSTEM_READY|SYSTEM_STATE", &values) {
            Ok(()) => log::info(
                log::SYSMONITOR,
                &format!("Posting system ready status {} to statedb", state.as_str()),
            ),
            Err(e) => log::error(log::SYSMONITOR, &format!("Unable to post system ready status: {e}")),
        }
    }

    /// Removes what sysmonitor wrote, on shutdown.
    pub fn cleanup(&self, ctx: &SysmonContext<'_>) {
        for pattern in ["ALL_SERVICE_STATUS|*", "SYSTEM_READY|*"] {
            if let Err(e) = ctx.state_db.delete_all_by_pattern(pattern) {
                log::error(log::SYSMONITOR, &format!("Failed to delete {pattern}: {e}"));
            }
        }
    }
}

/// Removes ALL_SERVICE_STATUS of a service; a stop propagated to a timer keeps the service's entry.
fn delete_service_status(ctx: &SysmonContext<'_>, unit: &str) {
    let Some((name, "service")) = unit.rsplit_once('.') else {
        return;
    };
    let key = format!("ALL_SERVICE_STATUS|{name}");
    if ctx.state_db.exists(&key).unwrap_or(false) {
        if let Err(e) = ctx.state_db.del(&key) {
            log::error(log::SYSMONITOR, &format!("Failed to delete {key}: {e}"));
        }
    }
}

fn feature_services(ctx: &SysmonContext<'_>, services: &mut Vec<String>) -> Result<()> {
    let features = ctx.config_db.get_table("FEATURE")?;
    let context = feature_context(ctx.device, &ctx.config_db.get_table("DEVICE_METADATA")?);
    for (name, fields) in &features {
        let state = fields
            .get("state")
            .ok_or_else(|| anyhow!("entry {name} has no state"))?;
        let state = render_value(state, &context, &FEATURE_STATES)?;
        if state != "disabled" && state != "always_disabled" {
            let service = format!("{name}.service");
            if !services.contains(&service) {
                services.push(service);
            }
        }
    }
    Ok(())
}

struct AppStatus {
    up: bool,
    fail_reason: String,
    update_time: String,
}

/// With FEATURE check_up_status "true", the service is up only once STATE_DB FEATURE up_status says so. A failed
/// read is an error, as the exception it was in Python.
fn app_ready_status(ctx: &SysmonContext<'_>, service: &str) -> Result<AppStatus> {
    let mut status = AppStatus {
        up: true,
        fail_reason: String::new(),
        update_time: "-".to_string(),
    };
    let features = ctx.config_db.get_table("FEATURE")?;
    let check = features
        .get(service)
        .and_then(|f| f.get("check_up_status"))
        .is_some_and(|v| v.to_lowercase() == "true");
    if !check {
        return Ok(status);
    }
    let key = format!("FEATURE|{service}");
    let get = |field: &str| ctx.state_db.get(&key, field);
    if get("up_status")?.is_none_or(|v| v.to_lowercase() != "true") {
        status.up = false;
        status.fail_reason = get("fail_reason")?.unwrap_or_else(|| "NA".to_string());
    }
    status.update_time = get("update_time")?.unwrap_or_else(|| "-".to_string());
    Ok(status)
}
