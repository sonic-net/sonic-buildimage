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

//! hw-management status LEDs.  Ports the parts of `led.py` the fan path uses.
//!
//! Two things here are easy to get wrong, and both are visible on the switch:
//!
//! * **Clearing is not one write.**  Turning a colour on writes `255` to that
//!   colour's file, but turning the LED off writes `0` to *every* colour the
//!   `_capability` file reports.  Writing only the colour you last set leaves
//!   another one lit.
//! * **A drawer's LED is shared by its fans.**  Each fan holds a colour and the
//!   physical LED shows the highest-priority one — red before green — so a
//!   single failed fan turns the whole drawer red.  Driving the LED per fan
//!   loses that: the last fan to be written wins instead.

use std::collections::{BTreeMap, BTreeSet};
use std::path::PathBuf;

use crate::utils;

/// Python's `Led.LED_PATH`.
pub const DEFAULT_LED_PATH: &str = "/var/run/hw-management/led";

/// Python's `Led.LED_ON` / `Led.LED_OFF`.
const LED_ON: &str = "255";
const LED_OFF: &str = "0";

pub const COLOR_OFF: &str = "off";
pub const COLOR_RED: &str = "red";
pub const COLOR_GREEN: &str = "green";

/// Python's `Led.SIMILAR_COLORS`: what to fall back to when the requested
/// colour is not in the capability list.
fn similar_colors(color: &str) -> &'static [&'static str] {
    match color {
        "red" => &["amber", "orange"],
        "amber" => &["red", "orange"],
        "orange" => &["red", "amber"],
        _ => &[],
    }
}

/// Python's `SharedLed.LED_PRIORITY`.  Lower wins, and an unknown colour must
/// not silently outrank red, so it sorts last.
fn priority(color: &str) -> u8 {
    match color {
        COLOR_RED => 0,
        COLOR_GREEN => 1,
        _ => 2,
    }
}

/// Python's `Led.PRIMARY_COLORS`.  A read-back never reports amber or orange:
/// those collapse to red so that a caller only ever sees "good" or "bad".
/// A colour outside the table is returned unchanged, which is what
/// `dict.get(color, color)` does.
fn primary_color(color: &str) -> &str {
    match color {
        "red" | "amber" | "orange" => COLOR_RED,
        "green" => COLOR_GREEN,
        "blue" => "blue",
        other => other,
    }
}

/// The LED tree, and the colour each fan is currently asking its drawer to show.
#[derive(Debug)]
pub struct FanLeds {
    path: PathBuf,
    /// drawer -> (fan -> colour).  Ordered so the aggregate is deterministic.
    wanted: BTreeMap<String, BTreeMap<String, String>>,
}

impl Default for FanLeds {
    fn default() -> Self {
        Self::new()
    }
}

impl FanLeds {
    pub fn new() -> Self {
        Self { path: PathBuf::from(DEFAULT_LED_PATH), wanted: BTreeMap::new() }
    }

    /// A tree rooted anywhere — for tests.  `#[cfg(test)]` reaches every
    /// module's tests in this crate, including `psu.rs`.
    #[cfg(test)]
    pub fn with_path<P: AsRef<std::path::Path>>(path: P) -> Self {
        Self { path: path.as_ref().to_path_buf(), wanted: BTreeMap::new() }
    }

    /// Record what one fan wants and drive its drawer's LED to the aggregate.
    ///
    /// `led_id` is the drawer's hw-management LED id, e.g. `fan1`.
    pub fn set_fan_color(&mut self, led_id: &str, drawer: &str, fan: &str, color: &str) -> bool {
        self.wanted
            .entry(drawer.to_string())
            .or_default()
            .insert(fan.to_string(), color.to_string());
        let target = self.drawer_color(drawer);
        self.set_status(led_id, &target)
    }

    /// The colour a drawer's LED should show: the highest-priority colour any
    /// of its fans is asking for, green when it has none.
    pub fn drawer_color(&self, drawer: &str) -> String {
        self.wanted
            .get(drawer)
            .and_then(|fans| fans.values().min_by_key(|c| priority(c)).cloned())
            .unwrap_or_else(|| COLOR_GREEN.to_string())
    }

    /// Colours this LED can show, from `led_{id}_capability`.
    ///
    /// `none` is skipped and a `_blink` suffix is stripped, as Python does.
    pub fn capability(&self, led_id: &str) -> BTreeSet<String> {
        let path = self.path.join(format!("led_{led_id}_capability"));
        let mut out = BTreeSet::new();
        let Some(caps) = utils::read_string(&path.to_string_lossy()) else {
            return out;
        };
        for cap in caps.split_whitespace() {
            if cap == "none" {
                continue;
            }
            match cap.find("_blink") {
                // Blink is not used on the fan path, so only the steady colour
                // is recorded.
                Some(pos) => out.insert(cap[..pos].to_string()),
                None => out.insert(cap.to_string()),
            };
        }
        out
    }

    /// Drive one LED.  `off` clears every supported colour; any other colour
    /// lights the nearest supported match.
    ///
    /// Returns false when the LED has no capability entry, which is what a
    /// simulated platform looks like.
    pub fn set_status(&self, led_id: &str, color: &str) -> bool {
        let caps = self.capability(led_id);
        if caps.is_empty() {
            return false;
        }
        if color == COLOR_OFF {
            for c in &caps {
                utils::write_sysfs_log(&self.led_file(led_id, c), LED_OFF);
            }
            return true;
        }
        let Some(actual) = self.actual_color(&caps, color) else {
            log::error!("Set LED {led_id} to color {color} is not supported");
            return false;
        };
        utils::write_sysfs_log(&self.led_file(led_id, &actual), LED_ON);
        true
    }

    /// Read one LED back, as Python's `Led.get_status()` does.
    ///
    /// Three details are load-bearing and none of them are obvious:
    ///
    /// * The two reads use **different** defaults.  A colour file is read with
    ///   Python's `read_str_from_file` default of `""`, and `"" != "0"`, so an
    ///   LED whose capability lists a colour but whose file is missing reads as
    ///   *lit*.  A blink file is read with an explicit default of `"0"`, so a
    ///   missing one reads as *not blinking*.  Normalising both to "absent
    ///   means off" would report `off` where Python reports a colour.
    /// * A blinking LED returns `"{color}_blink"` with no primary-colour
    ///   mapping, so `orange_blink` stays orange while a steady orange becomes
    ///   `red`.
    /// * Python iterates `self.supported_colors`, an unordered `set`, so with
    ///   two colours lit at once its answer is not deterministic.  Ordering the
    ///   capability set makes this one reproducible; the case cannot arise from
    ///   `set_status`, which lights exactly one colour.
    pub fn status(&self, led_id: &str) -> String {
        let caps = self.capability(led_id);
        if caps.is_empty() {
            // Python logs an error here unless the platform is simulated, then
            // returns "off" regardless.
            return COLOR_OFF.to_string();
        }

        if let Some(c) = caps.iter().find(|c| self.is_blinking(led_id, c)) {
            return format!("{c}_blink");
        }

        for c in &caps {
            if utils::read_string(&self.led_file(led_id, c)).unwrap_or_default() != LED_OFF {
                return primary_color(c).to_string();
            }
        }
        COLOR_OFF.to_string()
    }

    /// Python's `_is_led_blinking`: both delay files present and non-zero.
    fn is_blinking(&self, led_id: &str, color: &str) -> bool {
        let read = |suffix: &str| {
            utils::read_string(&self.delay_file(led_id, color, suffix))
                .unwrap_or_else(|| LED_OFF.to_string())
        };
        read("on") != LED_OFF && read("off") != LED_OFF
    }

    fn delay_file(&self, led_id: &str, color: &str, suffix: &str) -> String {
        self.path
            .join(format!("led_{led_id}_{color}_delay_{suffix}"))
            .to_string_lossy()
            .into_owned()
    }

    fn actual_color(&self, caps: &BTreeSet<String>, color: &str) -> Option<String> {
        if caps.contains(color) {
            return Some(color.to_string());
        }
        similar_colors(color)
            .iter()
            .find(|c| caps.contains(**c))
            .map(|c| c.to_string())
    }

    fn led_file(&self, led_id: &str, color: &str) -> String {
        self.path.join(format!("led_{led_id}_{color}")).to_string_lossy().into_owned()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn tree(caps: &str) -> (tempfile::TempDir, FanLeds) {
        let dir = tempfile::tempdir().unwrap();
        fs::write(dir.path().join("led_fan1_capability"), caps).unwrap();
        let leds = FanLeds::with_path(dir.path());
        (dir, leds)
    }

    fn read(dir: &tempfile::TempDir, name: &str) -> Option<String> {
        fs::read_to_string(dir.path().join(name)).ok()
    }

    #[test]
    fn capability_skips_none_and_strips_blink() {
        let (_d, leds) = tree("none red green red_blink\n");
        let caps = leds.capability("fan1");
        assert!(caps.contains("red") && caps.contains("green"));
        assert!(!caps.contains("none"));
        assert_eq!(caps.len(), 2);
    }

    #[test]
    fn setting_a_colour_writes_255_to_that_colour() {
        let (d, leds) = tree("red green\n");
        assert!(leds.set_status("fan1", COLOR_GREEN));
        assert_eq!(read(&d, "led_fan1_green").as_deref(), Some("255\n"));
        assert_eq!(read(&d, "led_fan1_red"), None);
    }

    /// Off must clear every supported colour, not just the last one set.
    #[test]
    fn off_clears_every_supported_colour() {
        let (d, leds) = tree("red green\n");
        leds.set_status("fan1", COLOR_RED);
        assert!(leds.set_status("fan1", COLOR_OFF));
        assert_eq!(read(&d, "led_fan1_red").as_deref(), Some("0\n"));
        assert_eq!(read(&d, "led_fan1_green").as_deref(), Some("0\n"));
    }

    #[test]
    fn red_falls_back_to_a_similar_supported_colour() {
        let (d, leds) = tree("orange green\n");
        assert!(leds.set_status("fan1", COLOR_RED));
        assert_eq!(read(&d, "led_fan1_orange").as_deref(), Some("255\n"));
    }

    #[test]
    fn no_capability_drives_nothing() {
        let dir = tempfile::tempdir().unwrap();
        let leds = FanLeds::with_path(dir.path());
        assert!(!leds.set_status("fan1", COLOR_GREEN));
        assert!(read(&dir, "led_fan1_green").is_none());
    }

    /// One failed fan turns the whole drawer red, and it stays red while that
    /// fan is still failed no matter what the others report.
    #[test]
    fn one_bad_fan_turns_the_drawer_red() {
        let (d, mut leds) = tree("red green\n");
        leds.set_fan_color("fan1", "drawer1", "fan1", COLOR_GREEN);
        assert_eq!(read(&d, "led_fan1_green").as_deref(), Some("255\n"));

        leds.set_fan_color("fan1", "drawer1", "fan2", COLOR_RED);
        assert_eq!(leds.drawer_color("drawer1"), COLOR_RED);
        assert_eq!(read(&d, "led_fan1_red").as_deref(), Some("255\n"));

        // The healthy fan reporting again must not clear its neighbour's red.
        leds.set_fan_color("fan1", "drawer1", "fan1", COLOR_GREEN);
        assert_eq!(leds.drawer_color("drawer1"), COLOR_RED);
    }

    #[test]
    fn a_recovered_fan_lets_the_drawer_go_green() {
        let (_d, mut leds) = tree("red green\n");
        leds.set_fan_color("fan1", "drawer1", "fan1", COLOR_RED);
        leds.set_fan_color("fan1", "drawer1", "fan2", COLOR_GREEN);
        assert_eq!(leds.drawer_color("drawer1"), COLOR_RED);
        leds.set_fan_color("fan1", "drawer1", "fan1", COLOR_GREEN);
        assert_eq!(leds.drawer_color("drawer1"), COLOR_GREEN);
    }

    #[test]
    fn drawers_do_not_bleed_into_each_other() {
        let (_d, mut leds) = tree("red green\n");
        leds.set_fan_color("fan1", "drawer1", "fan1", COLOR_RED);
        leds.set_fan_color("fan2", "drawer2", "fan2", COLOR_GREEN);
        assert_eq!(leds.drawer_color("drawer1"), COLOR_RED);
        assert_eq!(leds.drawer_color("drawer2"), COLOR_GREEN);
    }

    // ── status() ──────────────────────────────────────────────────────────

    /// No capability entry at all is what a simulated platform looks like.
    /// Python logs an error and still answers `off`.
    #[test]
    fn an_led_with_no_capability_reads_as_off() {
        let dir = tempfile::tempdir().unwrap();
        let leds = FanLeds::with_path(dir.path());
        assert_eq!(leds.status("fan1"), "off");
    }

    #[test]
    fn every_supported_colour_dark_reads_as_off() {
        let (dir, leds) = tree("green red\n");
        fs::write(dir.path().join("led_fan1_green"), "0").unwrap();
        fs::write(dir.path().join("led_fan1_red"), "0").unwrap();
        assert_eq!(leds.status("fan1"), "off");
    }

    /// The trap: a colour file is read with Python's `read_str_from_file`
    /// default of `""`, and `"" != "0"`, so a capability that lists a colour
    /// whose file does not exist reads as *lit*.  Treating absent as off would
    /// report `off` where Python reports a colour.
    #[test]
    fn a_missing_colour_file_counts_as_lit() {
        let (dir, leds) = tree("green\n");
        assert!(!dir.path().join("led_fan1_green").exists());
        assert_eq!(leds.status("fan1"), "green");
    }

    /// `_get_primary_color` collapses amber and orange onto red, so a caller
    /// only ever sees a good/bad signal from a steady LED.
    #[test]
    fn a_steady_orange_is_reported_as_red() {
        let (dir, leds) = tree("orange\n");
        fs::write(dir.path().join("led_fan1_orange"), "255").unwrap();
        assert_eq!(leds.status("fan1"), "red");
    }

    /// A blinking LED skips that mapping entirely and keeps its own colour.
    #[test]
    fn a_blinking_orange_keeps_its_colour_and_gains_a_suffix() {
        let (dir, leds) = tree("orange\n");
        fs::write(dir.path().join("led_fan1_orange"), "255").unwrap();
        fs::write(dir.path().join("led_fan1_orange_delay_on"), "50").unwrap();
        fs::write(dir.path().join("led_fan1_orange_delay_off"), "50").unwrap();
        assert_eq!(leds.status("fan1"), "orange_blink");
    }

    /// Both delay files must be non-zero.  Unlike the colour read, a *missing*
    /// delay file defaults to `"0"` — the two reads use different defaults and
    /// collapsing them would make every LED look like it was blinking.
    #[test]
    fn one_delay_file_alone_is_not_a_blink() {
        let (dir, leds) = tree("green\n");
        fs::write(dir.path().join("led_fan1_green"), "255").unwrap();
        fs::write(dir.path().join("led_fan1_green_delay_on"), "50").unwrap();
        assert_eq!(leds.status("fan1"), "green");
    }

    /// The capability list drives which files are consulted, so a colour the
    /// platform does not advertise is not read even when its file is lit.
    #[test]
    fn a_colour_outside_the_capability_list_is_ignored() {
        let (dir, leds) = tree("green\n");
        fs::write(dir.path().join("led_fan1_green"), "0").unwrap();
        fs::write(dir.path().join("led_fan1_red"), "255").unwrap();
        assert_eq!(leds.status("fan1"), "off");
    }

    /// Round trip: what `set_status` writes is what `status` reads back, which
    /// is the property `thermalctld` relies on after setting a fan LED.
    #[test]
    fn what_was_set_is_what_is_read_back() {
        let (dir, leds) = tree("green red\n");
        // Both colour files must exist and be dark to start with, as they are
        // on real hardware — see `a_missing_colour_file_counts_as_lit` for what
        // happens otherwise.
        fs::write(dir.path().join("led_fan1_green"), "0").unwrap();
        fs::write(dir.path().join("led_fan1_red"), "0").unwrap();
        assert!(leds.set_status("fan1", "red"));
        assert_eq!(leds.status("fan1"), "red");
        assert!(leds.set_status("fan1", "off"));
        assert_eq!(leds.status("fan1"), "off");
    }

    // ── The colour tables ─────────────────────────────────────────────────

    /// Python's `SIMILAR_COLORS`, which is what lets a daemon ask for "red" on
    /// a platform whose LED only knows "amber".  The three warm colours
    /// substitute for each other; nothing else has a substitute.
    #[test]
    fn the_warm_colours_stand_in_for_each_other() {
        assert_eq!(similar_colors("red"), ["amber", "orange"]);
        assert_eq!(similar_colors("amber"), ["red", "orange"]);
        assert_eq!(similar_colors("orange"), ["red", "amber"]);
        assert!(similar_colors("green").is_empty(), "green has no stand-in");
        assert!(similar_colors("blue").is_empty());
        assert!(similar_colors("off").is_empty());
    }

    /// The substitution is tried in order, so a platform that knows both amber
    /// and orange gets amber for a red request — the same one Python picks.
    #[test]
    fn a_substitution_takes_the_first_supported_candidate() {
        let (dir, leds) = tree("amber orange\n");
        assert!(leds.set_status("fan1", "red"));
        assert!(read(&dir, "led_fan1_amber").is_some(), "amber, not orange");
        assert!(read(&dir, "led_fan1_orange").is_none());
    }

    /// Red outranks green so that one bad fan colours its whole drawer, and an
    /// unknown colour must not silently outrank red — it sorts last.
    #[test]
    fn red_outranks_green_and_anything_unknown_ranks_below_both() {
        assert!(priority("red") < priority("green"));
        assert!(priority("green") < priority("blue"));
        assert!(priority("green") < priority("some_new_colour"));
    }

    /// A drawer nobody has asked anything of shows green: the daemon publishes
    /// a drawer's LED every cycle, and "no request yet" is a healthy drawer.
    #[test]
    fn a_drawer_with_no_requests_is_green() {
        let (_d, leds) = tree("green red\n");
        assert_eq!(leds.drawer_color("drawer1"), "green");
    }

    /// A read-back collapses the warm colours onto red so a caller only ever
    /// sees good or bad; blue and green pass through, and a colour outside the
    /// table is returned unchanged — Python's `dict.get(color, color)`.
    #[test]
    fn a_read_back_reports_only_primary_colours() {
        assert_eq!(primary_color("amber"), "red");
        assert_eq!(primary_color("orange"), "red");
        assert_eq!(primary_color("red"), "red");
        assert_eq!(primary_color("green"), "green");
        assert_eq!(primary_color("blue"), "blue");
        assert_eq!(primary_color("magenta"), "magenta", "unknown passes through");
    }

    /// A colour with no supported substitute is refused rather than written to
    /// a file that does not exist.
    #[test]
    fn an_unsupported_colour_with_no_substitute_is_refused() {
        let (dir, leds) = tree("green\n");
        assert!(!leds.set_status("fan1", "blue"));
        assert!(read(&dir, "led_fan1_blue").is_none());
        assert!(read(&dir, "led_fan1_green").is_none(), "and nothing else was driven");
    }
}
