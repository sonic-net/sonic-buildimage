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


//! hw-management's Python modules that `sonic_platform` uses, run as they
//! are: their logic -- the files hw-management-tc reads, the BMC account and
//! its TPM password -- is hw-management's to keep, so this crate calls them
//! through an embedded interpreter rather than carrying a copy.
//!
//! The daemon links libpython anyway, for `--platform-api pyo3`; the
//! interpreter starts on the first call here, so only a process that reaches
//! one of these modules -- thermalctld, through its thermal updater -- has
//! one.  It starts without Python's signal handlers (pyo3 initializes with
//! `Py_InitializeEx(0)`), so the daemon keeps its own.
//!
//! - `hw_management_independent_mode_update`: imported as
//!   `thermal_updater.py` imports it, from `/run/hw-management/bin`, where
//!   hw-management copies it for pmon.
//! - `hw_management_redfish_client`: loaded from its path, as `bmc.py` loads
//!   it.  hw-management installs it in the host's `/usr/bin`, which pmon does
//!   not have, so in pmon this fails as Python's does.
//!
//! `hw_management_dpu_thermal_update`, which Python's SmartSwitch thermal
//! updater calls, is not called: see README.md.

use std::collections::HashSet;
use std::ffi::CStr;
use std::sync::Mutex;

use platform_base::io;
use platform_base::py::{Exc, PyErr, PyResult, PyVal};
use pyo3::ffi::c_str;
use pyo3::prelude::*;
use pyo3::types::{PyDict, PyList, PyString, PyTuple};

use crate::utils;

/// Where `thermal_updater.py` and `sfp.py` look for hw-management's helper.
pub const INDEPENDENT_MODE_UPDATE_DIR: &str = "/run/hw-management/bin";
pub const INDEPENDENT_MODE_UPDATE_NAME: &str = "hw_management_independent_mode_update";
pub const INDEPENDENT_MODE_UPDATE_PATH: &str =
    "/run/hw-management/bin/hw_management_independent_mode_update.py";

/// `HW_MGMT_REDFISH_CLIENT_PATH` and `_NAME`, from `bmc.py`.
pub const REDFISH_CLIENT_PATH: &str = "/usr/bin/hw_management_redfish_client.py";
pub const REDFISH_CLIENT_NAME: &str = "hw_management_redfish_client";

/// The imported helper, and the `sys.path` entry its import appended.
struct Imported {
    module: Py<PyModule>,
    /// For a test to take out again; see [`forget_imports`].
    #[cfg_attr(not(test), allow(dead_code))]
    path: String,
}

static INDEPENDENT_MODE_UPDATE: Mutex<Option<Imported>> = Mutex::new(None);

/// A Python exception as this crate's: its class name and `str()`.
fn from_python(py: Python<'_>, e: &pyo3::PyErr) -> PyErr {
    let name = e
        .get_type(py)
        .name()
        .map(|n| n.to_string())
        .unwrap_or_else(|_| "Exception".to_string());
    let msg = e.value(py).str().map(|s| s.to_string()).unwrap_or_default();
    PyErr::new(Exc::Other(intern(&name)), vec![PyVal::Str(msg)])
}

/// A class name as the `&'static str` [`Exc::Other`] takes, kept once per
/// name: the same few classes come back on every failing cycle.
fn intern(name: &str) -> &'static str {
    static NAMES: Mutex<Option<HashSet<&'static str>>> = Mutex::new(None);
    let mut names = utils::lock(&NAMES);
    let names = names.get_or_insert_with(HashSet::new);
    if let Some(n) = names.get(name) {
        return n;
    }
    let n: &'static str = Box::leak(name.to_string().into_boxed_str());
    names.insert(n);
    n
}

/// `sys.path.append('/run/hw-management/bin')` and
/// `import hw_management_independent_mode_update`, once: the module-level
/// import of `thermal_updater.py`.  A missing module raises the
/// `ModuleNotFoundError` Python's import does.
pub fn import_independent_mode_update() -> PyResult<()> {
    Python::with_gil(|py| independent_mode_update(py).map(|_| ()))
}

fn independent_mode_update(py: Python<'_>) -> PyResult<Bound<'_, PyModule>> {
    // The lock is not held across the import: Python may release the GIL
    // while importing, and a thread that took it then would wait on this
    // lock with the GIL held.  Two threads importing at once is Python's
    // import lock's business; the module is the same either way.
    if let Some(i) = utils::lock(&INDEPENDENT_MODE_UPDATE).as_ref() {
        return Ok(i.module.bind(py).clone());
    }
    let path = io::real(INDEPENDENT_MODE_UPDATE_DIR).to_string_lossy().into_owned();
    let module = (|| -> pyo3::PyResult<Bound<'_, PyModule>> {
        let sys_path = py.import("sys")?.getattr("path")?;
        let entries = sys_path.downcast::<PyList>()?;
        if !entries.contains(&path)? {
            entries.append(&path)?;
        }
        // A failed attempt leaves the finder's view of the directory -- or
        // that there was none -- cached; hw-management may copy the module
        // there only later.  Only this directory's finder is dropped:
        // `importlib.invalidate_caches()` would import importlib.metadata,
        // and with it zipfile, lzma and bz2, which cost the daemon several MB.
        py.import("sys")?.getattr("path_importer_cache")?.call_method1("pop", (&path, py.None()))?;
        py.import(INDEPENDENT_MODE_UPDATE_NAME)
    })()
    .map_err(|e| from_python(py, &e))?;
    utils::lock(&INDEPENDENT_MODE_UPDATE).get_or_insert_with(|| Imported {
        module: module.clone().unbind(),
        path,
    });
    Ok(module)
}

/// An argument hw-management takes as Python passes it: the ASIC
/// temperature is `''` while STATE_DB has none.
#[derive(Clone, Debug, PartialEq)]
pub enum Val {
    Int(i128),
    Str(String),
}

impl Val {
    fn to_python<'py>(&self, py: Python<'py>) -> pyo3::PyResult<Bound<'py, PyAny>> {
        Ok(match self {
            Val::Int(i) => i.into_pyobject(py)?.into_any(),
            Val::Str(s) => PyString::new(py, s).into_any(),
        })
    }
}

/// `hw_management_independent_mode_update.<function>(*args)`.  What it
/// answers is ignored, as Python ignores it; what it raises is raised.
fn call(function: &str, args: &[Val]) -> PyResult<()> {
    Python::with_gil(|py| {
        let module = independent_mode_update(py)?;
        (|| -> pyo3::PyResult<()> {
            let args = args.iter().map(|a| a.to_python(py)).collect::<pyo3::PyResult<Vec<_>>>()?;
            module.getattr(function)?.call1(PyTuple::new(py, args)?)?;
            Ok(())
        })()
        .map_err(|e| from_python(py, &e))
    })
}

pub fn thermal_data_set_asic(asic_index: i128, temperature: Val, warning: Val, critical: Val, fault: Val) -> PyResult<()> {
    call("thermal_data_set_asic", &[Val::Int(asic_index), temperature, warning, critical, fault])
}

pub fn thermal_data_set_module(
    asic_index: i128,
    module_index: i128,
    temperature: Val,
    warning: Val,
    critical: Val,
    fault: Val,
) -> PyResult<()> {
    call(
        "thermal_data_set_module",
        &[Val::Int(asic_index), Val::Int(module_index), temperature, warning, critical, fault],
    )
}

/// `vendor_data_set_module(asic_index, module_index, {...})`, the dict in
/// the order given.
pub fn vendor_data_set_module(asic_index: i128, module_index: i128, vendor: &[(&str, String)]) -> PyResult<()> {
    Python::with_gil(|py| {
        let module = independent_mode_update(py)?;
        (|| -> pyo3::PyResult<()> {
            let info = PyDict::new(py);
            for (k, v) in vendor {
                info.set_item(k, v)?;
            }
            module.getattr("vendor_data_set_module")?.call1((asic_index, module_index, info))?;
            Ok(())
        })()
        .map_err(|e| from_python(py, &e))
    })
}

pub fn thermal_data_clean_asic(asic_index: i128) -> PyResult<()> {
    call("thermal_data_clean_asic", &[Val::Int(asic_index)])
}

pub fn thermal_data_clean_module(asic_index: i128, module_index: i128) -> PyResult<()> {
    call("thermal_data_clean_module", &[Val::Int(asic_index), Val::Int(module_index)])
}

pub fn module_data_set_module_counter(module_counter: i128) -> PyResult<()> {
    call("module_data_set_module_counter", &[Val::Int(module_counter)])
}

/// `bmc.py`'s two uses of the Redfish client, so that the `with` blocks and
/// the module's own load stay Python's.
const REDFISH: &CStr = c_str!(
    r#"
import contextlib
import importlib.util
import io
import os
import sys


def load(name, path, real_path):
    """_get_hw_mgmt_redfish_client(): the module is in sys.modules before it
    runs, so one that failed to run stays there, half loaded.  `path` is the
    one Python names, `real_path` where it is."""
    if name in sys.modules:
        return sys.modules[name]
    if not os.path.exists(real_path):
        raise ImportError(f"{name} not found at {path}")
    spec = importlib.util.spec_from_file_location(name, real_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def login_password(name, path, real_path):
    return load(name, path, real_path).BMCAccessor().get_login_password()


def login(name, path, real_path):
    """_configure_nos_account()'s call: (returned, its repr, whether it is
    not 0, what it printed) or the exception and what it printed."""
    output = io.StringIO()
    try:
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            ret = load(name, path, real_path).BMCAccessor().login()
    except Exception as e:
        return None, e, output.getvalue().splitlines()
    return (str(ret), bool(ret != 0)), None, output.getvalue().splitlines()
"#
);

fn redfish(py: Python<'_>) -> pyo3::PyResult<Bound<'_, PyModule>> {
    PyModule::from_code(py, REDFISH, c_str!("sonic_platform/bmc.py"), c_str!("_mlnx_bmc_redfish"))
}

fn redfish_args() -> (&'static str, &'static str, String) {
    (
        REDFISH_CLIENT_NAME,
        REDFISH_CLIENT_PATH,
        io::real(REDFISH_CLIENT_PATH).to_string_lossy().into_owned(),
    )
}

/// `_get_hw_mgmt_redfish_client().BMCAccessor().get_login_password()`.
pub fn redfish_login_password() -> PyResult<PyVal> {
    Python::with_gil(|py| {
        (|| -> pyo3::PyResult<PyVal> {
            let pw = redfish(py)?.getattr("login_password")?.call1(redfish_args())?;
            Ok(PyVal::Str(pw.str()?.to_string()))
        })()
        .map_err(|e| from_python(py, &e))
    })
}

/// What `BMCAccessor().login()` did: what it answered (its `str()`, and
/// whether it is not 0) or raised, and the lines it printed.
pub struct RedfishLogin {
    pub result: PyResult<(String, bool)>,
    pub printed: Vec<String>,
}

/// What the `login` helper above answers: `(str(ret), ret != 0)`, or the
/// exception, and the printed lines.
type LoginAnswer<'py> = (Option<(String, bool)>, Option<Bound<'py, PyAny>>, Vec<String>);

/// `BMCAccessor().login()` with its stdout and stderr captured, as
/// `_configure_nos_account()` calls it.
pub fn redfish_login() -> RedfishLogin {
    Python::with_gil(|py| {
        let r = (|| -> pyo3::PyResult<RedfishLogin> {
            let out = redfish(py)?.getattr("login")?.call1(redfish_args())?;
            let (ret, err, printed): LoginAnswer<'_> = out.extract()?;
            let result = match (ret, err) {
                (Some(r), _) => Ok(r),
                (None, Some(e)) => Err(from_python(py, &pyo3::PyErr::from_value(e))),
                (None, None) => Err(PyErr::runtime_error("no result")),
            };
            Ok(RedfishLogin { result, printed })
        })();
        r.unwrap_or_else(|e| RedfishLogin {
            result: Err(from_python(py, &e)),
            printed: Vec::new(),
        })
    })
}

/// Forget the imports, for a test that starts on a new root: the next call
/// imports from there.
#[cfg(test)]
pub fn forget_imports() {
    let old = utils::lock(&INDEPENDENT_MODE_UPDATE).take();
    Python::with_gil(|py| {
        let sys = py.import("sys").expect("sys");
        let modules = sys.getattr("modules").expect("sys.modules");
        for name in [INDEPENDENT_MODE_UPDATE_NAME, REDFISH_CLIENT_NAME] {
            let _ = modules.del_item(name);
        }
        if let Some(old) = old {
            let path = sys.getattr("path").expect("sys.path");
            let _ = path.call_method1("remove", (&old.path,));
            let cache = sys.getattr("path_importer_cache").expect("sys.path_importer_cache");
            let _ = cache.call_method1("pop", (&old.path, py.None()));
        }
    });
}

#[cfg(test)]
pub mod testing {
    //! Stand-ins for hw-management's modules, which record what they are
    //! called with: `install` puts them where the code looks for them under
    //! the test's root.

    use super::*;

    pub const INDEPENDENT_MODE_UPDATE: &str = include_str!("testdata/hw_mgmt/hw_management_independent_mode_update.py");
    pub const REDFISH_CLIENT: &str = include_str!("testdata/hw_mgmt/hw_management_redfish_client.py");

    /// hw-management's helper for pmon, as `/run/hw-management/bin` has it.
    pub fn install_independent_mode_update(root: &crate::testing::Root) {
        root.write(INDEPENDENT_MODE_UPDATE_PATH, INDEPENDENT_MODE_UPDATE);
    }

    /// hw-management's Redfish client, as the host's `/usr/bin` has it.
    pub fn install_redfish_client(root: &crate::testing::Root) {
        root.write(REDFISH_CLIENT_PATH, REDFISH_CLIENT);
    }

    /// The helper's recorded calls since the last `take_calls`, each as the
    /// `repr()` of `(function, *args)`.
    pub fn take_calls() -> Vec<String> {
        Python::with_gil(|py| {
            let module = independent_mode_update(py).expect("hw-management's helper");
            let calls = module.getattr("calls").expect("calls");
            let out: Vec<String> = calls
                .try_iter()
                .expect("a list")
                .map(|c| c.expect("a call").repr().expect("repr").to_string())
                .collect();
            calls.call_method0("clear").expect("clear");
            out
        })
    }

    /// Make the helper's `function` raise `RuntimeError(message)`.
    pub fn fail(function: &str, message: &str) {
        Python::with_gil(|py| {
            let module = independent_mode_update(py).expect("hw-management's helper");
            module.getattr("fail").expect("fail").set_item(function, message).expect("set");
        })
    }

    /// Set an attribute of the Redfish client's `BMCAccessor` stand-in.
    pub fn redfish_set(attr: &str, value: &str) {
        Python::with_gil(|py| {
            let (name, path, real) = redfish_args();
            let module = redfish(py).and_then(|r| r.getattr("load")?.call1((name, path, real))).expect("the client");
            let value = py.eval(&std::ffi::CString::new(value).expect("no NUL"), None, None).expect("a value");
            module.getattr("BMCAccessor").expect("BMCAccessor").setattr(attr, value).expect("set");
        })
    }
}
