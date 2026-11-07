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


//! `sonic_platform/eeprom.py`: the system EEPROM, read from STATE_DB when
//! syseepromd has published it and decoded from the VPD file otherwise.

use std::process::Command;
use std::sync::Mutex;

use platform_base::eeprom_tlvinfo::{
    hex, TlvInfoDecoder, Visitor, TLV_CODE_CRC_32, TLV_CODE_MAC_BASE,
    TLV_CODE_PART_NUMBER, TLV_CODE_PRODUCT_NAME, TLV_CODE_SERIAL_NUMBER, TLV_CODE_SERVICE_TAG,
    TLV_CODE_VENDOR_EXT,
};
use platform_base::py::{py_int, PyErr, PyResult, PyVal};
use platform_base::{io, BaseClass, PyObject};

use crate::device_data;
use crate::utils;

pub const EEPROM_SYMLINK: &str = "/var/run/hw-management/eeprom/vpd_info";
const WAIT_EEPROM_READY_SEC: f64 = 60.0;
const PLATFORM_SUPPORTING_SIMX: [&str; 6] = [
    "x86_64-nvidia_sn4280_simx-r0",
    "x86_64-mlnx_msn4700_simx-r0",
    "x86_64-nvidia_sn5640_simx-r0",
    "x86_64-nvidia_sn6600_simx-r0",
    "x86_64-nvidia_sn6600_ld_simx-r0",
    "x86_64-nvidia_sn6810_ld_simx-r0",
];

/// The module body: on a simx platform the EEPROM file is made from the
/// platform's `syseeprom.hex`, once, at import.  Errors propagate as the
/// import's, which is what `check_call` raising at module level does.
pub fn import() -> PyResult<()> {
    crate::imports::eeprom();
    platform_base::logger::import_fallible("sonic_platform.eeprom.body", module_body)
}

/// What the module body does, every time it runs.
fn module_body() -> PyResult<()> {
    let Some(p) = device_data::get_platform_name()? else { return Ok(()) };
    if !p.contains("simx") || PLATFORM_SUPPORTING_SIMX.contains(&p.as_str()) || io::exists(EEPROM_SYMLINK) {
        return Ok(());
    }
    let dir = if utils::is_host() {
        io::join("/usr/share/sonic/device", &p)
    } else {
        "/usr/share/sonic/platform".to_string()
    };
    let eeprom_dir = "/var/run/hw-management/eeprom";
    if !io::exists(eeprom_dir) {
        makedirs(eeprom_dir)?;
    }
    match Command::new("/usr/bin/xxd")
        .args(["-r", "-p", "syseeprom.hex", EEPROM_SYMLINK])
        .current_dir(io::real(&dir))
        .status()
    {
        Ok(s) if s.success() => Ok(()),
        Ok(s) => Err(platform_base::subprocess::called_process_error(
            &["/usr/bin/xxd", "-r", "-p", "syseeprom.hex", EEPROM_SYMLINK],
            platform_base::subprocess::returncode(s),
        )),
        // subprocess names the cwd when the child cannot change into it,
        // and the program otherwise.
        Err(e) => Err(PyErr::from_io(&e, Some(if io::is_dir(&dir) { "/usr/bin/xxd" } else { &dir }))),
    }
}

/// `os.makedirs(name)`: the missing parents first; the error is the
/// component that could not be made.
fn makedirs(name: &str) -> PyResult<()> {
    let (head, tail) = name.rsplit_once('/').unwrap_or(("", name));
    if !head.is_empty() && !tail.is_empty() && !io::exists(head) {
        match makedirs(head) {
            Err(e) if e.exc.name() == "FileExistsError" => {}
            r => r?,
        }
    }
    std::fs::create_dir(io::real(name)).map_err(|e| PyErr::from_io(&e, Some(name)))
}

/// `Eeprom()`.
pub struct Eeprom {
    decoder: TlvInfoDecoder,
    info: Mutex<Option<Vec<(String, PyVal)>>>,
}

impl Eeprom {
    /// Waits up to a minute for the VPD file, as Python does.
    pub fn new() -> PyResult<Eeprom> {
        import()?;
        if !utils::wait_until(|| io::exists(EEPROM_SYMLINK), WAIT_EEPROM_READY_SEC, 1.0) {
            utils::log_error("Nowhere to read syseeprom from! No symlink found");
            return Err(PyErr::runtime_error("No syseeprom symlink found"));
        }
        Ok(Eeprom {
            decoder: TlvInfoDecoder::new(EEPROM_SYMLINK, 0, "", true),
            info: Mutex::new(None),
        })
    }

    /// `get_system_eeprom_info()`, without its `@default_return({})`.
    fn system_eeprom_info(&self) -> PyResult<Vec<(String, PyVal)>> {
        if let Some(i) = utils::lock(&self.info).clone() {
            return Ok(i);
        }
        // `self._eeprom_info_dict = {}` happens before the reads: a failure
        // part way leaves it set, partly filled, for the next call.
        let mut info: Vec<(String, PyVal)> = Vec::new();
        let r = self.fill(&mut info);
        *utils::lock(&self.info) = Some(info.clone());
        r.map(|()| info)
    }

    fn fill(&self, info: &mut Vec<(String, PyVal)>) -> PyResult<()> {
        let d = &self.decoder;
        if d.redis_hget("EEPROM_INFO|State", "Initialized")?.as_deref() == Some("1") {
            for code in TLV_CODE_PRODUCT_NAME..=TLV_CODE_SERVICE_TAG {
                if let Some(v) = d.redis_hget(&format!("EEPROM_INFO|{}", hex(code)), "Value")? {
                    if !v.is_empty() {
                        info.push((hex(code), PyVal::Str(v)));
                    }
                }
            }
            let ext = hex(TLV_CODE_VENDOR_EXT);
            let num = d
                .redis_hget(&format!("EEPROM_INFO|{ext}"), "Num_vendor_ext")?
                .and_then(|n| py_int(&n).ok())
                .unwrap_or(0);
            for i in 0..num.max(0) {
                if let Some(v) = d.redis_hget(&format!("EEPROM_INFO|{ext}"), &format!("Value_{i}"))? {
                    if !v.is_empty() {
                        push_ext(info, &ext, v);
                    }
                }
            }
            if let Some(v) = d.redis_hget(&format!("EEPROM_INFO|{}", hex(TLV_CODE_CRC_32)), "Value")? {
                if !v.is_empty() {
                    info.push((hex(TLV_CODE_CRC_32), PyVal::Str(v)));
                }
            }
        } else {
            let e = d.read_eeprom()?;
            let mut v = ContentVisitor(info);
            d.visit_eeprom(&e, &mut v)?;
        }
        Ok(())
    }

    /// `_get_eeprom_value(code)`: `KeyError` when the code is absent.
    fn value(&self, code: u8) -> PyResult<PyVal> {
        let info = self.system_eeprom_info()?;
        info.iter()
            .find(|(k, _)| *k == hex(code))
            .map(|(_, v)| v.clone())
            .ok_or_else(|| PyErr::key_error(hex(code)))
    }

    fn get(&self, code: u8, method: &str) -> PyVal {
        utils::default_return(PyVal::str("Undefined."), self.value(code), method)
    }

    pub fn get_base_mac(&self) -> PyVal {
        self.get(TLV_CODE_MAC_BASE, "get_base_mac")
    }
    pub fn get_serial_number(&self) -> PyVal {
        self.get(TLV_CODE_SERIAL_NUMBER, "get_serial_number")
    }
    pub fn get_product_name(&self) -> PyVal {
        self.get(TLV_CODE_PRODUCT_NAME, "get_product_name")
    }
    pub fn get_part_number(&self) -> PyVal {
        self.get(TLV_CODE_PART_NUMBER, "get_part_number")
    }

    /// `get_system_eeprom_info()`: `{}` on any failure.
    pub fn get_system_eeprom_info(&self) -> PyVal {
        let r = self.system_eeprom_info().map(|i| {
            PyVal::Dict(i.into_iter().map(|(k, v)| (PyVal::Str(k), v)).collect())
        });
        utils::default_return(PyVal::Dict(Vec::new()), r, "get_system_eeprom_info")
    }
}

fn push_ext(info: &mut Vec<(String, PyVal)>, ext: &str, v: String) {
    match info.iter_mut().find(|(k, _)| k == ext) {
        Some((_, PyVal::List(l))) => l.push(PyVal::Str(v)),
        _ => info.push((ext.to_string(), PyVal::List(vec![PyVal::Str(v)]))),
    }
}

/// `EepromContentVisitor`.
struct ContentVisitor<'a>(&'a mut Vec<(String, PyVal)>);

impl Visitor for ContentVisitor<'_> {
    fn visit_tlv(&mut self, _name: &str, code: u8, _length: u8, value: &str) -> PyResult<()> {
        if code != TLV_CODE_VENDOR_EXT {
            let v = PyVal::Str(value.trim_end_matches('\0').to_string());
            match self.0.iter_mut().find(|(k, _)| *k == hex(code)) {
                Some(e) => e.1 = v,
                None => self.0.push((hex(code), v)),
            }
        } else if !value.is_empty() {
            let value = value.trim_end_matches('\0');
            if !value.is_empty() {
                push_ext(self.0, &hex(code), value.to_string());
            }
        }
        Ok(())
    }
}

impl PyObject for Eeprom {
    fn py_class(&self) -> &'static str {
        "Eeprom"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::TlvInfoDecoder
    }
    fn read_eeprom(&self) -> PyResult<PyVal> {
        self.decoder.read_eeprom().map(PyVal::Bytes)
    }
    fn update_eeprom_db(&self, e: PyVal) -> PyResult<PyVal> {
        match e {
            PyVal::Bytes(b) => self.decoder.update_eeprom_db(&b).map(PyVal::Int),
            other => Err(PyErr::type_error(format!(
                "object of type '{}' has no len()",
                other.type_name()
            ))),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, STATE_DB};

    /// Python's tests/mock_eeprom_data: an SN3800's.
    const MOCK_EEPROM: &[u8] = include_bytes!("../tests/data/mock_eeprom_data");

    fn eeprom(root: &Root) -> Eeprom {
        root.write(EEPROM_SYMLINK, "");
        Eeprom::new().unwrap()
    }

    fn keys(v: &PyVal) -> Vec<String> {
        v.iter_items().unwrap().iter().map(PyVal::py_str).collect()
    }

    /// Python's test_get_system_eeprom_info_from_hardware.
    #[test]
    fn without_syseepromd_the_vpd_file_is_decoded() {
        let root = Root::new();
        let e = eeprom(&root);
        std::fs::write(root.real(EEPROM_SYMLINK), MOCK_EEPROM).unwrap();
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        assert_eq!(e.get_product_name(), PyVal::str("MSN3800"));
        assert_eq!(e.get_part_number(), PyVal::str("MSN3800-CS2FO"));
        assert_eq!(e.get_base_mac(), PyVal::str("B8:59:9F:A9:34:00"));
        assert_eq!(e.get_serial_number(), PyVal::str("MT1937X00537"));
        let info = e.get_system_eeprom_info();
        assert_eq!(json_get(&info, "0xfe"), PyVal::str("0x9EFF0119"));
        assert!(matches!(json_get(&info, "0xfd"), PyVal::List(l) if !l.is_empty()));
        assert_eq!(e.read_eeprom().unwrap(), PyVal::Bytes(MOCK_EEPROM.to_vec()));
        // The decoded TLVs go to STATE_DB.
        assert_eq!(e.update_eeprom_db(PyVal::Bytes(MOCK_EEPROM.to_vec())).unwrap(), PyVal::Int(0));
        assert!(redis.exists(STATE_DB, "EEPROM_INFO|State"));
        let err = e.update_eeprom_db(PyVal::Int(1)).unwrap_err();
        assert_eq!((err.exc.name(), err.str()), ("TypeError", "object of type 'int' has no len()".to_string()));
        assert_eq!((e.py_class(), e.base_class()), ("Eeprom", BaseClass::TlvInfoDecoder));
    }

    fn json_get(d: &PyVal, k: &str) -> PyVal {
        platform_base::json::get(d, k).cloned().unwrap_or_default()
    }

    /// Python's test_get_system_eeprom_info_from_db.
    #[test]
    fn once_syseepromd_has_run_the_values_come_from_state_db() {
        let root = Root::new();
        let e = eeprom(&root);
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        redis.hset(STATE_DB, "EEPROM_INFO|State", &[("Initialized", "1")]);
        for (code, v) in [("0x22", "MSN3420-CB2FO"), ("0x21", "MSN3420"), ("0x24", "1C:34:DA:1C:9F:00"),
                          ("0x23", "MT2019X13878"), ("0x25", "")] {
            redis.hset(STATE_DB, &format!("EEPROM_INFO|{code}"), &[("Value", v)]);
        }
        redis.hset(STATE_DB, "EEPROM_INFO|0xfd", &[("Num_vendor_ext", "3"), ("Value_0", "ext1"), ("Value_1", ""), ("Value_2", "ext2")]);
        redis.hset(STATE_DB, "EEPROM_INFO|0xfe", &[("Value", "CRC_VALUE")]);
        let info = e.get_system_eeprom_info();
        assert_eq!(keys(&info), ["0x21", "0x22", "0x23", "0x24", "0xfd", "0xfe"]);
        assert_eq!(e.get_product_name(), PyVal::str("MSN3420"));
        assert_eq!(e.get_part_number(), PyVal::str("MSN3420-CB2FO"));
        assert_eq!(e.get_base_mac(), PyVal::str("1C:34:DA:1C:9F:00"));
        assert_eq!(e.get_serial_number(), PyVal::str("MT2019X13878"));
        assert_eq!(json_get(&info, "0xfd"), PyVal::List(vec![PyVal::str("ext1"), PyVal::str("ext2")]));
        assert_eq!(json_get(&info, "0xfe"), PyVal::str("CRC_VALUE"));
        // A count that is not a number is none.
        let e = eeprom(&root);
        redis.hset(STATE_DB, "EEPROM_INFO|0xfd", &[("Num_vendor_ext", "x")]);
        redis.hdel(STATE_DB, "EEPROM_INFO|0xfe", "Value");
        assert_eq!(keys(&e.get_system_eeprom_info()), ["0x21", "0x22", "0x23", "0x24"]);
    }

    #[test]
    fn a_failed_read_leaves_what_it_read_cached() {
        let root = Root::new();
        // No STATE_DB to ask: {} and Undefined., and {} stays.
        let e = eeprom(&root);
        assert_eq!(e.get_system_eeprom_info(), PyVal::Dict(vec![]));
        assert_eq!(e.get_base_mac(), PyVal::str("Undefined."));
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        redis.hset(STATE_DB, "EEPROM_INFO|State", &[("Initialized", "1")]);
        redis.hset(STATE_DB, "EEPROM_INFO|0x21", &[("Value", "MSN3420")]);
        assert_eq!(e.get_product_name(), PyVal::str("Undefined."));
        // A fresh one finds it, but no serial number.
        let e = eeprom(&root);
        assert_eq!(e.get_product_name(), PyVal::str("MSN3420"));
        assert_eq!(e.get_serial_number(), PyVal::str("Undefined."));
    }

    /// Python's test_eeprom_content_visitor.
    #[test]
    fn the_content_visitor_keeps_values_and_lists_vendor_extensions() {
        let mut content = Vec::new();
        let mut v = ContentVisitor(&mut content);
        v.visit_tlv("tlv1", TLV_CODE_PRODUCT_NAME, 7, "MSN3420\0\0").unwrap();
        v.visit_tlv("tlv1", TLV_CODE_PRODUCT_NAME, 7, "MSN3800").unwrap();
        v.visit_tlv("tlv2", TLV_CODE_VENDOR_EXT, 4, "ext1").unwrap();
        v.visit_tlv("tlv3", TLV_CODE_VENDOR_EXT, 4, "").unwrap();
        v.visit_tlv("tlv3", TLV_CODE_VENDOR_EXT, 4, "\0\0").unwrap();
        v.visit_tlv("tlv4", TLV_CODE_VENDOR_EXT, 4, "ext2\0").unwrap();
        assert_eq!(
            content,
            vec![
                ("0x21".to_string(), PyVal::str("MSN3800")),
                ("0xfd".to_string(), PyVal::List(vec![PyVal::str("ext1"), PyVal::str("ext2")])),
            ]
        );
    }

    /// A simx platform the table does not list makes its EEPROM file from
    /// `syseeprom.hex` with xxd.  Here there is no such file (and maybe no
    /// xxd), so the body fails as `check_call` does; which way depends on
    /// whether xxd is installed.
    #[test]
    fn a_simx_platform_makes_its_eeprom_file_from_its_hex_dump() {
        let root = Root::new();
        let p = "x86_64-nvidia_sn5600_simx-r0";
        if crate::utils::tests::platform(&root, p).as_deref() != Some(p) {
            return;
        }
        // On the host, from the device directory; it is not there.
        let e = module_body().unwrap_err();
        assert_eq!(e.exc.name(), "FileNotFoundError");
        assert!(root.exists("/var/run/hw-management/eeprom"));
        // In the container, from the platform directory, which is.
        std::fs::create_dir_all(root.real("/usr/share/sonic/platform")).unwrap();
        root.write("/.dockerenv", "");
        crate::utils::forget_caches();
        let e = module_body().unwrap_err();
        assert!(matches!(e.exc.name(), "CalledProcessError" | "FileNotFoundError"), "{}", e.repr());
        // The file already there, a listed simx platform, or no simx at all:
        // nothing to do.
        root.write(EEPROM_SYMLINK, "");
        assert!(module_body().is_ok());
        for p in ["x86_64-nvidia_sn5640_simx-r0", "x86_64-nvidia_sn5600-r0"] {
            crate::utils::tests::platform(&root, p);
            crate::utils::forget_caches();
            assert!(module_body().is_ok());
        }
        assert!(import().is_ok());
    }

    #[test]
    fn makedirs_names_the_component_it_could_not_make() {
        let root = Root::new();
        root.write("/var/run/hw-management", "a file");
        let e = makedirs("/var/run/hw-management/eeprom").unwrap_err();
        assert_eq!(e.str(), "[Errno 20] Not a directory: '/var/run/hw-management/eeprom'");
        std::fs::remove_file(root.real("/var/run/hw-management")).unwrap();
        makedirs("/var/run/hw-management/eeprom").unwrap();
        assert!(root.real("/var/run/hw-management/eeprom").is_dir());
    }
}
