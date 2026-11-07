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



//! `sonic_platform/sed_mgmt.py`: the SED password limits and the default
//! password, which the platform script reads from the TPM.  The changes
//! themselves are SedMgmtBase's.

use std::sync::{Arc, OnceLock};
use std::time::Duration;

use platform_base::py::{PyResult, PyVal};
use platform_base::{subprocess, BaseClass, PyObject};

const READ_DEFAULT_SED_PW_SCRIPT: &str = "/usr/local/bin/read_default_sed_pw_from_tpm.sh";

pub struct SedMgmt;

/// `SedMgmt._instance`.
static INSTANCE: OnceLock<Arc<SedMgmt>> = OnceLock::new();

impl SedMgmt {
    /// `SedMgmt.get_instance()`.
    pub fn get_instance() -> Arc<SedMgmt> {
        INSTANCE.get_or_init(|| Arc::new(SedMgmt)).clone()
    }
}

impl PyObject for SedMgmt {
    fn py_class(&self) -> &'static str {
        "SedMgmt"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::SedMgmtBase
    }
    fn get_min_sed_password_len(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(8))
    }
    fn get_max_sed_password_len(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(124))
    }
    /// The script's stdout, stripped, when it exits 0 with something to say
    /// within 10 seconds; None otherwise, whatever went wrong.
    fn get_default_sed_password(&self) -> PyResult<PyVal> {
        let script = crate::component::program_path(READ_DEFAULT_SED_PW_SCRIPT);
        let pw = subprocess::run_capture(&[&script.to_string_lossy()], Duration::from_secs(10))
            .and_then(|r| Ok((r.returncode, subprocess::text(&r.stdout)?, subprocess::text(&r.stderr)?)));
        Ok(match pw {
            Ok((0, out, _)) if !out.is_empty() => PyVal::str(platform_base::py::strip(&out)),
            _ => PyVal::None,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use std::process::{Command, Stdio};

    /// The script, written by a child so that no handle of ours is open on
    /// it when it runs.
    fn script(root: &Root, body: &str) {
        let real = root.real(READ_DEFAULT_SED_PW_SCRIPT);
        let mut child = Command::new("sh")
            .arg("-c")
            .arg("mkdir -p \"$(dirname \"$1\")\" && cat > \"$1\" && chmod 755 \"$1\"")
            .arg("sh")
            .arg(&real)
            .stdin(Stdio::piped())
            .spawn()
            .unwrap();
        std::io::Write::write_all(&mut child.stdin.take().unwrap(), format!("#!/bin/sh\n{body}\n").as_bytes()).unwrap();
        assert!(child.wait().unwrap().success());
    }

    #[test]
    fn the_default_password_is_the_script_s_stripped_answer() {
        let root = Root::new();
        let sed = SedMgmt::get_instance();
        assert!(Arc::ptr_eq(&sed, &SedMgmt::get_instance()));
        assert_eq!((sed.py_class(), sed.base_class()), ("SedMgmt", BaseClass::SedMgmtBase));
        assert_eq!((sed.get_min_sed_password_len().unwrap(), sed.get_max_sed_password_len().unwrap()), (PyVal::Int(8), PyVal::Int(124)));
        // No script.
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::None);
        script(&root, "echo '  Pa55word  '");
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::str("Pa55word"));
        // Blank but not empty: the empty string.
        script(&root, "echo '   '");
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::str(""));
        // Nothing said, a failure, or what the codec cannot read: None.
        script(&root, "exit 0");
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::None);
        script(&root, "echo pw; exit 3");
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::None);
        script(&root, "printf '\\377\\n'");
        assert_eq!(sed.get_default_sed_password().unwrap(), PyVal::None);
    }
}
