mod common;

use std::time::Duration;

use async_trait::async_trait;
use common::Harness;
use serde_json::{json, Value};
use system_health::config::{Config, CONFIG_FILE};
use system_health::led::{LedSet, StatusLed};
use system_health::manager::HealthCheckManager;
use system_health::publisher::{self, SYSTEM_HEALTH_TABLE_NAME};
use system_health::status::{STATUS_NOT_OK, STATUS_OK};
use system_health::user_defined::UserDefinedChecker;

#[derive(Default)]
struct RecordingLed {
    colors: Vec<String>,
}

#[async_trait(?Send)]
impl StatusLed for RecordingLed {
    async fn set_status(&mut self, color: &str) -> LedSet {
        self.colors.push(color.to_string());
        LedSet::Done(true)
    }

    async fn get_status(&mut self) -> String {
        self.colors.last().cloned().unwrap_or_default()
    }
}

fn healthy(h: &Harness) {
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "20"), ("high_threshold", "21")],
    );
}

fn manager(h: &Harness, config_json: &str) -> HealthCheckManager {
    let path = h.platform_dir().join(CONFIG_FILE);
    std::fs::write(&path, config_json).unwrap();
    let monitrc = h.dir.path().join("monitrc");
    std::fs::write(&monitrc, "set daemon 60\n    with start delay 300    # delay\n").unwrap();
    HealthCheckManager::new(Config::new(path, monitrc))
}

const CONFIG: &str = r#"{"devices_to_ignore": ["psu", "fan"], "polling_interval": 30,
    "led_color": {"fault": "amber", "normal": "green", "booting": "orange_blink"}}"#;

#[tokio::test]
async fn healthy_system_is_green() {
    let h = Harness::new();
    healthy(&h);
    let mut led = RecordingLed::default();
    let result = manager(&h, CONFIG).check(&h.ctx(), &mut led).await;
    assert_eq!(result.summary, STATUS_OK);
    assert_eq!(led.colors, ["green"]);
    assert_eq!(result.stats.keys().collect::<Vec<_>>(), ["Services", "Hardware"]);
}

#[tokio::test]
async fn led_is_booting_during_monit_start_delay_then_fault() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "30"), ("high_threshold", "21")],
    );
    let mut manager = manager(&h, CONFIG);
    let mut led = RecordingLed::default();

    h.set_uptime(120.0);
    let result = manager.check(&h.ctx(), &mut led).await;
    assert_eq!(result.summary, STATUS_NOT_OK);
    h.set_uptime(301.0);
    manager.check(&h.ctx(), &mut led).await;
    assert_eq!(led.colors, ["orange_blink", "amber"]);
}

#[tokio::test]
async fn default_led_colors_without_led_config() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "30"), ("high_threshold", "21")],
    );
    let mut led = RecordingLed::default();
    h.set_uptime(5000.0);
    manager(&h, r#"{"devices_to_ignore": ["psu", "fan"]}"#)
        .check(&h.ctx(), &mut led)
        .await;
    assert_eq!(led.colors, ["red"]);
}

#[tokio::test]
async fn checker_error_is_reported_as_internal() {
    let h = Harness::new();
    healthy(&h);
    h.config_db.set("FEATURE", "bgp", &[("auto_restart", "enabled")]);
    let mut led = RecordingLed::default();
    let result = manager(&h, CONFIG).check(&h.ctx(), &mut led).await;
    assert_eq!(result.summary, STATUS_NOT_OK);
    let internal = &result.stats["Internal"]["ServiceChecker"];
    assert_eq!(internal.object_type, "Internal");
    assert_eq!(
        internal.message,
        "Failed to perform health check for ServiceChecker due to exception - KeyError('state')"
    );
    assert!(!result.stats.contains_key("Services"));
}

#[tokio::test]
async fn user_defined_checkers_merge_by_category() {
    let h = Harness::new();
    healthy(&h);
    h.commands
        .set("check_a", "MyCategory\nDevice1:OK\nDevice2:Out of power\n");
    h.commands
        .set("check_b", "  MyCategory  \n\n Device3 : OK \nno colon here\n");
    h.commands.set("check_c", "Hardware\nfan9:Broken\n");
    let config = r#"{"devices_to_ignore": ["psu", "fan"], "polling_interval": 45,
        "user_defined_checkers": ["check_a", "check_b", "check_c", "check_missing", "check_a"]}"#;
    let mut led = RecordingLed::default();
    let result = manager(&h, config).check(&h.ctx(), &mut led).await;

    let my = &result.stats["MyCategory"];
    assert_eq!(my.keys().collect::<Vec<_>>(), ["Device1", "Device2", "Device3"]);
    assert_eq!(my["Device2"].message, "Out of power");
    assert_eq!(my["Device2"].object_type, "UserDefine");
    assert_eq!(my["Device3"].status, STATUS_OK);
    assert_eq!(result.stats["Hardware"]["fan9"].message, "Broken");
    let failed = &result.stats["UserDefine"]["UserDefinedChecker - check_missing"];
    assert_eq!(failed.message, "Failed to get output of command \"check_missing\"");
    assert_eq!(
        result.stats.keys().collect::<Vec<_>>(),
        ["Services", "Hardware", "MyCategory", "UserDefine"]
    );
    let user_calls: Vec<_> = h
        .commands
        .calls
        .borrow()
        .iter()
        .filter(|(c, _)| c.starts_with("check_"))
        .cloned()
        .collect();
    assert_eq!(user_calls.len(), 4);
    assert!(user_calls
        .iter()
        .all(|(_, timeout)| *timeout == Duration::from_secs(45)));
}

/// The vendors' check scripts (BlueField, Pensando, Super Micro), with outputs in their formats, give the categories,
/// objects and summary the Python health_checker gave (vendor_checks.json). Commands run in file order; Python ran
/// them in the order of a set.
#[tokio::test]
async fn vendor_check_scripts_as_in_python() {
    let golden: Value = serde_json::from_str(include_str!("vendor_checks.json")).unwrap();
    for case in golden.as_array().unwrap() {
        let h = Harness::new();
        healthy(&h);
        let mut commands = Vec::new();
        for output in case["outputs"].as_array().unwrap() {
            let command = output[0].as_str().unwrap();
            h.commands.set(command, output[1].as_str().unwrap());
            commands.push(command);
        }
        let config = json!({"devices_to_ignore": ["psu", "fan"], "user_defined_checkers": commands}).to_string();
        let result = manager(&h, &config).check(&h.ctx(), &mut RecordingLed::default()).await;
        let stats: Vec<Value> = result
            .stats
            .iter()
            .filter(|(category, _)| !["Services", "Hardware"].contains(&category.as_str()))
            .map(|(category, info)| {
                let objects: Vec<Value> = info
                    .iter()
                    .map(|(name, o)| json!([name, {"type": o.object_type, "status": o.status, "message": o.message}]))
                    .collect();
                json!([category, objects])
            })
            .collect();
        assert_eq!(Value::from(stats), case["python"]["stats"], "{}", case["case"]);
        assert_eq!(result.summary, case["python"]["summary"], "{}", case["case"]);
    }
}

#[tokio::test]
async fn user_defined_checker_alone() {
    let h = Harness::new();
    h.commands.set("echo", "   \n  \n");
    let (category, info) = UserDefinedChecker::new("echo")
        .check(&h.commands, Duration::from_secs(1))
        .await;
    assert_eq!(category, "UserDefine");
    assert_eq!(
        info.get("UserDefinedChecker - echo").unwrap().message,
        "Failed to get output of command \"echo\""
    );
}

#[tokio::test]
async fn publish_rewrites_the_table() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "30"), ("high_threshold", "21")],
    );
    let mut manager = manager(&h, CONFIG);
    let mut led = RecordingLed::default();
    let result = manager.check(&h.ctx(), &mut led).await;
    publisher::publish(&h.state_db, &result).unwrap();
    let table = h.state_db.entry(SYSTEM_HEALTH_TABLE_NAME).unwrap();
    assert_eq!(table["summary"], STATUS_NOT_OK);
    assert_eq!(
        table["ASIC"],
        "ASIC temperature is too hot, temperature=30.0, threshold=21.0"
    );
    assert_eq!(table["system"], "no critical process found");
    assert!(!table.contains_key("sonic"));

    healthy(&h);
    let result = manager.check(&h.ctx(), &mut led).await;
    publisher::publish(&h.state_db, &result).unwrap();
    let table = h.state_db.entry(SYSTEM_HEALTH_TABLE_NAME).unwrap();
    assert_eq!(table.len(), 1);
    assert_eq!(table["summary"], STATUS_OK);

    publisher::clear(&h.state_db).unwrap();
    assert!(h.state_db.entry(SYSTEM_HEALTH_TABLE_NAME).is_none());
}

#[tokio::test]
async fn critical_process_state_is_kept_across_cycles() {
    let h = Harness::new();
    healthy(&h);
    let mut manager = manager(&h, CONFIG);
    let mut led = RecordingLed::default();
    manager.check(&h.ctx(), &mut led).await;
    std::fs::remove_file(h.merged_dir("swss").join("etc/supervisor/critical_processes")).unwrap();
    h.set_supervisor("swss", &[("orchagent", "STOPPED")]);
    let result = manager.check(&h.ctx(), &mut led).await;
    assert_eq!(result.stats["Services"]["swss:orchagent"].status, STATUS_NOT_OK);
}
