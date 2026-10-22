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


//! A platform under the scratch root, for the SFP and change-event tests:
//! machine.conf, the platform directory with its platform.json, CONFIG_DB on
//! the fake redis with a hwsku, the hwsku's sai.profile, and hw-management's
//! helper, which `from . import sfp` needs.

use platform_base::testing::{FakeRedis, CONFIG_DB};

use crate::testing::Root;

/// The SDK attributes of module `index`, as the code names them.
pub fn module_path(index: usize, attr: &str) -> String {
    format!("/sys/module/sx_core/asic0/module{index}/{attr}")
}

/// `platform` with `sfps` ports, in module host management mode or not,
/// in a container (with /.dockerenv) or on the host.
pub fn platform(platform: &str, sfps: usize, host_mgmt: bool, container: bool) -> Root {
    let root = Root::new();
    root.write("/host/machine.conf", &format!("onie_platform={platform}\n"));
    let ports: Vec<String> = (1..=sfps).map(|i| format!("{{\"name\": \"sfp{i}\"}}")).collect();
    root.write(
        "/usr/share/sonic/platform/platform.json",
        &format!("{{\"chassis\": {{\"sfps\": [{}]}}, \"interfaces\": {{}}}}", ports.join(", ")),
    );
    if container {
        root.write("/.dockerenv", "");
    }
    let redis = FakeRedis::get();
    redis.flush();
    redis.install(&root);
    redis.set(CONFIG_DB, "CONFIG_DB_INITIALIZED", "1");
    redis.hset(CONFIG_DB, "DEVICE_METADATA|localhost", &[("hwsku", "HWSKU-C"), ("platform", platform)]);
    // Wherever the hwsku a process caches first says.
    let dir = crate::utils::get_path_to_hwsku_directory(None).expect("hwsku directory");
    root.write(&format!("{dir}/sai.profile"), if host_mgmt { "SAI_INDEPENDENT_MODULE_MODE=1\n" } else { "SAI_INDEPENDENT_MODULE_MODE=0\n" });
    crate::hw_mgmt::testing::install_independent_mode_update(&root);
    root
}

/// Module `index`'s attributes.
pub fn module(root: &Root, index: usize, attrs: &[(&str, &str)]) {
    for (attr, value) in attrs {
        root.write(&module_path(index, attr), value);
    }
}

/// Module `index`'s EEPROM page 0, a CMIS module's first byte.
pub fn eeprom(root: &Root, index: usize) {
    let page = root.real(&module_path(index, "eeprom/pages/0/i2c-0x50/data"));
    std::fs::create_dir_all(page.parent().expect("parent")).expect("eeprom dir");
    let mut data = vec![0u8; 256];
    data[0] = 0x18;
    std::fs::write(page, data).expect("eeprom");
}

/// A FIFO at `path` with a writer held open, so opening it does not block
/// -- and seeking it fails, as an unseekable attribute's would.
pub fn fifo(root: &Root, path: &str) -> std::fs::File {
    let real = root.real(path);
    std::fs::create_dir_all(real.parent().expect("parent")).expect("fifo dir");
    let _ = std::fs::remove_file(&real);
    let c = std::ffi::CString::new(real.to_string_lossy().into_owned()).expect("path");
    // SAFETY: a NUL-terminated path.
    assert_eq!(unsafe { libc::mkfifo(c.as_ptr(), 0o600) }, 0);
    std::fs::OpenOptions::new().read(true).write(true).open(real).expect("fifo writer")
}
