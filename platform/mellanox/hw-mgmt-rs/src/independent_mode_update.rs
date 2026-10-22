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


//! `hw_management_independent_mode_update.py`, which hw-management installs
//! under `/run/hw-management/bin` and the thermal updater imports from there.
//!
//! What it writes, and its `print()`s to stdout on failure, follow the
//! version README.md names.

use std::fmt::Display;

use platform_base::io;
use platform_base::py::py_int;

use crate::{print, read_utf8, write_utf8, BASE_PATH};

/// Where Python finds the module; its absence is the import's failure.
pub const MODULE_PATH: &str = "/run/hw-management/bin/hw_management_independent_mode_update.py";

/// A value the helpers write: `"{}\n".format(value)`.  (They skip a None,
/// which the platform never passes.)
#[derive(Clone, Debug)]
pub enum Val {
    Int(i128),
    Str(String),
}

impl Display for Val {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Val::Int(i) => write!(f, "{i}"),
            Val::Str(s) => f.write_str(s),
        }
    }
}

fn read_count(rel: &str, what: &str) -> Option<i128> {
    let file = format!("{BASE_PATH}/config/{rel}");
    if !io::exists(&file) {
        print(&format!("Error: Could not read {what} count from {file}"));
        return None;
    }
    match read_utf8(&file).and_then(|s| py_int(&s)) {
        Ok(n) => Some(n),
        Err(e) => {
            print(&format!("Error reading {what} count: {}", e.str()));
            None
        }
    }
}

/// `get_asic_count()`: None where Python answers False.
pub fn get_asic_count() -> Option<i128> {
    read_count("asic_num", "asic")
}

fn get_module_count() -> Option<i128> {
    read_count("module_counter", "module")
}

fn check_asic_index(asic_index: i128) -> bool {
    if let Some(n) = get_asic_count() {
        if (0..n).contains(&asic_index) {
            return true;
        }
    }
    print(&format!("asic_index {asic_index} is out of bound 0..ASIC"));
    false
}

fn check_module_index(asic_index: i128, module_index: i128) -> bool {
    if let Some(n) = get_module_count() {
        if (1..=n).contains(&module_index) {
            return true;
        }
    }
    print(&format!("module_index {module_index} of asic {asic_index} is out of bound 1..n"));
    false
}

/// `module_data_set_module_counter(module_counter)`.
pub fn module_data_set_module_counter(counter: i128) -> bool {
    if counter < 0 {
        print(&format!("Error: Could not set module count to {counter}"));
        return false;
    }
    match write_utf8(&format!("{BASE_PATH}/config/module_counter"), &counter.to_string()) {
        Ok(()) => true,
        Err(e) => {
            print(&format!("Error setting module counter: {}", e.str()));
            false
        }
    }
}

/// `write_file_data(dict)`: in order, stopping at the first failure.
fn write_file_data(items: &[(String, Val)]) -> bool {
    for (name, value) in items {
        if let Err(e) = write_utf8(&format!("{BASE_PATH}/thermal/{name}"), &format!("{value}\n")) {
            print(&format!("Error writing thermal data: {}", e.str()));
            return false;
        }
    }
    true
}

/// `remove_file_list(list)`.
fn remove_file_list(files: &[String]) -> bool {
    for f in files {
        let full = format!("{BASE_PATH}/thermal/{f}");
        if io::exists(&full) {
            if let Err(e) = std::fs::remove_file(io::real(&full)) {
                let e = platform_base::PyErr::from_io(&e, Some(&full));
                print(&format!("Error removing files: {} {}", platform_base::py::PyVal::List(files.iter().map(|s| platform_base::py::PyVal::str(s.as_str())).collect()).py_repr(), e.str()));
                return false;
            }
        }
    }
    true
}

/// `thermal_data_set_asic(asic_index, temperature, warning_threshold,
/// critical_threshold, fault)` -- positional, as the caller passes them.
pub fn thermal_data_set_asic(asic_index: i128, temperature: Val, warning: Val, critical: Val, fault: Val) -> bool {
    if !check_asic_index(asic_index) {
        return false;
    }
    let mut items: Vec<(String, Val)> = Vec::new();
    if asic_index == 0 {
        items.push(("asic_temp_crit".into(), critical.clone()));
        items.push(("asic".into(), temperature.clone()));
        items.push(("asic_temp_emergency".into(), warning.clone()));
        items.push(("asic_temp_fault".into(), fault.clone()));
    }
    let n = asic_index + 1;
    items.push((format!("asic{n}_temp_crit"), critical));
    items.push((format!("asic{n}"), temperature));
    items.push((format!("asic{n}_temp_emergency"), warning));
    items.push((format!("asic{n}_temp_fault"), fault));
    write_file_data(&items)
}

/// `thermal_data_set_module(asic_index, module_index, temperature,
/// warning_threshold, critical_threshold, fault)`.
pub fn thermal_data_set_module(asic_index: i128, module_index: i128, temperature: Val, warning: Val, critical: Val, fault: Val) -> bool {
    if !check_asic_index(asic_index) || !check_module_index(asic_index, module_index) {
        return false;
    }
    write_file_data(&[
        (format!("module{module_index}_temp_crit"), critical),
        (format!("module{module_index}_temp_input"), temperature),
        (format!("module{module_index}_temp_emergency"), warning),
        (format!("module{module_index}_temp_fault"), fault),
    ])
}

/// `vendor_data_set_module(asic_index, module_index, vendor_info)`.
pub fn vendor_data_set_module(asic_index: i128, module_index: i128, vendor: &[(&str, String)]) -> bool {
    if !check_asic_index(asic_index) || !check_module_index(asic_index, module_index) {
        return false;
    }
    let file = format!("{BASE_PATH}/eeprom/module{module_index}_data");
    let r = if !vendor.is_empty() {
        let lines: Vec<String> = vendor
            .iter()
            .map(|(k, v)| {
                let key = match k.to_lowercase().as_str() {
                    "part_number" => "PN".to_string(),
                    "manufacturer" => "Manufacturer".to_string(),
                    _ => k.to_string(),
                };
                format!("{key:<25}: {v}")
            })
            .collect();
        write_utf8(&file, &(lines.join("\n") + "\n"))
    } else if io::exists(&file) {
        std::fs::remove_file(io::real(&file)).map_err(|e| platform_base::PyErr::from_io(&e, Some(&file)))
    } else {
        Ok(())
    };
    match r {
        Ok(()) => true,
        Err(e) => {
            print(&format!("Error setting vendor data for Module {module_index}: {}", e.str()));
            false
        }
    }
}

/// `thermal_data_clean_asic(asic_index)`.
pub fn thermal_data_clean_asic(asic_index: i128) -> bool {
    if !check_asic_index(asic_index) {
        return false;
    }
    let n = asic_index + 1;
    let mut files = vec![
        format!("asic{n}_temp_crit"),
        format!("asic{n}"),
        format!("asic{n}_temp_emergency"),
        format!("asic{n}_temp_fault"),
    ];
    if asic_index == 0 {
        files.extend(["asic_temp_crit", "asic", "asic_temp_emergency", "asic_temp_fault"].map(String::from));
    }
    remove_file_list(&files)
}

/// `thermal_data_clean_module(asic_index, module_index)`.
pub fn thermal_data_clean_module(asic_index: i128, module_index: i128) -> bool {
    if !check_asic_index(asic_index) || !check_module_index(asic_index, module_index) {
        return false;
    }
    remove_file_list(&[
        format!("module{module_index}_temp_crit"),
        format!("module{module_index}_temp_input"),
        format!("module{module_index}_temp_emergency"),
        format!("module{module_index}_temp_fault"),
    ])
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    const T: &str = "/var/run/hw-management/thermal";

    /// What hw-management has made before anything writes there.
    fn tree(asics: &str, modules: &str) -> Root {
        let r = Root::new();
        r.write("/var/run/hw-management/config/asic_num", asics);
        r.write("/var/run/hw-management/config/module_counter", modules);
        r.write(&format!("{T}/.keep"), "");
        r.write("/var/run/hw-management/eeprom/.keep", "");
        r
    }

    #[test]
    fn asic_zero_writes_both_sets_of_files_with_a_newline() {
        let r = tree("1\n", "0");
        assert!(thermal_data_set_asic(0, Val::Str(String::new()), Val::Int(120000), Val::Int(105000), Val::Int(0)));
        assert_eq!(r.read(&format!("{T}/asic")).as_deref(), Some("\n"));
        assert_eq!(r.read(&format!("{T}/asic1")).as_deref(), Some("\n"));
        // The caller's third argument is the warning slot: emergency.
        assert_eq!(r.read(&format!("{T}/asic_temp_emergency")).as_deref(), Some("120000\n"));
        assert_eq!(r.read(&format!("{T}/asic1_temp_crit")).as_deref(), Some("105000\n"));
        assert_eq!(r.read(&format!("{T}/asic_temp_fault")).as_deref(), Some("0\n"));
    }

    #[test]
    fn an_index_out_of_bounds_writes_nothing() {
        let r = tree("1", "2");
        assert!(!thermal_data_set_asic(1, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(!thermal_data_set_module(0, 3, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(!thermal_data_set_module(0, 0, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(thermal_data_set_module(0, 2, Val::Int(30000), Val::Int(75000), Val::Int(70000), Val::Int(0)));
        assert_eq!(r.read(&format!("{T}/module2_temp_input")).as_deref(), Some("30000\n"));
        assert_eq!(r.read(&format!("{T}/module2_temp_emergency")).as_deref(), Some("75000\n"));
        assert!(r.read(&format!("{T}/module3_temp_input")).is_none());
    }

    #[test]
    fn no_count_file_means_out_of_bounds() {
        let _r = Root::new();
        assert_eq!(get_asic_count(), None);
        assert!(!thermal_data_set_asic(0, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
    }

    #[test]
    fn vendor_data_is_padded_and_renamed() {
        let r = tree("1", "4");
        assert!(vendor_data_set_module(0, 4, &[("manufacturer", "Mellanox".into()), ("part_number", "MCP1600".into())]));
        assert_eq!(
            r.read("/var/run/hw-management/eeprom/module4_data").as_deref(),
            Some("Manufacturer             : Mellanox\nPN                       : MCP1600\n")
        );
        assert!(vendor_data_set_module(0, 4, &[]));
        assert!(r.read("/var/run/hw-management/eeprom/module4_data").is_none());
        // Nothing to remove the second time.
        assert!(vendor_data_set_module(0, 4, &[]));
        // The helper writes UTF-8, whatever the locale.
        assert!(vendor_data_set_module(0, 4, &[("manufacturer", "M\u{e9}llanox".into())]));
        assert_eq!(
            r.read("/var/run/hw-management/eeprom/module4_data").as_deref(),
            Some("Manufacturer             : M\u{e9}llanox\n")
        );
    }

    #[test]
    fn cleaning_removes_what_setting_wrote() {
        let r = tree("1", "1");
        assert!(thermal_data_set_asic(0, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(thermal_data_set_module(0, 1, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(thermal_data_clean_asic(0));
        assert!(thermal_data_clean_module(0, 1));
        assert!(r.read(&format!("{T}/asic")).is_none());
        assert!(r.read(&format!("{T}/asic1_temp_fault")).is_none());
        assert!(r.read(&format!("{T}/module1_temp_input")).is_none());
        assert!(module_data_set_module_counter(65));
        assert_eq!(r.read("/var/run/hw-management/config/module_counter").as_deref(), Some("65"));
        assert!(!module_data_set_module_counter(-1));
    }

    #[test]
    fn counts_that_do_not_parse_are_none() {
        let r = tree("one", "x");
        assert_eq!(get_asic_count(), None);
        assert_eq!(get_module_count(), None);
        r.write("/var/run/hw-management/config/asic_num", "1");
        assert!(!thermal_data_set_module(0, 1, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(!vendor_data_set_module(0, 1, &[]));
        assert!(!thermal_data_clean_module(0, 1));
        assert!(!thermal_data_clean_asic(1));
        assert!(!vendor_data_set_module(1, 1, &[]));
    }

    #[test]
    fn a_write_or_removal_that_fails_answers_false() {
        // No thermal or eeprom folder to write to.
        let r = Root::new();
        r.write("/var/run/hw-management/config/asic_num", "2");
        r.write("/var/run/hw-management/config/module_counter", "1");
        assert!(!thermal_data_set_asic(1, Val::Int(1), Val::Int(2), Val::Int(3), Val::Int(0)));
        assert!(!vendor_data_set_module(0, 1, &[("serial", "S1".into())]));
        // A directory where a file is to be removed.
        std::fs::create_dir_all(r.real(&format!("{T}/asic2"))).unwrap();
        std::fs::create_dir_all(r.real("/var/run/hw-management/eeprom/module1_data")).unwrap();
        assert!(!thermal_data_clean_asic(1));
        assert!(!vendor_data_set_module(0, 1, &[]));
        // Removing what is not there is fine.
        assert!(thermal_data_clean_module(0, 1));
        // A module counter with no config folder to hold it.
        drop(r);
        let _r = Root::new();
        assert!(!module_data_set_module_counter(3));
    }

    #[test]
    fn vendor_keys_other_than_the_two_renamed_keep_their_spelling() {
        let r = tree("1", "1");
        assert!(vendor_data_set_module(0, 1, &[("Part_Number", "X".into()), ("serial_number", "S".into())]));
        assert_eq!(
            r.read("/var/run/hw-management/eeprom/module1_data").as_deref(),
            Some("PN                       : X\nserial_number            : S\n")
        );
        assert_eq!(format!("{}{}", Val::Int(-5), Val::Str("x".into())), "-5x");
    }
}
