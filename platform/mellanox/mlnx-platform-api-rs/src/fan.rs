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


//! `sonic_platform/fan.py`: drawer fans and PSU fans.

use std::process::Command;
use std::sync::{Arc, Weak};

use platform_base::py::{PyErr, PyResult, PyVal};
use platform_base::{io, BaseClass, PyObject};

use crate::fan_drawer::FanDrawer;
use crate::led::ComponentFaultyIndicator;
use crate::psu::Psu;
use crate::utils::{self, floor_div, round_half_even, Log};

pub const PWM_MAX: i128 = 255;
pub const FAN_PATH: &str = "/var/run/hw-management/thermal/";
pub const CONFIG_PATH: &str = "/var/run/hw-management/config";

pub const FAN_DIRECTION_INTAKE: &str = "intake";
pub const FAN_DIRECTION_EXHAUST: &str = "exhaust";
pub const FAN_DIRECTION_NOT_APPLICABLE: &str = "N/A";

/// `FAN_DIR.format(index)`.
pub fn fan_dir(index: i128) -> String {
    format!("/var/run/hw-management/thermal/fan{index}_dir")
}

/// `MlnxFan.get_fan_direction(dir_path)`.
pub fn get_fan_direction(dir_path: &str) -> String {
    match utils::read_int_raise(dir_path, Log::Error) {
        Ok(0) => FAN_DIRECTION_EXHAUST.into(),
        Ok(1) => FAN_DIRECTION_INTAKE.into(),
        Ok(2) => FAN_DIRECTION_NOT_APPLICABLE.into(),
        Ok(v) => {
            utils::log_error(&format!("Got wrong value {v} for fan direction {dir_path}"));
            FAN_DIRECTION_NOT_APPLICABLE.into()
        }
        Err(e) => {
            utils::log_error(&format!("Failed to read fan direction from {dir_path} - {}", e.str()));
            FAN_DIRECTION_NOT_APPLICABLE.into()
        }
    }
}

/// `MlnxFan.get_speed()`: percent of max, or raw RPM if max reads as 0.
fn mlnx_speed(speed_path: &str, max_path: &str) -> PyResult<i128> {
    let rpm = utils::read_int(speed_path, 0, Log::Error);
    let max = utils::read_int(max_path, 0, Log::Error);
    if max == 0 {
        return Ok(rpm);
    }
    Ok(floor_div(100 * rpm, max)?.min(100))
}

/// `Fan`: a fan in a drawer.
pub struct Fan {
    index: i128,
    position: i128,
    drawer: Arc<FanDrawer>,
    led: Arc<ComponentFaultyIndicator>,
    speed_get: String,
    speed_set: String,
    max_speed: String,
    status: String,
}

impl Fan {
    /// `Fan(fan_index, fan_drawer, position)`.
    pub fn new(fan_index: i128, drawer: Arc<FanDrawer>, position: i128) -> Arc<Fan> {
        crate::imports::fan();
        let index = fan_index + 1;
        let led = ComponentFaultyIndicator::new(drawer.led().clone());
        let p = |f: String| io::join(FAN_PATH, &f);
        Arc::new(Fan {
            index,
            position,
            drawer,
            led,
            speed_get: p(format!("fan{index}_speed_get")),
            speed_set: p(format!("fan{index}_speed_set")),
            max_speed: p(format!("fan{index}_max")),
            status: p(format!("fan{index}_fault")),
        })
    }
}

impl PyObject for Fan {
    fn py_class(&self) -> &'static str {
        "Fan"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::FanBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(format!("fan{}", self.index)))
    }
    fn get_speed(&self) -> PyResult<PyVal> {
        mlnx_speed(&self.speed_get, &self.max_speed).map(PyVal::Int)
    }
    fn set_status_led(&self, color: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.led.set_status(color)?))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.led.get_status()?))
    }
    fn get_speed_tolerance(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(50))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.position))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(false))
    }
    fn get_direction(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(if !self.drawer.is_virtual() {
            self.drawer.direction()
        } else {
            get_fan_direction(&fan_dir(self.index))
        }))
    }
    fn get_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(utils::read_int(&self.status, 1, Log::Error) == 0))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.drawer.presence()))
    }
    fn get_target_speed(&self) -> PyResult<PyVal> {
        let mut pwm = utils::read_int(&self.speed_set, 0, Log::Error);
        if !(0..=PWM_MAX).contains(&pwm) {
            utils::log_error(&format!(
                "Fan fan{}: invalid PWM {pwm} read from {}, clamping to [0, {PWM_MAX}]",
                self.index, self.speed_set
            ));
            pwm = pwm.clamp(0, PWM_MAX);
        }
        Ok(PyVal::Int(round_half_even(pwm as f64 * 100.0 / PWM_MAX as f64)))
    }
    fn set_speed(&self, speed: i128) -> PyResult<PyVal> {
        // `int(PWM_MAX*speed/100.0)`: the product is an exact int, rounded to
        // a float once.  Nothing checks the range: a speed of 150 is a PWM
        // of 382.
        let product = PWM_MAX.checked_mul(speed).map_or(PWM_MAX as f64 * speed as f64, |p| p as f64);
        let pwm = (product / 100.0).trunc() as i128;
        Ok(PyVal::Bool(
            utils::write_file(&self.speed_set, &pwm, true, Log::Error).is_ok(),
        ))
    }
}

/// `PsuFan.PSU_FAN_SPEED`.
const PSU_FAN_SPEED: [&str; 11] = [
    "0x3c", "0x3c", "0x3c", "0x3c", "0x3c", "0x3c", "0x3c", "0x46", "0x50", "0x5a", "0x64",
];

/// `PsuFan`: the fan inside a PSU.
pub struct PsuFan {
    index: i128,
    position: i128,
    psu: Weak<Psu>,
    led: Arc<ComponentFaultyIndicator>,
    speed_get: String,
    presence: String,
    max_speed: String,
    min_speed: String,
    i2c_bus: String,
    i2c_addr: String,
    i2c_command: String,
    dir_path: String,
}

impl PsuFan {
    /// `PsuFan(fan_index, position, psu)`.
    pub fn new(fan_index: i128, position: i128, psu: Weak<Psu>) -> Arc<PsuFan> {
        let index = fan_index + 1;
        let led = ComponentFaultyIndicator::new(Psu::get_shared_led());
        let t = |f: String| io::join(FAN_PATH, &f);
        let c = |f: String| io::join(CONFIG_PATH, &f);
        Arc::new(PsuFan {
            index,
            position,
            psu,
            led,
            speed_get: t(format!("psu{index}_fan1_speed_get")),
            presence: t(format!("psu{index}_fan1_speed_get")),
            max_speed: t(format!("psu{index}_fan_max")),
            min_speed: t(format!("psu{index}_fan_min")),
            i2c_bus: c(format!("psu{index}_i2c_bus")),
            i2c_addr: c(format!("psu{index}_i2c_addr")),
            i2c_command: c("fan_command".into()),
            dir_path: t(format!("psu{index}_fan_dir")),
        })
    }

    fn name(&self) -> String {
        format!("psu{}_fan{}", self.index, self.position)
    }

    /// `get_presence()`: the PSU there, powered, and its fan file present.
    fn presence(&self) -> bool {
        let Some(psu) = self.psu.upgrade() else {
            return false;
        };
        psu.presence() && psu.powergood() && io::exists(&self.presence)
    }

    fn speed_tolerance(&self) -> i128 {
        30
    }
}

impl PyObject for PsuFan {
    fn py_class(&self) -> &'static str {
        "PsuFan"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::FanBase
    }
    fn py_mro(&self) -> Vec<&'static str> {
        vec!["PsuFan", "MlnxFan"]
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name()))
    }
    fn set_status_led(&self, color: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.led.set_status(color)?))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.led.get_status()?))
    }
    fn get_speed_tolerance(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.speed_tolerance()))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.position))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(false))
    }
    fn get_direction(&self) -> PyResult<PyVal> {
        if !io::exists(&self.dir_path) || !self.presence() {
            return Ok(PyVal::str(FAN_DIRECTION_NOT_APPLICABLE));
        }
        Ok(PyVal::Str(get_fan_direction(&self.dir_path)))
    }
    fn get_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(true))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.presence()))
    }
    fn is_under_speed(&self) -> PyResult<PyVal> {
        if !self.presence() {
            return Ok(PyVal::Bool(false));
        }
        // In turn, as Python's `try` reads them: a failed speed read ends
        // it before the minimum is read, so only one failure is logged.
        let Ok(rpm) = utils::read_int_raise(&self.speed_get, Log::Error) else {
            return Ok(PyVal::Bool(false));
        };
        let Ok(min) = utils::read_int_raise(&self.min_speed, Log::Error) else {
            return Ok(PyVal::Bool(false));
        };
        Ok(PyVal::Bool(
            (rpm as f64) < min as f64 * (1.0 - self.speed_tolerance() as f64 / 100.0),
        ))
    }
    fn is_over_speed(&self) -> PyResult<PyVal> {
        if !self.presence() {
            return Ok(PyVal::Bool(false));
        }
        let Ok(rpm) = utils::read_int_raise(&self.speed_get, Log::Error) else {
            return Ok(PyVal::Bool(false));
        };
        let Ok(max) = utils::read_int_raise(&self.max_speed, Log::Error) else {
            return Ok(PyVal::Bool(false));
        };
        if max == 0 {
            return Ok(PyVal::Bool(false));
        }
        Ok(PyVal::Bool(
            (rpm as f64) > max as f64 * (1.0 + self.speed_tolerance() as f64 / 100.0),
        ))
    }
    fn get_target_speed(&self) -> PyResult<PyVal> {
        if !self.presence() {
            return Ok(PyVal::Int(0));
        }
        let reads = (|| -> PyResult<(i128, i128, i128)> {
            Ok((
                utils::read_int_raise(&self.speed_get, Log::Error)?,
                utils::read_int_raise(&self.max_speed, Log::Error)?,
                utils::read_int_raise(&self.min_speed, Log::Error)?,
            ))
        })();
        let Ok((rpm, max, min)) = reads else {
            return self.get_speed();
        };
        if max == 0 {
            return self.get_speed();
        }
        let target = if rpm < min {
            min
        } else if rpm > max {
            max
        } else {
            rpm
        };
        Ok(PyVal::Int(floor_div(100 * target, max)?.min(100)))
    }
    fn set_speed(&self, speed: i128) -> PyResult<PyVal> {
        if !self.presence() {
            return Ok(PyVal::Bool(false));
        }
        let r = (|| -> PyResult<bool> {
            let bus = utils::read_str_raise(&self.i2c_bus, Log::Error)?;
            let addr = utils::read_str_raise(&self.i2c_addr, Log::Error)?;
            let command = utils::read_str_raise(&self.i2c_command, Log::Error)?;
            // A list subscript: a negative one counts from the end.
            let mut idx = floor_div(speed, 10)?;
            if idx < 0 {
                idx += PSU_FAN_SPEED.len() as i128;
            }
            let value = usize::try_from(idx)
                .ok()
                .and_then(|i| PSU_FAN_SPEED.get(i))
                .ok_or_else(|| PyErr::index_error("list index out of range"))?;
            let argv = ["i2cset", "-f", "-y", &bus, &addr, &command, value, "wp"];
            let status = Command::new(argv[0]).args(&argv[1..]).status();
            match status {
                Ok(s) if s.success() => Ok(true),
                Ok(s) => {
                    utils::log_error(&format!(
                        "Failed to call command {}, return code={}, command output=None",
                        utils::py_list_repr(&argv),
                        utils::py_returncode(&s)
                    ));
                    Ok(false)
                }
                Err(e) => Err(PyErr::from_io(&e, Some("i2cset"))),
            }
        })();
        match r {
            Ok(b) => Ok(PyVal::Bool(b)),
            Err(e) => {
                utils::log_error(&format!("Failed to set PSU FAN speed - {}", e.str()));
                Ok(PyVal::Bool(false))
            }
        }
    }
    fn get_speed(&self) -> PyResult<PyVal> {
        if !self.presence() {
            utils::log_notice(&format!(
                "No PSU presence detected, returning default value for {}",
                self.name()
            ));
            return Ok(PyVal::Int(0));
        }
        mlnx_speed(&self.speed_get, &self.max_speed).map(PyVal::Int)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    const T: &str = "/var/run/hw-management/thermal";
    const C: &str = "/var/run/hw-management/config";

    fn b(v: PyResult<PyVal>) -> bool {
        v.unwrap().truthy()
    }

    fn s(v: PyResult<PyVal>) -> String {
        v.unwrap().py_str()
    }

    fn i(v: PyResult<PyVal>) -> String {
        v.unwrap().py_repr()
    }

    #[test]
    fn a_fan_direction_is_read_as_hw_management_writes_it() {
        let root = Root::new();
        assert_eq!(get_fan_direction("/x/dir"), FAN_DIRECTION_NOT_APPLICABLE);
        for (v, d) in [("0", FAN_DIRECTION_EXHAUST), ("1", FAN_DIRECTION_INTAKE), ("2", FAN_DIRECTION_NOT_APPLICABLE),
                       ("3", FAN_DIRECTION_NOT_APPLICABLE), ("-1", FAN_DIRECTION_NOT_APPLICABLE), ("x", FAN_DIRECTION_NOT_APPLICABLE)] {
            root.write("/x/dir", v);
            assert_eq!(get_fan_direction("/x/dir"), d, "{v}");
        }
        assert_eq!(fan_dir(3), "/var/run/hw-management/thermal/fan3_dir");
    }

    /// Python's test_system_fan_basic, over files.
    #[test]
    fn a_drawer_fan_reads_its_speed_status_and_target() {
        let root = Root::new();
        let drawer = FanDrawer::new(0, false);
        let fan = Fan::new(2, drawer.clone(), 1);
        assert_eq!((fan.py_class(), fan.base_class()), ("Fan", BaseClass::FanBase));
        assert_eq!(s(fan.get_name()), "fan3");
        assert_eq!(i(fan.get_position_in_parent()), "1");
        assert!(!b(fan.is_replaceable()));
        assert_eq!(i(fan.get_speed_tolerance()), "50");
        let f = |n: &str| format!("{T}/fan3_{n}");
        root.write(&f("speed_get"), "50");
        root.write(&f("max"), "100");
        root.write(&f("fault"), "0");
        root.write(&f("speed_set"), "153");
        assert_eq!(i(fan.get_speed()), "50");
        root.write(&f("speed_get"), "101");
        assert_eq!(i(fan.get_speed()), "100");
        root.write(&f("max"), "0");
        assert_eq!(i(fan.get_speed()), "101");
        assert!(b(fan.get_status()));
        root.write(&f("fault"), "1");
        assert!(!b(fan.get_status()));
        std::fs::remove_file(root.real(&f("fault"))).unwrap();
        assert!(!b(fan.get_status()), "an unreadable fault file is a fault");
        assert_eq!(i(fan.get_target_speed()), "60");
        // An out-of-range PWM is clamped.
        root.write(&f("speed_set"), "300");
        assert_eq!(i(fan.get_target_speed()), "100");
        root.write(&f("speed_set"), "-5");
        assert_eq!(i(fan.get_target_speed()), "0");
        root.write(&f("speed_set"), "128");
        assert_eq!(i(fan.get_target_speed()), "50");
        // The drawer's presence and direction.
        assert!(!b(fan.get_presence()));
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_NOT_APPLICABLE);
        root.write(&format!("{T}/fan1_status"), "1");
        root.write(&format!("{T}/fan1_dir"), "1");
        assert!(b(fan.get_presence()));
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_INTAKE);
        // A fan of fixed fans reads its own direction file.
        let fixed = Fan::new(2, FanDrawer::new(0, true), 1);
        root.write(&f("dir"), "0");
        assert_eq!(s(fixed.get_direction()), FAN_DIRECTION_EXHAUST);
        assert!(b(fixed.get_presence()));
        // Its LED is the drawer's.
        root.write("/var/run/hw-management/led/led_fan1_capability", "green red");
        root.write("/var/run/hw-management/led/led_fan1_green", "0");
        assert!(b(fan.set_status_led("red")));
        assert_eq!(s(fan.get_status_led()), "red");
    }

    #[test]
    fn a_speed_that_cannot_be_written_answers_false() {
        let _root = Root::new();
        let fan = Fan::new(0, FanDrawer::new(0, false), 1);
        assert!(!b(fan.set_speed(60)));
    }

    /// The PSU, there and powered, and its fan's own file.
    fn psu_fan(root: &Root) -> (Arc<Psu>, Arc<PsuFan>) {
        let psu = Psu::new(0);
        let fan = PsuFan::new(0, 1, Arc::downgrade(&psu));
        root.write(&format!("{T}/psu1_status"), "1");
        root.write(&format!("{T}/psu1_pwr_status"), "1");
        root.write(&format!("{T}/psu1_fan1_speed_get"), "15000");
        (psu, fan)
    }

    /// Python's test_psu_fan_basic.
    #[test]
    fn a_psu_fan_is_there_when_its_psu_is_on() {
        let root = Root::new();
        let psu = Psu::new(0);
        let fan = PsuFan::new(0, 1, Arc::downgrade(&psu));
        assert_eq!((fan.py_class(), fan.py_mro()), ("PsuFan", vec!["PsuFan", "MlnxFan"]));
        assert_eq!(fan.base_class(), BaseClass::FanBase);
        assert_eq!(s(fan.get_name()), "psu1_fan1");
        assert_eq!(i(fan.get_speed_tolerance()), "30");
        assert_eq!(i(fan.get_position_in_parent()), "1");
        assert!(!b(fan.is_replaceable()));
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_NOT_APPLICABLE);
        assert!(b(fan.get_status()));
        assert!(!b(fan.get_presence()));
        root.write(&format!("{T}/psu1_status"), "1");
        assert!(!b(fan.get_presence()));
        root.write(&format!("{T}/psu1_pwr_status"), "1");
        assert!(!b(fan.get_presence()));
        root.write(&format!("{T}/psu1_fan1_speed_get"), "100");
        assert!(b(fan.get_presence()));
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_NOT_APPLICABLE, "no direction file");
        root.write(&format!("{T}/psu1_fan_dir"), "1");
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_INTAKE);
        root.write(&format!("{T}/psu1_fan_dir"), "0");
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_EXHAUST);
        root.write(&format!("{T}/psu1_fan_dir"), "-1");
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_NOT_APPLICABLE);
        root.write(&format!("{T}/psu1_pwr_status"), "0");
        assert_eq!(s(fan.get_direction()), FAN_DIRECTION_NOT_APPLICABLE, "not present");
        // A PSU that is gone takes its fan with it.
        let orphan = PsuFan::new(1, 1, Weak::new());
        assert!(!b(orphan.get_presence()));
        drop(psu);
        assert!(!b(fan.get_presence()));
    }

    /// Python's test_psu_fan_is_under_speed / is_over_speed.
    #[test]
    fn a_psu_fan_is_under_or_over_its_rpm_limits_by_30_percent() {
        let root = Root::new();
        let fan = PsuFan::new(0, 1, Weak::new());
        assert!(!b(fan.is_under_speed()) && !b(fan.is_over_speed()));
        let (_psu, fan) = psu_fan(&root);
        let w = |n: &str, v: &str| root.write(&format!("{T}/psu1_{n}"), v);
        // Unreadable limits: neither.
        assert!(!b(fan.is_under_speed()) && !b(fan.is_over_speed()));
        w("fan1_speed_get", "3000");
        w("fan_min", "5000");
        assert!(b(fan.is_under_speed()));
        w("fan1_speed_get", "3500");
        assert!(!b(fan.is_under_speed()));
        w("fan1_speed_get", "0");
        assert!(b(fan.is_under_speed()));
        w("fan1_speed_get", "27000");
        w("fan_max", "20000");
        assert!(b(fan.is_over_speed()));
        w("fan1_speed_get", "26000");
        assert!(!b(fan.is_over_speed()));
        w("fan_max", "0");
        assert!(!b(fan.is_over_speed()));
        w("fan1_speed_get", "x");
        assert!(!b(fan.is_under_speed()) && !b(fan.is_over_speed()));
    }

    /// Python's test_psu_fan_get_target_speed.
    #[test]
    fn a_psu_fan_target_is_its_speed_within_its_limits() {
        let root = Root::new();
        assert_eq!(i(PsuFan::new(0, 1, Weak::new()).get_target_speed()), "0");
        let (_psu, fan) = psu_fan(&root);
        let w = |n: &str, v: &str| root.write(&format!("{T}/psu1_{n}"), v);
        // Unreadable limits: get_speed(), here the raw RPM.
        assert_eq!(i(fan.get_target_speed()), "15000");
        w("fan_min", "10000");
        w("fan_max", "20000");
        assert_eq!(i(fan.get_target_speed()), "75");
        w("fan1_speed_get", "5000");
        assert_eq!(i(fan.get_target_speed()), "50");
        w("fan1_speed_get", "30000");
        assert_eq!(i(fan.get_target_speed()), "100");
        assert_eq!(i(fan.get_speed()), "100");
        w("fan_max", "0");
        w("fan1_speed_get", "8000");
        assert_eq!(i(fan.get_target_speed()), "8000");
        w("fan_max", "60");
        w("fan1_speed_get", "60");
        assert_eq!(i(fan.get_speed()), "100");
    }

    /// Python's test_psu_fan_set_speed: what fails before i2cset runs, and
    /// i2cset failing (or not being there) in this environment.
    #[test]
    fn a_psu_fan_speed_is_set_through_i2c_or_answers_false() {
        let root = Root::new();
        let fan = PsuFan::new(0, 1, Weak::new());
        assert!(!b(fan.set_speed(60)));
        assert_eq!(i(fan.get_speed()), "0");
        let (_psu, fan) = psu_fan(&root);
        // The i2c files are not there.
        assert!(!b(fan.set_speed(60)));
        root.write(&format!("{C}/psu1_i2c_bus"), "bus");
        root.write(&format!("{C}/psu1_i2c_addr"), "addr");
        root.write(&format!("{C}/fan_command"), "command");
        // Past the speed table.
        assert!(!b(fan.set_speed(200)));
        assert!(!b(fan.set_speed(-200)));
        // `i2cset ... bus addr ...` cannot succeed here.
        assert!(!b(fan.set_speed(60)));
        assert!(!b(fan.set_speed(-5)));
        // Its LED is the PSUs' shared one.
        assert_eq!(s(fan.get_status_led()), "off");
        assert!(!b(fan.set_status_led("red")));
    }

    /// `Fan.set_speed` writes `int(255*speed/100.0)`, whatever the speed:
    /// Python 3.13's answers, the product exact before the one rounding.
    #[test]
    fn a_fan_speed_is_written_as_python_scales_it() {
        let root = Root::new();
        let file = format!("{FAN_PATH}fan1_speed_set");
        root.write(&file, "0");
        let fan = Fan::new(0, FanDrawer::new(0, false), 1);
        for (speed, pwm) in [
            (60, "153"),
            (150, "382"),
            (36028797018963971, "91873432398358128"),
            (99999999999999999999, "255000000000000000000"),
        ] {
            assert_eq!(fan.set_speed(speed).unwrap(), PyVal::Bool(true));
            assert_eq!(root.read(&file).as_deref(), Some(pwm), "{speed}");
        }
    }
}
