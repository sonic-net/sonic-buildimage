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


//! `sonic_platform/chassis.py` and `platform.py`.
//!
//! Everything is built on first use, as in Python, and each `open()` builds
//! a fresh chassis, as each `Platform()` does; what Python keeps on the class
//! (the system LEDs, `Chassis.chassis_instance`) is process-wide here.

use std::sync::{Arc, Mutex, OnceLock};
use std::thread::sleep;
use std::time::Duration;

use platform_base::py::{PyErr, PyResult, PyVal};
use platform_base::{device_info, io, BaseClass, PyObject};
use regex::Regex;

use crate::device_data;
use crate::eeprom::Eeprom;
use crate::fan::Fan;
use crate::fan_drawer::FanDrawer;
use crate::led::Led;
use crate::liquid_cooling::LiquidCooling;
use crate::linecard::LinecardModule;
use crate::module::DpuModule;
use crate::module_host_mgmt_initializer::ModuleHostMgmtInitializer;
use crate::pdb::Pdb;
use crate::psu::Psu;
use crate::sfp::{self, Kind, Sfp};
use crate::thermal_manager::ThermalManager;
use crate::utils::{self, Log};
use crate::sfp_events::{self as sfpe, PortDict, Reg, Registered};
use crate::poll::{PollSet, POLLERR_PRI};
use crate::watchdog::{self, Watchdog};

const VPD_DATA_FILE: &str = "/var/run/hw-management/eeprom/vpd_data";
const REVISION: &str = "REV";
const VPD_DATA_WAIT_TIMEOUT: f64 = 60.0;
const REBOOT_CAUSE_ROOT: &str = "/var/run/hw-management/system/";
const REBOOT_CAUSE_MAX_WAIT_TIME: i64 = 45;
const REBOOT_CAUSE_CHECK_INTERVAL: i64 = 5;
const REBOOT_CAUSE_READY_FILE: &str = "/run/hw-management/config/reset_attr_ready";
const REBOOT_TYPE_KEXEC_FILE: &str = "/proc/cmdline";
const SYS_DISPLAY: &str = "SYS_DISPLAY";

const REBOOT_CAUSE_POWER_LOSS: &str = "Power Loss";
const REBOOT_CAUSE_POWER_DOWN_REQUEST_FROM_BMC: &str = "Power down request from BMC";
const REBOOT_CAUSE_THERMAL_OVERLOAD_CPU: &str = "Thermal Overload: CPU";
const REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC: &str = "Thermal Overload: ASIC";
const REBOOT_CAUSE_WATCHDOG: &str = "Watchdog";
const REBOOT_CAUSE_HARDWARE_OTHER: &str = "Hardware - Other";
const REBOOT_CAUSE_HARDWARE_BIOS: &str = "BIOS";
const REBOOT_CAUSE_HARDWARE_BUTTON: &str = "Push button";
const REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC: &str = "Reset from ASIC";
const REBOOT_CAUSE_NON_HARDWARE: &str = "Non-Hardware";

/// `Chassis.chassis_instance`: the chassis built last.
static INSTANCE: Mutex<Option<Arc<Chassis>>> = Mutex::new(None);

/// `Chassis._led` and `Chassis._led_uid`: one pair per process.
static SYSTEM_LED: OnceLock<(Arc<Led>, Arc<Led>)> = OnceLock::new();

/// `Chassis.chassis_instance`, for the thermal manager: without one, the
/// AttributeError names `attr`, the method Python asks of it first.
pub fn instance(attr: &str) -> PyResult<Arc<Chassis>> {
    utils::lock(&INSTANCE).clone().ok_or_else(|| PyErr::missing("NoneType", attr))
}

/// Which `Chassis` class `Platform()` picked.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ChassisKind {
    Fixed,
    Modular,
    SmartSwitch,
}

#[derive(Default)]
struct SfpState {
    /// `_sfp_list`: `[]` until `initialize_sfp()` fills it, a port at a
    /// time -- one whose build raises leaves it short.
    list: Vec<Option<Arc<Sfp>>>,
    initialized_count: i128,
    /// `_asic_modules_dict`: the ports on each ASIC, by `asic<N>`.
    asic_modules: Vec<(String, Vec<Arc<Sfp>>)>,
}

pub struct Chassis {
    kind: ChassisKind,
    num_npus: i128,
    vpd_data: Mutex<Vec<(String, String)>>,
    psu_list: Mutex<Vec<PyVal>>,
    pdb_list: Mutex<Vec<PyVal>>,
    fan_drawer_list: Mutex<Vec<PyVal>>,
    thermal_list: Mutex<Vec<PyVal>>,
    eeprom: Mutex<Option<Arc<Eeprom>>>,
    liquid_cooling: Mutex<Option<Arc<LiquidCooling>>>,
    sfp: Mutex<SfpState>,
    rj45_ports: Mutex<Option<Option<Vec<i128>>>>,
    cpo_ports: Mutex<Option<Option<Vec<i128>>>>,
    asic_id_map: Mutex<Option<Vec<(i128, i128)>>>,
    /// `reboot_major_cause_dict['reset_sw_pwr_off']`: set by
    /// `initialize_reboot_cause()`, once, on the first call that needs it.
    sw_pwr_off_cause: Mutex<Option<&'static str>>,
    module_host_mgmt_initializer: ModuleHostMgmtInitializer,
    /// `_bmc_initialized` and `_bmc`.
    bmc_initialized: Mutex<bool>,
    bmc: Mutex<Option<Arc<crate::bmc::Bmc>>>,
    /// `_component_list`: in a container, only what the BMC adds.
    component_list: Mutex<Vec<PyVal>>,
    /// SmartSwitch: `_module_list`, one slot per DPU once any is built.
    modules: Mutex<Vec<Option<Arc<DpuModule>>>>,
    /// The change events' poll object, registered fds and remembered states.
    events: Mutex<EventState>,
    /// ModularChassis: `_module_list` and `module_initialized_count`.
    linecards: Mutex<(Vec<Option<Arc<LinecardModule>>>, usize)>,
    /// `_watchdog`: built on first use, and tried again while there is none.
    watchdog: Mutex<Option<Arc<Watchdog>>>,
}

/// `Platform()`: the chassis class follows the platform's DPUs and line cards.
pub fn platform() -> PyResult<Arc<Chassis>> {
    crate::imports::platform();
    if device_data::get_dpu_count()? > 0 {
        let chassis = Chassis::new(ChassisKind::SmartSwitch)?;
        chassis.initialize_modules()?;
        return Ok(chassis);
    }
    if device_data::get_linecard_count() == 0 {
        Chassis::new(ChassisKind::Fixed)
    } else {
        Chassis::new(ChassisKind::Modular)
    }
}

impl Chassis {
    /// `Chassis()` / `ModularChassis()`.
    fn new(kind: ChassisKind) -> PyResult<Arc<Chassis>> {
        let num_npus = device_info::get_num_npus()?;
        let module_host_mgmt_initializer = ModuleHostMgmtInitializer::new()?;
        let chassis = Arc::new(Chassis {
            kind,
            num_npus,
            vpd_data: Mutex::new(Vec::new()),
            psu_list: Mutex::new(Vec::new()),
            pdb_list: Mutex::new(Vec::new()),
            fan_drawer_list: Mutex::new(Vec::new()),
            thermal_list: Mutex::new(Vec::new()),
            eeprom: Mutex::new(None),
            liquid_cooling: Mutex::new(None),
            sfp: Mutex::new(SfpState::default()),
            rj45_ports: Mutex::new(None),
            cpo_ports: Mutex::new(None),
            asic_id_map: Mutex::new(None),
            sw_pwr_off_cause: Mutex::new(None),
            module_host_mgmt_initializer,
            modules: Mutex::new(Vec::new()),
            bmc_initialized: Mutex::new(false),
            bmc: Mutex::new(None),
            component_list: Mutex::new(Vec::new()),
            watchdog: Mutex::new(None),
            linecards: Mutex::new((Vec::new(), 0)),
            events: Mutex::new(EventState::default()),
        });
        *utils::lock(&INSTANCE) = Some(chassis.clone());
        utils::watch_shutdown_signals();
        utils::log_info("Chassis loaded successfully");
        Ok(chassis)
    }

    // -- SmartSwitch modules ---------------------------------------------------

    /// `initialize_single_module(index)` for every DPU.
    fn initialize_modules(&self) -> PyResult<()> {
        let count = device_data::get_dpu_count()? as i128;
        for index in 0..count {
            let mut list = utils::lock(&self.modules);
            if list.is_empty() {
                *list = vec![None; usize::try_from(count).unwrap_or(0)];
            }
            let slot = usize::try_from(index).ok().and_then(|i| list.get_mut(i));
            if let Some(slot) = slot {
                if slot.is_none() {
                    *slot = Some(DpuModule::new(index));
                }
            }
        }
        Ok(())
    }

    // -- PSU, PDB, fans, thermals -------------------------------------------

    /// `initialize_psu()`.
    fn initialize_psu(&self) -> Vec<PyVal> {
        let mut list = utils::lock(&self.psu_list);
        if list.is_empty() {
            crate::imports::psu();
            let count = device_data::get_psu_count();
            if count != 0 {
                let hot_swapable = device_data::is_psu_hotswapable();
                for index in 0..count.max(0) {
                    let psu = if hot_swapable { Psu::new(index) } else { Psu::new_fixed(index) };
                    list.push(PyVal::Obj(psu));
                }
            }
        }
        list.clone()
    }

    /// `initialize_pdb()`.
    fn initialize_pdb(&self) -> Vec<PyVal> {
        let mut list = utils::lock(&self.pdb_list);
        if list.is_empty() {
            let count = device_data::get_pdb_count();
            for index in 0..count.max(0) {
                list.push(PyVal::Obj(Pdb::new(index)));
            }
        }
        list.clone()
    }

    /// `initialize_fan()`.
    fn initialize_fan(&self) -> Vec<PyVal> {
        let mut list = utils::lock(&self.fan_drawer_list);
        if list.is_empty() {
            // `from .fan import Fan` and `from .fan_drawer import ...`.
            crate::imports::fan();
            let drawer_num = device_data::get_fan_drawer_count();
            if drawer_num != 0 {
                utils::ensure_sysfs_labels_ready();
                let hot_swapable = device_data::is_fan_hotswapable();
                let fan_num = device_data::get_fan_count();
                let per_drawer = fan_num.checked_div(drawer_num).unwrap_or(0);
                let mut fan_index: i128 = 0;
                for drawer_index in 0..drawer_num {
                    let drawer = FanDrawer::new(drawer_index as i128, !hot_swapable);
                    list.push(PyVal::Obj(drawer.clone()));
                    for index in 0..per_drawer {
                        let fan = Fan::new(fan_index, drawer.clone(), index as i128 + 1);
                        fan_index += 1;
                        drawer.add_fan(PyVal::Obj(fan));
                    }
                }
            }
        }
        list.clone()
    }

    /// `initialize_thermals()`.
    fn initialize_thermals(&self) -> PyResult<Vec<PyVal>> {
        let mut list = utils::lock(&self.thermal_list);
        if list.is_empty() {
            utils::ensure_sysfs_labels_ready();
            crate::imports::thermal();
            *list = crate::thermal::initialize_chassis_thermals()?;
        }
        Ok(list.clone())
    }

    /// `initialize_eeprom()`.
    fn initialize_eeprom(&self) -> PyResult<Arc<Eeprom>> {
        let mut slot = utils::lock(&self.eeprom);
        if let Some(e) = slot.as_ref() {
            return Ok(e.clone());
        }
        let e = Arc::new(Eeprom::new()?);
        *slot = Some(e.clone());
        Ok(e)
    }

    /// `initialize_liquid_cooling()`.
    fn initialize_liquid_cooling(&self) -> PyResult<Arc<LiquidCooling>> {
        let mut slot = utils::lock(&self.liquid_cooling);
        if let Some(l) = slot.as_ref() {
            return Ok(l.clone());
        }
        let l = LiquidCooling::new()?;
        *slot = Some(l.clone());
        Ok(l)
    }

    // -- SFPs ----------------------------------------------------------------

    /// The `RJ45_port_list` and `cpo_port_list` properties: read once.
    fn port_list(&self, slot: &Mutex<Option<Option<Vec<i128>>>>, rj45: bool) -> PyResult<Option<Vec<i128>>> {
        let mut s = utils::lock(slot);
        if let Some(l) = s.as_ref() {
            return Ok(l.clone());
        }
        let l = if rj45 {
            utils::extract_rj45_ports_index(self.num_npus)?
        } else {
            utils::extract_cpo_ports_index(self.num_npus)?
        };
        *s = Some(l.clone());
        Ok(l)
    }

    fn rj45_port_list(&self) -> PyResult<Option<Vec<i128>>> {
        self.port_list(&self.rj45_ports, true)
    }

    fn cpo_port_list(&self) -> PyResult<Option<Vec<i128>>> {
        self.port_list(&self.cpo_ports, false)
    }

    /// `get_num_sfps()`.  ModularChassis's is the sum of its line cards',
    /// through `reduce()` -- which has nothing to start from without them.
    pub fn get_num_sfps(&self) -> PyResult<i128> {
        if self.kind == ChassisKind::Modular {
            let cards = self.initialize_linecards()?;
            if cards.is_empty() {
                return Err(PyErr::type_error("reduce() of empty iterable with no initial value"));
            }
            let mut n = 0;
            for card in cards {
                let Some(card) = card else {
                    return Err(PyErr::missing("NoneType", "get_num_sfps"));
                };
                n += card.get_num_sfps()?;
            }
            return Ok(n);
        }
        let rj45 = self.rj45_port_list()?;
        let cpo = self.cpo_port_list()?;
        let mut n = device_data::get_sfp_count()?;
        n += rj45.map_or(0, |l| l.len() as i128);
        n += cpo.map_or(0, |l| l.len() as i128);
        Ok(n)
    }

    /// `_get_asic_id_by_sfp_index(sfp_index)`.
    fn asic_id_by_sfp_index(&self, index: i128) -> PyResult<String> {
        if !device_data::is_multi_asic_platform()? {
            return Ok("asic0".to_string());
        }
        let mut map = utils::lock(&self.asic_id_map);
        if map.as_ref().is_none_or(|m| m.is_empty()) {
            *map = Some(utils::extract_asic_id_map(device_data::get_asic_count()?)?);
        }
        Ok(match map.as_ref().and_then(|m| m.iter().find(|(i, _)| *i == index)) {
            Some((_, asic)) => format!("asic{asic}"),
            None => "asicNone".to_string(),
        })
    }

    /// The class `initialize_sfp` picks for a port.
    fn sfp_kind(&self, index: i128) -> PyResult<Kind> {
        if self.rj45_port_list()?.is_some_and(|l| l.contains(&index)) {
            return Ok(Kind::Rj45);
        }
        if self.cpo_port_list()?.is_some_and(|l| l.contains(&index)) {
            return Ok(Kind::Cpo);
        }
        Ok(Kind::Sfp)
    }

    /// `initialize_sfp()`: every port object, built once.
    pub fn initialize_sfp(&self) -> PyResult<()> {
        let sfp_count = self.get_num_sfps()?;
        let mut st = utils::lock(&self.sfp);
        if sfp_count == st.initialized_count {
            return Ok(());
        }
        if st.list.is_empty() {
            sfp::import()?;
            for index in 0..sfp_count.max(0) {
                let asic_id = self.asic_id_by_sfp_index(index)?;
                let obj = Sfp::new(self.sfp_kind(index)?, index, &asic_id)?;
                match st.asic_modules.iter_mut().find(|(a, _)| *a == asic_id) {
                    Some((_, v)) => v.push(obj.clone()),
                    None => st.asic_modules.push((asic_id, vec![obj.clone()])),
                }
                st.list.push(Some(obj));
            }
            st.initialized_count = sfp_count;
        } else if st.initialized_count != st.list.len() as i128 {
            sfp::import()?;
            for index in 0..st.list.len() {
                if st.list[index].is_none() {
                    let i = index as i128;
                    let asic_id = self.asic_id_by_sfp_index(i)?;
                    st.list[index] = Some(Sfp::new(self.sfp_kind(i)?, i, &asic_id)?);
                }
            }
            st.initialized_count = st.list.len() as i128;
        }
        Ok(())
    }

    fn sfp_list(&self) -> Vec<Arc<Sfp>> {
        utils::lock(&self.sfp).list.iter().flatten().cloned().collect()
    }

    /// `wait_sfp_eeprom_ready()`.
    fn wait_sfp_eeprom_ready(&self) -> PyResult<bool> {
        if device_data::is_simx_platform()? {
            return Ok(true);
        }
        let rj45 = self.rj45_port_list()?;
        let cpo = self.cpo_port_list()?;
        let mut checks: Vec<Box<dyn FnMut() -> PyResult<bool>>> = Vec::new();
        for s in self.sfp_list() {
            let idx = s.sdk_index;
            if rj45.as_ref().is_some_and(|l| l.contains(&idx)) || cpo.as_ref().is_some_and(|l| l.contains(&idx)) {
                continue;
            }
            checks.push(Box::new(move || s.check_eeprom_ready_if_present()));
        }
        utils::try_wait_until_conditions(checks, 10.0, 1.0)
    }

    /// `wait_sfp_ready_for_use()`.
    fn wait_sfp_ready_for_use(&self) -> PyResult<bool> {
        let ready_file = if utils::is_host() { "/tmp/nv-syncd-shared/sfp_ready" } else { "/tmp/sfp_ready" };
        if io::exists(ready_file) {
            return Ok(true);
        }
        if !device_data::wait_sysfs_ready(self.get_num_sfps()?)? {
            if utils::shutdown_requested() {
                utils::log_notice("SFP readiness wait aborted: daemon is shutting down");
            } else {
                utils::log_error("SFPs are not ready for usage");
            }
            return Ok(false);
        }
        if !self.wait_sfp_eeprom_ready()? {
            if utils::shutdown_requested() {
                utils::log_notice("SFP EEPROM readiness wait aborted: daemon is shutting down");
            } else {
                utils::log_error("SFPs are not ready for usage due to eeprom not ready");
            }
            return Ok(false);
        }
        std::fs::OpenOptions::new()
            .create(true)
            .append(true)
            .open(io::real(ready_file))
            .map_err(|e| PyErr::from_io(&e, Some(ready_file)))?;
        utils::log_notice("SFPs are ready for usage");
        Ok(true)
    }

    /// `get_all_sfps()`.  ModularChassis's is every line card's, joined by
    /// `reduce()` -- which has nothing to start from without line cards.
    pub fn get_all_sfps(&self) -> PyResult<Vec<Arc<Sfp>>> {
        if self.kind == ChassisKind::Modular {
            let cards = self.initialize_linecards()?;
            if cards.is_empty() {
                return Err(PyErr::type_error("reduce() of empty iterable with no initial value"));
            }
            let mut all = Vec::new();
            for card in cards {
                let Some(card) = card else {
                    return Err(PyErr::missing("NoneType", "get_all_sfps"));
                };
                all.extend(card.get_all_sfps()?.into_iter().flatten());
            }
            return Ok(all);
        }
        self.sfp_wait_ready_and_initialize()?;
        Ok(self.sfp_list())
    }

    // -- model, revision, reboot cause ---------------------------------------

    /// `_parse_vpd_data(filename)`.
    fn parse_vpd_data(&self, filename: &str) -> Vec<(String, String)> {
        if !utils::readable(filename) {
            // Inside Python's `try`, which logs whatever it catches.
            match device_data::is_simx_platform() {
                Ok(true) => return Vec::new(),
                Ok(false) => {}
                Err(e) => {
                    utils::log_error(&format!("Fail to decode vpd_data {filename} due to {}", e.repr()));
                    return Vec::new();
                }
            }
            if !utils::wait_for_file_creation(filename, VPD_DATA_WAIT_TIMEOUT) {
                utils::log_error(&format!("VPD data file {filename} not available after timeout"));
                return Vec::new();
            }
        }
        utils::read_key_value(filename, Log::Error, ": ")
    }

    fn vpd_get(&self, key: &str) -> String {
        let mut vpd = utils::lock(&self.vpd_data);
        if vpd.is_empty() {
            *vpd = self.parse_vpd_data(VPD_DATA_FILE);
        }
        vpd.iter().find(|(k, _)| k == key).map_or_else(|| "N/A".to_string(), |(_, v)| v.clone())
    }

    /// `_read_model_from_vpd()`: SKUs from Spectrum-4 on keep their model in
    /// the VPD file.  Python takes the first four digits of the SKU and
    /// raises on a SKU without any.
    fn read_model_from_vpd(&self) -> PyResult<bool> {
        model_in_vpd(device_info::get_hwsku().as_deref())
    }

    /// `_parse_warmfast_reboot_from_proc_cmdline()`.
    fn parse_warmfast_reboot_from_proc_cmdline(&self) -> PyResult<Option<&'static str>> {
        if !io::is_file(REBOOT_TYPE_KEXEC_FILE) {
            return Ok(None);
        }
        let text = io::read_text(REBOOT_TYPE_KEXEC_FILE)?;
        let line = text.split_inclusive('\n').next().unwrap_or("");
        for (pattern, cause) in [
            (r".*SONIC_BOOT_TYPE=(warm|fastfast).*", "warm-reboot"),
            (r".*SONIC_BOOT_TYPE=(fast|fast-reboot).*", "fast-reboot"),
        ] {
            let re = Regex::new(pattern).map_err(|e| PyErr::runtime_error(e.to_string()))?;
            if re.captures(line).and_then(|c| c.get(1)).is_some_and(|g| !g.as_str().is_empty()) {
                return Ok(Some(cause));
            }
        }
        Ok(None)
    }

    /// `_wait_reboot_cause_ready()`: up to 45 seconds.
    fn wait_reboot_cause_ready(&self) -> bool {
        let mut max_wait_time = REBOOT_CAUSE_MAX_WAIT_TIME;
        while max_wait_time > 0 {
            if utils::read_int(REBOOT_CAUSE_READY_FILE, 0, Log::Off) == 1 {
                return true;
            }
            sleep(Duration::from_secs(REBOOT_CAUSE_CHECK_INTERVAL as u64));
            max_wait_time -= REBOOT_CAUSE_CHECK_INTERVAL;
        }
        false
    }

    /// `get_model()` past `_read_model_from_vpd()`.
    fn model(&self, from_vpd: bool) -> PyResult<PyVal> {
        if from_vpd {
            let model = self.vpd_get(SYS_DISPLAY);
            if model == "N/A" && device_data::is_simx_platform()? {
                utils::log_notice("VPD SYS_DISPLAY unavailable on simx; falling back to EEPROM part number");
                return Ok(self.initialize_eeprom()?.get_part_number());
            }
            return Ok(PyVal::Str(model));
        }
        Ok(self.initialize_eeprom()?.get_part_number())
    }

    fn verify_reboot_cause(&self, filename: &str) -> bool {
        utils::read_int(&io::join(REBOOT_CAUSE_ROOT, filename), 0, Log::Off) != 0
    }
}

/// `_read_model_from_vpd()` for a given SKU.
fn model_in_vpd(sku: Option<&str>) -> PyResult<bool> {
    let Some(sku) = sku else {
        return Err(PyErr::type_error("expected string or bytes-like object, got 'NoneType'"));
    };
    let re = Regex::new("[0-9]{4}").map_err(|e| PyErr::runtime_error(e.to_string()))?;
    let Some(m) = re.find(sku) else {
        return Err(PyErr::missing("NoneType", "group"));
    };
    Ok(platform_base::py::py_int(m.as_str())? >= 5000)
}

fn reboot_cause(cause: &str, minor: &str) -> PyVal {
    PyVal::Tuple(vec![PyVal::str(cause), PyVal::str(minor)])
}

fn system_leds() -> &'static (Arc<Led>, Arc<Led>) {
    SYSTEM_LED.get_or_init(|| (Led::system(), Led::system_uid()))
}

impl PyObject for Chassis {
    fn py_class(&self) -> &'static str {
        match self.kind {
            ChassisKind::Fixed => "Chassis",
            ChassisKind::Modular => "ModularChassis",
            ChassisKind::SmartSwitch => "SmartSwitchChassis",
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ChassisBase
    }

    fn get_all_psus(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.initialize_psu()))
    }
    fn get_all_pdbs(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.initialize_pdb()))
    }
    fn get_all_fan_drawers(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.initialize_fan()))
    }
    fn get_all_thermals(&self) -> PyResult<PyVal> {
        Ok(PyVal::List(self.initialize_thermals()?))
    }
    fn get_eeprom(&self) -> PyResult<PyVal> {
        Ok(PyVal::Obj(self.initialize_eeprom()?))
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(self.initialize_eeprom()?.get_product_name())
    }
    fn get_model(&self) -> PyResult<PyVal> {
        let from_vpd = self.read_model_from_vpd()?;
        self.model(from_vpd)
    }
    fn get_base_mac(&self) -> PyResult<PyVal> {
        Ok(self.initialize_eeprom()?.get_base_mac())
    }
    fn get_serial(&self) -> PyResult<PyVal> {
        Ok(self.initialize_eeprom()?.get_serial_number())
    }
    fn get_system_eeprom_info(&self) -> PyResult<PyVal> {
        Ok(self.initialize_eeprom()?.get_system_eeprom_info())
    }
    fn get_revision(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.vpd_get(REVISION)))
    }

    fn initizalize_system_led(&self) -> PyResult<PyVal> {
        system_leds();
        Ok(PyVal::None)
    }
    fn set_status_led(&self, color: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(system_leds().0.set_status(color)?))
    }
    fn get_status_led(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(system_leds().0.get_status()?))
    }

    fn get_reboot_cause(&self) -> PyResult<PyVal> {
        if utils::is_host() && self.parse_warmfast_reboot_from_proc_cmdline()?.is_some() {
            return Ok(reboot_cause(REBOOT_CAUSE_NON_HARDWARE, ""));
        }
        if !self.wait_reboot_cause_ready() {
            utils::log_error("Hardware reboot cause is not ready");
            return Ok(reboot_cause(REBOOT_CAUSE_NON_HARDWARE, ""));
        }
        let mut cached = utils::lock(&self.sw_pwr_off_cause);
        let sw_pwr_off = match *cached {
            Some(c) => c,
            None => *cached.insert(if device_data::is_platform_with_bmc()? {
                REBOOT_CAUSE_POWER_DOWN_REQUEST_FROM_BMC
            } else {
                REBOOT_CAUSE_POWER_LOSS
            }),
        };
        drop(cached);
        let major: [(&str, &str); 25] = [
            ("reset_main_pwr_fail", REBOOT_CAUSE_POWER_LOSS),
            ("reset_ac_pwr_fail", REBOOT_CAUSE_POWER_LOSS),
            ("reset_aux_pwr_or_ref", REBOOT_CAUSE_POWER_LOSS),
            ("reset_aux_pwr_or_reload", REBOOT_CAUSE_POWER_LOSS),
            ("reset_aux_pwr_or_fu", REBOOT_CAUSE_POWER_LOSS),
            ("reset_comex_pwr_fail", REBOOT_CAUSE_POWER_LOSS),
            ("reset_main_51v", REBOOT_CAUSE_POWER_LOSS),
            ("reset_mgmt_pwr_fail", REBOOT_CAUSE_POWER_LOSS),
            ("reset_sw_pwr_off", sw_pwr_off),
            ("reset_asic_thermal", REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC),
            ("reset_cpu_thermal", REBOOT_CAUSE_THERMAL_OVERLOAD_CPU),
            ("reset_comex_thermal", REBOOT_CAUSE_THERMAL_OVERLOAD_CPU),
            ("reset_hotswap_or_wd", REBOOT_CAUSE_WATCHDOG),
            ("reset_comex_wd", REBOOT_CAUSE_WATCHDOG),
            ("reset_swb_wd", REBOOT_CAUSE_WATCHDOG),
            ("reset_sff_wd", REBOOT_CAUSE_WATCHDOG),
            ("reset_hotswap_or_halt", REBOOT_CAUSE_HARDWARE_OTHER),
            ("reset_voltmon_upgrade_fail", REBOOT_CAUSE_HARDWARE_OTHER),
            ("reset_pwr_converter_fail", REBOOT_CAUSE_HARDWARE_OTHER),
            ("reset_swb_dc_dc_pwr_fail", REBOOT_CAUSE_HARDWARE_OTHER),
            ("reset_reload_bios", REBOOT_CAUSE_HARDWARE_BIOS),
            ("reset_fw_reset", REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC),
            ("reset_from_asic", REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC),
            ("reset_long_pb", REBOOT_CAUSE_HARDWARE_BUTTON),
            ("reset_short_pb", REBOOT_CAUSE_HARDWARE_BUTTON),
        ];
        for (reset_file, cause) in major {
            if self.verify_reboot_cause(reset_file) {
                utils::log_info(&format!("Hardware reboot cause: {reset_file}"));
                return Ok(reboot_cause(cause, ""));
            }
        }
        if self.verify_reboot_cause("reset_sw_reset") {
            utils::log_info("Hardware reboot cause: the system was rebooted due to software requesting");
        } else {
            utils::log_info("Hardware reboot cause: no hardware reboot cause found");
        }
        Ok(reboot_cause(REBOOT_CAUSE_NON_HARDWARE, ""))
    }

    fn get_thermal_manager(&self) -> PyResult<PyVal> {
        ThermalManager::import()
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(-1))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(false))
    }
    fn get_liquid_cooling(&self) -> PyResult<PyVal> {
        Ok(PyVal::Obj(self.initialize_liquid_cooling()?))
    }
    fn is_modular_chassis(&self) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::Modular => Ok(PyVal::Bool(true)),
            _ => platform_base::defaults::is_modular_chassis(self),
        }
    }

    /// `get_all_modules()`: the DPUs on a SmartSwitch, the line cards on a
    /// modular chassis, the base class's on a fixed one.
    fn get_all_modules(&self) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::Fixed => platform_base::defaults::get_all_modules(self),
            ChassisKind::SmartSwitch => {
                self.initialize_modules()?;
                let list = utils::lock(&self.modules);
                Ok(PyVal::List(
                    list.iter()
                        .map(|m| m.as_ref().map_or(PyVal::None, |m| PyVal::Obj(m.clone())))
                        .collect(),
                ))
            }
            ChassisKind::Modular => Ok(PyVal::List(
                self.initialize_linecards()?
                    .iter()
                    .map(|m| m.as_ref().map_or(PyVal::None, |m| PyVal::Obj(m.clone())))
                    .collect(),
            )),
        }
    }
    /// `get_module_index(module_name)`: the number off the name on a modular
    /// chassis, -1 for one it cannot read; the DPU of that name on a
    /// SmartSwitch, KeyError for none; the base class's on a fixed one.
    fn get_module_index(&self, module_name: &str) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::Fixed => platform_base::defaults::get_module_index(self, module_name),
            // `int(module_name[len('LINE-CARD')-1:])` under `@default_return(-1)`.
            ChassisKind::Modular => {
                let tail: String = module_name.chars().skip("LINE-CARD".len() - 1).collect();
                let index = platform_base::py::py_int(&tail);
                Ok(PyVal::Int(utils::default_return(-1, index, "get_module_index")))
            }
            // `self.module_name_index_map[module_name.upper()]`, the map
            // `initialize_single_module()` fills from each DPU's `get_name()`.
            ChassisKind::SmartSwitch => {
                let want = PyVal::Str(module_name.to_uppercase());
                let modules = utils::lock(&self.modules).clone();
                for (index, module) in modules.iter().enumerate() {
                    if let Some(module) = module {
                        if module.get_name()?.py_eq(&want) {
                            return Ok(PyVal::Int(index as i128));
                        }
                    }
                }
                Err(PyErr::key_error(want))
            }
        }
    }
    fn is_smartswitch(&self) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::SmartSwitch => Ok(PyVal::Bool(true)),
            _ => platform_base::defaults::is_smartswitch(self),
        }
    }
    /// SmartSwitchChassis narrows it to `get_dpu_id(self, name)`, so the
    /// zero-argument call raises.
    fn get_dpu_id(&self) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::SmartSwitch => Err(PyErr::type_error(
                "SmartSwitchChassis.get_dpu_id() missing 1 required positional argument: 'name'",
            )),
            _ => platform_base::defaults::get_dpu_id(self),
        }
    }
    fn init_midplane_switch(&self) -> PyResult<PyVal> {
        match self.kind {
            ChassisKind::SmartSwitch => Ok(PyVal::Bool(true)),
            _ => platform_base::defaults::init_midplane_switch(self),
        }
    }
    /// `get_all_components()`: in a container only what `initialize_bmc()`
    /// added -- the BMC's component, once the BMC has been asked for.
    fn get_all_components(&self) -> PyResult<PyVal> {
        self.initialize_components()?;
        Ok(PyVal::List(utils::lock(&self.component_list).clone()))
    }
    /// Built when first asked for, not with the chassis: only one process
    /// can hold the device.  A failure is logged at INFO -- below the
    /// logger's threshold -- and answers None.
    fn get_watchdog(&self) -> PyResult<PyVal> {
        let mut w = utils::lock(&self.watchdog);
        if w.is_none() {
            match watchdog::get_watchdog() {
                Ok(found) => *w = found.map(Arc::new),
                Err(e) => utils::log_info(&format!("Fail to load watchdog due to {}", e.repr())),
            }
        }
        Ok(w.as_ref().map_or(PyVal::None, |w| PyVal::Obj(w.clone())))
    }
    /// `get_bmc()`: `_initialize_bmc()`, then whatever it found.
    fn get_bmc(&self) -> PyResult<PyVal> {
        self.initialize_components()?;
        self.initialize_bmc()?;
        let b = utils::lock(&self.bmc);
        Ok(b.as_ref().map_or(PyVal::None, |b| PyVal::Obj(b.clone())))
    }
    fn get_sed_mgmt(&self) -> PyResult<PyVal> {
        Ok(PyVal::Obj(crate::sed_mgmt::SedMgmt::get_instance()))
    }
    fn get_change_event(&self, timeout: i128) -> PyResult<PyVal> {
        self.change_event(timeout)
    }
}

impl Chassis {
    /// `initialize_components()`: in a container, nothing; on the host, the
    /// firmware components -- built into the list one by one, so that a
    /// failure partway leaves the ones before it, and the list is never
    /// rebuilt -- and then the BMC's, on a BMC platform.
    fn initialize_components(&self) -> PyResult<()> {
        if !utils::is_host() {
            return Ok(());
        }
        {
            let mut list = utils::lock(&self.component_list);
            if list.is_empty() {
                crate::component::host_component_list_into(&mut list)?;
            }
        }
        if device_data::is_platform_with_bmc()? {
            self.initialize_bmc()?;
        }
        Ok(())
    }

    /// `initialize_bmc()`: once per chassis, and only once it got that far
    /// -- an exception leaves it to be tried again.
    fn initialize_bmc(&self) -> PyResult<()> {
        let mut done = utils::lock(&self.bmc_initialized);
        if *done {
            return Ok(());
        }
        crate::imports::bmc();
        let bmc = crate::bmc::get_instance()?;
        *utils::lock(&self.bmc) = bmc.clone();
        if bmc.is_some() {
            match crate::component::bmc_component_list() {
                Ok(list) => utils::lock(&self.component_list).extend(list),
                Err(e) if platform_base::redfish_client::is_exception(&e) => {
                    utils::log_error("Fail to get BMC component list")
                }
                Err(e) => return Err(e),
            }
        }
        *done = true;
        Ok(())
    }
}

impl Chassis {
    /// ModularChassis `initialize_modules()`: one Module per line card slot,
    /// numbered from 1.
    fn initialize_linecards(&self) -> PyResult<Vec<Option<Arc<LinecardModule>>>> {
        let mut g = utils::lock(&self.linecards);
        let (list, initialized) = &mut *g;
        if list.is_empty() {
            let count = device_data::get_linecard_count();
            for index in 1..=count.max(0) {
                list.push(Some(Arc::new(LinecardModule::new(index))));
            }
            *initialized = usize::try_from(count).unwrap_or(0);
        } else if *initialized != list.len() {
            for (i, slot) in list.iter_mut().enumerate() {
                if slot.is_none() {
                    *slot = Some(Arc::new(LinecardModule::new(i as i128 + 1)));
                }
            }
            *initialized = list.len();
        }
        Ok(list.clone())
    }
}

/// `poll_obj`, `registered_fds`, `sfp_states_before_first_poll` and
/// `_last_asic_ready_states`.
#[derive(Default)]
struct EventState {
    poll: Option<PollSet>,
    registered: Registered,
    before_first_poll: Vec<(i128, &'static str)>,
    /// `{asicN_ready: content or None}`, empty until first captured.
    last_asic_ready: Vec<(String, Option<String>)>,
}

/// What a mode's poll loop answers: the dict, or -- when the ports were
/// not ready to poll -- the `(True, {'sfp': {}})` tuple it returns by
/// mistake, which `get_change_event` then treats as the dict.
enum ModeAnswer {
    Dict(Vec<(PyVal, PortDict)>),
    Tuple,
}

const ASIC_READY_DIR: &str = "/var/run/hw-management/config";
const MALFUNCTION_SYSFS_READ_ERROR_DELAY_SECS: f64 = 1.0;
const MALFUNCTION_SYSFS_READ_ERROR_MIN_SLEEP_SECS: f64 = 0.5;

fn none_attr(attr: &str) -> PyErr {
    PyErr::missing("NoneType", attr)
}

impl Chassis {
    /// `self._sfp_list[index]`, as Python indexes it.
    fn sfp_at(&self, index: i128) -> PyResult<Option<Arc<Sfp>>> {
        let st = utils::lock(&self.sfp);
        let n = st.list.len() as i128;
        let i = if index < 0 { index + n } else { index };
        if i < 0 || i >= n {
            return Err(PyErr::index_error("list index out of range"));
        }
        Ok(st.list[i as usize].clone())
    }

    /// `_asic_modules_dict.get(asic_id, ())`.  Python's is a set of SFP
    /// objects, iterated in the order of their addresses' hashes; here it
    /// is the order the ports were built in.
    pub(crate) fn asic_modules(&self, asic_id: &str) -> Vec<Arc<Sfp>> {
        utils::lock(&self.sfp).asic_modules.iter().find(|(a, _)| a == asic_id).map(|(_, v)| v.clone()).unwrap_or_default()
    }

    /// `sfp_wait_ready_and_initialize()`.
    fn sfp_wait_ready_and_initialize(&self) -> PyResult<()> {
        if device_data::is_module_host_management_mode()? {
            self.module_host_mgmt_initializer.initialize(self)
        } else {
            self.initialize_sfp()?;
            self.wait_sfp_ready_for_use()?;
            Ok(())
        }
    }

    /// `get_change_event(timeout)`.
    fn change_event(&self, timeout: i128) -> PyResult<PyVal> {
        self.sfp_wait_ready_and_initialize()?;
        let mut ev = utils::lock(&self.events);
        let asic_events = self.get_asic_change_event(&mut ev, 500.0)?;
        let answer = if device_data::is_module_host_management_mode()? {
            self.change_event_host_mgmt(&mut ev, timeout)?
        } else {
            self.change_event_legacy(&mut ev, timeout)?
        };
        let empty_sfp = || PyVal::Tuple(vec![PyVal::Bool(true), PyVal::Dict(vec![(PyVal::str("sfp"), PyVal::Dict(vec![]))])]);
        let dict = match answer {
            ModeAnswer::Tuple => {
                if !asic_events.is_empty() {
                    return Err(PyErr::missing("tuple", "setdefault"));
                }
                return Ok(PyVal::Tuple(vec![PyVal::Bool(true), empty_sfp()]));
            }
            ModeAnswer::Dict(mut d) => {
                if !asic_events.is_empty() {
                    match d.iter_mut().find(|(k, _)| k.py_eq(&PyVal::str("sfp"))) {
                        Some((_, sfp)) => {
                            for (k, v) in asic_events.0 {
                                sfp.set_val(k, v);
                            }
                        }
                        None => d.push((PyVal::str("sfp"), asic_events)),
                    }
                }
                PyVal::Dict(d.into_iter().map(|(k, v)| (k, v.to_pyval())).collect())
            }
        };
        Ok(PyVal::Tuple(vec![PyVal::Bool(true), dict]))
    }

    /// `capture_current_asic_states(dir, filenames)`: a missing file is
    /// None; any other failure raises.
    fn capture_asic_states(names: &[String]) -> PyResult<Vec<(String, Option<String>)>> {
        let mut out = Vec::new();
        for name in names {
            let path = platform_base::io::join(ASIC_READY_DIR, name);
            let v = match platform_base::io::read_text(&path) {
                Ok(t) => Some(platform_base::py::strip(&t).to_string()),
                Err(e) if e.exc.name() == "FileNotFoundError" => None,
                Err(e) => return Err(e),
            };
            out.push((name.clone(), v));
        }
        Ok(out)
    }

    /// `get_asic_change_event(timeout=500)`: every port of an ASIC whose
    /// ready file changed is reported removed or inserted, its pending
    /// reset waits and its polling dropped or restored.  A path seen both by
    /// inotify and by the before/after comparison is handled twice, as in
    /// Python.
    fn get_asic_change_event(&self, ev: &mut EventState, timeout_ms: f64) -> PyResult<PortDict> {
        let mut changes = PortDict::default();
        let asic_count = device_data::get_asic_count()?;
        // Python's is a set of names, iterated in string-hash order.
        let names: Vec<String> = (1..=asic_count.max(0)).map(|i| format!("asic{i}_ready")).collect();
        let mut watcher = crate::inotify_helper::InotifyEventHelper::new(ASIC_READY_DIR, &names)?;
        let mut changed_paths = watcher.wait_for_events(timeout_ms)?;
        let after = Self::capture_asic_states(&names)?;
        if !ev.last_asic_ready.is_empty() {
            for name in &names {
                let before = ev.last_asic_ready.iter().find(|(n, _)| n == name).map(|(_, v)| v.clone());
                let now = after.iter().find(|(n, _)| n == name).map(|(_, v)| v.clone());
                let Some(before) = before else {
                    return Err(PyErr::key_error(PyVal::str(name.as_str())));
                };
                if before != now.unwrap_or(None) {
                    changed_paths.push(platform_base::io::join(ASIC_READY_DIR, name));
                }
            }
        }
        ev.last_asic_ready = after;
        let wait_ready_task = if !changed_paths.is_empty() && device_data::is_module_host_management_mode()? {
            crate::imports::sfp();
            Some(crate::wait_sfp_ready_task::get())
        } else {
            None
        };
        for path in changed_paths {
            let name = path.rsplit('/').next().unwrap_or("").to_string();
            let num = name
                .split_once("asic")
                .map(|(_, r)| r.split('_').next().unwrap_or(""))
                .ok_or_else(|| PyErr::index_error("list index out of range"))?;
            let asic_index = platform_base::py::py_int(num)? - 1;
            let asic_id = format!("asic{asic_index}");
            let sfp_set = self.asic_modules(&asic_id);
            let ready = platform_base::io::exists(&path) && utils::read_int(&path, 0, Log::Error) != 0;
            let value = if ready {
                self.enable_polling_for_asic(ev, &sfp_set)?;
                "1"
            } else {
                if let Some(t) = &wait_ready_task {
                    for s in &sfp_set {
                        t.cancel_wait(s.sdk_index as u64);
                    }
                }
                self.disable_polling_for_asic(ev, &sfp_set);
                "0"
            };
            self.module_host_mgmt_initializer.set_asic_ready_value(asic_index, ready)?;
            for s in &sfp_set {
                changes.set_val(PyVal::Str((s.sdk_index + 1).to_string()), PyVal::str(value));
            }
        }
        Ok(changes)
    }

    /// `_disable_polling_for_asic(asic_id)`.
    fn disable_polling_for_asic(&self, ev: &mut EventState, sfp_set: &[Arc<Sfp>]) {
        let Some(poll) = ev.poll.as_mut() else {
            return;
        };
        if ev.registered.is_empty() || sfp_set.is_empty() {
            return;
        }
        let indices: Vec<i128> = sfp_set.iter().map(|s| s.sdk_index).collect();
        let doomed: Vec<i32> = ev.registered.iter().filter(|(_, r)| indices.contains(&r.sdk_index)).map(|(f, _)| *f).collect();
        for fileno in doomed {
            let _ = poll.unregister(fileno);
            ev.registered.remove(fileno);
        }
    }

    /// `_enable_polling_for_asic(asic_id)`: each port as its mode polls it;
    /// a failure is a warning.
    fn enable_polling_for_asic(&self, ev: &mut EventState, sfp_set: &[Arc<Sfp>]) -> PyResult<()> {
        if ev.poll.is_none() {
            return Ok(());
        }
        let host_mgmt = device_data::is_module_host_management_mode()?;
        for s in sfp_set {
            let r = (|| -> PyResult<()> {
                let poll = ev.poll.as_mut().ok_or_else(|| PyErr::runtime_error("no poll object"))?;
                if host_mgmt {
                    s.refresh_poll_obj(poll, &mut ev.registered)
                } else {
                    let Some(fd) = s.get_fd("present")? else {
                        return Ok(());
                    };
                    let fileno = fd.fileno();
                    poll.register(fileno, POLLERR_PRI);
                    ev.registered.insert(fileno, Reg { sdk_index: s.sdk_index, fd, fd_type: "" });
                    let status = s.get_module_status()?;
                    match ev.before_first_poll.iter_mut().find(|(i, _)| *i == s.sdk_index) {
                        Some(e) => e.1 = status,
                        None => ev.before_first_poll.push((s.sdk_index, status)),
                    }
                    Ok(())
                }
            })();
            if let Err(e) = r {
                utils::log_warning(&format!("Failed to enable polling for SFP {}: {}", s.sdk_index, e.str()));
            }
        }
        Ok(())
    }

    /// The poll timeout both modes use: at most a second, so a shutdown is
    /// noticed -- and 0, "wait forever", polls without blocking at all.
    fn poll_timeout(timeout: i128) -> (bool, f64) {
        (timeout == 0, if timeout >= 1000 { 1000.0 } else { timeout as f64 })
    }

    /// The end of a poll round with nothing to report: back off after read
    /// errors, give up once the (capped) timeout has passed.
    fn round_ends(has_read_error: bool, iteration_begin: std::time::Instant, begin: std::time::Instant, wait_forever: bool, timeout: f64) -> bool {
        let now = std::time::Instant::now();
        if has_read_error {
            let sleep_time = MALFUNCTION_SYSFS_READ_ERROR_DELAY_SECS - now.duration_since(iteration_begin).as_secs_f64();
            if sleep_time > MALFUNCTION_SYSFS_READ_ERROR_MIN_SLEEP_SECS {
                std::thread::sleep(Duration::from_secs_f64(sleep_time));
            }
        }
        !wait_forever && now.duration_since(begin).as_secs_f64() * 1000.0 >= timeout
    }

    /// `get_change_event_legacy(timeout)`: each port's `present` polled;
    /// the first event after registering is dropped if the status did not
    /// change; the status read from `status`, an error refined by
    /// `statuserror`.
    fn change_event_legacy(&self, ev: &mut EventState, timeout: i128) -> PyResult<ModeAnswer> {
        if ev.poll.is_none() {
            // Python keeps the poll object from here on, so a port that
            // raises leaves the ones before it registered for good.
            ev.poll = Some(PollSet::default());
            ev.registered = Registered::default();
            ev.before_first_poll = Vec::new();
            let list = utils::lock(&self.sfp).list.clone();
            for s in list {
                let s = s.ok_or_else(|| none_attr("get_fd_for_polling_legacy"))?;
                let Some(fd) = s.get_fd("present")? else {
                    ev.poll = None;
                    ev.registered = Registered::default();
                    ev.before_first_poll = Vec::new();
                    utils::log_warning("SFPs are not initialized, too early to get change event");
                    return Ok(ModeAnswer::Tuple);
                };
                let fileno = fd.fileno();
                if let Some(p) = ev.poll.as_mut() {
                    p.register(fileno, POLLERR_PRI);
                }
                ev.registered.insert(fileno, Reg { sdk_index: s.sdk_index, fd, fd_type: "" });
                let status = s.get_module_status()?;
                ev.before_first_poll.push((s.sdk_index, status));
            }
        }
        crate::imports::sfp();
        let (wait_forever, timeout) = Self::poll_timeout(timeout);
        let mut port_dict = PortDict::default();
        let mut error_dict = PortDict::default();
        let begin = std::time::Instant::now();
        loop {
            let iteration_begin = std::time::Instant::now();
            let mut has_read_error = false;
            let events = ev.poll.as_ref().map(|p| p.poll(timeout)).transpose()?.unwrap_or_default();
            for (fileno, _) in events {
                let Some(reg) = ev.registered.get_mut(fileno) else {
                    let repr = ev.registered.repr();
                    utils::log_error(&format!("Unknown file no {fileno} from poll event, registered files are {repr}"));
                    continue;
                };
                let sfp_index = reg.sdk_index;
                if let Err(e) = reg.fd.seek0() {
                    utils::log_warning(&format!("Failed to seek module sysfs fd for SFP {sfp_index}: {}", e.str()));
                    has_read_error = true;
                    continue;
                }
                match reg.fd.read() {
                    Ok(_) => {}
                    Err(e) if e.exc.is_os_error() => {
                        utils::log_warning(&format!("Failed to read module sysfs fd for SFP {sfp_index}: {}", e.str()));
                        has_read_error = true;
                        continue;
                    }
                    Err(e) => return Err(e),
                }
                let s = self.sfp_at(sfp_index)?.ok_or_else(|| none_attr("get_module_status"))?;
                let mut sfp_status = s.get_module_status()?.to_string();
                if let Some(pos) = ev.before_first_poll.iter().position(|(i, _)| *i == sfp_index) {
                    let (_, before) = ev.before_first_poll.remove(pos);
                    if before == sfp_status {
                        continue;
                    }
                }
                utils::log_notice(&format!("Got SFP event: index={sfp_index}, value={sfp_status}"));
                if sfp_status == sfpe::SFP_STATUS_UNKNOWN {
                    utils::log_notice("unknown module state, maybe the port suffers two adjacent insertion/removal");
                    continue;
                }
                if sfp_status == sfpe::SFP_STATUS_ERROR {
                    let (status, desc) = s.get_error_info_from_sdk_error_type();
                    sfp_status = status;
                    if let Some(d) = desc {
                        error_dict.set(sfp_index + 1, d);
                    }
                }
                port_dict.set(sfp_index + 1, sfp_status);
            }
            if !port_dict.is_empty() {
                return Ok(self.send_change_event(port_dict, error_dict));
            }
            if Self::round_ends(has_read_error, iteration_begin, begin, wait_forever, timeout) {
                return Ok(ModeAnswer::Dict(vec![(PyVal::str("sfp"), PortDict::default())]));
            }
        }
    }

    /// The common tail: log, re-initialise removed ports, answer.
    fn send_change_event(&self, port_dict: PortDict, error_dict: PortDict) -> ModeAnswer {
        utils::log_notice(&format!("Sending SFP change event: {}, error event: {}", port_dict.repr(), error_dict.repr()));
        self.reinit_sfps(&port_dict);
        ModeAnswer::Dict(vec![(PyVal::str("sfp"), port_dict), (PyVal::str("sfp_error"), error_dict)])
    }

    /// `reinit_sfps(port_dict)`: every port reported removed forgets its
    /// module; a failure is logged with its repr.
    fn reinit_sfps(&self, port_dict: &PortDict) {
        for (index, status) in &port_dict.0 {
            if !status.py_eq(&PyVal::str(sfpe::SFP_STATUS_REMOVED)) {
                continue;
            }
            let r = (|| -> PyResult<()> {
                let i = match index {
                    PyVal::Int(i) => *i,
                    other => platform_base::py::py_int(&other.py_str())?,
                };
                let s = self.sfp_at(i - 1)?.ok_or_else(|| none_attr("reinit"))?;
                s.reinit();
                Ok(())
            })();
            if let Err(e) = r {
                utils::log_error(&format!("Fail to re-initialize SFP {} - {}", index.py_str(), e.repr()));
            }
        }
    }

    /// `get_change_event_for_module_host_management_mode(timeout)`: the
    /// state machine fed by `hw_present`/`power_good` (software control) or
    /// `present` (firmware control), and by the ports whose reset wait is
    /// over.
    fn change_event_host_mgmt(&self, ev: &mut EventState, timeout: i128) -> PyResult<ModeAnswer> {
        if ev.poll.is_none() {
            ev.poll = Some(PollSet::default());
            ev.registered = Registered::default();
            let mut not_ready: Vec<i128> = Vec::new();
            if !ev.last_asic_ready.is_empty() {
                for i in 0..device_data::get_asic_count()?.max(0) {
                    let name = format!("asic{}_ready", i + 1);
                    let state = ev.last_asic_ready.iter().find(|(n, _)| *n == name).and_then(|(_, v)| v.clone());
                    if state.as_deref().is_none_or(|v| v == "0") {
                        not_ready.extend(self.asic_modules(&format!("asic{i}")).iter().map(|s| s.sdk_index));
                    }
                }
            }
            let list = utils::lock(&self.sfp).list.clone();
            for s in list {
                let s = s.ok_or_else(|| none_attr("sdk_index"))?;
                if not_ready.contains(&s.sdk_index) {
                    continue;
                }
                for (fd_type, fd) in s.get_fds_for_poling()? {
                    let Some(fd) = fd else {
                        ev.poll = None;
                        ev.registered = Registered::default();
                        utils::log_warning("SFPs are not initialized, too early to get change event");
                        return Ok(ModeAnswer::Tuple);
                    };
                    let fileno = fd.fileno();
                    if let Some(p) = ev.poll.as_mut() {
                        p.register(fileno, POLLERR_PRI);
                    }
                    ev.registered.insert(fileno, Reg { sdk_index: s.sdk_index, fd, fd_type });
                }
            }
        }
        crate::imports::sfp();
        let (wait_forever, timeout) = Self::poll_timeout(timeout);
        let mut port_dict = PortDict::default();
        let mut error_dict = PortDict::default();
        let begin = std::time::Instant::now();
        let wait_ready_task = crate::wait_sfp_ready_task::get();
        loop {
            let iteration_begin = std::time::Instant::now();
            let mut has_read_error = false;
            let events = ev.poll.as_ref().map(|p| p.poll(timeout)).transpose()?.unwrap_or_default();
            for (fileno, _) in events {
                let Some(reg) = ev.registered.get_mut(fileno) else {
                    let repr = ev.registered.repr();
                    utils::log_error(&format!("Unknown file no {fileno} from poll event, registered files are {repr}"));
                    continue;
                };
                let (sfp_index, fd_type) = (reg.sdk_index, reg.fd_type);
                let s = self.sfp_at(sfp_index)?;
                if let Err(e) = reg.fd.seek0() {
                    utils::log_warning(&format!("Failed to seek file {fd_type} for SFP {sfp_index}: {}", e.str()));
                    has_read_error = true;
                    continue;
                }
                let value = reg.fd.read().and_then(|t| platform_base::py::py_int(platform_base::py::strip(&t)));
                let fd_value = match value {
                    Ok(v) => v,
                    Err(e) if e.exc.is_os_error() || e.exc.name() == "ValueError" || e.exc.name() == "UnicodeDecodeError" => {
                        utils::log_warning(&format!("Failed to read value from file {fd_type} for SFP {sfp_index}: {}", e.str()));
                        has_read_error = true;
                        continue;
                    }
                    Err(e) => return Err(e),
                };
                let s = s.ok_or_else(|| none_attr("is_dummy_event"))?;
                if s.is_dummy_event(fd_type, fd_value)? {
                    continue;
                }
                utils::log_notice(&format!("Got SFP event: index={sfp_index}, type={fd_type}, value={fd_value}"));
                if fd_type == "hw_present" {
                    let event = if fd_value == 0 { sfpe::EVENT_NOT_PRESENT } else { sfpe::EVENT_PRESENT };
                    if fd_value == 1 {
                        s.processing_insert_event.store(true, std::sync::atomic::Ordering::Relaxed);
                    }
                    s.on_event(event)?;
                } else if fd_type == "present" {
                    if fd_value.to_string() == sfpe::SFP_STATUS_ERROR {
                        let (status, desc) = s.get_error_info_from_sdk_error_type();
                        port_dict.set(sfp_index + 1, status);
                        if let Some(d) = desc {
                            error_dict.set(sfp_index + 1, d);
                        }
                        continue;
                    } else if fd_value.to_string() == sfpe::SFP_STATUS_INSERTED {
                        port_dict.set(sfp_index + 1, sfpe::SFP_STATUS_INSERTED.to_string());
                        continue;
                    }
                    s.on_event(sfpe::EVENT_NOT_PRESENT)?;
                } else {
                    let event = if fd_value == 0 { sfpe::EVENT_POWER_BAD } else { sfpe::EVENT_POWER_GOOD };
                    s.on_event(event)?;
                }
                self.settle(&s, &mut port_dict, ev)?;
            }
            for sfp_index in wait_ready_task.get_ready_set() {
                let sfp_index = sfp_index as i128;
                let s = self.sfp_at(sfp_index)?.ok_or_else(|| none_attr("asic_id"))?;
                let asic_index = platform_base::py::py_int(&s.asic_id.replace("asic", ""))?;
                let ready_path = format!("/var/run/hw-management/config/asic{}_ready", asic_index + 1);
                if !platform_base::io::exists(&ready_path) || utils::read_int(&ready_path, 0, Log::Error) == 0 {
                    utils::log_info(&format!("SFP {sfp_index} ready event ignored: ASIC {asic_index} became not ready"));
                    continue;
                }
                s.on_event(sfpe::EVENT_RESET_DONE)?;
                if s.in_stable_state()? {
                    self.settle(&s, &mut port_dict, ev)?;
                } else {
                    utils::log_error(&format!("SFP {sfp_index} failed to reach stable state, state={}", s.get_state()?));
                }
            }
            if !port_dict.is_empty() {
                return Ok(self.send_change_event(port_dict, error_dict));
            }
            if Self::round_ends(has_read_error, iteration_begin, begin, wait_forever, timeout) {
                return Ok(ModeAnswer::Dict(vec![(PyVal::str("sfp"), PortDict::default())]));
            }
        }
    }

    /// A module in a stable state: its EEPROM given 2 seconds, its event
    /// filled in, its polling switched to what the state polls.
    fn settle(&self, s: &Arc<Sfp>, port_dict: &mut PortDict, ev: &mut EventState) -> PyResult<()> {
        if !s.in_stable_state()? {
            return Ok(());
        }
        sfpe::wait_sfp_eeprom_ready(std::slice::from_ref(s), 2.0)?;
        s.fill_change_event(port_dict)?;
        let poll = ev.poll.as_mut().ok_or_else(|| none_attr("register"))?;
        s.refresh_poll_obj(poll, &mut ev.registered)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, CONFIG_DB, STATE_DB};

    const PLATFORM_DIR: &str = "/usr/share/sonic/platform";
    const HW_CONFIG: &str = "/run/hw-management/config";
    const THERMAL: &str = "/run/hw-management/thermal";
    const SN5600: &str = "x86_64-nvidia_sn5600-r0";
    const SIMX: &str = "x86_64-nvidia_sn5600_simx-r0";

    /// `platform` with `platform_json`, seen from pmon (`/.dockerenv`), its
    /// sysfs labels ready.
    fn switch(platform: &str, platform_json: &str) -> Root {
        let root = Root::new();
        root.write("/host/machine.conf", &format!("onie_platform={platform}\n"));
        root.write(&format!("{PLATFORM_DIR}/platform.json"), platform_json);
        root.write("/.dockerenv", "");
        root.write(utils::SYSFS_LABELS_READY_FILE, "1");
        std::fs::create_dir_all(root.real("/tmp")).unwrap();
        root
    }

    /// The switch's SKU: `wanted`, unless the process already holds another
    /// (device_info keeps the first it reads), with its directory made.
    fn sku(root: &Root, wanted: &str) -> String {
        let redis = FakeRedis::get();
        redis.install(root);
        redis.set(CONFIG_DB, "CONFIG_DB_INITIALIZED", "1");
        redis.hset(CONFIG_DB, "DEVICE_METADATA|localhost", &[("hwsku", wanted)]);
        let sku = device_info::get_hwsku().expect("hwsku");
        root.write(&format!("{PLATFORM_DIR}/{sku}/sai.profile"), "SAI_VS_SWITCH_TYPE=SAI_VS_SWITCH_TYPE_MLNX\n");
        sku
    }

    fn classes(v: &PyVal) -> Vec<String> {
        let PyVal::List(items) = v else {
            panic!("not a list: {}", v.py_repr());
        };
        items
            .iter()
            .map(|x| match x {
                PyVal::Obj(o) => o.py_class().to_string(),
                other => other.py_repr(),
            })
            .collect()
    }

    fn err<T>(r: PyResult<T>) -> (String, String) {
        match r {
            Ok(_) => panic!("no exception"),
            Err(e) => (e.exc.name().to_string(), e.str()),
        }
    }

    fn fixed() -> Arc<Chassis> {
        Chassis::new(ChassisKind::Fixed).unwrap()
    }

    #[test]
    fn platform_picks_the_chassis_class_python_picks() {
        {
            // get_dpu_count reads platform.json: `json_data.get` on None.
            let root = switch(SN5600, "{}");
            std::fs::remove_file(root.real(&format!("{PLATFORM_DIR}/platform.json"))).unwrap();
            assert_eq!(err(platform()), ("AttributeError".into(), "'NoneType' object has no attribute 'get'".into()));
        }
        {
            let _root = switch(SN5600, "{}");
            let c = platform().unwrap();
            assert_eq!(c.py_class(), "Chassis");
            assert!(Arc::ptr_eq(&instance("get_all_sfps").unwrap(), &c));
            assert_eq!(c.is_modular_chassis().unwrap(), PyVal::Bool(false));
            assert_eq!(c.is_smartswitch().unwrap(), PyVal::Bool(false));
            assert_eq!(c.get_all_modules().unwrap(), PyVal::List(vec![]));
            assert_eq!(c.init_midplane_switch().unwrap(), PyVal::NotImplementedClass);
            assert_eq!(err(c.get_dpu_id()).0, "NotImplementedError");
            assert_eq!(err(c.get_module_index("DPU0")).0, "NotImplementedError");
            assert_eq!(c.get_position_in_parent().unwrap(), PyVal::Int(-1));
            assert_eq!(c.is_replaceable().unwrap(), PyVal::Bool(false));
        }
        {
            // A DPU in platform.json makes it a SmartSwitch, its DPUs built
            // with it, once.
            let _root = switch("x86_64-nvidia_sn4280-r0", r#"{"DPUS": {"dpu0": {}, "dpu1": {}}}"#);
            let c = platform().unwrap();
            assert_eq!(c.py_class(), "SmartSwitchChassis");
            assert_eq!(c.is_smartswitch().unwrap(), PyVal::Bool(true));
            assert_eq!(c.is_modular_chassis().unwrap(), PyVal::Bool(false));
            assert_eq!(c.init_midplane_switch().unwrap(), PyVal::Bool(true));
            assert_eq!(
                err(c.get_dpu_id()),
                (
                    "TypeError".into(),
                    "SmartSwitchChassis.get_dpu_id() missing 1 required positional argument: 'name'".into()
                )
            );
            let modules = c.get_all_modules().unwrap();
            assert_eq!(classes(&modules), ["DpuModule", "DpuModule"]);
            assert!(c.get_all_modules().unwrap().py_eq(&modules));
            // `module_name_index_map[module_name.upper()]`.
            assert_eq!(c.get_module_index("DPU1").unwrap(), PyVal::Int(1));
            assert_eq!(c.get_module_index("dpu0").unwrap(), PyVal::Int(0));
            assert_eq!(err(c.get_module_index("DPU2")), ("KeyError".into(), "'DPU2'".into()));
        }
        {
            // Line cards make it modular: one Module per slot, and the SFPs
            // of every card -- none while the cards are not active.
            let root = switch("x86_64-nvidia_sn4800-r0", "{}");
            root.write(&format!("{HW_CONFIG}/hotplug_linecards"), "2");
            let c = platform().unwrap();
            assert_eq!(c.py_class(), "ModularChassis");
            assert_eq!(c.is_modular_chassis().unwrap(), PyVal::Bool(true));
            assert_eq!(c.is_smartswitch().unwrap(), PyVal::Bool(false));
            // `int(module_name[len('LINE-CARD')-1:])`, -1 for anything else:
            // the slice starts one character early, so "LINE-CARD1" is "D1".
            assert_eq!(c.get_module_index("LINECARD2").unwrap(), PyVal::Int(2));
            assert_eq!(c.get_module_index("LINE-CARD1").unwrap(), PyVal::Int(-1));
            assert_eq!(c.get_module_index("").unwrap(), PyVal::Int(-1));
            let modules = c.get_all_modules().unwrap();
            assert_eq!(classes(&modules), ["Module", "Module"]);
            assert!(c.get_all_modules().unwrap().py_eq(&modules));
            assert!(c.get_all_sfps().unwrap().is_empty());
            // The SFP count is the cards' module counters added up, whether
            // or not the cards are active.
            root.write("/run/hw-management/lc1/config/module_counter", "16");
            root.write("/run/hw-management/lc2/config/module_counter", "8");
            assert_eq!(c.get_num_sfps().unwrap(), 24);
        }
    }

    #[test]
    fn psus_and_pdbs_are_what_hw_management_counts() {
        {
            let root = switch(SN5600, "{}");
            root.write(&format!("{HW_CONFIG}/hotplug_psus"), "2");
            root.write("/var/run/hw-management/config/hotplug_pdbs", "2");
            let c = fixed();
            let psus = c.get_all_psus().unwrap();
            assert_eq!(classes(&psus), ["Psu", "Psu"]);
            assert!(c.get_all_psus().unwrap().py_eq(&psus));
            assert_eq!(classes(&c.get_all_pdbs().unwrap()), ["Pdb", "Pdb"]);
        }
        {
            // Not hot-swappable: one fixed PSU per i2c address.
            let root = switch(SN5600, "{}");
            root.write(&format!("{HW_CONFIG}/hotplug_psus"), "0");
            for i in 1..=3 {
                root.write(&format!("{HW_CONFIG}/psu{i}_i2c_addr"), "0x59");
            }
            let c = fixed();
            assert_eq!(classes(&c.get_all_psus().unwrap()), ["FixedPsu", "FixedPsu", "FixedPsu"]);
            assert!(classes(&c.get_all_pdbs().unwrap()).is_empty());
        }
        {
            let _root = switch(SN5600, "{}");
            assert!(classes(&fixed().get_all_psus().unwrap()).is_empty());
        }
    }

    #[test]
    fn fans_are_shared_out_to_their_drawers() {
        let fans_of = |d: &PyVal| match d {
            PyVal::Obj(o) => classes(&o.get_all_fans().unwrap()).len(),
            _ => 0,
        };
        {
            // Hot-swappable: a drawer per status file, the fans shared out.
            let root = switch(SN5600, "{}");
            root.write(&format!("{HW_CONFIG}/hotplug_fans"), "4");
            for i in 1..=4 {
                root.write(&format!("{THERMAL}/fan{i}_status"), "1");
            }
            for i in 1..=8 {
                root.write(&format!("{THERMAL}/fan{i}_speed_get"), "6000");
            }
            let c = fixed();
            let drawers = c.get_all_fan_drawers().unwrap();
            assert_eq!(classes(&drawers), ["RealDrawer"; 4]);
            let PyVal::List(list) = &drawers else { unreachable!() };
            assert_eq!(list.iter().map(fans_of).collect::<Vec<_>>(), [2, 2, 2, 2]);
            assert!(c.get_all_fan_drawers().unwrap().py_eq(&drawers));
        }
        {
            // Fixed fans: one virtual drawer holding all of them.
            let root = switch(SN5600, "{}");
            for i in 1..=4 {
                root.write(&format!("{THERMAL}/fan{i}_status"), "1");
                root.write(&format!("{THERMAL}/fan{i}_speed_get"), "6000");
            }
            let drawers = fixed().get_all_fan_drawers().unwrap();
            assert_eq!(classes(&drawers), ["VirtualDrawer"]);
            let PyVal::List(list) = &drawers else { unreachable!() };
            assert_eq!(fans_of(&list[0]), 4);
        }
        {
            // No drawer: nothing, and no wait for the sysfs labels.
            let root = switch(SN5600, "{}");
            std::fs::remove_file(root.real(utils::SYSFS_LABELS_READY_FILE)).unwrap();
            assert!(classes(&fixed().get_all_fan_drawers().unwrap()).is_empty());
        }
    }

    #[test]
    fn thermals_are_built_once() {
        let _root = switch(SN5600, "{}");
        let c = fixed();
        let thermals = c.get_all_thermals().unwrap();
        assert!(matches!(thermals, PyVal::List(_)));
        assert!(c.get_all_thermals().unwrap().py_eq(&thermals));
    }

    #[test]
    fn eeprom_fields_come_from_state_db() {
        let root = switch(SN5600, "{}");
        root.write("/var/run/hw-management/eeprom/vpd_info", "");
        root.write(VPD_DATA_FILE, "SYS_DISPLAY: SN5600\n");
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.hset(STATE_DB, "EEPROM_INFO|State", &[("Initialized", "1")]);
        for (code, value) in [("0x21", "MSN5600"), ("0x22", "MSN5600-VS2FC"), ("0x23", "MT2245X01234"), ("0x24", "1C:34:DA:00:00:01")] {
            redis.hset(STATE_DB, &format!("EEPROM_INFO|{code}"), &[("Value", value)]);
        }
        let c = fixed();
        let PyVal::Obj(e) = c.get_eeprom().unwrap() else { panic!() };
        assert_eq!(e.py_class(), "Eeprom");
        let PyVal::Obj(again) = c.get_eeprom().unwrap() else { panic!() };
        assert!(Arc::ptr_eq(&e, &again));
        assert_eq!(c.get_name().unwrap(), PyVal::str("MSN5600"));
        assert_eq!(c.get_serial().unwrap(), PyVal::str("MT2245X01234"));
        assert_eq!(c.get_base_mac().unwrap(), PyVal::str("1C:34:DA:00:00:01"));
        assert_eq!(c.model(false).unwrap(), PyVal::str("MSN5600-VS2FC"));
        assert_eq!(
            c.get_system_eeprom_info().unwrap().py_repr(),
            "{'0x21': 'MSN5600', '0x22': 'MSN5600-VS2FC', '0x23': 'MT2245X01234', '0x24': '1C:34:DA:00:00:01'}"
        );
        // get_model asks the SKU which of the two it is -- the SKU the
        // process cached first, which may be one the SKU pattern does not
        // match, and then both raise alike.
        let sku = sku(&root, "ACS-SN5600");
        let want = model_in_vpd(Some(&sku)).and_then(|in_vpd| c.model(in_vpd));
        assert_eq!(c.get_model().map_err(|e| e.repr()), want.map_err(|e| e.repr()));
    }

    #[test]
    fn the_model_is_in_the_vpd_from_spectrum_4_on() {
        assert!(model_in_vpd(Some("ACS-SN5600")).unwrap());
        assert!(!model_in_vpd(Some("Mellanox-SN4700-O8C48")).unwrap());
        assert!(!model_in_vpd(Some("ACS-MSN2700")).unwrap());
        assert_eq!(err(model_in_vpd(Some("ACS-SN"))), ("AttributeError".into(), "'NoneType' object has no attribute 'group'".into()));
        assert_eq!(
            err(model_in_vpd(None)),
            ("TypeError".into(), "expected string or bytes-like object, got 'NoneType'".into())
        );
    }

    #[test]
    fn revision_and_model_come_from_the_vpd_file() {
        if crate::testing::platform_from_env() {
            return;
        }
        {
            // tests/vpd_data_file's lines that matter, as hw-management writes them.
            let root = switch(SN5600, "{}");
            root.write(
                VPD_DATA_FILE,
                "Part Number:             MSN4700-WS2FO\nREV:                     A7\nSYS_DISPLAY:             MSN4700\n",
            );
            let c = fixed();
            assert_eq!(c.get_revision().unwrap(), PyVal::str("A7"));
            assert_eq!(c.model(true).unwrap(), PyVal::str("MSN4700"));
        }
        {
            // No file, and no directory to wait in: N/A at once.
            let _root = switch(SN5600, "{}");
            assert_eq!(fixed().get_revision().unwrap(), PyVal::str("N/A"));
        }
        {
            // simx does not wait for the file, and takes the model from the
            // EEPROM instead.
            let root = switch(SIMX, "{}");
            root.write("/var/run/hw-management/eeprom/vpd_info", "");
            let redis = FakeRedis::get();
            redis.install(&root);
            redis.hset(STATE_DB, "EEPROM_INFO|State", &[("Initialized", "1")]);
            redis.hset(STATE_DB, "EEPROM_INFO|0x22", &[("Value", "MSN5600-SIMX")]);
            std::fs::create_dir_all(root.real("/var/run/hw-management/eeprom")).unwrap();
            let c = fixed();
            assert_eq!(c.get_revision().unwrap(), PyVal::str("N/A"));
            assert_eq!(c.model(true).unwrap(), PyVal::str("MSN5600-SIMX"));
        }
    }

    const MAJOR: [(&str, &str); 25] = [
        ("reset_main_pwr_fail", "Power Loss"),
        ("reset_ac_pwr_fail", "Power Loss"),
        ("reset_aux_pwr_or_ref", "Power Loss"),
        ("reset_aux_pwr_or_reload", "Power Loss"),
        ("reset_aux_pwr_or_fu", "Power Loss"),
        ("reset_comex_pwr_fail", "Power Loss"),
        ("reset_main_51v", "Power Loss"),
        ("reset_mgmt_pwr_fail", "Power Loss"),
        ("reset_sw_pwr_off", "Power Loss"),
        ("reset_asic_thermal", "Thermal Overload: ASIC"),
        ("reset_cpu_thermal", "Thermal Overload: CPU"),
        ("reset_comex_thermal", "Thermal Overload: CPU"),
        ("reset_hotswap_or_wd", "Watchdog"),
        ("reset_comex_wd", "Watchdog"),
        ("reset_swb_wd", "Watchdog"),
        ("reset_sff_wd", "Watchdog"),
        ("reset_hotswap_or_halt", "Hardware - Other"),
        ("reset_voltmon_upgrade_fail", "Hardware - Other"),
        ("reset_pwr_converter_fail", "Hardware - Other"),
        ("reset_swb_dc_dc_pwr_fail", "Hardware - Other"),
        ("reset_reload_bios", "BIOS"),
        ("reset_fw_reset", "Reset from ASIC"),
        ("reset_from_asic", "Reset from ASIC"),
        ("reset_long_pb", "Push button"),
        ("reset_short_pb", "Push button"),
    ];

    fn cause(c: &Chassis) -> String {
        c.get_reboot_cause().unwrap().py_repr()
    }

    #[test]
    fn the_reboot_cause_is_the_first_reset_file_set() {
        let root = switch(SN5600, "{}");
        root.write(REBOOT_CAUSE_READY_FILE, "1");
        let c = fixed();
        assert_eq!(cause(&c), "('Non-Hardware', '')");
        for (file, want) in MAJOR {
            let path = format!("{REBOOT_CAUSE_ROOT}{file}");
            root.write(&path, "1");
            assert_eq!(cause(&c), format!("('{want}', '')"), "{file}");
            root.write(&path, "0");
        }
        root.write(&format!("{REBOOT_CAUSE_ROOT}reset_sw_reset"), "1");
        assert_eq!(cause(&c), "('Non-Hardware', '')");
        // A BMC's power-down request, on a chassis built on a BMC platform
        // (Python settles the cause at a chassis's first reboot-cause call).
        root.write(&format!("{PLATFORM_DIR}/platform_env.conf"), "switch_host=1\n");
        root.write("/etc/sonic/bmc.json", r#"{"bmc_addr": "169.254.0.1"}"#);
        root.write(&format!("{REBOOT_CAUSE_ROOT}reset_sw_pwr_off"), "1");
        // `c` settled it before the BMC appeared.
        assert_eq!(cause(&c), "('Power Loss', '')");
        assert_eq!(cause(&fixed()), "('Power down request from BMC', '')");
    }

    #[test]
    fn on_the_host_a_warm_or_fast_boot_is_no_hardware_reboot() {
        let root = switch(SN5600, "{}");
        std::fs::remove_file(root.real("/.dockerenv")).unwrap();
        root.write(REBOOT_CAUSE_READY_FILE, "1");
        root.write(&format!("{REBOOT_CAUSE_ROOT}reset_main_pwr_fail"), "1");
        let c = fixed();
        assert_eq!(c.parse_warmfast_reboot_from_proc_cmdline().unwrap(), None);
        assert_eq!(cause(&c), "('Power Loss', '')");
        for (cmdline, want) in [
            ("BOOT_IMAGE=/image/boot/vmlinuz SONIC_BOOT_TYPE=warm quiet\n", Some("warm-reboot")),
            ("SONIC_BOOT_TYPE=fastfast\n", Some("warm-reboot")),
            ("SONIC_BOOT_TYPE=fast\n", Some("fast-reboot")),
            ("SONIC_BOOT_TYPE=fast-reboot", Some("fast-reboot")),
            ("SONIC_BOOT_TYPE=None\n", None),
            // Only the first line is read.
            ("quiet\nSONIC_BOOT_TYPE=warm\n", None),
        ] {
            root.write(REBOOT_TYPE_KEXEC_FILE, cmdline);
            assert_eq!(c.parse_warmfast_reboot_from_proc_cmdline().unwrap(), want, "{cmdline:?}");
        }
        root.write(REBOOT_TYPE_KEXEC_FILE, "SONIC_BOOT_TYPE=warm\n");
        assert_eq!(cause(&c), "('Non-Hardware', '')");
    }

    #[test]
    fn the_system_led_and_the_rest_of_the_chassis() {
        let root = switch(SN5600, "{}");
        let led = "/var/run/hw-management/led";
        root.write(&format!("{led}/led_status_capability"), "none green red green_blink");
        for color in ["green", "red"] {
            root.write(&format!("{led}/led_status_{color}"), "0");
        }
        let c = fixed();
        assert_eq!(c.initizalize_system_led().unwrap(), PyVal::None);
        assert_eq!(c.set_status_led("red").unwrap(), PyVal::Bool(true));
        assert_eq!(root.read(&format!("{led}/led_status_red")).as_deref(), Some("255"));
        assert_eq!(c.get_status_led().unwrap(), PyVal::str("red"));
        // The thermal manager's class, once hw-management's helper is there.
        assert_eq!(err(c.get_thermal_manager()).0, "ModuleNotFoundError");
        crate::hw_mgmt::testing::install_independent_mode_update(&root);
        assert!(c.get_thermal_manager().is_ok());
        let PyVal::Obj(lc) = c.get_liquid_cooling().unwrap() else { panic!() };
        assert_eq!(lc.py_class(), "LiquidCooling");
        let PyVal::Obj(lc2) = c.get_liquid_cooling().unwrap() else { panic!() };
        assert!(Arc::ptr_eq(&lc, &lc2));
        let PyVal::Obj(sed) = c.get_sed_mgmt().unwrap() else { panic!() };
        assert_eq!(sed.py_class(), "SedMgmt");
    }

    #[test]
    fn the_watchdog_is_found_when_first_asked_for() {
        let root = switch(SN5600, "{}");
        root.write("/run/hw-management/watchdog/main/state", "inactive");
        let c = fixed();
        // No /dev to look in: logged, and None.
        assert_eq!(c.get_watchdog().unwrap(), PyVal::None);
        std::fs::create_dir_all(root.real("/dev")).unwrap();
        assert_eq!(c.get_watchdog().unwrap(), PyVal::None);
        root.write("/dev/watchdog0", "");
        root.write("/sys/class/watchdog/watchdog0/identity", "mlx-wdt-main\n");
        let PyVal::Obj(w) = c.get_watchdog().unwrap() else { panic!() };
        assert_eq!(w.py_class(), "WatchdogType1");
        let PyVal::Obj(again) = c.get_watchdog().unwrap() else { panic!() };
        assert!(Arc::ptr_eq(&w, &again));
    }

    #[test]
    fn components_are_the_host_s() {
        if crate::testing::platform_from_env() {
            return;
        }
        {
            // In a container: none, and no BMC on a platform without one.
            let _root = switch(SN5600, "{}");
            let c = fixed();
            assert_eq!(c.get_all_components().unwrap(), PyVal::List(vec![]));
        }
        {
            let root = switch(SIMX, "{}");
            std::fs::remove_file(root.real("/.dockerenv")).unwrap();
            let c = fixed();
            let components = c.get_all_components().unwrap();
            assert_eq!(classes(&components), ["ComponentONIE", "ComponentSSD", "ComponentBIOS"]);
            assert!(c.get_all_components().unwrap().py_eq(&components));
            // The BMC is whatever bmc.py's instance is: none here.
            let bmc = crate::bmc::get_instance().unwrap().is_some();
            assert_eq!(matches!(c.get_bmc().unwrap(), PyVal::Obj(_)), bmc);
            assert_eq!(matches!(c.get_bmc().unwrap(), PyVal::Obj(_)), bmc);
        }
    }

    /// platform.json: `sfps` ports, and an interface per port.
    fn sfp_platform(sfps: usize, interfaces: usize) -> String {
        let ports: Vec<String> = (0..interfaces).map(|i| format!(r#""Ethernet{}": {{"index": "{},{},{},{}"}}"#, i * 8, i + 1, i + 1, i + 1, i + 1)).collect();
        format!(r#"{{"chassis": {{"sfps": [{}]}}, "interfaces": {{{}}}}}"#, vec!["{}"; sfps].join(", "), ports.join(", "))
    }

    #[test]
    fn rj45_and_cpo_ports_are_counted_with_the_sfps() {
        let root = switch(SN5600, &sfp_platform(3, 3));
        let sku = sku(&root, "ACS-SN5600");
        let c = fixed();
        assert_eq!(c.get_num_sfps().unwrap(), 3);
        drop(c);
        root.write(
            &format!("{PLATFORM_DIR}/{sku}/hwsku.json"),
            r#"{"interfaces": {"Ethernet0": {"port_type": "RJ45"}, "Ethernet8": {"port_type": "CPO"}, "Ethernet16": {"default_brkout_mode": "1x100G"}}}"#,
        );
        let c = fixed();
        assert_eq!(c.get_num_sfps().unwrap(), 5);
        assert_eq!(c.sfp_kind(0).unwrap(), Kind::Rj45);
        assert_eq!(c.sfp_kind(1).unwrap(), Kind::Cpo);
        assert_eq!(c.sfp_kind(2).unwrap(), Kind::Sfp);
    }

    #[test]
    fn sfps_are_built_once_ready_for_use() {
        let root = switch(SN5600, &sfp_platform(3, 3));
        let sku = sku(&root, "ACS-SN5600");
        // Ethernet0 is an RJ45 port as well: one more port, whose EEPROM is
        // not waited for.
        root.write(&format!("{PLATFORM_DIR}/{sku}/hwsku.json"), r#"{"interfaces": {"Ethernet0": {"port_type": "RJ45"}}}"#);
        let c = fixed();
        // sfp.py imports hw-management's helper, and fails without it.
        assert_eq!(err(c.get_all_sfps()).0, "ModuleNotFoundError");
        crate::hw_mgmt::testing::install_independent_mode_update(&root);
        // Not ready yet: every module's sysfs, then every EEPROM.
        for i in 0..4 {
            for node in ["present", "status", "statuserror"] {
                root.write(&format!("/sys/module/sx_core/asic0/module{i}/{node}"), "0");
            }
        }
        let sfps = c.get_all_sfps().unwrap();
        assert_eq!(sfps.iter().map(|s| s.sdk_index).collect::<Vec<_>>(), [0, 1, 2, 3]);
        assert_eq!(sfps.iter().map(|s| s.class_name()).collect::<Vec<_>>(), ["RJ45Port", "SFP", "SFP", "SFP"]);
        assert!(root.exists("/tmp/sfp_ready"));
        let again = c.get_all_sfps().unwrap();
        assert!(sfps.iter().zip(&again).all(|(a, b)| Arc::ptr_eq(a, b)));
    }

    #[test]
    fn simx_does_not_wait_for_sfp_eeproms() {
        if crate::testing::platform_from_env() {
            return;
        }
        let root = switch(SIMX, &sfp_platform(2, 2));
        sku(&root, "ACS-SN5600");
        crate::hw_mgmt::testing::install_independent_mode_update(&root);
        for i in 0..2 {
            for node in ["present", "status", "statuserror"] {
                root.write(&format!("/sys/module/sx_core/asic0/module{i}/{node}"), "1");
            }
        }
        let c = fixed();
        assert_eq!(c.get_all_sfps().unwrap().len(), 2);
        assert!(root.exists("/tmp/sfp_ready"));
    }

    #[test]
    fn on_a_multi_asic_switch_each_sfp_is_on_its_asic() {
        let root = switch(SN5600, &sfp_platform(4, 3));
        root.write(&format!("{PLATFORM_DIR}/asic.conf"), "NUM_ASIC=2\n");
        let sku = sku(&root, "ACS-SN5600");
        for (asic, ports) in [(0, r#""Ethernet0": {}"#), (1, r#""Ethernet8": {}, "Ethernet16": {}"#)] {
            root.write(&format!("{PLATFORM_DIR}/{sku}/{asic}/hwsku.json"), &format!(r#"{{"interfaces": {{{ports}}}}}"#));
            root.write(&format!("{PLATFORM_DIR}/{sku}/{asic}/sai.profile"), "");
        }
        crate::hw_mgmt::testing::install_independent_mode_update(&root);
        root.write("/tmp/sfp_ready", "");
        let c = fixed();
        assert_eq!(c.asic_id_by_sfp_index(0).unwrap(), "asic0");
        assert_eq!(c.asic_id_by_sfp_index(2).unwrap(), "asic1");
        // The fourth port has no interface, so no ASIC -- and `int('None')`.
        assert_eq!(c.asic_id_by_sfp_index(3).unwrap(), "asicNone");
        assert_eq!(
            err(c.get_all_sfps()),
            ("ValueError".into(), "invalid literal for int() with base 10: 'None'".into())
        );
    }

    #[test]
    fn the_last_chassis_built_is_the_instance() {
        let _root = switch(SN5600, "{}");
        let a = fixed();
        let b = fixed();
        assert!(Arc::ptr_eq(&instance("get_all_sfps").unwrap(), &b));
        assert!(!Arc::ptr_eq(&a, &b));
    }
}

/// The change-event paths' tests, kept apart from the rest of the chassis's.
#[cfg(test)]
#[path = "chassis_event_tests.rs"]
mod event_tests;
