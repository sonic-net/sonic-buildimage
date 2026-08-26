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

//! Sysfs read/write primitives for Mellanox/NVIDIA hw-management.

use std::fs;
use std::path::Path;

/// Chassis thermal / fan / config sysfs root.
///
/// On modern kernels `/run` and `/var/run` are the same mount-point;
/// hw-management uses `/run/hw-management` consistently.
pub const HW_MGMT_THERMAL: &str = "/run/hw-management/thermal";
pub const HW_MGMT_CONFIG:  &str = "/run/hw-management/config";

/// Read a sysfs file and parse as `i64`, trimming whitespace.
/// Returns `None` on I/O error or parse failure.
pub fn read_int(path: &str) -> Option<i64> {
    fs::read_to_string(path).ok()?.trim().parse().ok()
}

/// Read a sysfs file and parse as `f64`, trimming whitespace.
/// Returns `None` on I/O error or parse failure.
pub fn read_float(path: &str) -> Option<f64> {
    fs::read_to_string(path).ok()?.trim().parse().ok()
}

/// Read a sysfs file as a trimmed string.
/// Returns `None` on I/O error or if the result is empty.
pub fn read_string(path: &str) -> Option<String> {
    let s = fs::read_to_string(path).ok()?;
    let t = s.trim().to_string();
    if t.is_empty() { None } else { Some(t) }
}

/// Write a value to a sysfs file with a trailing newline.
///
/// Matches Python's `write_file_data()`: `"{}\n".format(value)`.
pub fn write_sysfs(path: &str, value: &str) -> std::io::Result<()> {
    fs::write(path, format!("{value}\n").as_bytes())
}

/// Write to a sysfs file, logging a warning on failure (never panics).
pub fn write_sysfs_log(path: &str, value: &str) {
    if let Err(e) = write_sysfs(path, value) {
        log::warn!("Failed to write '{value}' to {path}: {e}");
    }
}

/// Return `true` if `path` exists on the filesystem.
pub fn exists(path: &str) -> bool {
    Path::new(path).exists()
}

/// Count filesystem entries matching a glob pattern.
pub fn glob_count(pattern: &str) -> usize {
    glob::glob(pattern)
        .map(|paths| paths.filter(|r| r.is_ok()).count())
        .unwrap_or(0)
}

/// Read the platform name from `/etc/sonic/platform` (or the `PLATFORM`
/// environment variable as a fallback for unit tests).
pub fn get_platform_name() -> String {
    if let Some(s) = read_string("/etc/sonic/platform") {
        return s;
    }
    std::env::var("PLATFORM").unwrap_or_default()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write as _;

    #[test]
    fn write_and_read_roundtrip() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("sensor");
        let path_str = path.to_str().unwrap();
        write_sysfs(path_str, "42000").unwrap();
        let got = read_int(path_str).unwrap();
        assert_eq!(got, 42000);
        // Verify trailing newline is present.
        let raw = fs::read_to_string(path_str).unwrap();
        assert!(raw.ends_with('\n'));
    }

    #[test]
    fn glob_count_works() {
        let dir = tempfile::tempdir().unwrap();
        for i in 0..3 {
            let mut f = fs::File::create(dir.path().join(format!("fan{i}_speed_get"))).unwrap();
            let _ = f.write_all(b"1000\n");
        }
        let pattern = format!("{}/fan*_speed_get", dir.path().display());
        assert_eq!(glob_count(&pattern), 3);
    }
}
