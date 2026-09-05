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

//! SPC1 thermal file preparation.  Ports `unlink_hw_mgmt_thermal_files()` from
//! `thermal_updater.py:294-318`.
//!
//! On SPC1, hw-management creates the ASIC and per-module thermal files as
//! *symlinks*.  The temperature feed has to replace them with real files, so
//! the updater waits for every one of them to appear and then unlinks them.
//! Writing through a surviving symlink would put the temperature somewhere
//! hw-management-tc does not read.
//!
//! Only SPC1 does this.  Everywhere else the function returns immediately.

use std::path::Path;
use std::time::{Duration, Instant};

use crate::utils::HW_MGMT_THERMAL;

/// Python's `DeviceDataManager.is_spc1()`.
pub fn is_spc1(platform_name: &str) -> bool {
    matches!(platform_name, "x86_64-mlnx_msn2700-r0" | "x86_64-mlnx_msn2700a1-r0")
}

/// Python's `utils.wait_until_conditions(conditions, 300, 1)`.
const WAIT_TIMEOUT: Duration = Duration::from_secs(300);
const WAIT_INTERVAL: Duration = Duration::from_secs(1);

/// Wait for the ASIC and per-module thermal symlinks to appear, then remove
/// them so the feed writes real files.
///
/// A timeout is logged and returns `false`; the daemon carries on rather than
/// blocking start-up, so the rest of the polling still runs.
pub fn prepare_thermal_files(platform_name: &str, sfp_count: usize) -> bool {
    prepare_in(
        platform_name,
        sfp_count,
        Path::new(HW_MGMT_THERMAL),
        WAIT_TIMEOUT,
        WAIT_INTERVAL,
    )
}

fn prepare_in(platform_name: &str, sfp_count: usize, thermal: &Path, timeout: Duration, interval: Duration) -> bool {
    if !is_spc1(platform_name) {
        return true;
    }

    // Every file Python waits for: the ASIC alias plus four per module.
    let mut expected: Vec<String> = vec!["asic".to_string()];
    for i in 1..=sfp_count {
        for suffix in ["temp_input", "temp_fault", "temp_crit", "temp_emergency"] {
            expected.push(format!("module{i}_{suffix}"));
        }
    }

    // Waits for *symlinks*, and the block below replaces them, so a second
    // call in the same boot waits out the timeout and gives up.  Python is the
    // same shape -- `thermal_updater.py:296-311` builds `os.path.islink`
    // conditions and returns after logging the error -- and `initialize()` runs
    // once per process, so making this idempotent would only diverge.
    log::info!("Waiting for ASIC and modules thermal files to be created");
    let deadline = Instant::now() + timeout;
    loop {
        if expected.iter().all(|n| thermal.join(n).is_symlink()) {
            break;
        }
        if Instant::now() >= deadline {
            log::error!("Failed to wait for thermal files to be created");
            return false;
        }
        std::thread::sleep(interval);
    }
    log::info!("All ASIC and modules thermal files are created");

    // Remove every symlink among asic* and module*_temp_*, which is a wider set
    // than the one waited for: asic1, asic2, ... are unlinked too.
    unlink_symlinks(thermal, |n| n.starts_with("asic"));
    unlink_symlinks(thermal, |n| n.starts_with("module") && n.contains("_temp_"));
    true
}

fn unlink_symlinks(dir: &Path, want: impl Fn(&str) -> bool) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().into_owned();
        if !want(&name) {
            continue;
        }
        let path = entry.path();
        if path.is_symlink() {
            if let Err(e) = std::fs::remove_file(&path) {
                log::warn!("Failed to unlink {}: {e}", path.display());
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use std::os::unix::fs::symlink;

    fn tree() -> tempfile::TempDir {
        tempfile::tempdir().unwrap()
    }

    fn link(dir: &tempfile::TempDir, name: &str) {
        let target = dir.path().join(format!("{name}.real"));
        fs::write(&target, "0\n").unwrap();
        symlink(&target, dir.path().join(name)).unwrap();
    }

    #[test]
    fn non_spc1_does_nothing_and_succeeds() {
        let d = tree();
        assert!(prepare_in(
            "x86_64-nvidia_sn5640-r0",
            4,
            d.path(),
            Duration::from_millis(10),
            Duration::from_millis(1)
        ));
        // No wait, no unlink: an unrelated symlink survives.
        link(&d, "asic");
        assert!(d.path().join("asic").is_symlink());
    }

    #[test]
    fn spc1_unlinks_every_thermal_symlink() {
        let d = tree();
        link(&d, "asic");
        link(&d, "asic1");
        for i in 1..=2 {
            for s in ["temp_input", "temp_fault", "temp_crit", "temp_emergency"] {
                link(&d, &format!("module{i}_{s}"));
            }
        }
        assert!(prepare_in(
            "x86_64-mlnx_msn2700-r0",
            2,
            d.path(),
            Duration::from_millis(50),
            Duration::from_millis(1)
        ));
        for n in ["asic", "asic1", "module1_temp_input", "module2_temp_emergency"] {
            assert!(!d.path().join(n).is_symlink(), "{n} still a symlink");
        }
        // The real files behind them are untouched.
        assert!(d.path().join("asic.real").exists());
    }

    #[test]
    fn a_missing_file_times_out_without_unlinking() {
        let d = tree();
        link(&d, "asic");
        // module1_* never appear.
        assert!(!prepare_in(
            "x86_64-mlnx_msn2700-r0",
            1,
            d.path(),
            Duration::from_millis(20),
            Duration::from_millis(1)
        ));
        assert!(d.path().join("asic").is_symlink(), "unlinked despite the timeout");
    }

    #[test]
    fn real_files_are_left_alone() {
        let d = tree();
        // Already-prepared tree: real files, no symlinks anywhere.
        fs::write(d.path().join("asic"), "42000\n").unwrap();
        assert!(!prepare_in(
            "x86_64-mlnx_msn2700-r0",
            1,
            d.path(),
            Duration::from_millis(20),
            Duration::from_millis(1)
        ));
        assert_eq!(fs::read_to_string(d.path().join("asic")).unwrap(), "42000\n");
    }
}
