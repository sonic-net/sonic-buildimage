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


//! `sonic_platform/fan_drawer.py`.

use std::sync::{Arc, Mutex};

use platform_base::py::{PyResult, PyVal};
use platform_base::{io, BaseClass, PyObject};

use crate::fan::{self, FAN_PATH};
use crate::led::{Led, SharedLed};
use crate::utils::{self, Log};

/// `RealDrawer` (a hot-swappable drawer) or `VirtualDrawer` (fixed fans).
pub struct FanDrawer {
    index: i128,
    is_virtual: bool,
    presence_path: String,
    led: Arc<SharedLed>,
    fans: Mutex<Vec<PyVal>>,
}

impl FanDrawer {
    /// `RealDrawer(index)` or `VirtualDrawer(index)`.
    pub fn new(index: i128, is_virtual: bool) -> Arc<FanDrawer> {
        crate::imports::fan_drawer();
        // `from .fan import FAN_PATH` inside `__init__`.
        crate::imports::fan();
        let index = index + 1;
        let led = SharedLed::new(Led::fan(if is_virtual { None } else { Some(index) }));
        Arc::new(FanDrawer {
            index,
            is_virtual,
            presence_path: io::join(FAN_PATH, &format!("fan{index}_status")),
            led,
            fans: Mutex::new(Vec::new()),
        })
    }

    #[cfg(test)]
    pub fn index(&self) -> i128 {
        self.index
    }

    pub fn is_virtual(&self) -> bool {
        self.is_virtual
    }

    /// `get_led()`.
    pub fn led(&self) -> &Arc<SharedLed> {
        &self.led
    }

    /// `self._fan_list.append(fan)`.
    pub fn add_fan(&self, fan: PyVal) {
        utils::lock(&self.fans).push(fan);
    }

    /// `get_presence()`.
    pub fn presence(&self) -> bool {
        if self.is_virtual {
            return true;
        }
        utils::read_int(&self.presence_path, 0, Log::Error) == 1
    }

    /// `get_direction()`.
    pub fn direction(&self) -> String {
        if !self.presence() {
            return fan::FAN_DIRECTION_NOT_APPLICABLE.into();
        }
        fan::get_fan_direction(&fan::fan_dir(self.index))
    }
}

impl PyObject for FanDrawer {
    fn py_class(&self) -> &'static str {
        if self.is_virtual {
            "VirtualDrawer"
        } else {
            "RealDrawer"
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::FanDrawerBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(if self.is_virtual {
            "N/A".into()
        } else {
            format!("drawer{}", self.index)
        }))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.presence()))
    }
    fn set_status_led(&self, _color: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(true))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.led.get_status()?))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.index))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(!self.is_virtual))
    }
    fn get_all_fans(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(utils::lock(&self.fans).clone()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    /// Python's test_fan_drawer_basic, over files.
    #[test]
    fn a_real_drawer_is_there_when_its_status_says_so() {
        let root = Root::new();
        let d = FanDrawer::new(0, false);
        assert_eq!((d.py_class(), d.base_class(), d.index()), ("RealDrawer", BaseClass::FanDrawerBase, 1));
        assert!(!d.is_virtual());
        assert_eq!(d.get_name().unwrap().py_str(), "drawer1");
        assert_eq!(d.get_position_in_parent().unwrap(), PyVal::Int(1));
        assert_eq!(d.is_replaceable().unwrap(), PyVal::Bool(true));
        assert_eq!(d.get_presence().unwrap(), PyVal::Bool(false));
        assert_eq!(d.direction(), fan::FAN_DIRECTION_NOT_APPLICABLE);
        root.write(&format!("{FAN_PATH}fan1_status"), "0");
        assert!(!d.presence());
        root.write(&format!("{FAN_PATH}fan1_status"), "1");
        assert!(d.presence());
        for (v, dir) in [("0", "exhaust"), ("1", "intake"), ("2", "N/A"), ("7", "N/A"), ("x", "N/A")] {
            root.write(&format!("{FAN_PATH}fan1_dir"), v);
            assert_eq!(d.direction(), dir, "{v}");
        }
        assert_eq!(d.set_status_led("red").unwrap(), PyVal::Bool(true));
        root.write("/var/run/hw-management/led/led_fan1_capability", "green");
        root.write("/var/run/hw-management/led/led_fan1_green", "255");
        assert_eq!(d.get_status_led().unwrap().py_str(), "green");
        assert_eq!(d.get_all_fans().unwrap(), PyVal::List(vec![]));
        d.add_fan(PyVal::str("fan1"));
        assert_eq!(d.get_all_fans().unwrap(), PyVal::List(vec![PyVal::str("fan1")]));
    }

    #[test]
    fn a_virtual_drawer_is_always_there_and_not_replaceable() {
        let root = Root::new();
        let d = FanDrawer::new(0, true);
        assert_eq!(d.py_class(), "VirtualDrawer");
        assert!(d.is_virtual());
        assert_eq!(d.get_name().unwrap().py_str(), "N/A");
        assert!(d.presence());
        assert_eq!(d.is_replaceable().unwrap(), PyVal::Bool(false));
        // Its LED is the plain `fan` one.
        root.write("/var/run/hw-management/led/led_fan_capability", "red");
        root.write("/var/run/hw-management/led/led_fan_red", "255");
        assert_eq!(d.get_status_led().unwrap().py_str(), "red");
        root.write(&format!("{FAN_PATH}fan1_dir"), "1");
        assert_eq!(d.direction(), "intake");
    }
}
