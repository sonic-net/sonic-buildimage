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

//! hw-management's Python helpers that the NVIDIA platform API imports, in
//! Rust.  They are hw-management's code, not the platform API's: each module
//! follows, line by line, the hw-management version README.md names, and
//! has to be brought up to date when the image takes a hw-management whose
//! copy of that module differs.
//!
//! - [`independent_mode_update`] is `hw_management_independent_mode_update.py`;
//! - [`dpu_thermal_update`] is `hw_management_dpu_thermal_update.py`.
//!
//! The third module the platform API imports, `hw_management_redfish_client.py`,
//! is not here: pmon does not have it, so its import fails there in Python
//! too (see platform-native's `bmc`).

use std::io::Write;

use platform_base::io;
use platform_base::py::{codec, PyErr, PyResult};

pub mod dpu_thermal_update;
pub mod independent_mode_update;
#[cfg(any(test, feature = "testing"))]
pub mod testing;

pub(crate) const BASE_PATH: &str = "/var/run/hw-management";

/// `open(path, encoding="utf-8").read()`: hw-management's helpers name
/// their codec, so the locale's (ASCII in pmon) does not apply.
pub(crate) fn read_utf8(path: &str) -> PyResult<String> {
    let b = std::fs::read(io::real(path)).map_err(|e| PyErr::from_io(&e, Some(path)))?;
    Ok(codec::universal_newlines(codec::decode(codec::Codec::Utf8, &b)?))
}

/// `open(path, 'w', encoding="utf-8").write(text)`.
pub(crate) fn write_utf8(path: &str, text: &str) -> PyResult<()> {
    std::fs::write(io::real(path), text).map_err(|e| PyErr::from_io(&e, Some(path)))
}

pub(crate) fn print(msg: &str) {
    let _ = writeln!(std::io::stdout(), "{msg}");
}
