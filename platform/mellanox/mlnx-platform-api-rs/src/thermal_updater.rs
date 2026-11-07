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
//! fills -- to hw-management-tc, which drives the fans from them, through
//! hw-management's own helper module (see [`crate::hw_mgmt`]).
//!
//! Python's SmartSwitch updater also feeds the DPUs' temperatures, through
//! hw-management's DPU helper; this one does not (see README.md), so both
//! kinds of switch get the same updater.

use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use platform_base::atexit;
use platform_base::py::{int_of_float, py_float, py_int, PyErr, PyResult, PyVal};
use platform_base::{io, json};
use regex::Regex;

use crate::db_table_helper::{self as dbt, TEMPERATURE_INFO_TABLE};
use crate::device_data;
use crate::hw_mgmt::{self, Val};
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
/// Only the modules' cleanup is guarded; an ASIC's that raises ends it.
fn clean_thermal_data(sfp_list: &[Arc<Sfp>]) -> PyResult<()> {
    for asic_index in 0..device_data::get_asic_count()? {
        hw_mgmt::thermal_data_clean_asic(asic_index)?;
    }
    if sfp_list.is_empty() {
        return Ok(());
    }
    hw_mgmt::module_data_set_module_counter(sfp_list.len() as i128)?;
    for sfp in sfp_list {
        if let Err(e) = hw_mgmt::thermal_data_clean_module(0, sfp.sdk_index + 1) {
            utils::log_warning(&format!("Cleanup skipped for module {}: {}", sfp.sdk_index + 1, e.str()));
        }
    }
    Ok(())
}

/// `ThermalUpdater(sfp_list)`.
pub struct ThermalUpdater {
    sfp_list: Vec<Arc<Sfp>>,
    sfp_status: Mutex<HashMap<i128, bool>>,
    timer: Timer,
    asic_names: Vec<String>,
}

impl ThermalUpdater {
    pub fn new(sfp_list: Vec<Arc<Sfp>>) -> PyResult<Arc<ThermalUpdater>> {
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

    /// `start()`.
    pub fn start(self: &Arc<Self>) -> PyResult<()> {
        control_tc(false);
        self.load_tc_config()?;
        self.unlink_hw_mgmt_thermal_files()?;
        self.timer.start()
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
                hw_mgmt::thermal_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    Val::Int(int_of_float(temperature * TEMPERATURE_SCALE)?),
                    Val::Int(int_of_float(critical * TEMPERATURE_SCALE)?),
                    Val::Int(int_of_float(warning * TEMPERATURE_SCALE)?),
                    Val::Int(fault),
                )?;
                hw_mgmt::vendor_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    &[("manufacturer", vendor_name), ("part_number", part_number)],
                )?;
            } else if pre_presence != Some(presence) {
                // hw-management-tc wants zeros for a module that is gone.
                hw_mgmt::thermal_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    Val::Int(0),
                    Val::Int(0),
                    Val::Int(0),
                    Val::Int(0),
                )?;
                hw_mgmt::vendor_data_set_module(
                    sfp.get_asic_index()?,
                    sfp.sdk_index + 1,
                    &[("manufacturer", String::new()), ("part_number", String::new())],
                )?;
            }
            if pre_presence != Some(presence) {
                utils::lock(&self.sfp_status).insert(sfp.sdk_index, presence);
            }
            Ok(())
        })();
        if let Err(e) = r {
            utils::log_error(&format!("Failed to update module {} thermal data - {}", sfp.sdk_index, e.str()));
            // An RJ45 port has no get_asic_index, and the helper may raise:
            // either raises out of the handler, and out of the timer thread.
            hw_mgmt::thermal_data_set_module(
                sfp.get_asic_index()?,
                sfp.sdk_index + 1,
                Val::Int(0),
                Val::Int(0),
                Val::Int(0),
                Val::Int(ERROR_READ_THERMAL_DATA),
            )?;
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
                hw_mgmt::thermal_data_set_asic(
                    asic_index,
                    temp,
                    Val::Int(ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD),
                    Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                    Val::Int(fault),
                )?;
            }
            Ok(())
        })();
        if let Err(e) = r {
            utils::log_error(&format!("Failed to update ASIC thermal data - {}", e.str()));
            hw_mgmt::thermal_data_set_asic(
                asic_index,
                Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                Val::Int(ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD),
                Val::Int(ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD),
                Val::Int(ERROR_READ_THERMAL_DATA),
            )?;
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
    use crate::hw_mgmt::testing::{fail, take_calls};
    use crate::sfp::Kind;
    use crate::test_support_d::{platform, platform_settable, redis_with_chassis_db};
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, CONFIG_DB, STATE_DB};

    /// A port no other test maps, and its name.
    const SDK: i128 = 200;
    const PORT: &str = "Ethernet9201";
    const CFG: &str = "/var/run/hw-management/config";

    /// Port `SDK` present and in STATE_DB: an SKU without independent
    /// module mode, ASIC 1 ready, the module's page 0, and xcvrd's tables.
    fn present_port(root: &Root) -> (&'static FakeRedis, Arc<Sfp>) {
        platform(root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {"sfps": []}}"#);
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

    /// `thermal_data_set_module` for module 201 on ASIC 0.
    fn module(temperature: i128, warning: i128, critical: i128, fault: i128) -> String {
        format!("('thermal_data_set_module', 0, 201, {temperature}, {warning}, {critical}, {fault})")
    }

    #[test]
    fn a_present_module_s_temperature_and_thresholds_reach_hw_management() {
        let root = Root::new();
        let (redis, sfp) = present_port(&root);
        let u = ThermalUpdater::new(vec![sfp.clone()]).unwrap();
        take_calls();
        u.update_single_module(&sfp).unwrap();
        // Positional, as Python passes them: the critical threshold in the
        // helper's warning slot, the warning in its critical one.
        assert_eq!(
            take_calls(),
            [
                module(40500, 80000, 70000, 0),
                "('vendor_data_set_module', 0, 201, {'manufacturer': 'Mellanox', 'part_number': 'MCP1600'})".to_string()
            ]
        );
        // A warning above the critical threshold is a fault.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_THRESHOLD|{PORT}"), &[("temphighwarning", "90")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(take_calls()[0], module(40500, 80000, 90000, 254000));
        // xcvrd could not read it: zeros, and a fault.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), &[("temperature", "None")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(take_calls()[0], module(0, 0, 0, 254000));
        // Not there yet: zeros, no fault.
        redis.hdel(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), "temperature");
        u.update_single_module(&sfp).unwrap();
        assert_eq!(take_calls()[0], module(0, 0, 0, 0));
        // Unreadable: the error is logged, and only the fault written.
        redis.hset(STATE_DB, &format!("TRANSCEIVER_DOM_TEMPERATURE|{PORT}"), &[("temperature", "hot")]);
        u.update_single_module(&sfp).unwrap();
        assert_eq!(take_calls(), [module(0, 0, 0, 254000)]);
        // Gone: zeros and an empty vendor once, then left alone.
        root.write(&format!("/sys/module/sx_core/asic0/module{SDK}/present"), "0");
        u.update_module().unwrap();
        assert_eq!(
            take_calls(),
            [
                module(0, 0, 0, 0),
                "('vendor_data_set_module', 0, 201, {'manufacturer': '', 'part_number': ''})".to_string()
            ]
        );
        u.update_module().unwrap();
        assert!(take_calls().is_empty());
    }

    /// What hw-management's helper raises is Python's to handle: inside the
    /// `try` it is logged and the fault written; in the `except` it escapes.
    #[test]
    fn what_hw_management_s_helper_raises_is_handled_where_python_handles_it() {
        let root = Root::new();
        let (_redis, sfp) = present_port(&root);
        let u = ThermalUpdater::new(vec![sfp.clone()]).unwrap();
        take_calls();
        fail("vendor_data_set_module", "eeprom directory is gone");
        utils::tests::LOGGED.with(|l| l.borrow_mut().clear());
        u.update_single_module(&sfp).unwrap();
        assert_eq!(take_calls()[2], module(0, 0, 0, 254000));
        assert_eq!(
            utils::tests::LOGGED.with(|l| l.borrow().clone()),
            ["Failed to update module 200 thermal data - eeprom directory is gone"]
        );
        fail("thermal_data_set_module", "thermal directory is gone");
        let e = u.update_single_module(&sfp).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("RuntimeError", "thermal directory is gone".to_string()));
        fail("thermal_data_set_asic", "no ASIC");
        assert_eq!(u.update_asic().unwrap_err().str(), "no ASIC");
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
        take_calls();
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "45.5")]);
        u.update_asic().unwrap();
        // The thresholds go positionally too: the critical first.
        assert_eq!(take_calls(), ["('thermal_data_set_asic', 0, 45500, 120000, 105000, 0)"]);
        assert_eq!(u.get_asic_temp("ASIC"), Some(45500));
        // Not in STATE_DB yet, or N/A: an empty string, which
        // hw-management-tc skips.
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "N/A")]);
        u.update_asic().unwrap();
        assert_eq!(take_calls(), ["('thermal_data_set_asic', 0, '', 120000, 105000, 0)"]);
        redis.hdel(STATE_DB, "TEMPERATURE_INFO|ASIC", "temperature");
        assert_eq!(u.get_asic_temp("ASIC"), Some(0));
        // Unreadable: the warning threshold, and a fault.
        redis.hset(STATE_DB, "TEMPERATURE_INFO|ASIC", &[("temperature", "warm")]);
        assert_eq!(u.get_asic_temp("ASIC"), None);
        u.update_asic().unwrap();
        assert_eq!(take_calls(), ["('thermal_data_set_asic', 0, 105000, 120000, 105000, 254000)"]);
        // A second ASIC the updater was not built for: the first is fed,
        // then the second's slot gets the fault.
        root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        crate::utils::forget_caches();
        u.update_asic().unwrap();
        assert_eq!(
            take_calls(),
            [
                "('thermal_data_set_asic', 0, 105000, 120000, 105000, 254000)",
                "('thermal_data_set_asic', 1, 105000, 120000, 105000, 254000)"
            ]
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
            match u.load_tc_config() {
                Ok(()) => "Ok".to_string(),
                Err(e) => format!("{}: {}", e.exc.name(), e.str()),
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
    }

    #[test]
    fn starting_suspends_hw_management_tc_and_stopping_resumes_it() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {}}"#);
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
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        take_calls();
        clean_thermal_data(&[]).unwrap();
        assert_eq!(take_calls(), ["('thermal_data_clean_asic', 0)"]);
        clean_thermal_data(std::slice::from_ref(&sfp)).unwrap();
        assert_eq!(
            take_calls(),
            ["('thermal_data_clean_asic', 0)", "('module_data_set_module_counter', 1)", "('thermal_data_clean_module', 0, 1)"]
        );
        // A module's cleanup that raises is skipped, with a warning; an
        // ASIC's ends the cleanup.
        fail("thermal_data_clean_module", "not there");
        utils::tests::LOGGED.with(|l| l.borrow_mut().clear());
        clean_thermal_data(std::slice::from_ref(&sfp)).unwrap();
        assert_eq!(utils::tests::LOGGED.with(|l| l.borrow().clone()), ["Cleanup skipped for module 1: not there"]);
        fail("thermal_data_clean_asic", "busy");
        assert_eq!(clean_thermal_data(&[sfp]).unwrap_err().str(), "busy");
        atexit_failed(&PyErr::runtime_error("x"));
        atexit_failed(&PyErr::not_implemented());
    }
}
