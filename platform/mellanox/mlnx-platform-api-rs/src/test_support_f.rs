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


//! A switch under a scratch root, for the PSU, thermal, PDB and PCIe tests:
//! `/var/run` is `/run`, as on a switch, and machine.conf names the
//! platform.

use crate::testing::Root;

/// Where the platform code finds hw-management.
pub const HW: &str = "/var/run/hw-management";

/// A root shaped like `platform`'s switch, with nothing in it yet.
pub fn switch(platform: &str) -> Root {
    let root = Root::new();
    root.write("/host/machine.conf", &format!("onie_platform={platform}\n"));
    std::fs::create_dir_all(root.dir.join("run/hw-management")).expect("run");
    std::fs::create_dir_all(root.dir.join("var")).expect("var");
    std::os::unix::fs::symlink("../run", root.dir.join("var/run")).expect("var/run");
    root
}

/// Whether the platform the code sees is `platform`: `$PLATFORM`, which
/// device_info reads first, may name another -- and a test then says it
/// skipped (see [`crate::testing::platform_from_env`]).
pub fn is(platform: &str) -> bool {
    if crate::device_data::get_platform_name().unwrap().as_deref() == Some(platform) {
        return true;
    }
    crate::testing::platform_from_env();
    false
}

/// `files` under hw-management, as `(path, content)`.
pub fn hw(root: &Root, files: &[(&str, &str)]) {
    for (f, v) in files {
        root.write(&format!("{HW}/{f}"), v);
    }
}
