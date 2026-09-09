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

//! SmartSwitch DPU thermal data.  Ports `smartswitch_thermal_updater.py`.
//!
//! On a SmartSwitch the platform API feeds hw-management each DPU's CPU, DDR
//! and drive temperatures alongside the ASIC and module feed.  The values come
//! from `CHASSIS_STATE_DB`, where each DPU publishes
//! `TEMPERATURE_INFO_{dpu_id}|{CPU,DDR,NVME}`; they are written out through the
//! three set/clear pairs in `hw_management_rs`.
//!
//! Three details are easy to get wrong:
//!
//! * **The ids differ by one.** `TEMPERATURE_INFO_{dpu_id}` is 0-based, and
//!   hw-management's `dpu{n}` is `dpu_id + 1`.
//! * **An offline DPU is cleared, not left stale**, and only on the transition,
//!   so hw-management-tc does not keep reacting to a temperature nothing is
//!   refreshing.
//! * **A missing or unparsable field is a fault, not a zero.** The value
//!   written is 0 but the fault code goes with it, so tc takes its error path
//!   rather than believing a plausible reading.

use std::collections::HashMap;

use hw_management_rs::{DpuSensor, HwMgmt};
use swss_common::{DbConnector, Table};

use crate::utils;

/// Python's `HW_BASE` for the DPU boot-progress files.
const HW_BASE: &str = "/var/run/hw-management";
/// `BootProgEnum.OS_RUN`: the only value that counts as online.
const BOOT_PROG_OS_RUN: i64 = 5;
/// Python's `ERROR_READ_THERMAL_DATA`.
const ERROR_READ_THERMAL_DATA: i64 = 254_000;
/// Default DPU poll interval when `tc_config.json` carries none.
pub const DEFAULT_DPU_POLL_SECS: f64 = 3.0;

const CHASSIS_STATE_DB: &str = "CHASSIS_STATE_DB";

/// The three hashes a DPU publishes, and the hw-management sensor each feeds.
const FIELDS: [(&str, DpuSensor); 3] = [
    ("CPU", DpuSensor::CpuCore),
    ("DDR", DpuSensor::Ddr),
    ("NVME", DpuSensor::Drive),
];

/// Whether this platform has DPUs, from `platform.json`'s `DPUS` key.
pub fn dpu_ids(platform_json: &str) -> Vec<u32> {
    let Ok(text) = std::fs::read_to_string(platform_json) else {
        return Vec::new();
    };
    let Ok(root) = serde_json::from_str::<serde_json::Value>(&text) else {
        return Vec::new();
    };
    let Some(dpus) = root.get("DPUS") else {
        return Vec::new();
    };
    // The key is an object keyed by DPU name in the platforms that have it, and
    // the id is the trailing number of that name.
    match dpus {
        serde_json::Value::Object(map) => {
            let mut ids: Vec<u32> = map
                .keys()
                .filter_map(|k| k.trim_start_matches(|c: char| !c.is_ascii_digit()).parse().ok())
                .collect();
            ids.sort_unstable();
            ids
        }
        serde_json::Value::Array(items) => (0..items.len() as u32).collect(),
        _ => Vec::new(),
    }
}

/// `dpu{id+1}/system/boot_progress` reaching `OS_RUN` is what "online" means.
///
/// The index is hw-management's, not the DPU id: Python builds this path from
/// `get_hwmgmt_name()`, which is `dpu{id+1}` (`dpuctlplat.py:254-256`,
/// `:122-123`).  Reading `dpu0` finds no file, and every DPU then looks
/// permanently offline.
pub fn is_online_at(base: &str, dpu_id: u32) -> bool {
    utils::read_int(&boot_progress_path(base, dpu_id)) == Some(BOOT_PROG_OS_RUN)
}

fn boot_progress_path(base: &str, dpu_id: u32) -> String {
    format!("{base}/dpu{}/system/boot_progress", dpu_id + 1)
}

/// One component's reading, as `CHASSIS_STATE_DB` carries it.
#[derive(Debug, Default, Clone, PartialEq)]
pub struct Reading {
    pub temperature: i64,
    pub high_threshold: i64,
    pub critical_high_threshold: i64,
    /// `ERROR_READ_THERMAL_DATA` when any of the three was missing or
    /// unparsable, else 0.
    pub fault: i64,
}

/// Parse one component's hash.  Python treats a missing *or* unparsable value
/// as a fault and writes 0 for it.
pub fn parse_reading(fields: &HashMap<String, String>) -> Reading {
    let mut fault = false;
    let mut get = |name: &str| -> i64 {
        match fields.get(name).map(|s| s.trim()) {
            Some(s) if !s.is_empty() => match s.parse::<f64>() {
                Ok(v) => v as i64,
                Err(_) => {
                    log::error!("Unable to obtain temperature data for DPU {name}: {s}");
                    fault = true;
                    0
                }
            },
            _ => {
                fault = true;
                0
            }
        }
    };
    let temperature = get("temperature");
    let high_threshold = get("high_threshold");
    let critical_high_threshold = get("critical_high_threshold");
    Reading {
        temperature,
        high_threshold,
        critical_high_threshold,
        fault: if fault { ERROR_READ_THERMAL_DATA } else { 0 },
    }
}

/// Feeds hw-management from the DPU tables, tracking each DPU's presence.
pub struct DpuUpdater {
    hw: HwMgmt,
    ids: Vec<u32>,
    tables: HashMap<u32, Box<dyn RowReader>>,
    online: HashMap<u32, bool>,
    /// Root the `boot_progress` files are read under.
    hw_base: String,
}

impl DpuUpdater {
    /// Opens one `CHASSIS_STATE_DB` handle per DPU and clears every DPU's
    /// thermal data, as Python's `start()` does before scheduling.
    pub fn new(ids: Vec<u32>) -> Self {
        let hw = HwMgmt::new();
        let mut tables = HashMap::new();
        for &id in &ids {
            match DbConnector::new_named(CHASSIS_STATE_DB, false, 0)
                .and_then(|c| Table::new(c, &format!("TEMPERATURE_INFO_{id}")))
            {
                Ok(t) => {
                    tables.insert(id, Box::new(t) as Box<dyn RowReader>);
                }
                Err(e) => log::warn!("DPU {id}: cannot open {CHASSIS_STATE_DB}: {e:?}"),
            }
            clear_all(&hw, id);
        }
        Self {
            hw,
            ids,
            tables,
            online: HashMap::new(),
            hw_base: HW_BASE.to_string(),
        }
    }

    /// The same updater with its two roots supplied.
    ///
    /// `new()` opens a redis handle per DPU and reads `boot_progress` from an
    /// absolute path; both are given here instead, which is what lets the
    /// online/offline transitions below be driven.
    #[cfg(test)]
    pub fn with_parts(hw: HwMgmt, ids: Vec<u32>, tables: HashMap<u32, Box<dyn RowReader>>, hw_base: &str) -> Self {
        for &id in &ids {
            clear_all(&hw, id);
        }
        Self {
            hw,
            ids,
            tables,
            online: HashMap::new(),
            hw_base: hw_base.to_string(),
        }
    }

    /// One pass over every DPU.
    pub fn update(&mut self) {
        for &id in &self.ids {
            let online = is_online_at(&self.hw_base, id);
            let was = self.online.insert(id, online);

            if online {
                for (field, sensor) in FIELDS {
                    let fields = self.tables.get(&id).and_then(|t| t.row(field)).unwrap_or_default();
                    let r = parse_reading(&fields);
                    // hw-management's dpu index is one past the DPU id.
                    self.hw.thermal_data_dpu_set(
                        sensor,
                        id as i64 + 1,
                        &r.temperature.to_string(),
                        Some(&r.high_threshold.to_string()),
                        Some(&r.critical_high_threshold.to_string()),
                        &r.fault.to_string(),
                    );
                }
            } else if was != Some(false) {
                // Only on the transition to offline, so a powered-down DPU is
                // cleared once rather than every cycle.
                clear_all(&self.hw, id);
            }
        }
    }
}

fn clear_all(hw: &HwMgmt, dpu_id: u32) {
    for (_, sensor) in FIELDS {
        hw.thermal_data_dpu_clear(sensor, dpu_id as i64 + 1);
    }
}

/// One whole row of `CHASSIS_STATE_DB`, which is what a DPU reading is.
///
/// Declared here rather than reusing `thermal_updater`'s single-field reader:
/// the DPU path wants every field of a key at once, and a per-field trait would
/// turn one round trip into four.  Local trait, foreign type — so it can be
/// implemented for `swss_common::Table` without touching `platform-traits`.
pub trait RowReader {
    fn row(&self, key: &str) -> Option<HashMap<String, String>>;
}

impl RowReader for Table {
    fn row(&self, key: &str) -> Option<HashMap<String, String>> {
        let fvs = self.get(key).ok()??;
        Some(
            fvs.into_iter()
                .filter_map(|(k, v)| v.to_str().ok().map(|s| (k, s.to_string())))
                .collect(),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    fn json(body: &str) -> tempfile::NamedTempFile {
        let mut f = tempfile::NamedTempFile::new().unwrap();
        f.write_all(body.as_bytes()).unwrap();
        f
    }

    fn fields(pairs: &[(&str, &str)]) -> HashMap<String, String> {
        pairs.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect()
    }

    #[test]
    fn a_platform_without_dpus_has_none() {
        let f = json(r#"{"chassis": {}}"#);
        assert!(dpu_ids(f.path().to_str().unwrap()).is_empty());
        assert!(dpu_ids("/nonexistent/platform.json").is_empty());
    }

    #[test]
    fn dpu_ids_come_from_the_key_names() {
        let f = json(r#"{"DPUS": {"dpu0": {}, "dpu2": {}, "dpu1": {}}}"#);
        assert_eq!(dpu_ids(f.path().to_str().unwrap()), vec![0, 1, 2]);
    }

    #[test]
    fn a_complete_reading_carries_no_fault() {
        let r = parse_reading(&fields(&[
            ("temperature", "45.5"),
            ("high_threshold", "90"),
            ("critical_high_threshold", "100"),
        ]));
        assert_eq!(
            r,
            Reading {
                temperature: 45,
                high_threshold: 90,
                critical_high_threshold: 100,
                fault: 0
            }
        );
    }

    /// A missing field is a fault, and the value written is 0 — not a plausible
    /// temperature tc would believe.
    #[test]
    fn a_missing_field_is_a_fault() {
        let r = parse_reading(&fields(&[("temperature", "45")]));
        assert_eq!(r.temperature, 45);
        assert_eq!(r.high_threshold, 0);
        assert_eq!(r.fault, ERROR_READ_THERMAL_DATA);
    }

    #[test]
    fn an_unparsable_field_is_a_fault_too() {
        let r = parse_reading(&fields(&[
            ("temperature", "N/A"),
            ("high_threshold", "90"),
            ("critical_high_threshold", "100"),
        ]));
        assert_eq!(r.temperature, 0);
        assert_eq!(r.fault, ERROR_READ_THERMAL_DATA);
    }

    #[test]
    fn an_empty_reading_is_all_fault() {
        let r = parse_reading(&HashMap::new());
        assert_eq!(
            r,
            Reading {
                temperature: 0,
                high_threshold: 0,
                critical_high_threshold: 0,
                fault: ERROR_READ_THERMAL_DATA
            }
        );
    }

    /// The same off-by-one governs the file that decides whether a DPU is
    /// online.  Reading `dpu0` finds nothing and the DPU looks permanently
    /// offline, which is silent: its thermals are simply never fed.
    #[test]
    fn boot_progress_is_read_from_the_hw_management_index() {
        assert_eq!(
            boot_progress_path(HW_BASE, 0),
            "/var/run/hw-management/dpu1/system/boot_progress"
        );
        assert_eq!(
            boot_progress_path(HW_BASE, 3),
            "/var/run/hw-management/dpu4/system/boot_progress"
        );
    }

    /// The DPU id and hw-management's index differ by one; writing dpu0 would
    /// land outside the range hw-management accepts.
    #[test]
    fn the_hw_management_index_is_one_past_the_dpu_id() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("config")).unwrap();
        std::fs::write(dir.path().join("config/dpu_num"), "2").unwrap();
        let hw = HwMgmt::with_base(dir.path());
        // DPU id 0 -> dpu1, which is in range 1..=2.
        let dpu_id: i64 = 0;
        assert!(hw.thermal_data_dpu_set(DpuSensor::CpuCore, dpu_id + 1, "45", None, None, "0"));
        assert!(dir.path().join("dpu1/thermal/cpu_pack").exists());
    }

    // ── DpuUpdater ────────────────────────────────────────────────────────

    /// A row store standing in for `CHASSIS_STATE_DB`.
    #[derive(Default)]
    struct FakeRows(HashMap<String, HashMap<String, String>>);

    impl FakeRows {
        fn with(key: &str, pairs: &[(&str, &str)]) -> Self {
            let mut m = HashMap::new();
            m.insert(key.to_string(), fields(pairs));
            Self(m)
        }
    }

    impl RowReader for FakeRows {
        fn row(&self, key: &str) -> Option<HashMap<String, String>> {
            self.0.get(key).cloned()
        }
    }

    /// A hw-management tree with the DPU counter its index checks need.
    fn dpu_tree(dpus: usize) -> (tempfile::TempDir, HwMgmt) {
        let dir = tempfile::tempdir().unwrap();
        for sub in ["config", "thermal"] {
            std::fs::create_dir_all(dir.path().join(sub)).unwrap();
        }
        std::fs::write(dir.path().join("config/dpu_num"), dpus.to_string()).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        (dir, hw)
    }

    /// Write the boot-progress file for a DPU, under hw-management's index.
    fn set_boot_progress(root: &std::path::Path, dpu_id: u32, value: i64) {
        let d = root.join(format!("dpu{}/system", dpu_id + 1));
        std::fs::create_dir_all(&d).unwrap();
        std::fs::write(d.join("boot_progress"), value.to_string()).unwrap();
    }

    /// A DPU's files live under `{base}/dpu{n}/thermal/`, indexed by
    /// hw-management's number rather than the DPU id.
    fn dpu_file(dir: &tempfile::TempDir, dpu_index: u32, name: &str) -> Option<String> {
        std::fs::read_to_string(dir.path().join(format!("dpu{dpu_index}/thermal")).join(name))
            .ok()
            .map(|s| s.trim().to_string())
    }

    #[test]
    fn a_dpu_is_online_only_at_os_run() {
        let root = tempfile::tempdir().unwrap();
        let base = root.path().to_string_lossy().into_owned();
        assert!(!is_online_at(&base, 0), "no file at all");

        set_boot_progress(root.path(), 0, BOOT_PROG_OS_RUN - 1);
        assert!(!is_online_at(&base, 0), "still booting");

        set_boot_progress(root.path(), 0, BOOT_PROG_OS_RUN);
        assert!(is_online_at(&base, 0));
    }

    /// An online DPU's readings reach hw-management under the index one past
    /// its id, which is the off-by-one that made every DPU look offline for the
    /// whole of development.
    #[test]
    fn an_online_dpu_is_fed_under_the_hw_management_index() {
        let (dir, hw) = dpu_tree(2);
        let root = tempfile::tempdir().unwrap();
        set_boot_progress(root.path(), 0, BOOT_PROG_OS_RUN);

        let mut tables: HashMap<u32, Box<dyn RowReader>> = HashMap::new();
        tables.insert(
            0,
            Box::new(FakeRows::with(
                FIELDS[0].0,
                &[
                    ("temperature", "45.0"),
                    ("high_threshold", "70.0"),
                    ("critical_high_threshold", "80.0"),
                ],
            )),
        );

        let mut u = DpuUpdater::with_parts(hw, vec![0], tables, &root.path().to_string_lossy());
        u.update();

        // hw-management index 1 for DPU id 0.  The values are degrees, not
        // millidegrees: the DPU feed passes `int(float(v))` straight through
        // (`smartswitch_thermal_updater.py:93-105`), unlike the ASIC and module
        // feeds which scale by 1000.  Scaling here would report every DPU a
        // thousand times too hot.
        assert_eq!(dpu_file(&dir, 1, "cpu_pack").as_deref(), Some("45"), "under dpu1");
        assert_eq!(dpu_file(&dir, 1, "cpu_pack_max").as_deref(), Some("70"));
        assert_eq!(dpu_file(&dir, 1, "cpu_pack_crit").as_deref(), Some("80"));
        assert_eq!(dpu_file(&dir, 1, "cpu_pack_fault").as_deref(), Some("0"));
        assert!(dpu_file(&dir, 0, "cpu_pack").is_none(), "never under dpu0");
    }

    /// An offline DPU is cleared once, on the transition, and not on every
    /// cycle after it — the same rule the module feed follows.
    #[test]
    fn an_offline_dpu_is_cleared_once_and_not_every_cycle() {
        let (dir, hw) = dpu_tree(1);
        let root = tempfile::tempdir().unwrap();
        set_boot_progress(root.path(), 0, BOOT_PROG_OS_RUN);

        let mut tables: HashMap<u32, Box<dyn RowReader>> = HashMap::new();
        tables.insert(0, Box::new(FakeRows::with(FIELDS[0].0, &[("temperature", "45.0")])));
        let mut u = DpuUpdater::with_parts(hw, vec![0], tables, &root.path().to_string_lossy());

        u.update();
        assert!(dpu_file(&dir, 1, "cpu_pack").is_some());

        // The DPU powers down.
        std::fs::remove_file(root.path().join("dpu1/system/boot_progress")).unwrap();
        u.update();
        assert!(
            dpu_file(&dir, 1, "cpu_pack").is_none(),
            "the transition clears the file"
        );

        // A second offline pass must not write anything back.
        u.update();
        assert!(dpu_file(&dir, 1, "cpu_pack").is_none());
    }

    /// A DPU whose table was never opened still gets fed, with the all-fault
    /// reading — a missing handle is not a reason to leave tc without a value.
    #[test]
    fn a_dpu_with_no_table_is_fed_the_fault_reading() {
        let (dir, hw) = dpu_tree(1);
        let root = tempfile::tempdir().unwrap();
        set_boot_progress(root.path(), 0, BOOT_PROG_OS_RUN);

        let mut u = DpuUpdater::with_parts(hw, vec![0], HashMap::new(), &root.path().to_string_lossy());
        u.update();

        assert_eq!(dpu_file(&dir, 1, "cpu_pack").as_deref(), Some("0"));
        assert_eq!(
            dpu_file(&dir, 1, "cpu_pack_fault").as_deref(),
            Some(ERROR_READ_THERMAL_DATA.to_string().as_str())
        );
    }

    /// `DPUS` may be a list rather than an object, in which case the ids are
    /// its positions.  A platform.json that uses the list form would otherwise
    /// report no DPUs and its thermals would never be fed.
    #[test]
    fn a_dpus_list_is_indexed_by_position() {
        let f = json(r#"{"DPUS": ["dpu0", "dpu1", "dpu2"]}"#);
        assert_eq!(dpu_ids(f.path().to_str().unwrap()), vec![0, 1, 2]);
    }

    /// Anything else — a string, a number, no key at all — is no DPUs.
    #[test]
    fn an_unrecognisable_dpus_value_is_no_dpus() {
        for body in [r#"{"DPUS": "two"}"#, r#"{"DPUS": 2}"#, r#"{}"#, "not json"] {
            let f = json(body);
            assert!(dpu_ids(f.path().to_str().unwrap()).is_empty(), "{body}");
        }
        assert!(dpu_ids("/nonexistent/platform.json").is_empty());
    }
}
