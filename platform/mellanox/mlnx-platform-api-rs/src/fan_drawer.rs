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

//! Fan-drawer reading for Mellanox/NVIDIA hw-management.
//!
//! Ports Python `fan_drawer.py` — `RealDrawer` and `VirtualDrawer`.

use platform_traits::FanDrawerInfo;

use crate::led::FanLeds;
use crate::utils;

/// Read one fan drawer's state from sysfs.
///
/// `drawer_num` is 1-based (matching Python `i+1`).
/// `hotswappable` determines `is_replaceable`.
pub fn read_fan_drawer(
    thermal: &str,
    drawer_num: usize,
    hotswappable: bool,
    leds: &FanLeds,
) -> FanDrawerInfo {
    // presence: fan{N}_status == 1 for a real drawer.  A virtual drawer is not
    // a physical part, so `VirtualDrawer.get_presence()` overrides this and
    // returns True unconditionally (`fan_drawer.py:121-122`).
    let presence = !hotswappable
        || utils::read_int(&format!("{thermal}/fan{drawer_num}_status")) == Some(1);

    // Mellanox inherits DeviceBase.get_status(), which raises
    // NotImplementedError, so Python reports N/A for the drawer's own health.
    // Reporting presence here instead would publish a value Python does not.
    let status = None;

    // Virtual drawers (non-hotswappable) use "N/A" as name per Python.
    let name = if hotswappable {
        format!("drawer{drawer_num}")
    } else {
        "N/A".to_string()
    };

    FanDrawerInfo {
        name,
        position_in_parent: drawer_num as u32,
        presence,
        status,
        is_replaceable: hotswappable,
        model: None,
        serial: None,
        // `MellanoxFanDrawer.get_status_led()` reads the drawer's LED back
        // (`fan_drawer.py:77-85`), and `thermalctld:581` publishes it.  A real
        // drawer owns `led_fan{N}_*`; a virtual one shares the unnumbered
        // `led_fan_*`, because `VirtualDrawer` builds `FanLed(None)`.
        status_led: Some(leds.status(&led_id(drawer_num, hotswappable))),
    }
}

/// The hw-management LED id for a drawer: `fan{N}` for a real one, `fan` for a
/// virtual one (`fan_drawer.py:105-107`, `:115-116`).
pub fn led_id(drawer_num: usize, hotswappable: bool) -> String {
    if hotswappable {
        format!("fan{drawer_num}")
    } else {
        "fan".to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// An LED tree with no capability files: `status()` answers `off` for every
    /// id, so these cases stay independent of the host's sysfs.
    fn no_leds() -> (tempfile::TempDir, FanLeds) {
        let d = tempfile::tempdir().unwrap();
        let leds = FanLeds::with_path(d.path());
        (d, leds)
    }

    /// A virtual drawer is not a physical part, so Python overrides presence to
    /// True rather than reading `fan{N}_status` (`fan_drawer.py:121-122`).  This
    /// branch touches no sysfs, so the assertion holds on any host.
    #[test]
    fn a_virtual_drawer_is_always_present_and_unnamed() {
        let (dir, leds) = no_leds();
        let d = read_fan_drawer(&dir.path().to_string_lossy(), 1, false, &leds);
        assert!(d.presence);
        assert_eq!(d.name, "N/A");
        assert!(!d.is_replaceable);
    }

    #[test]
    fn a_real_drawer_is_named_after_its_index() {
        let (dir, leds) = no_leds();
        let d = read_fan_drawer(&dir.path().to_string_lossy(), 2, true, &leds);
        assert_eq!(d.name, "drawer2");
        assert!(d.is_replaceable);
    }

    /// A real drawer owns `led_fan{N}_*`; a virtual one shares `led_fan_*`,
    /// because `VirtualDrawer` builds `FanLed(None)` (`fan_drawer.py:115-116`).
    #[test]
    fn the_led_id_drops_the_index_for_a_virtual_drawer() {
        assert_eq!(led_id(3, true), "fan3");
        assert_eq!(led_id(3, false), "fan");
    }

    /// Python publishes whatever the LED reads back (`thermalctld:581`), and an
    /// LED with no capability entry reads as `off` — not `N/A`.
    #[test]
    fn a_drawer_reports_its_leds_colour() {
        let d = tempfile::tempdir().unwrap();
        std::fs::write(d.path().join("led_fan2_capability"), "green red\n").unwrap();
        std::fs::write(d.path().join("led_fan2_green"), "255\n").unwrap();
        std::fs::write(d.path().join("led_fan2_red"), "0\n").unwrap();
        let leds = FanLeds::with_path(d.path());

        let drawer = read_fan_drawer(&d.path().to_string_lossy(), 2, true, &leds);
        assert_eq!(drawer.status_led.as_deref(), Some("green"));
    }
}
