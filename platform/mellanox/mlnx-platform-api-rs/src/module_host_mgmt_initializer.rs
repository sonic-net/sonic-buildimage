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


//! `sonic_platform/module_host_mgmt_initializer.py`.
//!
//! In module host management mode only xcvrd initialises the modules -- runs
//! each ASIC's ports through the SFP state machine and marks the ASIC done
//! with a ready file; every other process just builds its port objects.

use std::os::unix::fs::PermissionsExt;
use std::sync::Mutex;

use platform_base::io;
use platform_base::py::{PyErr, PyResult};

use crate::chassis::Chassis;
use crate::device_data;
use crate::utils::{self, Log};

const NV_SYNCD_SHARED_DIR: &str = "/tmp/nv-syncd-shared";
const ASIC_READY_FILE_PREFIX: &str = "module_host_mgmt_asic_ready";
const DEDICATE_INIT_DAEMON: &str = "xcvrd";

/// `ASIC_READY_DIR`, decided when the module is imported.
fn asic_ready_dir() -> String {
    if utils::is_host() {
        io::join(NV_SYNCD_SHARED_DIR, "asic_ready")
    } else {
        "/tmp/asic_ready".to_string()
    }
}

/// `get_asic_ready_file_path(asic_id)`: `asic_id` as `'asic0'` or `0`.
pub fn get_asic_ready_file_path(asic_id: &str) -> String {
    let suffix = if asic_id.starts_with("asic") {
        asic_id.to_string()
    } else {
        format!("asic{asic_id}")
    };
    io::join(&asic_ready_dir(), &format!("{ASIC_READY_FILE_PREFIX}_{suffix}"))
}

/// `ModuleHostMgmtInitializer`.
pub struct ModuleHostMgmtInitializer {
    initialized_list: Mutex<Vec<bool>>,
    /// `self.lock`: one initialization at a time.
    lock: Mutex<()>,
}

impl ModuleHostMgmtInitializer {
    /// `ModuleHostMgmtInitializer()`: creates the ready-file directory.
    pub fn new() -> PyResult<ModuleHostMgmtInitializer> {
        let asic_count = device_data::get_asic_count()?;
        let nv_dir_existed = io::exists(NV_SYNCD_SHARED_DIR);
        let dir = asic_ready_dir();
        std::fs::create_dir_all(io::real(&dir)).map_err(|e| PyErr::from_io(&e, Some(&dir)))?;
        // SAFETY: getuid has no preconditions.
        if dir.starts_with(NV_SYNCD_SHARED_DIR) && !nv_dir_existed && unsafe { libc::getuid() } == 0 {
            std::fs::set_permissions(io::real(NV_SYNCD_SHARED_DIR), std::fs::Permissions::from_mode(0o777))
                .map_err(|e| PyErr::from_io(&e, Some(NV_SYNCD_SHARED_DIR)))?;
        }
        Ok(ModuleHostMgmtInitializer {
            initialized_list: Mutex::new(vec![false; usize::try_from(asic_count).unwrap_or(0)]),
            lock: Mutex::new(()),
        })
    }

    /// `initialize(chassis)`.
    pub fn initialize(&self, chassis: &Chassis) -> PyResult<()> {
        let pending = utils::lock(&self.initialized_list).iter().any(|done| !done);
        if !pending {
            return Ok(());
        }
        if utils::is_host() || !is_initialization_owner() {
            return chassis.initialize_sfp();
        }
        let not_initialized: Vec<usize> =
            utils::lock(&self.initialized_list).iter().enumerate().filter(|(_, done)| !**done).map(|(i, _)| i).collect();
        let _guard = utils::lock(&self.lock);
        utils::log_notice("Waiting for modules to be ready...");
        let sfp_count = chassis.get_num_sfps()?;
        if !device_data::wait_sysfs_ready(sfp_count)? {
            if utils::shutdown_requested() {
                utils::log_notice("Module sysfs readiness wait aborted: daemon is shutting down");
                return Ok(());
            }
            utils::log_error("Modules are not ready");
        } else {
            utils::log_notice("Modules are ready");
        }
        utils::log_notice("Starting module initialization for module host management...");
        remove_asics_from_ready_file(&not_initialized)?;
        chassis.initialize_sfp()?;
        let mut asic_ready_list = Vec::new();
        let mut sfp_list = Vec::new();
        for asic_id in not_initialized {
            if utils::read_int(&format!("/var/run/hw-management/config/asic{}_ready", asic_id + 1), 0, Log::Error) == 1 {
                asic_ready_list.push(asic_id);
                sfp_list.extend(chassis.asic_modules(&format!("asic{asic_id}")));
            }
        }
        crate::imports::sfp();
        if !sfp_list.is_empty() {
            crate::sfp_events::initialize_sfp_modules(&sfp_list)?;
            add_asics_to_ready_file(&asic_ready_list)?;
            let mut l = utils::lock(&self.initialized_list);
            for i in &asic_ready_list {
                if let Some(slot) = l.get_mut(*i) {
                    *slot = true;
                }
            }
            utils::log_notice("Module initialization for module host management done");
        }
        Ok(())
    }

    /// `set_asic_ready_value(asic_id, ready)`: an ASIC that is no longer
    /// ready loses its ready file.
    pub fn set_asic_ready_value(&self, asic_id: i128, ready: bool) -> PyResult<()> {
        {
            let mut l = utils::lock(&self.initialized_list);
            let n = l.len() as i128;
            let i = if asic_id < 0 { asic_id + n } else { asic_id };
            if i < 0 || i >= n {
                return Err(PyErr::index_error("list assignment index out of range"));
            }
            l[i as usize] = ready;
        }
        if !ready {
            // The id as given: -1 names `..._asic-1`, as Python's f-string does.
            remove_asics_from_ready_file(&[asic_id])?;
        }
        Ok(())
    }
}

#[cfg(test)]
impl ModuleHostMgmtInitializer {
    /// `initialized_list`.
    pub(crate) fn initialized(&self) -> Vec<bool> {
        self.initialized_list.lock().map(|l| l.clone()).unwrap_or_default()
    }
}

/// `remove_asics_from_ready_file(asic_ids)`.
fn remove_asics_from_ready_file<T: std::fmt::Display>(asic_ids: &[T]) -> PyResult<()> {
    for id in asic_ids {
        let path = get_asic_ready_file_path(&id.to_string());
        match std::fs::remove_file(io::real(&path)) {
            Ok(()) => {}
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => {}
            Err(e) => return Err(PyErr::from_io(&e, Some(&path))),
        }
    }
    Ok(())
}

/// `add_asics_to_ready_file(asic_ids)`.
fn add_asics_to_ready_file(asic_ids: &[usize]) -> PyResult<()> {
    let dir = asic_ready_dir();
    std::fs::create_dir_all(io::real(&dir)).map_err(|e| PyErr::from_io(&e, Some(&dir)))?;
    for id in asic_ids {
        io::write_text(&get_asic_ready_file_path(&id.to_string()), &format!("asic{id}\n"))?;
    }
    Ok(())
}

/// `is_initialization_owner()`: whether `sys.argv[0]` names xcvrd.
fn is_initialization_owner() -> bool {
    let argv0 = platform_base::logger::argv0();
    let cmd = argv0.rsplit('/').next().unwrap_or(&argv0);
    cmd.contains(DEDICATE_INIT_DAEMON)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    #[test]
    fn the_ready_files_live_by_where_the_code_runs() {
        let root = Root::new();
        // No /.dockerenv: the host, under the directory syncd shares.
        assert_eq!(get_asic_ready_file_path("0"), "/tmp/nv-syncd-shared/asic_ready/module_host_mgmt_asic_ready_asic0");
        assert_eq!(get_asic_ready_file_path("asic3"), "/tmp/nv-syncd-shared/asic_ready/module_host_mgmt_asic_ready_asic3");
        drop(root);
        let root = Root::new();
        root.write("/.dockerenv", "");
        assert_eq!(get_asic_ready_file_path("asic1"), "/tmp/asic_ready/module_host_mgmt_asic_ready_asic1");
    }

    #[test]
    fn the_initializer_makes_its_directory_and_tracks_each_asic() {
        let root = Root::new();
        let init = ModuleHostMgmtInitializer::new().unwrap();
        assert!(root.real("/tmp/nv-syncd-shared/asic_ready").is_dir());
        assert_eq!(*init.initialized_list.lock().unwrap(), [false]);
        // Ready: noted.  Not ready: noted, and its ready file goes.
        add_asics_to_ready_file(&[0]).unwrap();
        let file = get_asic_ready_file_path("0");
        assert_eq!(root.read(&file).as_deref(), Some("asic0\n"));
        init.set_asic_ready_value(0, true).unwrap();
        assert_eq!(*init.initialized_list.lock().unwrap(), [true]);
        assert!(root.exists(&file));
        init.set_asic_ready_value(0, false).unwrap();
        assert_eq!(*init.initialized_list.lock().unwrap(), [false]);
        assert!(!root.exists(&file));
        // Gone already: nothing to do.
        init.set_asic_ready_value(0, false).unwrap();
        // A list index, as Python's: -1 is the last, 1 is out of range.
        init.set_asic_ready_value(-1, true).unwrap();
        assert_eq!(*init.initialized_list.lock().unwrap(), [true]);
        assert_eq!(init.set_asic_ready_value(1, true).unwrap_err().str(), "list assignment index out of range");
        assert_eq!(init.set_asic_ready_value(-2, true).unwrap_err().exc.name(), "IndexError");
        // A ready "file" that is a directory cannot be removed: os.remove raises.
        std::fs::create_dir_all(root.real(&file)).unwrap();
        assert_eq!(init.set_asic_ready_value(0, false).unwrap_err().exc.name(), "IsADirectoryError");
    }

    #[test]
    fn only_xcvrd_owns_the_initialization() {
        // sys.argv[0] is the process's: hold the root's lock while it changes.
        let _root = Root::new();
        let before = platform_base::logger::argv0();
        platform_base::logger::set_argv0("/usr/local/bin/xcvrd");
        assert!(is_initialization_owner());
        platform_base::logger::set_argv0("thermalctld");
        assert!(!is_initialization_owner());
        platform_base::logger::set_argv0(&before);
    }
}
