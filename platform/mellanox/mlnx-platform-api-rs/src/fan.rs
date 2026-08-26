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

//! Fan reading for Mellanox/NVIDIA hw-management.
//!
//! Ports Python `fan.py`:
//!  - `get_speed()`:        `speed_rpm / max_rpm * 100`, capped at 100
//!  - `get_target_speed()`: `round(pwm * 100 / 255)`
//!  - `get_direction()`:    `fan{N}_dir`: 0→EXHAUST, 1→INTAKE
//!  - `get_status()`:       `fan{N}_fault == 0`
//!  - under/over speed:     50 % tolerance (drawer fans)

use platform_traits::{FanDirection, FanInfo, FanKind};

use crate::led::FanLeds;
use crate::utils;

const PWM_MAX: f64 = 255.0;
const DRAWER_SPEED_TOLERANCE: f64 = 0.50; // 50 % tolerance for drawer fans
/// PSU fans use a tighter band than drawer fans, and compare **RPM against the
/// hardware's own min/max files** rather than a percentage against a target
/// (`fan.py:215-224`, `:226-263`).  Two different comparisons, two different
/// tolerances; sharing either one moves the alarm point.
const PSU_SPEED_TOLERANCE: f64 = 0.30;

/// Read one drawer fan's state from sysfs.
///
/// `fan_abs` — absolute 1-based fan index (e.g. fan 3 of drawer 2 is index 3
///             on a 2-fans-per-drawer system).
/// `drawer_name` — e.g. `"drawer2"`.
/// `position_in_drawer` — 1-based fan position within this drawer.
/// `drawer_present` is the *drawer's* presence, not a per-fan reading: Python's
/// `Fan.get_presence()` delegates to `self.fan_drawer.get_presence()`
/// (`fan.py:383-390`), and the drawer reads `fan{drawer_index}_status`
/// (`fan_drawer.py:45`, `:55`).  Reading `fan{fan_abs}_status` here instead
/// would index a per-drawer file with an absolute fan number, so on a platform
/// with two fans per drawer the second fan of drawer 1 would report drawer 2's
/// presence — and on a platform without the file at all every fan would read as
/// absent.
#[allow(clippy::too_many_arguments)]
pub fn read_drawer_fan(
    thermal: &str,
    fan_abs: usize,
    drawer_name: &str,
    position_in_drawer: usize,
    drawer_present: bool,
    leds: &FanLeds,
) -> FanInfo {
    let speed_pct = read_speed_pct_in(thermal, fan_abs);
    let target_pct = read_target_speed_pct_in(thermal, fan_abs);

    let is_under_speed = speed_pct.zip(target_pct).map(|(s, t)| {
        (s as f64) < (t as f64) * (1.0 - DRAWER_SPEED_TOLERANCE)
    });
    let is_over_speed = speed_pct.zip(target_pct).map(|(s, t)| {
        (s as f64) > (t as f64) * (1.0 + DRAWER_SPEED_TOLERANCE)
    });

    let status = utils::read_int(&format!("{thermal}/fan{fan_abs}_fault"))
        .map(|v| v == 0)
        .unwrap_or(false);

    let presence = drawer_present;

    // The *drawer's* direction, not the fan's, and for the same reason as its
    // presence: `Fan.get_direction()` delegates to `self.fan_drawer` unless the
    // drawer is virtual (`fan.py:349-372`), and the drawer reads
    // `fan{drawer_index}_dir` (`fan_drawer.py:57-62`).  hw-management publishes
    // one such file per drawer, so on a platform with two fans per drawer the
    // second fan of every drawer would find no file and report `N/A` where
    // Python reports the drawer's airflow.  A virtual drawer has no index to
    // delegate to and falls back to the fan's own, which is the same number
    // there.
    //
    // An absent drawer reports no direction at all, which is what the drawer
    // returns before it reads anything.
    let dir_index = drawer_name
        .strip_prefix("drawer")
        .and_then(|n| n.parse::<usize>().ok())
        .unwrap_or(fan_abs);
    let direction = drawer_present
        .then(|| utils::read_int(&format!("{thermal}/fan{dir_index}_dir")))
        .flatten()
        .and_then(|v| match v {
            0 => Some(FanDirection::Exhaust),
            1 => Some(FanDirection::Intake),
            _ => None,
        });

    FanInfo {
        name: format!("fan{fan_abs}"),
        kind: FanKind::Drawer,
        drawer_name: drawer_name.to_string(),
        parent_name: drawer_name.to_string(),
        position_in_parent: position_in_drawer as u32,
        presence,
        status,
        speed_pct,
        target_speed_pct: target_pct,
        direction,
        is_under_speed,
        is_over_speed,
        // `MlnxFan.is_replaceable()` returns False unconditionally and neither
        // `Fan` nor `PsuFan` overrides it (`fan.py:126-132`), so *every*
        // Mellanox fan reports False -- including the ones in a drawer that is
        // itself replaceable.  The drawer is the field-replaceable unit; a fan
        // inside it is not pulled on its own.
        is_replaceable: false,
        model: None,
        serial: None,
        // `Fan.led` is a `ComponentFaultyIndicator` over the *drawer's* shared
        // LED (`fan.py:339`), and `get_status_led()` reads that LED back — so
        // every fan in a drawer reports the drawer's colour, not its own.
        status_led: Some(leds.status(&drawer_led_id(drawer_name))),
    }
}

/// Read one PSU fan's state from sysfs.
///
/// `psu_num` — 1-based PSU number.
/// `fan_in_psu` — always 1 for Mellanox (one fan per PSU).
pub fn read_psu_fan(
    thermal: &str,
    psu_num: usize,
    fan_in_psu: usize,
    leds: &FanLeds,
) -> FanInfo {
    // hw-management file layout: psuN_fan1_speed_get / psuN_fan_max / psuN_fan_dir
    let speed_file = format!("{thermal}/psu{psu_num}_fan1_speed_get");
    let max_file   = format!("{thermal}/psu{psu_num}_fan_max");

    let min_file   = format!("{thermal}/psu{psu_num}_fan_min");

    let speed_rpm = utils::read_int(&speed_file);
    let max_rpm   = utils::read_int(&max_file);
    let min_rpm   = utils::read_int(&min_file);

    let speed_pct = speed_rpm.zip(max_rpm).map(|(s, m)| speed_pct_from(s, m));

    let status = utils::read_int(&format!("{thermal}/psu{psu_num}_pwr_status")) == Some(1);

    // Python also requires the speed file to exist before calling the fan
    // present (`fan.py:206-212`); a powered PSU whose fan node is missing is
    // absent, not a fan reporting nothing.
    let presence = status && utils::exists(&speed_file);

    // `PsuFan` overrides all three of these (`fan.py:226-295`).  Leaving them
    // unset publishes `N/A` for a PSU fan's target speed and both speed checks,
    // and — because the checks feed the bad-fan counter and the LED — a PSU fan
    // running below its minimum RPM is never noticed at all.
    let under_min = |s: i64, m: i64| (s as f64) < (m as f64) * (1.0 - PSU_SPEED_TOLERANCE);
    let over_max = |s: i64, m: i64| (s as f64) > (m as f64) * (1.0 + PSU_SPEED_TOLERANCE);

    // Python's `except (ValueError, IOError): return False` — an unreadable
    // file is not a fault, it is an unknown, and it reports as healthy.
    let is_under_speed =
        Some(presence && speed_rpm.zip(min_rpm).is_some_and(|(s, m)| under_min(s, m)));
    let is_over_speed = Some(
        presence
            // A zero maximum has no band, so nothing is over it.
            && speed_rpm.zip(max_rpm).is_some_and(|(s, m)| m != 0 && over_max(s, m)),
    );

    // `get_target_speed`: the current RPM clamped into [min, max], as a
    // percentage of max.  A PSU fan has no commanded speed — hw-management
    // drives it — so the target it reports is where the hardware says it should
    // be sitting.  Any read failure falls back to the current speed, and an
    // absent fan reports 0 rather than nothing.
    let target_speed_pct = Some(if !presence {
        0
    } else {
        match (speed_rpm, max_rpm, min_rpm) {
            (Some(s), Some(max), Some(min)) if max != 0 => {
                // Python's three-way chain, not `clamp`: `Ord::clamp` panics
                // when `min > max`, and the two values come from separate sysfs
                // files, so a corrupt or inverted pair would abort the daemon
                // rather than publish a number.  Python has a defined answer
                // for that pair and this has to give the same one.
                let target = if s < min {
                    min
                } else if s > max {
                    max
                } else {
                    s
                };
                ((100 * target / max) as u32).min(100)
            }
            _ => speed_pct.unwrap_or(0),
        }
    });

    // PSU fan direction (0=EXHAUST, 1=INTAKE), same encoding as drawer fans.
    let direction = utils::read_int(&format!("{thermal}/psu{psu_num}_fan_dir"))
        .and_then(|v| match v {
            0 => Some(FanDirection::Exhaust),
            1 => Some(FanDirection::Intake),
            _ => None,
        });

    let psu_name = format!("PSU {psu_num}");

    FanInfo {
        name: format!("psu{psu_num}_fan{fan_in_psu}"),
        kind: FanKind::Psu,
        drawer_name: String::new(),
        parent_name: psu_name,
        position_in_parent: fan_in_psu as u32,
        presence,
        // `PsuFan.get_status()` returns True unconditionally (`fan.py:197-204`).
        // The field is only published while the fan is present, so this is the
        // same thing observed downstream.
        status: true,
        speed_pct,
        target_speed_pct,
        direction,
        is_under_speed,
        is_over_speed,
        is_replaceable: false, // Python: PSU fans not user-replaceable
        model: None,
        serial: None,
        // `PsuFan.led` is the PSU *shared* LED (`fan.py:161`), not a per-PSU
        // one, so every PSU fan reports the same colour.
        status_led: Some(leds.status("psu")),
    }
}

/// The drawer LED a fan reports through.
///
/// Derived from the drawer's name because that is the only handle this function
/// has on it: a real drawer is `drawer{N}` and owns `led_fan{N}_*`, and a
/// virtual one is named `N/A` and shares the unnumbered `led_fan_*`.
fn drawer_led_id(drawer_name: &str) -> String {
    match drawer_name.strip_prefix("drawer") {
        Some(num) => format!("fan{num}"),
        None => "fan".to_string(),
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

/// `rpm / max * 100`, capped at 100.
///
/// A maximum of zero returns the raw RPM rather than 0, which is what Python
/// does (`fan.py:77-78`) — the reading is unusable either way, but 0 would look
/// like a stopped fan and trip the under-speed check.  The division truncates,
/// as Python's `//` does: 6999 of 10000 is 69.
fn speed_pct_from(rpm: i64, max: i64) -> u32 {
    if max == 0 {
        return rpm.clamp(0, u32::MAX as i64) as u32;
    }
    ((rpm * 100) / max).min(100) as u32
}

/// `fan{N}_speed_get / fan{N}_max * 100`, capped at 100.
fn read_speed_pct_in(thermal: &str, fan_abs: usize) -> Option<u32> {
    let rpm = utils::read_int(&format!("{thermal}/fan{fan_abs}_speed_get"))?;
    let max = utils::read_int(&format!("{thermal}/fan{fan_abs}_max"))?;
    Some(speed_pct_from(rpm, max))
}

/// `round(fan{N}_speed_set / 255 * 100)`.
fn read_target_speed_pct_in(thermal: &str, fan_abs: usize) -> Option<u32> {
    let pwm = utils::read_int(&format!("{thermal}/fan{fan_abs}_speed_set"))? as f64;
    Some((pwm * 100.0 / PWM_MAX).round() as u32)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn speed_pct_caps_at_100() {
        assert_eq!(speed_pct_from(12000, 10000), 100);
    }

    /// Python returns the raw RPM when the maximum reads zero; 0 would look
    /// like a stopped fan to the under-speed check.
    #[test]
    fn a_zero_maximum_yields_the_raw_rpm() {
        assert_eq!(speed_pct_from(4200, 0), 4200);
    }

    /// Integer division truncates in both implementations.
    #[test]
    fn the_percentage_truncates_like_pythons_floor_division() {
        assert_eq!(speed_pct_from(6999, 10000), 69);
    }

    #[test]
    fn target_speed_rounds_correctly() {
        // PWM 128 / 255 * 100 ≈ 50.2 → rounds to 50.
        let pwm = 128.0_f64;
        let pct = (pwm * 100.0 / PWM_MAX).round() as u32;
        assert_eq!(pct, 50);
    }

    /// A drawer fan reports its *drawer's* LED: `Fan.led` wraps the drawer's
    /// shared one (`fan.py:339`).  A real drawer owns `led_fan{N}_*`; a virtual
    /// one is named `N/A` and shares the unnumbered `led_fan_*`, so the id has
    /// to come from the name rather than from the fan's own index.
    #[test]
    fn a_fan_reports_through_its_drawers_led_id() {
        assert_eq!(drawer_led_id("drawer3"), "fan3");
        assert_eq!(drawer_led_id("N/A"), "fan");
    }

    /// The colour published for a fan is the one its drawer's LED is showing.
    #[test]
    fn a_drawer_fan_publishes_its_drawers_colour() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("led_fan1_capability"), "green red\n").unwrap();
        std::fs::write(dir.path().join("led_fan1_green"), "0\n").unwrap();
        std::fs::write(dir.path().join("led_fan1_red"), "255\n").unwrap();
        let leds = FanLeds::with_path(dir.path());

        let fan =
            read_drawer_fan(&dir.path().to_string_lossy(), 1, "drawer1", 1, true, &leds);
        assert_eq!(fan.status_led.as_deref(), Some("red"));
    }

    /// Every PSU fan reports the one shared PSU LED (`fan.py:161`), never a
    /// per-PSU one, so two PSU fans always agree.
    #[test]
    fn psu_fans_all_report_the_one_shared_led() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("led_psu_capability"), "green red\n").unwrap();
        std::fs::write(dir.path().join("led_psu_green"), "255\n").unwrap();
        std::fs::write(dir.path().join("led_psu_red"), "0\n").unwrap();
        // A per-PSU file that must be ignored.
        std::fs::write(dir.path().join("led_psu2_capability"), "red\n").unwrap();
        std::fs::write(dir.path().join("led_psu2_red"), "255\n").unwrap();
        let leds = FanLeds::with_path(dir.path());

        let root = dir.path().to_string_lossy();
        assert_eq!(read_psu_fan(&root, 1, 1, &leds).status_led.as_deref(), Some("green"));
        assert_eq!(read_psu_fan(&root, 2, 1, &leds).status_led.as_deref(), Some("green"));
    }

    // ── A fan read end to end from a hw-management tree ───────────────────

    fn tree(files: &[(&str, &str)]) -> tempfile::TempDir {
        let d = tempfile::tempdir().unwrap();
        for (n, v) in files {
            std::fs::write(d.path().join(n), v).unwrap();
        }
        d
    }

    fn no_leds() -> (tempfile::TempDir, FanLeds) {
        let d = tempfile::tempdir().unwrap();
        let leds = FanLeds::with_path(d.path());
        (d, leds)
    }

    /// Everything a healthy drawer fan reports, by value.  The helpers above
    /// pin the arithmetic; this pins which file each field comes out of, which
    /// is the part that goes wrong when hw-management renames one.
    #[test]
    fn a_healthy_drawer_fan_reports_every_field_by_value() {
        let t = tree(&[
            ("fan3_speed_get", "5000\n"),
            ("fan3_max", "10000\n"),
            ("fan3_speed_set", "128\n"),
            ("fan3_fault", "0\n"),
            // Direction is the *drawer's*: fan3 is in drawer2, so drawer 2's
            // file is the one read.
            ("fan2_dir", "1\n"),
        ]);
        let (_l, leds) = no_leds();
        let f = read_drawer_fan(&t.path().to_string_lossy(), 3, "drawer2", 1, true, &leds);

        assert_eq!(f.name, "fan3");
        assert_eq!(f.kind, FanKind::Drawer);
        assert_eq!(f.drawer_name, "drawer2");
        assert_eq!(f.parent_name, "drawer2", "a drawer fan's parent is its drawer");
        assert_eq!(f.position_in_parent, 1);
        assert!(f.presence, "taken from the drawer, not from a file of its own");
        assert!(f.status, "fault 0 means healthy");
        assert_eq!(f.speed_pct, Some(50), "5000 of 10000");
        assert_eq!(f.target_speed_pct, Some(50), "PWM 128 of 255");
        assert_eq!(f.direction, Some(FanDirection::Intake), "1 is intake, from drawer2");
        assert_eq!(f.is_under_speed, Some(false));
        assert_eq!(f.is_over_speed, Some(false));
        assert!(
            !f.is_replaceable,
            "no Mellanox fan is replaceable on its own; the drawer is the unit"
        );
        assert_eq!(f.model, None, "FanBase.get_model is not overridden");
        assert_eq!(f.serial, None);
    }

    /// `fan{N}_fault` is inverted: 1 is a fault, 0 is healthy.  Reading it as a
    /// health bit reports every working fan as broken.
    #[test]
    fn the_fault_file_is_inverted() {
        let (_l, leds) = no_leds();
        let bad = tree(&[("fan1_fault", "1\n")]);
        assert!(!read_drawer_fan(&bad.path().to_string_lossy(), 1, "drawer1", 1, true, &leds).status);

        let good = tree(&[("fan1_fault", "0\n")]);
        assert!(read_drawer_fan(&good.path().to_string_lossy(), 1, "drawer1", 1, true, &leds).status);

        let missing = tree(&[]);
        assert!(
            !read_drawer_fan(&missing.path().to_string_lossy(), 1, "drawer1", 1, true, &leds).status,
            "an unreadable fault file reads as faulty, not as healthy"
        );
    }

    /// The direction encoding: 0 exhaust, 1 intake, anything else unknown.  A
    /// third value is `FAN_DIRECTION_NOT_APPLICABLE` in Python and must not be
    /// forced into one of the two.
    #[test]
    fn the_direction_encoding_has_a_third_state() {
        let (_l, leds) = no_leds();
        for (v, want) in [
            ("0", Some(FanDirection::Exhaust)),
            ("1", Some(FanDirection::Intake)),
            ("2", None),
        ] {
            let t = tree(&[("fan1_dir", v)]);
            let f = read_drawer_fan(&t.path().to_string_lossy(), 1, "drawer1", 1, true, &leds);
            assert_eq!(f.direction, want, "fan1_dir = {v}");
        }
    }

    /// Both speed checks need both readings; either one missing leaves them
    /// unknown rather than assuming the fan is fine.
    #[test]
    fn the_speed_checks_need_both_readings() {
        let (_l, leds) = no_leds();
        let t = tree(&[("fan1_speed_get", "5000"), ("fan1_max", "10000")]);
        let f = read_drawer_fan(&t.path().to_string_lossy(), 1, "drawer1", 1, true, &leds);
        assert_eq!(f.speed_pct, Some(50));
        assert_eq!(f.target_speed_pct, None, "no speed_set file");
        assert_eq!(f.is_under_speed, None);
        assert_eq!(f.is_over_speed, None);
    }

    /// The tolerance band: a fan far enough below its target is under speed,
    /// far enough above is over speed, and inside the band is neither.
    #[test]
    fn the_speed_band_is_a_tolerance_around_the_target() {
        let (_l, leds) = no_leds();
        // target PWM 255 -> 100%.
        let at = |rpm: &str| {
            let t = tree(&[
                ("fan1_speed_get", rpm),
                ("fan1_max", "10000"),
                ("fan1_speed_set", "255"),
            ]);
            read_drawer_fan(&t.path().to_string_lossy(), 1, "drawer1", 1, true, &leds)
        };
        // The band is +/-50% of the target, and the comparison is strict: 50%
        // of a 100% target sits exactly on the edge and is not under it.
        assert_eq!(at("4900").is_under_speed, Some(true), "49% of a 100% target");
        assert_eq!(at("5000").is_under_speed, Some(false), "exactly on the edge");
        assert_eq!(at("10000").is_under_speed, Some(false));
        assert_eq!(at("10000").is_over_speed, Some(false));
    }

    /// Everything a healthy PSU fan reports, by value.
    #[test]
    fn a_psu_fan_reports_every_field_by_value() {
        let t = tree(&[
            ("psu2_fan1_speed_get", "6000\n"),
            ("psu2_fan_max", "12000\n"),
            ("psu2_fan_min", "3000\n"),
            ("psu2_pwr_status", "1\n"),
            ("psu2_fan_dir", "0\n"),
        ]);
        let (_l, leds) = no_leds();
        let f = read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds);

        assert_eq!(f.name, "psu2_fan1");
        assert_eq!(f.kind, FanKind::Psu);
        assert_eq!(f.drawer_name, "", "a PSU fan is in no drawer");
        assert_eq!(f.parent_name, "PSU 2");
        assert_eq!(f.position_in_parent, 1);
        assert!(f.presence);
        assert!(f.status, "PsuFan.get_status() is unconditionally true");
        assert_eq!(f.speed_pct, Some(50), "6000 of 12000");
        assert_eq!(f.direction, Some(FanDirection::Exhaust));
        assert_eq!(f.target_speed_pct, Some(50), "6000 sits inside [3000, 12000]");
        assert_eq!(f.is_under_speed, Some(false));
        assert_eq!(f.is_over_speed, Some(false));
        assert!(!f.is_replaceable);
    }

    /// An inverted min/max pair must publish a number, not abort the process.
    ///
    /// `psuN_fan_min` and `psuN_fan_max` are separate sysfs files, so nothing
    /// stops them arriving the wrong way round.  `Ord::clamp` panics on that
    /// pair, and this daemon builds with `panic = "abort"`, so the whole
    /// process would go down over one bad file.  Python's three-way chain has a
    /// defined answer -- a speed below `min` reports `min`, whatever `max` says
    /// -- and this has to give the same one.
    #[test]
    fn an_inverted_speed_range_reports_a_number_rather_than_aborting() {
        let t = tree(&[
            ("psu1_fan1_speed_get", "1000\n"),
            ("psu1_fan_max", "3000\n"),
            ("psu1_fan_min", "9000\n"), // min > max
            ("psu1_pwr_status", "1\n"),
        ]);
        let (_l, leds) = no_leds();
        let f = read_psu_fan(&t.path().to_string_lossy(), 1, 1, &leds);

        // 1000 < min, so the target is min (9000); 100 * 9000 / 3000 = 300,
        // capped at 100.  Python arrives at the same 100.
        assert_eq!(f.target_speed_pct, Some(100));
        assert_eq!(f.speed_pct, Some(33), "1000 of 3000, truncated");
    }

    /// A PSU fan is present only when the PSU is powered *and* the speed node
    /// exists: a powered PSU with no fan node is absent, not a fan reporting
    /// nothing.
    #[test]
    fn a_psu_fan_needs_both_power_and_a_speed_node_to_be_present() {
        let (_l, leds) = no_leds();
        let unpowered = tree(&[("psu2_pwr_status", "0\n"), ("psu2_fan1_speed_get", "6000")]);
        assert!(!read_psu_fan(&unpowered.path().to_string_lossy(), 2, 1, &leds).presence);

        let no_node = tree(&[("psu2_pwr_status", "1\n")]);
        assert!(!read_psu_fan(&no_node.path().to_string_lossy(), 2, 1, &leds).presence);

        let both = tree(&[("psu2_pwr_status", "1\n"), ("psu2_fan1_speed_get", "6000")]);
        assert!(read_psu_fan(&both.path().to_string_lossy(), 2, 1, &leds).presence);
    }

    /// An absent PSU fan reports 0 for its target and false for both checks,
    /// which is what `PsuFan` returns before it reads anything.  `None` here
    /// would publish `N/A` where Python publishes a value.
    #[test]
    fn an_absent_psu_fan_reports_zero_and_no_alarm() {
        let t = tree(&[("psu2_pwr_status", "0\n")]);
        let (_l, leds) = no_leds();
        let f = read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds);
        assert!(!f.presence);
        assert_eq!(f.target_speed_pct, Some(0));
        assert_eq!(f.is_under_speed, Some(false));
        assert_eq!(f.is_over_speed, Some(false));
    }

    /// The PSU band is RPM against the hardware's own min/max files with a 30%
    /// tolerance — not a percentage against a target, and not the drawer fans'
    /// 50%.  This is the check that makes a failing PSU fan visible at all.
    #[test]
    fn a_psu_fan_below_its_minimum_rpm_is_under_speed() {
        let (_l, leds) = no_leds();
        let at = |rpm: &str| {
            let t = tree(&[
                ("psu2_pwr_status", "1"),
                ("psu2_fan1_speed_get", rpm),
                ("psu2_fan_min", "3000"),
                ("psu2_fan_max", "12000"),
            ]);
            read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds)
        };
        // 30% below 3000 is 2100; the comparison is strict.
        assert_eq!(at("2000").is_under_speed, Some(true));
        assert_eq!(at("2100").is_under_speed, Some(false), "exactly on the edge");
        assert_eq!(at("3000").is_under_speed, Some(false));

        // 30% above 12000 is 15600.
        assert_eq!(at("16000").is_over_speed, Some(true));
        assert_eq!(at("15600").is_over_speed, Some(false), "exactly on the edge");
    }

    /// A maximum of zero has no band, so nothing is over it — dividing by it
    /// would be worse than reporting healthy.
    #[test]
    fn a_zero_maximum_rpm_cannot_be_exceeded() {
        let t = tree(&[
            ("psu2_pwr_status", "1"),
            ("psu2_fan1_speed_get", "6000"),
            ("psu2_fan_max", "0"),
        ]);
        let (_l, leds) = no_leds();
        let f = read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds);
        assert_eq!(f.is_over_speed, Some(false));
    }

    /// An unreadable min or max is an unknown, not a fault: Python catches the
    /// read error and returns False, so a missing file must not raise an alarm.
    #[test]
    fn an_unreadable_threshold_does_not_raise_an_alarm() {
        let t = tree(&[("psu2_pwr_status", "1"), ("psu2_fan1_speed_get", "6000")]);
        let (_l, leds) = no_leds();
        let f = read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds);
        assert_eq!(f.is_under_speed, Some(false));
        assert_eq!(f.is_over_speed, Some(false));
        assert_eq!(f.target_speed_pct, Some(0), "falls back to the current speed");
    }

    /// The target is the current RPM clamped into the hardware's band, so a fan
    /// spinning below its minimum still reports the minimum as where it should
    /// be — not its own speed.
    #[test]
    fn the_psu_target_speed_is_the_clamped_current_speed() {
        let (_l, leds) = no_leds();
        let at = |rpm: &str| {
            let t = tree(&[
                ("psu2_pwr_status", "1"),
                ("psu2_fan1_speed_get", rpm),
                ("psu2_fan_min", "3000"),
                ("psu2_fan_max", "12000"),
            ]);
            read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds).target_speed_pct
        };
        assert_eq!(at("1000"), Some(25), "clamped up to the 3000 minimum");
        assert_eq!(at("6000"), Some(50), "inside the band, so itself");
        assert_eq!(at("20000"), Some(100), "clamped down to the maximum");
    }

    /// A PSU fan's direction uses the same three-state encoding as a drawer
    /// fan's, read from its own file.
    #[test]
    fn a_psu_fans_direction_has_the_same_three_states() {
        let (_l, leds) = no_leds();
        for (v, want) in [
            ("0", Some(FanDirection::Exhaust)),
            ("1", Some(FanDirection::Intake)),
            ("2", None),
        ] {
            let t = tree(&[("psu2_pwr_status", "1"), ("psu2_fan1_speed_get", "6000"), ("psu2_fan_dir", v)]);
            let f = read_psu_fan(&t.path().to_string_lossy(), 2, 1, &leds);
            assert_eq!(f.direction, want, "psu2_fan_dir = {v}");
        }
    }

    /// Neither a drawer fan nor a PSU fan is replaceable on its own: Python's
    /// `MlnxFan.is_replaceable()` returns False and no subclass overrides it.
    /// The *drawer* is replaceable, and reporting the fan as replaceable too
    /// would offer an operator a part that cannot be pulled.
    #[test]
    fn no_fan_is_replaceable_on_its_own() {
        let (_l, leds) = no_leds();
        let t = tree(&[("fan1_speed_get", "5000"), ("psu2_pwr_status", "1"), ("psu2_fan1_speed_get", "6000")]);
        let root = t.path().to_string_lossy();
        assert!(!read_drawer_fan(&root, 1, "drawer1", 1, true, &leds).is_replaceable);
        assert!(!read_psu_fan(&root, 2, 1, &leds).is_replaceable);
    }

    /// A fan reports its *drawer's* airflow, from the drawer's file.
    /// hw-management publishes one `fan{n}_dir` per drawer, so on a platform
    /// with two fans per drawer the second fan of each would find no file and
    /// report `N/A` where Python reports the drawer's direction -- which is
    /// what an SN5640 does: five `fan*_dir` files against ten fans.
    #[test]
    fn a_fan_reports_its_drawers_airflow_not_a_file_of_its_own() {
        let (_l, leds) = no_leds();
        // Five drawers of two fans; only the drawers have a direction file.
        let t = tree(&[("fan3_dir", "0"), ("fan4_dir", "1")]);
        let root = t.path().to_string_lossy();

        // Fans 5 and 6 are drawer 3's; both take drawer 3's file.
        for fan_abs in [5, 6] {
            let f = read_drawer_fan(&root, fan_abs, "drawer3", 1, true, &leds);
            assert_eq!(
                f.direction,
                Some(FanDirection::Exhaust),
                "fan{fan_abs} must read fan3_dir"
            );
        }
        // And drawer 4's fans take drawer 4's.
        let f = read_drawer_fan(&root, 8, "drawer4", 2, true, &leds);
        assert_eq!(f.direction, Some(FanDirection::Intake));
    }

    /// A virtual drawer has no index to delegate to, so its fan reads its own.
    #[test]
    fn a_fan_in_a_virtual_drawer_reads_its_own_direction_file() {
        let (_l, leds) = no_leds();
        let t = tree(&[("fan2_dir", "1")]);
        let f = read_drawer_fan(&t.path().to_string_lossy(), 2, "N/A", 1, true, &leds);
        assert_eq!(f.direction, Some(FanDirection::Intake));
    }

    /// An absent drawer reports no direction: the drawer returns
    /// NOT_APPLICABLE before it reads anything.
    #[test]
    fn an_absent_drawer_reports_no_direction() {
        let (_l, leds) = no_leds();
        let t = tree(&[("fan1_dir", "0")]);
        let f = read_drawer_fan(&t.path().to_string_lossy(), 1, "drawer1", 1, false, &leds);
        assert_eq!(f.direction, None);
    }
}
