//! healthd: system health checks every polling interval, and system ready.

use std::future::Future;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::{Duration, Instant};

use anyhow::{anyhow, Context, Result};
use sonic_rs_common::{restart_waiter, DeviceInfo};
use swss_common::{DbConnector, SelectResult, SubscriberStateTable};
use system_health::command::TokioCommandRunner;
use system_health::config::{platform_name, seconds, Config};
use system_health::context::CheckContext;
use system_health::db::{ConfigDb, RedisErrors, SwssConfigDb, SwssStateDb};
use system_health::docker::BollardRuntime;
use system_health::events::SwssEvents;
use system_health::led::{sonic_platform_missing, NoLed, PlatformLed, StatusLed};
use system_health::manager::HealthCheckManager;
use system_health::pyfmt::py_repr;
use system_health::sysmonitor::{DevConsole, SysmonContext, Sysmonitor, PERIODIC_POLL_INTERVAL};
use system_health::systemd::{watch_job_removed, Systemctl};
use system_health::{log, publisher};
use tokio::signal::unix::{signal, Signal, SignalKind};
use tokio::sync::{mpsc, watch};

const DEVICE_PATH: &str = "/usr/share/sonic/device";
const MONITOR_RETRY_DELAY: Duration = Duration::from_secs(5);
/// A monitor that keeps failing is retried every 5 s; its ERR is logged at most this often.
const MONITOR_ERROR_INTERVAL: Duration = Duration::from_secs(600);

fn main() {
    if let Err(e) = restart_waiter::wait_advanced_boot_done(restart_waiter::DEFAULT_MAX_WAIT) {
        log::error(log::HEALTHD, &format!("Failed to wait for warm or fast boot: {e:?}"));
    }
    log::notice(log::HEALTHD, "Starting up...");
    // The vendor's chassis first, on the main thread, as in Python. Without sonic_platform healthd stops, as Python's
    // did; a vendor platform that fails to load leaves only the LED out, so the checks go on without it.
    let mut platform = match PlatformLed::open(log::HEALTHD) {
        Ok(led) => Some(led),
        Err(e) if sonic_platform_missing(&e) => {
            log::warning(
                log::HEALTHD,
                "sonic_platform package not installed. Cannot start system-health daemon",
            );
            std::process::exit(1);
        }
        Err(e) => {
            log::error(
                log::HEALTHD,
                &format!("Failed to load the platform API; running without the status LED: {e}"),
            );
            None
        }
    };
    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .expect("async runtime");
    let result = runtime.block_on(async {
        let result = match platform.as_mut() {
            Some(led) => run(led).await,
            None => run(&mut NoLed).await,
        };
        if let Some(led) = platform {
            led.close().await;
        }
        result
    });
    if let Err(e) = result {
        log::error(log::HEALTHD, &format!("Exiting on error: {e:#}"));
        std::process::exit(1);
    }
}

async fn run(led: &mut dyn StatusLed) -> Result<()> {
    let device = DeviceInfo::new();
    let platform = platform_name(&device, || {
        SwssConfigDb::connect(RedisErrors::default())
            .ok()?
            .get_table("DEVICE_METADATA")
            .ok()
    });
    let config = || Config::for_platform(&PathBuf::from(DEVICE_PATH), &platform);
    if !config().config_file_exists() {
        log::warning(log::HEALTHD, "System health configuration file not found, exit...");
        return Ok(());
    }

    let signals = Signals::install()?;
    let (stop, _stopped) = watch::channel(false);
    let (unit_tx, unit_rx) = mpsc::unbounded_channel();
    let feature_stop = Arc::new(AtomicBool::new(false));
    let feature_thread = spawn_feature_monitor(unit_tx.clone(), feature_stop.clone());
    let dbus_task = tokio::spawn(watch_systemd_jobs(unit_tx));

    let redis = RedisErrors::default();
    let checker_state_db = SwssStateDb::connect(redis.clone())?;
    let checker_config_db = SwssConfigDb::connect(redis.clone())?;
    let sysmon_state_db = SwssStateDb::connect(redis.clone())?;
    let sysmon_config_db = SwssConfigDb::connect(redis.clone())?;
    let systemd = Systemctl;
    let docker = BollardRuntime::default();
    let commands = TokioCommandRunner;
    let events = SwssEvents;

    let check_ctx = CheckContext {
        state_db: &checker_state_db,
        config_db: &checker_config_db,
        docker: &docker,
        commands: &commands,
        events: &events,
        device: &device,
        proc_root: PathBuf::from("/proc"),
        redis_errors: Some(&redis),
    };
    let sysmon_ctx = SysmonContext {
        state_db: &sysmon_state_db,
        config_db: &sysmon_config_db,
        systemd: &systemd,
        device: &device,
        console: &DevConsole,
        systemd_dir: PathBuf::from("/etc/systemd/system"),
        redis_errors: Some(&redis),
    };
    let mut manager = HealthCheckManager::new(config());
    let mut sysmon = Sysmonitor::new(config());

    let (signalled, (), ()) = tokio::join!(
        handle_signals(signals, &stop),
        checker_loop(&mut manager, &check_ctx, led, &stop, &redis),
        sysmon_loop(&mut sysmon, &sysmon_ctx, unit_rx, &stop, &redis),
    );
    // Stopped on a signal, healthd stops as usual, even if a Redis call failed meanwhile.
    let redis_error = if signalled { None } else { redis.first() };
    if redis_error.is_none() {
        led.notify_shutdown().await;
        if let Err(e) = publisher::clear(&checker_state_db) {
            log::error(
                log::HEALTHD,
                &format!("Failed to clear {}: {e}", publisher::SYSTEM_HEALTH_TABLE_NAME),
            );
        }
        sysmon.cleanup(&sysmon_ctx);
    }
    feature_stop.store(true, Ordering::Relaxed);
    dbus_task.abort();
    let _ = tokio::task::spawn_blocking(move || feature_thread.join()).await;
    // The tables stay as they are: the restarted healthd writes them anew.
    match redis_error {
        Some(e) => Err(anyhow!(
            "A Redis call failed, so healthd stops, and systemd restarts it with new connections: {e}"
        )),
        None => Ok(()),
    }
}

/// After a Redis error, stops healthd; true when it did.
fn stop_on_redis_error(redis: &RedisErrors, stop: &watch::Sender<bool>) -> bool {
    if redis.first().is_none() {
        return false;
    }
    stop.send_replace(true);
    true
}

/// The signals healthd stops on, installed before anything starts: without them it could not stop.
struct Signals {
    term: Signal,
    int: Signal,
    hup: Signal,
}

impl Signals {
    fn install() -> Result<Self> {
        let install = |kind| signal(kind).context("Failed to install signal handlers");
        Ok(Self {
            term: install(SignalKind::terminate())?,
            int: install(SignalKind::interrupt())?,
            hup: install(SignalKind::hangup())?,
        })
    }
}

/// Stops healthd on SIGTERM or SIGINT; true when a signal stopped it, false when it stopped on a Redis error.
async fn handle_signals(mut signals: Signals, stop: &watch::Sender<bool>) -> bool {
    let mut stopped = stop.subscribe();
    loop {
        tokio::select! {
            _ = signals.term.recv() => { log::notice(log::HEALTHD, "Caught SIGTERM - exiting..."); break; }
            _ = signals.int.recv() => { log::notice(log::HEALTHD, "Caught SIGINT - exiting..."); break; }
            _ = signals.hup.recv() => log::notice(log::HEALTHD, "Caught SIGHUP - ignoring..."),
            _ = stopped.wait_for(|stopped| *stopped) => return false,
        }
    }
    stop.send_replace(true);
    true
}

/// Waits `duration`, or until stopped; returns true when stopped.
async fn wait(stop: &mut watch::Receiver<bool>, duration: Duration) -> bool {
    tokio::time::timeout(duration, stop.wait_for(|stopped| *stopped))
        .await
        .is_ok()
}

/// Runs `work` unless a stop comes first; None when stopped. Python's stop waited for a scan in progress.
async fn unless_stopped<T>(stop: &mut watch::Receiver<bool>, work: impl Future<Output = T>) -> Option<T> {
    tokio::select! {
        result = work => Some(result),
        _ = stop.wait_for(|stopped| *stopped) => None,
    }
}

/// Sleeps `duration` in short steps, so a stop is not kept waiting.
fn sleep_unless_stopped(stop: &AtomicBool, duration: Duration) {
    let end = Instant::now() + duration;
    while !stop.load(Ordering::Relaxed) {
        let left = end.saturating_duration_since(Instant::now());
        if left.is_zero() {
            break;
        }
        std::thread::sleep(left.min(Duration::from_millis(100)));
    }
}

async fn checker_loop(
    manager: &mut HealthCheckManager,
    ctx: &CheckContext<'_>,
    led: &mut dyn StatusLed,
    stop: &watch::Sender<bool>,
    redis: &RedisErrors,
) {
    let mut stopped = stop.subscribe();
    loop {
        let begin = Instant::now();
        let result = tokio::select! {
            result = manager.check(ctx, led) => result,
            _ = stopped.wait_for(|stopped| *stopped) => return,
        };
        // A cycle that met a Redis error is not published.
        if stop_on_redis_error(redis, stop) {
            return;
        }
        if let Err(e) = publisher::publish(ctx.state_db, &result) {
            log::error(
                log::HEALTHD,
                &format!("Failed to update {}: {e}", publisher::SYSTEM_HEALTH_TABLE_NAME),
            );
        }
        if stop_on_redis_error(redis, stop) {
            return;
        }
        let elapsed = begin.elapsed().as_secs_f64();
        let mut sleep = manager.config.interval - elapsed;
        if sleep < 0.0 {
            log::notice(
                log::HEALTHD,
                &format!("System health takes {} seconds for one iteration", py_repr(elapsed)),
            );
            sleep = 1.0;
        }
        if wait(&mut stopped, seconds(sleep)).await {
            return;
        }
    }
}

async fn sysmon_loop(
    sysmon: &mut Sysmonitor,
    ctx: &SysmonContext<'_>,
    mut units: mpsc::UnboundedReceiver<String>,
    stop: &watch::Sender<bool>,
    redis: &RedisErrors,
) {
    let mut stopped = stop.subscribe();
    // A scan or a unit check can take a systemctl timeout per unit; a stop does not wait for it.
    if unless_stopped(&mut stopped, sysmon.update_system_status(ctx))
        .await
        .is_none()
        || stop_on_redis_error(redis, stop)
    {
        return;
    }
    let mut last_full_scan = Instant::now();
    let mut units_open = true;
    loop {
        let next_scan = tokio::time::Instant::from_std(last_full_scan + PERIODIC_POLL_INTERVAL);
        let unit = tokio::select! {
            _ = stopped.wait_for(|stopped| *stopped) => return,
            unit = units.recv(), if units_open => {
                units_open = unit.is_some();
                unit
            }
            _ = tokio::time::sleep_until(next_scan) => None,
        };
        if let Some(unit) = unit {
            log::info(log::SYSMONITOR, &format!("check_unit_status for [ {unit} ] "));
            if unless_stopped(&mut stopped, sysmon.check_unit_status(ctx, &unit))
                .await
                .is_none()
                || stop_on_redis_error(redis, stop)
            {
                return;
            }
        }
        if last_full_scan.elapsed() >= PERIODIC_POLL_INTERVAL {
            if unless_stopped(&mut stopped, sysmon.update_system_status(ctx))
                .await
                .is_none()
                || stop_on_redis_error(redis, stop)
            {
                return;
            }
            last_full_scan = Instant::now();
        }
    }
}

/// systemd JobRemoved signals, resubscribed after any bus failure.
async fn watch_systemd_jobs(units: mpsc::UnboundedSender<String>) {
    let mut errors = log::RepeatedFailure::new(MONITOR_ERROR_INTERVAL);
    loop {
        log::info(
            log::SYSMONITOR,
            &format!("Start Listening to systemd bus (pid {})", std::process::id()),
        );
        let result = watch_job_removed(|unit| {
            let _ = units.send(unit);
        })
        .await;
        if let Err(e) = result {
            errors.log(log::SYSMONITOR, &format!("systemd bus monitor failed, retrying: {e:#}"));
        }
        tokio::time::sleep(MONITOR_RETRY_DELAY).await;
    }
}

/// STATE_DB FEATURE changes as "<feature>.service" units, resubscribed after any failure.
fn spawn_feature_monitor(units: mpsc::UnboundedSender<String>, stop: Arc<AtomicBool>) -> std::thread::JoinHandle<()> {
    std::thread::spawn(move || {
        let mut errors = log::RepeatedFailure::new(MONITOR_ERROR_INTERVAL);
        while !stop.load(Ordering::Relaxed) {
            if let Err(e) = watch_feature_table(&units, &stop) {
                errors.log(log::SYSMONITOR, &format!("subscribe_statedb exited- {e:?}"));
                sleep_unless_stopped(&stop, MONITOR_RETRY_DELAY);
            }
        }
    })
}

fn watch_feature_table(units: &mpsc::UnboundedSender<String>, stop: &AtomicBool) -> Result<(), swss_common::Exception> {
    let db = DbConnector::new_named("STATE_DB", false, 0)?;
    let table = SubscriberStateTable::new(db, "FEATURE", None, None)?;
    while !stop.load(Ordering::Relaxed) {
        if table.read_data(Duration::from_secs(1), false)? == SelectResult::Data {
            for kfv in table.pops()? {
                let _ = units.send(format!("{}.service", kfv.key));
            }
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn a_stop_ends_the_work_in_progress() {
        let (stop_tx, mut stop) = watch::channel(false);
        assert_eq!(unless_stopped(&mut stop, async { 7 }).await, Some(7));
        stop_tx.send(true).unwrap();
        // A sysmonitor scan of systemctl calls that do not answer.
        assert_eq!(unless_stopped(&mut stop, std::future::pending::<()>()).await, None);
    }

    #[test]
    fn the_retry_wait_ends_at_the_stop() {
        let stop = AtomicBool::new(false);
        let begin = Instant::now();
        sleep_unless_stopped(&stop, Duration::from_millis(50));
        assert!(begin.elapsed() >= Duration::from_millis(50));
        stop.store(true, Ordering::Relaxed);
        let begin = Instant::now();
        sleep_unless_stopped(&stop, MONITOR_RETRY_DELAY);
        assert!(begin.elapsed() < Duration::from_secs(1), "{:?}", begin.elapsed());
    }

    #[test]
    fn a_redis_connection_error_stops_healthd() {
        let (stop, stopped) = watch::channel(false);
        let redis = RedisErrors::default();
        assert!(!stop_on_redis_error(&redis, &stop));
        assert!(!*stopped.borrow());
        let broken: Result<(), std::io::Error> = Err(std::io::Error::other("Broken pipe"));
        assert!(redis.check(broken, || false).is_err());
        assert!(stop_on_redis_error(&redis, &stop));
        assert!(*stopped.borrow());
    }

    /// Signals no test runner sends, so that the test does not take SIGTERM or SIGINT from the process.
    fn test_signals() -> Signals {
        Signals {
            term: signal(SignalKind::user_defined1()).unwrap(),
            int: signal(SignalKind::user_defined2()).unwrap(),
            hup: signal(SignalKind::window_change()).unwrap(),
        }
    }

    #[tokio::test]
    async fn the_signal_task_ends_when_healthd_stops_on_an_error() {
        let (stop, _stopped) = watch::channel(false);
        let task = handle_signals(test_signals(), &stop);
        let stopper = async {
            stop.send_replace(true);
        };
        let (signalled, ()) = tokio::time::timeout(Duration::from_secs(5), async { tokio::join!(task, stopper) })
            .await
            .expect("the signal task did not end");
        assert!(!signalled);
    }

    #[tokio::test]
    async fn a_signal_stops_healthd() {
        let (stop, stopped) = watch::channel(false);
        let task = handle_signals(test_signals(), &stop);
        let sender = async {
            // The handlers are installed; the task waits for a signal.
            tokio::time::sleep(Duration::from_millis(50)).await;
            let pid = std::process::id().to_string();
            assert!(std::process::Command::new("kill")
                .args(["-USR1", &pid])
                .status()
                .unwrap()
                .success());
        };
        let (signalled, ()) = tokio::time::timeout(Duration::from_secs(5), async { tokio::join!(task, sender) })
            .await
            .expect("the signal task did not end");
        assert!(signalled);
        assert!(*stopped.borrow());
    }
}
