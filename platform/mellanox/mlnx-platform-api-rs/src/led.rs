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


//! `sonic_platform/led.py`.
//!
//! One difference from Python that Python itself does not keep constant: the
//! supported colours are a `set` there, and a set of strings iterates in an
//! order that changes with every interpreter start (string hashing is
//! randomised).  Which colour file is read first, and the order of the writes
//! that turn an LED off, therefore vary between two runs of the Python daemon.
//! Here they follow the capability file's order.  The answer differs only on
//! hardware with two colours lit at once.

use std::sync::{Arc, Mutex};
use std::thread::sleep;
use std::time::Duration;

use platform_base::io;
use platform_base::py::PyResult;

use crate::device_data;
use crate::utils::{self, Log};

pub const STATUS_LED_COLOR_GREEN: &str = "green";
// Python's `Led.STATUS_LED_COLOR_RED`, kept with the others; only the
// tests name it, the code matches "red" through the capability file.
#[allow(dead_code)]
pub const STATUS_LED_COLOR_RED: &str = "red";
pub const STATUS_LED_COLOR_OFF: &str = "off";

const LED_ON: &str = "255";
const LED_OFF: &str = "0";
const LED_BLINK: &str = "50";
const LED_PATH: &str = "/var/run/hw-management/led/";

/// `SIMILAR_COLORS`.
fn similar_colors(color: &str) -> Option<&'static [&'static str]> {
    match color {
        "red" => Some(&["amber", "orange"]),
        "amber" => Some(&["red", "orange"]),
        "orange" => Some(&["red", "amber"]),
        _ => None,
    }
}

/// `PRIMARY_COLORS.get(color, color)`.
fn primary_color(color: &str) -> String {
    match color {
        "red" | "amber" | "orange" => "red".into(),
        "green" => "green".into(),
        "blue" => "blue".into(),
        other => other.into(),
    }
}

/// `Led`: one LED's colour files under hw-management.
pub struct Led {
    led_id: String,
    supported_colors: Mutex<Vec<String>>,
    supported_blinks: Mutex<Vec<String>>,
}

impl Led {
    fn with_id(led_id: String) -> Arc<Led> {
        crate::imports::led();
        Arc::new(Led {
            led_id,
            supported_colors: Mutex::new(Vec::new()),
            supported_blinks: Mutex::new(Vec::new()),
        })
    }

    /// `FanLed(index)`.
    pub fn fan(index: Option<i128>) -> Arc<Led> {
        Led::with_id(index.map_or("fan".into(), |i| format!("fan{i}")))
    }

    /// `PsuLed(index)`.
    pub fn psu(index: Option<i128>) -> Arc<Led> {
        Led::with_id(index.map_or("psu".into(), |i| format!("psu{i}")))
    }

    /// `SystemLed()`.
    pub fn system() -> Arc<Led> {
        Led::with_id("status".into())
    }

    /// `SystemUidLed()`.
    pub fn system_uid() -> Arc<Led> {
        Led::with_id("uid".into())
    }

    fn colors(&self) -> Vec<String> {
        utils::lock(&self.supported_colors).clone()
    }

    fn blinks(&self) -> Vec<String> {
        utils::lock(&self.supported_blinks).clone()
    }

    fn actual_color(&self, color: &str) -> Option<String> {
        let colors = self.colors();
        if colors.iter().any(|c| c == color) {
            return Some(color.to_string());
        }
        similar_colors(color)?
            .iter()
            .find(|c| colors.iter().any(|s| s == *c))
            .map(|c| c.to_string())
    }

    fn actual_blink_color(&self, color: &str) -> Option<String> {
        let blinks = self.blinks();
        if blinks.iter().any(|c| c == color) {
            return Some(color.to_string());
        }
        similar_colors(color)?
            .iter()
            .find(|c| blinks.iter().any(|s| s == *c))
            .map(|c| c.to_string())
    }

    /// `set_status(color)`.
    pub fn set_status(&self, color: &str) -> PyResult<bool> {
        self.get_capability();
        if self.colors().is_empty() {
            if !device_data::is_simx_platform()? {
                utils::log_error(&format!("Failed to get LED capability for {} LED", self.led_id));
            }
            return Ok(false);
        }
        self.stop_blink();
        if let Some(pos) = color.find("_blink") {
            return Ok(self.set_status_blink(&color[..pos]));
        }
        if color != STATUS_LED_COLOR_OFF {
            let Some(actual) = self.actual_color(color) else {
                utils::log_error(&format!("Set LED to color {color} is not supported"));
                return Ok(false);
            };
            let _ = utils::write_file(&self.led_path(&actual), &LED_ON, false, Log::Error);
        } else {
            for c in self.colors() {
                let _ = utils::write_file(&self.led_path(&c), &LED_OFF, false, Log::Error);
            }
        }
        Ok(true)
    }

    fn set_status_blink(&self, color: &str) -> bool {
        let Some(actual) = self.actual_blink_color(color) else {
            utils::log_error(&format!("Set LED to color {color}_blink is not supported"));
            return false;
        };
        let _ = utils::write_file(&self.led_trigger(&actual), &"timer", false, Log::Error);
        self.set_led_blink_status(&actual)
    }

    fn stop_blink(&self) {
        for c in self.colors() {
            let _ = utils::write_file(&self.led_trigger(&c), &"none", false, Log::Error);
        }
    }

    fn set_led_blink_status(&self, color: &str) -> bool {
        let on = self.led_delay_on_path(color);
        let off = self.led_delay_off_path(color);
        if !wait_files_ready(&[&on, &off]) {
            return false;
        }
        let _ = utils::write_file(&on, &LED_BLINK, false, Log::Error);
        let _ = utils::write_file(&off, &LED_BLINK, false, Log::Error);
        true
    }

    /// `get_status()`.
    pub fn get_status(&self) -> PyResult<String> {
        self.get_capability();
        if self.colors().is_empty() {
            if !device_data::is_simx_platform()? {
                utils::log_error(&format!("Failed to get LED capability for {} LED", self.led_id));
            }
            return Ok(STATUS_LED_COLOR_OFF.into());
        }
        if let Some(b) = self.blink_status() {
            return Ok(b);
        }
        for c in self.colors() {
            // A file that cannot be read reads as '' -- not '0', so "lit".
            if utils::read_str(&self.led_path(&c), "", Log::Error) != LED_OFF {
                return Ok(primary_color(&c));
            }
        }
        Ok(STATUS_LED_COLOR_OFF.into())
    }

    fn blink_status(&self) -> Option<String> {
        self.colors()
            .into_iter()
            .find(|c| self.is_led_blinking(c))
            .map(|c| format!("{c}_blink"))
    }

    fn is_led_blinking(&self, color: &str) -> bool {
        let on = utils::read_str(&self.led_delay_on_path(color), LED_OFF, Log::Off);
        let off = utils::read_str(&self.led_delay_off_path(color), LED_OFF, Log::Off);
        on != LED_OFF && off != LED_OFF
    }

    /// `get_capability()`: read every time; the sets only ever grow.
    pub fn get_capability(&self) {
        let caps = utils::read_str(&self.led_cap_path(), "", Log::Error);
        for cap in caps.split_whitespace() {
            if cap == "none" {
                continue;
            }
            let (set, name) = match cap.find("_blink") {
                Some(pos) => (&self.supported_blinks, &cap[..pos]),
                None => (&self.supported_colors, cap),
            };
            let mut s = utils::lock(set);
            if !s.iter().any(|c| c == name) {
                s.push(name.to_string());
            }
        }
    }

    fn led_path(&self, color: &str) -> String {
        io::join(LED_PATH, &format!("led_{}_{color}", self.led_id))
    }
    fn led_trigger(&self, color: &str) -> String {
        io::join(LED_PATH, &format!("led_{}_{color}_trigger", self.led_id))
    }
    fn led_delay_off_path(&self, color: &str) -> String {
        io::join(LED_PATH, &format!("led_{}_{color}_delay_off", self.led_id))
    }
    fn led_delay_on_path(&self, color: &str) -> String {
        io::join(LED_PATH, &format!("led_{}_{color}_delay_on", self.led_id))
    }
    fn led_cap_path(&self) -> String {
        io::join(LED_PATH, &format!("led_{}_capability", self.led_id))
    }
}

/// `_wait_files_ready(file_list)`: 5 s, sleeping 10 ms and doubling.
fn wait_files_ready(files: &[&str]) -> bool {
    let mut wait_time = 5.0f64;
    let mut initial = 0.01f64;
    while wait_time > 0.0 {
        if files.iter().all(|f| io::exists(f)) {
            return true;
        }
        sleep(Duration::from_secs_f64(initial));
        wait_time -= initial;
        initial *= 2.0;
    }
    false
}

/// `SharedLed`: one physical LED that several components drive.
pub struct SharedLed {
    led: Arc<Led>,
    virtual_leds: Mutex<Vec<Arc<ComponentFaultyIndicator>>>,
}

impl SharedLed {
    pub fn new(led: Arc<Led>) -> Arc<SharedLed> {
        Arc::new(SharedLed { led, virtual_leds: Mutex::new(Vec::new()) })
    }

    #[cfg(test)]
    pub fn led(&self) -> &Arc<Led> {
        &self.led
    }

    /// `update_status_led()`: red if any component is red, else green; any
    /// other colour is a `KeyError` in Python's priority table, answered as
    /// False without touching the LED.
    fn update_status_led(&self) -> PyResult<bool> {
        let leds = utils::lock(&self.virtual_leds).clone();
        let prio = |c: &str| match c {
            "red" => Some(0),
            "green" => Some(1),
            _ => None,
        };
        let mut target = STATUS_LED_COLOR_GREEN.to_string();
        for v in leds {
            let c = v.get_led_color();
            let (Some(a), Some(b)) = (prio(&c), prio(&target)) else {
                return Ok(false);
            };
            if a < b {
                target = c;
            }
        }
        self.led.set_status(&target)
    }

    pub fn get_status(&self) -> PyResult<String> {
        self.led.get_status()
    }
}

/// `ComponentFaultyIndicator`: one component's vote on a shared LED.
pub struct ComponentFaultyIndicator {
    color: Mutex<String>,
    shared: Arc<SharedLed>,
}

impl ComponentFaultyIndicator {
    /// Registered with the shared LED for good, as Python's list holds it.
    pub fn new(shared: Arc<SharedLed>) -> Arc<ComponentFaultyIndicator> {
        let me = Arc::new(ComponentFaultyIndicator {
            color: Mutex::new(STATUS_LED_COLOR_GREEN.into()),
            shared: shared.clone(),
        });
        utils::lock(&shared.virtual_leds).push(me.clone());
        me
    }

    /// `set_status(color)`: the old colour comes back if the LED refused --
    /// not if it raised.
    pub fn set_status(&self, color: &str) -> PyResult<bool> {
        let current = self.get_led_color();
        *utils::lock(&self.color) = color.to_string();
        if self.shared.update_status_led()? {
            Ok(true)
        } else {
            *utils::lock(&self.color) = current;
            Ok(false)
        }
    }

    pub fn get_led_color(&self) -> String {
        utils::lock(&self.color).clone()
    }

    pub fn get_status(&self) -> PyResult<String> {
        self.shared.get_status()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    const CAP_ALL: &str = "none green green_blink red red_blink orange blue";

    /// An LED's files as Python's test_led mocks them: green lit, the rest
    /// off, every delay file there and 0.
    fn files(root: &Root, led: &Led, cap: &str) {
        root.write(&led.led_cap_path(), cap);
        for c in ["green", "red", "orange", "blue", "amber"] {
            root.write(&led.led_path(c), if c == "green" { LED_ON } else { LED_OFF });
            root.write(&led.led_delay_on_path(c), LED_OFF);
            root.write(&led.led_delay_off_path(c), LED_OFF);
        }
    }

    fn set(root: &Root, path: &str, v: &str) {
        root.write(path, v);
    }

    #[test]
    fn each_kind_of_led_has_its_own_files() {
        let paths = |l: &Led| (l.led_path("green"), l.led_trigger("red"), l.led_cap_path());
        assert_eq!(paths(&Led::fan(Some(1))).0, "/var/run/hw-management/led/led_fan1_green");
        assert_eq!(paths(&Led::fan(None)).1, "/var/run/hw-management/led/led_fan_red_trigger");
        assert_eq!(paths(&Led::psu(Some(2))).2, "/var/run/hw-management/led/led_psu2_capability");
        assert_eq!(paths(&Led::psu(None)).0, "/var/run/hw-management/led/led_psu_green");
        assert_eq!(paths(&Led::system()).0, "/var/run/hw-management/led/led_status_green");
        assert_eq!(paths(&Led::system_uid()).0, "/var/run/hw-management/led/led_uid_green");
    }

    /// Python's `_verify_non_shared_led`, over files.
    #[test]
    fn a_led_shows_and_blinks_the_colours_it_has() {
        let root = Root::new();
        let led = Led::system();
        files(&root, &led, CAP_ALL);
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
        set(&root, &led.led_path("green"), LED_OFF);
        assert!(led.set_status(STATUS_LED_COLOR_RED).unwrap());
        assert_eq!(root.read(&led.led_path("red")).as_deref(), Some(LED_ON));
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_RED);
        set(&root, &led.led_path("red"), LED_OFF);
        assert!(led.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
        set(&root, &led.led_path("green"), LED_OFF);
        assert!(led.set_status("orange").unwrap());
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_RED, "orange reads as its primary");
        set(&root, &led.led_path("orange"), LED_OFF);

        assert!(led.set_status("red_blink").unwrap());
        assert_eq!(root.read(&led.led_trigger("red")).as_deref(), Some("timer"));
        assert_eq!(root.read(&led.led_delay_on_path("red")).as_deref(), Some(LED_BLINK));
        assert_eq!(led.get_status().unwrap(), "red_blink");
        set(&root, &led.led_delay_off_path("red"), LED_OFF);
        set(&root, &led.led_delay_on_path("red"), LED_OFF);
        assert!(led.set_status("green_blink").unwrap());
        assert_eq!(led.get_status().unwrap(), "green_blink");
        // Setting any colour stops every blink first.
        assert_eq!(root.read(&led.led_trigger("red")).as_deref(), Some("none"));
        set(&root, &led.led_delay_off_path("green"), LED_OFF);
        set(&root, &led.led_delay_on_path("green"), LED_OFF);
        // orange has no blink: red's, its similar colour.
        assert!(led.set_status("orange_blink").unwrap());
        assert_eq!(led.get_status().unwrap(), "red_blink");

        // Off: every colour written 0.
        assert!(led.set_status(STATUS_LED_COLOR_OFF).unwrap());
        for c in ["green", "red", "orange", "blue"] {
            assert_eq!(root.read(&led.led_path(c)).as_deref(), Some(LED_OFF), "{c}");
        }
        set(&root, &led.led_delay_off_path("red"), LED_OFF);
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_OFF);
        // What it cannot show.
        assert!(!led.set_status("black").unwrap());
        assert!(!led.set_status("blue_blink").unwrap());
        assert!(!led.set_status("black_blink").unwrap());
    }

    /// Python's `_verify_uid_led`.
    #[test]
    fn the_uid_led_shows_blue() {
        let root = Root::new();
        let led = Led::system_uid();
        files(&root, &led, CAP_ALL);
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
        set(&root, &led.led_path("green"), LED_OFF);
        assert!(led.set_status("blue").unwrap());
        assert_eq!(led.get_status().unwrap(), "blue");
    }

    #[test]
    fn a_missing_colour_takes_a_similar_one() {
        let root = Root::new();
        let led = Led::psu(Some(1));
        files(&root, &led, "amber green amber_blink");
        set(&root, &led.led_path("green"), LED_OFF);
        assert!(led.set_status(STATUS_LED_COLOR_RED).unwrap());
        assert_eq!(root.read(&led.led_path("amber")).as_deref(), Some(LED_ON));
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_RED);
        assert!(led.set_status("orange").unwrap());
        assert!(led.set_status("red_blink").unwrap());
        assert_eq!(led.get_status().unwrap(), "amber_blink");
        assert_eq!(led.actual_color("blue"), None);
        assert_eq!(led.actual_blink_color("green"), None);
    }

    #[test]
    fn a_colour_outside_the_primary_table_reads_as_itself() {
        let root = Root::new();
        let led = Led::system();
        root.write(&led.led_cap_path(), "white");
        root.write(&led.led_path("white"), LED_ON);
        assert_eq!(led.get_status().unwrap(), "white");
    }

    #[test]
    fn a_colour_file_that_cannot_be_read_reads_as_lit() {
        let root = Root::new();
        let led = Led::fan(Some(1));
        root.write(&led.led_cap_path(), "green");
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
    }

    #[test]
    fn without_a_capability_there_is_nothing_to_show() {
        let root = Root::new();
        let led = Led::fan(None);
        assert!(!led.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_OFF);
        root.write(&led.led_cap_path(), "none");
        assert!(!led.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        // A simx platform has no LEDs, and says nothing about it.
        crate::utils::tests::platform(&root, "x86_64-nvidia_sn5600_simx-r0");
        assert!(!led.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(led.get_status().unwrap(), STATUS_LED_COLOR_OFF);
    }

    #[test]
    fn blinking_waits_for_the_delay_files() {
        let root = Root::new();
        let led = Led::system();
        root.write(&led.led_cap_path(), "green green_blink");
        root.write(&led.led_path("green"), LED_OFF);
        let (on, off) = (root.real(&led.led_delay_on_path("green")), root.real(&led.led_delay_off_path("green")));
        let t = std::thread::spawn(move || {
            sleep(Duration::from_millis(50));
            std::fs::write(on, LED_OFF).unwrap();
            std::fs::write(off, LED_OFF).unwrap();
        });
        assert!(led.set_status("green_blink").unwrap());
        t.join().unwrap();
        assert_eq!(led.get_status().unwrap(), "green_blink");
        assert!(wait_files_ready(&[]));
    }

    /// Python's `_verify_shared_led`: red if any component says red.
    #[test]
    fn a_shared_led_is_red_while_any_component_is() {
        let root = Root::new();
        let shared = SharedLed::new(Led::psu(None));
        files(&root, shared.led(), "green red");
        let (a, b) = (ComponentFaultyIndicator::new(shared.clone()), ComponentFaultyIndicator::new(shared.clone()));
        let path = |c: &str| shared.led().led_path(c);
        assert!(a.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(b.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
        set(&root, &path("green"), LED_OFF);
        assert!(b.set_status(STATUS_LED_COLOR_RED).unwrap());
        assert_eq!(b.get_status().unwrap(), STATUS_LED_COLOR_RED);
        assert!(a.set_status(STATUS_LED_COLOR_RED).unwrap());
        assert_eq!(b.get_status().unwrap(), STATUS_LED_COLOR_RED);
        set(&root, &path("red"), LED_OFF);
        assert!(a.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(a.get_status().unwrap(), STATUS_LED_COLOR_RED);
        assert_eq!(shared.get_status().unwrap(), STATUS_LED_COLOR_RED);
        set(&root, &path("red"), LED_OFF);
        assert!(b.set_status(STATUS_LED_COLOR_GREEN).unwrap());
        assert_eq!(a.get_status().unwrap(), STATUS_LED_COLOR_GREEN);
        // A colour outside the priority table: refused, the old one kept.
        assert!(!a.set_status("orange").unwrap());
        assert_eq!(a.get_led_color(), STATUS_LED_COLOR_GREEN);
        // The LED refusing (no capability) also keeps the old colour.
        std::fs::remove_file(root.real(&shared.led().led_cap_path())).unwrap();
        let lone = SharedLed::new(Led::psu(Some(9)));
        let c = ComponentFaultyIndicator::new(lone);
        assert!(!c.set_status(STATUS_LED_COLOR_RED).unwrap());
        assert_eq!(c.get_led_color(), STATUS_LED_COLOR_GREEN);
    }
}
