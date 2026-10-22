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


//! The NVIDIA (Mellanox) platform API, natively in Rust.
//!
//! The same platform the vendor's Python `sonic_platform` package describes,
//! answering `platform_api::PlatformApi` the way the PyO3 bridge over that
//! package answers it -- value for value, error for error, side effect for
//! side effect.  A daemon is switched to it with
//! `"platform_api_<daemon>": "native"` in `pmon_daemon_control.json`.
//!
//! [`open`] takes any daemon: every trait method is implemented.

use platform_api::PlatformError;
use platform_base::escape_hatch::EscapeHatches;
use platform_base::{BaseFacade, PyVal};

mod bmc;
mod chassis;
mod component;
mod db_table_helper;
mod device_data;
mod dpuctlplat;
mod eeprom;
mod fan;
mod fan_drawer;
mod imports;
mod inotify_helper;
mod led;
mod linecard;
mod liquid_cooling;
mod module;
mod module_host_mgmt_initializer;
mod pcie;
mod pdb;
mod poll;
mod psu;
mod sed_mgmt;
mod sfp;
mod sfp_events;
#[cfg(test)]
mod testing;
#[cfg(test)]
mod test_support_d;
#[cfg(test)]
mod test_support_f;
mod thermal;
mod thermal_manager;
mod thermal_updater;
mod utils;
mod vpd_parser;
mod wait_sfp_ready_task;
mod watchdog;

/// The native Mellanox platform: the generated facade over this crate's
/// chassis, as the PyO3 bridge is `sonic_platform_api/facade.py` over Python's.
pub type Platform = BaseFacade;

/// Open the platform for `syslog_ident`.
///
/// What the bridge does to open, in the same order: `sys.argv[0]` becomes the
/// identifier the package's loggers tag their lines with, the package is
/// imported, and `Platform()` builds the chassis.
pub fn open(syslog_ident: &str) -> Result<Platform, PlatformError> {
    platform_base::logger::set_argv0(syslog_ident);
    utils::note_main_thread();
    let chassis = chassis::platform().map_err(|e| e.to_platform())?;
    Ok(BaseFacade::new(
        PyVal::Obj(chassis),
        EscapeHatches::with_pcie(pcie::factory).on_shutdown(utils::notify_shutdown),
    ))
}
