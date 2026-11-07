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


//! `sonic_platform/psu.py`: `FixedPsu`, `Psu` and `InvalidPsuVolWA`.

use std::process::Command;
use std::sync::{Arc, Mutex, OnceLock, Weak};
use std::thread::sleep;
use std::time::Duration;

use platform_base::py::{PyErr, PyResult, PyVal};
use platform_base::{bodies, defaults, io, BaseClass, PyObject};

use crate::device_data;
use crate::fan::PsuFan;
use crate::led::{ComponentFaultyIndicator, Led, SharedLed};
use crate::thermal;
use crate::utils::{self, Log};
use crate::vpd_parser::{VpdParser, MFR_VPD_FIELD};

const PSU_PATH: &str = "/var/run/hw-management/";
const PLATFORM_SENSORS_CONF: &str = "/usr/share/sonic/platform/sensors.conf";
const ETC_SENSORS_CONF: &str = "/etc/sensors.d/sensors.conf";
const PSU_SENSORS_CONF_UPDATER: &str = "/usr/share/sonic/platform/psu_sensors_conf_updater";

fn hw(rel: &str) -> String {
    io::join(PSU_PATH, rel)
}

/// `Psu.shared_led`: class-level, built on first use.
static SHARED_LED: Mutex<Option<Arc<SharedLed>>> = Mutex::new(None);

/// The LED a fixed PSU drives directly, or the indicator a hot-swap PSU votes
/// with on the shared one.  Built on first use, as the `led` property is.
enum PsuLedKind {
    Own(Arc<Led>),
    Voter(Arc<ComponentFaultyIndicator>),
}

/// `FixedPsu` (fixed: true) or `Psu`.
pub struct Psu {
    index: i128,
    fixed: bool,
    led: OnceLock<PsuLedKind>,
    oper_status: String,
    model: Mutex<String>,
    // Psu only, from here.
    voltage_path: Mutex<Option<String>>,
    voltage_in: String,
    current: String,
    current_in: String,
    power: String,
    power_max: String,
    presence_path: String,
    power_max_capacity: String,
    temp: String,
    temp_threshold: String,
    power_slope: String,
    vpd: Option<VpdParser>,
    fans: Vec<PyVal>,
    thermals: Vec<PyVal>,
}

const AMBIENT_TEMP_CRITICAL_THRESHOLD: &str = "/var/run/hw-management/config/amb_tmp_crit_limit";
const AMBIENT_TEMP_WARNING_THRESHOLD: &str = "/var/run/hw-management/config/amb_tmp_warn_limit";
const PORT_AMBIENT_TEMP: &str = "/var/run/hw-management/thermal/port_amb";
const FAN_AMBIENT_TEMP: &str = "/var/run/hw-management/thermal/fan_amb";

/// `get_presence()` over a PSU's files.
fn presence(fixed: bool, presence_path: &str) -> bool {
    fixed || utils::read_int(presence_path, 0, Log::Error) == 1
}

/// `get_powergood_status()` over a PSU's files.
fn powergood(oper_status: &str) -> bool {
    utils::read_int(oper_status, 0, Log::Error) == 1
}

/// `get_power_available_status()` over a PSU's files.
fn power_available(fixed: bool, presence_path: &str, oper_status: &str) -> (bool, String) {
    if !presence(fixed, presence_path) {
        (false, "absence of PSU".into())
    } else if !powergood(oper_status) {
        (false, "absence of power".into())
    } else {
        (true, String::new())
    }
}

impl Psu {
    /// `Psu.get_shared_led()`.
    pub fn get_shared_led() -> Arc<SharedLed> {
        let mut slot = utils::lock(&SHARED_LED);
        slot.get_or_insert_with(|| SharedLed::new(Led::psu(None))).clone()
    }

    /// `Psu.shared_led = None`: a test's PSUs vote on a LED of their own.
    #[cfg(test)]
    pub(crate) fn forget_shared_led() {
        *SHARED_LED.lock().unwrap_or_else(|e| e.into_inner()) = None;
    }

    /// `FixedPsu(psu_index)`.
    pub fn new_fixed(psu_index: i128) -> Arc<Psu> {
        crate::imports::psu();
        bodies::psu_base::init();
        let index = psu_index + 1;
        Arc::new(Psu::blank(index, true))
    }

    fn blank(index: i128, fixed: bool) -> Psu {
        let p = |f: String| hw(&f);
        Psu {
            index,
            fixed,
            led: OnceLock::new(),
            oper_status: p(format!("thermal/psu{index}_pwr_status")),
            model: Mutex::new("N/A".into()),
            voltage_path: Mutex::new(None),
            voltage_in: p(format!("power/psu{index}_volt_in")),
            current: p(format!("power/psu{index}_curr")),
            current_in: p(format!("power/psu{index}_curr_in")),
            power: p(format!("power/psu{index}_power")),
            power_max: p(format!("power/psu{index}_power_max")),
            presence_path: p(format!("thermal/psu{index}_status")),
            power_max_capacity: p(format!("config/psu{index}_power_capacity")),
            temp: p(format!("thermal/psu{index}_temp1")),
            temp_threshold: p(format!("thermal/psu{index}_temp1_max")),
            power_slope: p(format!("config/psu{index}_power_slope")),
            vpd: None,
            fans: Vec::new(),
            thermals: Vec::new(),
        }
    }

    /// `Psu(psu_index)`: its fan, its VPD, its thermal, and its model read now.
    pub fn new(psu_index: i128) -> Arc<Psu> {
        crate::imports::psu();
        bodies::psu_base::init();
        let index = psu_index + 1;
        Arc::new_cyclic(|me: &Weak<Psu>| {
            let mut psu = Psu::blank(index, false);
            // `from .fan import PsuFan`, inside `__init__`.
            crate::imports::fan();
            psu.fans.push(PyVal::Obj(PsuFan::new(psu_index, 1, me.clone())));
            psu.vpd = Some(VpdParser::new(&hw(&format!("eeprom/psu{index}_vpd"))));
            // `from .thermal import initialize_psu_thermal`.
            crate::imports::thermal();
            // `self.get_power_available_status`, bound now: it reads only the
            // PSU's own files, so it answers during construction too.
            let (fixed, presence_path, oper_status) = (psu.fixed, psu.presence_path.clone(), psu.oper_status.clone());
            let presence: thermal::PresenceCb =
                Arc::new(move || power_available(fixed, &presence_path, &oper_status));
            psu.thermals = thermal::initialize_psu_thermal(psu_index, presence);
            let model = psu.vpd.as_ref().map_or_else(|| "N/A".into(), VpdParser::get_model);
            psu.model = Mutex::new(model);
            psu
        })
    }

    #[cfg(test)]
    pub fn index(&self) -> i128 {
        self.index
    }

    /// `get_presence()`.
    pub fn presence(&self) -> bool {
        presence(self.fixed, &self.presence_path)
    }

    /// `get_powergood_status()`.
    pub fn powergood(&self) -> bool {
        powergood(&self.oper_status)
    }

    /// `get_power_available_status()`: bound into the thermal's presence
    /// callback at construction; the method itself only the tests call.
    #[cfg(test)]
    pub fn power_available_status(&self) -> (bool, String) {
        power_available(self.fixed, &self.presence_path, &self.oper_status)
    }

    fn led(&self) -> &PsuLedKind {
        self.led.get_or_init(|| {
            if self.fixed {
                PsuLedKind::Own(Led::psu(Some(self.index)))
            } else {
                PsuLedKind::Voter(ComponentFaultyIndicator::new(Psu::get_shared_led()))
            }
        })
    }

    /// `psu_voltage`: `volt_out2` if it exists, else `volt`; kept once found.
    fn psu_voltage(&self) -> Option<String> {
        let mut v = utils::lock(&self.voltage_path);
        if v.is_none() {
            for f in ["volt_out2", "volt"] {
                let p = hw(&format!("power/psu{}_{f}", self.index));
                if io::exists(&p) {
                    *v = Some(p);
                    break;
                }
            }
        }
        v.clone()
    }

    /// `float(read_int_from_file(path, log_func=log_info)) / div` when
    /// power-good: a failed read is 0, so it answers 0.0, not None.
    fn scaled_when_good(&self, path: &str, div: f64) -> PyVal {
        if self.powergood() {
            PyVal::Float(utils::read_int(path, 0, Log::Info) as f64 / div)
        } else {
            PyVal::None
        }
    }

    fn vpd(&self) -> Option<&VpdParser> {
        self.vpd.as_ref()
    }

    /// `get_voltage_high_threshold()` / `_low_`: `@default_return(None)`.
    fn voltage_threshold(&self, which: &str) -> PyVal {
        let r = (|| -> PyResult<PyVal> {
            let Some(v) = self.psu_voltage() else { return Ok(PyVal::None) };
            let cap = format!("{v}_capability");
            let lim = format!("{v}_{which}");
            if io::exists(&cap) && io::exists(&lim) && self.powergood() {
                let capability = utils::read_str(&cap, "", Log::Error);
                if capability.contains(which) {
                    let value = utils::read_int(&lim, 0, Log::Info);
                    let value = InvalidPsuVolWA::run(self, value, &lim)?;
                    if let Some(value) = value.filter(|v| *v != 0) {
                        return Ok(PyVal::Float(value as f64 / 1000.0));
                    }
                }
            }
            Ok(PyVal::None)
        })();
        let name = if which == "max" { "get_voltage_high_threshold" } else { "get_voltage_low_threshold" };
        utils::default_return(PyVal::None, r, name)
    }

    /// The power thresholds' shared reads.
    fn ambient(&self) -> i128 {
        let fan = utils::read_int(FAN_AMBIENT_TEMP, 0, Log::Error);
        let port = utils::read_int(PORT_AMBIENT_TEMP, 0, Log::Error);
        fan.min(port)
    }
}

impl PyObject for Psu {
    fn py_class(&self) -> &'static str {
        if self.fixed {
            "FixedPsu"
        } else {
            "Psu"
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::PsuBase
    }
    fn py_mro(&self) -> Vec<&'static str> {
        if self.fixed {
            vec!["FixedPsu"]
        } else {
            vec!["Psu", "FixedPsu"]
        }
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(format!("PSU {}", self.index)))
    }
    fn get_model(&self) -> PyResult<PyVal> {
        if self.fixed {
            return Ok(PyVal::str("N/A"));
        }
        let current = self.vpd().map_or_else(|| "N/A".into(), VpdParser::get_model);
        let stored = utils::lock(&self.model).clone();
        if current != stored && io::exists(PSU_SENSORS_CONF_UPDATER) {
            let src = if io::is_file(PLATFORM_SENSORS_CONF) { PLATFORM_SENSORS_CONF } else { ETC_SENSORS_CONF };
            if !io::is_file(src) {
                *utils::lock(&self.model) = current.clone();
                return Ok(PyVal::Str(current));
            }
            let script = format!(
                "source \"{PSU_SENSORS_CONF_UPDATER}\" && update_psu_sensors_configuration \"{src}\" \"{ETC_SENSORS_CONF}\""
            );
            let cmds: [Vec<&str>; 2] = [vec!["bash", "-c", &script], vec!["service", "sensord", "restart"]];
            for argv in &cmds {
                match Command::new(argv[0]).args(&argv[1..]).status() {
                    Ok(s) if s.success() => {}
                    Ok(s) => {
                        utils::log_error(&format!(
                            "Failed to update PSU sensors configuration, return code={}, cmd={}",
                            utils::py_returncode(&s),
                            utils::py_list_repr(argv)
                        ));
                        return Ok(PyVal::Str(current));
                    }
                    Err(e) => {
                        utils::log_error(&format!(
                            "Failed to update PSU sensors configuration - {}",
                            PyErr::from_io(&e, Some(argv[0])).str()
                        ));
                        return Ok(PyVal::Str(current));
                    }
                }
            }
        }
        *utils::lock(&self.model) = current.clone();
        Ok(PyVal::Str(current))
    }
    fn get_serial(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(if self.fixed { "N/A".into() } else { self.vpd().map_or("N/A".into(), VpdParser::get_serial) }))
    }
    fn get_revision(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(if self.fixed { "N/A".into() } else { self.vpd().map_or("N/A".into(), VpdParser::get_revision) }))
    }
    fn get_powergood_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.powergood()))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.presence()))
    }
    fn get_voltage(&self) -> PyResult<PyVal> {
        if self.fixed {
            return Ok(PyVal::None);
        }
        if self.powergood() {
            if let Some(v) = self.psu_voltage() {
                return Ok(PyVal::Float(utils::read_int(&v, 0, Log::Info) as f64 / 1000.0));
            }
        }
        Ok(PyVal::None)
    }
    fn get_current(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.current, 1000.0) })
    }
    fn get_power(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.power, 1_000_000.0) })
    }
    fn set_status_led(&self, color: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(match self.led() {
            PsuLedKind::Own(l) => l.set_status(color)?,
            PsuLedKind::Voter(v) => v.set_status(color)?,
        }))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(if self.fixed {
            match self.led() {
                PsuLedKind::Own(l) => l.get_status()?,
                PsuLedKind::Voter(v) => v.get_status()?,
            }
        } else {
            Psu::get_shared_led().get_status()?
        }))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.index))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(!self.fixed))
    }
    fn get_temperature(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.temp, 1000.0) })
    }
    fn get_temperature_high_threshold(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.temp_threshold, 1000.0) })
    }
    fn get_input_voltage(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.voltage_in, 1000.0) })
    }
    fn get_input_current(&self) -> PyResult<PyVal> {
        Ok(if self.fixed { PyVal::None } else { self.scaled_when_good(&self.current_in, 1000.0) })
    }
    fn get_voltage_high_threshold(&self) -> PyResult<PyVal> {
        if self.fixed {
            return defaults::get_voltage_high_threshold(self);
        }
        Ok(self.voltage_threshold("max"))
    }
    fn get_voltage_low_threshold(&self) -> PyResult<PyVal> {
        if self.fixed {
            return defaults::get_voltage_low_threshold(self);
        }
        Ok(self.voltage_threshold("min"))
    }
    fn get_maximum_supplied_power(&self) -> PyResult<PyVal> {
        if self.fixed {
            return defaults::get_maximum_supplied_power(self);
        }
        Ok(self.scaled_when_good(&self.power_max, 1_000_000.0))
    }
    fn get_psu_power_warning_suppress_threshold(&self) -> PyResult<PyVal> {
        if self.fixed {
            return defaults::get_psu_power_warning_suppress_threshold(self);
        }
        if self.powergood() && io::exists(&self.power_max_capacity) {
            let cap = utils::read_int(&self.power_max_capacity, 0, Log::Error);
            let warn = utils::read_int(AMBIENT_TEMP_WARNING_THRESHOLD, 0, Log::Error);
            let amb = self.ambient();
            let slope = utils::read_int(&self.power_slope, 0, Log::Error) * 1000;
            let mut th = if amb < warn { cap - slope * 1000 } else { cap - (1000 + amb - warn) * slope };
            if th <= 0 {
                utils::log_warning(&format!("Got negative PSU power threshold {th} for PSU {}", self.index));
                th = 0;
            }
            return Ok(PyVal::Float(th as f64 / 1_000_000.0));
        }
        Ok(PyVal::None)
    }
    fn get_psu_power_critical_threshold(&self) -> PyResult<PyVal> {
        if self.fixed {
            return defaults::get_psu_power_critical_threshold(self);
        }
        if self.powergood() && io::exists(&self.power_max_capacity) {
            let cap = utils::read_int(&self.power_max_capacity, 0, Log::Error);
            let crit = utils::read_int(AMBIENT_TEMP_CRITICAL_THRESHOLD, 0, Log::Error);
            let amb = self.ambient();
            let mut th = if amb < crit {
                cap
            } else {
                let slope = utils::read_int(&self.power_slope, 0, Log::Error) * 1000;
                cap - (amb - crit) * slope
            };
            if th <= 0 {
                utils::log_warning(&format!("Got negative PSU power threshold {th} for PSU {}", self.index));
                th = 0;
            }
            return Ok(PyVal::Float(th as f64 / 1_000_000.0));
        }
        Ok(PyVal::None)
    }
    fn get_all_fans(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.fans.clone()))
    }
    fn get_all_thermals(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.thermals.clone()))
    }
}

/// `InvalidPsuVolWA`: the Delta PSU whose voltage limit reads 127998.
struct InvalidPsuVolWA;

impl InvalidPsuVolWA {
    const INVALID: i128 = 127998;
    const PLATFORMS: [&'static str; 4] = [
        "x86_64-mlnx_msn3700-r0",
        "x86_64-mlnx_msn3700c-r0",
        "x86_64-mlnx_msn3800-r0",
        "x86_64-mlnx_msn4600c-r0",
    ];

    /// `run(psu, threshold_value, threshold_file)`: the value, or after the
    /// workaround the re-read one, or None if it did not recover.
    fn run(psu: &Psu, value: i128, file: &str) -> PyResult<Option<i128>> {
        if value != Self::INVALID {
            return Ok(Some(value));
        }
        let platform = device_data::get_platform_name()?;
        if !platform.as_deref().is_some_and(|p| Self::PLATFORMS.contains(&p)) {
            utils::log_warning(&format!(
                "PSU {} threshold file {file} value {value}, but platform is {}",
                psu.index,
                platform.unwrap_or_else(|| "None".into())
            ));
            return Ok(Some(value));
        }
        // A FixedPsu has no vpd_parser at all.
        let vpd = psu.vpd().ok_or_else(|| {
            PyErr::new(
                platform_base::py::Exc::AttributeError,
                vec![PyVal::Str(format!("'{}' object has no attribute 'vpd_parser'", psu.py_class()))],
            )
        })?;
        let vendor = vpd.get_entry_value(MFR_VPD_FIELD);
        if vendor != "N/A" && vendor != "DELTA" {
            utils::log_warning(&format!(
                "PSU {} threshold file {file} value {value}, but its vendor is {vendor}",
                psu.index
            ));
            return Ok(Some(value));
        }
        let capacity = vpd.get_entry_value("CAPACITY");
        if capacity != "N/A" && capacity != "1100" {
            utils::log_warning(&format!(
                "PSU {} threshold file {file} value {value}, but its capacity is {capacity}",
                psu.index
            ));
            return Ok(Some(value));
        }
        utils::run_command(&["sensors", "-s"]);
        let mut wait = 1;
        while wait > 0 {
            let v = utils::read_int(file, 0, Log::Info);
            if v != Self::INVALID {
                return Ok(Some(v));
            }
            wait -= 1;
            sleep(Duration::from_secs(1));
        }
        utils::log_warning("sensors -s does not recover PSU threshold sensor after 1 seconds");
        Ok(None)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_f::{hw, is, switch, HW};

    /// tests/mock_psu_vpd.
    const VPD: &str = "SN_VPD_FIELD: MT1946X07684\nEFT_REV: 1\nPN_VPD_FIELD: MTEF-PSF-AC-C\nREV_VPD_FIELD: A3\n\
                       MFG_DATE_FIELD: 1B94BF\nMFR_NAME: DELTA\nFEED: AC/DC\nCAPACITY: 1100\nMAX_RPM:23000\nMIN_RPM:4600\n";

    /// The VPD file with `content`, its mtime distinct from any before, as
    /// the parser reloads only on a new mtime.
    fn vpd(root: &crate::testing::Root, index: i128, content: &str) {
        use std::sync::atomic::{AtomicU64, Ordering};
        static TICK: AtomicU64 = AtomicU64::new(1_000_000);
        let path = format!("{HW}/eeprom/psu{index}_vpd");
        root.write(&path, content);
        let t = std::time::UNIX_EPOCH + std::time::Duration::from_secs(TICK.fetch_add(1, Ordering::Relaxed));
        std::fs::File::options().write(true).open(root.real(&path)).unwrap().set_modified(t).unwrap();
    }

    fn r(v: PyResult<PyVal>) -> String {
        match v {
            Ok(v) => v.py_repr(),
            Err(e) => format!("raise {}", e.exc.name()),
        }
    }

    /// test_psu's sysfs content for PSU 1, powered.
    fn psu1(root: &crate::testing::Root) {
        hw(
            root,
            &[
                ("thermal/psu1_status", "1"),
                ("thermal/psu1_pwr_status", "1"),
                ("power/psu1_volt", "10234"),
                ("power/psu1_volt_min", "9000"),
                ("power/psu1_volt_max", "12000"),
                ("power/psu1_volt_capability", "min max"),
                ("power/psu1_curr", "20345"),
                ("power/psu1_power", "30456"),
                ("thermal/psu1_temp1", "40567"),
                ("thermal/psu1_temp1_max", "50678"),
                ("power/psu1_volt_in", "102345"),
                ("power/psu1_curr_in", "676"),
                ("power/psu1_power_max", "1234567"),
            ],
        );
        vpd(root, 1, VPD);
    }

    #[test]
    fn a_fixed_psu_answers_as_python_s_fixed_psu() {
        let root = switch("x86_64-mlnx_msn2010-r0");
        let psu = Psu::new_fixed(0);
        assert_eq!(r(psu.get_name()), "'PSU 1'");
        for v in [psu.get_model(), psu.get_serial(), psu.get_revision()] {
            assert_eq!(r(v), "'N/A'");
        }
        assert_eq!(psu.power_available_status(), (false, "absence of power".to_string()));
        hw(&root, &[("thermal/psu1_pwr_status", "1")]);
        assert_eq!(r(psu.get_powergood_status()), "True");
        assert_eq!(psu.power_available_status(), (true, String::new()));
        hw(&root, &[("thermal/psu1_pwr_status", "0")]);
        assert_eq!(r(psu.get_powergood_status()), "False");
        assert_eq!(r(psu.get_presence()), "True");
        for v in [
            psu.get_voltage(),
            psu.get_current(),
            psu.get_power(),
            psu.get_temperature(),
            psu.get_temperature_high_threshold(),
            psu.get_input_voltage(),
            psu.get_input_current(),
        ] {
            assert_eq!(r(v), "None");
        }
        // What PsuBase leaves abstract stays abstract.
        for v in [
            psu.get_voltage_high_threshold(),
            psu.get_voltage_low_threshold(),
            psu.get_maximum_supplied_power(),
            psu.get_psu_power_warning_suppress_threshold(),
            psu.get_psu_power_critical_threshold(),
        ] {
            assert_eq!(r(v), "raise NotImplementedError");
        }
        assert_eq!(r(psu.get_position_in_parent()), "1");
        assert_eq!(r(psu.is_replaceable()), "False");
        assert_eq!((psu.py_class(), psu.py_mro()), ("FixedPsu", vec!["FixedPsu"]));
        assert_eq!(psu.index(), 1);
        assert_eq!(r(psu.get_all_fans()), "[]");
        // Its own LED, psu1.
        assert_eq!(r(psu.get_status_led()), "'off'");
        assert_eq!(r(psu.set_status_led("red")), "False");
        root.write("/var/run/hw-management/led/led_psu1_capability", "none green red orange\n");
        root.write("/var/run/hw-management/led/led_psu1_green", "0");
        root.write("/var/run/hw-management/led/led_psu1_red", "0");
        assert_eq!(r(psu.set_status_led("red")), "True");
        assert_eq!(root.read("/var/run/hw-management/led/led_psu1_red").as_deref(), Some("255"));
        assert_eq!(r(psu.get_status_led()), "'red'");
    }

    #[test]
    fn a_psu_reads_its_sysfs_scaled_only_while_powered() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        psu1(&root);
        let psu = Psu::new(0);
        assert_eq!((psu.py_class(), psu.py_mro()), ("Psu", vec!["Psu", "FixedPsu"]));
        let PyVal::List(fans) = psu.get_all_fans().unwrap() else { panic!() };
        assert_eq!(fans.len(), 1);
        let PyVal::Obj(fan) = &fans[0] else { panic!() };
        assert_eq!(r(fan.get_name()), "'psu1_fan1'");
        assert_eq!(r(psu.get_presence()), "True");
        assert_eq!(r(psu.get_maximum_supplied_power()), "1.234567");
        hw(&root, &[("thermal/psu1_status", "0")]);
        assert_eq!(r(psu.get_presence()), "False");
        assert_eq!(psu.power_available_status(), (false, "absence of PSU".to_string()));
        assert_eq!(r(psu.get_powergood_status()), "True");
        hw(&root, &[("thermal/psu1_pwr_status", "0")]);
        assert_eq!(r(psu.get_powergood_status()), "False");
        for v in [
            psu.get_voltage(),
            psu.get_current(),
            psu.get_power(),
            psu.get_temperature(),
            psu.get_temperature_high_threshold(),
            psu.get_input_voltage(),
            psu.get_input_current(),
            psu.get_maximum_supplied_power(),
            psu.get_voltage_high_threshold(),
            psu.get_psu_power_warning_suppress_threshold(),
            psu.get_psu_power_critical_threshold(),
        ] {
            assert_eq!(r(v), "None");
        }
        hw(&root, &[("thermal/psu1_status", "1"), ("thermal/psu1_pwr_status", "1")]);
        assert_eq!(psu.power_available_status(), (true, String::new()));
        assert_eq!(r(psu.get_voltage()), "10.234");
        assert_eq!(r(psu.get_voltage_high_threshold()), "12.0");
        assert_eq!(r(psu.get_voltage_low_threshold()), "9.0");
        assert_eq!(r(psu.get_current()), "20.345");
        assert_eq!(r(psu.get_power()), "0.030456");
        assert_eq!(r(psu.get_temperature()), "40.567");
        assert_eq!(r(psu.get_temperature_high_threshold()), "50.678");
        assert_eq!(r(psu.get_input_voltage()), "102.345");
        assert_eq!(r(psu.get_input_current()), "0.676");
        assert_eq!(r(psu.get_position_in_parent()), "1");
        assert_eq!(r(psu.is_replaceable()), "True");
        // A reading the file cannot give is 0, so 0.0 -- not None.
        hw(&root, &[("power/psu1_curr", "N/A")]);
        assert_eq!(r(psu.get_current()), "0.0");
        // The PSU's thermal reads the same file, through /run.
        let PyVal::List(thermals) = psu.get_all_thermals().unwrap() else { panic!() };
        let PyVal::Obj(t) = &thermals[0] else { panic!() };
        assert_eq!((t.py_class(), r(t.get_name()), r(t.get_temperature())), ("RemovableThermal", "'PSU-1 Temp'".into(), "40.567".into()));
        hw(&root, &[("thermal/psu1_pwr_status", "0")]);
        assert_eq!(r(t.get_temperature()), "None");
        drop(psu);
        // The PSU gone, its thermal reads as absent.
        assert_eq!(r(t.get_high_threshold()), "None");
    }

    #[test]
    fn the_voltage_thresholds_need_the_capability_and_a_non_zero_limit() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        psu1(&root);
        let psu = Psu::new(0);
        hw(&root, &[("power/psu1_volt_capability", "max")]);
        assert_eq!((r(psu.get_voltage_high_threshold()), r(psu.get_voltage_low_threshold())), ("12.0".into(), "None".into()));
        hw(&root, &[("power/psu1_volt_max", "0")]);
        assert_eq!(r(psu.get_voltage_high_threshold()), "None");
        std::fs::remove_file(root.real(&format!("{HW}/power/psu1_volt_capability"))).unwrap();
        assert_eq!(r(psu.get_voltage_high_threshold()), "None");
        // volt_out2, when there is one, is the output voltage; once found
        // it is kept.
        hw(&root, &[("thermal/psu2_status", "1"), ("thermal/psu2_pwr_status", "1"), ("power/psu2_volt", "12000")]);
        let psu2 = Psu::new(1);
        assert_eq!(r(psu2.get_voltage()), "12.0");
        hw(&root, &[("power/psu2_volt_out2", "12100")]);
        assert_eq!(r(psu2.get_voltage()), "12.0");
        let psu2 = Psu::new(1);
        assert_eq!(r(psu2.get_voltage()), "12.1");
        // No voltage file at all.
        hw(&root, &[("thermal/psu3_status", "1"), ("thermal/psu3_pwr_status", "1")]);
        let psu3 = Psu::new(2);
        assert_eq!((r(psu3.get_voltage()), r(psu3.get_voltage_high_threshold())), ("None".into(), "None".into()));
        // An invalid limit on a platform the workaround is not for stays.
        if is("x86_64-nvidia_sn5600-r0") {
            hw(&root, &[("power/psu1_volt_capability", "min max"), ("power/psu1_volt_max", "127998")]);
            assert_eq!(r(psu.get_voltage_high_threshold()), "127.998");
        }
    }

    #[test]
    fn the_vpd_is_read_again_when_it_comes_back() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        psu1(&root);
        let psu = Psu::new(0);
        assert_eq!((r(psu.get_model()), r(psu.get_serial()), r(psu.get_revision())), ("'MTEF-PSF-AC-C'".into(), "'MT1946X07684'".into(), "'A3'".into()));
        std::fs::remove_file(root.real(&format!("{HW}/eeprom/psu1_vpd"))).unwrap();
        assert_eq!((r(psu.get_model()), r(psu.get_serial()), r(psu.get_revision())), ("'N/A'".into(), "'N/A'".into(), "'N/A'".into()));
        vpd(&root, 1, VPD);
        assert_eq!(r(psu.get_model()), "'MTEF-PSF-AC-C'");
        assert_eq!(psu.vpd().unwrap().get_entry_value("MFR_NAME"), "DELTA");
    }

    #[test]
    fn a_new_model_rebuilds_the_sensors_configuration_when_it_can() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        psu1(&root);
        let psu = Psu::new(0);
        let stored = || psu.model.lock().unwrap().clone();
        assert_eq!(stored(), "MTEF-PSF-AC-C");
        // Another PSU in the slot, an updater, and no sensors.conf at all:
        // the model is taken as it is.
        vpd(&root, 1, "PN_VPD_FIELD: MTEF-PSF-AC-E\n");
        root.write(PSU_SENSORS_CONF_UPDATER, "update_psu_sensors_configuration() { :; }\n");
        assert_eq!(r(psu.get_model()), "'MTEF-PSF-AC-E'");
        assert_eq!(stored(), "MTEF-PSF-AC-E");
        // With a sensors.conf, the updater runs (from the real filesystem,
        // as the shell sees it: there is none here, so it fails) and the
        // stored model stays for the next call to try again.
        vpd(&root, 1, "PN_VPD_FIELD: MTEF-PSF-AC-F\n");
        root.write(PLATFORM_SENSORS_CONF, "chip \"dps460-i2c-*-58\"\n");
        if !std::path::Path::new(PSU_SENSORS_CONF_UPDATER).exists() {
            assert_eq!(r(psu.get_model()), "'MTEF-PSF-AC-F'");
            assert_eq!(stored(), "MTEF-PSF-AC-E");
        }
        // Without the updater, a change is only noted.
        std::fs::remove_file(root.real(PSU_SENSORS_CONF_UPDATER)).unwrap();
        assert_eq!(r(psu.get_model()), "'MTEF-PSF-AC-F'");
        assert_eq!(stored(), "MTEF-PSF-AC-F");
    }

    #[test]
    fn the_invalid_voltage_workaround_applies_to_delta_1100w_psus_only() {
        let root = switch("x86_64-mlnx_msn3700-r0");
        let wa_platform = is("x86_64-mlnx_msn3700-r0");
        psu1(&root);
        let psu = Psu::new(0);
        let file = format!("{HW}/power/psu1_volt_max");
        let run = |v| InvalidPsuVolWA::run(&psu, v, &file).unwrap();
        assert_eq!(run(9999), Some(9999));
        if !wa_platform {
            return;
        }
        // A vendor or a capacity it is not for: the value as read.
        vpd(&root, 1, "MFR_NAME: some psu\nCAPACITY: 1100\n");
        assert_eq!(run(127998), Some(127998));
        vpd(&root, 1, "MFR_NAME: DELTA\nCAPACITY: some capacity\n");
        assert_eq!(run(127998), Some(127998));
        // Delta 1100W, or a VPD that does not say: `sensors -s`, then the
        // file read again.
        vpd(&root, 1, VPD);
        hw(&root, &[("power/psu1_volt_max", "12000")]);
        assert_eq!(run(127998), Some(12000));
        vpd(&root, 1, "PN_VPD_FIELD: X\n");
        assert_eq!(run(127998), Some(12000));
        assert_eq!(r(psu.get_voltage_high_threshold()), "12.0");
        // Still invalid after the wait: None.
        hw(&root, &[("power/psu1_volt_max", "127998")]);
        assert_eq!(run(127998), None);
        assert_eq!(r(psu.get_voltage_high_threshold()), "None");
        // A FixedPsu has no vpd_parser to ask.
        let fixed = Psu::new_fixed(0);
        let e = InvalidPsuVolWA::run(&fixed, 127998, &file).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("AttributeError", "'FixedPsu' object has no attribute 'vpd_parser'".to_string()));
    }

    #[test]
    fn the_power_thresholds_follow_the_ambient_temperature() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        psu1(&root);
        let psu = Psu::new(0);
        // test_psu_not_support_power_threshold: no capacity file.
        assert_eq!((r(psu.get_psu_power_warning_suppress_threshold()), r(psu.get_psu_power_critical_threshold())), ("None".into(), "None".into()));
        hw(
            &root,
            &[
                ("config/psu1_power_capacity", "100000000"),
                ("config/amb_tmp_crit_limit", "65000"),
                ("config/amb_tmp_warn_limit", "55000"),
                ("config/psu1_power_slope", "2"),
            ],
        );
        // test_psu_power_threshold: normal, warning and critical ambients.
        for (port, fan, warning, critical) in
            [("55000", "50000", "98.0", "100.0"), ("65000", "60000", "88.0", "100.0"), ("70000", "75000", "68.0", "90.0")]
        {
            hw(&root, &[("thermal/port_amb", port), ("thermal/fan_amb", fan)]);
            assert_eq!(r(psu.get_psu_power_warning_suppress_threshold()), warning, "{port} {fan}");
            assert_eq!(r(psu.get_psu_power_critical_threshold()), critical, "{port} {fan}");
        }
        // A threshold at or below zero is 0.
        hw(&root, &[("config/psu1_power_slope", "100000")]);
        assert_eq!(r(psu.get_psu_power_warning_suppress_threshold()), "0.0");
        assert_eq!(r(psu.get_psu_power_critical_threshold()), "0.0");
    }

    #[test]
    fn hot_swap_psus_vote_on_one_shared_led() {
        let root = switch("x86_64-nvidia_sn5600-r0");
        Psu::forget_shared_led();
        let led = |c: &str| format!("/var/run/hw-management/led/led_psu_{c}");
        root.write("/var/run/hw-management/led/led_psu_capability", "none green red orange green_blink\n");
        // What the LED driver does: one colour lit at a time.
        let dark = || {
            for c in ["green", "red", "orange"] {
                root.write(&led(c), "0");
            }
        };
        let lit = || ["green", "red", "orange"].into_iter().filter(|c| root.read(&led(c)).as_deref() == Some("255")).collect::<Vec<_>>();
        dark();
        hw(&root, &[("thermal/psu1_status", "1"), ("thermal/psu2_status", "1")]);
        let (p1, p2) = (Psu::new(0), Psu::new(1));
        assert_eq!(r(p1.set_status_led("green")), "True");
        assert_eq!((lit(), r(p1.get_status_led())), (vec!["green"], "'green'".to_string()));
        dark();
        assert_eq!(r(p2.set_status_led("red")), "True");
        assert_eq!((lit(), r(p1.get_status_led())), (vec!["red"], "'red'".to_string()));
        // Green from one PSU does not clear the other's red.
        dark();
        assert_eq!(r(p1.set_status_led("green")), "True");
        assert_eq!(lit(), ["red"]);
        dark();
        assert_eq!(r(p2.set_status_led("green")), "True");
        assert_eq!(lit(), ["green"]);
        // A colour the vote has no rank for is refused, and forgotten.
        dark();
        assert_eq!(r(p1.set_status_led("blue")), "False");
        assert!(lit().is_empty());
        assert_eq!(r(p2.set_status_led("red")), "True");
        assert_eq!(lit(), ["red"]);
        assert!(Arc::ptr_eq(&Psu::get_shared_led(), &Psu::get_shared_led()));
        Psu::forget_shared_led();
    }
}
