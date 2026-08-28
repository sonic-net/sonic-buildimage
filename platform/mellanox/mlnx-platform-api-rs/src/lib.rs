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

//! Mellanox/NVIDIA SONiC platform API implementation.
//!
//! `Platform` implements `platform_traits::PlatformApi` by reading
//! sysfs files exposed by `hw-management`.  No gRPC, no Python interpreter,
//! no shared library — all hardware access is direct synchronous file I/O.
//!
//! `MellanoxThermalManager` implements `ThermalManager` by spawning a
//! background `std::thread` (via `thermal_updater`) that pushes ASIC and
//! module temperatures from STATE_DB into hw-management sysfs so that
//! `hw-management-tc` can operate in independent mode.

mod device_data;
mod dpu;

/// Where `DPUS` is declared on a SmartSwitch.
const PLATFORM_JSON: &str = "/usr/share/sonic/platform/platform.json";
mod fan;
mod fan_drawer;
mod led;
mod liquid_cooling;
mod pdb;
mod psu;
mod spc1;
mod thermal;
mod thermal_updater;
pub mod utils;
mod vpd;

use platform_traits::{
    ChassisInfo, FanDrawerInfo, FanInfo, PlatformApi, PlatformError, PsuInfo, ThermalInfo, ThermalManager,
};

use device_data::{
    get_asic_count, get_fan_count, get_fan_drawer_count, get_pdb_count, get_psu_count, is_fan_hotswappable,
    is_multi_asic, is_psu_hotswappable,
};
use thermal::ThermalEntry;
use thermal_updater::{start_thermal_updater, ThermalUpdaterConfig, ThermalUpdaterHandle};

// ── MellanoxThermalManager ────────────────────────────────────────────────────

/// Platform-specific thermal management for Mellanox/NVIDIA.
///
/// `initialize()` suspends hw-management-tc and starts the ThermalUpdater
/// background thread.  `deinitialize()` cancels the thread (which restores
/// hw-management-tc).  `run_policy()` is a no-op: fan control is owned by
/// hw-management-tc.
pub struct MellanoxThermalManager {
    updater_cfg: Option<ThermalUpdaterConfig>,
    handle: Option<ThermalUpdaterHandle>,
    /// Needed to decide whether the SPC1 thermal file preparation applies.
    platform_name: String,
}

impl MellanoxThermalManager {
    fn new(cfg: ThermalUpdaterConfig, platform_name: String) -> Self {
        Self {
            updater_cfg: Some(cfg),
            handle: None,
            platform_name,
        }
    }
}

impl ThermalManager for MellanoxThermalManager {
    fn initialize(&mut self) -> Result<(), PlatformError> {
        // Python's three-way branch in thermal_manager.py:43-58.  A liquid
        // cooled platform feeds nothing: hw-management-tc is not driven from
        // STATE_DB there, so starting the updater would suspend it for no
        // reason.  Tested before the configuration is consumed, so that this
        // path leaves the manager re-initialisable — it never started anything.
        if liquid_cooling::sensor_count() > 0 {
            log::info!("Liquid cooling platform detected, thermal updater is disabled");
            return Ok(());
        }

        let cfg = self
            .updater_cfg
            .take()
            .ok_or_else(|| PlatformError::Other("ThermalManager already initialized".into()))?;
        // On SPC1 the thermal files start life as symlinks; they have to be
        // replaced by real files before the feed writes to them.  A timeout
        // here is logged and the daemon carries on, so the rest of the polling
        // still runs.
        spc1::prepare_thermal_files(&self.platform_name, cfg.module_count);

        // On a SmartSwitch the same thread also feeds each DPU's CPU, DDR and
        // drive temperatures, at its own interval.
        let dpu_ids = dpu::dpu_ids(PLATFORM_JSON);
        if !dpu_ids.is_empty() {
            log::info!("SmartSwitch platform: feeding {} DPU(s)", dpu_ids.len());
        }

        // From here on tc is driven from temperatures this process feeds it, so
        // every way this process can stop has to put tc back.  The graceful
        // paths go through deinitialize(); a panic aborts without unwinding, so
        // it needs its own hook.  Installed here rather than at start-up so it
        // is absent on the liquid-cooled path above, which never touches
        // `suspend` at all.
        thermal_updater::install_panic_hook(cfg.asic_count, cfg.module_count);

        self.handle = Some(start_thermal_updater(cfg, dpu_ids));
        Ok(())
    }

    fn deinitialize(&mut self) {
        if let Some(h) = self.handle.take() {
            h.cancel();
        }
    }
}

// ── Platform ──────────────────────────────────────────────────────────

/// Mellanox/NVIDIA chassis platform API implementation.
pub struct Platform {
    /// Platform name from /etc/sonic/platform; drives the capability lookup
    /// and the SPC1 thermal file preparation.
    platform_name: String,
    /// Per-drawer LED aggregate; see `led.rs`.
    fan_leds: led::FanLeds,
    /// Pre-discovered list of thermal sensors in STATE_DB key order.
    thermals: Vec<ThermalEntry>,
    fan_drawer_count: usize,
    #[allow(dead_code)]
    fan_count: usize,
    psu_count: usize,
    /// Python caches this with `@read_only_cache()`; it decides which of the
    /// two PSU classes the platform builds and cannot change under us.
    psu_hotswappable: bool,
    /// Built once: each PSU caches its VPD and the last model it saw.
    psus: Vec<psu::Psu>,
    pdb_count: usize,
    fans_per_drawer: usize,
    hotswappable: bool,
    asic_count: usize,
    asic_names: Vec<String>,
    /// hw-management's `thermal` directory, and the tree it sits in.
    ///
    /// Held rather than reached for through a constant so that the six trait
    /// methods below — the whole of what a daemon sees — can be exercised
    /// against a temporary tree.  `new()` fills both with the real paths.
    hw_thermal: String,
    hw_base: String,
}

impl Platform {
    /// Construct and initialise the platform, discovering all sensors.
    pub fn new() -> Result<Self, PlatformError> {
        let platform_name = utils::get_platform_name();
        let cap = device_data::get_thermal_capability(&platform_name);
        log::info!("Platform: {platform_name}");
        log::info!(
            "Thermal capability: comex={}, cpu_pack={}, pch={}, cpu_amb={}, swb_amb={}",
            cap.comex_amb,
            cap.cpu_pack,
            cap.pch_temp,
            cap.cpu_amb,
            cap.swb_amb
        );

        let thermals = thermal::discover_thermals(&cap);

        let fan_drawer_count = get_fan_drawer_count();
        let fan_count = get_fan_count();
        let psu_count = get_psu_count();
        let psu_hotswappable = is_psu_hotswappable();
        let psus = psu::discover(psu::HW_MGMT_BASE, psu_count, psu_hotswappable, &platform_name);
        let pdb_count = get_pdb_count();
        let hotswappable = is_fan_hotswappable();
        // Truncating, and the remainder is dropped: `chassis.py:306-315` does
        // `fan_num // drawer_num` and then loops `drawer_num` x that, so a fan
        // count that is not a multiple of the drawer count leaves the tail
        // unenumerated in Python too.  Warning about it here would put a line
        // in syslog that the Python daemon does not.
        let fans_per_drawer = if fan_drawer_count == 0 {
            0
        } else {
            fan_count / fan_drawer_count
        };

        let asic_count = get_asic_count();
        let multi = is_multi_asic();
        let asic_names = (0..asic_count)
            .map(|i| if multi { format!("ASIC{i}") } else { "ASIC".to_string() })
            .collect();

        Ok(Self {
            platform_name: platform_name.clone(),
            fan_leds: led::FanLeds::new(),
            thermals,
            fan_drawer_count,
            fan_count,
            psu_count,
            psu_hotswappable,
            psus,
            pdb_count,
            fans_per_drawer,
            hotswappable,
            asic_count,
            asic_names,
            hw_thermal: utils::HW_MGMT_THERMAL.to_string(),
            hw_base: psu::HW_MGMT_BASE.to_string(),
        })
    }
}

/// The hw-management LED id behind a PSU name, or `None` for something that has
/// no writable LED.
///
/// One LED is shared by every hot-swappable PSU and each fixed PSU has its own —
/// the same split `psu::Psu::led_id` makes on the read side, and getting it
/// backwards drives a file no platform has.  A PDB is rejected here: its LED is
/// the aggregate `led_power` file, which nothing writes.
fn psu_led_id(psu_name: &str, hotswappable: bool) -> Option<String> {
    let num = psu_name.strip_prefix("PSU ")?;
    Some(if hotswappable {
        "psu".to_string()
    } else {
        format!("psu{num}")
    })
}

impl PlatformApi for Platform {
    fn chassis_info(&self) -> Result<ChassisInfo, PlatformError> {
        // Standard Mellanox fixed-form-factor chassis.
        // is_liquid_cooled: if no leak sensors present, treat as air-cooled.
        let leak_count = liquid_cooling::sensor_count();
        Ok(ChassisInfo {
            is_modular_chassis: false,
            // A SmartSwitch is one that declares DPUs; the DPU feed itself is
            // driven from the same list, inside the thermal updater.
            is_smartswitch: !dpu::dpu_ids(PLATFORM_JSON).is_empty(),
            is_dpu: false,
            is_liquid_cooled: leak_count > 0,
            // Fixed-form-factor: neither a chassis slot nor a DPU, so the
            // daemon writes no slot-suffixed table.  A modular or SmartSwitch
            // platform reports its id here.
            slot_or_dpu_id: None,
        })
    }

    // `&mut self` per the trait: a vendor that implements
    // `get_minimum_recorded` / `get_maximum_recorded` needs somewhere to keep
    // the running extremes.  Mellanox does not — see `thermal::read_thermal` —
    // so nothing here mutates.
    fn get_thermals(&mut self) -> Result<Vec<ThermalInfo>, PlatformError> {
        Ok(self.thermals.iter().map(thermal::read_thermal).collect())
    }

    fn get_fan_drawers(&self) -> Result<Vec<FanDrawerInfo>, PlatformError> {
        let mut result = Vec::with_capacity(self.fan_drawer_count);
        for drawer_num in 1..=self.fan_drawer_count {
            result.push(fan_drawer::read_fan_drawer(
                &self.hw_thermal,
                drawer_num,
                self.hotswappable,
                &self.fan_leds,
            ));
        }
        Ok(result)
    }

    fn get_fans(&self) -> Result<Vec<FanInfo>, PlatformError> {
        let mut result = Vec::new();

        // Drawer fans
        for drawer_num in 1..=self.fan_drawer_count {
            let drawer_name = if self.hotswappable {
                format!("drawer{drawer_num}")
            } else {
                "N/A".to_string()
            };
            // One presence read per drawer, shared by its fans, because that is
            // what a drawer fan's presence *is* in Python.  It re-reads what
            // `get_fan_drawers` already read this cycle, which is the same
            // count Python ends up with -- `Fan.get_presence()` delegates to
            // the drawer's, so Python reads once *per fan* where this reads
            // once per drawer.  `read_fan_drawer` opens one file, so caching
            // it across the two calls would save five `read_int`s a cycle and
            // add a staleness window; not worth the trade.
            let drawer_present =
                fan_drawer::read_fan_drawer(&self.hw_thermal, drawer_num, self.hotswappable, &self.fan_leds).presence;
            for pos in 1..=self.fans_per_drawer {
                let fan_abs = (drawer_num - 1) * self.fans_per_drawer + pos;
                result.push(fan::read_drawer_fan(
                    &self.hw_thermal,
                    fan_abs,
                    &drawer_name,
                    pos,
                    drawer_present,
                    &self.fan_leds,
                ));
            }
        }

        // PSU fans
        for psu_num in 1..=self.psu_count {
            result.push(fan::read_psu_fan(&self.hw_thermal, psu_num, 1, &self.fan_leds));
        }

        Ok(result)
    }

    fn set_fan_led(&mut self, fan_name: &str, drawer_name: &str, color: &str) -> Result<(), PlatformError> {
        // hw-management names a drawer's LED led_fan{N}_*, where N is the
        // drawer number this crate put into the drawer name.
        let Some(num) = drawer_name.strip_prefix("drawer") else {
            // Virtual (non-hotswappable) drawers are named "N/A" and have no LED.
            return Err(PlatformError::NotSupported(format!(
                "fan '{fan_name}' is not in a drawer with an LED"
            )));
        };
        let led_id = format!("fan{num}");
        if self.fan_leds.set_fan_color(&led_id, drawer_name, fan_name, color) {
            Ok(())
        } else {
            Err(PlatformError::NotSupported(format!("no LED capability for {led_id}")))
        }
    }

    fn get_psus(&mut self) -> Result<Vec<PsuInfo>, PlatformError> {
        let mut result = Vec::with_capacity(self.psus.len() + self.pdb_count);
        for p in &mut self.psus {
            result.push(p.read(&self.fan_leds));
        }
        // `psud` walks the PSU list then the PDB list, and publishes them under
        // different key templates; the kind on each row carries that apart.
        for i in 1..=self.pdb_count {
            result.push(pdb::read_pdb(&self.hw_base, i));
        }
        Ok(result)
    }

    fn set_psu_led(&mut self, psu_name: &str, color: &str) -> Result<(), PlatformError> {
        let Some(led_id) = psu_led_id(psu_name, self.psu_hotswappable) else {
            return Err(PlatformError::NotSupported(format!(
                "'{psu_name}' has no writable status LED"
            )));
        };
        if self.fan_leds.set_status(&led_id, color) {
            Ok(())
        } else {
            Err(PlatformError::NotSupported(format!("no LED capability for {led_id}")))
        }
    }

    fn get_leak_profiles(&self) -> Vec<platform_traits::LeakProfile> {
        liquid_cooling::profiles()
    }

    fn get_leak_sensors(&self) -> Vec<platform_traits::LeakSensorInfo> {
        liquid_cooling::sensors()
    }

    fn get_thermal_manager(&self) -> Box<dyn ThermalManager> {
        let cfg = ThermalUpdaterConfig::from_platform(self.asic_count, self.asic_names.clone());
        Box::new(MellanoxThermalManager::new(cfg, self.platform_name.clone()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Hot-swappable PSUs share one LED; fixed ones do not.  Driving `psu1`
    /// on a shared platform writes a file that does not exist, and driving
    /// `psu` on a fixed one moves every PSU's LED at once.
    #[test]
    fn the_psu_led_id_follows_whether_the_led_is_shared() {
        assert_eq!(psu_led_id("PSU 2", true).as_deref(), Some("psu"));
        assert_eq!(psu_led_id("PSU 2", false).as_deref(), Some("psu2"));
    }

    /// A PDB reports the aggregate `led_power` file, which is read-only, so it
    /// must not resolve to an id at all.
    #[test]
    fn a_pdb_has_no_writable_led() {
        assert_eq!(psu_led_id("PDB 1", true), None);
        assert_eq!(psu_led_id("PDB 1", false), None);
    }

    // ── The trait surface, against a hw-management tree ───────────────────

    use platform_traits::{FanKind, PowerEntityKind};

    fn tree(files: &[(&str, &str)]) -> tempfile::TempDir {
        let d = tempfile::tempdir().unwrap();
        for sub in ["thermal", "power", "config", "eeprom", "led", "system", "environment"] {
            std::fs::create_dir_all(d.path().join(sub)).unwrap();
        }
        for (n, v) in files {
            std::fs::write(d.path().join(n), v).unwrap();
        }
        d
    }

    /// A platform wired to a temporary tree, with the discovery step already
    /// done — what is exercised here is the assembly and the six accessors, not
    /// the sysfs scan that fills the counts in.
    fn platform_at(dir: &tempfile::TempDir, drawers: usize, fans: usize, psus: usize) -> Platform {
        let base = dir.path().to_string_lossy().into_owned();
        Platform {
            platform_name: "x86_64-nvidia_sn5640-r0".to_string(),
            fan_leds: led::FanLeds::with_path(dir.path().join("led")),
            thermals: Vec::new(),
            fan_drawer_count: drawers,
            fan_count: fans,
            psu_count: psus,
            psu_hotswappable: true,
            psus: psu::discover(&base, psus, true, "x86_64-nvidia_sn5640-r0"),
            pdb_count: 0,
            fans_per_drawer: if drawers == 0 { 0 } else { fans / drawers },
            hotswappable: true,
            asic_count: 1,
            asic_names: vec!["ASIC".to_string()],
            hw_thermal: format!("{base}/thermal"),
            hw_base: base,
        }
    }

    /// The fan list is drawer fans first, then PSU fans — the order `psud` and
    /// `thermalctld` publish them in — and a drawer's fans are numbered
    /// absolutely across the chassis while their position is within the drawer.
    #[test]
    fn the_fan_list_is_drawer_fans_then_psu_fans() {
        let d = tree(&[]);
        let p = platform_at(&d, 2, 4, 2);
        let fans = p.get_fans().unwrap();

        let names: Vec<&str> = fans.iter().map(|f| f.name.as_str()).collect();
        assert_eq!(names, ["fan1", "fan2", "fan3", "fan4", "psu1_fan1", "psu2_fan1"]);

        assert!(fans[..4].iter().all(|f| f.kind == FanKind::Drawer));
        assert!(fans[4..].iter().all(|f| f.kind == FanKind::Psu));

        // fan3 is the first fan of drawer 2: absolute name, relative position.
        assert_eq!(fans[2].drawer_name, "drawer2");
        assert_eq!(fans[2].position_in_parent, 1);
        assert_eq!(fans[3].position_in_parent, 2);
    }

    /// A platform with no drawers has no drawer fans, and dividing by the
    /// drawer count must not be reached.
    #[test]
    fn a_platform_with_no_drawers_reports_only_psu_fans() {
        let d = tree(&[]);
        let p = platform_at(&d, 0, 0, 1);
        let fans = p.get_fans().unwrap();
        assert_eq!(fans.len(), 1);
        assert_eq!(fans[0].kind, FanKind::Psu);
        assert!(p.get_fan_drawers().unwrap().is_empty());
    }

    /// Each drawer's presence is read once and shared by its fans, because that
    /// is what a drawer fan's presence *is*.  Reading `fan{n}_status` with an
    /// absolute fan number would give the second fan of drawer 1 drawer 2's
    /// answer.
    #[test]
    fn a_drawers_fans_share_the_drawers_presence() {
        let d = tree(&[("thermal/fan1_status", "1\n"), ("thermal/fan2_status", "0\n")]);
        let p = platform_at(&d, 2, 4, 0);
        let fans = p.get_fans().unwrap();

        assert!(fans[0].presence && fans[1].presence, "both fans of drawer 1");
        assert!(!fans[2].presence && !fans[3].presence, "both fans of drawer 2");
    }

    /// `get_psus` walks the PSUs and then the PDBs, and the kind on each row is
    /// what separates the two key templates downstream.
    #[test]
    fn get_psus_walks_psus_then_pdbs() {
        let d = tree(&[]);
        let mut p = platform_at(&d, 0, 0, 2);
        p.pdb_count = 2;

        let rows = p.get_psus().unwrap();
        let names: Vec<&str> = rows.iter().map(|r| r.name.as_str()).collect();
        assert_eq!(names, ["PSU 1", "PSU 2", "PDB 1", "PDB 2"]);
        assert_eq!(rows[0].kind, PowerEntityKind::Psu);
        assert_eq!(rows[2].kind, PowerEntityKind::Pdb);
    }

    /// A virtual drawer is named `N/A` and has no LED, so the daemon is told
    /// so rather than driving a file that does not exist.
    #[test]
    fn a_fan_outside_a_named_drawer_has_no_led_to_set() {
        let d = tree(&[]);
        let mut p = platform_at(&d, 1, 1, 0);
        let err = p.set_fan_led("fan1", "N/A", "red").unwrap_err();
        assert!(matches!(err, PlatformError::NotSupported(_)), "{err}");
    }

    /// A drawer that has an LED but no capability entry is a simulated
    /// platform; the daemon is told, and does not treat it as a hard failure.
    #[test]
    fn a_drawer_with_no_led_capability_reports_not_supported() {
        let d = tree(&[]);
        let mut p = platform_at(&d, 1, 1, 0);
        let err = p.set_fan_led("fan1", "drawer1", "red").unwrap_err();
        assert!(matches!(err, PlatformError::NotSupported(_)), "{err}");
    }

    /// With a capability entry the write lands in the drawer's LED file.
    #[test]
    fn setting_a_fan_led_drives_its_drawers_file() {
        let d = tree(&[("led/led_fan1_capability", "green red\n")]);
        let mut p = platform_at(&d, 1, 1, 0);
        p.set_fan_led("fan1", "drawer1", "red").unwrap();
        assert_eq!(
            std::fs::read_to_string(d.path().join("led/led_fan1_red"))
                .unwrap()
                .trim(),
            "255"
        );
    }

    /// The PSU LED is one shared file on a hot-swappable platform, so a name
    /// that is not a PSU — a PDB — has nothing to drive.
    #[test]
    fn only_a_psu_has_a_writable_status_led() {
        let d = tree(&[("led/led_psu_capability", "green red\n")]);
        let mut p = platform_at(&d, 0, 0, 1);

        p.set_psu_led("PSU 1", "green").unwrap();
        assert_eq!(
            std::fs::read_to_string(d.path().join("led/led_psu_green"))
                .unwrap()
                .trim(),
            "255"
        );

        assert!(p.set_psu_led("PDB 1", "green").is_err());
    }

    /// A platform with no leak sensors is air-cooled, and the daemon's leak
    /// thread never starts.
    #[test]
    fn a_platform_without_leak_sensors_is_not_liquid_cooled() {
        let d = tree(&[]);
        let p = platform_at(&d, 0, 0, 0);
        let info = p.chassis_info().unwrap();
        assert!(!info.is_liquid_cooled);
        assert!(!info.is_modular_chassis);
        assert!(!info.is_dpu);
        assert_eq!(info.slot_or_dpu_id, None, "a fixed platform writes no slot table");
        assert!(p.get_leak_sensors().is_empty());
        assert!(p.get_leak_profiles().is_empty());
    }
}
