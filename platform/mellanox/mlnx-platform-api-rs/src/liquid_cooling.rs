//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//! Leak sensors.  Ports `liquid_cooling.py`.
//!
//! Two things here look wrong and are deliberate, because the daemon's job is
//! to behave as the Python one does:
//!
//! * **The polarity is inverted.**  `0` in a `leakage*` file means *leaking* and
//!   `1` means dry — the opposite of every other status file in the tree.
//! * **An unreadable sensor reads as leaking.**  Python returns the string
//!   `'N/A'` in that case, and `if sensor_is_leak:` treats a non-empty string as
//!   true, so the sensor enters the leaking state machine and its row says
//!   `leaking = Yes`.  Reporting it as a sensor fault instead would be the
//!   better behaviour but is not what runs today.
//!
//! Mellanox also inherits `is_leak_sensor_ok`, `get_leak_severity` and
//! `get_leak_profile` from the base class rather than overriding them, so on
//! this platform a sensor is always "ok", its severity is always `CRITICAL`,
//! and there are no profiles.  The daemon-side logic for the other cases is
//! still there for a vendor that does override them.

use platform_traits::{LeakProfile, LeakSensorInfo, LeakSeverity};

use crate::utils;

/// Python's `LIQUID_COOLING_SENSOR_PATH`.
const SENSOR_DIR: &str = "/var/run/hw-management/system";
/// How many of the `leakage*` files are real sensors.
const COUNTER_FILE: &str = "/var/run/hw-management/config/leakage_counter";

/// Base-class defaults that `liquid_cooling.py` does not override.
const DEFAULT_TYPE: &str = "unknown";
const DEFAULT_LOCATION: &str = "unknown";
const DEFAULT_SEVERITY: LeakSeverity = LeakSeverity::Critical;

/// Number of leak sensors, or zero on an air-cooled platform.
pub fn sensor_count() -> usize {
    utils::read_int(COUNTER_FILE).unwrap_or(0).max(0) as usize
}

/// Mellanox publishes no profiles, so `LEAK_PROFILE` stays empty.
pub fn profiles() -> Vec<LeakProfile> {
    Vec::new()
}

/// Read every sensor, in the numeric order of the `leakage*` suffix.
pub fn sensors() -> Vec<LeakSensorInfo> {
    read_sensors(SENSOR_DIR, sensor_count())
}

fn read_sensors(dir: &str, count: usize) -> Vec<LeakSensorInfo> {
    let mut files: Vec<(u32, String)> = Vec::new();
    let Ok(entries) = std::fs::read_dir(dir) else {
        return Vec::new();
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().into_owned();
        let Some(suffix) = name.strip_prefix("leakage") else {
            continue;
        };
        // Python sorts by int(suffix); a non-numeric suffix would raise there,
        // so anything that is not a number is not a sensor.
        let Ok(index) = suffix.parse::<u32>() else {
            continue;
        };
        files.push((index, name));
    }
    files.sort_by_key(|(i, _)| *i);
    files.truncate(count);

    files
        .into_iter()
        .map(|(_, name)| {
            let path = format!("{dir}/{name}");
            LeakSensorInfo {
                is_leak: is_leak(&path),
                name,
                // Never overridden on Mellanox, so always true.
                is_ok: true,
                severity: Some(DEFAULT_SEVERITY),
                profile_type: None,
                sensor_type: DEFAULT_TYPE.to_string(),
                location: DEFAULT_LOCATION.to_string(),
            }
        })
        .collect()
}

/// `0` is leaking, `1` is dry, and anything else — including an unreadable
/// file — counts as leaking, because Python's `'N/A'` return value is truthy at
/// the call site.
fn is_leak(path: &str) -> bool {
    match utils::read_int(path) {
        Some(1) => false,
        Some(0) => true,
        other => {
            log::error!("Failed to read leakage sensor {path} value: {other:?}");
            true
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn tree(files: &[(&str, &str)]) -> tempfile::TempDir {
        let d = tempfile::tempdir().unwrap();
        for (name, content) in files {
            fs::write(d.path().join(name), content).unwrap();
        }
        d
    }

    fn names(s: &[LeakSensorInfo]) -> Vec<&str> {
        s.iter().map(|x| x.name.as_str()).collect()
    }

    #[test]
    fn zero_is_leaking_and_one_is_dry() {
        let d = tree(&[("leakage1", "0\n"), ("leakage2", "1\n")]);
        let s = read_sensors(d.path().to_str().unwrap(), 2);
        assert!(s[0].is_leak, "0 must mean leaking");
        assert!(!s[1].is_leak, "1 must mean dry");
    }

    /// Deliberately copied from Python: an unreadable sensor reads as leaking,
    /// not as a sensor fault.
    #[test]
    fn an_unreadable_sensor_reads_as_leaking() {
        let d = tree(&[("leakage1", "garbage\n")]);
        let s = read_sensors(d.path().to_str().unwrap(), 1);
        assert!(s[0].is_leak);
        assert!(s[0].is_ok, "and it is still reported as a working sensor");
    }

    #[test]
    fn sensors_are_ordered_numerically_not_lexically() {
        let d = tree(&[("leakage1", "1\n"), ("leakage2", "1\n"), ("leakage10", "1\n")]);
        let s = read_sensors(d.path().to_str().unwrap(), 3);
        assert_eq!(names(&s), ["leakage1", "leakage2", "leakage10"]);
    }

    #[test]
    fn only_the_counted_sensors_are_used() {
        let d = tree(&[("leakage1", "1\n"), ("leakage2", "1\n"), ("leakage3", "1\n")]);
        let s = read_sensors(d.path().to_str().unwrap(), 2);
        assert_eq!(names(&s), ["leakage1", "leakage2"]);
    }

    #[test]
    fn unrelated_files_are_ignored() {
        let d = tree(&[("leakage1", "1\n"), ("leakage_counter", "9\n"), ("other", "x")]);
        let s = read_sensors(d.path().to_str().unwrap(), 5);
        assert_eq!(names(&s), ["leakage1"]);
    }

    #[test]
    fn an_air_cooled_platform_has_no_sensors() {
        let d = tree(&[]);
        assert!(read_sensors(d.path().to_str().unwrap(), 0).is_empty());
    }

    #[test]
    fn mellanox_defaults_match_the_base_class() {
        let d = tree(&[("leakage1", "1\n")]);
        let s = read_sensors(d.path().to_str().unwrap(), 1);
        assert!(s[0].is_ok);
        assert_eq!(s[0].severity, Some(LeakSeverity::Critical));
        assert_eq!(s[0].sensor_type, "unknown");
        assert_eq!(s[0].location, "unknown");
        assert_eq!(s[0].profile_type, None);
        assert!(profiles().is_empty());
    }
}
