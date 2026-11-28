//! One check cycle: every checker, the summary and the status LED.

use std::io::Write;

use crate::config::{seconds, Config};
use crate::context::CheckContext;
use crate::hardware::HardwareChecker;
use crate::led::{LedSet, StatusLed};
use crate::log;
use crate::service::ServiceChecker;
use crate::status::{CategoryInfo, CheckerInfo, ObjectInfo, Stats, STATUS_NOT_OK, STATUS_OK};
use crate::user_defined::UserDefinedChecker;

pub const INTERNAL_CATEGORY: &str = "Internal";
/// A user-defined command gets the polling interval, but at least this many seconds. Python gave it no limit.
const USER_DEFINED_MIN_TIMEOUT: f64 = 60.0;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CheckResult {
    pub stats: Stats,
    pub summary: String,
    pub led_color: String,
}

impl CheckResult {
    pub fn is_ok(&self) -> bool {
        self.summary == STATUS_OK
    }
}

/// Keeps checker state across cycles in the daemon: known critical processes and leaking sensors.
pub struct HealthCheckManager {
    pub config: Config,
    service: ServiceChecker,
    hardware: HardwareChecker,
}

impl HealthCheckManager {
    pub fn new(config: Config) -> Self {
        Self {
            config,
            service: ServiceChecker::default(),
            hardware: HardwareChecker::default(),
        }
    }

    /// Reloads the config, runs every checker and sets the LED.
    pub async fn check(&mut self, ctx: &CheckContext<'_>, led: &mut dyn StatusLed) -> CheckResult {
        self.config.load_config();
        let mut collector = Collector::default();

        let service = self.service.check(&self.config, ctx).await;
        collector.add(ServiceChecker::CATEGORY, ServiceChecker::NAME, service);

        let hardware = self.hardware.check(&self.config, ctx.state_db, ctx.events);
        collector.add(HardwareChecker::CATEGORY, HardwareChecker::NAME, hardware);

        let timeout = seconds(self.config.interval.max(USER_DEFINED_MIN_TIMEOUT));
        for cmd in self.config.user_defined_checkers.clone().unwrap_or_default() {
            let checker = UserDefinedChecker::new(&cmd);
            let (category, info) = checker.check(ctx.commands, timeout).await;
            collector.add(&category, &checker.name(), Ok(info));
        }

        let led_color = self.led_target_color(collector.not_ok, ctx.uptime());
        // A cycle that lost its Redis connection is not acted on: healthd exits, and its next run sets the LED.
        if !ctx.redis_failed() {
            let set = led.set_status(&led_color).await;
            if let Some(message) = set.message() {
                // Printed, as the Python manager printed it: healthd's stdout goes to the journal, healthcheck's to
                // stderr.
                let _ = writeln!(std::io::stdout().lock(), "{message}");
            } else if set != LedSet::Done(true) {
                log::debug(log::HEALTHD, &format!("Failed to set system led to {led_color}"));
            }
        }
        CheckResult {
            stats: collector.stats,
            summary: if collector.not_ok { STATUS_NOT_OK } else { STATUS_OK }.to_string(),
            led_color,
        }
    }

    fn led_target_color(&self, not_ok: bool, uptime: f64) -> String {
        if !not_ok {
            self.config.get_led_color("normal")
        } else if uptime < self.config.get_bootup_timeout() {
            self.config.get_led_color("booting")
        } else {
            self.config.get_led_color("fault")
        }
    }
}

#[derive(Default)]
struct Collector {
    stats: Stats,
    not_ok: bool,
}

impl Collector {
    fn add(&mut self, category: &str, checker_name: &str, result: anyhow::Result<CheckerInfo>) {
        match result {
            Ok(info) => {
                self.not_ok |= info.has_not_ok();
                merge(self.stats.entry(category.to_string()).or_default(), info.into_info());
            }
            Err(e) => {
                self.not_ok = true;
                self.stats.entry(INTERNAL_CATEGORY.to_string()).or_default().insert(
                    checker_name.to_string(),
                    ObjectInfo {
                        object_type: "Internal".to_string(),
                        message: format!("Failed to perform health check for {checker_name} due to exception - {e:#}"),
                        status: STATUS_NOT_OK.to_string(),
                    },
                );
            }
        }
    }
}

fn merge(target: &mut CategoryInfo, info: CategoryInfo) {
    for (name, object) in info {
        target.insert(name, object);
    }
}
