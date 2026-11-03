//! Hardware checks over the STATE_DB tables pmon writes.

use std::collections::BTreeSet;

use anyhow::Result;

use crate::config::Config;
use crate::db::{FieldMap, StateDb};
use crate::events::{EventSink, Params};
use crate::natsort::natsorted;
use crate::pyfmt::{py_float, py_repr};
use crate::status::CheckerInfo;

const ASIC_TEMPERATURE_KEY: &str = "TEMPERATURE_INFO|ASIC";
const FAN_TABLE_NAME: &str = "FAN_INFO";
const PSU_TABLE_NAME: &str = "PSU_INFO";
const LIQUID_COOLING_TABLE_NAME: &str = "LIQUID_COOLING_INFO";
const LEAK_EVENT_TAG: &str = "liquid-cooling-leak";
const LEAK_EVENT: &str = "sensor report leaking event";
const LEAK_RECOVERED_EVENT: &str = "leaking sensor report recovered";

/// A value converted to float when possible, shown the way Python shows it in messages.
enum Num {
    Parsed(f64),
    Raw(String),
}

impl std::fmt::Display for Num {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Num::Parsed(v) => f.write_str(&py_repr(*v)),
            Num::Raw(s) => f.write_str(s),
        }
    }
}

/// Converts in order and stops at the first failure, as a Python `try` block of `float()` calls.
fn convert_all(values: &[&str]) -> (Vec<Num>, bool) {
    let mut result = Vec::new();
    let mut ok = true;
    for value in values {
        match (ok, py_float(value)) {
            (true, Some(v)) => result.push(Num::Parsed(v)),
            _ => {
                ok = false;
                result.push(Num::Raw(value.to_string()));
            }
        }
    }
    (result, ok)
}

fn get<'a>(data: &'a FieldMap, field: &str) -> Option<&'a str> {
    data.get(field).map(String::as_str)
}

#[derive(Default)]
pub struct HardwareChecker {
    leaking_sensors: BTreeSet<String>,
}

impl HardwareChecker {
    pub const CATEGORY: &'static str = "Hardware";
    pub const NAME: &'static str = "HardwareChecker";

    pub fn check(&mut self, config: &Config, db: &dyn StateDb, events: &dyn EventSink) -> Result<CheckerInfo> {
        let mut info = CheckerInfo::default();
        check_asic_status(config, db, &mut info)?;
        check_fan_status(config, db, &mut info)?;
        check_psu_status(config, db, &mut info)?;
        self.check_liquid_cooling_status(config, db, events, &mut info)?;
        Ok(info)
    }

    /// A leaking sensor is Not OK every cycle; an event is sent when it starts and when it stops leaking.
    /// A sensor that is ignored or no longer in the table keeps its leak state, so its recovery is still reported.
    fn check_liquid_cooling_status(
        &mut self,
        config: &Config,
        db: &dyn StateDb,
        events: &dyn EventSink,
        info: &mut CheckerInfo,
    ) -> Result<()> {
        if !config.includes_device("liquid_cooling") {
            return Ok(());
        }
        // An empty table (thermalctld deletes it when it stops) keeps the leak state.
        let keys = db.keys(&format!("{LIQUID_COOLING_TABLE_NAME}|*"))?;
        if keys.is_empty() {
            info.set_object_not_ok(
                "Liquid Cooling",
                "Liquid Cooling",
                "Failed to get liquid cooling information",
            );
            return Ok(());
        }
        let mut new_leaking = Vec::new();
        for key in natsorted(keys) {
            let parts: Vec<&str> = key.split('|').collect();
            if parts.len() != 2 {
                info.set_object_not_ok(
                    "Liquid Cooling",
                    &key,
                    format!("Invalid key for {LIQUID_COOLING_TABLE_NAME}: {key}"),
                );
                continue;
            }
            let name = parts[1];
            // An ignored or missing sensor keeps its leak state, so its recovery is still reported.
            if config.ignores_device(name) {
                continue;
            }
            let data = db.get_all(&key)?;
            // thermalctld writes `leaking`; `leak_status` is its copy for older readers.
            let Some(value) = get(&data, "leaking")
                .or_else(|| get(&data, "leak_status"))
                .filter(|value| *value != "N/A")
            else {
                info.set_object_not_ok(
                    "Liquid Cooling",
                    name,
                    format!("Failed to get leakage sensor status for {name}"),
                );
                continue;
            };
            match value.to_lowercase().as_str() {
                "yes" => {
                    info.set_object_not_ok("Liquid Cooling", name, format!("Leakage sensor {name} is leaking"));
                    if self.leaking_sensors.insert(name.to_string()) {
                        new_leaking.push(name.to_string());
                    }
                }
                "no" => {
                    info.set_object_ok("Liquid Cooling", name);
                    if self.leaking_sensors.remove(name) {
                        events.publish(LEAK_EVENT_TAG, &[&[(LEAK_RECOVERED_EVENT, name)]]);
                    }
                }
                // A leaking sensor stays leaking.
                _ => info.set_object_not_ok(
                    "Liquid Cooling",
                    name,
                    format!("Unexpected leakage sensor status {value} for {name}"),
                ),
            }
        }
        let new_leaking: Vec<[(&str, &str); 1]> =
            new_leaking.iter().map(|name| [(LEAK_EVENT, name.as_str())]).collect();
        let batch: Vec<Params> = new_leaking.iter().map(|params| &params[..]).collect();
        events.publish(LEAK_EVENT_TAG, &batch);
        Ok(())
    }
}

fn ignore_check(config: &Config, category: &str, name: &str, check_point: &str) -> bool {
    config.ignores_device(&format!("{category}.{check_point}"))
        || config.ignores_device(&format!("{name}.{check_point}"))
}

fn check_asic_status(config: &Config, db: &dyn StateDb, info: &mut CheckerInfo) -> Result<()> {
    if config.ignores_device("asic") {
        return Ok(());
    }
    for key in natsorted(db.keys(&format!("{ASIC_TEMPERATURE_KEY}*"))?) {
        let temperature = db.get(&key, "temperature")?.filter(|s| !s.is_empty());
        let threshold = db.get(&key, "high_threshold")?.filter(|s| !s.is_empty());
        let name = key.split('|').nth(1).unwrap_or_default().to_string();
        match (temperature, threshold) {
            (None, _) => info.set_object_not_ok("ASIC", &name, format!("Failed to get {name} temperature")),
            (_, None) => info.set_object_not_ok("ASIC", &name, format!("Failed to get {name} temperature threshold")),
            (Some(t), Some(th)) => match convert_all(&[&t, &th]) {
                (v, true) => {
                    if let (Num::Parsed(t), Num::Parsed(th)) = (&v[0], &v[1]) {
                        if t > th {
                            info.set_object_not_ok(
                                "ASIC",
                                &name,
                                format!(
                                    "{name} temperature is too hot, temperature={}, threshold={}",
                                    v[0], v[1]
                                ),
                            );
                        } else {
                            info.set_object_ok("ASIC", &name);
                        }
                    }
                }
                (v, false) => info.set_object_not_ok(
                    "ASIC",
                    &name,
                    format!(
                        "Invalid {name} temperature data, temperature={}, threshold={}",
                        v[0], v[1]
                    ),
                ),
            },
        }
    }
    Ok(())
}

fn check_fan_status(config: &Config, db: &dyn StateDb, info: &mut CheckerInfo) -> Result<()> {
    if config.ignores_device("fan") {
        return Ok(());
    }
    let keys = db.keys(&format!("{FAN_TABLE_NAME}*"))?;
    if keys.is_empty() {
        info.set_object_not_ok("Fan", "Fan", "Failed to get fan information");
        return Ok(());
    }
    let mut expect_direction: Option<(String, String)> = None;
    for key in natsorted(keys) {
        let parts: Vec<&str> = key.split('|').collect();
        if parts.len() != 2 {
            info.set_object_not_ok("Fan", &key, format!("Invalid key for {FAN_TABLE_NAME}: {key}"));
            continue;
        }
        let name = parts[1];
        if config.ignores_device(name) {
            continue;
        }
        let data = db.get_all(&key)?;
        if get(&data, "presence").unwrap_or("false").to_lowercase() != "true" {
            info.set_object_not_ok("Fan", name, format!("{name} is missing"));
            continue;
        }

        if !ignore_check(config, "fan", name, "speed") {
            let speed = get(&data, "speed").filter(|s| !s.is_empty());
            let target = get(&data, "speed_target").filter(|s| !s.is_empty());
            let under = get(&data, "is_under_speed");
            let over = get(&data, "is_over_speed");
            match (speed, target, under, over) {
                (None, ..) => {
                    info.set_object_not_ok("Fan", name, format!("Failed to get actual speed data for {name}"));
                    continue;
                }
                (_, None, ..) => {
                    // The Python message says "speed date", a typo fixed here.
                    info.set_object_not_ok("Fan", name, format!("Failed to get target speed data for {name}"));
                    continue;
                }
                (_, _, None, _) => {
                    info.set_object_not_ok(
                        "Fan",
                        name,
                        format!("Failed to get under speed threshold check for {name}"),
                    );
                    continue;
                }
                (_, _, _, None) => {
                    info.set_object_not_ok(
                        "Fan",
                        name,
                        format!("Failed to get over speed threshold check for {name}"),
                    );
                    continue;
                }
                (Some(speed), Some(target), Some(under), Some(over)) => match convert_all(&[speed, target]) {
                    (v, true) => {
                        if under.to_lowercase() == "true" || over.to_lowercase() == "true" {
                            info.set_object_not_ok(
                                "Fan",
                                name,
                                format!("{name} speed is out of range, speed={}, target={}", v[0], v[1]),
                            );
                            continue;
                        }
                    }
                    (v, false) => {
                        info.set_object_not_ok(
                            "Fan",
                            name,
                            format!(
                                "Invalid fan speed data for {name}, speed={}, target={}, is_under_speed={under}, is_over_speed={over}",
                                v[0], v[1]
                            ),
                        );
                        continue;
                    }
                },
            }
        }

        if !ignore_check(config, "fan", name, "direction") {
            let direction = get(&data, "direction").unwrap_or("N/A");
            if direction != "N/A" {
                match &expect_direction {
                    None => expect_direction = Some((name.to_string(), direction.to_string())),
                    Some((expect_name, expect)) if direction != expect => {
                        info.set_object_not_ok(
                            "Fan",
                            name,
                            format!(
                                "{name} direction {direction} is not aligned with {expect_name} direction {expect}"
                            ),
                        );
                        continue;
                    }
                    Some(_) => {}
                }
            }
        }

        if get(&data, "status").unwrap_or("false").to_lowercase() != "true" {
            info.set_object_not_ok("Fan", name, format!("{name} is broken"));
            continue;
        }
        info.set_object_ok("Fan", name);
    }
    Ok(())
}

fn check_psu_status(config: &Config, db: &dyn StateDb, info: &mut CheckerInfo) -> Result<()> {
    let ignore_psu = config.ignores_device("psu");
    let ignore_pdb = config.ignores_device("pdb");
    if ignore_psu && ignore_pdb {
        return Ok(());
    }
    let keys = db.keys(&format!("{PSU_TABLE_NAME}*"))?;
    if keys.is_empty() {
        if !ignore_psu {
            info.set_object_not_ok("PSU", "PSU", "Failed to get PSU information");
        }
        return Ok(());
    }
    for key in natsorted(keys) {
        let parts: Vec<&str> = key.split('|').collect();
        if parts.len() != 2 {
            info.set_object_not_ok("PSU", &key, format!("Invalid key for {PSU_TABLE_NAME}: {key}"));
            continue;
        }
        let name = parts[1];
        if config.ignores_device(name) {
            continue;
        }
        let is_pdb = name.to_uppercase().starts_with("PDB");
        if (is_pdb && ignore_pdb) || (!is_pdb && ignore_psu) {
            continue;
        }
        let data = db.get_all(&key)?;
        if get(&data, "presence").unwrap_or("false").to_lowercase() != "true" {
            info.set_object_not_ok("PSU", name, format!("{name} is missing or not available"));
            continue;
        }
        if get(&data, "status").unwrap_or("false").to_lowercase() != "true" {
            info.set_object_not_ok("PSU", name, format!("{name} is out of power"));
            continue;
        }

        if !ignore_check(config, "psu", name, "temperature") {
            match (get(&data, "temp"), get(&data, "temp_threshold")) {
                (None, _) => {
                    info.set_object_not_ok("PSU", name, format!("Failed to get temperature data for {name}"));
                    continue;
                }
                (_, None) => {
                    info.set_object_not_ok(
                        "PSU",
                        name,
                        format!("Failed to get temperature threshold data for {name}"),
                    );
                    continue;
                }
                (Some(temp), Some(threshold)) if threshold != "N/A" => match convert_all(&[temp, threshold]) {
                    (v, true) => {
                        if let (Num::Parsed(t), Num::Parsed(th)) = (&v[0], &v[1]) {
                            if t > th {
                                info.set_object_not_ok(
                                    "PSU",
                                    name,
                                    format!(
                                        "{name} temperature is too hot, temperature={}, threshold={}",
                                        v[0], v[1]
                                    ),
                                );
                                continue;
                            }
                        }
                    }
                    (v, false) => {
                        info.set_object_not_ok(
                            "PSU",
                            name,
                            format!(
                                "Invalid temperature data for {name}, temperature={}, threshold={}",
                                v[0], v[1]
                            ),
                        );
                        continue;
                    }
                },
                _ => {}
            }
        }

        if !ignore_check(config, "psu", name, "voltage") {
            match (
                get(&data, "voltage"),
                get(&data, "voltage_min_threshold"),
                get(&data, "voltage_max_threshold"),
            ) {
                (None, ..) => {
                    info.set_object_not_ok("PSU", name, format!("Failed to get voltage data for {name}"));
                    continue;
                }
                (_, None, _) => {
                    info.set_object_not_ok(
                        "PSU",
                        name,
                        format!("Failed to get voltage minimum threshold data for {name}"),
                    );
                    continue;
                }
                (_, _, None) => {
                    info.set_object_not_ok(
                        "PSU",
                        name,
                        format!("Failed to get voltage maximum threshold data for {name}"),
                    );
                    continue;
                }
                (Some(voltage), Some(min), Some(max)) if min != "N/A" && max != "N/A" => {
                    match convert_all(&[voltage, min, max]) {
                        (v, true) => {
                            if let (Num::Parsed(volt), Num::Parsed(lo), Num::Parsed(hi)) = (&v[0], &v[1], &v[2]) {
                                if volt < lo || volt > hi {
                                    info.set_object_not_ok(
                                        "PSU",
                                        name,
                                        format!(
                                            "{name} voltage is out of range, voltage={}, range=[{},{}]",
                                            v[0], v[1], v[2]
                                        ),
                                    );
                                    continue;
                                }
                            }
                        }
                        (v, false) => {
                            info.set_object_not_ok(
                                "PSU",
                                name,
                                format!(
                                    "Invalid voltage data for {name}, voltage={}, range=[{},{}]",
                                    v[0], v[1], v[2]
                                ),
                            );
                            continue;
                        }
                    }
                }
                _ => {}
            }
        }

        if !ignore_check(config, "psu", name, "power_threshold") && get(&data, "power_overload") == Some("True") {
            match (get(&data, "power"), get(&data, "power_critical_threshold")) {
                (Some(_), Some(critical)) => {
                    info.set_object_not_ok("PSU", name, format!("System power exceeds threshold ({critical}w)"))
                }
                _ => info.set_object_not_ok(
                    "PSU",
                    name,
                    "System power exceeds threshold but power_critical_threshold is invalid",
                ),
            }
            continue;
        }

        info.set_object_ok("PSU", name);
    }
    Ok(())
}
