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


//! `sonic_platform/thermal_manager.py`: no policies of its own; it starts the
//! thermal updater, unless the platform is liquid cooled.  Python starts the
//! SmartSwitch updater on a SmartSwitch, which also feeds the DPUs'
//! temperatures; this starts the same updater everywhere (see README.md).

use std::sync::{Arc, Mutex};

use platform_base::py::{PyResult, PyVal};
use platform_base::{BaseClass, PyObject};

use crate::chassis;
use crate::liquid_cooling::LiquidCooling;
use crate::thermal_updater::ThermalUpdater;
use crate::utils;

/// `ThermalManager.thermal_updater_task`, a class attribute.
static THERMAL_UPDATER_TASK: Mutex<Option<Arc<ThermalUpdater>>> = Mutex::new(None);

/// The `ThermalManager` class, which `get_thermal_manager()` answers and
/// whose classmethods the escape hatch calls.
pub struct ThermalManager;

impl ThermalManager {
    /// `get_thermal_manager()`: `from .thermal_manager import ThermalManager`,
    /// whose own imports include hw-management's helper module.  The helper
    /// is imported here, so this is where the interpreter starts.
    pub fn import() -> PyResult<PyVal> {
        // thermal_manager imports thermal_updater first, whose body imports
        // the helper after its own imports and before its logger.
        crate::imports::sonic_platform_base();
        crate::imports::device_data();
        crate::imports::db_table_helper();
        crate::imports::utils();
        crate::hw_mgmt::import_independent_mode_update()?;
        crate::imports::thermal_manager();
        Ok(PyVal::Obj(Arc::new(ThermalManager)))
    }
}

fn set_task(task: Option<Arc<ThermalUpdater>>) {
    *utils::lock(&THERMAL_UPDATER_TASK) = task;
}

impl PyObject for ThermalManager {
    fn py_class(&self) -> &'static str {
        "ThermalManager"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ThermalManagerBase
    }

    fn run_policy(&self, _chassis: PyVal) -> PyResult<PyVal> {
        Ok(PyVal::None)
    }

    fn initialize(&self) -> PyResult<PyVal> {
        if LiquidCooling::new()?.num_leak_sensors() > 0 {
            utils::log_notice("Liquid cooling platform detected, thermal updater is disabled");
            set_task(None);
            return Ok(PyVal::None);
        }
        crate::imports::chassis();
        let task = ThermalUpdater::new(chassis::instance("get_all_sfps")?.get_all_sfps()?)?;
        set_task(Some(task.clone()));
        task.start()?;
        Ok(PyVal::None)
    }

    fn deinitialize(&self) -> PyResult<PyVal> {
        let task = utils::lock(&THERMAL_UPDATER_TASK).clone();
        if let Some(task) = task {
            task.stop();
        }
        Ok(PyVal::None)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_d::platform;
    use crate::testing::Root;

    fn task() -> Option<Arc<ThermalUpdater>> {
        THERMAL_UPDATER_TASK.lock().unwrap().clone()
    }

    /// Python's own import, run: without the module it fails as Python's
    /// `import` does, and a module that does not load fails with its error.
    #[test]
    fn the_class_needs_hw_management_s_helper_module() {
        let root = Root::new();
        let e = ThermalManager::import().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("ModuleNotFoundError", "No module named 'hw_management_independent_mode_update'".to_string()));
        root.write(crate::hw_mgmt::INDEPENDENT_MODE_UPDATE_PATH, "import nonexistent_dependency\n");
        let e = ThermalManager::import().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("ModuleNotFoundError", "No module named 'nonexistent_dependency'".to_string()));
        crate::hw_mgmt::testing::install_independent_mode_update(&root);
        let PyVal::Obj(tm) = ThermalManager::import().unwrap() else { panic!("no class") };
        assert_eq!((tm.py_class(), tm.base_class()), ("ThermalManager", BaseClass::ThermalManagerBase));
        assert!(tm.run_policy(PyVal::None).unwrap().is_none());
    }

    #[test]
    fn a_liquid_cooled_platform_runs_no_updater() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn6600_ld-r0", r#"{"chassis": {}}"#);
        root.write("/var/run/hw-management/config/leakage_counter", "1");
        root.write("/var/run/hw-management/system/leakage1", "1");
        set_task(None);
        ThermalManager.initialize().unwrap();
        assert!(task().is_none());
        ThermalManager.deinitialize().unwrap();
    }

    /// On a SmartSwitch too, the updater feeds hw-management the ASIC's and
    /// the modules' temperatures only: hw-management's DPU helper is never
    /// imported.
    #[test]
    fn the_updater_runs_over_the_chassis_ports_and_not_its_dpus() {
        let root = Root::new();
        root.write("/run/hw-management/config/suspend", "1");
        let redis = crate::test_support_d::redis_with_chassis_db(&root);
        redis.set(platform_base::testing::CONFIG_DB, "CONFIG_DB_INITIALIZED", "1");
        redis.hset(platform_base::testing::CONFIG_DB, "DEVICE_METADATA|localhost", &[("hwsku", "SKU-D")]);
        let sku = platform_base::device_info::get_hwsku().unwrap();
        root.write(&format!("/usr/share/sonic/platform/{sku}/hwsku.json"), r#"{"interfaces": {}}"#);
        // A fixed switch.
        platform(&root, "x86_64-nvidia_sn5600-r0", r#"{"chassis": {"sfps": []}, "interfaces": {}}"#);
        chassis::platform().unwrap();
        ThermalManager.initialize().unwrap();
        assert!(task().is_some());
        assert_eq!(root.read("/run/hw-management/config/suspend").as_deref(), Some("0"));
        ThermalManager.deinitialize().unwrap();
        assert_eq!(root.read("/run/hw-management/config/suspend").as_deref(), Some("1"));
        // A SmartSwitch: its DPUs too.
        platform(&root, "x86_64-nvidia_sn4280-r0", r#"{"chassis": {"sfps": []}, "interfaces": {}, "DPUS": {"dpu0": {}, "dpu1": {}}}"#);
        crate::utils::forget_caches();
        chassis::platform().unwrap();
        ThermalManager.initialize().unwrap();
        assert!(task().is_some());
        ThermalManager.deinitialize().unwrap();
        use pyo3::prelude::*;
        let dpu_helper_imported = Python::with_gil(|py| {
            py.import("sys").unwrap().getattr("modules").unwrap().contains("hw_management_dpu_thermal_update").unwrap()
        });
        assert!(!dpu_helper_imported);
        assert!(!root.exists("/var/run/hw-management/dpu1/thermal"));
        set_task(None);
        ThermalManager.deinitialize().unwrap();
    }
}
