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


//! `sonic_platform/thermal_updater.py`: feeds the ASIC and module
//! temperatures in STATE_DB -- the ASIC's in TEMPERATURE_INFO, which
//! thermalctld publishes, the modules' in the TRANSCEIVER tables xcvrd
//! fills -- to hw-management-tc, which drives the fans from them.

use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use hw_mgmt::dpu_thermal_update::{self, Part};
use hw_mgmt::independent_mode_update::{self, Val};
use platform_base::atexit;
use platform_base::py::{int_of_float, py_float, py_int, PyErr, PyResult, PyVal};
use platform_base::{io, json};
use regex::Regex;

use crate::db_table_helper::{self as dbt, TEMPERATURE_INFO_TABLE};
use crate::device_data;
use crate::module::DpuModule;
use crate::sfp::Sfp;
use crate::utils::{self, Log, Timer};

const TEMPERATURE_SCALE: f64 = 1000.0;
const ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD: i128 = 105000;
const ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD: i128 = 120000;
const ERROR_READ_THERMAL_DATA: i128 = 254000;
/// Written while STATE_DB has no ASIC temperature yet: hw-management-tc
/// fails to parse it and recovers, where 0 would latch an emergency.
const ASIC_TEMP_NOT_READY: &str = "";
const TC_CONFIG_FILE: &str = "/run/hw-management/config/tc_config.json";

/// `clean_thermal_data(sfp_list)`, registered with `atexit` per updater.
fn clean_thermal_data(sfp_list: &[Arc<Sfp>]) -> PyResult<()> {
    for asic_index in 0..device_data::get_asic_count()? {
        independent_mode_update::thermal_data_clean_asic(asic_index);
    }
    if sfp_list.is_empty() {
        return Ok(());
    }
    independent_mode_update::module_data_set_module_counter(sfp_list.len() as i128);
    // Python wraps each call in `except Exception` ("Cleanup skipped for
    // module ..."), which never fires: the helper catches everything itself.
    for sfp in sfp_list {
        independent_mode_update::thermal_data_clean_module(0, sfp.sdk_index + 1);
    }
    Ok(())
}

/// `ThermalUpdater(sfp_list)`, or `SmartswitchThermalUpdater(sfp_list,
/// dpu_list)`, which also feeds the DPUs' temperatures.
pub struct ThermalUpdater {
    sfp_list: Vec<Arc<Sfp>>,
    sfp_status: Mutex<HashMap<i128, bool>>,
    timer: Timer,
    asic_names: Vec<String>,
    /// `Some` for the SmartSwitch updater.
    dpus: Option<Vec<Arc<DpuModule>>>,
    dpu_status: Mutex<HashMap<String, &'static str>>,
}

impl ThermalUpdater {
    /// `SmartswitchThermalUpdater(sfp_list, dpu_list)`.
    pub fn new_smartswitch(sfp_list: Vec<Arc<Sfp>>, dpus: Vec<Arc<DpuModule>>) -> PyResult<Arc<ThermalUpdater>> {
        Self::build(sfp_list, Some(dpus))
    }

    pub fn new(sfp_list: Vec<Arc<Sfp>>) -> PyResult<Arc<ThermalUpdater>> {
        Self::build(sfp_list, None)
    }

    fn build(sfp_list: Vec<Arc<Sfp>>, dpus: Option<Vec<Arc<DpuModule>>>) -> PyResult<Arc<ThermalUpdater>> {
        let asic_count = device_data::get_asic_count()?;
        let asic_names = if asic_count > 1 {
            (0..asic_count).map(|i| format!("ASIC{i}")).collect()
        } else {
            vec!["ASIC".to_string()]
        };
        let for_exit = sfp_list.clone();
        atexit::register(move || {
            if let Err(e) = clean_thermal_data(&for_exit) {
                atexit_failed(&e);
            }
        });
        Ok(Arc::new(ThermalUpdater {
            sfp_list,
            sfp_status: Mutex::new(HashMap::new()),
            timer: Timer::new(),
            asic_names,
            dpus,
            dpu_status: Mutex::new(HashMap::new()),
        }))
    }

    /// `load_tc_config()`: the two polling intervals, then their timers.
    fn load_tc_config(self: &Arc<Self>) -> PyResult<()> {
        let mut asic_poll_interval: i128 = 1;
        let mut sfp_poll_interval: i128 = 10;
        let data = utils::load_json_file(TC_CONFIG_FILE, Log::Off).unwrap_or(PyVal::None);
        if !data.truthy() {
            utils::log_error(&format!("{TC_CONFIG_FILE} does not exist, use default polling interval"));
        } else {
            let dev_parameters = dict_get(&data, "dev_parameters")?;
            if !dev_parameters.truthy() {
                utils::log_error("dev_parameters not configured or empty, using default intervals");
            } else {
                // The patterns are raw strings with a doubled backslash, so
                // they match a literal backslash: hw-management's own keys,
                // such as `module\d+`.
                let (asic_key, asic_parameter) = find_matching_key(&dev_parameters, r"asic\\d*")?;
                let asic_key = asic_key.unwrap_or_default();
                // `if asic_parameter is not None`: a key whose value is null
                // is as good as no key.
                match asic_parameter.filter(|p| !p.is_none()) {
                    Some(p) => {
                        let config = dict_get(&p, "poll_time")?;
                        if config.truthy() {
                            asic_poll_interval = int_of(&config)?;
                            utils::log_notice(&format!(
                                "ASIC parameter found with key \"{asic_key}\", poll_time: {}, interval: {asic_poll_interval}",
                                config.py_str()
                            ));
                        } else {
                            utils::log_error(&format!(
                                "ASIC poll_time not configured in \"{asic_key}\", using default interval: {asic_poll_interval}"
                            ));
                        }
                    }
                    None => utils::log_error(&format!(
                        "ASIC parameter not found (pattern: asic\\d*), using default interval: {asic_poll_interval}"
                    )),
                }
                let (module_key, module_parameter) = find_matching_key(&dev_parameters, r"module\\d+")?;
                let module_key = module_key.unwrap_or_default();
                match module_parameter.filter(|p| !p.is_none()) {
                    Some(p) => {
                        let config = dict_get(&p, "poll_time")?;
                        if config.truthy() {
                            sfp_poll_interval = int_of(&config)?;
                            utils::log_notice(&format!(
                                "Module parameter found with key \"{module_key}\", poll_time: {}, interval: {sfp_poll_interval}",
                                config.py_str()
                            ));
                        } else {
                            utils::log_error(&format!(
                                "Module poll_time not configured in \"{module_key}\", using default interval: {sfp_poll_interval}"
                            ));
                        }
                    }
                    None => utils::log_error(&format!(
                        "Module parameter not found (pattern: module\\d+), using default interval: {sfp_poll_interval}"
                    )),
                }
            }
        }
        utils::log_notice(&format!("ASIC polling interval: {asic_poll_interval}"));
        let me = self.clone();
        self.timer.schedule(asic_poll_interval as f64, move || me.update_asic());
        utils::log_notice(&format!("Module polling interval: {sfp_poll_interval}"));
        let me = self.clone();
        self.timer.schedule(sfp_poll_interval as f64, move || me.update_module());
        Ok(())
    }

    /// `start()`: the SmartSwitch updater clears and schedules its DPUs
    /// first, then starts as the plain one does.
    pub fn start(self: &Arc<Self>) -> PyResult<()> {
        if let Some(dpus) = &self.dpus {
            for dpu in dpus {
                thermal_data_dpu_clear(dpu.get_hw_mgmt_id());
            }
            self.load_tc_config_dpu()?;
        }
        control_tc(false);
        self.load_tc_config()?;
        self.unlink_hw_mgmt_thermal_files()?;
        self.timer.start()
    }

    /// How many DPUs a SmartSwitch updater feeds; None for the plain one.
    #[cfg(test)]
    pub(crate) fn dpu_count(&self) -> Option<usize> {
        self.dpus.as_ref().map(Vec::len)
    }

    /// `stop()`.
    pub fn stop(&self) {
        self.timer.stop();
        control_tc(true);
    }

    /// `get_asic_temp(asic_name)`: milli-degrees, 0 when STATE_DB has none,
    /// None when it cannot be read.
    fn get_asic_temp(&self, asic_name: &str) -> Option<i128> {
        let mut temperature = PyVal::None;
        let r = (|| -> PyResult<i128> {
            let (present, value) = dbt::table_hget(TEMPERATURE_INFO_TABLE, asic_name, "temperature")?;
            let value = value.unwrap_or_default();
            temperature = PyVal::Str(value.clone());
            if !present || value == "N/A" {
                return Ok(0);
            }
            int_of_float(py_float(&value)? * TEMPERATURE_SCALE)
        })();
        match r {
            Ok(t) => Some(t),
            Err(e) => {
                utils::log_error(&format!(
                    "Failed to read ASIC {asic_name} temperature - {} - {}",
                    temperature.py_str(),
                    e.str()
                ));
                None
            }
        }
    }

    /// `update_single_module(sfp)`.
    fn update_single_module(&self, sfp: &Sfp) -> PyResult<()> {
        let r = (|| -> PyResult<()> {
            let presence = sfp.get_presence();
            let pre_presence = utils::lock(&self.sfp_status).get(&sfp.sdk_index).copied();
            if presence {
                let mut fault = 0;
                let mut temperature = sfp.get_temperature_from_db()?;
                let (warning, critical);
                if temperature > 0.0 {
                    warning = sfp.get_warning_threshold_from_db()?;
                    critical = sfp.get_critical_threshold_from_db()?;
                    if warning > critical {
                        fault = ERROR_READ_THERMAL_DATA;
                    }
                } else {
                    if temperature == -1.0 {
                        fault = ERROR_READ_THERMAL_DATA;
                    }
                    temperature = 0.0;
                    warning = 0.0;
                    critical = 0.0;
                }
                let vendor_name = sfp.get_vendor_name_from_db()?;
                let part_number = sfp.get_part_number_from_db()?;
                // Positional, as Python passes them: the critical threshold
                // lands in the warning slot and the warning in the critical.
                independent_mode_update::thermal_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    Val::Int(int_of_float(temperature * TEMPERATURE_SCALE)?),
                    Val::Int(int_of_float(critical * TEMPERATURE_SCALE)?),
                    Val::Int(int_of_float(warning * TEMPERATURE_SCALE)?),
                    Val::Int(fault),
                );
                independent_mode_update::vendor_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    &[("manufacturer", vendor_name), ("part_number", part_number)],
                );
            } else if pre_presence != Some(presence) {
                // hw-management-tc wants zeros for a module that is gone.
                independent_mode_update::thermal_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    Val::Int(0),
                    Val::Int(0),
                    Val::Int(0),
                    Val::Int(0),
                );
                independent_mode_update::vendor_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    &[("manufacturer", String::new()), ("part_number", String::new())],
                );
            }
            if pre_presence != Some(presence) {
                utils::lock(&self.sfp_status).insert(sfp.sdk_index, presence);
            }
            Ok(())
        })();
        if let Err(e) = r {
            utils::log_error(&format!("Failed to update module {} thermal data - {}", sfp.sdk_index, e.str()));
            // An RJ45 port has no get_asic_index: this raises out of the
            // handler, and out of the timer thread.
            independent_mode_update::thermal_data_set_module(
                sfp.get_asic_index()?,
                sfp.sdk_index + 1,
                Val::Int(0),
                Val::Int(0),
                Val::Int(0),
                Val::Int(ERROR_READ_THERMAL_DATA),
            );
        }
        Ok(())
    }

    /// `update_module()`.
    fn update_module(&self) -> PyResult<()> {
        for sfp in &self.sfp_list {
            self.update_single_module(sfp)?;
        }
        Ok(())
    }

    /// `update_asic()`.
    fn update_asic(&self) -> PyResult<()> {
        let mut asic_index: i128 = 0;
        let r = (|| -> PyResult<()> {
            for i in 0..device_data::get_asic_count()? {
                asic_index = i;
                let name = usize::try_from(i)
                    .ok()
                    .and_then(|i| self.asic_names.get(i))
                    .ok_or_else(|| PyErr::index_error("list index out of range"))?;
                let mut fault = 0;
                let temp = match self.get_asic_temp(name) {
                    None => {
                        utils::log_error(&format!(
                            "Failed to read ASIC {asic_index} temperature, send fault to hw-management-tc"
                        ));
                        fault = ERROR_READ_THERMAL_DATA;
                        Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD)
                    }
                    Some(0) => Val::Str(ASIC_TEMP_NOT_READY.to_string()),
                    Some(t) => Val::Int(t),
                };
                independent_mode_update::thermal_data_set_asic(
                    asic_index,
                    temp,
                    Val::Int(ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD),
                    Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                    Val::Int(fault),
                );
            }
            Ok(())
        })();
        if let Err(e) = r {
            utils::log_error(&format!("Failed to update ASIC thermal data - {}", e.str()));
            independent_mode_update::thermal_data_set_asic(
                asic_index,
                Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                Val::Int(ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD),
                Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                Val::Int(ERROR_READ_THERMAL_DATA),
            );
        }
        Ok(())
    }

    /// `unlink_hw_mgmt_thermal_files()`: on SPC1 hw-management first links
    /// the thermal files to its own sensors; the updater replaces them.
    fn unlink_hw_mgmt_thermal_files(&self) -> PyResult<()> {
        if !device_data::is_spc1()? {
            return Ok(());
        }
        let mut conditions: Vec<Box<dyn FnMut() -> bool>> =
            vec![Box::new(|| io::is_link("/run/hw-management/thermal/asic"))];
        for sfp_index in 0..device_data::get_sfp_count()? {
            let idx = sfp_index + 1;
            for suffix in ["temp_input", "temp_fault", "temp_crit", "temp_emergency"] {
                let path = format!("/run/hw-management/thermal/module{idx}_{suffix}");
                conditions.push(Box::new(move || io::is_link(&path)));
            }
        }
        utils::log_notice("Waiting for ASIC and modules thermal files to be created");
        if !utils::wait_until_conditions(conditions, 300.0, 1.0) {
            utils::log_error("Failed to wait for thermal files to be created");
            return Ok(());
        }
        utils::log_notice("All ASIC and modules thermal files are created");
        for pattern in ["/run/hw-management/thermal/asic*", "/run/hw-management/thermal/module*_temp_*"] {
            for f in utils::glob(pattern) {
                if io::is_link(&f) {
                    std::fs::remove_file(io::real(&f)).map_err(|e| PyErr::from_io(&e, Some(&f)))?;
                }
            }
        }
        Ok(())
    }
}

impl ThermalUpdater {
    /// `load_tc_config_dpu()`: half the configured `dpu\d+_module` poll
    /// time -- a float -- or 3.
    fn load_tc_config_dpu(self: &Arc<Self>) -> PyResult<()> {
        let mut interval = PyVal::Int(3);
        let data = utils::load_json_file(TC_CONFIG_FILE, Log::Off).unwrap_or(PyVal::None);
        if data.truthy() {
            let dev_parameters = dict_get_or(&data, "dev_parameters", PyVal::Dict(Vec::new()))?;
            let dpu_parameter = dict_get_or(&dev_parameters, "dpu\\d+_module", PyVal::Dict(Vec::new()))?;
            let config = dict_get(&dpu_parameter, "poll_time")?;
            if config.truthy() {
                interval = PyVal::Float(int_of(&config)? as f64 / 2.0);
            }
        } else {
            utils::log_notice(&format!("{TC_CONFIG_FILE} does not exist, use default polling interval"));
        }
        utils::log_notice(&format!("DPU polling interval: {}", interval.py_str()));
        let secs = match interval {
            PyVal::Float(f) => f,
            PyVal::Int(i) => i as f64,
            _ => 3.0,
        };
        let me = self.clone();
        self.timer.schedule(secs, move || me.update_dpu());
        Ok(())
    }

    /// `update_dpu()`.
    fn update_dpu(&self) -> PyResult<()> {
        for dpu in self.dpus.iter().flatten() {
            self.update_single_dpu(dpu);
        }
        Ok(())
    }

    /// `update_single_dpu(dpu)`.
    fn update_single_dpu(&self, dpu: &DpuModule) {
        let r = (|| -> PyResult<()> {
            let status = dpu.oper_status()?;
            let pre = utils::lock(&self.dpu_status).get(dpu.name()).copied();
            if status == "Online" {
                self.update_dpu_temperature(dpu, false)?;
            } else if pre != Some(status) {
                thermal_data_dpu_clear(dpu.get_hw_mgmt_id());
            }
            if pre != Some(status) {
                utils::lock(&self.dpu_status).insert(dpu.name().to_string(), status);
            }
            Ok(())
        })();
        if let Err(e) = r {
            utils::log_error(&format!(
                "Failed to update DPU {} thermal data - {}",
                dpu.get_hw_mgmt_id(),
                e.str()
            ));
            let _ = self.update_dpu_temperature(dpu, true);
        }
    }

    /// `update_dpu_temperature(dpu, fault_state)`: CPU, NVME and DDR, in
    /// that order.
    fn update_dpu_temperature(&self, dpu: &DpuModule, fault_state: bool) -> PyResult<()> {
        let data = if fault_state { Vec::new() } else { dpu.get_temperature_dict() };
        for (key, part) in [("CPU", Part::CpuCore), ("NVME", Part::Drive), ("DDR", Part::Ddr)] {
            let fields = data.iter().find(|(k, _)| *k == key).map(|(_, f)| f.as_slice()).unwrap_or(&[]);
            let mut fault = false;
            let mut values = [0i128; 3];
            for (i, field) in ["temperature", "high_threshold", "critical_high_threshold"].iter().enumerate() {
                let (v, f) = dpu_temperature_value(fields, field)?;
                values[i] = v;
                fault |= f;
            }
            let fault = if fault { ERROR_READ_THERMAL_DATA } else { 0 };
            if !dpu_thermal_update::thermal_data_set(part, dpu.get_hw_mgmt_id(), values[0], values[1], values[2], fault) {
                utils::log_error(&format!(
                    "Unable to update Temperature data to hw-mgmt for {key} for {}",
                    dpu.name()
                ));
            }
        }
        Ok(())
    }
}

/// `get_dpu_temperature_data_from_dict_obj(data, field_name)`: the value as
/// an int, and whether it was missing or unreadable.  An infinite reading
/// raises `OverflowError`, which nothing here catches.
fn dpu_temperature_value(fields: &[(String, String)], field: &str) -> PyResult<(i128, bool)> {
    let value = fields.iter().find(|(k, _)| k == field).map(|(_, v)| v.as_str()).unwrap_or("");
    if value.is_empty() {
        return Ok((0, true));
    }
    match py_float(value).and_then(int_of_float) {
        Ok(v) => Ok((v, false)),
        Err(e) if e.is(&platform_base::Exc::ValueError) => {
            utils::log_error(&format!("Unable to obtain temperature data for DPU {field}: {value}"));
            Ok((0, true))
        }
        Err(e) => Err(e),
    }
}

/// `thermal_data_dpu_clear(dpu_index)`.
fn thermal_data_dpu_clear(dpu_index: i128) {
    dpu_thermal_update::thermal_data_clear(Part::CpuCore, dpu_index);
    dpu_thermal_update::thermal_data_clear(Part::Ddr, dpu_index);
    dpu_thermal_update::thermal_data_clear(Part::Drive, dpu_index);
}

/// `d.get(key, default)` where `d` came from JSON.
fn dict_get_or(d: &PyVal, key: &str, default: PyVal) -> PyResult<PyVal> {
    match d {
        PyVal::Dict(_) => Ok(json::get(d, key).cloned().unwrap_or(default)),
        other => Err(PyErr::missing(&other.type_name(), "get")),
    }
}

/// `control_tc(suspend)`: hw-management-tc runs only while the updater
/// does not.
fn control_tc(suspend: bool) {
    utils::log_notice(&format!(
        "Set hw-management-tc to {}",
        if suspend { "suspend" } else { "resume" }
    ));
    let _ = utils::write_file("/run/hw-management/config/suspend", &i32::from(suspend), false, Log::Error);
}

/// `_find_matching_key(dev_parameters, pattern)`: the first key `re.match`
/// accepts, and its value.
fn find_matching_key(dev_parameters: &PyVal, pattern: &str) -> PyResult<(Option<String>, Option<PyVal>)> {
    let PyVal::Dict(items) = dev_parameters else {
        return Err(PyErr::missing(&dev_parameters.type_name(), "keys"));
    };
    let re = Regex::new(&format!("^(?:{pattern})")).map_err(|e| PyErr::runtime_error(e.to_string()))?;
    for (k, v) in items {
        let key = k.py_str();
        if re.is_match(&key) {
            return Ok((Some(key), Some(v.clone())));
        }
    }
    Ok((None, None))
}

/// `d.get(key)` where `d` came from JSON and need not be a dict.
fn dict_get(d: &PyVal, key: &str) -> PyResult<PyVal> {
    match d {
        PyVal::Dict(_) => Ok(json::get(d, key).cloned().unwrap_or(PyVal::None)),
        other => Err(PyErr::missing(&other.type_name(), "get")),
    }
}

/// `int(x)` on a JSON value.
fn int_of(v: &PyVal) -> PyResult<i128> {
    match v {
        PyVal::Int(i) => Ok(*i),
        PyVal::Bool(b) => Ok(i128::from(*b)),
        PyVal::Float(f) => int_of_float(*f),
        PyVal::Str(s) => py_int(s),
        other => Err(PyErr::type_error(format!(
            "int() argument must be a string, a bytes-like object or a real number, not '{}'",
            other.type_name()
        ))),
    }
}

/// What CPython prints for an `atexit` callback that raised.
pub fn atexit_failed(e: &PyErr) {
    use std::io::Write;
    let last = match e.str() {
        s if s.is_empty() => e.exc.name().to_string(),
        s => format!("{}: {s}", e.exc.name()),
    };
    let _ = writeln!(
        std::io::stderr(),
        "Exception ignored in atexit callback: functools.partial(<function clean_thermal_data>)\nTraceback (most recent call last):\n{last}"
    );
}

#[cfg(test)]
mod tests {
    use super::*;

    fn dict(items: &[(&str, PyVal)]) -> PyVal {
        PyVal::Dict(items.iter().map(|(k, v)| (PyVal::str(*k), v.clone())).collect())
    }

    /// test_thermal_updater.py::test_find_matching_key: the patterns match a
    /// literal backslash, which is how hw-management spells its keys.
    #[test]
    fn matching_keys_take_the_backslash_literally() {
        let poll = |n: i128| dict(&[("poll_time", PyVal::Int(n))]);
        let params = dict(&[("asic\\d*", poll(3)), ("module\\d+", poll(20)), ("sensor_amb", poll(30))]);
        let (k, v) = find_matching_key(&params, r"asic\\d*").unwrap();
        assert_eq!(k.as_deref(), Some("asic\\d*"));
        assert!(v.unwrap().py_eq(&poll(3)));
        let (k, v) = find_matching_key(&params, r"module\\d+").unwrap();
        assert_eq!(k.as_deref(), Some("module\\d+"));
        assert!(v.unwrap().py_eq(&poll(20)));
        let (k, v) = find_matching_key(&params, "nonexistent").unwrap();
        assert!(k.is_none() && v.is_none());
        let (k, v) = find_matching_key(&dict(&[]), r"asic\\d*").unwrap();
        assert!(k.is_none() && v.is_none());
        // A plain `asic` key, as some tc_config.json files have it, is not
        // what the pattern finds.
        let (k, _) = find_matching_key(&dict(&[("asic", poll(2))]), r"asic\\d*").unwrap();
        assert!(k.is_none());
    }

    #[test]
    fn dev_parameters_that_are_not_a_dict_raise_as_python_does() {
        let e = find_matching_key(&PyVal::List(vec![]), r"asic\\d*").unwrap_err();
        assert_eq!(e.str(), "'list' object has no attribute 'keys'");
    }
}

#[cfg(test)]
mod updater_tests {
    use super::*;
    use crate::sfp::Kind;
    use crate::test_support_d::{platform, platform_settable, redis_with_chassis_db};
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, CONFIG_DB, STATE_DB};

    /// A port no other test maps, and its name.
    const SDK: i128 = 200;
    const PORT: &str = "Ethernet9201";
    const T: &str = "/var/run/hw-management/thermal";
    const CFG: &str = "/var/run/hw-management/config";

    /// hw-management's files the helpers write into, for one ASIC.
    fn hw_management(root: &Root, asics: i128) {
        root.write(&format!("{CFG}/asic_num"), &asics.to_string());
        root.write(&format!("{CFG}/module_counter"), "256");
        root.write(&format!("{T}/.keep"), "");
        root.write("/var/run/hw-management/eeprom/.keep", "");
    }

    /// Port `SDK` present and in STATE_DB: an SKU without independent
    /// module mode, ASIC 1 ready, the module's page 0, and xcvrd's tables.
    fn present_port(root: &Root) -> (&'static FakeRedis, Arc<Sfp>) {
        platform(root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {"sfps": []}}"#);
        hw_management(root, 1);
        let redis = redis_with_chassis_db(root);
        redis.set(CONFIG_DB, "CONFIG_DB_INITIALIZED", "1");
        redis.hset(CONFIG_DB, "DEVICE_METADATA|localhost", &[("hwsku", "SKU-D")]);
        let sku = platform_base::device_info::get_hwsku().unwrap();
        root.write(&format!("/usr/share/sonic/platform/{sku}/sai.profile"), "SAI_INIT_CONFIG_FILE=/x\n");
        root.write(&format!("{CFG}/asic1_ready"), "1");
        let m = format!("/sys/module/sx_core/asic0/module{SDK}");
        root.write(&format!("{m}/present"), "1");
        root.write(&format!("{m}/eeprom/pages/0/i2c-0x50/data"), "\x18");
        redis.set(0, "PORT_TABLE:PortConfigDone", "1");
        redis.hset(CONFIG_DB, &format!("PORT|{PORT}"), &[("index", "201")]);
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), &[("temperature", "40.5")]);
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_THRESHOLD|{PORT}"), &[("temphighwarning", "70"), ("temphighalarm", "80")]);
        redis.hset(STATE_DB, &format!("TRANSCEIVER_INFO|{PORT}"), &[("manufacturer", " Mellanox "), ("model", "MCP1600")]);
        (redis, Sfp::new(Kind::Sfp, SDK, "asic0").unwrap())
    }

    /// `module201_temp_<x>` for crit, input, emergency, fault.
    fn module_files(root: &Root) -> Vec<Option<String>> {
        ["crit", "input", "emergency", "fault"].iter().map(|x| root.read(&format!("{T}/module201_temp_{x}"))).collect()
    }

    fn some(v: &[&str]) -> Vec<Option<String>> {
        v.iter().map(|s| Some(format!("{s}\n"))).collect()
    }

    #[test]
    fn a_present_module_s_temperature_and_thresholds_reach_hw_management() {
        let root = Root::new();
        let (redis, sfp) = present_port(&root);
        let u = ThermalUpdater::new(vec![sfp.clone()]).unwrap();
        u.update_single_module(&sfp).unwrap();
        // Positional, as Python passes them: the warning in temp_crit.
        assert_eq!(module_files(&root), some(&["70000", "40500", "80000", "0"]));
        assert_eq!(
            root.read("/var/run/hw-management/eeprom/module201_data").as_deref(),
            Some("Manufacturer             : Mellanox\nPN                       : MCP1600\n")
        );
        // A warning above the critical threshold is a fault.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_THRESHOLD|{PORT}"), &[("temphighwarning", "90")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(module_files(&root), some(&["90000", "40500", "80000", "254000"]));
        // xcvrd could not read it: zeros, and a fault.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), &[("temperature", "None")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(module_files(&root), some(&["0", "0", "0", "254000"]));
        // Not there yet: zeros, no fault.
        redis.hdel(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), "temperature");
        u.update_single_module(&sfp).unwrap();
        assert_eq!(module_files(&root), some(&["0", "0", "0", "0"]));
        // Unreadable: the error is logged, and the fault written.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), &[("temperature", "hot")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(module_files(&root), some(&["0", "0", "0", "254000"]));
        // Gone: zeros and no vendor once, then left alone.
        root.write(&format!("/sys/module/sx_core/asic0/module{SDK}/present"), "0");
        u.update_module().unwrap();
        assert_eq!(module_files(&root), some(&["0", "0", "0", "0"]));
        assert_eq!(
            root.read("/var/run/hw-management/eeprom/module201_data").as_deref(),
            Some("Manufacturer             : \nPN                       : \n")
        );
        std::fs::remove_file(root.real(&format!("{T}/module201_temp_input"))).unwrap();
        u.update_module().unwrap();
        assert_eq!(root.read(&format!("{T}/module201_temp_input")), None);
    }

    #[test]
    fn an_rj45_port_has_no_asic_index_and_its_error_escapes() {
        let root = Root::new();
        let (_redis, _) = present_port(&root);
        root.write("/sys/module/sx_core/asic0/module201/present", "1");
        let rj45 = Sfp::new(Kind::Rj45, 201, "asic0").unwrap();
        let u = ThermalUpdater::new(vec![rj45.clone()]).unwrap();
        let e = u.update_module().unwrap_err();
        assert_eq!(e.str(), "'RJ45Port' object has no attribute 'get_asic_index'");
    }

    #[test]
    fn the_asic_temperature_reaches_hw_management_or_says_it_is_not_ready() {
        let root = Root::new();
        let (redis, _) = present_port(&root);
        let u = ThermalUpdater::new(Vec::new()).unwrap();
        let asic = || ["asic_temp_crit", "asic", "asic_temp_emergency", "asic_temp_fault", "asic1"].map(|f| root.read(&format!("{T}/{f}")));
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "45.5")]);
        u.update_asic().unwrap();
        assert_eq!(asic().to_vec(), some(&["105000", "45500", "120000", "0", "45500"]));
        assert_eq!(u.get_asic_temp("ASIC"), Some(45500));
        // Not in STATE_DB yet, or N/A: empty, which hw-management-tc skips.
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "N/A")]);
        u.update_asic().unwrap();
        assert_eq!(asic()[1].as_deref(), Some("\n"));
        redis.hdel(STATE_DB, "TEMPERATURE_INFO|ASIC", "temperature");
        assert_eq!(u.get_asic_temp("ASIC"), Some(0));
        // Unreadable: the warning threshold, and a fault.
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "warm")]);
        assert_eq!(u.get_asic_temp("ASIC"), None);
        u.update_asic().unwrap();
        assert_eq!(asic().to_vec(), some(&["105000", "105000", "120000", "254000", "105000"]));
        // A second ASIC the updater was not built for: its slot gets the fault.
        root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        root.write(&format!("{CFG}/asic_num"), "2");
        crate::utils::forget_caches();
        u.update_asic().unwrap();
        assert_eq!(
            ["asic2_temp_crit", "asic2", "asic2_temp_emergency", "asic2_temp_fault"].map(|f| root.read(&format!("{T}/{f}"))).to_vec(),
            some(&["105000", "105000", "120000", "254000"])
        );
    }

    #[test]
    fn two_asics_are_named_by_index() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {}}"#);
        root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        let u = ThermalUpdater::new(Vec::new()).unwrap();
        assert_eq!(u.asic_names, ["ASIC0", "ASIC1"]);
    }

    #[test]
    fn tc_config_sets_the_polling_intervals_or_raises_as_python_does() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {}}"#);
        let tc = "/run/hw-management/config/tc_config.json";
        let load = |json: Option<&str>| {
            match json {
                Some(j) => root.write(tc, j),
                None => {
                    let _ = std::fs::remove_file(root.real(tc));
                }
            }
            let u = ThermalUpdater::new(Vec::new()).unwrap();
            match (u.load_tc_config(), u.load_tc_config_dpu()) {
                (Ok(()), Ok(())) => "Ok".to_string(),
                (Err(e), _) | (_, Err(e)) => format!("{}: {}", e.exc.name(), e.str()),
            }
        };
        assert_eq!(load(None), "Ok");
        assert_eq!(load(Some("{}")), "Ok");
        assert_eq!(load(Some(r#"{"dev_parameters": {}}"#)), "Ok");
        assert_eq!(
            load(Some(r#"{"dev_parameters": {"asic\\d*": {"poll_time": 3}, "module\\d+": {"poll_time": "20"}, "dpu\\d+_module": {"poll_time": 6}}}"#)),
            "Ok"
        );
        assert_eq!(load(Some(r#"{"dev_parameters": {"asic\\d*": {}, "module\\d+": {"poll_time": 0}}}"#)), "Ok");
        assert_eq!(load(Some(r#"{"dev_parameters": {"sensor_amb": {"poll_time": 30}}}"#)), "Ok");
        // A matching key whose value is null: not found, the defaults kept.
        assert_eq!(load(Some(r#"{"dev_parameters": {"asic\\d*": null, "module\\d+": null}}"#)), "Ok");
        assert_eq!(
            load(Some(r#"{"dev_parameters": {"asic\\d*": {"poll_time": "fast"}}}"#)),
            "ValueError: invalid literal for int() with base 10: 'fast'"
        );
        assert_eq!(
            load(Some(r#"{"dev_parameters": {"module\\d+": 5}}"#)),
            "AttributeError: 'int' object has no attribute 'get'"
        );
        assert_eq!(load(Some(r#"{"dev_parameters": [1]}"#)), "AttributeError: 'list' object has no attribute 'keys'");
        assert_eq!(load(Some(r#"{"dev_parameters": 5}"#)), "AttributeError: 'int' object has no attribute 'keys'");
        assert_eq!(int_of(&PyVal::Float(2.9)).unwrap(), 2);
        assert_eq!(int_of(&PyVal::Bool(true)).unwrap(), 1);
        assert_eq!(
            int_of(&PyVal::None).unwrap_err().str(),
            "int() argument must be a string, a bytes-like object or a real number, not 'NoneType'"
        );
        assert_eq!(dict_get_or(&PyVal::Int(1), "x", PyVal::None).unwrap_err().str(), "'int' object has no attribute 'get'");
    }

    #[test]
    fn starting_suspends_hw_management_tc_and_stopping_resumes_it() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {}}"#);
        hw_management(&root, 1);
        root.write("/run/hw-management/config/suspend", "1");
        let u = ThermalUpdater::new(Vec::new()).unwrap();
        u.start().unwrap();
        assert_eq!(root.read("/run/hw-management/config/suspend").as_deref(), Some("0"));
        assert_eq!(u.start().unwrap_err().str(), "threads can only be started once");
        u.stop();
        assert_eq!(root.read("/run/hw-management/config/suspend").as_deref(), Some("1"));
    }

    #[test]
    fn spc1_s_own_thermal_links_are_replaced() {
        if !platform_settable() {
            return;
        }
        let root = Root::new();
        platform(&root, "x86_64-mlnx_msn2700-r0", r#"{"chassis": {"sfps": [{"name": "sfp1"}]}}"#);
        let th = "/run/hw-management/thermal";
        root.write(&format!("{th}/asic1"), "45000");
        for f in ["asic", "module1_temp_input", "module1_temp_fault", "module1_temp_crit", "module1_temp_emergency"] {
            std::os::unix::fs::symlink("/sys/x", root.real(&format!("{th}/{f}"))).unwrap();
        }
        let u = ThermalUpdater::new(Vec::new()).unwrap();
        u.unlink_hw_mgmt_thermal_files().unwrap();
        let mut left = utils::glob(&format!("{th}/*"));
        left.sort();
        assert_eq!(left, [format!("{th}/asic1")]);
    }

    /// On SPC1 `start()` waits up to 300 s for hw-management to link the
    /// thermal files.  A shutdown ends that wait at once, logged as Python
    /// logs one that timed out, and nothing is unlinked.
    #[test]
    fn a_shutdown_ends_spc1_s_wait_for_the_thermal_links() {
        if !platform_settable()
            || !crate::testing::in_own_process("thermal_updater::updater_tests::a_shutdown_ends_spc1_s_wait_for_the_thermal_links")
        {
            return;
        }
        let root = Root::new();
        platform(&root, "x86_64-mlnx_msn2700-r0", r#"{"chassis": {"sfps": [{"name": "sfp1"}]}}"#);
        let th = "/run/hw-management/thermal";
        root.write(&format!("{th}/asic1"), "45000");
        // The ASIC's link is there; the module's never come.
        std::os::unix::fs::symlink("/sys/x", root.real(&format!("{th}/asic"))).unwrap();
        utils::note_main_thread();
        utils::watch_shutdown_signals();
        let u = ThermalUpdater::new(Vec::new()).unwrap();

        std::thread::spawn(|| {
            std::thread::sleep(std::time::Duration::from_millis(200));
            utils::notify_shutdown();
        });
        utils::tests::LOGGED.with(|l| l.borrow_mut().clear());
        let started = std::time::Instant::now();
        u.unlink_hw_mgmt_thermal_files().unwrap();
        assert!(started.elapsed() < std::time::Duration::from_secs(10), "{:?}", started.elapsed());
        assert_eq!(
            utils::tests::LOGGED.with(|l| l.borrow().clone()),
            ["Waiting for ASIC and modules thermal files to be created", "Failed to wait for thermal files to be created"]
        );
        assert!(io::is_link(&format!("{th}/asic")));
    }

    #[test]
    fn at_exit_the_asic_and_module_files_are_removed() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {}}"#);
        hw_management(&root, 1);
        for f in ["asic", "asic_temp_crit", "asic1", "asic1_temp_fault", "module1_temp_input", "module1_temp_crit"] {
            root.write(&format!("{T}/{f}"), "1");
        }
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        clean_thermal_data(&[]).unwrap();
        assert_eq!(root.read(&format!("{T}/asic")), None);
        assert_eq!(root.read(&format!("{T}/module1_temp_input")).as_deref(), Some("1"));
        clean_thermal_data(&[sfp]).unwrap();
        assert_eq!(root.read(&format!("{CFG}/module_counter")).as_deref(), Some("1"));
        assert_eq!(root.read(&format!("{T}/module1_temp_input")), None);
        atexit_failed(&PyErr::runtime_error("x"));
        atexit_failed(&PyErr::not_implemented());
    }

    /// DPU0 booted to `boot`, and hw-management's DPU helper in place.
    fn dpu0(root: &Root, boot: &str) -> (&'static FakeRedis, Arc<DpuModule>) {
        platform(root, "x86_64-nvidia_sn4280-r0", r#"{"DPUS": {"dpu0": {"bus_info": "0000:08:00.0"}}}"#);
        hw_management(root, 1);
        root.write(dpu_thermal_update::MODULE_PATH, "");
        root.write(&format!("{CFG}/dpu_num"), "4");
        root.write("/var/run/hw-management/dpu1/system/boot_progress", boot);
        let redis = redis_with_chassis_db(root);
        (redis, DpuModule::new(0))
    }

    /// The CPU's four files, if hw-management's DPU helper was found.
    fn cpu(root: &Root) -> [Option<String>; 4] {
        ["cpu_pack", "cpu_pack_max", "cpu_pack_crit", "cpu_pack_fault"].map(|f| root.read(&format!("/var/run/hw-management/dpu1/thermal/{f}")))
    }

    /// Whether the helper module was there when first looked for; without it
    /// Python's updater calls MagicMocks and nothing is written.
    fn helper_writes(root: &Root) -> bool {
        dpu_thermal_update::thermal_data_set(Part::CpuCore, 1, 1, 1, 1, 1) && root.read("/var/run/hw-management/dpu1/thermal/cpu_pack").is_some()
    }

    #[test]
    fn dpu_temperatures_reach_hw_management_while_the_dpu_runs() {
        let root = Root::new();
        let (redis, dpu) = dpu0(&root, "5");
        let writes = helper_writes(&root);
        redis.hset(13, "TEMPERATURE_INFO_0|CPU", &[("temperature", "55.5"), ("high_threshold", "95"), ("critical_high_threshold", "105")]);
        redis.hset(13, "TEMPERATURE_INFO_0|DDR", &[("temperature", "cool"), ("high_threshold", "85")]);
        let u = ThermalUpdater::new_smartswitch(Vec::new(), vec![dpu.clone()]).unwrap();
        u.update_dpu().unwrap();
        if writes {
            assert_eq!(cpu(&root), [Some("55".into()), Some("95".into()), Some("105".into()), Some("0".into())]);
            assert_eq!(root.read("/var/run/hw-management/dpu1/thermal/sodimm_temp_fault").as_deref(), Some("254000"));
            assert_eq!(root.read("/var/run/hw-management/dpu1/thermal/drivetemp").as_deref(), Some("0"));
        }
        // Offline: cleared once.
        root.write("/var/run/hw-management/dpu1/system/boot_progress", "0");
        u.update_dpu().unwrap();
        assert_eq!(cpu(&root), [None, None, None, None]);
        assert_eq!(u.dpu_status.lock().unwrap().get("DPU0").copied(), Some("Offline"));
        // An infinite reading raises past the reader: a fault.
        root.write("/var/run/hw-management/dpu1/system/boot_progress", "5");
        redis.hset(13, "TEMPERATURE_INFO_0|CPU", &[("temperature", "inf")]);
        u.update_dpu().unwrap();
        if writes {
            assert_eq!(cpu(&root)[3].as_deref(), Some("254000"));
        }
        // hw-management's helper refusing the DPU's index: logged.
        std::fs::remove_file(root.real(&format!("{CFG}/dpu_num"))).unwrap();
        redis.hset(13, "TEMPERATURE_INFO_0|CPU", &[("temperature", "50")]);
        u.update_dpu().unwrap();
        root.write(&format!("{CFG}/dpu_num"), "4");
        // No boot progress: the status read fails, a fault is written.
        std::fs::remove_file(root.real("/var/run/hw-management/dpu1/system/boot_progress")).unwrap();
        u.update_dpu().unwrap();
        if writes {
            assert_eq!(cpu(&root), [Some("0".into()), Some("0".into()), Some("0".into()), Some("254000".into())]);
        }
    }

    #[test]
    fn a_smartswitch_updater_clears_its_dpus_before_starting() {
        let root = Root::new();
        let (_redis, dpu) = dpu0(&root, "0");
        root.write("/run/hw-management/config/suspend", "0");
        let writes = helper_writes(&root);
        let u = ThermalUpdater::new_smartswitch(Vec::new(), vec![dpu]).unwrap();
        u.start().unwrap();
        u.stop();
        if writes {
            assert_eq!(cpu(&root), [None, None, None, None]);
        }
        assert_eq!(root.read("/run/hw-management/config/suspend").as_deref(), Some("1"));
        assert_eq!(dpu_temperature_value(&[("t".into(), "12.9".into())], "t").unwrap(), (12, false));
    }
}
