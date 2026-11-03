mod common;

use common::Harness;
use system_health::config::Config;
use system_health::db::StateDb;
use system_health::hardware::HardwareChecker;
use system_health::status::{CheckerInfo, STATUS_NOT_OK, STATUS_OK};

fn config(ignore: &[&str], include: &[&str]) -> Config {
    let dir = std::env::temp_dir();
    let mut config = Config::new(dir.join("no-such-config.json"), dir.join("no-such-monitrc"));
    if !ignore.is_empty() {
        config.ignore_devices = Some(ignore.iter().map(|s| s.to_string()).collect());
    }
    if !include.is_empty() {
        config.include_devices = Some(include.iter().map(|s| s.to_string()).collect());
    }
    config
}

fn status(info: &CheckerInfo, name: &str) -> String {
    info.get(name)
        .map(|o| o.status.clone())
        .unwrap_or_else(|| panic!("{name} not checked"))
}

fn message(info: &CheckerInfo, name: &str) -> String {
    info.get(name)
        .map(|o| o.message.clone())
        .unwrap_or_else(|| panic!("{name} not checked"))
}

fn psu(voltage: &str, min: &str, max: &str) -> Vec<(&'static str, String)> {
    vec![
        ("presence", "True".into()),
        ("status", "True".into()),
        ("temp", "55".into()),
        ("temp_threshold", "100".into()),
        ("voltage", voltage.into()),
        ("voltage_min_threshold", min.into()),
        ("voltage_max_threshold", max.into()),
    ]
}

fn set(h: &Harness, key: &str, fields: &[(&str, String)]) {
    let fields: Vec<(&str, &str)> = fields.iter().map(|(f, v)| (*f, v.as_str())).collect();
    h.state_db.set(key, &fields);
}

fn fan(presence: &str, status: &str, speed: &str, under: &str, over: &str) -> Vec<(&'static str, String)> {
    vec![
        ("presence", presence.into()),
        ("status", status.into()),
        ("speed", speed.into()),
        ("speed_target", "60".into()),
        ("is_under_speed", under.into()),
        ("is_over_speed", over.into()),
    ]
}

#[test]
fn hardware_checker_matches_python() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "20"), ("high_threshold", "21")],
    );
    let mut fan1 = fan("True", "True", "60", "False", "False");
    fan1.push(("direction", "intake".into()));
    set(&h, "FAN_INFO|fan1", &fan1);
    set(&h, "FAN_INFO|fan2", &fan("False", "True", "60", "False", "False"));
    set(&h, "FAN_INFO|fan3", &fan("True", "False", "60", "False", "False"));
    set(&h, "FAN_INFO|fan4", &fan("True", "True", "20", "True", "False"));
    set(&h, "FAN_INFO|fan5", &fan("True", "True", "90", "False", "True"));
    let mut fan6 = fan("True", "True", "60", "False", "False");
    fan6.push(("direction", "exhaust".into()));
    set(&h, "FAN_INFO|fan6", &fan6);

    set(&h, "PSU_INFO|PSU 1", &psu("10", "8", "15"));
    let mut psu2 = psu("10", "8", "15");
    psu2[0].1 = "False".into();
    set(&h, "PSU_INFO|PSU 2", &psu2);
    let mut psu3 = psu("10", "8", "15");
    psu3[1].1 = "False".into();
    set(&h, "PSU_INFO|PSU 3", &psu3);
    let mut psu4 = psu("10", "8", "15");
    psu4[2].1 = "101".into();
    set(&h, "PSU_INFO|PSU 4", &psu4);
    set(&h, "PSU_INFO|PSU 5", &psu("10", "12", "15"));
    let mut psu6 = psu("12", "12", "15");
    psu6.extend([
        ("power_overload", "True".into()),
        ("power", "101.0".into()),
        ("power_critical_threshold", "100.0".into()),
        ("power_warning_suppress_threshold", "90.0".into()),
    ]);
    set(&h, "PSU_INFO|PSU 6", &psu6);
    let mut psu7 = psu("12", "12", "15");
    psu7.extend([("power_overload", "True".into()), ("power", "101.0".into())]);
    set(&h, "PSU_INFO|PSU 7", &psu7);
    set(&h, "PSU_INFO|PDB 1", &psu("10", "8", "15"));
    h.state_db
        .set("PSU_INFO|PDB 2", &[("presence", "True"), ("status", "False")]);
    h.state_db
        .set("PSU_INFO|PDB 3", &[("presence", "False"), ("status", "True")]);

    for (i, leak) in ["Yes", "No", "Yes", "No", "Yes", "No"].iter().enumerate() {
        let name = format!("liquid_cooling_{}", i + 1);
        h.state_db.set(
            &format!("LIQUID_COOLING_INFO|{name}"),
            &[("leak_status", leak), ("leak_sensor_name", &name)],
        );
    }

    let mut checker = HardwareChecker::default();
    let info = checker
        .check(&config(&[], &["liquid_cooling"]), &h.state_db, &h.events)
        .unwrap();

    assert_eq!(status(&info, "ASIC"), STATUS_OK);
    assert_eq!(status(&info, "fan1"), STATUS_OK);
    for name in ["fan2", "fan3", "fan4", "fan5", "fan6"] {
        assert_eq!(status(&info, name), STATUS_NOT_OK, "{name}");
    }
    assert_eq!(message(&info, "fan2"), "fan2 is missing");
    assert_eq!(message(&info, "fan3"), "fan3 is broken");
    assert_eq!(
        message(&info, "fan4"),
        "fan4 speed is out of range, speed=20.0, target=60.0"
    );
    assert_eq!(
        message(&info, "fan6"),
        "fan6 direction exhaust is not aligned with fan1 direction intake"
    );

    assert_eq!(status(&info, "PSU 1"), STATUS_OK);
    assert_eq!(message(&info, "PSU 2"), "PSU 2 is missing or not available");
    assert_eq!(message(&info, "PSU 3"), "PSU 3 is out of power");
    assert_eq!(
        message(&info, "PSU 4"),
        "PSU 4 temperature is too hot, temperature=101.0, threshold=100.0"
    );
    assert_eq!(
        message(&info, "PSU 5"),
        "PSU 5 voltage is out of range, voltage=10.0, range=[12.0,15.0]"
    );
    assert_eq!(message(&info, "PSU 6"), "System power exceeds threshold (100.0w)");
    assert_eq!(
        message(&info, "PSU 7"),
        "System power exceeds threshold but power_critical_threshold is invalid"
    );

    for (name, expected) in [
        (1, STATUS_NOT_OK),
        (2, STATUS_OK),
        (3, STATUS_NOT_OK),
        (4, STATUS_OK),
        (5, STATUS_NOT_OK),
        (6, STATUS_OK),
    ] {
        assert_eq!(status(&info, &format!("liquid_cooling_{name}")), expected);
    }
    assert_eq!(
        message(&info, "liquid_cooling_1"),
        "Leakage sensor liquid_cooling_1 is leaking"
    );

    assert_eq!(info.get("PDB 1").unwrap().object_type, "PSU");
    assert_eq!(status(&info, "PDB 1"), STATUS_OK);
    assert_eq!(message(&info, "PDB 2"), "PDB 2 is out of power");
    assert_eq!(message(&info, "PDB 3"), "PDB 3 is missing or not available");
    assert!(info.has_not_ok());
}

#[test]
fn objects_follow_natural_order() {
    let h = Harness::new();
    for i in [10, 2, 1] {
        set(
            &h,
            &format!("FAN_INFO|fan{i}"),
            &fan("True", "True", "60", "False", "False"),
        );
    }
    let info = HardwareChecker::default()
        .check(&config(&["psu", "asic"], &[]), &h.state_db, &h.events)
        .unwrap();
    let names: Vec<&String> = info.info().keys().collect();
    assert_eq!(names, ["fan1", "fan2", "fan10"]);
}

#[test]
fn invalid_numbers_are_reported_as_python_does() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "hot"), ("high_threshold", "21")],
    );
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC1",
        &[("temperature", "30.5"), ("high_threshold", "21")],
    );
    h.state_db.set("TEMPERATURE_INFO|ASIC2", &[("temperature", "20")]);
    set(&h, "FAN_INFO|fan1", &fan("True", "True", "60", "False", "False"));
    h.state_db.set("FAN_INFO|fan1", &[("speed_target", "abc")]);
    let mut psu1 = psu("10", "8", "x");
    psu1[2].1 = "55".into();
    set(&h, "PSU_INFO|PSU 1", &psu1);
    let mut psu2 = psu("10", "8", "15");
    psu2[3].1 = "".into();
    set(&h, "PSU_INFO|PSU 2", &psu2);

    let info = HardwareChecker::default()
        .check(&config(&[], &[]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(
        message(&info, "ASIC"),
        "Invalid ASIC temperature data, temperature=hot, threshold=21"
    );
    assert_eq!(
        message(&info, "ASIC1"),
        "ASIC1 temperature is too hot, temperature=30.5, threshold=21.0"
    );
    assert_eq!(message(&info, "ASIC2"), "Failed to get ASIC2 temperature threshold");
    assert_eq!(
        message(&info, "fan1"),
        "Invalid fan speed data for fan1, speed=60.0, target=abc, is_under_speed=False, is_over_speed=False"
    );
    assert_eq!(
        message(&info, "PSU 1"),
        "Invalid voltage data for PSU 1, voltage=10.0, range=[8.0,x]"
    );
    assert_eq!(
        message(&info, "PSU 2"),
        "Invalid temperature data for PSU 2, temperature=55.0, threshold="
    );
}

#[test]
fn missing_tables_and_fields() {
    let h = Harness::new();
    let info = HardwareChecker::default()
        .check(&config(&[], &[]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(message(&info, "Fan"), "Failed to get fan information");
    assert_eq!(message(&info, "PSU"), "Failed to get PSU information");

    h.state_db
        .set("FAN_INFO|fan1", &[("presence", "True"), ("speed", "60")]);
    h.state_db.set(
        "FAN_INFO|fan2",
        &[("presence", "True"), ("speed", "60"), ("speed_target", "60")],
    );
    h.state_db.set("FAN_INFO|bad|key", &[("presence", "True")]);
    h.state_db
        .set("PSU_INFO|PSU 1", &[("presence", "True"), ("status", "True")]);
    h.state_db.set(
        "PSU_INFO|PSU 2",
        &[
            ("presence", "True"),
            ("status", "True"),
            ("temp", "1"),
            ("temp_threshold", "N/A"),
        ],
    );
    let info = HardwareChecker::default()
        .check(&config(&[], &[]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(message(&info, "fan1"), "Failed to get target speed data for fan1");
    assert_eq!(
        message(&info, "fan2"),
        "Failed to get under speed threshold check for fan2"
    );
    assert_eq!(
        message(&info, "FAN_INFO|bad|key"),
        "Invalid key for FAN_INFO: FAN_INFO|bad|key"
    );
    assert_eq!(message(&info, "PSU 1"), "Failed to get temperature data for PSU 1");
    assert_eq!(message(&info, "PSU 2"), "Failed to get voltage data for PSU 2");
}

#[test]
fn ignored_check_points() {
    let h = Harness::new();
    set(&h, "FAN_INFO|fan1", &fan("True", "True", "20", "True", "False"));
    let mut fan2 = fan("True", "True", "60", "False", "False");
    fan2.push(("direction", "intake".into()));
    set(&h, "FAN_INFO|fan2", &fan2);
    let mut fan3 = fan("True", "True", "60", "False", "False");
    fan3.push(("direction", "exhaust".into()));
    set(&h, "FAN_INFO|fan3", &fan3);
    let mut psu1 = psu("10", "12", "15");
    psu1[2].1 = "101".into();
    set(&h, "PSU_INFO|PSU 1", &psu1);

    let checks = ["fan.speed", "fan3.direction", "psu.temperature", "PSU 1.voltage"];
    let info = HardwareChecker::default()
        .check(&config(&checks, &[]), &h.state_db, &h.events)
        .unwrap();
    for name in ["fan1", "fan2", "fan3", "PSU 1"] {
        assert_eq!(status(&info, name), STATUS_OK, "{name}");
    }
}

#[test]
fn ignored_devices() {
    let h = Harness::new();
    h.state_db.set(
        "TEMPERATURE_INFO|ASIC",
        &[("temperature", "30"), ("high_threshold", "21")],
    );
    set(&h, "FAN_INFO|fan1", &fan("False", "True", "60", "False", "False"));
    set(&h, "FAN_INFO|fan2", &fan("False", "True", "60", "False", "False"));
    set(&h, "PSU_INFO|PDB 1", &psu("10", "8", "15"));
    let info = HardwareChecker::default()
        .check(&config(&["asic", "fan2", "PDB 1"], &[]), &h.state_db, &h.events)
        .unwrap();
    assert!(info.get("ASIC").is_none());
    assert!(info.get("fan2").is_none());
    assert_eq!(status(&info, "fan1"), STATUS_NOT_OK);
    assert!(info.get("PDB 1").is_none());
}

#[test]
fn psu_and_pdb_ignores() {
    let h = Harness::new();
    set(&h, "PSU_INFO|PSU 1", &psu("10", "8", "15"));
    let info = HardwareChecker::default()
        .check(&config(&["psu", "pdb", "fan"], &[]), &h.state_db, &h.events)
        .unwrap();
    assert!(info.get("PSU 1").is_none());

    let h = Harness::new();
    let info = HardwareChecker::default()
        .check(&config(&["psu", "fan"], &[]), &h.state_db, &h.events)
        .unwrap();
    assert!(info.get("PSU").is_none());

    let h = Harness::new();
    h.state_db
        .set("PSU_INFO|PSU 1", &[("presence", "False"), ("status", "True")]);
    h.state_db
        .set("PSU_INFO|PDB 1", &[("presence", "True"), ("status", "False")]);
    let info = HardwareChecker::default()
        .check(&config(&["psu", "fan"], &[]), &h.state_db, &h.events)
        .unwrap();
    assert!(info.get("PSU 1").is_none());
    assert_eq!(status(&info, "PDB 1"), STATUS_NOT_OK);
}

#[test]
fn leak_is_not_ok_every_cycle_with_one_event_per_change() {
    let h = Harness::new();
    let config = config(&["fan", "psu", "asic"], &["liquid_cooling"]);
    let mut checker = HardwareChecker::default();
    h.state_db.set("LIQUID_COOLING_INFO|sensor1", &[("leak_status", "Yes")]);
    h.state_db.set("LIQUID_COOLING_INFO|sensor2", &[("leak_status", "No")]);

    for _ in 0..3 {
        let info = checker.check(&config, &h.state_db, &h.events).unwrap();
        assert_eq!(status(&info, "sensor1"), STATUS_NOT_OK);
        assert!(info.has_not_ok());
    }
    assert_eq!(
        *h.events.events.borrow(),
        vec![(
            "liquid-cooling-leak".to_string(),
            vec![("sensor report leaking event".to_string(), "sensor1".to_string())]
        )]
    );

    h.state_db.set("LIQUID_COOLING_INFO|sensor1", &[("leak_status", "No")]);
    let info = checker.check(&config, &h.state_db, &h.events).unwrap();
    assert_eq!(status(&info, "sensor1"), STATUS_OK);
    assert!(!info.has_not_ok());
    checker.check(&config, &h.state_db, &h.events).unwrap();
    let events = h.events.events.borrow();
    assert_eq!(events.len(), 2);
    assert_eq!(
        events[1].1,
        vec![("leaking sensor report recovered".to_string(), "sensor1".to_string())]
    );
}

const LEAKED: &str = "sensor report leaking event";
const RECOVERED: &str = "leaking sensor report recovered";

fn event(event: &str, sensor: &str) -> (String, String) {
    (event.to_string(), sensor.to_string())
}

/// The (event, sensor) pairs published since the last call.
fn take_leak_events(h: &Harness) -> Vec<(String, String)> {
    h.events
        .events
        .borrow_mut()
        .drain(..)
        .flat_map(|(tag, params)| {
            assert_eq!(tag, "liquid-cooling-leak");
            params
        })
        .collect()
}

/// Writes the sensor as thermalctld does: `leaking` and its `leak_status` copy.
fn set_leaking(h: &Harness, sensor: &str, value: &str) {
    h.state_db.set(
        &format!("LIQUID_COOLING_INFO|{sensor}"),
        &[("leaking", value), ("leak_status", value)],
    );
}

#[test]
fn leaking_field_is_read_before_its_compat_copy() {
    let h = Harness::new();
    h.state_db.set(
        "LIQUID_COOLING_INFO|sensor1",
        &[("leaking", "Yes"), ("leak_status", "No")],
    );
    h.state_db.set("LIQUID_COOLING_INFO|sensor2", &[("leaking", "No")]);
    h.state_db.set("LIQUID_COOLING_INFO|sensor3", &[("leak_status", "Yes")]);
    let info = HardwareChecker::default()
        .check(&config(&[], &["liquid_cooling"]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(status(&info, "sensor1"), STATUS_NOT_OK);
    assert_eq!(status(&info, "sensor2"), STATUS_OK);
    assert_eq!(status(&info, "sensor3"), STATUS_NOT_OK);
}

#[test]
fn unreadable_or_unexpected_leak_status_is_not_ok() {
    let h = Harness::new();
    for (i, value) in ["N/A", "n/a", "Unknown", ""].iter().enumerate() {
        set_leaking(&h, &format!("sensor{i}"), value);
    }
    h.state_db
        .set("LIQUID_COOLING_INFO|sensor4", &[("leak_sensor_status", "Good")]);
    let info = HardwareChecker::default()
        .check(&config(&[], &["liquid_cooling"]), &h.state_db, &h.events)
        .unwrap();
    for i in 0..5 {
        assert_eq!(status(&info, &format!("sensor{i}")), STATUS_NOT_OK, "sensor{i}");
    }
    assert_eq!(
        message(&info, "sensor0"),
        "Failed to get leakage sensor status for sensor0"
    );
    assert_eq!(
        message(&info, "sensor1"),
        "Unexpected leakage sensor status n/a for sensor1"
    );
    assert_eq!(
        message(&info, "sensor2"),
        "Unexpected leakage sensor status Unknown for sensor2"
    );
    assert_eq!(
        message(&info, "sensor3"),
        "Unexpected leakage sensor status  for sensor3"
    );
    assert_eq!(
        message(&info, "sensor4"),
        "Failed to get leakage sensor status for sensor4"
    );
}

#[test]
fn leak_events_per_sensor_and_per_change() {
    let h = Harness::new();
    let config = config(&[], &["liquid_cooling"]);
    let mut checker = HardwareChecker::default();
    let mut cycle = || checker.check(&config, &h.state_db, &h.events).unwrap();

    set_leaking(&h, "sensor1", "Yes");
    set_leaking(&h, "sensor2", "Yes");
    let info = cycle();
    assert_eq!(status(&info, "sensor1"), STATUS_NOT_OK);
    assert_eq!(status(&info, "sensor2"), STATUS_NOT_OK);
    assert_eq!(
        take_leak_events(&h),
        [event(LEAKED, "sensor1"), event(LEAKED, "sensor2")]
    );
    // As in Python: the new leaks in one batch.
    assert_eq!(take_batches(&h), [2]);
    cycle();
    assert!(take_leak_events(&h).is_empty());

    set_leaking(&h, "sensor1", "No");
    cycle();
    assert_eq!(take_leak_events(&h), [event(RECOVERED, "sensor1")]);
    set_leaking(&h, "sensor1", "Yes");
    cycle();
    assert_eq!(take_leak_events(&h), [event(LEAKED, "sensor1")]);

    // An unreadable sensor neither ends nor restarts the leak.
    set_leaking(&h, "sensor1", "N/A");
    assert_eq!(status(&cycle(), "sensor1"), STATUS_NOT_OK);
    set_leaking(&h, "sensor1", "Yes");
    cycle();
    assert!(take_leak_events(&h).is_empty());
    set_leaking(&h, "sensor1", "N/A");
    cycle();
    set_leaking(&h, "sensor1", "No");
    cycle();
    assert_eq!(take_leak_events(&h), [event(RECOVERED, "sensor1")]);

    set_leaking(&h, "sensor2", "No");
    cycle();
    assert_eq!(take_leak_events(&h), [event(RECOVERED, "sensor2")]);

    // Each recovery in a batch of its own, as in Python.
    set_leaking(&h, "sensor1", "Yes");
    set_leaking(&h, "sensor2", "Yes");
    cycle();
    take_leak_events(&h);
    take_batches(&h);
    set_leaking(&h, "sensor1", "No");
    set_leaking(&h, "sensor2", "No");
    cycle();
    assert_eq!(
        take_leak_events(&h),
        [event(RECOVERED, "sensor1"), event(RECOVERED, "sensor2")]
    );
    assert_eq!(take_batches(&h), [1, 1]);
}

/// The size of each event batch published since the last call.
fn take_batches(h: &Harness) -> Vec<usize> {
    h.events.batches.borrow_mut().drain(..).collect()
}

#[test]
fn ignored_or_missing_sensor_keeps_its_leak_state() {
    let h = Harness::new();
    let included = config(&[], &["liquid_cooling"]);
    let ignored = config(&["sensor1"], &["liquid_cooling"]);
    let mut checker = HardwareChecker::default();
    set_leaking(&h, "sensor1", "Yes");
    set_leaking(&h, "sensor2", "No");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(take_leak_events(&h), [event(LEAKED, "sensor1")]);

    // Ignored, then back: still the same leak; its recovery while ignored is reported once it is back.
    let info = checker.check(&ignored, &h.state_db, &h.events).unwrap();
    assert!(info.get("sensor1").is_none());
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert!(take_leak_events(&h).is_empty());
    checker.check(&ignored, &h.state_db, &h.events).unwrap();
    set_leaking(&h, "sensor1", "No");
    checker.check(&ignored, &h.state_db, &h.events).unwrap();
    assert!(take_leak_events(&h).is_empty());
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(take_leak_events(&h), [event(RECOVERED, "sensor1")]);

    // Missing, or liquid cooling not included: the same.
    set_leaking(&h, "sensor1", "Yes");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(take_leak_events(&h), [event(LEAKED, "sensor1")]);
    h.state_db.del("LIQUID_COOLING_INFO|sensor1").unwrap();
    checker.check(&included, &h.state_db, &h.events).unwrap();
    checker.check(&config(&[], &[]), &h.state_db, &h.events).unwrap();
    set_leaking(&h, "sensor1", "Yes");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert!(take_leak_events(&h).is_empty());
    set_leaking(&h, "sensor1", "No");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(take_leak_events(&h), [event(RECOVERED, "sensor1")]);

    // thermalctld deletes the whole table when it stops; the leak is not reported again after its restart.
    set_leaking(&h, "sensor1", "Yes");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(take_leak_events(&h), [event(LEAKED, "sensor1")]);
    h.state_db.delete_all_by_pattern("LIQUID_COOLING_INFO|*").unwrap();
    let info = checker.check(&included, &h.state_db, &h.events).unwrap();
    assert_eq!(status(&info, "Liquid Cooling"), STATUS_NOT_OK);
    set_leaking(&h, "sensor1", "Yes");
    set_leaking(&h, "sensor2", "No");
    checker.check(&included, &h.state_db, &h.events).unwrap();
    assert!(take_leak_events(&h).is_empty());
}

#[test]
fn liquid_cooling_only_when_included() {
    let h = Harness::new();
    h.state_db.set("LIQUID_COOLING_INFO|sensor1", &[("leak_status", "Yes")]);
    let info = HardwareChecker::default()
        .check(&config(&["fan", "psu"], &[]), &h.state_db, &h.events)
        .unwrap();
    assert!(info.get("sensor1").is_none());

    let h = Harness::new();
    h.state_db.set("LIQUID_COOLING_INFO|sensor1", &[("leak_status", "N/A")]);
    let info = HardwareChecker::default()
        .check(&config(&["fan", "psu"], &["liquid_cooling"]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(
        message(&info, "sensor1"),
        "Failed to get leakage sensor status for sensor1"
    );

    let h = Harness::new();
    let info = HardwareChecker::default()
        .check(&config(&["fan", "psu"], &["liquid_cooling"]), &h.state_db, &h.events)
        .unwrap();
    assert_eq!(
        message(&info, "Liquid Cooling"),
        "Failed to get liquid cooling information"
    );
}
