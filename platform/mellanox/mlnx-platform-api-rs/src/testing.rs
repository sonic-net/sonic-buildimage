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


//! The scratch root tests put beneath every path: platform-base's, so a
//! test here and platform-base's own test doubles (FakeRedis, FakeCurl) hold
//! the same lock -- `io::set_root` is process-wide.  A new root is a new
//! platform, so it also forgets what the process kept from the last one:
//! every read-only cache, the port mapping, hw-management's imports, the
//! BMC, the PSUs' shared LED -- and the fake Redis's data, which a later
//! test would otherwise read as its own.

use std::io::Write;
use std::ops::Deref;

/// Whether `$PLATFORM` names the platform, overriding machine.conf:
/// platform-base's `get_platform()` reads it first, so a test that sets one
/// particular platform up through machine.conf cannot run under it.  The
/// skip is said on stderr itself -- past the capture `cargo test` puts on
/// `eprintln!` -- since the test then passes without checking anything.
pub fn platform_from_env() -> bool {
    match std::env::var("PLATFORM") {
        Ok(p) if !p.is_empty() => {
            let test = std::thread::current();
            let line = format!("skipped {}: $PLATFORM={p} overrides machine.conf\n", test.name().unwrap_or("?"));
            let _ = std::io::stderr().write_all(line.as_bytes());
            true
        }
        _ => false,
    }
}

/// Whether this is the process to run the calling test in: in the test
/// binary's own it is not -- `test` is run again in a process of its own,
/// which must pass -- and in that process it is.  For a test that sets what
/// the others must not see: the shutdown event ends every wait in the
/// process, and is never cleared.
pub fn in_own_process(test: &str) -> bool {
    const OWN_PROCESS: &str = "MLNX_TEST_OWN_PROCESS";
    if std::env::var_os(OWN_PROCESS).is_some() {
        return true;
    }
    let out = std::process::Command::new(std::env::current_exe().expect("test binary"))
        .args([test, "--exact", "--test-threads=1"])
        .env(OWN_PROCESS, test)
        .output()
        .expect("test binary");
    let stdout = String::from_utf8_lossy(&out.stdout);
    assert!(
        out.status.success() && stdout.contains("test result: ok. 1 passed"),
        "{test}, in a process of its own:\n{stdout}{}",
        String::from_utf8_lossy(&out.stderr)
    );
    false
}

pub struct Root(platform_base::testing::Root);

impl Root {
    pub fn new() -> Root {
        let root = platform_base::testing::Root::new();
        crate::utils::forget_caches();
        crate::sfp::forget_port_mapping();
        crate::hw_mgmt::forget_imports();
        crate::bmc::forget_instance();
        crate::psu::Psu::forget_shared_led();
        platform_base::testing::FakeRedis::get().flush();
        Root(root)
    }
}

impl Deref for Root {
    type Target = platform_base::testing::Root;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}
