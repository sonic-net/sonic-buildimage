//! Every vendor's system_health_monitoring_config.json, read as the Python health_checker read it.
//!
//! vendor_configs.json holds each distinct file under device/ once, the platforms that use it, and what Python's
//! Config made of it: the interval, the lists (sets in Python, sorted here), the optional containers and the LED
//! color for each status.

use serde_json::{json, Value};
use system_health::config::{Config, CONFIG_FILE};

fn sorted(list: &Option<Vec<String>>) -> Value {
    json!(list.as_ref().map(|items| {
        let mut items = items.clone();
        items.sort();
        items
    }))
}

#[test]
fn vendor_configs_load_as_in_python() {
    let golden: Value = serde_json::from_str(include_str!("vendor_configs.json")).unwrap();
    let cases = golden.as_array().unwrap();
    assert!(cases.len() >= 50);
    let dir = tempfile::tempdir().unwrap();
    let path = dir.path().join(CONFIG_FILE);
    for case in cases {
        std::fs::write(&path, case["config"].as_str().unwrap()).unwrap();
        let mut config = Config::new(path.clone(), dir.path().join("monitrc"));
        config.load_config();
        let (platforms, mut python) = (&case["platforms"], case["python"].clone());
        let interval = python.as_object_mut().unwrap().remove("interval").unwrap();
        assert_eq!(config.interval, interval.as_f64().unwrap(), "{platforms}");
        let ours = json!({
            "services_to_ignore": sorted(&config.ignore_services),
            "devices_to_ignore": sorted(&config.ignore_devices),
            "user_defined_checkers": sorted(&config.user_defined_checkers),
            "include_devices": sorted(&config.include_devices),
            "optional_containers": config.optional_containers,
            "led_color": {
                "normal": config.get_led_color("normal"),
                "booting": config.get_led_color("booting"),
                "fault": config.get_led_color("fault"),
            },
        });
        assert_eq!(ours, python, "{platforms}");
    }
}
