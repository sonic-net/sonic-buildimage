//! The status LED through the real platform API: the PyO3 bridge and sonic_platform_api.facade of the
//! sonic-platform-common submodule, over a stub sonic_platform package.
//!
//! One test: the interpreter, sys.modules and sys.argv are shared by the whole process.

use std::collections::BTreeSet;
use std::time::Duration;

use platform_provider::PlatformImpl;
use pyo3::ffi::c_str;
use pyo3::prelude::*;
use pyo3::types::PyModule;
use serde_json::Value;
use system_health::led::{sonic_platform_missing, LedSet, PlatformLed, StatusLed};

/// A stub vendor sonic_platform.platform module.
const VENDOR: &str = include_str!("vendor_platform.py");

fn vendor_attr<T: for<'a, 'py> FromPyObject<'a, 'py, Error = PyErr>>(expr: &str) -> T {
    Python::attach(|py| {
        let module = py.import("sonic_platform.platform").unwrap();
        py.eval(&std::ffi::CString::new(expr).unwrap(), Some(&module.dict()), None)
            .unwrap()
            .extract()
            .unwrap()
    })
}

/// Makes Platform().get_chassis() build `class`, and clears what Vendor recorded.
fn use_chassis(class: &str) {
    Python::attach(|py| {
        let platform = py.import("sonic_platform.platform").unwrap();
        let chassis = platform.getattr(class).unwrap();
        platform
            .getattr("Platform")
            .unwrap()
            .setattr("chassis", chassis)
            .unwrap();
        let vendor = platform.getattr("Vendor").unwrap();
        vendor.setattr("led", py.None()).unwrap();
        vendor.setattr("calls", 0).unwrap();
    });
}

/// Every LED color the vendors' system_health_monitoring_config.json files name.
fn vendor_colors() -> BTreeSet<String> {
    let configs: Value = serde_json::from_str(include_str!("vendor_configs.json")).unwrap();
    configs
        .as_array()
        .unwrap()
        .iter()
        .flat_map(|case| case["python"]["led_color"].as_object().unwrap().values())
        .map(|color| color.as_str().unwrap().to_string())
        .collect()
}

#[tokio::test]
async fn the_led_through_the_platform_api() {
    // The first open starts the interpreter, as in both binaries; the facade is found through PYTHONPATH.
    let common = concat!(env!("CARGO_MANIFEST_DIR"), "/../../../sonic-platform-common");
    std::env::set_var("PYTHONPATH", common);

    // No vendor package: the error names the module, as Python's ImportError does.
    match PlatformLed::open("healthd") {
        Err(platform_api::PlatformError::Backend(text)) => {
            assert!(text.starts_with("No module named 'sonic_platform'"), "{text}")
        }
        Err(e) => panic!("expected an import error, got {e:?}"),
        Ok(_) => panic!("opened a platform without sonic_platform"),
    }
    // Nothing flushes the vendor's sys.stdout at exit, so what it prints is written at once.
    let write_through: bool = Python::attach(|py| {
        py.import("sys")
            .unwrap()
            .getattr("stdout")
            .unwrap()
            .getattr("write_through")
            .unwrap()
            .extract()
            .unwrap()
    });
    assert!(write_through);

    Python::attach(|py| {
        let code = std::ffi::CString::new(VENDOR).unwrap();
        let platform =
            PyModule::from_code(py, &code, c_str!("platform.py"), c_str!("sonic_platform.platform")).unwrap();
        let package = PyModule::new(py, "sonic_platform").unwrap();
        package.setattr("platform", &platform).unwrap();
        let modules = py.import("sys").unwrap().getattr("modules").unwrap();
        modules.set_item("sonic_platform", &package).unwrap();
        modules.set_item("sonic_platform.platform", &platform).unwrap();
    });

    let mut led = PlatformLed::open("healthd").unwrap();
    let argv: Vec<String> = Python::attach(|py| py.import("sys").unwrap().getattr("argv").unwrap().extract().unwrap());
    assert_eq!(argv, ["healthd"]);
    assert_eq!(led.get_status().await, "N/A");
    assert_eq!(led.set_status("amber_blink").await, LedSet::Done(true));
    led.initialize().await;
    assert_eq!(led.get_status().await, "amber_blink");
    assert!(vendor_attr::<bool>("Chassis.initialized"));
    // The handler the chassis installed on SIGTERM is kept and runs when the daemon stops, as in a Python daemon.
    assert!(!vendor_attr::<bool>("Chassis.shutdown"));
    led.notify_shutdown().await;
    assert!(vendor_attr::<bool>("Chassis.shutdown"));
    assert!(!vendor_attr::<bool>("exited"));
    led.close().await;
    assert!(vendor_attr::<bool>("exited"));

    // A chassis without the LED methods: nothing is set, the LED reads as not available.
    use_chassis("BareChassis");
    let mut led = PlatformLed::open("show").unwrap();
    assert_eq!(led.set_status("green").await, LedSet::NotImplemented);
    led.initialize().await;
    assert_eq!(led.get_status().await, "N/A");

    // A chassis that raises while it is built: the open fails, but sonic_platform is there, so both binaries go on
    // without the LED instead of stopping.
    use_chassis("FailsToBuild");
    match PlatformLed::open("healthd") {
        Err(e) => {
            assert!(!sonic_platform_missing(&e), "{e:?}");
            assert!(e.to_string().contains("chassis init failed"), "{e}");
        }
        Ok(_) => panic!("opened a chassis that raised"),
    }

    the_vendor_kinds().await;
    a_slow_vendor_call().await;
}

async fn the_vendor_kinds() {
    // Every vendor color reaches the vendor as written, spaces and all.
    use_chassis("Chassis");
    let mut led = PlatformLed::open("healthd").unwrap();
    let colors = vendor_colors();
    assert!(colors.contains("blinking green"), "{colors:?}");
    for color in &colors {
        assert_eq!(led.set_status(color).await, LedSet::Done(true), "{color}");
        assert_eq!(led.get_status().await, *color);
    }

    // Methods that answer None: the color is set, there is none to read.
    use_chassis("FallsThrough");
    let mut led = PlatformLed::open("healthd").unwrap();
    assert_eq!(led.set_status("green").await, LedSet::Done(false));
    assert_eq!(vendor_attr::<String>("Vendor.led"), "green");
    led.initialize().await;
    assert_eq!(led.get_status().await, "N/A");

    // False from get_status_led reads as the facade's str() of it; the Python CLI failed on it.
    use_chassis("AnswersFalse");
    let mut led = PlatformLed::open("healthd").unwrap();
    assert_eq!(led.set_status("green").await, LedSet::Done(false));
    led.initialize().await;
    assert_eq!(led.get_status().await, "False");

    use_chassis("Unsupported");
    let mut led = PlatformLed::open("healthd").unwrap();
    assert_eq!(led.set_status("green").await, LedSet::NotImplemented);
    led.initialize().await;
    assert_eq!(led.get_status().await, "N/A");

    // An exception is answered like Python's except clause: not set, and the next call still reaches the vendor.
    use_chassis("Raises");
    let mut led = PlatformLed::open("healthd").unwrap();
    let set = led.set_status("green").await;
    // The exception's message, not Python's repr() of it: the platform API passes no exception type.
    assert_eq!(
        set.message().as_deref(),
        Some("Failed to set system led due to - LED write failed")
    );
    assert_eq!(led.get_status().await, "N/A");
    assert!(matches!(led.set_status("green").await, LedSet::Failed(_)));
    assert_eq!(vendor_attr::<i64>("Vendor.calls"), 3);
}

/// A vendor call longer than the timeout is abandoned; the LED is skipped until it returns, then used again.
async fn a_slow_vendor_call() {
    use_chassis("SlowOnce");
    let mut led = PlatformLed::spawn(
        || platform_provider::open("healthd", PlatformImpl::Pyo3),
        Duration::from_millis(200),
    )
    .unwrap();
    assert_eq!(led.set_status("amber").await, LedSet::Skipped);
    assert_eq!(led.get_status().await, "N/A");
    // The platform thread can run late on a loaded machine: wait for it on the real clock, 5 s at most.
    let started = std::time::Instant::now();
    while vendor_attr::<i64>("Vendor.calls") == 0 {
        assert!(
            started.elapsed() < Duration::from_secs(5),
            "the platform thread never took the set"
        );
        tokio::time::sleep(Duration::from_millis(10)).await;
    }
    assert_eq!(vendor_attr::<i64>("Vendor.calls"), 1);
    // Free again once the slow call returns: the read reaches the vendor and finds the color it set.
    while led.get_status().await != "amber" {
        assert!(
            started.elapsed() < Duration::from_secs(5),
            "the slow call never returned"
        );
        tokio::time::sleep(Duration::from_millis(10)).await;
    }
    assert_eq!(led.set_status("green").await, LedSet::Done(true));
    assert_eq!(led.get_status().await, "green");
}
