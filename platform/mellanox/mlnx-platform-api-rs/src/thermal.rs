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

//! Thermal sensor discovery and reading for Mellanox/NVIDIA chassis.
//!
//! Replicates Python's `thermal.py` — `THERMAL_NAMING_RULE` + the five
//! `create_*_thermal` helpers — producing an ordered `Vec<ThermalEntry>` that
//! must match the STATE_DB key order `show platform temperature` expects.

use platform_traits::{Threshold, ThermalInfo};

use crate::device_data::{
    ThermalCapability, get_asic_count, is_multi_asic,
    get_gearbox_count, get_sodimm_indices,
    get_psu_count, get_pdb_count,
};
use crate::utils::{self, HW_MGMT_THERMAL};

// ── ThermalEntry ──────────────────────────────────────────────────────────────

/// Static metadata for one thermal sensor, built at daemon startup.
///
/// All paths are absolute; `read_thermal()` reads them on each polling cycle.
#[derive(Debug, Clone)]
pub struct ThermalEntry {
    pub name: String,
    /// Absolute path to the temperature sysfs file.
    pub temp_path: String,
    /// Absolute path to the high-threshold sysfs file, if one exists.
    pub high_th_path: Option<String>,
    /// Absolute path to the high-critical-threshold sysfs file, if one exists.
    pub crit_th_path: Option<String>,
    /// Divide the raw sysfs integer by this value to get degrees Celsius.
    /// ASIC: 8 (sx_core reports units of 1/8 °C); others: 1000 (millidegrees).
    pub scale: f64,
    /// Fallback high threshold when the sysfs file is absent or zero.
    /// `Int` for ASIC defaults (writes `"105"` to STATE_DB), `Float` for files.
    pub high_th_default: Option<Threshold>,
    /// Fallback critical threshold.
    pub crit_th_default: Option<Threshold>,
    /// `"chassis 1"` for chassis-level thermals, `"PSU N"` for PSU thermals.
    pub parent_name: String,
    /// 1-based position within the parent.
    pub position: u32,
    /// Whether the component can be hot-swapped.
    pub is_replaceable: bool,
    /// If `Some`, read this file and skip the temperature when it is not `"1"`.
    /// Used for PSU thermals: `psuN_pwr_status`.
    pub presence_path: Option<String>,
}

// ── Sensor discovery ──────────────────────────────────────────────────────────

/// Build the ordered list of thermal entries for this platform.
///
/// The entry order exactly matches Python's `initialize_chassis_thermals()` +
/// PSU thermals.  STATE_DB keys in TEMPERATURE_INFO follow this order.
pub fn discover_thermals(cap: &ThermalCapability) -> Vec<ThermalEntry> {
    discover_thermals_with(cap, &DeviceCounts::from_sysfs())
}

/// How many of each device the platform has.
///
/// Every field is read from `/sys/module/sx_core` or hw-management's `config`
/// directory, both absolute paths — so a host without them reports zero of
/// everything and the discovery below produces almost nothing.  Gathering the
/// reads into one value lets a test state the platform it means and exercise
/// the discovery itself, which is what decides STATE_DB's key order.
#[derive(Debug, Clone, Default)]
pub struct DeviceCounts {
    pub asic: usize,
    /// Whether ASIC keys carry an index, i.e. `ASIC0` rather than `ASIC`.
    pub multi_asic: bool,
    pub gearbox: usize,
    /// The DIMM slots that are populated, which need not be contiguous.
    pub sodimm: Vec<usize>,
    pub pdb: usize,
    pub psu: usize,
}

impl DeviceCounts {
    pub fn from_sysfs() -> Self {
        Self {
            asic: get_asic_count(),
            multi_asic: is_multi_asic(),
            gearbox: get_gearbox_count(),
            sodimm: get_sodimm_indices(),
            pdb: get_pdb_count(),
            psu: get_psu_count(),
        }
    }
}

/// The body of [`discover_thermals`], with the device counts passed in.
///
/// The split exists so the tests do not depend on what the host running them
/// happens to have fitted.
pub fn discover_thermals_with(
    cap: &ThermalCapability,
    counts: &DeviceCounts,
) -> Vec<ThermalEntry> {
    let asic_count = counts.asic;
    let multi = counts.multi_asic;
    let mut entries: Vec<ThermalEntry> = Vec::new();
    let mut position: u32 = 1;

    // ── 1. ASIC thermals (asic_indexable) ────────────────────────────────────
    for asic_idx in 0..asic_count {
        let name = if multi {
            format!("ASIC{asic_idx}")
        } else {
            "ASIC".to_string()
        };
        let folder = format!("/sys/module/sx_core/asic{asic_idx}/temperature");
        let temp_path = format!("{folder}/input");
        entries.push(ThermalEntry {
            name,
            temp_path,
            high_th_path: None,
            crit_th_path: None,
            scale: 8.0,
            high_th_default: Some(Threshold::Int(cap.asic_warn_default)),
            crit_th_default: Some(Threshold::Int(cap.asic_crit_default)),
            parent_name: CHASSIS_PARENT.to_string(),
            position,
            is_replaceable: false,
            presence_path: None,
        });
        position += 1;
    }

    // ── 2. Ambient Port Side Temp ─────────────────────────────────────────────
    if cap.port_amb {
        entries.push(single(
            "Ambient Port Side Temp", "port_amb", None, None, position,
        ));
        position += 1;
    }

    // ── 3. Ambient Fan Side Temp ──────────────────────────────────────────────
    if cap.fan_amb {
        entries.push(single(
            "Ambient Fan Side Temp", "fan_amb", None, None, position,
        ));
        position += 1;
    }

    // ── 4. Ambient COMEX Temp ─────────────────────────────────────────────────
    if cap.comex_amb {
        entries.push(single(
            "Ambient COMEX Temp", "comex_amb", None, None, position,
        ));
        position += 1;
    }

    // ── 5. CPU Pack Temp ──────────────────────────────────────────────────────
    if cap.cpu_pack {
        entries.push(single(
            "CPU Pack Temp",
            "cpu_pack",
            Some("cpu_pack_max"),
            Some("cpu_pack_crit"),
            position,
        ));
        position += 1;
    }

    // No CPU Core entries: CPU thermal control is dead code in the Python
    // platform API, so publishing them would be a deviation, not a fix.

    // ── 7. Gearbox N Temp (indexable, start_index=1) ──────────────────────────
    let gb_count = counts.gearbox;
    for gb_idx in 0..gb_count {
        let idx = gb_idx + 1; // start_index=1
        entries.push(indexable(
            &format!("Gearbox {idx} Temp"),
            &format!("gearbox{idx}_temp_input"),
            Some(&format!("gearbox{idx}_temp_emergency")),
            Some(&format!("gearbox{idx}_temp_trip_crit")),
            position,
        ));
        position += 1;
    }

    // ── 8. Ambient CPU Board Temp ─────────────────────────────────────────────
    if cap.cpu_amb {
        entries.push(single(
            "Ambient CPU Board Temp", "cpu_amb", None, None, position,
        ));
        position += 1;
    }

    // ── 9. Ambient Switch Board Temp ──────────────────────────────────────────
    if cap.swb_amb {
        entries.push(single(
            "Ambient Switch Board Temp", "swb_amb", None, None, position,
        ));
        position += 1;
    }

    // ── 10. PCH Temp ─────────────────────────────────────────────────────────
    if cap.pch_temp {
        entries.push(single("PCH Temp", "pch_temp", None, None, position));
        position += 1;
    }

    // ── 11. SODIMM N Temp (discrete — enumerate existing files) ───────────────
    for &sodimm_idx in &counts.sodimm {
        // create_discrete_thermal passes `index - 1` to create_indexable_thermal
        // which then does `index + start_index(1)` = sodimm_idx again.
        entries.push(indexable(
            &format!("SODIMM {sodimm_idx} Temp"),
            &format!("sodimm{sodimm_idx}_temp_input"),
            Some(&format!("sodimm{sodimm_idx}_temp_max")),
            Some(&format!("sodimm{sodimm_idx}_temp_crit")),
            position,
        ));
        position += 1;
    }

    // ── 12. PSU N Temp (1-based, indexable, presence-checked per poll) ─────────
    let psu_count = counts.psu;
    for psu_num in 1..=psu_count {
        let psu_name = format!("PSU {psu_num}");
        let presence_path = format!("{HW_MGMT_THERMAL}/psu{psu_num}_pwr_status");
        entries.push(ThermalEntry {
            name: format!("PSU-{psu_num} Temp"),
            temp_path: format!("{HW_MGMT_THERMAL}/psu{psu_num}_temp1"),
            high_th_path: Some(format!("{HW_MGMT_THERMAL}/psu{psu_num}_temp1_max")),
            crit_th_path: None,
            scale: 1000.0,
            high_th_default: None,
            crit_th_default: None,
            parent_name: psu_name,
            position: 1, // position within PSU, always 1
            // A PSU can be pulled; its *sensor* is still not a replaceable unit,
            // and those are two different flags on this struct. Python publishes
            // this field from the thermal object, and `Thermal.is_replaceable()`
            // returns False (`thermal.py:412-418`). `RemovableThermal` -- what a
            // PSU's sensor actually is -- overrides only the three readers, to
            // gate them on presence (`thermal.py:421-466`); it does not override
            // this. So every Mellanox thermal reports False, PSU ones included.
            // `presence_path` below is what carries "can be pulled".
            is_replaceable: false,
            presence_path: Some(presence_path),
        });
        // PSU thermals are *not* included in the chassis position sequence
    }

    // ── 13. PDB-N Temp (1-based) ───────────────────────────────────────────────
    //
    // After the PSUs, because that is the order `thermalctld` collects in:
    // chassis thermals, then each PSU's, then each PDB's, then the modules'
    // (`thermalctld:1103`, `:1109`, `:1121`). This block used to sit above the
    // PSU one, which contradicted both that order and its own step number.
    //
    // Only the liquid-cooled platforms have PDBs.  The sensor is created only
    // when its input file exists, as Python does, and its parent is "PDB {n}"
    // while the sensor itself is "PDB-{n} Temp" - the two spellings differ.
    for pdb_num in 1..=counts.pdb {
        let temp_path = format!("{HW_MGMT_THERMAL}/pdb_hotswap{pdb_num}_temp1_input");
        if !utils::exists(&temp_path) {
            log::error!("PDB {pdb_num} temperature file {temp_path} does not exist");
            continue;
        }
        let high = format!("{HW_MGMT_THERMAL}/pdb_hotswap{pdb_num}_temp1_max");
        let crit = format!("{HW_MGMT_THERMAL}/pdb_hotswap{pdb_num}_temp1_crit");
        entries.push(ThermalEntry {
            name: format!("PDB-{pdb_num} Temp"),
            temp_path,
            high_th_path: utils::exists(&high).then_some(high),
            crit_th_path: utils::exists(&crit).then_some(crit),
            scale: 1000.0,
            high_th_default: None,
            crit_th_default: None,
            parent_name: format!("PDB {pdb_num}"),
            position: 1,
            is_replaceable: false,
            presence_path: None,
        });
        // Like PSU thermals, PDB thermals are not in the chassis position
        // sequence.
    }

    entries
}

// ── Read ──────────────────────────────────────────────────────────────────────

/// Read a thermal entry's current values.
///
/// `min_recorded` and `max_recorded` are always `None`.  Mellanox's Python
/// `Thermal` does not override `ThermalBase.get_minimum_recorded()` or
/// `get_maximum_recorded()` — neither name occurs anywhere in
/// `mlnx-platform-api` — so the base class raises `NotImplementedError`,
/// `thermalctld`'s `try_get` returns its `NOT_AVAILABLE` default and STATE_DB
/// carries `N/A` for both fields.  Tracking them here would make
/// `show platform temperature` print numbers where Python prints `N/A`.
///
/// `ThermalInfo` keeps the fields because other vendors do implement them
/// (Arista, Celestica, Micas, Ruijie and Ragile all define
/// `get_minimum_recorded`).
pub fn read_thermal(entry: &ThermalEntry) -> ThermalInfo {
    // Presence check for removable components (PSU).
    if let Some(pres_path) = &entry.presence_path {
        if utils::read_int(pres_path) != Some(1) {
            return ThermalInfo {
                name: entry.name.clone(),
                parent_name: entry.parent_name.clone(),
                position_in_parent: entry.position,
                temperature: None,
                high_threshold: entry.high_th_default,
                low_threshold: None,
                high_critical_threshold: entry.crit_th_default,
                low_critical_threshold: None,
                min_recorded: None,
                max_recorded: None,
                is_replaceable: entry.is_replaceable,
            };
        }
    }

    // Temperature: raw sysfs value / scale.  0 means "not ready" (like None).
    let temperature = utils::read_float(&entry.temp_path).and_then(|raw| {
        if raw == 0.0 { None } else { Some(raw / entry.scale) }
    });

    // High threshold: file (→ Float) or default (already typed).
    let high_threshold = read_threshold_file(&entry.high_th_path, entry.scale)
        .or(entry.high_th_default);

    // Critical threshold.
    let high_critical_threshold = read_threshold_file(&entry.crit_th_path, entry.scale)
        .or(entry.crit_th_default);

    ThermalInfo {
        name: entry.name.clone(),
        parent_name: entry.parent_name.clone(),
        position_in_parent: entry.position,
        temperature,
        high_threshold,
        low_threshold: None,
        high_critical_threshold,
        low_critical_threshold: None,
        min_recorded: None,
        max_recorded: None,
        is_replaceable: entry.is_replaceable,
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

const CHASSIS_PARENT: &str = "chassis 1";
const SCALE: f64 = 1000.0; // millidegrees → °C

fn single(name: &str, file: &str, high: Option<&str>, crit: Option<&str>, pos: u32) -> ThermalEntry {
    ThermalEntry {
        name:           name.to_string(),
        temp_path:      format!("{HW_MGMT_THERMAL}/{file}"),
        high_th_path:   high.map(|h| format!("{HW_MGMT_THERMAL}/{h}")),
        crit_th_path:   crit.map(|c| format!("{HW_MGMT_THERMAL}/{c}")),
        scale:          SCALE,
        high_th_default:  None,
        crit_th_default:  None,
        parent_name:    CHASSIS_PARENT.to_string(),
        position:       pos,
        is_replaceable: false,
        presence_path:  None,
    }
}

fn indexable(name: &str, file: &str, high: Option<&str>, crit: Option<&str>, pos: u32) -> ThermalEntry {
    // Identical to single() — separate name for readability.
    single(name, file, high, crit, pos)
}

/// Read a threshold file and return `Threshold::Float(v / scale)`.
/// Returns `None` when the file is absent, unreadable, or zero.
fn read_threshold_file(path: &Option<String>, scale: f64) -> Option<Threshold> {
    let p = path.as_deref()?;
    let raw = utils::read_float(p)?;
    if raw == 0.0 { return None; }
    Some(Threshold::Float(raw / scale))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::device_data::ThermalCapability;

    #[test]
    fn single_asic_platform_uses_plain_name() {
        let cap = ThermalCapability { comex_amb: false, ..Default::default() };
        let entries = discover_thermals_with(&cap, &DeviceCounts { asic: 1, ..Default::default() });
        assert_eq!(entries[0].name, "ASIC");
        assert_eq!(entries[0].scale, 8.0);
    }

    /// A multi-ASIC platform numbers them instead, and the count decides how
    /// many entries there are.  Reading this from the host's sx_core would make
    /// the result depend on the machine running the tests.
    #[test]
    fn a_multi_asic_platform_numbers_them() {
        let cap = ThermalCapability { comex_amb: false, ..Default::default() };
        let entries = discover_thermals_with(&cap, &DeviceCounts { asic: 2, multi_asic: true, ..Default::default() });
        assert_eq!(entries[0].name, "ASIC0");
        assert_eq!(entries[1].name, "ASIC1");
        assert_eq!(entries[0].position, 1);
        assert_eq!(entries[1].position, 2);
    }

    #[test]
    fn asic_defaults_are_int_typed() {
        let cap = ThermalCapability::default();
        let entries = discover_thermals_with(&cap, &DeviceCounts { asic: 1, ..Default::default() });
        assert!(matches!(entries[0].high_th_default, Some(Threshold::Int(105))));
        assert!(matches!(entries[0].crit_th_default, Some(Threshold::Int(120))));
    }

    #[test]
    fn comex_absent_when_capability_false() {
        let cap = ThermalCapability { comex_amb: false, ..Default::default() };
        let entries = discover_thermals_with(&cap, &DeviceCounts { asic: 1, ..Default::default() });
        assert!(!entries.iter().any(|e| e.name == "Ambient COMEX Temp"));
    }

    fn entry_at(temp_path: &str) -> ThermalEntry {
        ThermalEntry {
            name:           "Test".into(),
            temp_path:      temp_path.into(),
            high_th_path:   None,
            crit_th_path:   None,
            scale:          1.0,
            high_th_default: None,
            crit_th_default: None,
            parent_name:    CHASSIS_PARENT.into(),
            position:       1,
            is_replaceable: false,
            presence_path:  None,
        }
    }

    #[test]
    fn an_unreadable_sensor_has_no_temperature() {
        let info = read_thermal(&entry_at("/nonexistent"));
        assert!(info.temperature.is_none());
    }

    /// Mellanox does not override `get_minimum_recorded` / `get_maximum_recorded`,
    /// so Python reports `N/A` for both however many cycles have run.  Reading a
    /// real temperature must not start filling them in.
    #[test]
    fn min_and_max_recorded_stay_absent_like_python() {
        let dir = tempfile::tempdir().unwrap();
        let temp = dir.path().join("chassis_temp");
        std::fs::write(&temp, "42000\n").unwrap();

        let mut entry = entry_at(temp.to_str().unwrap());
        entry.scale = 1000.0;

        for _ in 0..3 {
            let info = read_thermal(&entry);
            assert_eq!(info.temperature, Some(42.0));
            assert!(info.min_recorded.is_none());
            assert!(info.max_recorded.is_none());
        }
    }

    /// The absent-device early return takes a separate path out of the function
    /// and must agree.
    #[test]
    fn an_absent_device_also_reports_no_min_or_max() {
        let dir = tempfile::tempdir().unwrap();
        let pres = dir.path().join("psu1_pwr_status");
        std::fs::write(&pres, "0\n").unwrap();

        let mut entry = entry_at("/nonexistent");
        entry.presence_path = Some(pres.to_str().unwrap().to_string());

        let info = read_thermal(&entry);
        assert!(info.temperature.is_none());
        assert!(info.min_recorded.is_none());
        assert!(info.max_recorded.is_none());
    }

    // ── The discovery order ───────────────────────────────────────────────

    fn names(entries: &[ThermalEntry]) -> Vec<&str> {
        entries.iter().map(|e| e.name.as_str()).collect()
    }

    fn full_platform() -> DeviceCounts {
        DeviceCounts {
            asic: 1,
            multi_asic: false,
            gearbox: 2,
            sodimm: vec![1, 2],
            pdb: 1,
            psu: 2,
        }
    }

    /// STATE_DB's TEMPERATURE_INFO keys come out in discovery order, so the
    /// order *is* the schema: ASICs, then the chassis sensors the capability
    /// allows, then gearboxes, DIMMs, PSUs and finally PDBs.  A reordering here
    /// is a visible change to `show platform temperature`.
    ///
    /// PSUs before PDBs is `thermalctld`'s own collection order -- chassis
    /// thermals, each PSU's, each PDB's, then the modules' (`thermalctld:1103`,
    /// `:1109`, `:1121`).
    #[test]
    fn the_entry_order_groups_devices_the_way_python_does() {
        let cap = ThermalCapability {
            comex_amb: true,
            cpu_pack: true,
            pch_temp: true,
            cpu_amb: true,
            swb_amb: true,
            ..Default::default()
        };
        let got = discover_thermals_with(&cap, &full_platform());
        let n = names(&got);

        let pos = |needle: &str| n.iter().position(|x| x.contains(needle));
        let asic = pos("ASIC").unwrap();
        let gearbox = pos("Gearbox").unwrap();
        let dimm = pos("SODIMM").unwrap();
        let psu = pos("PSU").unwrap();

        assert!(asic < gearbox, "{n:?}");
        assert!(gearbox < dimm, "{n:?}");
        assert!(dimm < psu, "{n:?}");
        // A PDB sensor is only created when its hw-management file exists, so
        // this arm is inert off a liquid-cooled device -- including in CI. It is
        // here so that where a PDB *is* discovered, the order is still checked.
        if let Some(pdb) = pos("PDB") {
            assert!(psu < pdb, "PSUs come before PDBs, as in thermalctld: {n:?}");
        }
    }

    /// A PDB sensor is created only where its input file already exists, which
    /// is what Python does (`pdb.py:57-64`) and why `counts.pdb` alone does not
    /// produce one.  The check reads an absolute hw-management path, so on a
    /// host without that tree — every build machine — the group is empty.
    #[test]
    fn a_pdb_sensor_needs_its_input_file_to_exist() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(
            &cap,
            &DeviceCounts { pdb: 4, ..Default::default() },
        );
        assert!(!names(&got).iter().any(|n| n.contains("PDB")));
    }

    /// `position` is per *parent*, not per chassis.  Chassis-parented sensors
    /// get a dense 1-based run — it is what PHYSICAL_ENTITY_INFO publishes, and
    /// a gap or a repeat would put two sensors at the same place in the tree —
    /// while each PSU's single sensor is position 1 within that PSU.  Numbering
    /// PSU sensors into the chassis run instead would give every one of them a
    /// position no consumer can resolve.
    #[test]
    fn positions_are_dense_within_each_parent() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(&cap, &full_platform());

        let chassis: Vec<u32> = got
            .iter()
            .filter(|e| e.parent_name == CHASSIS_PARENT)
            .map(|e| e.position)
            .collect();
        assert_eq!(chassis, (1..=chassis.len() as u32).collect::<Vec<_>>());

        for e in got.iter().filter(|e| e.parent_name != CHASSIS_PARENT) {
            assert_eq!(e.position, 1, "{} under {}", e.name, e.parent_name);
        }
    }

    /// A PSU's sensor is parented to the PSU, not to the chassis, which is what
    /// keeps its position meaningful.
    #[test]
    fn a_psu_sensor_is_parented_to_its_psu() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(&cap, &DeviceCounts { psu: 2, ..Default::default() });
        let parents: Vec<&str> = got
            .iter()
            .filter(|e| e.name.contains("PSU"))
            .map(|e| e.parent_name.as_str())
            .collect();
        assert_eq!(parents, ["PSU 1", "PSU 2"]);
    }

    /// Every device count scales its own group and nothing else.
    #[test]
    fn each_count_adds_only_its_own_entries() {
        let cap = ThermalCapability::default();
        let base = discover_thermals_with(&cap, &DeviceCounts::default()).len();

        let one_psu = DeviceCounts { psu: 1, ..Default::default() };
        let two_psu = DeviceCounts { psu: 2, ..Default::default() };
        assert_eq!(
            discover_thermals_with(&cap, &two_psu).len()
                - discover_thermals_with(&cap, &one_psu).len(),
            discover_thermals_with(&cap, &one_psu).len() - base,
            "each PSU contributes the same number of sensors"
        );
    }

    /// A DIMM slot list need not be contiguous — an unpopulated slot 1 with a
    /// populated slot 2 is a real configuration, and numbering the entries from
    /// the loop counter instead of the slot would mislabel it.
    #[test]
    fn sodimm_entries_follow_the_populated_slots_not_a_counter() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(
            &cap,
            &DeviceCounts { sodimm: vec![2, 4], ..Default::default() },
        );
        let dimms: Vec<&str> =
            names(&got).into_iter().filter(|n| n.to_uppercase().contains("DIMM")).collect();
        assert_eq!(dimms.len(), 2, "{dimms:?}");
        assert!(dimms.iter().any(|d| d.contains('2')), "{dimms:?}");
        assert!(dimms.iter().any(|d| d.contains('4')), "{dimms:?}");
    }

    /// A PSU sensor is removable, so it carries a presence file; a chassis
    /// sensor does not, and reading one for it would report every chassis
    /// sensor absent.
    #[test]
    fn only_psu_sensors_carry_a_presence_file() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(
            &cap,
            &DeviceCounts { asic: 1, psu: 1, ..Default::default() },
        );
        for e in &got {
            if e.name.contains("PSU") {
                assert!(e.presence_path.is_some(), "{}", e.name);
            } else {
                assert!(e.presence_path.is_none(), "{}", e.name);
            }
        }
    }

    /// No thermal is replaceable, including a PSU's. Removable and replaceable
    /// are different questions and this struct carries them separately:
    /// `presence_path` answers the first, this field the second. Python answers
    /// the second from the sensor -- `Thermal.is_replaceable()` returns False
    /// and `RemovableThermal` overrides only the presence-gated readers -- so
    /// reporting True here offers an operator a part that does not exist.
    #[test]
    fn no_thermal_is_replaceable_not_even_a_psus() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(
            &cap,
            &DeviceCounts { asic: 1, psu: 4, pdb: 1, ..Default::default() },
        );
        assert!(got.iter().any(|e| e.name.contains("PSU")), "no PSU sensor discovered");
        for e in &got {
            assert!(!e.is_replaceable, "{} reports replaceable", e.name);
        }
    }

    /// A platform with nothing fitted still discovers its ASICs and whatever
    /// the capability allows, and nothing else.
    #[test]
    fn an_empty_platform_discovers_no_device_sensors() {
        let cap = ThermalCapability::default();
        let got = discover_thermals_with(&cap, &DeviceCounts { asic: 1, ..Default::default() });
        let n = names(&got);
        assert!(n.iter().any(|x| x.contains("ASIC")), "{n:?}");
        assert!(!n.iter().any(|x| x.contains("PSU")), "{n:?}");
        assert!(!n.iter().any(|x| x.contains("PDB")), "{n:?}");
        assert!(!n.iter().any(|x| x.contains("Gearbox")), "{n:?}");
    }
}
