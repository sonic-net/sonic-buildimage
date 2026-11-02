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


//! What importing each `sonic_platform` module does that anyone can see.
//!
//! Module bodies here only define things, with one exception: most construct a
//! `Logger`, and every `Logger()` calls `openlog`, which re-tags every line the
//! vendor logs from then on (see `platform_base::logger`).  So the identifier
//! on a given line depends on which modules were imported before it -- the
//! thermal manager's import tags lines `thermal_manager` until the next module
//! with a plain `Logger()` is imported.  Each function below is one module's
//! body, as far as that goes: its imports of other modules in source order, and
//! its loggers.  The native code calls these where Python's import statements
//! run, including the ones inside functions.  The order inside each is
//! `generator`-style mechanical: taken from the module's top-level statements.

use std::sync::Arc;

use platform_base::logger::{self, Logger, ModuleLogger};

/// The logger a module body constructed, once imported.
#[cfg(test)]
pub fn module_logger(cell: &ModuleLogger) -> Arc<Logger> {
    cell.get_or_init(|| Arc::new(Logger::new(None))).clone()
}

macro_rules! module {
    ($fn:ident, $name:literal, [$($dep:ident),*], $logger:ident, $ident:expr) => {
        pub static $logger: ModuleLogger = ModuleLogger::new();
        pub fn $fn() {
            logger::import($name, || {
                $($dep();)*
                let _ = $logger.set(Arc::new(Logger::new($ident)));
            });
        }
    };
    ($fn:ident, $name:literal, [$($dep:ident),*]) => {
        pub fn $fn() {
            logger::import($name, || {
                $($dep();)*
            });
        }
    };
}

// sonic_platform_base: the package's `__init__` imports every base module,
// and two of them name their loggers.  sed_mgmt_base is not in the package's
// list, so it is imported on its own, by sonic_platform.sed_mgmt.
pub fn sonic_platform_base() {
    logger::import("sonic_platform_base", platform_base::bmc::import);
}
pub fn sed_mgmt_base() {
    sonic_platform_base();
    platform_base::bodies::sed_mgmt_base::import();
}

module!(utils, "sonic_platform.utils", [], UTILS_LOGGER, None);
module!(device_data, "sonic_platform.device_data", [sonic_platform_base, utils]);
module!(inotify_helper, "sonic_platform.inotify_helper", [utils]);
module!(module_host_mgmt_initializer, "sonic_platform.module_host_mgmt_initializer",
        [utils, device_data], MHMI_LOGGER, None);
module!(sed_mgmt, "sonic_platform.sed_mgmt", [sed_mgmt_base]);
// Its logger is a SysLogger, which takes no part in openlog (dpuctlplat.rs).
module!(dpuctlplat, "sonic_platform.dpuctlplat", [inotify_helper, utils, device_data]);
module!(chassis, "sonic_platform.chassis",
        [sonic_platform_base, inotify_helper, utils, module_host_mgmt_initializer, device_data, sed_mgmt],
        CHASSIS_LOGGER, None);
module!(platform, "sonic_platform.platform", [sonic_platform_base, chassis, device_data]);
module!(led, "sonic_platform.led", [utils, device_data], LED_LOGGER, None);
module!(thermal, "sonic_platform.thermal", [sonic_platform_base, device_data, utils], THERMAL_LOGGER, None);
module!(fan_drawer, "sonic_platform.fan_drawer", [sonic_platform_base, led, utils], FAN_DRAWER_LOGGER, None);
module!(fan, "sonic_platform.fan", [sonic_platform_base, led, utils, thermal, fan_drawer], FAN_LOGGER, None);
module!(vpd_parser, "sonic_platform.vpd_parser", [utils], VPD_PARSER_LOGGER, None);
module!(psu, "sonic_platform.psu", [sonic_platform_base, device_data, led, utils, vpd_parser], PSU_LOGGER, None);
module!(pdb, "sonic_platform.pdb", [sonic_platform_base, utils, thermal], PDB_LOGGER, None);
module!(eeprom, "sonic_platform.eeprom", [sonic_platform_base, device_data, utils], EEPROM_LOGGER, None);
/// liquid_cooling's body calls `logging.basicConfig(level=logging.DEBUG)`.
pub fn liquid_cooling() {
    logger::import("sonic_platform.liquid_cooling", || {
        sonic_platform_base();
        utils();
        logger::basic_config();
    });
}
module!(db_table_helper, "sonic_platform.db_table_helper", []);
module!(thermal_updater, "sonic_platform.thermal_updater", [device_data, db_table_helper, utils],
        THERMAL_UPDATER_LOGGER, Some("thermal-updater"));
module!(smartswitch_thermal_updater, "sonic_platform.smartswitch_thermal_updater", [utils, thermal_updater],
        SMARTSWITCH_THERMAL_UPDATER_LOGGER, Some("smart-switch-thermal-updater"));
module!(thermal_manager, "sonic_platform.thermal_manager",
        [sonic_platform_base, thermal_updater, smartswitch_thermal_updater, device_data, liquid_cooling],
        THERMAL_MANAGER_LOGGER, Some("thermal_manager"));
module!(sfp, "sonic_platform.sfp", [utils, db_table_helper, device_data, module_host_mgmt_initializer],
        SFP_LOGGER, None);
module!(state_machine, "sonic_platform.state_machine", [], STATE_MACHINE_LOGGER, None);
module!(wait_sfp_ready_task, "sonic_platform.wait_sfp_ready_task", [], WAIT_SFP_READY_LOGGER, None);
module!(watchdog, "sonic_platform.watchdog", [sonic_platform_base, utils, device_data]);
module!(component, "sonic_platform.component", [sonic_platform_base, utils], COMPONENT_LOGGER,
        Some("mlnx-platform-api"));
module!(bmc, "sonic_platform.bmc", [sonic_platform_base], BMC_LOGGER, Some("bmc"));

#[cfg(test)]
mod tests {
    use super::*;

    /// Every module body runs once, whatever imports it again, and the
    /// loggers they build are there afterwards.
    #[test]
    fn every_module_imports_once() {
        let all: [fn(); 30] = [
            sonic_platform_base, sed_mgmt_base, utils, device_data, inotify_helper, module_host_mgmt_initializer,
            sed_mgmt, dpuctlplat, chassis, platform, led, thermal, fan_drawer, fan, vpd_parser, psu, pdb, eeprom,
            liquid_cooling, db_table_helper, thermal_updater, smartswitch_thermal_updater, thermal_manager, sfp,
            state_machine, wait_sfp_ready_task, watchdog, component, bmc, utils,
        ];
        for f in all {
            f();
            f();
        }
        for l in [&UTILS_LOGGER, &CHASSIS_LOGGER, &LED_LOGGER, &THERMAL_LOGGER, &FAN_LOGGER, &COMPONENT_LOGGER, &BMC_LOGGER] {
            assert!(l.get().is_some());
        }
        let fresh = ModuleLogger::new();
        let a = module_logger(&fresh);
        assert!(Arc::ptr_eq(&a, &module_logger(&fresh)));
    }
}
