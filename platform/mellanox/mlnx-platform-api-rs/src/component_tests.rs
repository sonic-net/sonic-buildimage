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

//! The component scenarios: the firmware tools faked beneath an io root by
//! testdata/component/setup.sh, each group's steps run through the
//! components, and every answer and every tool call compared with what
//! Python's component.py gave over the same fakes (testdata/component/
//! README.md says how those were made).

use std::process::Command;
use std::sync::Arc;

use platform_base::py::ops;
use platform_base::{PyErr, PyObject, PyResult, PyVal};
use regex::Regex;

use super::*;
use crate::testing::Root;

const SETUP: &str = include_str!("testdata/component/setup.sh");

fn arg(a: &PyVal, k: usize) -> PyVal {
    ops::getitem(a, &PyVal::Int(k as i128)).unwrap_or_default()
}

fn raise(e: &PyErr) -> String {
    format!("raise {}: {}", e.exc.name(), e.str())
}

fn kind(name: &str) -> Kind {
    match name {
        "ONIE" => Kind::Onie,
        "SSD" => Kind::Ssd,
        "BIOS" => Kind::Bios,
        "BIOSSN2201" => Kind::BiosSn2201,
        "CPLD" => Kind::Cpld,
        "CPLDSN2201" => Kind::CpldSn2201,
        "CPLDSN4280" => Kind::CpldSn4280,
        _ => Kind::FpgaDpu,
    }
}

/// driver.py's steps, one `STEP i => answer` line each: `new` builds a
/// component, `call` calls one, `host_list` builds the host's list, `ini`
/// reads a file with configparser, `shell` runs a command -- with `env`
/// added to its environment.
pub(super) fn run_steps(steps: &PyVal, env: &[(&str, String)]) -> Vec<String> {
    crate::imports::platform();
    crate::imports::component();
    let mut objs: Vec<(String, Arc<HostComponent>)> = Vec::new();
    let mut lines = Vec::new();
    for (i, st) in ops::iterate(steps).unwrap_or_default().iter().enumerate() {
        print(&format!("=== STEP {i}"));
        let r: PyResult<PyVal> = match arg(st, 0).py_str().as_str() {
            "new" => {
                let idx = match arg(st, 3) {
                    PyVal::Int(n) => n,
                    _ => 0,
                };
                HostComponent::new(kind(&arg(st, 2).py_str()), idx).map(|c| {
                    objs.push((arg(st, 1).py_str(), Arc::new(c)));
                    PyVal::str("ok")
                })
            }
            "host_list" => {
                let mut v = Vec::new();
                let r = host_component_list_into(&mut v);
                let names = PyVal::List(
                    v.iter()
                        .map(|o| match o {
                            PyVal::Obj(o) => PyVal::Tuple(vec![PyVal::str(o.py_class()), o.get_name().unwrap_or_default()]),
                            other => other.clone(),
                        })
                        .collect(),
                );
                Ok(match r {
                    Ok(()) => names,
                    Err(e) => PyVal::Str(format!("{} | partial={}", raise(&e), names.py_repr())),
                })
            }
            "call" => {
                let id = arg(st, 1).py_str();
                let o = objs.iter().rev().find(|(n, _)| *n == id).map(|(_, o)| o.clone());
                let a = arg(st, 3);
                let s = |k| arg(&a, k).py_str();
                match (o, arg(st, 2).py_str().as_str()) {
                    (None, _) => Ok(PyVal::str("no such component")),
                    (Some(o), "get_name") => o.get_name(),
                    (Some(o), "get_description") => o.get_description(),
                    (Some(o), "get_firmware_version") => o.get_firmware_version(),
                    (Some(o), "get_available_firmware_version") => o.get_available_firmware_version(&s(0)),
                    (Some(o), "get_firmware_update_notification") => o.get_firmware_update_notification(&s(0)),
                    (Some(o), "install_firmware") => o.install_firmware(&s(0)),
                    (Some(o), "update_firmware") => o.update_firmware(&s(0)),
                    (Some(o), "auto_update_firmware") => o.auto_update_firmware(&s(0), &s(1)),
                    _ => Ok(PyVal::str("bad call")),
                }
            }
            "shell" => {
                let _ = Command::new("sh")
                    .arg("-c")
                    .arg(arg(st, 1).py_str())
                    .envs(env.iter().map(|(k, v)| (*k, v.as_str())))
                    .status();
                Ok(PyVal::str("ok"))
            }
            "ini" => match Ini::read(&arg(st, 1).py_str()) {
                Err(e) => Ok(PyVal::Str(raise(&e))),
                Ok(ini) => {
                    let pairs = arg(st, 2);
                    Ok(PyVal::List(
                        ops::iterate(&pairs)
                            .unwrap_or_default()
                            .iter()
                            .map(|p| {
                                let (sec, opt) = (arg(p, 0).py_str(), arg(p, 1).py_str());
                                let got = match ini.get(&sec, &opt) {
                                    Ok(v) => v,
                                    Err(e) => raise(&e),
                                };
                                PyVal::Tuple(vec![PyVal::Bool(ini.has_option(&sec, &opt)), PyVal::Str(got)])
                            })
                            .collect(),
                    ))
                }
            },
            _ => Ok(PyVal::str("bad op")),
        };
        let line = match r {
            Ok(v) => v.py_repr(),
            Err(e) => raise(&e),
        };
        lines.push(format!("STEP {i} => {line}\n"));
    }
    lines
}

/// What differs between two runs: the MPFAs' random directory names, and
/// the temporary directory itself if `$TMPDIR` moved it.
fn normalize(s: &str, tmp: &str) -> String {
    let s = if tmp == "/tmp" { s.to_string() } else { s.replace(&format!("{tmp}/"), "/tmp/") };
    Regex::new("mpfa-[a-z0-9_]{8}").map(|re| re.replace_all(&s, "mpfa-XXXXXXXX").into_owned()).unwrap_or(s)
}

/// A `.rec` file's entries, `STEP i => ...` up to the next.
fn entries(rec: &str) -> Vec<&str> {
    let re = Regex::new(r"(?m)^STEP \d+ => ").unwrap();
    let starts: Vec<usize> = re.find_iter(rec).map(|m| m.start()).collect();
    starts.iter().enumerate().map(|(k, &a)| &rec[a..starts.get(k + 1).copied().unwrap_or(rec.len())]).collect()
}

/// One group: a fresh root set up for `platform`, its steps, and Python's
/// answers and tool calls.
fn run_group(platform: &str, steps: &str, rec: &str, calls: &str) {
    // `$PLATFORM` comes before machine.conf, and a test leaves the process's
    // environment alone.
    if crate::testing::platform_from_env() {
        return;
    }
    let root = Root::new();
    let r = root.dir.to_string_lossy().into_owned();
    let st = Command::new("sh")
        .arg("-c")
        .arg(SETUP)
        .env("R", &r)
        .env("BIN", format!("{r}/usr/bin"))
        .env("PLAT", platform)
        .status()
        .expect("setup");
    assert!(st.success(), "setup failed");
    let tmp = gettempdir().unwrap_or_else(|_| "/tmp".into());
    let steps = platform_base::json::loads(steps).expect("steps");
    let got = run_steps(&steps, &[("R", r.clone())]);
    let want = entries(rec);
    assert_eq!(got.len(), want.len());
    for (g, w) in got.iter().zip(&want) {
        assert_eq!(normalize(g, &tmp), normalize(w, &tmp));
    }
    let got_calls = std::fs::read_to_string(root.real("/tmp/c/calls")).unwrap_or_default();
    assert_eq!(normalize(&got_calls, &tmp), normalize(calls, &tmp));
}

/// configparser's corpus, and ONIE, SSD, BIOS and CPLD on a plain switch:
/// their tools failing every way the steps make them.
#[test]
fn a_plain_switch_s_components_answer_as_python_s() {
    run_group(
        "x86_64-nvidia_sn5600-r0",
        include_str!("testdata/component/generic.json"),
        include_str!("testdata/component/generic.rec"),
        include_str!("testdata/component/generic.calls"),
    );
}

#[test]
fn sn2201_s_bios_and_cpld_answer_as_python_s() {
    run_group(
        "x86_64-nvidia_sn2201-r0",
        include_str!("testdata/component/sn2201.json"),
        include_str!("testdata/component/sn2201.rec"),
        include_str!("testdata/component/sn2201.calls"),
    );
}

/// The SmartSwitch's CPLDs and DPU FPGAs, whose update reboots the switch
/// once the running DPUs are back.
#[test]
fn sn4280_s_cpld_and_dpu_fpga_answer_as_python_s() {
    run_group(
        "x86_64-nvidia_sn4280-r0",
        include_str!("testdata/component/sn4280.json"),
        include_str!("testdata/component/sn4280.rec"),
        include_str!("testdata/component/sn4280.calls"),
    );
}

#[test]
fn a_simx_platform_has_no_cplds() {
    run_group(
        "x86_64-nvidia_sn4800_simx-r0",
        include_str!("testdata/component/simx.json"),
        include_str!("testdata/component/simx.rec"),
        include_str!("testdata/component/simx.calls"),
    );
}

/// On a BMC platform a CPLD update ends in an aux power cycle.
#[test]
fn a_bmc_platform_s_cpld_update_power_cycles() {
    run_group(
        "x86_64-nvidia_sn6600_ld-r0",
        include_str!("testdata/component/bmc.json"),
        include_str!("testdata/component/bmc.rec"),
        include_str!("testdata/component/bmc.calls"),
    );
}

/// hw-management's BMC updater, faked beneath `root`: it logs its argv and
/// whether it got a session of its own, and exits with `rc`.
fn bmc_updater(root: &Root, rc: i32) {
    let real = root.real(BMC_FW_UPDATE);
    let r = root.dir.to_string_lossy().into_owned();
    let body = format!(
        "#!/bin/sh\nsid=$(cut -d' ' -f6 /proc/$$/stat)\necho \"$* session=$([ \"$sid\" = $$ ] && echo new || echo old)\" >> '{r}/tmp/bmc_calls'\nexit {rc}\n"
    );
    let st = Command::new("sh")
        .arg("-c")
        .arg("mkdir -p \"$(dirname \"$1\")\" && printf '%s' \"$2\" > \"$1\" && chmod 755 \"$1\"")
        .arg("sh")
        .arg(&real)
        .arg(body)
        .status()
        .expect("updater");
    assert!(st.success());
}

/// `ComponentBMC`: its version is the BMC's, its install hw-management's
/// updater in a session of its own.
#[test]
fn the_bmc_component_installs_through_hw_management_s_updater() {
    let root = Root::new();
    crate::bmc::forget_instance();
    // Not a BMC platform: no BMC behind the component.
    let list = bmc_component_list().unwrap();
    let PyVal::Obj(c) = &list[0] else { panic!() };
    assert_eq!((c.py_class(), c.base_class()), ("ComponentBMC", BaseClass::ComponentBase));
    assert_eq!(c.get_name().unwrap(), PyVal::str("BMC"));
    assert_eq!(c.get_description().unwrap(), PyVal::str("BMC - Baseboard Management Controller"));
    assert_eq!(c.get_firmware_version().unwrap(), PyVal::str("N/A"));
    assert_eq!(
        c.get_available_firmware_version("x").unwrap_err().str(),
        "BMC component doesn't support available firmware version query"
    );
    assert_eq!(
        c.get_firmware_update_notification("x").unwrap(),
        PyVal::str("BMC will be automatically restarted to complete BMC firmware update")
    );
    // The updater: an image of the wrong kind, a missing one, then success
    // and failure, and no updater at all.
    root.write("/tmp/fw/bmc.fwpkg", "pkg");
    root.write("/tmp/fw/bmc.bin", "bin");
    assert_eq!(c.install_firmware("/tmp/fw/bmc.bin").unwrap(), PyVal::Bool(false));
    assert_eq!(c.install_firmware("/tmp/fw/none.fwpkg").unwrap(), PyVal::Bool(false));
    assert_eq!(c.auto_update_firmware("/tmp/fw/none.fwpkg", "cold").unwrap(), PyVal::Int(FW_AUTO_ERR_IMAGE));
    assert_eq!(c.auto_update_firmware("/tmp/fw/bmc.fwpkg", "warm").unwrap(), PyVal::Int(FW_AUTO_ERR_BOOT_TYPE));
    assert_eq!(c.install_firmware("/tmp/fw/bmc.fwpkg").unwrap(), PyVal::Bool(false));
    bmc_updater(&root, 0);
    assert_eq!(c.install_firmware("/tmp/fw/bmc.fwpkg").unwrap(), PyVal::Bool(true));
    assert_eq!(c.update_firmware("/tmp/fw/bmc.fwpkg").unwrap(), PyVal::Bool(true));
    assert_eq!(c.auto_update_firmware("/tmp/fw/bmc.fwpkg", "cold").unwrap(), PyVal::Int(FW_AUTO_INSTALLED));
    bmc_updater(&root, 3);
    assert_eq!(c.auto_update_firmware("/tmp/fw/bmc.fwpkg", "cold").unwrap(), PyVal::Int(FW_AUTO_ERR_UNKNOWN));
    assert_eq!(
        root.read("/tmp/bmc_calls").unwrap(),
        "/tmp/fw/bmc.fwpkg session=new\n".repeat(4)
    );
    // With a BMC: its version.
    let curl = platform_base::testing::FakeCurl::new();
    curl.reply("GET", "/redfish/v1/UpdateService/FirmwareInventory/FW_BMC_0", 200, r#"{"Version": "89.0001.0100"}"#);
    let c = ComponentBmc { bmc: Some(Arc::new(crate::bmc::test_bmc(&curl.path()))) };
    assert_eq!(c.get_firmware_version().unwrap(), PyVal::str("89.0001.0100"));
}

/// ONIE's version parse, and the host's component list built whole.
#[test]
fn onie_versions_parse_as_python_s_patterns_do() {
    assert_eq!(
        OnieUpdater::parse_onie_version("2025.05-5.3.0021-115200-dev", false).unwrap(),
        ("5".into(), "3".into(), "0021".into())
    );
    assert_eq!(OnieUpdater::parse_onie_version("5.2.0016", true).unwrap(), ("5".into(), "2".into(), "0016".into()));
    assert_eq!(
        OnieUpdater::parse_onie_version("garbage", false).unwrap_err().str(),
        format!("Failed to parse ONIE version: pattern={ONIE_VERSION_PARSE_PATTERN}, version=garbage")
    );
    if crate::testing::platform_from_env() {
        return;
    }
    let root = Root::new();
    root.write("/var/run/hw-management/config/cpld_num", "1\n");
    let names: Vec<String> = host_component_list()
        .unwrap()
        .iter()
        .map(|o| match o {
            PyVal::Obj(o) => o.get_name().unwrap().py_str(),
            _ => String::new(),
        })
        .collect();
    assert_eq!(names, ["ONIE", "SSD", "BIOS", "CPLD1"]);
}
