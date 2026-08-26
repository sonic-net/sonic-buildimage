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

//! Rust port of hw-management's thermal write modules.
//!
//! hw-management publishes its file layout to one language only:
//! `hw_management_independent_mode_update` and `hw_management_dpu_thermal_update`
//! are Python modules with no C library beneath them, so a Rust platform API has
//! nothing to bind to.  This crate reimplements exactly what the Python platform
//! API calls, and nothing else.
//!
//! It is a separate crate so that handing it to hw-management is a directory
//! move and a dependency line, not a rewrite.  Function and parameter names are
//! deliberately identical to the Python originals.
//!
//! # The threshold trap
//!
//! [`HwMgmt::thermal_data_set_asic`] and [`HwMgmt::thermal_data_set_module`] map
//! their parameters *straight through*: `_temp_crit` takes `critical_threshold`
//! and `_temp_emergency` takes `warning_threshold`.  The Python caller in
//! `mlnx-platform-api` swaps them, so on a running system `_temp_crit` holds the
//! warning value and `_temp_emergency` the critical one.  Do not "fix" either
//! side alone.

use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};

/// Python's `BASE_PATH`.  `/var/run` and `/run` are the same mount point on
/// every kernel SONiC supports.
pub const DEFAULT_BASE_PATH: &str = "/var/run/hw-management";

/// The hw-management tree this instance writes to.
///
/// The root is a field rather than a constant so that tests can point it at a
/// temporary directory; production callers use [`HwMgmt::new`].
#[derive(Debug, Clone)]
pub struct HwMgmt {
    base: PathBuf,
}

impl Default for HwMgmt {
    fn default() -> Self {
        Self::new()
    }
}

impl HwMgmt {
    /// The real tree at [`DEFAULT_BASE_PATH`].
    pub fn new() -> Self {
        Self { base: PathBuf::from(DEFAULT_BASE_PATH) }
    }

    /// A tree rooted anywhere — for tests.
    pub fn with_base<P: AsRef<Path>>(base: P) -> Self {
        Self { base: base.as_ref().to_path_buf() }
    }

    pub fn base(&self) -> &Path {
        &self.base
    }

    // ── Python: get_asic_count / get_module_count ────────────────────────────

    /// `{base}/config/asic_num`, or `None` when absent or unparsable.
    pub fn get_asic_count(&self) -> Option<i64> {
        read_int(&self.base.join("config/asic_num"))
    }

    /// `{base}/config/module_counter`, or `None` when absent or unparsable.
    pub fn get_module_count(&self) -> Option<i64> {
        read_int(&self.base.join("config/module_counter"))
    }

    /// Python: `0 <= asic_index < get_asic_count()`.
    fn check_asic_index(&self, asic_index: i64) -> bool {
        match self.get_asic_count() {
            Some(n) if asic_index >= 0 && asic_index < n => true,
            _ => {
                log::warn!("asic_index {asic_index} is out of bound 0..ASIC");
                false
            }
        }
    }

    /// Python: `1 <= module_index <= get_module_count()`.  Note the 1-based
    /// range, which differs from the ASIC check above.
    fn check_module_index(&self, asic_index: i64, module_index: i64) -> bool {
        match self.get_module_count() {
            Some(n) if module_index >= 1 && module_index <= n => true,
            _ => {
                log::warn!("module_index {module_index} of asic {asic_index} is out of bound 1..n");
                false
            }
        }
    }

    // ── Python: module_data_set_module_counter ───────────────────────────────

    /// Writes `{base}/config/module_counter`.
    ///
    /// Unlike the thermal writes this one appends no newline, matching Python.
    pub fn module_data_set_module_counter(&self, module_counter: i64) -> bool {
        if module_counter < 0 {
            log::error!("Could not set module count to {module_counter}");
            return false;
        }
        let path = self.base.join("config/module_counter");
        match fs::write(&path, module_counter.to_string()) {
            Ok(()) => true,
            Err(e) => {
                log::error!("Error setting module counter: {e}");
                false
            }
        }
    }

    // ── Python: write_file_data / remove_file_list ───────────────────────────

    /// Writes `{base}/thermal/{name}` for each entry, with a trailing newline.
    fn write_thermal(&self, files: &BTreeMap<String, String>) -> bool {
        let dir = self.base.join("thermal");
        let mut ok = true;
        for (name, value) in files {
            if let Err(e) = fs::write(dir.join(name), format!("{value}\n")) {
                log::error!("Error writing thermal data to {name}: {e}");
                ok = false;
            }
        }
        ok
    }

    /// Removes `{base}/thermal/{name}` for each name that exists.
    fn remove_thermal(&self, names: &[String]) -> bool {
        let dir = self.base.join("thermal");
        let mut ok = true;
        for name in names {
            let path = dir.join(name);
            if path.exists() {
                if let Err(e) = fs::remove_file(&path) {
                    log::error!("Error removing {name}: {e}");
                    ok = false;
                }
            }
        }
        ok
    }

    // ── Python: thermal_data_set_asic / _clean_asic ──────────────────────────

    /// Writes one ASIC's temperature and thresholds.
    ///
    /// `asic_index` 0 additionally writes the un-indexed `asic*` aliases, and
    /// every index writes `asic{asic_index + 1}*`.
    ///
    /// See the threshold trap in the module docs: `_temp_crit` takes
    /// `critical_threshold`, `_temp_emergency` takes `warning_threshold`.
    pub fn thermal_data_set_asic(
        &self,
        asic_index: i64,
        temperature: &str,
        warning_threshold: &str,
        critical_threshold: &str,
        fault: &str,
    ) -> bool {
        if !self.check_asic_index(asic_index) {
            return false;
        }
        let mut files = BTreeMap::new();
        if asic_index == 0 {
            files.insert("asic_temp_crit".into(), critical_threshold.to_string());
            files.insert("asic".into(), temperature.to_string());
            files.insert("asic_temp_emergency".into(), warning_threshold.to_string());
            files.insert("asic_temp_fault".into(), fault.to_string());
        }
        let n = asic_index + 1;
        files.insert(format!("asic{n}_temp_crit"), critical_threshold.to_string());
        files.insert(format!("asic{n}"), temperature.to_string());
        files.insert(format!("asic{n}_temp_emergency"), warning_threshold.to_string());
        files.insert(format!("asic{n}_temp_fault"), fault.to_string());
        self.write_thermal(&files)
    }

    /// Removes the files [`Self::thermal_data_set_asic`] writes.
    pub fn thermal_data_clean_asic(&self, asic_index: i64) -> bool {
        if !self.check_asic_index(asic_index) {
            return false;
        }
        let n = asic_index + 1;
        let mut names = vec![
            format!("asic{n}_temp_crit"),
            format!("asic{n}"),
            format!("asic{n}_temp_emergency"),
            format!("asic{n}_temp_fault"),
        ];
        if asic_index == 0 {
            names.extend([
                "asic_temp_crit".to_string(),
                "asic".to_string(),
                "asic_temp_emergency".to_string(),
                "asic_temp_fault".to_string(),
            ]);
        }
        self.remove_thermal(&names)
    }

    // ── Python: thermal_data_set_module / _clean_module ──────────────────────

    /// Writes one transceiver module's temperature and thresholds.  Same
    /// threshold trap as [`Self::thermal_data_set_asic`].
    pub fn thermal_data_set_module(
        &self,
        asic_index: i64,
        module_index: i64,
        temperature: &str,
        warning_threshold: &str,
        critical_threshold: &str,
        fault: &str,
    ) -> bool {
        if !self.check_asic_index(asic_index) || !self.check_module_index(asic_index, module_index) {
            return false;
        }
        let m = module_index;
        let mut files = BTreeMap::new();
        files.insert(format!("module{m}_temp_crit"), critical_threshold.to_string());
        files.insert(format!("module{m}_temp_input"), temperature.to_string());
        files.insert(format!("module{m}_temp_emergency"), warning_threshold.to_string());
        files.insert(format!("module{m}_temp_fault"), fault.to_string());
        self.write_thermal(&files)
    }

    /// Removes the files [`Self::thermal_data_set_module`] writes.
    pub fn thermal_data_clean_module(&self, asic_index: i64, module_index: i64) -> bool {
        if !self.check_asic_index(asic_index) || !self.check_module_index(asic_index, module_index) {
            return false;
        }
        let m = module_index;
        self.remove_thermal(&[
            format!("module{m}_temp_crit"),
            format!("module{m}_temp_input"),
            format!("module{m}_temp_emergency"),
            format!("module{m}_temp_fault"),
        ])
    }

    // ── Python: vendor_data_set_module / vendor_data_clear_module ────────────

    /// Writes `{base}/eeprom/module{n}_data`, one `key: value` line per entry,
    /// the key left-padded to 25 columns.  `part_number` and `manufacturer` are
    /// renamed to `PN` and `Manufacturer`; the match is case-insensitive and any
    /// other key is written through unchanged.
    ///
    /// An empty `vendor_info` removes the file, as Python's `None` does.
    pub fn vendor_data_set_module(
        &self,
        asic_index: i64,
        module_index: i64,
        vendor_info: &[(String, String)],
    ) -> bool {
        if !self.check_asic_index(asic_index) || !self.check_module_index(asic_index, module_index) {
            return false;
        }
        let path = self.base.join(format!("eeprom/module{module_index}_data"));
        if vendor_info.is_empty() {
            return remove_if_exists(&path);
        }
        let mut lines = Vec::with_capacity(vendor_info.len());
        for (key, value) in vendor_info {
            let name = match key.to_lowercase().as_str() {
                "part_number" => "PN",
                "manufacturer" => "Manufacturer",
                _ => key.as_str(),
            };
            lines.push(format!("{name:<25}: {value}"));
        }
        match fs::write(&path, lines.join("\n") + "\n") {
            Ok(()) => true,
            Err(e) => {
                log::error!("Error setting vendor data for Module {module_index}: {e}");
                false
            }
        }
    }

    /// Removes the file [`Self::vendor_data_set_module`] writes.
    pub fn vendor_data_clear_module(&self, asic_index: i64, module_index: i64) -> bool {
        if !self.check_asic_index(asic_index) || !self.check_module_index(asic_index, module_index) {
            return false;
        }
        remove_if_exists(&self.base.join(format!("eeprom/module{module_index}_data")))
    }
}

// ── Python: hw_management_dpu_thermal_update ─────────────────────────────────

/// The three sensor kinds a DPU publishes, and the file stem each uses.
///
/// The stems are not uniform: the CPU one has no `_temp` in it and the drive
/// one is `drivetemp`, so they are listed rather than derived.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DpuSensor {
    CpuCore,
    Ddr,
    Drive,
}

impl DpuSensor {
    fn stem(self) -> &'static str {
        match self {
            DpuSensor::CpuCore => "cpu_pack",
            DpuSensor::Ddr => "sodimm_temp",
            DpuSensor::Drive => "drivetemp",
        }
    }

    /// `cpu_pack` and `drivetemp` take the suffix directly; `sodimm_temp`
    /// already carries `_temp`, so the input file is the stem itself.
    fn input(self) -> String {
        self.stem().to_string()
    }

    fn with(self, suffix: &str) -> String {
        format!("{}_{suffix}", self.stem())
    }
}

impl HwMgmt {
    /// `{base}/config/dpu_num`, or `None` when absent or unparsable.
    pub fn get_dpu_count(&self) -> Option<i64> {
        read_int(&self.base.join("config/dpu_num"))
    }

    /// Python: `0 < dpu_index <= get_dpu_count()`.  DPUs are 1-based.
    fn check_dpu_index(&self, dpu_index: i64) -> bool {
        match self.get_dpu_count() {
            Some(n) if dpu_index > 0 && dpu_index <= n => true,
            _ => {
                log::warn!("dpu_index {dpu_index} is out of bound 1..DPU");
                false
            }
        }
    }

    /// Write one DPU sensor's temperature, fault and optional thresholds.
    ///
    /// The `{base}/dpu{n}/thermal` directory is created if missing, as Python
    /// does.  A `None` threshold leaves that file alone rather than writing an
    /// empty one.
    pub fn thermal_data_dpu_set(
        &self,
        sensor: DpuSensor,
        dpu_index: i64,
        temperature: &str,
        warning_threshold: Option<&str>,
        critical_temperature: Option<&str>,
        fault: &str,
    ) -> bool {
        if !self.check_dpu_index(dpu_index) {
            return false;
        }
        let dir = self.base.join(format!("dpu{dpu_index}/thermal"));
        if let Err(e) = fs::create_dir_all(&dir) {
            log::error!("Error creating {}: {e}", dir.display());
            return false;
        }
        let mut writes = vec![
            (sensor.input(), temperature.to_string()),
            (sensor.with("fault"), fault.to_string()),
        ];
        if let Some(v) = warning_threshold {
            writes.push((sensor.with("max"), v.to_string()));
        }
        if let Some(v) = critical_temperature {
            writes.push((sensor.with("crit"), v.to_string()));
        }
        let mut ok = true;
        for (name, value) in writes {
            // No trailing newline here, unlike the module thermal writes.
            if let Err(e) = fs::write(dir.join(&name), &value) {
                log::error!("Error setting thermal data for DPU {dpu_index} {name}: {e}");
                ok = false;
            }
        }
        ok
    }

    /// Remove the four files [`Self::thermal_data_dpu_set`] writes.
    pub fn thermal_data_dpu_clear(&self, sensor: DpuSensor, dpu_index: i64) -> bool {
        if !self.check_dpu_index(dpu_index) {
            return false;
        }
        let dir = self.base.join(format!("dpu{dpu_index}/thermal"));
        let mut ok = true;
        for name in [sensor.input(), sensor.with("fault"), sensor.with("max"), sensor.with("crit")] {
            if !remove_if_exists(&dir.join(name)) {
                ok = false;
            }
        }
        ok
    }
}

fn read_int(path: &Path) -> Option<i64> {
    fs::read_to_string(path).ok()?.trim().parse().ok()
}

fn remove_if_exists(path: &Path) -> bool {
    if !path.exists() {
        return true;
    }
    match fs::remove_file(path) {
        Ok(()) => true,
        Err(e) => {
            log::error!("Error removing {}: {e}", path.display());
            false
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A tree with `asic_num` and `module_counter` already seeded, so the index
    /// checks pass.
    fn tree(asics: i64, modules: i64) -> (tempfile::TempDir, HwMgmt) {
        let dir = tempfile::tempdir().unwrap();
        fs::create_dir_all(dir.path().join("thermal")).unwrap();
        fs::create_dir_all(dir.path().join("config")).unwrap();
        fs::create_dir_all(dir.path().join("eeprom")).unwrap();
        fs::write(dir.path().join("config/asic_num"), asics.to_string()).unwrap();
        fs::write(dir.path().join("config/module_counter"), modules.to_string()).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        (dir, hw)
    }

    fn read(dir: &tempfile::TempDir, rel: &str) -> String {
        fs::read_to_string(dir.path().join(rel)).unwrap()
    }

    #[test]
    fn asic_zero_writes_both_the_alias_and_the_index() {
        let (dir, hw) = tree(2, 0);
        assert!(hw.thermal_data_set_asic(0, "45000", "105000", "120000", "0"));
        assert_eq!(read(&dir, "thermal/asic"), "45000\n");
        assert_eq!(read(&dir, "thermal/asic1"), "45000\n");
        assert!(!dir.path().join("thermal/asic2").exists());
    }

    #[test]
    fn asic_one_writes_only_its_index() {
        let (dir, hw) = tree(2, 0);
        assert!(hw.thermal_data_set_asic(1, "45000", "105000", "120000", "0"));
        assert_eq!(read(&dir, "thermal/asic2"), "45000\n");
        assert!(!dir.path().join("thermal/asic").exists());
    }

    /// The library maps straight through; the swap lives at the call site.
    #[test]
    fn crit_file_takes_the_critical_parameter() {
        let (dir, hw) = tree(1, 0);
        hw.thermal_data_set_asic(0, "45000", "warn", "crit", "0");
        assert_eq!(read(&dir, "thermal/asic_temp_crit"), "crit\n");
        assert_eq!(read(&dir, "thermal/asic_temp_emergency"), "warn\n");
    }

    /// A not-ready ASIC writes an empty string, never a plausible zero.
    #[test]
    fn empty_temperature_is_written_as_empty() {
        let (dir, hw) = tree(1, 0);
        hw.thermal_data_set_asic(0, "", "105000", "120000", "0");
        assert_eq!(read(&dir, "thermal/asic"), "\n");
    }

    #[test]
    fn out_of_range_indices_write_nothing() {
        let (dir, hw) = tree(1, 4);
        assert!(!hw.thermal_data_set_asic(1, "1", "2", "3", "0"));
        assert!(!hw.thermal_data_set_module(0, 0, "1", "2", "3", "0")); // modules are 1-based
        assert!(!hw.thermal_data_set_module(0, 5, "1", "2", "3", "0"));
        assert!(!dir.path().join("thermal/asic2").exists());
        assert!(!dir.path().join("thermal/module0_temp_input").exists());
    }

    #[test]
    fn missing_count_files_fail_closed() {
        let dir = tempfile::tempdir().unwrap();
        fs::create_dir_all(dir.path().join("thermal")).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        assert!(!hw.thermal_data_set_asic(0, "45000", "1", "2", "0"));
        assert!(!dir.path().join("thermal/asic").exists());
    }

    #[test]
    fn module_set_then_clean_round_trips() {
        let (dir, hw) = tree(1, 4);
        assert!(hw.thermal_data_set_module(0, 3, "40000", "70000", "80000", "0"));
        assert_eq!(read(&dir, "thermal/module3_temp_input"), "40000\n");
        assert_eq!(read(&dir, "thermal/module3_temp_crit"), "80000\n");
        assert!(hw.thermal_data_clean_module(0, 3));
        assert!(!dir.path().join("thermal/module3_temp_input").exists());
    }

    #[test]
    fn clean_asic_zero_removes_the_alias_too() {
        let (dir, hw) = tree(1, 0);
        hw.thermal_data_set_asic(0, "45000", "1", "2", "0");
        assert!(hw.thermal_data_clean_asic(0));
        for f in ["asic", "asic1", "asic_temp_crit", "asic1_temp_fault"] {
            assert!(!dir.path().join("thermal").join(f).exists(), "{f} survived");
        }
    }

    #[test]
    fn module_counter_has_no_trailing_newline() {
        let (dir, hw) = tree(1, 0);
        assert!(hw.module_data_set_module_counter(64));
        assert_eq!(read(&dir, "config/module_counter"), "64");
        assert!(!hw.module_data_set_module_counter(-1));
    }

    #[test]
    fn vendor_data_renames_keys_and_pads() {
        let (dir, hw) = tree(1, 4);
        let info = vec![
            ("manufacturer".to_string(), "NVIDIA".to_string()),
            ("part_number".to_string(), "MMA1B00-C100D".to_string()),
        ];
        assert!(hw.vendor_data_set_module(0, 1, &info));
        let got = read(&dir, "eeprom/module1_data");
        assert_eq!(got, "Manufacturer             : NVIDIA\nPN                       : MMA1B00-C100D\n");
    }

    #[test]
    fn empty_vendor_data_removes_the_file() {
        let (dir, hw) = tree(1, 4);
        let info = vec![("manufacturer".to_string(), "NVIDIA".to_string())];
        hw.vendor_data_set_module(0, 1, &info);
        assert!(dir.path().join("eeprom/module1_data").exists());
        assert!(hw.vendor_data_set_module(0, 1, &[]));
        assert!(!dir.path().join("eeprom/module1_data").exists());
    }

    fn dpu_tree(dpus: i64) -> (tempfile::TempDir, HwMgmt) {
        let dir = tempfile::tempdir().unwrap();
        fs::create_dir_all(dir.path().join("config")).unwrap();
        fs::write(dir.path().join("config/dpu_num"), dpus.to_string()).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        (dir, hw)
    }

    #[test]
    fn dpu_set_creates_the_tree_and_writes_four_files() {
        let (dir, hw) = dpu_tree(2);
        assert!(hw.thermal_data_dpu_set(DpuSensor::CpuCore, 1, "45", Some("90"), Some("100"), "0"));
        let d = dir.path().join("dpu1/thermal");
        assert_eq!(fs::read_to_string(d.join("cpu_pack")).unwrap(), "45");
        assert_eq!(fs::read_to_string(d.join("cpu_pack_fault")).unwrap(), "0");
        assert_eq!(fs::read_to_string(d.join("cpu_pack_max")).unwrap(), "90");
        assert_eq!(fs::read_to_string(d.join("cpu_pack_crit")).unwrap(), "100");
    }

    /// The stems are not uniform, and getting one wrong writes a file
    /// hw-management-tc never reads.
    #[test]
    fn each_sensor_kind_uses_its_own_file_stem() {
        let (dir, hw) = dpu_tree(1);
        hw.thermal_data_dpu_set(DpuSensor::Ddr, 1, "40", None, None, "0");
        hw.thermal_data_dpu_set(DpuSensor::Drive, 1, "41", None, None, "0");
        let d = dir.path().join("dpu1/thermal");
        assert_eq!(fs::read_to_string(d.join("sodimm_temp")).unwrap(), "40");
        assert_eq!(fs::read_to_string(d.join("drivetemp")).unwrap(), "41");
    }

    #[test]
    fn an_absent_threshold_writes_no_file() {
        let (dir, hw) = dpu_tree(1);
        hw.thermal_data_dpu_set(DpuSensor::CpuCore, 1, "45", None, None, "0");
        let d = dir.path().join("dpu1/thermal");
        assert!(d.join("cpu_pack").exists());
        assert!(!d.join("cpu_pack_max").exists(), "an empty threshold must not be written");
    }

    /// DPUs are 1-based, and 0 is out of range - unlike the ASIC index.
    #[test]
    fn dpu_indices_are_one_based() {
        let (dir, hw) = dpu_tree(2);
        assert!(!hw.thermal_data_dpu_set(DpuSensor::CpuCore, 0, "1", None, None, "0"));
        assert!(!hw.thermal_data_dpu_set(DpuSensor::CpuCore, 3, "1", None, None, "0"));
        assert!(hw.thermal_data_dpu_set(DpuSensor::CpuCore, 2, "1", None, None, "0"));
        assert!(!dir.path().join("dpu0").exists());
        assert!(!dir.path().join("dpu3").exists());
    }

    #[test]
    fn a_missing_dpu_count_writes_nothing() {
        let dir = tempfile::tempdir().unwrap();
        let hw = HwMgmt::with_base(dir.path());
        assert!(!hw.thermal_data_dpu_set(DpuSensor::CpuCore, 1, "45", None, None, "0"));
        assert!(!dir.path().join("dpu1").exists());
    }

    #[test]
    fn dpu_clear_removes_every_file_of_that_kind_only() {
        let (dir, hw) = dpu_tree(1);
        hw.thermal_data_dpu_set(DpuSensor::CpuCore, 1, "45", Some("90"), Some("100"), "0");
        hw.thermal_data_dpu_set(DpuSensor::Ddr, 1, "40", None, None, "0");
        assert!(hw.thermal_data_dpu_clear(DpuSensor::CpuCore, 1));
        let d = dir.path().join("dpu1/thermal");
        for f in ["cpu_pack", "cpu_pack_fault", "cpu_pack_max", "cpu_pack_crit"] {
            assert!(!d.join(f).exists(), "{f} survived");
        }
        assert!(d.join("sodimm_temp").exists(), "the DDR files must be untouched");
    }
}
