//! The system status LED, through the vendor's platform API as the Python daemon and CLI call it: the chassis is
//! built once, then set every cycle. The API is sonic-platform-common's platform facade, reached over PyO3.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc};
use std::time::Duration;

use async_trait::async_trait;
use platform_api::{ChassisInfoCol, ChassisInfoCols, PlatformApi, PlatformError};
use platform_provider::PlatformImpl;
use tokio::sync::oneshot;

use crate::log;

/// How setting the LED went.
#[derive(Clone, Debug, PartialEq)]
pub enum LedSet {
    /// The vendor's method ran; whether it answered True.
    Done(bool),
    /// The platform does not implement the method: NotImplementedError in Python.
    NotImplemented,
    /// The vendor's method raised.
    Failed(String),
    /// Not sent: the platform is still in an earlier call, or did not answer in time.
    Skipped,
}

impl LedSet {
    /// What the Python manager printed for it.
    pub fn message(&self) -> Option<String> {
        match self {
            LedSet::NotImplemented => Some("chassis.set_status_led is not implemented".to_string()),
            LedSet::Failed(e) => Some(format!("Failed to set system led due to - {e}")),
            LedSet::Done(_) | LedSet::Skipped => None,
        }
    }
}

#[async_trait(?Send)]
pub trait StatusLed {
    async fn set_status(&mut self, color: &str) -> LedSet;
    async fn get_status(&mut self) -> String;

    /// Tells the platform the daemon is stopping on a signal.
    async fn notify_shutdown(&mut self) {}
}

const LED_NOT_AVAILABLE: &str = "N/A";
const STATUS_LED: ChassisInfoCols = ChassisInfoCols::of(ChassisInfoCol::StatusLed);
/// The environment variable that makes Python's standard streams unbuffered.
const PYTHON_UNBUFFERED: &str = "PYTHONUNBUFFERED";

type Reply<T> = oneshot::Sender<Result<T, PlatformError>>;

enum Request {
    Set(String, Reply<Option<bool>>),
    Get(Reply<Option<String>>),
    Initialize(Reply<Option<bool>>),
    NotifyShutdown(Reply<()>),
    Finalize(Reply<()>),
}

/// The vendor's chassis, on a thread of its own. A vendor call hung in the interpreter cannot be cancelled: a caller
/// stops waiting after the timeout, and the LED is left alone until that call returns.
pub struct PlatformLed {
    requests: mpsc::Sender<Request>,
    /// Set while the platform thread is in a call; still set after a caller stopped waiting for it.
    busy: Arc<AtomicBool>,
    timeout: Duration,
    /// Once stopping: when the work done at stop gives up, one timeout after it began.
    stop_deadline: Option<tokio::time::Instant>,
}

impl PlatformLed {
    /// Opens the vendor's platform API, waiting as long as the vendor takes, as Python does. `ident` is the syslog
    /// identifier the vendor's Python logs with. The process's SIGTERM and SIGINT handlers are left as they were; the
    /// handlers the vendor installs while its chassis is built are kept for `notify_shutdown`.
    ///
    /// Call it before other threads start: it makes the standard streams of the interpreter it starts unbuffered.
    /// Nothing runs the interpreter's exit, which is where Python flushes sys.stdout, so the vendor's prints are
    /// written at once.
    pub fn open(ident: &str) -> Result<Self, PlatformError> {
        if std::env::var_os(PYTHON_UNBUFFERED).is_none() {
            std::env::set_var(PYTHON_UNBUFFERED, "1");
        }
        let ident = ident.to_string();
        Self::spawn(
            move || platform_provider::open(&ident, PlatformImpl::Pyo3),
            crate::CALL_TIMEOUT,
        )
    }

    /// Opens a platform with `open` on the calling thread, where Python builds its chassis: a vendor installs its
    /// signal handlers on the main thread only. A new thread then serves every call.
    pub fn spawn<P, F>(open: F, timeout: Duration) -> Result<Self, PlatformError>
    where
        P: PlatformApi + Send + 'static,
        F: FnOnce() -> Result<P, PlatformError>,
    {
        let mut platform = open()?;
        let (requests, received) = mpsc::channel();
        let busy = Arc::new(AtomicBool::new(false));
        let thread_busy = Arc::clone(&busy);
        std::thread::Builder::new()
            .name("platform-api".to_string())
            .spawn(move || {
                for request in received {
                    serve(&mut platform, request, &thread_busy);
                }
            })
            .map_err(|e| PlatformError::Backend(format!("cannot start the platform API thread: {e}")))?;
        Ok(Self {
            requests,
            busy,
            timeout,
            stop_deadline: None,
        })
    }

    /// Initializes the LED, as the Python CLI does after the check.
    pub async fn initialize(&mut self) {
        self.call(Request::Initialize).await;
    }

    /// Runs the vendor's Python atexit handlers, as a Python process does on exit.
    pub async fn close(mut self) {
        self.call_when_free(Request::Finalize, "exit handlers").await;
    }

    /// A call made at stop, after waiting for an earlier call to end: the work done at stop is not skipped because
    /// the check was cut short in the middle of a call. All of it shares one timeout.
    async fn call_when_free<T>(
        &mut self,
        request: impl FnOnce(Reply<T>) -> Request,
        what: &str,
    ) -> Option<Result<T, PlatformError>> {
        let timeout = self.timeout;
        let deadline = *self
            .stop_deadline
            .get_or_insert_with(|| tokio::time::Instant::now() + timeout);
        while self.busy.load(Ordering::SeqCst) {
            if tokio::time::Instant::now() >= deadline {
                log::warning(
                    log::HEALTHD,
                    &format!("The platform API is still in an earlier call; the vendor's {what} are not run"),
                );
                return None;
            }
            tokio::time::sleep(Duration::from_millis(20)).await;
        }
        // Sent now, the call would be cut off by the exit instead of waited for.
        if tokio::time::Instant::now() >= deadline {
            log::warning(
                log::HEALTHD,
                &format!("The stop is out of time; the vendor's {what} are not run"),
            );
            return None;
        }
        let skipped = format!("the vendor's {what} did not finish");
        self.call_until(request, deadline, &skipped).await
    }

    /// One call on the platform thread; None when it is still in an earlier call, ended, or did not answer in time.
    async fn call<T>(&mut self, request: impl FnOnce(Reply<T>) -> Request) -> Option<Result<T, PlatformError>> {
        let deadline = tokio::time::Instant::now() + self.timeout;
        self.call_until(request, deadline, "the status LED is skipped until it does")
            .await
    }

    /// `call` with its own deadline; `skipped` says in the log what a call that does not answer in time leaves undone.
    async fn call_until<T>(
        &mut self,
        request: impl FnOnce(Reply<T>) -> Request,
        deadline: tokio::time::Instant,
        skipped: &str,
    ) -> Option<Result<T, PlatformError>> {
        if self.busy.swap(true, Ordering::SeqCst) {
            return None;
        }
        let (reply, answer) = oneshot::channel();
        self.requests.send(request(reply)).ok()?;
        match tokio::time::timeout_at(deadline, answer).await {
            Ok(answer) => answer.ok(),
            Err(_) => {
                log::warning(
                    log::HEALTHD,
                    &format!(
                        "The platform API did not answer in {} seconds; {skipped}",
                        self.timeout.as_secs()
                    ),
                );
                None
            }
        }
    }
}

/// Runs one request; `busy` is cleared before the answer is sent, so the caller can make the next call at once.
fn serve(platform: &mut impl PlatformApi, request: Request, busy: &AtomicBool) {
    fn answer<T>(reply: Reply<T>, result: Result<T, PlatformError>, busy: &AtomicBool) {
        busy.store(false, Ordering::SeqCst);
        let _ = reply.send(result);
    }
    match request {
        Request::Set(color, reply) => answer(reply, platform.set_chassis_led(&color), busy),
        Request::Get(reply) => answer(
            reply,
            platform.get_chassis_info(STATUS_LED).map(|info| info.status_led),
            busy,
        ),
        Request::Initialize(reply) => answer(reply, platform.initialize_system_led(), busy),
        Request::NotifyShutdown(reply) => answer(reply, platform.notify_shutdown(), busy),
        Request::Finalize(reply) => {
            platform.finalize();
            answer(reply, Ok(()), busy);
        }
    }
}

#[async_trait(?Send)]
impl StatusLed for PlatformLed {
    async fn set_status(&mut self, color: &str) -> LedSet {
        let color = color.to_string();
        match self.call(|reply| Request::Set(color, reply)).await {
            Some(Ok(answer)) => LedSet::Done(answer == Some(true)),
            Some(Err(PlatformError::NotSupported(_))) => LedSet::NotImplemented,
            Some(Err(PlatformError::Backend(e) | PlatformError::NotFound(e))) => LedSet::Failed(e),
            None => LedSet::Skipped,
        }
    }

    async fn get_status(&mut self) -> String {
        match self.call(Request::Get).await {
            Some(Ok(Some(led))) => led,
            _ => LED_NOT_AVAILABLE.to_string(),
        }
    }

    /// Runs the SIGTERM handlers the vendor installed while its chassis was built, which the platform keeps rather
    /// than letting them act on the signal. In a Python daemon they run first on SIGTERM, then the daemon's own.
    async fn notify_shutdown(&mut self) {
        if let Some(Err(e)) = self.call_when_free(Request::NotifyShutdown, "SIGTERM handlers").await {
            log::warning(
                log::HEALTHD,
                &format!("The platform failed to handle the shutdown: {e}"),
            );
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use platform_api::ChassisInfo;
    use std::sync::Mutex;

    /// A chassis whose LED holds the last color set; `hang` delays every call.
    #[derive(Clone, Default)]
    struct FakeChassis {
        led: Arc<Mutex<Option<String>>>,
        calls: Arc<Mutex<Vec<String>>>,
        hang: Option<Duration>,
        unsupported: bool,
        finalized: Arc<AtomicBool>,
    }

    impl FakeChassis {
        fn record(&self, call: String) -> Result<(), PlatformError> {
            self.calls.lock().unwrap().push(call);
            if let Some(hang) = self.hang {
                std::thread::sleep(hang);
            }
            if self.unsupported {
                return Err(PlatformError::NotSupported("ChassisBase.set_status_led".to_string()));
            }
            Ok(())
        }
    }

    impl PlatformApi for FakeChassis {
        fn get_chassis_info(&mut self, cols: ChassisInfoCols) -> Result<ChassisInfo, PlatformError> {
            self.record(format!("get_chassis_info {}", cols.bits()))?;
            Ok(ChassisInfo {
                status_led: self.led.lock().unwrap().clone(),
                ..Default::default()
            })
        }

        fn set_chassis_led(&mut self, color: &str) -> Result<Option<bool>, PlatformError> {
            self.record(format!("set_chassis_led {color}"))?;
            *self.led.lock().unwrap() = Some(color.to_string());
            Ok(Some(true))
        }

        fn initialize_system_led(&mut self) -> Result<Option<bool>, PlatformError> {
            self.record("initialize_system_led".to_string())?;
            Ok(Some(true))
        }

        fn notify_shutdown(&mut self) -> Result<(), PlatformError> {
            self.record("notify_shutdown".to_string())
        }

        fn finalize(&mut self) {
            self.finalized.store(true, Ordering::SeqCst);
        }
    }

    fn led_over(chassis: &FakeChassis, timeout: Duration) -> PlatformLed {
        let chassis = chassis.clone();
        PlatformLed::spawn(move || Ok(chassis), timeout).unwrap()
    }

    /// Waits on the real clock for the platform thread, which a loaded machine can run late; fails after 5 s.
    fn wait_for(what: &str, done: impl Fn() -> bool) {
        let started = std::time::Instant::now();
        while !done() {
            assert!(started.elapsed() < Duration::from_secs(5), "{what}");
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    #[tokio::test]
    async fn sets_initializes_reads_notifies_and_finalizes_the_chassis() {
        let chassis = FakeChassis::default();
        let mut led = led_over(&chassis, Duration::from_secs(5));
        assert_eq!(led.set_status("red").await, LedSet::Done(true));
        led.initialize().await;
        assert_eq!(led.get_status().await, "red");
        led.notify_shutdown().await;
        led.close().await;
        assert_eq!(
            *chassis.calls.lock().unwrap(),
            [
                "set_chassis_led red".to_string(),
                "initialize_system_led".to_string(),
                format!("get_chassis_info {}", STATUS_LED.bits()),
                "notify_shutdown".to_string(),
            ]
        );
        assert!(chassis.finalized.load(Ordering::SeqCst));
    }

    #[tokio::test]
    async fn a_platform_without_the_led_api_reads_as_not_available() {
        let chassis = FakeChassis {
            unsupported: true,
            ..Default::default()
        };
        let mut led = led_over(&chassis, Duration::from_secs(5));
        assert_eq!(led.set_status("red").await, LedSet::NotImplemented);
        assert_eq!(led.get_status().await, "N/A");
        assert_eq!(led.set_status("red").await, LedSet::NotImplemented);
        assert_eq!(chassis.calls.lock().unwrap().len(), 3);
    }

    #[tokio::test]
    async fn an_led_never_set_reads_as_not_available() {
        let mut led = led_over(&FakeChassis::default(), Duration::from_secs(5));
        assert_eq!(led.get_status().await, "N/A");
    }

    #[test]
    fn a_platform_that_cannot_open_is_an_error() {
        let opened = PlatformLed::spawn(
            || -> Result<FakeChassis, _> {
                Err(PlatformError::Backend("No module named 'sonic_platform'".to_string()))
            },
            Duration::from_secs(5),
        );
        assert_eq!(
            opened.err(),
            Some(PlatformError::Backend("No module named 'sonic_platform'".to_string()))
        );
    }

    #[tokio::test]
    async fn a_hung_call_is_abandoned_and_the_led_skipped_until_it_returns() {
        let chassis = FakeChassis {
            hang: Some(Duration::from_millis(600)),
            ..Default::default()
        };
        let mut led = led_over(&chassis, Duration::from_millis(100));
        assert_eq!(led.set_status("red").await, LedSet::Skipped);
        // Still in the first call: nothing is sent to the platform.
        assert_eq!(led.set_status("amber").await, LedSet::Skipped);
        assert_eq!(led.get_status().await, "N/A");
        wait_for("the platform thread never took the set", || {
            !chassis.calls.lock().unwrap().is_empty()
        });
        assert_eq!(chassis.calls.lock().unwrap().len(), 1);

        wait_for("the hung call never returned", || !led.busy.load(Ordering::SeqCst));
        assert_eq!(*chassis.led.lock().unwrap(), Some("red".to_string()));
        let mut led = PlatformLed {
            timeout: Duration::from_secs(5),
            ..led
        };
        assert_eq!(led.set_status("green").await, LedSet::Done(true));
        assert_eq!(chassis.calls.lock().unwrap().len(), 2);
    }

    #[tokio::test]
    async fn the_stop_waits_for_a_call_cut_short() {
        let chassis = FakeChassis {
            hang: Some(Duration::from_millis(300)),
            ..Default::default()
        };
        let mut led = led_over(&chassis, Duration::from_secs(5));
        // The check is cut short by the stop while the set is running.
        let cut_short = tokio::time::timeout(Duration::from_millis(50), led.set_status("red")).await;
        assert!(cut_short.is_err());
        // The stop waits for the set rather than skipping the vendor's handlers.
        led.notify_shutdown().await;
        led.close().await;
        assert_eq!(
            *chassis.calls.lock().unwrap(),
            ["set_chassis_led red".to_string(), "notify_shutdown".to_string()]
        );
        assert!(chassis.finalized.load(Ordering::SeqCst));
    }

    // Paused time: the waits end on tokio's clock, so a loaded machine cannot stretch them; a stop that blocked on
    // the hung call would still be caught by the calls and finalize checks.
    #[tokio::test(start_paused = true)]
    async fn the_stop_gives_up_on_a_call_that_does_not_end() {
        let chassis = FakeChassis {
            hang: Some(Duration::from_secs(3)),
            ..Default::default()
        };
        let mut led = led_over(&chassis, Duration::from_millis(300));
        assert_eq!(led.set_status("red").await, LedSet::Skipped);
        // Paused time runs ahead of the platform thread: let it take the set before the stop.
        wait_for("the platform thread never took the set", || {
            !chassis.calls.lock().unwrap().is_empty()
        });
        let stop = tokio::time::Instant::now();
        led.notify_shutdown().await;
        led.close().await;
        // One timeout for the whole stop, not one per step (two would be 600 ms).
        assert!(stop.elapsed() < Duration::from_millis(550), "{:?}", stop.elapsed());
        assert_eq!(chassis.calls.lock().unwrap().len(), 1);
        assert!(!chassis.finalized.load(Ordering::SeqCst));
    }

    #[tokio::test]
    async fn nothing_is_sent_once_the_stop_ran_out_of_time() {
        let chassis = FakeChassis::default();
        let mut led = led_over(&chassis, Duration::from_millis(300));
        // The rest of the stop used up the time before the vendor's handlers came.
        led.stop_deadline = Some(tokio::time::Instant::now());
        led.notify_shutdown().await;
        led.close().await;
        // A request sent anyway would be served while the process exits; give the thread time to show one.
        std::thread::sleep(Duration::from_millis(200));
        assert!(chassis.calls.lock().unwrap().is_empty());
        assert!(!chassis.finalized.load(Ordering::SeqCst));
    }

    #[test]
    fn the_messages_python_printed() {
        assert_eq!(
            LedSet::NotImplemented.message().as_deref(),
            Some("chassis.set_status_led is not implemented")
        );
        assert_eq!(
            LedSet::Failed("ValueError: bad".to_string()).message().as_deref(),
            Some("Failed to set system led due to - ValueError: bad")
        );
        assert_eq!(LedSet::Done(false).message(), None);
        assert_eq!(LedSet::Skipped.message(), None);
    }
}
