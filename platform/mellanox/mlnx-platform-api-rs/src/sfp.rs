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


//! `sonic_platform/sfp.py`, as far as the platform API reaches without
//! `sonic_xcvr`: the port objects the chassis builds, their presence and
//! EEPROM reads, and the values xcvrd leaves in STATE_DB that the thermal
//! updater forwards to hw-management.  The module state machine that
//! `get_change_event` drives is in sfp_events.rs.

use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use platform_base::db::Db;
use platform_base::py::{py_float, py_int, PyErr, PyResult};
use platform_base::{io, multi_asic};

use crate::db_table_helper::{self as dbt, TRANSCEIVER_DOM_TEMPERATURE_TABLE, TRANSCEIVER_DOM_THRESHOLD_TABLE, TRANSCEIVER_INFO_TABLE};
use crate::device_data;
use crate::module_host_mgmt_initializer::get_asic_ready_file_path;
use crate::utils::{self, Log};

const SFP_SDK_MODULE_SYSFS_ROOT: &str = "/sys/module/sx_core/asic0/module";
const SFP_PAGE0_PATH: &str = "0/i2c-0x50/data";
const SFP_A2H_PAGE0_PATH: &str = "0/i2c-0x51/data";
const SFP_PAGE_SIZE: i128 = 256;
const SFP_UPPER_PAGE_OFFSET: i128 = 128;
const SFP_EEPROM_NOT_AVAILABLE: &str = "Input/output error";

const STATE_DOWN: &str = "Down";
const STATE_FCP_DOWN: &str = "Down(Firmware Control)";

const PORT_CONFIG_DONE: &str = "PORT_TABLE:PortConfigDone";

/// `from . import sfp`: the module imports hw-management's helper after its
/// own imports and before its logger, and fails without it.
pub fn import() -> PyResult<()> {
    crate::imports::utils();
    crate::imports::db_table_helper();
    crate::imports::device_data();
    crate::imports::module_host_mgmt_initializer();
    if !io::exists(hw_mgmt::independent_mode_update::MODULE_PATH) {
        return Err(PyErr::other(
            "ModuleNotFoundError",
            "No module named 'hw_management_independent_mode_update'",
        ));
    }
    crate::imports::sfp();
    Ok(())
}

/// `SFP.port_mapping`: a class attribute, so every port shares it.
static PORT_MAPPING: Mutex<Option<HashMap<i128, String>>> = Mutex::new(None);

/// `SFP`, `CpoPort` (an `SFP`) or `RJ45Port` (only `NvidiaSFPCommon`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Sfp,
    Cpo,
    Rj45,
}

/// The type `_get_sfp_type_str` reads from byte 0.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum SfpType {
    Cmis,
    Sff8636,
    Sff8472,
}

pub struct Sfp {
    kind: Kind,
    /// `self.index`: 1-based.
    pub index: i128,
    /// `self.sdk_index`: 0-based.
    pub sdk_index: i128,
    pub asic_id: String,
    /// A line card port's `self.index`: a float, from a true division.
    lc_index: Option<f64>,
    asic_index: i128,
    namespace: String,
    sfp_type_str: Mutex<Option<SfpType>>,
    /// `self.state`, for the module host management state machine.
    pub(crate) state: Mutex<&'static str>,
    /// `self.processing_insert_event`.
    pub(crate) processing_insert_event: std::sync::atomic::AtomicBool,
}

impl Sfp {
    /// `SFP(index, asic_id=...)`, `CpoPort(...)` or `RJ45Port(...)`.
    pub fn new(kind: Kind, sfp_index: i128, asic_id: &str) -> PyResult<Arc<Sfp>> {
        // NvidiaSFPCommon.__init__
        let asic_index = multi_asic::get_asic_index_from_namespace(asic_id)?;
        let mut state = STATE_DOWN;
        let mut namespace = multi_asic::DEFAULT_NAMESPACE.to_string();
        if kind != Kind::Rj45 {
            // `from .thermal import initialize_sfp_thermal`; its ModuleThermal
            // is reached only through the SFP, which no facade method visits.
            crate::imports::thermal();
            if device_data::get_always_fw_control_ports()?.is_some_and(|p| p.contains(&sfp_index)) {
                state = STATE_FCP_DOWN;
            }
            if device_data::is_multi_asic_platform()? {
                namespace = asic_id.to_string();
            }
        }
        Ok(Arc::new(Sfp {
            kind,
            index: sfp_index + 1,
            sdk_index: sfp_index,
            asic_id: asic_id.to_string(),
            lc_index: None,
            asic_index,
            namespace,
            sfp_type_str: Mutex::new(None),
            state: Mutex::new(state),
            processing_insert_event: std::sync::atomic::AtomicBool::new(false),
        }))
    }

    /// `SFP(sfp_index, slot_id=..., linecard_port_count=..., lc_name=...)`:
    /// a line card's port, numbered across the chassis.
    pub fn new_linecard(sfp_index: i128, slot_id: i128, linecard_port_count: i128, lc_name: &str) -> PyResult<Arc<Sfp>> {
        let base = Sfp::new(Kind::Sfp, sfp_index, "asic0")?;
        let max_linecard_count = device_data::get_linecard_count();
        let max_linecard_port_count = device_data::get_linecard_max_port_count()?;
        if max_linecard_count == 0 {
            return Err(PyErr::other("ZeroDivisionError", "integer modulo by zero"));
        }
        if linecard_port_count == 0 {
            return Err(PyErr::other("ZeroDivisionError", "division by zero"));
        }
        let index = (slot_id.rem_euclid(max_linecard_count) - 1) as f64 * max_linecard_port_count as f64
            + sfp_index as f64 * (max_linecard_port_count as f64 / linecard_port_count as f64)
            + 1.0;
        crate::thermal::initialize_linecard_sfp_thermal(lc_name, slot_id, sfp_index);
        let mut s = Arc::try_unwrap(base).map_err(|_| PyErr::runtime_error("SFP shared while built"))?;
        s.lc_index = Some(index);
        Ok(Arc::new(s))
    }

    /// The key `self.index` finds in the port mapping: a float matches the
    /// int it equals, and nothing else.
    fn mapping_key(&self) -> Option<i128> {
        match self.lc_index {
            None => Some(self.index),
            Some(f) if f.is_finite() && f.fract() == 0.0 => Some(f as i128),
            Some(_) => None,
        }
    }

    /// What `reinit()` forgets of an `SFP`: the module's EEPROM type.
    pub(crate) fn forget_type(&self) {
        *utils::lock(&self.sfp_type_str) = None;
    }

    pub fn kind(&self) -> Kind {
        self.kind
    }

    pub(crate) fn class_name(&self) -> &'static str {
        match self.kind {
            Kind::Sfp => "SFP",
            Kind::Cpo => "CpoPort",
            Kind::Rj45 => "RJ45Port",
        }
    }

    /// What an `RJ45Port` answers for a method only `SFP` has.
    pub(crate) fn sfp_only(&self, method: &str) -> PyResult<()> {
        if self.kind == Kind::Rj45 {
            return Err(PyErr::missing(self.class_name(), method));
        }
        Ok(())
    }

    pub(crate) fn sysfs(&self, node: &str) -> String {
        format!("{SFP_SDK_MODULE_SYSFS_ROOT}{}/{node}", self.sdk_index)
    }

    /// `get_presence()`.
    pub fn get_presence(&self) -> bool {
        if self.kind == Kind::Rj45 {
            return utils::read_int(&self.sysfs("present"), 0, Log::Error) == 1;
        }
        match self.presence() {
            Ok(p) => p,
            Err(e) => {
                utils::log_warning(&format!("Failed to check presence of SFP {}: {}", self.sdk_index, e.str()));
                false
            }
        }
    }

    fn presence(&self) -> PyResult<bool> {
        let n = py_int(&self.asic_id.replace("asic", ""))? + 1;
        if utils::read_int(&format!("/var/run/hw-management/config/asic{n}_ready"), 0, Log::Off) != 1 {
            return Ok(false);
        }
        if device_data::is_module_host_management_mode()? && !io::exists(&get_asic_ready_file_path(&self.asic_id)) {
            return Ok(false);
        }
        if self.is_sw_control()? {
            return Ok(utils::read_int(&self.sysfs("hw_present"), 0, Log::Off) == 1);
        }
        if utils::read_int(&self.sysfs("present"), 0, Log::Off) != 0 {
            return Ok(self.read_eeprom(0, 1, false)?.is_some());
        }
        Ok(false)
    }

    /// `is_sw_control()`.
    pub fn is_sw_control(&self) -> PyResult<bool> {
        if !device_data::is_module_host_management_mode()? {
            return Ok(false);
        }
        utils::read_int_raise(&self.sysfs("control"), Log::Off)
            .map(|v| v == 1)
            .map_err(|_| PyErr::other("Exception", format!("control sysfs for SFP {} does not exist", self.sdk_index)))
    }

    /// `check_eeprom_ready_if_present()`.
    pub fn check_eeprom_ready_if_present(&self) -> PyResult<bool> {
        self.sfp_only("check_eeprom_ready_if_present")?;
        let node = if self.is_sw_control()? { "hw_present" } else { "present" };
        if utils::read_int(&self.sysfs(node), 0, Log::Off) == 0 {
            return Ok(true);
        }
        Ok(self.read_eeprom(0, 1, false)?.is_some())
    }

    fn eeprom_path(&self) -> String {
        format!("{SFP_SDK_MODULE_SYSFS_ROOT}{}/eeprom/pages", self.sdk_index)
    }

    /// `_get_page_and_page_offset(overall_offset)`: the page file and offset.
    fn page_and_offset(&self, overall_offset: i128) -> PyResult<Option<(String, i128)>> {
        let eeprom_path = self.eeprom_path();
        if !io::exists(&eeprom_path) {
            utils::log_error(&format!("EEPROM file path for sfp {} does not exist", self.sdk_index));
            return Ok(None);
        }
        if overall_offset < SFP_PAGE_SIZE {
            return Ok(Some((io::join(&eeprom_path, SFP_PAGE0_PATH), overall_offset)));
        }
        let page1h_start = if self.sfp_type_str(&eeprom_path)? == Some(SfpType::Sff8472) {
            let start = SFP_PAGE_SIZE * 2;
            if overall_offset < start {
                return Ok(Some((io::join(&eeprom_path, SFP_A2H_PAGE0_PATH), overall_offset - SFP_PAGE_SIZE)));
            }
            start
        } else {
            SFP_PAGE_SIZE
        };
        let rel = overall_offset - page1h_start;
        let page_num = rel.div_euclid(SFP_UPPER_PAGE_OFFSET) + 1;
        Ok(Some((io::join(&eeprom_path, &format!("{page_num}/data")), rel.rem_euclid(SFP_UPPER_PAGE_OFFSET))))
    }

    /// `_get_sfp_type_str(eeprom_path)`: cached once known.
    fn sfp_type_str(&self, eeprom_path: &str) -> PyResult<Option<SfpType>> {
        if let Some(t) = *utils::lock(&self.sfp_type_str) {
            return Ok(Some(t));
        }
        let page = io::join(eeprom_path, SFP_PAGE0_PATH);
        let t = match read_at(&page, 0, 1) {
            Ok(b) => match b.first() {
                Some(0x18 | 0x19 | 0x1e | 0x80) => Some(SfpType::Cmis),
                Some(0x11 | 0x0d) => Some(SfpType::Sff8636),
                Some(0x03) => Some(SfpType::Sff8472),
                Some(id) => {
                    utils::log_error(&format!("Unsupported sfp type {id}"));
                    None
                }
                // `bytearray(b'')[0]`: an IndexError, which the `except
                // OSError` does not take, out through `_read_eeprom`.
                None => return Err(PyErr::index_error("bytearray index out of range")),
            },
            Err(e) => {
                let e = PyErr::from_io(&e, Some(&page));
                if !e.str().contains(SFP_EEPROM_NOT_AVAILABLE) {
                    utils::log_error(&format!("Failed to get SFP type, index={}, error={}", self.sdk_index, e.str()));
                }
                return Ok(None);
            }
        };
        *utils::lock(&self.sfp_type_str) = t;
        Ok(t)
    }

    /// `_read_eeprom(offset, num_bytes, log_on_error)`: what it reads, or
    /// None; it raises only what finding the page raises.
    pub fn read_eeprom(&self, offset: i128, num_bytes: i128, log_on_error: bool) -> PyResult<Option<Vec<u8>>> {
        let mut offset = offset;
        let mut num_bytes = num_bytes;
        let mut result: Vec<u8> = Vec::new();
        while num_bytes > 0 {
            let Some((page, page_offset)) = self.page_and_offset(offset)? else {
                return Ok(None);
            };
            let r = (|| -> std::io::Result<Option<usize>> {
                use std::io::{Read, Seek, SeekFrom};
                let mut f = std::fs::File::open(io::real(&page))?;
                f.seek(SeekFrom::Start(u64::try_from(page_offset).unwrap_or(0)))?;
                let mut buf = vec![0u8; usize::try_from(num_bytes).unwrap_or(0)];
                let n = f.read(&mut buf)?;
                buf.truncate(n);
                result.extend_from_slice(&buf);
                if n == 0 {
                    return Ok(None);
                }
                num_bytes -= n as i128;
                if num_bytes > 0 {
                    let page_size = f.seek(SeekFrom::End(0))? as i128;
                    if page_offset + n as i128 == page_size {
                        offset += n as i128;
                    } else {
                        num_bytes = 0;
                    }
                }
                Ok(Some(n))
            })();
            match r {
                Ok(Some(_)) => {}
                Ok(None) => {
                    utils::log_error(&format!("SFP {}: EEPROM page {page} is empty, no data retrieved", self.sdk_index));
                    return Ok(None);
                }
                Err(e) => {
                    if log_on_error {
                        let e = PyErr::from_io(&e, Some(&page));
                        utils::log_warning(&format!(
                            "Failed to read sfp={} EEPROM page={page}, page_offset={page_offset}, size={num_bytes}, offset={offset}, error = {}",
                            self.sdk_index,
                            e.str()
                        ));
                    }
                    return Ok(None);
                }
            }
        }
        Ok(Some(result))
    }

    /// `get_asic_index()`.
    pub fn get_asic_index(&self) -> PyResult<i128> {
        self.sfp_only("get_asic_index")?;
        Ok(self.asic_index)
    }

    /// `get_logical_port()`.
    fn get_logical_port(&self) -> PyResult<Option<String>> {
        if let Some(p) = self.mapping_key().and_then(mapped) {
            return Ok(Some(p));
        }
        // get_port_config_done(namespace): the namespace's APPL_DB.
        if !dbt::appl_db_exists(&self.namespace, PORT_CONFIG_DONE)? {
            return Ok(None);
        }
        build_port_mapping(&self.namespace)?;
        Ok(self.mapping_key().and_then(mapped))
    }

    /// `_get_data_from_db(table_cb, key)`: `(present, value)`.
    fn data_from_db(&self, table: &str, field: &str) -> PyResult<(bool, String)> {
        let Some(port) = self.get_logical_port()? else {
            return Ok((false, String::new()));
        };
        let (present, v) = dbt::table_hget(table, &port, field)?;
        Ok((present, v.unwrap_or_default()))
    }

    /// `get_temperature_from_db()`: 0 when absent, -1 when xcvrd wrote None.
    pub fn get_temperature_from_db(&self) -> PyResult<f64> {
        self.sfp_only("get_temperature_from_db")?;
        let (present, value) = self.data_from_db(TRANSCEIVER_DOM_TEMPERATURE_TABLE, "temperature")?;
        if !present {
            return Ok(0.0);
        }
        if value == "None" {
            return Ok(-1.0);
        }
        py_float(&value)
    }

    fn threshold_from_db(&self, method: &str, field: &str) -> PyResult<f64> {
        self.sfp_only(method)?;
        let (present, value) = self.data_from_db(TRANSCEIVER_DOM_THRESHOLD_TABLE, field)?;
        if !present || value == "N/A" {
            return Ok(0.0);
        }
        py_float(&value)
    }

    /// `get_warning_threshold_from_db()`.
    pub fn get_warning_threshold_from_db(&self) -> PyResult<f64> {
        self.threshold_from_db("get_warning_threshold_from_db", "temphighwarning")
    }

    /// `get_critical_threshold_from_db()`.
    pub fn get_critical_threshold_from_db(&self) -> PyResult<f64> {
        self.threshold_from_db("get_critical_threshold_from_db", "temphighalarm")
    }

    fn info_from_db(&self, method: &str, field: &str) -> PyResult<String> {
        self.sfp_only(method)?;
        let (present, value) = self.data_from_db(TRANSCEIVER_INFO_TABLE, field)?;
        if !present {
            return Ok(String::new());
        }
        Ok(platform_base::py::strip(&value).to_string())
    }

    /// `get_vendor_name_from_db()`.
    pub fn get_vendor_name_from_db(&self) -> PyResult<String> {
        self.info_from_db("get_vendor_name_from_db", "manufacturer")
    }

    /// `get_part_number_from_db()`.
    pub fn get_part_number_from_db(&self) -> PyResult<String> {
        self.info_from_db("get_part_number_from_db", "model")
    }
}

/// `open(page, 'rb', buffering=0)`, `seek`, `read(n)`.
fn read_at(path: &str, offset: u64, n: usize) -> std::io::Result<Vec<u8>> {
    use std::io::{Read, Seek, SeekFrom};
    let mut f = std::fs::File::open(io::real(path))?;
    f.seek(SeekFrom::Start(offset))?;
    let mut buf = vec![0u8; n];
    let got = f.read(&mut buf)?;
    buf.truncate(got);
    Ok(buf)
}

fn mapped(index: i128) -> Option<String> {
    utils::lock(&PORT_MAPPING).as_ref().and_then(|m| m.get(&index).cloned()).filter(|p| !p.is_empty())
}

/// `build_port_mapping(namespace)`: CONFIG_DB's PORT table, in natural
/// order of port name, first port per index wins.
fn build_port_mapping(namespace: &str) -> PyResult<()> {
    // A namespace other than the default one only exists on a multi-ASIC
    // system, where get_appl_db() has already raised KeyError('').
    if !namespace.is_empty() {
        return Err(PyErr::key_error(""));
    }
    let db = Db::connect("CONFIG_DB")?;
    let mut table = db.get_table("PORT")?;
    table.sort_by_key(|(k, _)| utils::natsort_key(k));
    for (logical_port, fields) in table {
        let index = match fields.iter().find(|(f, _)| f == "index") {
            Some((_, v)) => py_int(v)?,
            None => {
                return Err(PyErr::type_error(
                    "int() argument must be a string, a bytes-like object or a real number, not 'NoneType'",
                ))
            }
        };
        utils::lock(&PORT_MAPPING).get_or_insert_with(HashMap::new).entry(index).or_insert(logical_port);
    }
    Ok(())
}

/// Forget `SFP.port_mapping`, for a test that builds its own.
#[cfg(test)]
pub(crate) fn forget_port_mapping() {
    if let Ok(mut m) = PORT_MAPPING.lock() {
        *m = None;
    }
}

/// The platform fixtures the SFP and change-event tests share.
#[cfg(test)]
#[path = "test_support_c.rs"]
pub(crate) mod test_support;

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, CONFIG_DB, STATE_DB};
    use test_support::{eeprom, module, platform};

    const M0: &str = "/sys/module/sx_core/asic0/module0";

    #[test]
    fn eeprom_reads_cross_into_the_upper_pages() {
        let r = Root::new();
        let mut page0 = vec![0u8; 256];
        page0[0] = 0x18;
        page0[255] = 0xaa;
        std::fs::create_dir_all(io::real(&format!("{M0}/eeprom/pages/0/i2c-0x50"))).unwrap();
        std::fs::write(io::real(&format!("{M0}/eeprom/pages/0/i2c-0x50/data")), &page0).unwrap();
        r.write(&format!("{M0}/eeprom/pages/1/data"), "\u{1}\u{2}");
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        assert_eq!(sfp.read_eeprom(0, 1, true).unwrap(), Some(vec![0x18]));
        // CMIS: page 1 starts at 256, 128 bytes per upper page.
        assert_eq!(sfp.page_and_offset(256).unwrap().unwrap().1, 0);
        assert!(sfp.page_and_offset(256).unwrap().unwrap().0.ends_with("/eeprom/pages/1/data"));
        assert!(sfp.page_and_offset(384 + 5).unwrap().unwrap().0.ends_with("/eeprom/pages/2/data"));
        assert_eq!(sfp.page_and_offset(384 + 5).unwrap().unwrap().1, 5);
        // Reading past the end of page 0 continues on page 1.
        assert_eq!(sfp.read_eeprom(255, 3, true).unwrap(), Some(vec![0xaa, 1, 2]));
    }

    #[test]
    fn a_missing_or_empty_eeprom_reads_as_none() {
        let r = Root::new();
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        assert_eq!(sfp.read_eeprom(0, 1, false).unwrap(), None);
        r.write(&format!("{M0}/eeprom/pages/0/i2c-0x50/data"), "");
        assert_eq!(sfp.read_eeprom(0, 1, false).unwrap(), None);
    }

    #[test]
    fn an_rj45_port_lacks_what_only_sfp_defines() {
        let _r = Root::new();
        let port = Sfp::new(Kind::Rj45, 3, "asic0").unwrap();
        assert_eq!(port.get_asic_index().unwrap_err().str(), "'RJ45Port' object has no attribute 'get_asic_index'");
        assert_eq!(
            port.get_temperature_from_db().unwrap_err().str(),
            "'RJ45Port' object has no attribute 'get_temperature_from_db'"
        );
    }

    #[test]
    fn a_port_on_no_known_asic_fails_presence_with_pythons_message() {
        let _r = Root::new();
        let sfp = Sfp::new(Kind::Sfp, 7, "asicNone").unwrap();
        assert_eq!(sfp.presence().unwrap_err().str(), "invalid literal for int() with base 10: 'None'");
        assert!(!sfp.get_presence());
    }

    #[test]
    fn the_module_needs_hw_management_s_helper() {
        let root = Root::new();
        let e = import().unwrap_err();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("ModuleNotFoundError", "No module named 'hw_management_independent_mode_update'".to_string())
        );
        root.write(hw_mgmt::independent_mode_update::MODULE_PATH, "");
        import().unwrap();
    }

    #[test]
    fn each_kind_of_port_is_its_python_class() {
        let _root = Root::new();
        for (kind, class) in [(Kind::Sfp, "SFP"), (Kind::Cpo, "CpoPort"), (Kind::Rj45, "RJ45Port")] {
            let p = Sfp::new(kind, 4, "asic0").unwrap();
            assert_eq!((p.kind(), p.class_name(), p.index, p.sdk_index), (kind, class, 5, 4));
            assert_eq!(p.mapping_key(), Some(5));
        }
        assert_eq!(Sfp::new(Kind::Sfp, 4, "asic0").unwrap().get_asic_index().unwrap(), 0);
    }

    #[test]
    fn a_line_card_port_is_numbered_across_the_chassis() {
        if crate::testing::platform_from_env() {
            return;
        }
        let root = Root::new();
        root.write("/host/machine.conf", "onie_platform=x86_64-nvidia_sn4800-r0\n");
        // No line cards: Python's modulo by zero.
        let e = Sfp::new_linecard(0, 1, 16, "LC1").err().unwrap();
        assert_eq!((e.exc.name(), e.str()), ("ZeroDivisionError", "integer modulo by zero".to_string()));
        root.write("/run/hw-management/config/hotplug_linecards", "8\n");
        crate::utils::forget_caches();
        let e = Sfp::new_linecard(0, 1, 0, "LC1").err().unwrap();
        assert_eq!((e.exc.name(), e.str()), ("ZeroDivisionError", "division by zero".to_string()));
        // (slot % 8 - 1) * 16 + index * (16 / count) + 1, a float.
        let p = Sfp::new_linecard(2, 2, 16, "LC2").unwrap();
        assert_eq!((p.lc_index, p.mapping_key(), p.sdk_index), (Some(19.0), Some(19), 2));
        let p = Sfp::new_linecard(1, 1, 32, "LC1").unwrap();
        assert_eq!((p.lc_index, p.mapping_key()), (Some(1.5), None));
    }

    #[test]
    fn presence_follows_the_asic_the_control_and_the_eeprom() {
        // Legacy: the ASIC ready, `present`, and an EEPROM that reads.
        let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
        let s = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        module(&root, 0, &[("present", "1")]);
        assert!(!s.get_presence());
        root.write("/var/run/hw-management/config/asic1_ready", "1\n");
        assert!(!s.get_presence());
        assert!(!s.check_eeprom_ready_if_present().unwrap());
        eeprom(&root, 0);
        assert!(s.get_presence());
        assert!(s.check_eeprom_ready_if_present().unwrap());
        module(&root, 0, &[("present", "0")]);
        assert!(!s.get_presence());
        assert!(s.check_eeprom_ready_if_present().unwrap());
        assert!(!s.is_sw_control().unwrap());
        // An RJ45 port reads `present` alone.
        let r = Sfp::new(Kind::Rj45, 1, "asic0").unwrap();
        module(&root, 1, &[("present", "1")]);
        assert!(r.get_presence());
        assert_eq!(r.check_eeprom_ready_if_present().unwrap_err().exc.name(), "AttributeError");
        drop(root);

        // Module host management: the ASIC's ready file, then the control.
        let root = platform("x86_64-nvidia_sn5600-r0", 2, true, false);
        root.write("/var/run/hw-management/config/asic1_ready", "1\n");
        let s = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        module(&root, 0, &[("hw_present", "1")]);
        assert!(!s.get_presence());
        crate::module_host_mgmt_initializer::ModuleHostMgmtInitializer::new().unwrap();
        root.write(&get_asic_ready_file_path("asic0"), "asic0\n");
        // No `control`: the exception, logged, is absence.
        assert!(!s.get_presence());
        assert_eq!(s.is_sw_control().unwrap_err().str(), "control sysfs for SFP 0 does not exist");
        module(&root, 0, &[("control", "1")]);
        assert!(s.get_presence());
        assert!(!s.check_eeprom_ready_if_present().unwrap());
        module(&root, 0, &[("hw_present", "0")]);
        assert!(!s.get_presence());
        assert!(s.check_eeprom_ready_if_present().unwrap());
        // Firmware control: `present` and the EEPROM, as in legacy.
        module(&root, 0, &[("control", "0"), ("present", "1")]);
        eeprom(&root, 0);
        assert!(s.get_presence());
    }

    #[test]
    fn the_eeprom_pages_follow_the_module_s_type() {
        let r = Root::new();
        let pages = format!("{M0}/eeprom/pages");
        let write = |page: &str, data: &[u8]| {
            let p = io::real(&format!("{pages}/{page}"));
            std::fs::create_dir_all(p.parent().unwrap()).unwrap();
            std::fs::write(p, data).unwrap();
        };
        let mut page0 = vec![0u8; 256];
        // SFF-8472: A2h follows A0h, then the upper pages.
        page0[0] = 0x03;
        write("0/i2c-0x50/data", &page0);
        write("0/i2c-0x51/data", &[7u8; 256]);
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        let (p, o) = sfp.page_and_offset(256 + 10).unwrap().unwrap();
        assert!(p.ends_with("/0/i2c-0x51/data") && o == 10);
        let (p, o) = sfp.page_and_offset(512 + 130).unwrap().unwrap();
        assert!(p.ends_with("/pages/2/data") && o == 2);
        assert_eq!(sfp.read_eeprom(256, 2, true).unwrap(), Some(vec![7, 7]));
        // SFF-8636 and an unsupported type: upper pages start at 256.
        for id in [0x11u8, 0x99] {
            page0[0] = id;
            write("0/i2c-0x50/data", &page0);
            let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
            assert!(sfp.page_and_offset(256).unwrap().unwrap().0.ends_with("/pages/1/data"));
        }
        // The type cannot be read: neither cached nor logged as I/O error.
        std::fs::remove_file(io::real(&format!("{pages}/0/i2c-0x50/data"))).unwrap();
        std::fs::create_dir_all(io::real(&format!("{pages}/0/i2c-0x50/data"))).unwrap();
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        assert!(sfp.page_and_offset(300).unwrap().unwrap().0.ends_with("/pages/1/data"));
        assert!(sfp.sfp_type_str.lock().unwrap().is_none());
        // A page that cannot be read: None, logged or not.
        assert_eq!(sfp.read_eeprom(0, 1, true).unwrap(), None);
        assert_eq!(sfp.read_eeprom(0, 1, false).unwrap(), None);
        // An empty type byte: Python's `bytearray(b'')[0]`, raised out of
        // the read; a read inside page 0 never asks for the type.
        std::fs::remove_dir(io::real(&format!("{pages}/0/i2c-0x50/data"))).unwrap();
        write("0/i2c-0x50/data", &[]);
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        let e = sfp.page_and_offset(300).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("IndexError", "bytearray index out of range".to_string()));
        assert_eq!(sfp.read_eeprom(300, 1, true).unwrap_err().exc.name(), "IndexError");
        assert!(sfp.page_and_offset(0).unwrap().is_some());
        drop(r);
    }

    #[test]
    fn a_read_past_a_short_page_is_none() {
        let r = Root::new();
        let mut page0 = vec![0u8; 256];
        page0[0] = 0x18;
        std::fs::create_dir_all(io::real(&format!("{M0}/eeprom/pages/0/i2c-0x50"))).unwrap();
        std::fs::write(io::real(&format!("{M0}/eeprom/pages/0/i2c-0x50/data")), &page0).unwrap();
        r.write(&format!("{M0}/eeprom/pages/1/data"), "\u{1}\u{2}");
        let sfp = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        assert_eq!(sfp.read_eeprom(256, 2, true).unwrap(), Some(vec![1, 2]));
        // Page 1 has two bytes, not 128: the read goes on at its end, finds
        // nothing there, and the whole read is None, as in Python.
        assert_eq!(sfp.read_eeprom(256, 5, true).unwrap(), None);
        // Page 2 is not there.
        assert_eq!(sfp.read_eeprom(384, 1, true).unwrap(), None);
        sfp.forget_type();
        assert!(sfp.sfp_type_str.lock().unwrap().is_none());
    }

    #[test]
    fn readings_come_from_state_db_by_the_logical_port() {
        let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
        forget_port_mapping();
        let redis = FakeRedis::get();
        let s = Sfp::new(Kind::Sfp, 0, "asic0").unwrap();
        // No ports configured yet: nothing.
        assert_eq!(s.get_temperature_from_db().unwrap(), 0.0);
        assert_eq!(s.get_warning_threshold_from_db().unwrap(), 0.0);
        assert_eq!(s.get_vendor_name_from_db().unwrap(), "");
        // CONFIG_DB's PORT table, in natural order: the first port of an
        // index is its logical port.
        redis.set(0, "PORT_TABLE:PortConfigDone", "");
        redis.hset(CONFIG_DB, "PORT|Ethernet8", &[("index", "1")]);
        redis.hset(CONFIG_DB, "PORT|Ethernet10", &[("index", "2")]);
        redis.hset(CONFIG_DB, "PORT|Ethernet9", &[("index", "2")]);
        assert_eq!(s.get_temperature_from_db().unwrap(), 0.0);
        redis.hset(STATE_DB, "TRANSCEIVER_DOM_TEMPERATURE|Ethernet8", &[("temperature", "45.5")]);
        redis.hset(STATE_DB, "TRANSCEIVER_DOM_THRESHOLD|Ethernet8", &[("temphighwarning", "N/A"), ("temphighalarm", "75")]);
        redis.hset(STATE_DB, "TRANSCEIVER_INFO|Ethernet8", &[("manufacturer", " Mellanox "), ("model", "MCP1600")]);
        assert_eq!(s.get_temperature_from_db().unwrap(), 45.5);
        assert_eq!(s.get_warning_threshold_from_db().unwrap(), 0.0);
        assert_eq!(s.get_critical_threshold_from_db().unwrap(), 75.0);
        assert_eq!(s.get_vendor_name_from_db().unwrap(), "Mellanox");
        assert_eq!(s.get_part_number_from_db().unwrap(), "MCP1600");
        redis.hset(STATE_DB, "TRANSCEIVER_DOM_TEMPERATURE|Ethernet8", &[("temperature", "None")]);
        assert_eq!(s.get_temperature_from_db().unwrap(), -1.0);
        redis.hset(STATE_DB, "TRANSCEIVER_DOM_TEMPERATURE|Ethernet8", &[("temperature", "hot")]);
        assert_eq!(s.get_temperature_from_db().unwrap_err().exc.name(), "ValueError");
        let p2 = Sfp::new(Kind::Sfp, 1, "asic0").unwrap();
        assert_eq!(p2.get_logical_port().unwrap().as_deref(), Some("Ethernet9"));
        // A port without an index: int(None).
        forget_port_mapping();
        redis.hset(CONFIG_DB, "PORT|Ethernet12", &[("lanes", "1")]);
        let e = s.get_temperature_from_db().unwrap_err();
        assert_eq!(e.exc.name(), "TypeError");
        forget_port_mapping();
        drop(root);
    }

    #[test]
    fn a_port_on_a_multi_asic_system_is_in_its_asic_s_namespace() {
        let r = Root::new();
        r.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        let s = Sfp::new(Kind::Sfp, 3, "asic1").unwrap();
        assert_eq!((s.get_asic_index().unwrap(), s.namespace.as_str()), (1, "asic1"));
        // Its databases are not the default namespace's: Python's KeyError.
        forget_port_mapping();
        assert_eq!(s.get_temperature_from_db().unwrap_err().exc.name(), "KeyError");
    }

    #[test]
    fn a_namespace_other_than_the_default_has_no_port_mapping() {
        let _r = Root::new();
        assert_eq!(build_port_mapping("asic1").unwrap_err().str(), "''");
        assert_eq!(mapped(12345), None);
    }
}
