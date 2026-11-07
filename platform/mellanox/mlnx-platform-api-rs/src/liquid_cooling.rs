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


//! `sonic_platform/liquid_cooling.py`.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

use platform_base::logger::PyLevel;
use platform_base::py::{py_int, PyEnum, PyResult, PyVal};
use platform_base::{BaseClass, PyObject};

use crate::utils::{self, Log};

const LIQUID_COOLING_SENSOR_PATH: &str = "/var/run/hw-management/system/";

/// `logging.getLogger('sonic_platform.liquid_cooling').error(msg)`: to
/// stderr, through the root handler the module's own import installed.
fn logging_error(msg: &str) {
    platform_base::logger::py_log("sonic_platform.liquid_cooling", PyLevel::Error, msg);
}

/// `LeakageSensor(name, path)`: base-class state plus the sysfs path.
pub struct LeakageSensor {
    name: String,
    path: String,
    leaking: AtomicBool,
}

impl PyObject for LeakageSensor {
    fn py_class(&self) -> &'static str {
        "LeakageSensor"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::LeakageSensorBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name.clone()))
    }
    /// `is_leak()`: 1 is dry, 0 is a leak; anything else, including a read
    /// that failed, is the string 'N/A' -- which callers test as true.
    fn is_leak(&self) -> PyResult<PyVal> {
        // `read_int_from_file(self.path, 'N/A')`: the default is a string.
        let content = utils::read_from_file(
            &self.path,
            |s| py_int(s).map(PyVal::Int),
            PyVal::str("N/A"),
            false,
            Log::Error,
        )?;
        match content {
            PyVal::Int(1) => {
                self.leaking.store(false, Ordering::Relaxed);
                Ok(PyVal::Bool(false))
            }
            PyVal::Int(0) => {
                self.leaking.store(true, Ordering::Relaxed);
                Ok(PyVal::Bool(true))
            }
            other => {
                logging_error(&format!(
                    "Failed to read leakage sensor {} value: {}",
                    self.name,
                    other.py_str()
                ));
                Ok(PyVal::str("N/A"))
            }
        }
    }
    fn is_leak_sensor_ok(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(true))
    }
    fn get_leak_sensor_type(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("unknown"))
    }
    fn get_leak_sensor_location(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("unknown"))
    }
    /// The base class's: CRITICAL while `leaking`, which only `is_leak` sets.
    fn get_leak_severity(&self) -> PyResult<PyVal> {
        Ok(if self.leaking.load(Ordering::Relaxed) {
            PyVal::Enum(PyEnum {
                class: "LeakSeverity",
                name: "CRITICAL",
                value: Box::new(PyVal::str("CRITICAL")),
            })
        } else {
            PyVal::None
        })
    }
}

/// `LiquidCooling()`.
pub struct LiquidCooling {
    sensors: Vec<PyVal>,
}

impl LiquidCooling {
    pub fn new() -> PyResult<Arc<LiquidCooling>> {
        crate::imports::liquid_cooling();
        let num = utils::read_int("/var/run/hw-management/config/leakage_counter", 0, Log::Error);
        let mut files = utils::glob(&format!("{LIQUID_COOLING_SENSOR_PATH}leakage*"));
        // `sort(key=lambda x: int(x.split("leakage")[-1]))`: a name whose
        // suffix is not a number raises, as the constructor does in Python.
        let mut keyed = Vec::new();
        for f in files.drain(..) {
            let suffix = f.rsplit("leakage").next().unwrap_or("").to_string();
            keyed.push((py_int(&suffix)?, f));
        }
        keyed.sort_by_key(|(k, _)| *k);
        let files: Vec<String> = keyed.into_iter().map(|(_, f)| f).collect();
        // `files[:num]`, with Python's slice semantics for a negative bound.
        let n = files.len() as i128;
        let end = if num < 0 { (n + num).max(0) } else { num.min(n) };
        let sensors = files
            .into_iter()
            .take(usize::try_from(end).unwrap_or(0))
            .map(|path| {
                let name = path.rsplit('/').next().unwrap_or(&path).to_string();
                PyVal::obj(LeakageSensor { name, path, leaking: AtomicBool::new(false) })
            })
            .collect();
        Ok(Arc::new(LiquidCooling { sensors }))
    }

    /// `get_num_leak_sensors()`.
    pub fn num_leak_sensors(&self) -> usize {
        self.sensors.len()
    }
}

impl PyObject for LiquidCooling {
    fn py_class(&self) -> &'static str {
        "LiquidCooling"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::LiquidCoolingBase
    }
    fn get_all_leak_sensors(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.sensors.clone()))
    }
    /// `list(self.profiles.values())`: no profiles on this platform.
    fn get_all_profiles(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(Vec::new()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    const SYS: &str = "/var/run/hw-management/system";

    fn names(lc: &LiquidCooling) -> Vec<String> {
        lc.sensors.iter().map(|s| match s {
            PyVal::Obj(o) => o.get_name().unwrap().py_str(),
            _ => String::new(),
        }).collect()
    }

    #[test]
    fn sensors_are_the_counted_leakage_files_in_numeric_order() {
        let root = Root::new();
        for n in [10, 2, 1] {
            root.write(&format!("{SYS}/leakage{n}"), "1");
        }
        let counted = |n: &str| {
            root.write("/var/run/hw-management/config/leakage_counter", n);
            names(&LiquidCooling::new().unwrap())
        };
        assert_eq!(counted("2"), ["leakage1", "leakage2"]);
        assert_eq!(counted("9"), ["leakage1", "leakage2", "leakage10"]);
        // `files[:num]` with a negative bound counts from the end.
        assert_eq!(counted("-1"), ["leakage1", "leakage2"]);
        assert!(counted("-5").is_empty());
        let lc = LiquidCooling::new().unwrap();
        assert_eq!(lc.num_leak_sensors(), 0);
        assert_eq!(lc.get_all_profiles().unwrap().py_repr(), "[]");
        assert_eq!((lc.py_class(), lc.get_all_leak_sensors().unwrap().py_repr()), ("LiquidCooling", "[]".to_string()));
        assert_eq!(lc.base_class(), BaseClass::LiquidCoolingBase);
        // A suffix that is no number raises, as the sort's key does.
        root.write(&format!("{SYS}/leakage_x"), "1");
        let e = LiquidCooling::new().err().unwrap();
        assert_eq!(e.str(), "invalid literal for int() with base 10: '_x'");
    }

    #[test]
    fn a_sensor_reads_1_as_dry_0_as_a_leak_and_anything_else_as_na() {
        let root = Root::new();
        root.write(&format!("{SYS}/leakage1"), "1");
        root.write("/var/run/hw-management/config/leakage_counter", "1");
        let lc = LiquidCooling::new().unwrap();
        let PyVal::Obj(sensor) = &lc.get_all_leak_sensors().unwrap().iter_items().unwrap()[0] else {
            panic!("no sensor");
        };
        let read = |v: Option<&str>| {
            match v {
                Some(v) => root.write(&format!("{SYS}/leakage1"), v),
                None => std::fs::remove_file(root.real(&format!("{SYS}/leakage1"))).unwrap(),
            }
            (sensor.is_leak().unwrap().py_repr(), sensor.get_leak_severity().unwrap().py_repr())
        };
        assert_eq!(read(Some("1")), ("False".into(), "None".into()));
        assert_eq!(read(Some("0")), ("True".into(), "<LeakSeverity.CRITICAL: 'CRITICAL'>".into()));
        // An unreadable value leaves the last state.
        assert_eq!(read(Some("x")), ("'N/A'".into(), "<LeakSeverity.CRITICAL: 'CRITICAL'>".into()));
        assert_eq!(read(None), ("'N/A'".into(), "<LeakSeverity.CRITICAL: 'CRITICAL'>".into()));
        assert_eq!(
            [sensor.is_leak_sensor_ok(), sensor.get_leak_sensor_type(), sensor.get_leak_sensor_location()].map(|v| v.unwrap().py_repr()),
            ["True", "'unknown'", "'unknown'"]
        );
        assert_eq!((sensor.py_class(), sensor.base_class()), ("LeakageSensor", BaseClass::LeakageSensorBase));
    }
}
