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


//! `sonic_platform/pdb.py`: the power distribution board of a DC platform.

use std::sync::Arc;

use platform_base::py::{PyResult, PyVal};
use platform_base::{bodies, io, BaseClass, PyObject};

use crate::thermal::{new_thermal, Rule};
use crate::utils::{self, Log};

const PDB_PATH: &str = "/var/run/hw-management";
const CURRENT_SCALE_DIVISOR: f64 = 1000.0;
const POWER_SCALE_DIVISOR: f64 = 1_000_000.0;

/// `Pdb(pdb_index)`.
pub struct Pdb {
    index: i128,
    pwr_status: String,
    thermals: Vec<PyVal>,
    in_voltage: String,
    in_current: String,
    in_current_scale: String,
    in_power: String,
    in_power_scale: String,
    power_max: String,
}

impl Pdb {
    pub fn new(pdb_index: i128) -> Arc<Pdb> {
        crate::imports::pdb();
        bodies::psu_base::init();
        let index = pdb_index + 1;
        let p = |a: &str, f: String| io::join(&io::join(PDB_PATH, a), &f);
        let temp = p("thermal", format!("pdb_hotswap{index}_temp1_input"));
        let high = p("thermal", format!("pdb_hotswap{index}_temp1_max"));
        let crit = p("thermal", format!("pdb_hotswap{index}_temp1_crit"));
        let mut thermals = Vec::new();
        if io::exists(&temp) {
            // `Thermal(name, temp, high or None, crit or None, None, None, 1000, 1)`:
            // built directly, so without the platform's thermal capability or
            // the existence checks a rule goes through.
            thermals.push(new_thermal(
                format!("PDB-{index} Temp"),
                temp.clone(),
                io::exists(&high).then_some(high),
                io::exists(&crit).then_some(crit),
                &Rule::default(),
                1,
                None,
            ));
        } else {
            utils::log_error(&format!("PDB {index} temperature file {temp} does not exist"));
        }
        let env = |f: String| p("environment", f);
        Arc::new(Pdb {
            index,
            pwr_status: p("system", format!("pdb{index}_pwr_status")),
            thermals,
            in_voltage: env(format!("pdb_hotswap{index}_in1_input")),
            in_current: env(format!("pdb_hotswap{index}_curr1_input")),
            in_current_scale: env(format!("pdb_hotswap{index}_curr1_scale")),
            in_power: env(format!("pdb_hotswap{index}_power1_input")),
            in_power_scale: env(format!("pdb_hotswap{index}_power1_scale")),
            power_max: env(format!("pdb_hotswap{index}_power1_max")),
        })
    }

    fn scale_factor(&self, path: &str) -> f64 {
        if io::exists(path) {
            utils::read_float(path, 1.0, Log::Info)
        } else {
            1.0
        }
    }

    fn scaled(&self, input: &str, scale: &str, divisor: f64) -> PyVal {
        if !io::exists(input) {
            return PyVal::None;
        }
        let v = utils::read_int(input, 0, Log::Info);
        PyVal::Float(v as f64 * self.scale_factor(scale) / divisor)
    }

    fn status(&self) -> bool {
        utils::read_int(&self.pwr_status, 0, Log::Off) == 1
    }
}

impl PyObject for Pdb {
    fn py_class(&self) -> &'static str {
        "Pdb"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::PdbBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(format!("PDB {}", self.index)))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(true))
    }
    fn get_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.status()))
    }
    fn get_powergood_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.status()))
    }
    fn get_model(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("N/A"))
    }
    fn get_serial(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("N/A"))
    }
    fn get_revision(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("N/A"))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.index))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(false))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        let text = utils::read_str(&io::join(PDB_PATH, "led/led_power"), "", Log::Off);
        let text = platform_base::py::strip(&text);
        if text.is_empty() || text.to_lowercase() == "none" {
            return Ok(PyVal::str("N/A"));
        }
        Ok(PyVal::str(text))
    }
    fn get_temperature(&self) -> PyResult<PyVal> {
        match self.thermals.first() {
            Some(PyVal::Obj(t)) => t.get_temperature(),
            _ => Ok(PyVal::None),
        }
    }
    fn get_input_current(&self) -> PyResult<PyVal> {
        Ok(self.scaled(&self.in_current, &self.in_current_scale, CURRENT_SCALE_DIVISOR))
    }
    fn get_input_power(&self) -> PyResult<PyVal> {
        Ok(self.scaled(&self.in_power, &self.in_power_scale, POWER_SCALE_DIVISOR))
    }
    fn get_input_voltage(&self) -> PyResult<PyVal> {
        if io::exists(&self.in_voltage) {
            return Ok(PyVal::Float(utils::read_int(&self.in_voltage, 0, Log::Info) as f64 / 1000.0));
        }
        Ok(PyVal::None)
    }
    fn get_maximum_supplied_power(&self) -> PyResult<PyVal> {
        if io::exists(&self.power_max) {
            return Ok(PyVal::Float(utils::read_int(&self.power_max, 0, Log::Info) as f64 / 1_000_000.0));
        }
        Ok(PyVal::None)
    }
    fn get_voltage(&self) -> PyResult<PyVal> {
        self.get_input_voltage()
    }
    fn get_current(&self) -> PyResult<PyVal> {
        self.get_input_current()
    }
    fn get_power(&self) -> PyResult<PyVal> {
        self.get_input_power()
    }
    fn get_all_thermals(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.thermals.clone()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_f::{hw, switch};

    fn r(v: PyResult<PyVal>) -> String {
        v.map_or_else(|e| format!("raise {}", e.exc.name()), |v| v.py_repr())
    }

    /// The SN6600_LD sandbox's PDBs: PDB 1 complete, PDB 2 without a
    /// temperature and with readings that do not parse.
    fn sn6600_ld() -> crate::testing::Root {
        let root = switch("x86_64-nvidia_sn6600_ld-r0");
        hw(
            &root,
            &[
                ("system/pdb1_pwr_status", "1"),
                ("system/pdb2_pwr_status", "0"),
                ("thermal/pdb_hotswap1_temp1_input", "41000"),
                ("thermal/pdb_hotswap1_temp1_max", "85000"),
                ("environment/pdb_hotswap1_in1_input", "54123"),
                ("environment/pdb_hotswap1_curr1_input", "30500"),
                ("environment/pdb_hotswap1_curr1_scale", "2.5"),
                ("environment/pdb_hotswap1_power1_input", "1650000"),
                ("environment/pdb_hotswap1_power1_scale", "1"),
                ("environment/pdb_hotswap1_power1_max", "3000000000"),
                ("environment/pdb_hotswap2_in1_input", "N/A"),
                ("environment/pdb_hotswap2_curr1_input", "12000"),
                ("environment/pdb_hotswap2_curr1_scale", "x"),
                ("environment/pdb_hotswap2_power1_max", ""),
                ("led/led_power", "green"),
            ],
        );
        root
    }

    #[test]
    fn a_pdb_is_named_and_placed_by_its_index() {
        let _root = switch("x86_64-nvidia_sn6600_ld-r0");
        let pdb = Pdb::new(0);
        assert_eq!(r(pdb.get_name()), "'PDB 1'");
        for v in [pdb.get_model(), pdb.get_serial(), pdb.get_revision()] {
            assert_eq!(r(v), "'N/A'");
        }
        assert_eq!((r(pdb.get_presence()), r(pdb.is_replaceable())), ("True".into(), "False".into()));
        assert_eq!((pdb.py_class(), pdb.base_class()), ("Pdb", BaseClass::PdbBase));
        // Entity-MIB's index comes from the position: each PDB its own.
        assert_eq!([Pdb::new(0), Pdb::new(1)].map(|p| r(p.get_position_in_parent())), ["1", "2"]);
        // No temperature file: no thermal, no temperature.
        assert_eq!((r(pdb.get_all_thermals()), r(pdb.get_temperature())), ("[]".into(), "None".into()));
        // No readings at all.
        for v in [pdb.get_input_voltage(), pdb.get_input_current(), pdb.get_input_power(), pdb.get_maximum_supplied_power()] {
            assert_eq!(r(v), "None");
        }
        assert_eq!((r(pdb.get_status()), r(pdb.get_status_led())), ("False".into(), "'N/A'".into()));
    }

    #[test]
    fn pdb_readings_are_scaled_as_python_scales_them() {
        let root = sn6600_ld();
        let pdb = Pdb::new(0);
        assert_eq!((r(pdb.get_status()), r(pdb.get_powergood_status())), ("True".into(), "True".into()));
        assert_eq!((r(pdb.get_input_voltage()), r(pdb.get_voltage())), ("54.123".into(), "54.123".into()));
        assert_eq!((r(pdb.get_input_current()), r(pdb.get_current())), ("76.25".into(), "76.25".into()));
        assert_eq!((r(pdb.get_input_power()), r(pdb.get_power())), ("1.65".into(), "1.65".into()));
        assert_eq!(r(pdb.get_maximum_supplied_power()), "3000.0");
        assert_eq!(r(pdb.get_temperature()), "41.0");
        let PyVal::List(ts) = pdb.get_all_thermals().unwrap() else { panic!() };
        let PyVal::Obj(t) = &ts[0] else { panic!() };
        assert_eq!((r(t.get_name()), r(t.get_high_threshold()), r(t.get_high_critical_threshold())), ("'PDB-1 Temp'".into(), "85.0".into(), "None".into()));
        // A current with no scale file is read unscaled.
        std::fs::remove_file(root.real("/var/run/hw-management/environment/pdb_hotswap1_curr1_scale")).unwrap();
        assert_eq!(r(pdb.get_input_current()), "30.5");
        // PDB 2: off; what does not parse reads as 0, a bad scale as 1.
        let pdb2 = Pdb::new(1);
        assert_eq!((r(pdb2.get_status()), r(pdb2.get_temperature())), ("False".into(), "None".into()));
        assert_eq!(r(pdb2.get_input_voltage()), "0.0");
        assert_eq!(r(pdb2.get_input_current()), "12.0");
        assert_eq!(r(pdb2.get_input_power()), "None");
        assert_eq!(r(pdb2.get_maximum_supplied_power()), "0.0");
    }

    #[test]
    fn the_power_led_is_what_hw_management_says_or_n_a() {
        let root = sn6600_ld();
        let pdb = Pdb::new(0);
        for (content, want) in [("green", "'green'"), (" none \n", "'N/A'"), ("", "'N/A'"), ("red\n", "'red'"), ("None", "'N/A'")] {
            root.write("/var/run/hw-management/led/led_power", content);
            assert_eq!(r(pdb.get_status_led()), want, "{content:?}");
        }
    }
}
