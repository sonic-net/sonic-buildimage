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


//! `sonic_platform/thermal.py`: thermal objects, created from naming rules.

use std::sync::Arc;

use platform_base::py::{PyResult, PyVal};
use platform_base::{io, BaseClass, PyObject};
use regex::Regex;

use crate::device_data;
use crate::utils::{self, Log};

const DEFAULT_TEMP_SCALE: f64 = 1000.0;
pub const CHASSIS_THERMAL_SYSFS_FOLDER: &str = "/run/hw-management/thermal";

/// One entry of `THERMAL_NAMING_RULE`.
#[derive(Clone, Debug, Default)]
pub struct Rule {
    pub name: String,
    pub temperature: String,
    pub high_threshold: Option<String>,
    pub high_critical_threshold: Option<String>,
    pub high_threshold_default: Option<i128>,
    pub high_critical_threshold_default: Option<i128>,
    pub sysfs_folder: Option<String>,
    pub scale: Option<f64>,
    pub kind: Kind,
    pub start_index: Option<i128>,
    pub default_present: Option<bool>,
    pub search_pattern: Option<String>,
    pub index_pattern: Option<String>,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Kind {
    #[default]
    Single,
    Indexable,
    AsicIndexable,
    Discrete,
}

fn rule(name: &str, temp: &str) -> Rule {
    Rule { name: name.into(), temperature: temp.into(), ..Default::default() }
}

/// `THERMAL_NAMING_RULE['psu thermals']`.
pub fn psu_rule() -> Rule {
    Rule {
        high_threshold: Some("psu{}_temp1_max".into()),
        kind: Kind::Indexable,
        ..rule("PSU-{} Temp", "psu{}_temp1")
    }
}

/// `THERMAL_NAMING_RULE['sfp thermals']`.
pub fn sfp_rule() -> Rule {
    Rule {
        high_threshold: Some("module{}_temp_crit".into()),
        high_critical_threshold: Some("module{}_temp_emergency".into()),
        kind: Kind::Indexable,
        ..rule("xSFP module {} Temp", "module{}_temp_input")
    }
}

/// `THERMAL_NAMING_RULE['linecard thermals']`.
pub fn linecard_rule() -> Rule {
    Rule {
        high_threshold: Some("gearbox{}_temp_emergency".into()),
        high_critical_threshold: Some("gearbox{}_temp_trip_crit".into()),
        kind: Kind::Indexable,
        ..rule("Gearbox {} Temp", "gearbox{}_temp_input")
    }
}

/// `THERMAL_NAMING_RULE['chassis thermals']`, in order.
pub fn chassis_rules() -> Vec<Rule> {
    vec![
        Rule {
            high_threshold_default: Some(105),
            high_critical_threshold_default: Some(120),
            sysfs_folder: Some("/sys/module/sx_core/asic{}/temperature".into()),
            scale: Some(8.0),
            kind: Kind::AsicIndexable,
            start_index: Some(0),
            ..rule("ASIC{}", "input")
        },
        Rule { default_present: Some(true), ..rule("Ambient Port Side Temp", "port_amb") },
        Rule { default_present: Some(true), ..rule("Ambient Fan Side Temp", "fan_amb") },
        rule("Ambient COMEX Temp", "comex_amb"),
        Rule {
            high_threshold: Some("cpu_pack_max".into()),
            high_critical_threshold: Some("cpu_pack_crit".into()),
            ..rule("CPU Pack Temp", "cpu_pack")
        },
        Rule {
            high_threshold: Some("cpu_core{}_max".into()),
            high_critical_threshold: Some("cpu_core{}_crit".into()),
            kind: Kind::Indexable,
            start_index: Some(0),
            ..rule("CPU Core {} Temp", "cpu_core{}")
        },
        Rule {
            high_threshold: Some("gearbox{}_temp_emergency".into()),
            high_critical_threshold: Some("gearbox{}_temp_trip_crit".into()),
            kind: Kind::Indexable,
            ..rule("Gearbox {} Temp", "gearbox{}_temp_input")
        },
        Rule { default_present: Some(false), ..rule("Ambient CPU Board Temp", "cpu_amb") },
        Rule { default_present: Some(false), ..rule("Ambient Switch Board Temp", "swb_amb") },
        Rule { default_present: Some(false), ..rule("PCH Temp", "pch_temp") },
        Rule {
            high_threshold: Some("sodimm{}_temp_max".into()),
            high_critical_threshold: Some("sodimm{}_temp_crit".into()),
            search_pattern: Some("/run/hw-management/thermal/sodimm*_temp_input".into()),
            index_pattern: Some(r"sodimm(\d+)_temp_input".into()),
            kind: Kind::Discrete,
            ..rule("SODIMM {} Temp", "sodimm{}_temp_input")
        },
    ]
}

/// `str.format` with one positional field, which is all the rules use.
fn fmt1(template: &str, v: &dyn std::fmt::Display) -> String {
    template.replacen("{}", &v.to_string(), 1)
}

/// What `presence_cb()` answers: `(status, hint)`.
pub type PresenceCb = Arc<dyn Fn() -> (bool, String) + Send + Sync>;

/// `initialize_chassis_thermals()`.
pub fn initialize_chassis_thermals() -> PyResult<Vec<PyVal>> {
    let mut out = Vec::new();
    let mut position: i128 = 1;
    for r in chassis_rules() {
        match r.kind {
            Kind::Indexable => {
                let count = if r.name.contains("Gearbox") {
                    device_data::get_gearbox_count("/run/hw-management/config")
                } else if r.name.contains("CPU Core") {
                    device_data::get_cpu_thermal_count() as i128
                } else {
                    0
                };
                if count == 0 {
                    utils::log_debug(&format!("Failed to get thermal object count for {}", r.name));
                    continue;
                }
                for index in 0..count {
                    out.push(create_indexable_thermal(&r, index, CHASSIS_THERMAL_SYSFS_FOLDER, position, None));
                    position += 1;
                }
            }
            Kind::Discrete => {
                let ts = create_discrete_thermal(&r, position);
                position += ts.len() as i128;
                out.extend(ts);
            }
            Kind::AsicIndexable => {
                let multi = device_data::is_multi_asic_platform()?;
                let count = device_data::get_asic_count()?;
                for asic in 0..count {
                    out.push(create_asic_thermal(&r, asic, position, multi)?);
                    position += 1;
                }
            }
            Kind::Single => {
                if let Some(t) = create_single_thermal(&r, CHASSIS_THERMAL_SYSFS_FOLDER, position, None, true)? {
                    out.push(t);
                    position += 1;
                }
            }
        }
    }
    Ok(out)
}

/// `create_asic_thermal(rule, asic_index, position, is_multi_asic)`.
fn create_asic_thermal(r: &Rule, asic: i128, position: i128, multi: bool) -> PyResult<PyVal> {
    let mut r = r.clone();
    r.name = if multi { fmt1(&r.name, &asic) } else { fmt1(&r.name, &"") };
    let folder = fmt1(r.sysfs_folder.as_deref().unwrap_or(""), &asic);
    r.sysfs_folder = Some(folder.clone());
    Ok(create_single_thermal(&r, &folder, position, None, false)?.unwrap_or_default())
}

/// `initialize_psu_thermal(psu_index, presence_cb)`.
pub fn initialize_psu_thermal(psu_index: i128, presence: PresenceCb) -> Vec<PyVal> {
    vec![create_indexable_thermal(&psu_rule(), psu_index, CHASSIS_THERMAL_SYSFS_FOLDER, 1, Some(presence))]
}

/// `initialize_linecard_thermals(lc_name, lc_index)`.
pub fn initialize_linecard_thermals(lc_name: &str, lc_index: i128) -> Vec<PyVal> {
    let mut r = linecard_rule();
    r.name = format!("{lc_name} {}", r.name);
    let folder = format!("/run/hw-management/lc{lc_index}/thermal");
    let count = device_data::get_gearbox_count(&format!("/run/hw-management/lc{lc_index}/config"));
    (0..count.max(0))
        .map(|i| create_indexable_thermal(&r, i, &folder, i + 1, None))
        .collect()
}

/// `initialize_linecard_sfp_thermal(lc_name, lc_index, sfp_index)`.
pub fn initialize_linecard_sfp_thermal(lc_name: &str, lc_index: i128, sfp_index: i128) -> Vec<PyVal> {
    let mut r = sfp_rule();
    r.name = format!("{lc_name} {}", r.name);
    let folder = format!("/run/hw-management/lc{lc_index}/thermal");
    vec![create_indexable_thermal(&r, sfp_index, &folder, 1, None)]
}

/// `create_indexable_thermal(rule, index, sysfs_folder, position, presence_cb)`.
pub fn create_indexable_thermal(
    r: &Rule,
    index: i128,
    sysfs_folder: &str,
    position: i128,
    presence: Option<PresenceCb>,
) -> PyVal {
    let index = index + r.start_index.unwrap_or(1);
    let name = fmt1(&r.name, &index);
    let folder = r.sysfs_folder.clone().unwrap_or_else(|| sysfs_folder.to_string());
    let temp = io::join(&folder, &fmt1(&r.temperature, &index));
    check_existence(&temp, presence.as_ref());
    let high = r.high_threshold.as_ref().map(|h| {
        let p = io::join(&folder, &fmt1(h, &index));
        check_existence(&p, presence.as_ref());
        p
    });
    let crit = r.high_critical_threshold.as_ref().map(|h| {
        let p = io::join(&folder, &fmt1(h, &index));
        check_existence(&p, presence.as_ref());
        p
    });
    new_thermal(name, temp, high, crit, r, position, presence)
}

/// `create_single_thermal(rule, sysfs_folder, position, presence_cb, check_presence)`.
pub fn create_single_thermal(
    r: &Rule,
    sysfs_folder: &str,
    position: i128,
    presence: Option<PresenceCb>,
    check_presence: bool,
) -> PyResult<Option<PyVal>> {
    let default_present = r.default_present.unwrap_or(true);
    match device_data::get_thermal_capability()? {
        Some(cap) if !cap.is_empty() => {
            let present = cap
                .iter()
                .find(|(k, _)| *k == r.temperature)
                .map_or(default_present, |(_, v)| *v);
            if !present {
                return Ok(None);
            }
        }
        _ => {
            if !default_present {
                return Ok(None);
            }
        }
    }
    let folder = r.sysfs_folder.clone().unwrap_or_else(|| sysfs_folder.to_string());
    let temp = io::join(&folder, &r.temperature);
    if check_presence {
        check_existence(&temp, presence.as_ref());
    }
    let high = r.high_threshold.as_ref().map(|h| {
        let p = io::join(&folder, h);
        if check_presence {
            check_existence(&p, presence.as_ref());
        }
        p
    });
    let crit = r.high_critical_threshold.as_ref().map(|h| {
        let p = io::join(&folder, h);
        if check_presence {
            check_existence(&p, presence.as_ref());
        }
        p
    });
    Ok(Some(new_thermal(r.name.clone(), temp, high, crit, r, position, presence)))
}

/// `create_discrete_thermal(rule, position)`.
fn create_discrete_thermal(r: &Rule, mut position: i128) -> Vec<PyVal> {
    let pattern = r.index_pattern.clone().unwrap_or_default();
    let re = Regex::new(&pattern).ok();
    let mut out = Vec::new();
    for path in utils::glob(r.search_pattern.as_deref().unwrap_or("")) {
        let file = path.rsplit('/').next().unwrap_or(&path).to_string();
        let index = re
            .as_ref()
            .and_then(|re| re.captures(&file))
            .and_then(|c| c.get(1))
            .and_then(|m| m.as_str().parse::<i128>().ok());
        let Some(index) = index else {
            utils::log_error(&format!("Failed to extract index from {file} using pattern {pattern}"));
            continue;
        };
        out.push(create_indexable_thermal(r, index - 1, CHASSIS_THERMAL_SYSFS_FOLDER, position, None));
        position += 1;
    }
    out
}

/// `_check_thermal_sysfs_existence(file_path, presence_cb)`.
fn check_existence(path: &str, presence: Option<&PresenceCb>) {
    if let Some(cb) = presence {
        if !cb().0 {
            return;
        }
    }
    if !io::exists(path) {
        utils::log_error(&format!("Thermal sysfs {path} does not exist"));
    }
}

pub(crate) fn new_thermal(
    name: String,
    temp: String,
    high: Option<String>,
    crit: Option<String>,
    r: &Rule,
    position: i128,
    presence: Option<PresenceCb>,
) -> PyVal {
    PyVal::obj(Thermal {
        name,
        position,
        temperature: temp,
        high_threshold: high,
        high_critical_threshold: crit,
        high_th_default: r.high_threshold_default,
        high_crit_th_default: r.high_critical_threshold_default,
        scale: r.scale.unwrap_or(DEFAULT_TEMP_SCALE),
        presence,
    })
}

/// `Thermal`, and `RemovableThermal` when it has a presence callback.
pub struct Thermal {
    name: String,
    position: i128,
    temperature: String,
    high_threshold: Option<String>,
    high_critical_threshold: Option<String>,
    high_th_default: Option<i128>,
    high_crit_th_default: Option<i128>,
    scale: f64,
    presence: Option<PresenceCb>,
}

impl Thermal {
    /// `value / self.scale if (value is not None and value != 0) else d`.
    fn scaled(&self, path: &str, default: Option<i128>) -> PyVal {
        match utils::read_float_opt(path, Log::Info) {
            Some(v) if v != 0.0 => PyVal::Float(v / self.scale),
            _ => default.map_or(PyVal::None, PyVal::Int),
        }
    }

    /// RemovableThermal: an absent PSU answers None without reading.
    fn absent(&self, what: &str) -> bool {
        match &self.presence {
            Some(cb) => {
                let (status, hint) = cb();
                if !status {
                    utils::log_debug(&format!("{what} for {} failed due to {hint}", self.name));
                }
                !status
            }
            None => false,
        }
    }
}

impl PyObject for Thermal {
    fn py_class(&self) -> &'static str {
        if self.presence.is_some() {
            "RemovableThermal"
        } else {
            "Thermal"
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ThermalBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name.clone()))
    }
    fn get_temperature(&self) -> PyResult<PyVal> {
        if self.absent("get_temperature") {
            return Ok(PyVal::None);
        }
        Ok(self.scaled(&self.temperature.clone(), None))
    }
    fn get_high_threshold(&self) -> PyResult<PyVal> {
        if self.absent("get_high_threshold") {
            return Ok(PyVal::None);
        }
        match &self.high_threshold {
            None => Ok(self.high_th_default.map_or(PyVal::None, PyVal::Int)),
            Some(p) => Ok(self.scaled(p, self.high_th_default)),
        }
    }
    fn get_high_critical_threshold(&self) -> PyResult<PyVal> {
        if self.absent("get_high_critical_threshold") {
            return Ok(PyVal::None);
        }
        match &self.high_critical_threshold {
            None => Ok(self.high_crit_th_default.map_or(PyVal::None, PyVal::Int)),
            Some(p) => Ok(self.scaled(p, self.high_crit_th_default)),
        }
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.position))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(false))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_f::{is, switch};
    use crate::testing::Root;

    const T: &str = "/run/hw-management/thermal";

    fn r(v: PyResult<PyVal>) -> String {
        v.map_or_else(|e| format!("raise {}", e.exc.name()), |v| v.py_repr())
    }

    fn obj(v: &PyVal) -> &Arc<dyn PyObject> {
        let PyVal::Obj(o) = v else { panic!("{}", v.py_repr()) };
        o
    }

    /// `(name, position)` of each thermal, in order.
    fn names(ts: &[PyVal]) -> Vec<(String, i128)> {
        ts.iter()
            .map(|t| {
                let o = obj(t);
                let PyVal::Int(p) = o.get_position_in_parent().unwrap() else { panic!() };
                (o.get_name().unwrap().py_str(), p)
            })
            .collect()
    }

    fn files(root: &Root, fs: &[(&str, &str)]) {
        for (f, v) in fs {
            root.write(&format!("{T}/{f}"), v);
        }
    }

    /// Every chassis thermal file test_chassis_thermal mocks as present.
    fn chassis_tree(root: &Root) {
        files(
            root,
            &[
                ("port_amb", "26500"),
                ("fan_amb", "25000"),
                ("comex_amb", "30000"),
                ("cpu_pack", "45000"),
                ("cpu_pack_max", "85000"),
                ("cpu_core0", "44000"),
                ("cpu_core0_max", "85000"),
                ("cpu_core0_crit", "105000"),
                ("cpu_core1", "43000"),
                ("cpu_core1_max", "85000"),
                ("cpu_core1_crit", "105000"),
                ("gearbox1_temp_input", "50000"),
                ("gearbox1_temp_emergency", "110000"),
                ("gearbox1_temp_trip_crit", "120000"),
                ("gearbox2_temp_input", "51000"),
                ("gearbox2_temp_emergency", "110000"),
                ("gearbox2_temp_trip_crit", "120000"),
                ("cpu_amb", "30000"),
                ("swb_amb", "31000"),
                ("pch_temp", "52000"),
                ("sodimm1_temp_input", "36000"),
                ("sodimm1_temp_max", "85000"),
                ("sodimm1_temp_crit", "95000"),
            ],
        );
        root.write("/run/hw-management/config/gearbox_counter", "2");
        root.write("/sys/module/sx_core/asic0/temperature/input", "360");
    }

    fn by_name<'a>(ts: &'a [PyVal], name: &str) -> &'a Arc<dyn PyObject> {
        ts.iter().map(obj).find(|o| o.get_name().unwrap().py_str() == name).unwrap_or_else(|| panic!("{name}"))
    }

    #[test]
    fn chassis_thermals_follow_the_naming_rules() {
        // test_chassis_thermal: SN2700, no COMEX.
        let root = switch("x86_64-mlnx_msn2700-r0");
        chassis_tree(&root);
        let ts = initialize_chassis_thermals().unwrap();
        let want: &[&str] = if is("x86_64-mlnx_msn2700-r0") {
            &[
                "ASIC",
                "Ambient Port Side Temp",
                "Ambient Fan Side Temp",
                "CPU Pack Temp",
                "CPU Core 0 Temp",
                "CPU Core 1 Temp",
                "Gearbox 1 Temp",
                "Gearbox 2 Temp",
                "SODIMM 1 Temp",
            ]
        } else {
            return;
        };
        let got = names(&ts);
        assert_eq!(got.iter().map(|(n, _)| n.as_str()).collect::<Vec<_>>(), want);
        assert_eq!(got.iter().map(|(_, p)| *p).collect::<Vec<_>>(), (1..=want.len() as i128).collect::<Vec<_>>());
        // The ASIC: scale 8, and thresholds by default.
        let asic = by_name(&ts, "ASIC");
        assert_eq!((r(asic.get_temperature()), r(asic.get_high_threshold()), r(asic.get_high_critical_threshold())), ("45.0".into(), "105".into(), "120".into()));
        assert_eq!((asic.py_class(), asic.base_class(), r(asic.is_replaceable())), ("Thermal", BaseClass::ThermalBase, "False".into()));
        let pack = by_name(&ts, "CPU Pack Temp");
        assert_eq!((r(pack.get_temperature()), r(pack.get_high_threshold()), r(pack.get_high_critical_threshold())), ("45.0".into(), "85.0".into(), "None".into()));
        let core = by_name(&ts, "CPU Core 1 Temp");
        assert_eq!((r(core.get_temperature()), r(core.get_high_critical_threshold())), ("43.0".into(), "105.0".into()));
        let gb = by_name(&ts, "Gearbox 2 Temp");
        assert_eq!((r(gb.get_temperature()), r(gb.get_high_threshold()), r(gb.get_high_critical_threshold())), ("51.0".into(), "110.0".into(), "120.0".into()));
        let port = by_name(&ts, "Ambient Port Side Temp");
        assert_eq!((r(port.get_temperature()), r(port.get_high_threshold()), r(port.get_high_critical_threshold())), ("26.5".into(), "None".into(), "None".into()));
        // A reading of 0, or none at all, is no temperature.
        files(&root, &[("port_amb", "0")]);
        assert_eq!(r(port.get_temperature()), "None");
        std::fs::remove_file(root.real(&format!("{T}/port_amb"))).unwrap();
        assert_eq!(r(port.get_temperature()), "None");
        // A threshold of 0 falls back to the default (none for a sysfs one).
        root.write("/sys/module/sx_core/asic0/temperature/input", "0");
        assert_eq!(r(asic.get_temperature()), "None");
        files(&root, &[("cpu_pack_max", "0")]);
        assert_eq!(r(pack.get_high_threshold()), "None");
    }

    #[test]
    fn the_capability_table_decides_which_single_thermals_exist() {
        let cases: &[(&str, &[&str], &[&str])] = &[
            // SN5600: a PCH, no COMEX.
            ("x86_64-nvidia_sn5600-r0", &["PCH Temp", "Ambient Port Side Temp"], &["Ambient COMEX Temp", "Ambient CPU Board Temp"]),
            // SN2201: a CPU board ambient.
            ("x86_64-nvidia_sn2201-r0", &["Ambient CPU Board Temp"], &["Ambient Switch Board Temp", "Ambient COMEX Temp"]),
            // SN4700: no capability table, so each rule's default.
            ("x86_64-mlnx_msn4700-r0", &["Ambient COMEX Temp", "CPU Pack Temp"], &["PCH Temp", "Ambient CPU Board Temp"]),
            // SN6600_LD: no ambient sensors.
            ("x86_64-nvidia_sn6600_ld-r0", &["CPU Pack Temp"], &["Ambient Port Side Temp", "Ambient Fan Side Temp"]),
            // A platform the table does not list.
            ("x86_64-acme_unknown-r0", &["Ambient COMEX Temp"], &["PCH Temp"]),
        ];
        for (platform, present, absent) in cases {
            let root = switch(platform);
            chassis_tree(&root);
            if !is(platform) {
                continue;
            }
            let ts = initialize_chassis_thermals().unwrap();
            let got: Vec<String> = names(&ts).into_iter().map(|(n, _)| n).collect();
            for n in *present {
                assert!(got.iter().any(|g| g == n), "{platform}: {n} missing from {got:?}");
            }
            for n in *absent {
                assert!(!got.iter().any(|g| g == n), "{platform}: {n} in {got:?}");
            }
        }
    }

    #[test]
    fn without_counts_the_indexable_thermals_are_skipped() {
        let root = switch("x86_64-mlnx_msn4700-r0");
        files(&root, &[("port_amb", "26500"), ("fan_amb", "25000")]);
        if !is("x86_64-mlnx_msn4700-r0") {
            return;
        }
        let got: Vec<String> = names(&initialize_chassis_thermals().unwrap()).into_iter().map(|(n, _)| n).collect();
        assert!(!got.iter().any(|n| n.contains("Core") || n.contains("Gearbox") || n.contains("SODIMM")), "{got:?}");
        // The single thermals are made whether or not their files exist.
        assert!(got.iter().any(|n| n == "CPU Pack Temp"));
    }

    #[test]
    fn each_asic_of_a_multi_asic_switch_has_its_thermal() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        root.write("/sys/module/sx_core/asic0/temperature/input", "320");
        root.write("/sys/module/sx_core/asic1/temperature/input", "328");
        if !is("x86_64-nvidia_sn5600-r0") {
            return;
        }
        let ts = initialize_chassis_thermals().unwrap();
        let got = names(&ts);
        assert_eq!(&got[..2], &[("ASIC0".to_string(), 1), ("ASIC1".to_string(), 2)]);
        assert_eq!((r(obj(&ts[0]).get_temperature()), r(obj(&ts[1]).get_temperature())), ("40.0".into(), "41.0".into()));
    }

    #[test]
    fn a_sodimm_file_without_an_index_is_skipped() {
        let root = switch("x86_64-mlnx_msn2700-r0");
        files(&root, &[("sodimm2_temp_input", "37000"), ("sodimmX_temp_input", "1")]);
        let ts = create_discrete_thermal(&chassis_rules()[10], 7);
        assert_eq!(names(&ts), [("SODIMM 2 Temp".to_string(), 7)]);
        let t = obj(&ts[0]);
        assert_eq!((r(t.get_temperature()), r(t.get_high_threshold())), ("37.0".into(), "None".into()));
        drop(root);
    }

    #[test]
    fn a_psu_thermal_reads_only_while_its_psu_is_there() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        files(&root, &[("psu1_temp1", "40567"), ("psu1_temp1_max", "50678")]);
        let ts = initialize_psu_thermal(0, Arc::new(|| (true, String::new())));
        assert_eq!(names(&ts), [("PSU-1 Temp".to_string(), 1)]);
        let t = obj(&ts[0]);
        assert_eq!(t.py_class(), "RemovableThermal");
        assert_eq!((r(t.get_temperature()), r(t.get_high_threshold()), r(t.get_high_critical_threshold())), ("40.567".into(), "50.678".into(), "None".into()));
        assert_eq!(r(t.is_replaceable()), "False");
        // test_psu_thermal: not present, so None without reading.
        let ts = initialize_psu_thermal(0, Arc::new(|| (false, "Not present".into())));
        let t = obj(&ts[0]);
        assert_eq!((r(t.get_temperature()), r(t.get_high_threshold()), r(t.get_high_critical_threshold())), ("None".into(), "None".into(), "None".into()));
        // A present PSU whose files are gone: logged, and still made.
        let ts = initialize_psu_thermal(1, Arc::new(|| (true, String::new())));
        assert_eq!(r(obj(&ts[0]).get_temperature()), "None");
        drop(root);
    }

    #[test]
    fn a_line_card_s_thermals_live_under_its_own_folder() {
        let root = switch("x86_64-nvidia_sn4800-r0");
        root.write("/run/hw-management/lc1/config/gearbox_counter", "2");
        root.write("/run/hw-management/lc1/thermal/gearbox2_temp_input", "61000");
        root.write("/run/hw-management/lc1/thermal/gearbox2_temp_emergency", "105000");
        root.write("/run/hw-management/lc1/thermal/module3_temp_input", "33000");
        root.write("/run/hw-management/lc1/thermal/module3_temp_crit", "70000");
        root.write("/run/hw-management/lc1/thermal/module3_temp_emergency", "75000");
        let ts = initialize_linecard_thermals("LC1", 1);
        assert_eq!(names(&ts), [("LC1 Gearbox 1 Temp".to_string(), 1), ("LC1 Gearbox 2 Temp".to_string(), 2)]);
        assert_eq!((r(obj(&ts[1]).get_temperature()), r(obj(&ts[1]).get_high_threshold())), ("61.0".into(), "105.0".into()));
        let sfp = initialize_linecard_sfp_thermal("LC1", 1, 2);
        assert_eq!(names(&sfp), [("LC1 xSFP module 3 Temp".to_string(), 1)]);
        let t = obj(&sfp[0]);
        assert_eq!((r(t.get_temperature()), r(t.get_high_threshold()), r(t.get_high_critical_threshold())), ("33.0".into(), "70.0".into(), "75.0".into()));
        // A line card without gearboxes has none.
        assert!(initialize_linecard_thermals("LC2", 2).is_empty());
        assert_eq!(sfp_rule().name, "xSFP module {} Temp");
    }

    #[test]
    fn a_rule_present_by_capability_only_is_made_without_checking_files() {
        let root = switch("x86_64-nvidia_sn2201-r0");
        let cpu_amb = Rule { default_present: Some(false), ..rule("Ambient CPU Board Temp", "cpu_amb") };
        let t = create_single_thermal(&cpu_amb, T, 4, None, true).unwrap();
        if is("x86_64-nvidia_sn2201-r0") {
            let t = t.unwrap();
            assert_eq!(names(std::slice::from_ref(&t)), [("Ambient CPU Board Temp".to_string(), 4)]);
            assert_eq!(r(obj(&t).get_temperature()), "None");
        }
        // Without a capability entry, the rule's own default decides.
        let pch = Rule { default_present: Some(false), ..rule("PCH Temp", "pch_temp") };
        assert!(create_single_thermal(&pch, T, 1, None, true).unwrap().is_none());
        drop(root);
    }
}
