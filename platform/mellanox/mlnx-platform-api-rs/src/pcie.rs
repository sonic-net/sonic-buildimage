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


//! `sonic_platform/pcie.py`: pcie.yaml checked by device id rather than by
//! bus, since a BIOS update can renumber buses; and a SmartSwitch's DPUs
//! checked against what chassisd has detached.

use std::sync::{Arc, Mutex};

use platform_base::db::Db;
use platform_base::logger::Logger;
use platform_base::redfish_client::is_exception;
use platform_base::io;
use platform_base::pcie::{self, PcieApi, PcieUtil};
use platform_base::py::{PyErr, PyResult, PyVal};
use regex::Regex;

use crate::device_data;

const SYSFS_PCI_DEVICE_PATH: &str = "/sys/bus/pci/devices/";
const PCIE_DETACH_INFO_TABLE: &str = "PCIE_DETACH_INFO";
const PCIE_OPERATION_DETACHING: &str = "detaching";

/// `Pcie(platform_path)`.
pub struct Pcie {
    base: PcieUtil,
    /// `_device_id_to_bus_map`: `"<id>:<dev>"` to bus, from sysfs.
    device_id_to_bus: Vec<(String, String)>,
    dpu_pcie_devices: Vec<String>,
    state_db: Mutex<Option<Arc<Db>>>,
    /// `self.logger = logger.Logger()`, made last in `__init__`.
    logger: Logger,
}

/// The constructor the escape hatch is given.
pub fn factory(platform_path: &str) -> PyResult<Arc<dyn PcieApi>> {
    Ok(Arc::new(Pcie::new(platform_path)?))
}

impl Pcie {
    pub fn new(platform_path: &str) -> PyResult<Pcie> {
        crate::imports::device_data();
        let base = PcieUtil::new(platform_path);
        let device_id_to_bus = create_device_id_to_bus_map(&base)?;
        let dpu_pcie_devices = get_dpu_pcie_devices()?;
        Ok(Pcie { base, device_id_to_bus, dpu_pcie_devices, state_db: Mutex::new(None), logger: Logger::new(None) })
    }

    /// `pcie_check_dpu(bus, dev, fn)`: None for a device being detached --
    /// or for one whose check raised, which is logged and left out.
    fn pcie_check_dpu(&self, bus: &PyVal, dev: &PyVal, func: &PyVal) -> PyResult<Option<&'static str>> {
        let r = (|| -> PyResult<Option<&'static str>> {
            let db = {
                let mut slot = crate::utils::lock(&self.state_db);
                match slot.as_ref() {
                    Some(db) => db.clone(),
                    None => {
                        let db = Arc::new(Db::connect("STATE_DB")?);
                        *slot = Some(db.clone());
                        db
                    }
                }
            };
            let key = format!("{PCIE_DETACH_INFO_TABLE}|0000:{}:{}.{}", bus.py_str(), dev.py_str(), func.py_str());
            let info = db.hgetall(&key)?;
            if !info.is_empty() && info.iter().any(|(k, v)| k == "dpu_state" && v == PCIE_OPERATION_DETACHING) {
                return Ok(None);
            }
            if self.base.check_pcie_sysfs(0, pcie::int_hex(bus)?, pcie::int_hex(dev)?, pcie::int_hex(func)?) {
                return Ok(Some("Passed"));
            }
            Ok(Some("Failed"))
        })();
        match r {
            Err(e) if is_exception(&e) => {
                self.logger.log_error(&format!("Error: {}", e.str()));
                Ok(None)
            }
            other => other,
        }
    }
}

/// `_create_device_id_to_bus_map()`: every PCI function's `device` file,
/// keyed with its slot.  It loads the config first, for nothing but the
/// failure.
fn create_device_id_to_bus_map(base: &PcieUtil) -> PyResult<Vec<(String, String)>> {
    base.load_config_file()?;
    let re = Regex::new(r"....:(..):(..)\..").map_err(|e| PyErr::runtime_error(e.to_string()))?;
    let mut map: Vec<(String, String)> = Vec::new();
    for folder in io::listdir(SYSFS_PCI_DEVICE_PATH)? {
        let Some(c) = re.captures(&folder) else { continue };
        let (bus, dev) = (c[1].to_string(), c[2].to_string());
        let path = io::join(&io::join("/sys/bus/pci/devices", &folder), "device");
        let id = platform_base::py::strip(&io::read_text(&path)?).replace("0x", "");
        let key = format!("{id}:{dev}");
        match map.iter_mut().find(|(k, _)| *k == key) {
            Some(e) => e.1 = bus,
            None => map.push((key, bus)),
        }
    }
    Ok(map)
}

/// `get_dpu_pcie_devices()`: both PCI functions of every DPU that names them.
fn get_dpu_pcie_devices() -> PyResult<Vec<String>> {
    let count = device_data::get_dpu_count()?;
    let mut out = Vec::new();
    for dpu_id in 0..count {
        let name = format!("dpu{dpu_id}");
        let pci = crate::dpuctlplat::dpu_interface(&name, "bus_info")?;
        let rshim = crate::dpuctlplat::dpu_interface(&name, "rshim_bus_info")?;
        match (pci, rshim) {
            (Some(p), Some(r)) if p.truthy() && r.truthy() => {
                out.push(p.py_str());
                out.push(r.py_str());
            }
            _ => continue,
        }
    }
    Ok(out)
}

impl PcieApi for Pcie {
    /// `get_pcie_check()`: by device id and slot; the DPUs' functions by
    /// their detach state.
    fn get_pcie_check(&self) -> PyResult<PyVal> {
        let conf = self.base.load_config_file()?;
        let mut out = Vec::new();
        for mut item_conf in pcie::iterate(&conf)? {
            let id = pcie::item(&item_conf, "id")?;
            let dev = pcie::item(&item_conf, "dev")?;
            let func = pcie::item(&item_conf, "fn")?;
            let bus = pcie::item(&item_conf, "bus")?;
            let device_id = format!("0000:{}:{}.{}", bus.py_str(), dev.py_str(), func.py_str());
            if self.dpu_pcie_devices.contains(&device_id) {
                if let Some(status) = self.pcie_check_dpu(&bus, &dev, &func)? {
                    pcie::set_item(&mut item_conf, "result", PyVal::str(status))?;
                    out.push(item_conf);
                }
                continue;
            }
            let key = format!("{}:{}", id.py_str(), dev.py_str());
            let mapped = self.device_id_to_bus.iter().find(|(k, _)| *k == key).map(|(_, b)| b.clone());
            let passed = match mapped.filter(|b| !b.is_empty()) {
                Some(b) => self.base.check_pcie_sysfs(
                    0,
                    pcie::int_hex(&PyVal::Str(b))?,
                    pcie::int_hex(&dev)?,
                    pcie::int_hex(&func)?,
                ),
                None => false,
            };
            pcie::set_item(&mut item_conf, "result", PyVal::str(if passed { "Passed" } else { "Failed" }))?;
            out.push(item_conf);
        }
        Ok(PyVal::List(out))
    }

    fn get_pcie_aer_stats(&self, domain: i64, bus: i64, dev: i64, func: i64) -> PyResult<PyVal> {
        self.base.get_pcie_aer_stats(domain, bus, dev, func)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_f::switch;
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, STATE_DB};

    const PLATFORM: &str = "/usr/share/sonic/platform";

    fn r(v: PyResult<PyVal>) -> String {
        v.map_or_else(|e| format!("raise {}: {}", e.exc.name(), e.str()), |v| v.py_repr())
    }

    /// A PCI function in sysfs, with its `device` id.
    fn function(root: &Root, slot: &str, id: &str) {
        root.write(&format!("/sys/bus/pci/devices/{slot}/device"), &format!("{id}\n"));
    }

    /// The `result` of each entry `get_pcie_check` answers.
    fn results(p: &Pcie) -> Vec<String> {
        let PyVal::List(items) = p.get_pcie_check().unwrap() else { panic!() };
        items.iter().map(|i| pcie::item(i, "result").unwrap().py_str()).collect()
    }

    fn switch_with_pcie(yaml: &str) -> Root {
        let root = switch("x86_64-nvidia_sn5600-r0");
        root.write(&format!("{PLATFORM}/pcie.yaml"), yaml);
        root.write(&format!("{PLATFORM}/platform.json"), "{\"chassis\": {}}");
        root
    }

    #[test]
    fn a_device_is_found_by_its_id_wherever_its_bus_moved() {
        // test_check_multiple_bus, over a sysfs whose buses a BIOS update moved.
        let root = switch_with_pcie(
            "- bus: '00'\n  id: 1f0b\n  dev: '00'\n  fn: '00'\n\
             - bus: '01'\n  id: 1f0b\n  dev: '11'\n  fn: '00'\n\
             - bus: '02'\n  id: 1f0b\n  dev: '22'\n  fn: '00'\n\
             - bus: '03'\n  id: 1533\n  dev: '00'\n  fn: '1'\n",
        );
        function(&root, "0000:05:00.0", "0x1f0b");
        function(&root, "0000:06:11.0", "0x1f0b");
        function(&root, "0000:07:00.0", "0x1533");
        // Not a function's folder: skipped.
        root.write("/sys/bus/pci/devices/pci0000:00/device", "0x0000\n");
        let p = Pcie::new(PLATFORM).unwrap();
        let mut map = p.device_id_to_bus.clone();
        map.sort();
        assert_eq!(map, [("1533:00".to_string(), "07".to_string()), ("1f0b:00".into(), "05".into()), ("1f0b:11".into(), "06".into())]);
        // The fourth: its bus is found, but no function 1 there.
        assert_eq!(results(&p), ["Passed", "Passed", "Failed", "Failed"]);
        function(&root, "0000:07:00.1", "0x1533");
        assert_eq!(results(&p), ["Passed", "Passed", "Failed", "Passed"]);
        // test_create_device_id_to_bus_map: `0x23` in 0000:01:22.0.
        let root2 = {
            drop(p);
            drop(root);
            switch_with_pcie("[]\n")
        };
        function(&root2, "0000:01:22.0", "0x23");
        let p = Pcie::new(PLATFORM).unwrap();
        assert_eq!(p.device_id_to_bus, [("23:22".to_string(), "01".to_string())]);
        assert_eq!(r(p.get_pcie_check()), "[]");
    }

    #[test]
    fn what_the_yaml_or_sysfs_cannot_give_raises_as_in_python() {
        let root = switch_with_pcie("- bus: '00'\n  id: 1f0b\n  dev: 00\n  fn: '0'\n");
        function(&root, "0000:05:00.0", "0x1f0b");
        let p = Pcie::new(PLATFORM).unwrap();
        // An unquoted 00 is the int 0: its key `1f0b:0` is no device's, so
        // nothing converts it.
        assert_eq!(results(&p), ["Failed"]);
        // An unquoted fn on a device that is found: int(0, 16) refuses it.
        root.write(&format!("{PLATFORM}/pcie.yaml"), "- bus: '00'\n  id: 1f0b\n  dev: '00'\n  fn: 0\n");
        assert_eq!(r(p.get_pcie_check()), "raise TypeError: int() can't convert non-string with explicit base");
        root.write(&format!("{PLATFORM}/pcie.yaml"), "- bus: '00'\n  dev: '00'\n  fn: '0'\n");
        assert_eq!(r(p.get_pcie_check()), "raise KeyError: 'id'");
        root.write(&format!("{PLATFORM}/pcie.yaml"), "# nothing\n");
        assert_eq!(r(p.get_pcie_check()), "raise TypeError: 'NoneType' object is not iterable");
        // A function without its device file stops the construction.
        std::fs::create_dir_all(root.real("/sys/bus/pci/devices/0000:09:00.0")).unwrap();
        let e = Pcie::new(PLATFORM).err().unwrap();
        assert_eq!(e.exc.name(), "FileNotFoundError");
        // No pcie.yaml: SystemExit, before sysfs is even looked at.
        std::fs::remove_file(root.real(&format!("{PLATFORM}/pcie.yaml"))).unwrap();
        assert_eq!(factory(PLATFORM).err().unwrap().exc.name(), "SystemExit");
    }

    #[test]
    fn a_dpu_being_detached_is_left_out() {
        // test_get_pcie_check_bluefield_*: a SmartSwitch's DPU functions,
        // checked against what chassisd has detached.
        let root = switch("x86_64-nvidia_sn4280-r0");
        root.write(
            &format!("{PLATFORM}/pcie.yaml"),
            "- bus: '08'\n  id: c2d5\n  dev: '00'\n  fn: '0'\n\
             - bus: '08'\n  id: a2dc\n  dev: '00'\n  fn: '1'\n\
             - bus: '09'\n  id: c2d5\n  dev: '00'\n  fn: '0'\n",
        );
        root.write(
            &format!("{PLATFORM}/platform.json"),
            r#"{"DPUS": {"dpu0": {"bus_info": "0000:08:00.0", "rshim_bus_info": "0000:08:00.1"},
                         "dpu1": {"bus_info": "0000:09:00.0"}}}"#,
        );
        function(&root, "0000:08:00.0", "0xc2d5");
        function(&root, "0000:09:00.0", "0xc2d5");
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        let p = Pcie::new(PLATFORM).unwrap();
        // dpu1 names no rshim function, so none of its functions are a DPU's.
        assert_eq!(p.dpu_pcie_devices, ["0000:08:00.0", "0000:08:00.1"]);
        redis.hset(STATE_DB, "PCIE_DETACH_INFO|0000:08:00.0", &[("dpu_state", "detaching")]);
        redis.hset(STATE_DB, "PCIE_DETACH_INFO|0000:08:00.1", &[("dpu_state", "attached")]);
        let PyVal::List(items) = p.get_pcie_check().unwrap() else { panic!() };
        let got: Vec<(String, String)> = items
            .iter()
            .map(|i| (pcie::item(i, "fn").unwrap().py_str(), pcie::item(i, "result").unwrap().py_str()))
            .collect();
        // 08:00.0 is detaching; 08:00.1 is not, and is not in sysfs; 09's
        // bus is found by id.
        assert_eq!(got, [("1".to_string(), "Failed".to_string()), ("0".into(), "Passed".into())]);
        function(&root, "0000:08:00.1", "0xa2dc");
        redis.hset(STATE_DB, "PCIE_DETACH_INFO|0000:08:00.0", &[("dpu_state", "attached")]);
        let p = Pcie::new(PLATFORM).unwrap();
        assert_eq!(results(&p), ["Passed", "Passed", "Passed"]);
        redis.flush();
    }

    #[test]
    fn a_dpu_function_whose_check_raises_is_logged_and_left_out() {
        // pcie_check_dpu's `except Exception`: logged through self.logger,
        // and the entry dropped -- here int('zz', 16) raises.
        let root = switch("x86_64-nvidia_sn4280-r0");
        root.write(
            &format!("{PLATFORM}/pcie.yaml"),
            "- bus: 'zz'\n  id: c2d5\n  dev: '00'\n  fn: '0'\n- bus: '09'\n  id: c2d5\n  dev: '00'\n  fn: '0'\n",
        );
        root.write(
            &format!("{PLATFORM}/platform.json"),
            r#"{"DPUS": {"dpu0": {"bus_info": "0000:zz:00.0", "rshim_bus_info": "0000:zz:00.1"}}}"#,
        );
        function(&root, "0000:09:00.0", "0xc2d5");
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        let p = Pcie::new(PLATFORM).unwrap();
        assert_eq!(results(&p), ["Passed"]);
    }

    #[test]
    fn aer_stats_are_platform_base_s() {
        let root = switch_with_pcie("[]\n");
        function(&root, "0000:05:00.0", "0x1f0b");
        root.write("/sys/bus/pci/devices/0000:05:00.0/aer_dev_correctable", "RxErr 2\nBadTLP 0\n");
        let p = Pcie::new(PLATFORM).unwrap();
        assert_eq!(
            r(p.get_pcie_aer_stats(0, 5, 0, 0)),
            "{'correctable': {'RxErr': '2', 'BadTLP': '0'}, 'fatal': {}, 'non_fatal': {}}"
        );
    }
}
