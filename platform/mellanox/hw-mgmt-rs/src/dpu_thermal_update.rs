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

//! `hw_management_dpu_thermal_update.py`, which hw-management installs under
//! `/run/hw-management/bin` and the thermal updater imports from there: the
//! DPU temperatures, under `/var/run/hw-management/dpu<N>/thermal`.
//!
//! What it writes, and its `print()`s to stdout on failure, follow the
//! version README.md names.

use platform_base::io;
use platform_base::py::py_int;

use crate::{print, read_utf8, write_utf8, BASE_PATH};

pub const MODULE_PATH: &str = "/run/hw-management/bin/hw_management_dpu_thermal_update.py";

/// Whether the import found the module, decided by the first call.
static PRESENT: std::sync::Mutex<Option<bool>> = std::sync::Mutex::new(None);

/// Whether the import found the module.  Without it the updater runs
/// against `MagicMock`s: every call does nothing and answers truthy.
fn present() -> bool {
    let mut p = PRESENT.lock().unwrap_or_else(std::sync::PoisonError::into_inner);
    *p.get_or_insert_with(|| io::exists(MODULE_PATH))
}

/// The module imported again, as a new process would.
#[cfg(any(test, feature = "testing"))]
pub fn forget_import() {
    *PRESENT.lock().unwrap_or_else(std::sync::PoisonError::into_inner) = None;
}

/// `get_dpu_count()`: -1 when it cannot be read.
fn get_dpu_count() -> i128 {
    let file = format!("{BASE_PATH}/config/dpu_num");
    if !io::exists(&file) {
        print(&format!("Error: Could not read DPU count from {file}"));
        return -1;
    }
    match read_utf8(&file).and_then(|s| py_int(&s)) {
        Ok(n) => n,
        Err(e) => {
            print(&format!("Error reading DPU count: {}", e.str()));
            -1
        }
    }
}

fn check_dpu_index(dpu_index: i128) -> bool {
    let count = get_dpu_count();
    if count == -1 {
        return false;
    }
    if 0 < dpu_index && dpu_index <= count {
        return true;
    }
    print(&format!("dpu_index {dpu_index} is out of bound 1..DPU count"));
    false
}

/// `create_path_safe(path)`.
fn create_path_safe(path: &str) -> bool {
    if !io::exists(path) {
        if let Err(e) = std::fs::create_dir(io::real(path)) {
            let e = platform_base::PyErr::from_io(&e, Some(path));
            print(&format!("Path can't be created {path}: {}", e.str()));
            return false;
        }
    }
    true
}

/// `remove_file_safe(file_path)`.
fn remove_file_safe(path: &str) {
    if io::exists(path) {
        if let Err(e) = std::fs::remove_file(io::real(path)) {
            let e = platform_base::PyErr::from_io(&e, Some(path));
            print(&format!("file path didn't exist {path}: {}", e.str()));
        }
    }
}

/// One of the three components: its file names and how it is called in
/// the error message.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Part {
    CpuCore,
    Ddr,
    Drive,
}

impl Part {
    fn files(self) -> (&'static str, &'static str, &'static str, &'static str) {
        match self {
            Part::CpuCore => ("cpu_pack", "cpu_pack_fault", "cpu_pack_max", "cpu_pack_crit"),
            Part::Ddr => ("sodimm_temp_input", "sodimm_temp_fault", "sodimm_temp_max", "sodimm_temp_crit"),
            Part::Drive => ("drivetemp", "drivetemp_fault", "drivetemp_max", "drivetemp_crit"),
        }
    }
    fn label(self) -> &'static str {
        match self {
            Part::CpuCore => "CPU",
            Part::Ddr => "DDR",
            Part::Drive => "drive",
        }
    }
}

/// `thermal_data_dpu_<part>_set(dpu_index, temperature,
/// warning_threshold, critical_temperature, fault)`: values written as
/// `str()`, without a newline.
pub fn thermal_data_set(part: Part, dpu_index: i128, temperature: i128, warning: i128, critical: i128, fault: i128) -> bool {
    if !present() {
        return true;
    }
    if !check_dpu_index(dpu_index) {
        return false;
    }
    let dir = format!("{BASE_PATH}/dpu{dpu_index}");
    if !create_path_safe(&dir) || !create_path_safe(&format!("{dir}/thermal")) {
        return false;
    }
    let (input, fault_f, max, crit) = part.files();
    for (f, v) in [(input, temperature), (fault_f, fault), (max, warning), (crit, critical)] {
        if let Err(e) = write_utf8(&format!("{dir}/thermal/{f}"), &v.to_string()) {
            print(&format!("Error setting thermal data for DPU {} {dpu_index}: {}", part.label(), e.str()));
            return false;
        }
    }
    true
}

/// `thermal_data_dpu_<part>_clear(dpu_index)`.
pub fn thermal_data_clear(part: Part, dpu_index: i128) -> bool {
    if !present() {
        return true;
    }
    if !check_dpu_index(dpu_index) {
        return false;
    }
    let dir = format!("{BASE_PATH}/dpu{dpu_index}/thermal");
    let (input, fault_f, max, crit) = part.files();
    for f in [input, fault_f, max, crit] {
        remove_file_safe(&format!("{dir}/{f}"));
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    const D: &str = "/var/run/hw-management";

    fn dpu_switch(count: &str) -> Root {
        let r = Root::new();
        forget_import();
        r.write(MODULE_PATH, "");
        r.write(&format!("{D}/config/dpu_num"), count);
        r
    }

    #[test]
    fn without_hw_management_s_module_every_call_is_a_mock() {
        let r = Root::new();
        forget_import();
        assert!(thermal_data_set(Part::CpuCore, 9, 1, 2, 3, 0));
        assert!(thermal_data_clear(Part::Ddr, 9));
        assert!(r.read(&format!("{D}/dpu9/thermal/cpu_pack")).is_none());
        // The module appearing later changes nothing: the import is done.
        r.write(MODULE_PATH, "");
        assert!(thermal_data_set(Part::CpuCore, 9, 1, 2, 3, 0));
        forget_import();
    }

    #[test]
    fn a_dpu_s_temperatures_are_written_without_a_newline() {
        let r = dpu_switch("4\n");
        assert!(thermal_data_set(Part::CpuCore, 1, 55000, 95000, 105000, 0));
        let t = |f: &str| r.read(&format!("{D}/dpu1/thermal/{f}"));
        assert_eq!(
            [t("cpu_pack"), t("cpu_pack_fault"), t("cpu_pack_max"), t("cpu_pack_crit")].map(|v| v.unwrap_or_default()),
            ["55000", "0", "95000", "105000"]
        );
        assert!(thermal_data_set(Part::Ddr, 1, 40000, 85000, 95000, 1));
        assert_eq!(t("sodimm_temp_fault").as_deref(), Some("1"));
        assert!(thermal_data_set(Part::Drive, 4, 38000, 70000, 80000, 0));
        assert_eq!(r.read(&format!("{D}/dpu4/thermal/drivetemp_max")).as_deref(), Some("70000"));
        assert!(thermal_data_clear(Part::CpuCore, 1));
        assert!(t("cpu_pack").is_none() && t("cpu_pack_crit").is_none());
        assert_eq!(t("sodimm_temp_input").as_deref(), Some("40000"));
        // Clearing what is not there.
        assert!(thermal_data_clear(Part::Drive, 2));
        forget_import();
    }

    #[test]
    fn a_dpu_index_is_checked_against_dpu_num() {
        let r = dpu_switch("2");
        for i in [0, 3, -1] {
            assert!(!thermal_data_set(Part::CpuCore, i, 1, 2, 3, 0), "{i}");
            assert!(!thermal_data_clear(Part::CpuCore, i), "{i}");
        }
        r.write(&format!("{D}/config/dpu_num"), "two");
        assert!(!thermal_data_set(Part::Ddr, 1, 1, 2, 3, 0));
        std::fs::remove_file(r.real(&format!("{D}/config/dpu_num"))).unwrap();
        assert!(!thermal_data_clear(Part::Ddr, 1));
        forget_import();
    }

    #[test]
    fn what_cannot_be_created_or_written_or_removed_is_reported() {
        let r = dpu_switch("2");
        // dpu1 is a file: its thermal folder cannot be made.
        r.write(&format!("{D}/dpu1"), "");
        assert!(!thermal_data_set(Part::CpuCore, 1, 1, 2, 3, 0));
        // dpu2 a dangling link: not there, yet it cannot be made.
        let link = r.real(&format!("{D}/dpu2"));
        std::os::unix::fs::symlink("/nonexistent/dpu2", &link).unwrap();
        assert!(!thermal_data_set(Part::Drive, 2, 1, 2, 3, 0));
        std::fs::remove_file(&link).unwrap();
        // A folder where a temperature file goes: not written, and not
        // removed, though clearing carries on.
        for f in ["drivetemp", "cpu_pack", "sodimm_temp_input"] {
            std::fs::create_dir_all(r.real(&format!("{D}/dpu2/thermal/{f}"))).unwrap();
        }
        for part in [Part::Drive, Part::CpuCore, Part::Ddr] {
            assert!(!thermal_data_set(part, 2, 1, 2, 3, 0), "{part:?}");
        }
        r.write(&format!("{D}/dpu2/thermal/drivetemp_max"), "1");
        assert!(thermal_data_clear(Part::Drive, 2));
        assert!(r.read(&format!("{D}/dpu2/thermal/drivetemp_max")).is_none());
        forget_import();
    }
}
