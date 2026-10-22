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


//! What `sonic_platform/sfp.py` and `state_machine.py` give the chassis's
//! change events: the module's SDK attributes, its status and error code,
//! and -- in module host management mode -- the per-module state machine
//! that powers, resets and hands each module to software or firmware
//! control.
//!
//! Deciding software control, the module's power class, its I2C frequency
//! and disabling an SFF optic's TX all go through sonic_xcvr's
//! `XcvrApiFactory`, which is not ported: those steps answer `not_ported`.

use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::time::Duration;

use platform_base::py::{PyErr, PyResult, PyVal};

use crate::poll::{PollSet, SysFile, POLLERR_PRI};
use crate::sfp::{Kind, Sfp};
use crate::utils::{self, Log};
use crate::wait_sfp_ready_task;

pub const SFP_STATUS_REMOVED: &str = "0";
pub const SFP_STATUS_INSERTED: &str = "1";
pub const SFP_STATUS_ERROR: &str = "2";
pub const SFP_STATUS_UNKNOWN: &str = "-1";

const SFP_SW_CONTROL: i128 = 1;
const SFP_FW_CONTROL: i128 = 0;

const SFP_STATUS_BIT_INSERTED: i128 = 0x1;
const SFP_ERROR_BIT_BLOCKING: i128 = 0x2;
const SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED: i128 = 0x4;
const SFP_ERROR_BIT_I2C_STUCK: i128 = 0x8;
const SFP_ERROR_BIT_BAD_EEPROM: i128 = 0x10;
const SFP_ERROR_BIT_UNSUPPORTED_CABLE: i128 = 0x20;
const SFP_ERROR_BIT_HIGH_TEMP: i128 = 0x40;
const SFP_ERROR_BIT_BAD_CABLE: i128 = 0x80;
const SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE: i128 = 0x0001_0000;
const SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST: i128 = 0x0002_0000;
const SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED: i128 = 0x0004_0000;
const SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED: i128 = 0x0008_0000;
/// `SDK_SFP_BLOCKING_ERRORS`.
const SDK_SFP_BLOCKING_ERRORS: [i128; 5] = [0x2, 0x3, 0x5, 0x6, 0x7];

pub const STATE_DOWN: &str = "Down";
pub const STATE_INIT: &str = "Initializing";
pub const STATE_RESETTING: &str = "Resetting";
pub const STATE_POWERED_ON: &str = "Power On";
pub const STATE_SW_CONTROL: &str = "Software Control";
pub const STATE_FW_CONTROL: &str = "Firmware Control";
pub const STATE_POWER_BAD: &str = "Power Bad";
pub const STATE_POWER_LIMIT_ERROR: &str = "Exceed Power Limit";
pub const STATE_NOT_PRESENT: &str = "Not Present";
pub const STATE_FCP_DOWN: &str = "Down(Firmware Control)";
pub const STATE_FCP_INIT: &str = "Initializing(Firmware Control)";
pub const STATE_FCP_NOT_PRESENT: &str = "Not Present(Firmware Control)";
pub const STATE_FCP_PRESENT: &str = "Present(Firmware Control)";

pub const EVENT_START: &str = "Start";
pub const EVENT_NOT_PRESENT: &str = "Not Present";
pub const EVENT_RESET: &str = "Reset";
pub const EVENT_POWER_ON: &str = "Power On";
pub const EVENT_RESET_DONE: &str = "Reset Done";
pub const EVENT_POWER_BAD: &str = "Power Bad";
pub const EVENT_SW_CONTROL: &str = "Software Control";
pub const EVENT_FW_CONTROL: &str = "Firmware Control";
pub const EVENT_POWER_LIMIT_EXCEED: &str = "Power Limit Exceed";
pub const EVENT_POWER_GOOD: &str = "Power Good";
pub const EVENT_PRESENT: &str = "Present";

const ACTION_ON_START: &str = "On Start";
const ACTION_ON_RESET: &str = "On Reset";
const ACTION_ON_POWERED: &str = "On Powered";
const ACTION_ON_SW_CONTROL: &str = "On Software Control";
const ACTION_ON_FW_CONTROL: &str = "On Firmware Control";
const ACTION_ON_POWER_LIMIT_ERROR: &str = "On Power Limit Error";
const ACTION_ON_CANCEL_WAIT: &str = "On Cancel Wait";
const ACTION_FCP_ON_START: &str = "On Start(Firmware Control)";

/// One state of `SFP.get_state_machine()`: its entry action and its
/// transitions as `(event, next state, event action)`.
type StateDef = (Option<&'static str>, &'static [(&'static str, &'static str, Option<&'static str>)]);

/// The state machine `get_state_machine()` builds, state by state.
fn state_def(state: &str) -> Option<StateDef> {
    Some(match state {
        STATE_DOWN => (None, &[(EVENT_START, STATE_INIT, None)]),
        STATE_INIT => (
            Some(ACTION_ON_START),
            &[
                (EVENT_NOT_PRESENT, STATE_NOT_PRESENT, None),
                (EVENT_RESET, STATE_RESETTING, None),
                (EVENT_POWER_ON, STATE_POWERED_ON, None),
                (EVENT_FW_CONTROL, STATE_FW_CONTROL, None),
            ],
        ),
        STATE_RESETTING => (
            Some(ACTION_ON_RESET),
            &[
                (EVENT_RESET_DONE, STATE_POWERED_ON, None),
                (EVENT_NOT_PRESENT, STATE_NOT_PRESENT, Some(ACTION_ON_CANCEL_WAIT)),
            ],
        ),
        STATE_POWERED_ON => (
            Some(ACTION_ON_POWERED),
            &[
                (EVENT_POWER_BAD, STATE_POWER_BAD, None),
                (EVENT_SW_CONTROL, STATE_SW_CONTROL, None),
                (EVENT_FW_CONTROL, STATE_FW_CONTROL, None),
            ],
        ),
        STATE_SW_CONTROL => (
            Some(ACTION_ON_SW_CONTROL),
            &[
                (EVENT_NOT_PRESENT, STATE_NOT_PRESENT, None),
                (EVENT_POWER_LIMIT_EXCEED, STATE_POWER_LIMIT_ERROR, None),
                (EVENT_POWER_BAD, STATE_POWER_BAD, None),
            ],
        ),
        STATE_FW_CONTROL => (Some(ACTION_ON_FW_CONTROL), &[(EVENT_NOT_PRESENT, STATE_NOT_PRESENT, None)]),
        STATE_POWER_BAD => (
            None,
            &[(EVENT_POWER_GOOD, STATE_POWERED_ON, None), (EVENT_NOT_PRESENT, STATE_NOT_PRESENT, None)],
        ),
        STATE_NOT_PRESENT => (None, &[(EVENT_PRESENT, STATE_INIT, None)]),
        STATE_POWER_LIMIT_ERROR => (
            Some(ACTION_ON_POWER_LIMIT_ERROR),
            &[(EVENT_POWER_GOOD, STATE_POWERED_ON, None), (EVENT_NOT_PRESENT, STATE_NOT_PRESENT, None)],
        ),
        STATE_FCP_DOWN => (None, &[(EVENT_START, STATE_FCP_INIT, None)]),
        STATE_FCP_INIT => (
            Some(ACTION_FCP_ON_START),
            &[(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT, None), (EVENT_PRESENT, STATE_FCP_PRESENT, None)],
        ),
        STATE_FCP_NOT_PRESENT => (None, &[(EVENT_PRESENT, STATE_FCP_PRESENT, None)]),
        STATE_FCP_PRESENT => (None, &[(EVENT_NOT_PRESENT, STATE_FCP_NOT_PRESENT, None)]),
        _ => return None,
    })
}

/// The sonic_xcvr steps, unported.
fn xcvr(what: &str) -> PyErr {
    PyErr::not_ported(format!("SFP.{what}: needs sonic_xcvr's XcvrApiFactory"))
}

/// `get_fds_for_poling()`'s answer: `(fd_type, fd or None)` in dict order.
pub type PollFds = Vec<(&'static str, Option<SysFile>)>;

impl Sfp {
    // -- SDK attributes ------------------------------------------------------

    /// `get_fd(fd_type)`: the attribute opened, or None -- with a warning --
    /// when it is not there.
    pub fn get_fd(&self, fd_type: &str) -> PyResult<Option<SysFile>> {
        let path = self.sysfs(fd_type);
        match SysFile::open(&path) {
            Ok(f) => Ok(Some(f)),
            Err(e) if e.exc.name() == "FileNotFoundError" => {
                utils::log_warning(&format!("Trying to access {path} file which does not exist"));
                Ok(None)
            }
            Err(e) => Err(e),
        }
    }

    /// `get_module_status()`: the SDK status as a SONiC status; a status
    /// the table does not know is a `KeyError`.  An RJ45 port's unknown is
    /// removed.
    pub fn get_module_status(&self) -> PyResult<&'static str> {
        let status = utils::read_int(&self.sysfs("status"), 0, Log::Error);
        let s = match status {
            0x1 => SFP_STATUS_INSERTED,
            0x2 | 0x4 => SFP_STATUS_REMOVED,
            0x3 => SFP_STATUS_ERROR,
            0x5 => SFP_STATUS_UNKNOWN,
            other => return Err(PyErr::key_error(PyVal::Int(other))),
        };
        if self.kind() == Kind::Rj45 && s == SFP_STATUS_UNKNOWN {
            return Ok(SFP_STATUS_REMOVED);
        }
        Ok(s)
    }

    /// `get_error_info_from_sdk_error_type()`: the status bits as a string,
    /// and the vendor's description of the error if it has one.
    pub fn get_error_info_from_sdk_error_type(&self) -> (String, Option<String>) {
        let error_type = utils::read_int(&self.sysfs("statuserror"), -1, Log::Error);
        let bits = match error_type {
            0x0 => SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED,
            0x1 => SFP_MLNX_ERROR_BIT_LONGRANGE_NON_MLNX_CABLE,
            0x2 => SFP_ERROR_BIT_I2C_STUCK,
            0x3 => SFP_ERROR_BIT_BAD_EEPROM,
            0x4 => SFP_MLNX_ERROR_BIT_ENFORCE_PART_NUMBER_LIST,
            0x5 => SFP_ERROR_BIT_UNSUPPORTED_CABLE,
            0x6 => SFP_ERROR_BIT_HIGH_TEMP,
            0x7 => SFP_ERROR_BIT_BAD_CABLE,
            0x8 => SFP_MLNX_ERROR_BIT_PMD_TYPE_NOT_ENABLED,
            0xc => SFP_MLNX_ERROR_BIT_PCIE_POWER_SLOT_EXCEEDED,
            _ => {
                utils::log_error(&format!("Unrecognized error {error_type} detected on SFP {}", self.sdk_index));
                return (SFP_STATUS_ERROR.to_string(), Some(format!("Unknown error ({error_type})")));
            }
        };
        let mut bits = bits;
        if SDK_SFP_BLOCKING_ERRORS.contains(&error_type) {
            bits |= SFP_ERROR_BIT_BLOCKING;
        }
        bits |= SFP_STATUS_BIT_INSERTED;
        let description = match error_type {
            0x1 => Some("Long range for non-Mellanox cable or module"),
            0x4 => Some("Enforce part number list"),
            0x8 => Some("PMD type not enabled"),
            0xc => Some("PCIE system power slot exceeded"),
            _ => None,
        };
        (bits.to_string(), description.map(str::to_string))
    }

    /// `reinit()`: a new module forgets the old one's type.  Nothing for
    /// RJ45 and CPO ports.
    pub fn reinit(&self) {
        if self.kind() == Kind::Sfp {
            self.forget_type();
        }
    }

    fn read_attr(&self, node: &str) -> i128 {
        utils::read_int(&self.sysfs(node), 0, Log::Error)
    }

    fn write_attr(&self, node: &str, value: i128) -> PyResult<()> {
        utils::write_file(&self.sysfs(node), &value, false, Log::Error).map(|_| ())
    }

    // -- the state machine ---------------------------------------------------

    pub fn get_state(&self) -> PyResult<&'static str> {
        self.sfp_only("state")?;
        Ok(*utils::lock(&self.state))
    }

    fn change_state(&self, new_state: &'static str) {
        *utils::lock(&self.state) = new_state;
    }

    /// `on_event(event)`: `StateMachine.on_event` -- an event the state does
    /// not know is logged and ignored; the event's own action runs first,
    /// then the new state's entry action, which may raise further events.
    pub fn on_event(&self, event: &str) -> PyResult<()> {
        self.sfp_only("on_event")?;
        crate::imports::state_machine();
        let current = self.get_state()?;
        let Some((_, transitions)) = state_def(current) else {
            return Err(PyErr::runtime_error(format!("Unknown state {current}")));
        };
        let next = match transitions.iter().find(|(e, _, _)| *e == event) {
            None => {
                utils::log_error(&format!("{event} is not defined in state {current}"));
                current
            }
            Some((_, next, action)) => {
                if let Some(a) = action {
                    self.on_action(a)?;
                }
                next
            }
        };
        let Some((entry, _)) = state_def(next) else {
            return Err(PyErr::runtime_error(format!("Unknown next state {next}")));
        };
        if next != current {
            self.change_state(next);
            if let Some(a) = entry {
                self.on_action(a)?;
            }
        }
        Ok(())
    }

    fn on_action(&self, action: &str) -> PyResult<()> {
        match action {
            ACTION_ON_START => self.action_on_start(),
            ACTION_FCP_ON_START => self.action_fcp_on_start(),
            ACTION_ON_RESET => {
                utils::log_info(&format!("SFP {} is scheduled to wait for resetting done", self.sdk_index));
                wait_sfp_ready_task::get().schedule_wait(self.sdk_index as u64);
                Ok(())
            }
            ACTION_ON_POWERED => self.action_on_powered(),
            ACTION_ON_SW_CONTROL => self.action_on_sw_control(),
            ACTION_ON_FW_CONTROL => {
                if self.read_attr("control") != SFP_FW_CONTROL {
                    utils::log_info(&format!("SFP {} is set to firmware control", self.sdk_index));
                    self.write_attr("control", SFP_FW_CONTROL)?;
                }
                Ok(())
            }
            ACTION_ON_CANCEL_WAIT => {
                wait_sfp_ready_task::get().cancel_wait(self.sdk_index as u64);
                Ok(())
            }
            ACTION_ON_POWER_LIMIT_ERROR => {
                utils::log_info(&format!("SFP {} is powered off due to exceeding power limit", self.sdk_index));
                self.write_attr("power_on", 0)?;
                self.write_attr("hw_reset", 0)
            }
            other => Err(PyErr::key_error(other)),
        }
    }

    /// `action_on_start`: firmware control already (a warm reboot), not
    /// present, not powered, in reset, or ready -- each its event.
    fn action_on_start(&self) -> PyResult<()> {
        let n = self.sdk_index;
        if self.read_attr("control") == SFP_FW_CONTROL {
            utils::log_info(&format!("SFP {n} is already FW control, probably in warm reboot"));
            return self.on_event(EVENT_FW_CONTROL);
        }
        if self.read_attr("hw_present") != 1 {
            utils::log_info(&format!("SFP {n} is not present"));
            return self.on_event(EVENT_NOT_PRESENT);
        }
        if self.read_attr("power_on") != 1 {
            utils::log_info(&format!("SFP {n} is not powered on"));
            self.write_attr("power_on", 1)?;
            self.write_attr("hw_reset", 1)?;
            return self.on_event(EVENT_RESET);
        }
        if self.read_attr("hw_reset") != 1 {
            utils::log_info(&format!("SFP {n} is in reset state"));
            self.write_attr("hw_reset", 1)?;
            return self.on_event(EVENT_RESET);
        }
        if !self.processing_insert_event.load(Ordering::Relaxed) {
            return self.on_event(EVENT_POWER_ON);
        }
        self.processing_insert_event.store(false, Ordering::Relaxed);
        utils::log_info(&format!("SFP {n} is processing insert event and needs to wait module ready"));
        self.on_event(EVENT_RESET)
    }

    /// `action_fcp_on_start`: `present` read as a truth value.
    fn action_fcp_on_start(&self) -> PyResult<()> {
        if self.read_attr("present") != 0 {
            self.on_event(EVENT_PRESENT)
        } else {
            self.on_event(EVENT_NOT_PRESENT)
        }
    }

    /// `action_on_powered`: power bad, or the control type the module's
    /// EEPROM decides -- which is sonic_xcvr's.
    fn action_on_powered(&self) -> PyResult<()> {
        if self.read_attr("power_good") != 1 {
            utils::log_error(&format!("SFP {} is not in power good state", self.sdk_index));
            return self.on_event(EVENT_POWER_BAD);
        }
        let control_type = self.determine_control_type()?;
        if control_type == SFP_SW_CONTROL {
            self.on_event(EVENT_SW_CONTROL)
        } else {
            self.on_event(EVENT_FW_CONTROL)
        }
    }

    /// `determine_control_type()`: `get_xcvr_api()` first.
    fn determine_control_type(&self) -> PyResult<i128> {
        Err(xcvr("get_xcvr_api"))
    }

    /// `action_on_sw_control`: power capability, I2C frequency and TX
    /// disable all read the module through sonic_xcvr.
    fn action_on_sw_control(&self) -> PyResult<()> {
        Err(xcvr("get_module_max_power"))
    }

    /// `in_stable_state()`: waiting on an SDK event.
    pub fn in_stable_state(&self) -> PyResult<bool> {
        Ok(matches!(
            self.get_state()?,
            STATE_NOT_PRESENT
                | STATE_SW_CONTROL
                | STATE_FW_CONTROL
                | STATE_POWER_BAD
                | STATE_POWER_LIMIT_ERROR
                | STATE_FCP_NOT_PRESENT
                | STATE_FCP_PRESENT
        ))
    }

    /// The attributes polled in the current state.
    fn poll_types(&self) -> PyResult<&'static [&'static str]> {
        Ok(match self.get_state()? {
            STATE_FW_CONTROL | STATE_FCP_NOT_PRESENT | STATE_FCP_PRESENT => &["present"],
            _ => &["hw_present", "power_good"],
        })
    }

    /// `get_fds_for_poling()`.
    pub fn get_fds_for_poling(&self) -> PyResult<PollFds> {
        self.sfp_only("get_fds_for_poling")?;
        let mut out = Vec::new();
        for t in self.poll_types()? {
            out.push((*t, self.get_fd(t)?));
        }
        Ok(out)
    }

    /// `fill_change_event(port_dict)`.
    pub fn fill_change_event(&self, port_dict: &mut PortDict) -> PyResult<()> {
        let key = self.sdk_index + 1;
        match self.get_state()? {
            STATE_NOT_PRESENT | STATE_FCP_NOT_PRESENT => port_dict.set(key, SFP_STATUS_REMOVED.to_string()),
            STATE_SW_CONTROL | STATE_FW_CONTROL | STATE_FCP_PRESENT => port_dict.set(key, SFP_STATUS_INSERTED.to_string()),
            STATE_POWER_BAD | STATE_POWER_LIMIT_ERROR => {
                port_dict.set(key, (SFP_ERROR_BIT_POWER_BUDGET_EXCEEDED | SFP_STATUS_BIT_INSERTED).to_string())
            }
            _ => {}
        }
        Ok(())
    }

    /// `refresh_poll_obj(poll_obj, all_registered_fds)`: poll what the new
    /// state polls; drop -- and close -- what it no longer does.
    pub fn refresh_poll_obj(&self, poll: &mut PollSet, registered: &mut Registered) -> PyResult<()> {
        self.sfp_only("refresh_poll_obj")?;
        let mut current: Vec<(&'static str, i32)> = registered
            .iter()
            .filter(|(_, r)| r.sdk_index == self.sdk_index)
            .map(|(fileno, r)| (r.fd_type, *fileno))
            .collect();
        for t in self.poll_types()? {
            match current.iter().position(|(ty, _)| ty == t) {
                None => {
                    let Some(fd) = self.get_fd(t)? else {
                        // `poll_obj.register(None, ...)`
                        return Err(PyErr::type_error(
                            "argument must be an int, or have a fileno() method.",
                        ));
                    };
                    poll.register(fd.fileno(), POLLERR_PRI);
                    registered.insert(fd.fileno(), Reg { sdk_index: self.sdk_index, fd, fd_type: t });
                }
                Some(i) => {
                    current.remove(i);
                }
            }
        }
        for (_, fileno) in current {
            poll.unregister(fileno)?;
            registered.remove(fileno);
        }
        Ok(())
    }

    /// `is_dummy_event(fd_type, fd_value)`: the event the SDK sends for a
    /// state the module is already in.
    pub fn is_dummy_event(&self, fd_type: &str, fd_value: i128) -> PyResult<bool> {
        let state = self.get_state()?;
        Ok(match fd_type {
            "hw_present" | "present" => match fd_value {
                1 => matches!(
                    state,
                    STATE_SW_CONTROL | STATE_FW_CONTROL | STATE_POWER_BAD | STATE_POWER_LIMIT_ERROR | STATE_FCP_PRESENT
                ),
                0 => matches!(state, STATE_NOT_PRESENT | STATE_FCP_NOT_PRESENT),
                _ => false,
            },
            "power_good" => {
                if fd_value == 1 {
                    matches!(state, STATE_SW_CONTROL | STATE_NOT_PRESENT | STATE_RESETTING)
                } else {
                    matches!(state, STATE_POWER_BAD | STATE_POWER_LIMIT_ERROR | STATE_NOT_PRESENT)
                }
            }
            _ => false,
        })
    }
}

/// `SFP.wait_sfp_eeprom_ready(sfp_list, wait_time)`: up to `wait_time`
/// seconds, in steps of 0.1 the way Python counts them, for the modules
/// under firmware control to answer an EEPROM read.
pub fn wait_sfp_eeprom_ready(sfps: &[Arc<Sfp>], wait_time: f64) -> PyResult<()> {
    let mut not_ready: Vec<Arc<Sfp>> = sfps.to_vec();
    let mut wait_time = wait_time;
    while wait_time > 0.0 {
        let mut still = Vec::new();
        for s in not_ready {
            if s.get_state()? == STATE_FW_CONTROL && s.read_eeprom(0, 2, false)?.is_none() {
                still.push(s);
            }
        }
        not_ready = still;
        if not_ready.is_empty() {
            return Ok(());
        }
        std::thread::sleep(Duration::from_secs_f64(0.1));
        wait_time -= 0.1;
    }
    for s in &not_ready {
        utils::log_error(&format!("SFP {} eeprom is not ready", s.sdk_index));
    }
    Ok(())
}

/// `SFP.initialize_sfp_modules(sfp_list)`: start every module's state
/// machine; give the ones that were reset their 3.5 seconds; report where
/// each ended up.  A ready index picks `sfp_list[index]` -- the position in
/// this list, as in Python.
pub fn initialize_sfp_modules(sfps: &[Arc<Sfp>]) -> PyResult<()> {
    let task = wait_sfp_ready_task::get();
    task.start_once()?;
    for s in sfps {
        s.on_event(EVENT_START)?;
    }
    if !task.empty() {
        while !task.is_alive() {}
        let max_wait_time = 3.5;
        let begin = std::time::Instant::now();
        loop {
            for i in task.get_ready_set() {
                let s = sfps
                    .get(i as usize)
                    .ok_or_else(|| PyErr::index_error("list index out of range"))?;
                s.on_event(EVENT_RESET_DONE)?;
            }
            if begin.elapsed().as_secs_f64() < max_wait_time {
                std::thread::sleep(Duration::from_secs_f64(0.5));
            } else {
                break;
            }
        }
    }
    for (index, s) in sfps.iter().enumerate() {
        let state = s.get_state()?;
        if !s.in_stable_state()? {
            utils::log_error(&format!("SFP {index} is not in stable state after initializing, state={state}"));
        }
        utils::log_notice(&format!("SFP {index} is in state {state} after module initialization"));
    }
    wait_sfp_eeprom_ready(sfps, 2.0)
}

/// A registered fd: `(sdk_index, fd, fd_type)` (legacy mode has no type).
pub struct Reg {
    pub sdk_index: i128,
    pub fd: SysFile,
    pub fd_type: &'static str,
}

/// `registered_fds`: by file number, in insertion order.
#[derive(Default)]
pub struct Registered(Vec<(i32, Reg)>);

impl Registered {
    pub fn insert(&mut self, fileno: i32, r: Reg) {
        match self.0.iter_mut().find(|(f, _)| *f == fileno) {
            Some(e) => e.1 = r,
            None => self.0.push((fileno, r)),
        }
    }
    pub fn remove(&mut self, fileno: i32) -> Option<Reg> {
        let i = self.0.iter().position(|(f, _)| *f == fileno)?;
        Some(self.0.remove(i).1)
    }
    pub fn get_mut(&mut self, fileno: i32) -> Option<&mut Reg> {
        self.0.iter_mut().find(|(f, _)| *f == fileno).map(|(_, r)| r)
    }
    pub fn iter(&self) -> impl Iterator<Item = &(i32, Reg)> {
        self.0.iter()
    }
    #[cfg(test)]
    pub fn retain(&mut self, f: impl FnMut(&(i32, Reg)) -> bool) {
        self.0.retain(f)
    }
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
    /// `str(self.registered_fds)`, for the one error that shows it.
    pub fn repr(&self) -> String {
        let items: Vec<String> = self
            .0
            .iter()
            .map(|(f, r)| {
                let fd = format!(
                    "<_io.TextIOWrapper name='{}' mode='r' encoding='{}'>",
                    r.fd.name,
                    match platform_base::py::codec::locale() {
                        platform_base::py::codec::Codec::Ascii => "ANSI_X3.4-1968",
                        platform_base::py::codec::Codec::Latin1 => "ISO-8859-1",
                        platform_base::py::codec::Codec::Utf8 => "UTF-8",
                    }
                );
                if r.fd_type.is_empty() {
                    format!("{f}: ({}, {fd})", r.sdk_index)
                } else {
                    format!("{f}: ({}, {fd}, '{}')", r.sdk_index, r.fd_type)
                }
            })
            .collect();
        format!("{{{}}}", items.join(", "))
    }
}

/// `port_dict` / `error_dict`: a dict of port index to status, in insertion
/// order; ASIC events add string keys.
#[derive(Default, Clone, Debug)]
pub struct PortDict(pub Vec<(PyVal, PyVal)>);

impl PortDict {
    pub fn set(&mut self, key: i128, value: String) {
        self.set_val(PyVal::Int(key), PyVal::Str(value));
    }
    pub fn set_val(&mut self, key: PyVal, value: PyVal) {
        match self.0.iter_mut().find(|(k, _)| k.py_eq(&key)) {
            Some(e) => e.1 = value,
            None => self.0.push((key, value)),
        }
    }
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
    pub fn to_pyval(&self) -> PyVal {
        PyVal::Dict(self.0.clone())
    }
    pub fn repr(&self) -> String {
        self.to_pyval().py_repr()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sfp::test_support::{eeprom, module, module_path};
    use crate::testing::Root;
    use crate::wait_sfp_ready_task::WaitSfpReadyTask;

    fn port(index: i128) -> Arc<Sfp> {
        Sfp::new(Kind::Sfp, index, "asic0").unwrap()
    }

    fn set_state(s: &Sfp, state: &'static str) {
        *s.state.lock().unwrap() = state;
    }

    fn attr(root: &Root, index: usize, name: &str) -> String {
        root.read(&module_path(index, name)).unwrap_or_default()
    }

    /// The process's wait task, emptied: these tests hold the root's lock
    /// while they use it.
    fn task() -> Arc<WaitSfpReadyTask> {
        let t = wait_sfp_ready_task::get();
        t.reset();
        t
    }

    fn not_ported(e: PyErr, what: &str) {
        assert_eq!(e.exc.name(), "RuntimeError");
        assert_eq!(e.str(), format!("not ported to the native platform API: SFP.{what}: needs sonic_xcvr's XcvrApiFactory"));
    }

    // -- test_sfp_sm.py --------------------------------------------------------

    #[test]
    fn a_module_under_firmware_control_stays_so_on_a_warm_reboot() {
        let root = Root::new();
        module(&root, 0, &[("control", "0")]);
        let s = port(0);
        assert_eq!(s.get_state().unwrap(), STATE_DOWN);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FW_CONTROL);
        assert_eq!(attr(&root, 0, "control"), "0");
    }

    #[test]
    fn a_module_not_there_is_not_present() {
        let root = Root::new();
        module(&root, 0, &[("control", "1"), ("hw_present", "0")]);
        let s = port(0);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_NOT_PRESENT);
        assert!(s.in_stable_state().unwrap());
        // Coming back is a new start.
        module(&root, 0, &[("hw_present", "1"), ("power_on", "1"), ("hw_reset", "1"), ("power_good", "0")]);
        s.on_event(EVENT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_POWER_BAD);
    }

    #[test]
    fn a_module_powered_off_is_powered_reset_and_waited_for() {
        let root = Root::new();
        let task = task();
        module(&root, 0, &[("control", "1"), ("hw_present", "1"), ("power_on", "0"), ("hw_reset", "0")]);
        let s = port(0);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_RESETTING);
        assert!(!s.in_stable_state().unwrap());
        assert_eq!((attr(&root, 0, "power_on"), attr(&root, 0, "hw_reset")), ("1".into(), "1".into()));
        assert_eq!(task.waiting(), [0]);
        task.reset();
    }

    #[test]
    fn a_module_in_reset_is_released_and_a_removal_cancels_its_wait() {
        let root = Root::new();
        let task = task();
        module(&root, 0, &[("control", "1"), ("hw_present", "1"), ("power_on", "1"), ("hw_reset", "0")]);
        let s = port(0);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_RESETTING);
        assert_eq!(attr(&root, 0, "hw_reset"), "1");
        assert_eq!(task.waiting(), [0]);
        s.on_event(EVENT_NOT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_NOT_PRESENT);
        assert!(task.waiting().is_empty());
    }

    #[test]
    fn a_module_inserted_while_ready_still_waits_for_its_firmware() {
        let root = Root::new();
        let task = task();
        module(&root, 2, &[("control", "1"), ("hw_present", "1"), ("power_on", "1"), ("hw_reset", "1")]);
        let s = port(2);
        s.processing_insert_event.store(true, Ordering::Relaxed);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_RESETTING);
        assert!(!s.processing_insert_event.load(Ordering::Relaxed));
        assert_eq!(task.waiting(), [2]);
        task.reset();
    }

    #[test]
    fn a_powered_module_is_power_bad_or_needs_sonic_xcvr() {
        let root = Root::new();
        module(&root, 0, &[("control", "1"), ("hw_present", "1"), ("power_on", "1"), ("hw_reset", "1"), ("power_good", "0")]);
        let s = port(0);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_POWER_BAD);
        s.on_event(EVENT_NOT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_NOT_PRESENT);
        // Power good: deciding software or firmware control reads the
        // module through sonic_xcvr.
        module(&root, 0, &[("power_good", "1")]);
        let s = port(0);
        not_ported(s.on_event(EVENT_START).unwrap_err(), "get_xcvr_api");
        assert_eq!(s.get_state().unwrap(), STATE_POWERED_ON);
        // So does a reset that is done.
        set_state(&s, STATE_RESETTING);
        not_ported(s.on_event(EVENT_RESET_DONE).unwrap_err(), "get_xcvr_api");
        // And software control's power, frequency and TX checks.
        set_state(&s, STATE_POWERED_ON);
        not_ported(s.on_event(EVENT_SW_CONTROL).unwrap_err(), "get_module_max_power");
        assert_eq!(s.get_state().unwrap(), STATE_SW_CONTROL);
    }

    #[test]
    fn firmware_control_sets_the_control_attribute_once() {
        let root = Root::new();
        module(&root, 0, &[("control", "1")]);
        let s = port(0);
        set_state(&s, STATE_POWERED_ON);
        s.on_event(EVENT_FW_CONTROL).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FW_CONTROL);
        assert_eq!(attr(&root, 0, "control"), "0");
        // Already firmware control: nothing written.
        root.write(&module_path(0, "control"), "0\n");
        set_state(&s, STATE_POWERED_ON);
        s.on_event(EVENT_FW_CONTROL).unwrap();
        assert_eq!(attr(&root, 0, "control"), "0\n");
    }

    #[test]
    fn exceeding_the_power_limit_powers_the_module_off() {
        let root = Root::new();
        module(&root, 0, &[("power_on", "1"), ("hw_reset", "1")]);
        let s = port(0);
        set_state(&s, STATE_SW_CONTROL);
        s.on_event(EVENT_POWER_LIMIT_EXCEED).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_POWER_LIMIT_ERROR);
        assert_eq!((attr(&root, 0, "power_on"), attr(&root, 0, "hw_reset")), ("0".into(), "0".into()));
        s.on_event(EVENT_NOT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_NOT_PRESENT);
    }

    #[test]
    fn an_always_firmware_controlled_port_follows_present() {
        if crate::testing::platform_from_env() {
            return;
        }
        let root = Root::new();
        root.write("/host/machine.conf", "onie_platform=x86_64-nvidia_sn5600-r0\n");
        module(&root, 64, &[("present", "1")]);
        let s = port(64);
        assert_eq!(s.get_state().unwrap(), STATE_FCP_DOWN);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FCP_PRESENT);
        s.on_event(EVENT_NOT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FCP_NOT_PRESENT);
        s.on_event(EVENT_PRESENT).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FCP_PRESENT);
        module(&root, 64, &[("present", "0")]);
        let s = port(64);
        s.on_event(EVENT_START).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_FCP_NOT_PRESENT);
        // Port 63 is not on the list.
        assert_eq!(port(63).get_state().unwrap(), STATE_DOWN);
    }

    // -- test_statemachine.py --------------------------------------------------

    #[test]
    fn an_event_a_state_does_not_know_is_ignored_and_an_unknown_state_raises() {
        let _root = Root::new();
        let s = port(0);
        s.on_event(EVENT_RESET_DONE).unwrap();
        assert_eq!(s.get_state().unwrap(), STATE_DOWN);
        set_state(&s, "Bogus");
        let e = s.on_event(EVENT_START).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("RuntimeError", "Unknown state Bogus".to_string()));
        assert!(state_def("Bogus").is_none());
        assert_eq!(s.on_action("No Such Action").unwrap_err().exc.name(), "KeyError");
    }

    #[test]
    fn an_rj45_port_has_no_state_machine() {
        let _root = Root::new();
        let p = Sfp::new(Kind::Rj45, 1, "asic0").unwrap();
        for e in [p.get_state().unwrap_err(), p.on_event(EVENT_START).unwrap_err(), p.get_fds_for_poling().err().unwrap()] {
            assert_eq!(e.exc.name(), "AttributeError");
        }
        assert_eq!(p.get_state().unwrap_err().str(), "'RJ45Port' object has no attribute 'state'");
        let mut poll = PollSet::default();
        let mut reg = Registered::default();
        assert_eq!(
            p.refresh_poll_obj(&mut poll, &mut reg).unwrap_err().str(),
            "'RJ45Port' object has no attribute 'refresh_poll_obj'"
        );
    }

    // -- status and errors -----------------------------------------------------

    #[test]
    fn the_sdk_status_maps_to_sonic_s_and_an_unknown_one_is_a_key_error() {
        let root = Root::new();
        let s = port(0);
        let r = Sfp::new(Kind::Rj45, 0, "asic0").unwrap();
        for (sdk, sonic, rj45) in [("1", "1", "1"), ("2", "0", "0"), ("3", "2", "2"), ("4", "0", "0"), ("5", "-1", "0")] {
            module(&root, 0, &[("status", sdk)]);
            assert_eq!(s.get_module_status().unwrap(), sonic, "{sdk}");
            assert_eq!(r.get_module_status().unwrap(), rj45, "{sdk}");
        }
        module(&root, 0, &[("status", "9")]);
        assert_eq!(s.get_module_status().unwrap_err().str(), "9");
        std::fs::remove_file(root.real(&module_path(0, "status"))).unwrap();
        let e = s.get_module_status().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("KeyError", "0".to_string()));
    }

    #[test]
    fn an_sdk_error_is_its_status_bits_and_a_vendor_error_its_description() {
        let root = Root::new();
        let s = port(0);
        let cases: [(&str, &str, Option<&str>); 11] = [
            ("0", "5", None),
            ("1", "65537", Some("Long range for non-Mellanox cable or module")),
            ("2", "11", None),
            ("3", "19", None),
            ("4", "131073", Some("Enforce part number list")),
            ("5", "35", None),
            ("6", "67", None),
            ("7", "131", None),
            ("8", "262145", Some("PMD type not enabled")),
            ("12", "524289", Some("PCIE system power slot exceeded")),
            ("9", "2", Some("Unknown error (9)")),
        ];
        for (code, bits, desc) in cases {
            module(&root, 0, &[("statuserror", code)]);
            assert_eq!(s.get_error_info_from_sdk_error_type(), (bits.to_string(), desc.map(str::to_string)), "{code}");
        }
        std::fs::remove_file(root.real(&module_path(0, "statuserror"))).unwrap();
        assert_eq!(s.get_error_info_from_sdk_error_type(), ("2".to_string(), Some("Unknown error (-1)".to_string())));
    }

    #[test]
    fn a_missing_attribute_is_no_fd_and_a_present_one_opens() {
        let root = Root::new();
        let s = port(5);
        assert!(s.get_fd("present").unwrap().is_none());
        module(&root, 5, &[("present", "1")]);
        let fd = s.get_fd("present").unwrap().unwrap();
        assert_eq!(fd.name, module_path(5, "present"));
        // Anything but a missing file raises.
        root.write("/sys/module/sx_core/asic0/module6", "not a directory");
        assert_eq!(port(6).get_fd("present").err().unwrap().exc.name(), "NotADirectoryError");
    }

    // -- polling ---------------------------------------------------------------

    #[test]
    fn each_state_polls_and_reports_what_python_s_does() {
        let root = Root::new();
        module(&root, 0, &[("present", "1"), ("hw_present", "1"), ("power_good", "1")]);
        let s = port(0);
        let types = |s: &Sfp| s.get_fds_for_poling().unwrap().into_iter().map(|(t, fd)| (t, fd.is_some())).collect::<Vec<_>>();
        assert_eq!(types(&s), [("hw_present", true), ("power_good", true)]);
        for st in [STATE_FW_CONTROL, STATE_FCP_NOT_PRESENT, STATE_FCP_PRESENT] {
            set_state(&s, st);
            assert_eq!(types(&s), [("present", true)]);
        }
        let report = |st: &'static str| {
            set_state(&s, st);
            let mut d = PortDict::default();
            s.fill_change_event(&mut d).unwrap();
            d.repr()
        };
        assert_eq!(report(STATE_NOT_PRESENT), "{1: '0'}");
        assert_eq!(report(STATE_FCP_NOT_PRESENT), "{1: '0'}");
        for st in [STATE_SW_CONTROL, STATE_FW_CONTROL, STATE_FCP_PRESENT] {
            assert_eq!(report(st), "{1: '1'}");
        }
        assert_eq!(report(STATE_POWER_BAD), "{1: '5'}");
        assert_eq!(report(STATE_POWER_LIMIT_ERROR), "{1: '5'}");
        assert_eq!(report(STATE_RESETTING), "{}");
        let stable = |st: &'static str| {
            set_state(&s, st);
            s.in_stable_state().unwrap()
        };
        for st in [STATE_DOWN, STATE_INIT, STATE_RESETTING, STATE_POWERED_ON, STATE_FCP_DOWN, STATE_FCP_INIT] {
            assert!(!stable(st), "{st}");
        }
    }

    #[test]
    fn a_dummy_event_repeats_the_state_the_module_is_in() {
        let _root = Root::new();
        let s = port(0);
        let dummy = |st: &'static str, ty: &str, v: i128| {
            set_state(&s, st);
            s.is_dummy_event(ty, v).unwrap()
        };
        for st in [STATE_SW_CONTROL, STATE_FW_CONTROL, STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_FCP_PRESENT] {
            assert!(dummy(st, "hw_present", 1) && dummy(st, "present", 1), "{st}");
        }
        assert!(!dummy(STATE_NOT_PRESENT, "hw_present", 1));
        assert!(dummy(STATE_NOT_PRESENT, "hw_present", 0) && dummy(STATE_FCP_NOT_PRESENT, "present", 0));
        assert!(!dummy(STATE_SW_CONTROL, "hw_present", 0));
        assert!(!dummy(STATE_SW_CONTROL, "hw_present", 2));
        for st in [STATE_SW_CONTROL, STATE_NOT_PRESENT, STATE_RESETTING] {
            assert!(dummy(st, "power_good", 1), "{st}");
        }
        assert!(!dummy(STATE_POWER_BAD, "power_good", 1));
        for st in [STATE_POWER_BAD, STATE_POWER_LIMIT_ERROR, STATE_NOT_PRESENT] {
            assert!(dummy(st, "power_good", 0), "{st}");
        }
        assert!(!dummy(STATE_SW_CONTROL, "power_good", 0));
        assert!(!dummy(STATE_SW_CONTROL, "other", 1));
    }

    #[test]
    fn refreshing_polls_what_the_new_state_polls_and_drops_the_rest() {
        let root = Root::new();
        module(&root, 0, &[("present", "1"), ("hw_present", "1"), ("power_good", "1")]);
        module(&root, 1, &[("present", "1")]);
        let (s, other) = (port(0), port(1));
        let mut poll = PollSet::default();
        let mut reg = Registered::default();
        assert!(reg.is_empty());
        // Another port's fd stays whatever this one does.
        set_state(&other, STATE_FW_CONTROL);
        other.refresh_poll_obj(&mut poll, &mut reg).unwrap();
        s.refresh_poll_obj(&mut poll, &mut reg).unwrap();
        let types = |reg: &Registered| reg.iter().map(|(_, r)| (r.sdk_index, r.fd_type)).collect::<Vec<_>>();
        assert_eq!(types(&reg), [(1, "present"), (0, "hw_present"), (0, "power_good")]);
        // Again in the same state: nothing changes.
        s.refresh_poll_obj(&mut poll, &mut reg).unwrap();
        assert_eq!(types(&reg).len(), 3);
        // Firmware control polls `present` only.
        set_state(&s, STATE_FW_CONTROL);
        s.refresh_poll_obj(&mut poll, &mut reg).unwrap();
        assert_eq!(types(&reg), [(1, "present"), (0, "present")]);
        // An attribute that is not there cannot be registered.
        set_state(&s, STATE_NOT_PRESENT);
        std::fs::remove_file(root.real(&module_path(0, "power_good"))).unwrap();
        let e = s.refresh_poll_obj(&mut poll, &mut reg).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("TypeError", "argument must be an int, or have a fileno() method.".to_string()));
        // The registered fds, as Python prints them.
        let enc = match platform_base::py::codec::locale() {
            platform_base::py::codec::Codec::Ascii => "ANSI_X3.4-1968",
            platform_base::py::codec::Codec::Latin1 => "ISO-8859-1",
            platform_base::py::codec::Codec::Utf8 => "UTF-8",
        };
        let first = reg.iter().next().map(|(f, _)| *f).unwrap();
        assert!(reg.repr().starts_with(&format!(
            "{{{first}: (1, <_io.TextIOWrapper name='{}' mode='r' encoding='{enc}'>, 'present')",
            module_path(1, "present")
        )));
        let mut legacy = Registered::default();
        let fd = SysFile::open(&module_path(1, "present")).unwrap();
        let fileno = fd.fileno();
        legacy.insert(fileno, Reg { sdk_index: 1, fd, fd_type: "" });
        // The same file number again replaces the entry, in its place.
        let again = SysFile::open(&module_path(1, "present")).unwrap();
        legacy.insert(fileno, Reg { sdk_index: 7, fd: again, fd_type: "" });
        assert_eq!(legacy.iter().map(|(_, r)| r.sdk_index).collect::<Vec<_>>(), [7]);
        assert!(legacy.repr().ends_with(&format!("encoding='{enc}'>)}}")));
        assert!(legacy.get_mut(fileno).is_some());
        legacy.retain(|_| false);
        assert!(legacy.is_empty() && legacy.remove(fileno).is_none());
    }

    #[test]
    fn a_port_dict_is_a_python_dict() {
        let mut d = PortDict::default();
        assert!(d.is_empty());
        d.set(3, "1".into());
        d.set_val(PyVal::str("2"), PyVal::str("0"));
        d.set(3, "0".into());
        assert_eq!(d.repr(), "{3: '0', '2': '0'}");
        assert_eq!(d.to_pyval().py_repr(), d.repr());
    }

    // -- initialization ----------------------------------------------------------

    #[test]
    fn eeprom_readiness_is_waited_for_under_firmware_control_only() {
        let root = Root::new();
        let (a, b) = (port(0), port(1));
        set_state(&a, STATE_FW_CONTROL);
        set_state(&b, STATE_SW_CONTROL);
        // Neither is waited for: b is not under firmware control, a reads.
        eeprom(&root, 0);
        let begin = std::time::Instant::now();
        wait_sfp_eeprom_ready(&[a.clone(), b.clone()], 2.0).unwrap();
        assert!(begin.elapsed() < Duration::from_millis(500));
        // a cannot be read: waited for, in steps of 0.1, then reported.
        std::fs::remove_dir_all(root.real(&module_path(0, "eeprom"))).unwrap();
        let begin = std::time::Instant::now();
        wait_sfp_eeprom_ready(&[a.clone(), b.clone()], 0.3).unwrap();
        assert!(begin.elapsed() >= Duration::from_millis(250));
        // No time at all: every port is reported.
        wait_sfp_eeprom_ready(&[a, b], 0.0).unwrap();
    }

    #[test]
    fn initializing_starts_every_module_and_waits_out_a_reset() {
        let root = Root::new();
        let task = task();
        // 0: firmware control, readable.  1: not present.  2: powered off,
        // so reset; when its wait is over its power is bad.
        module(&root, 0, &[("control", "0")]);
        eeprom(&root, 0);
        module(&root, 1, &[("control", "1"), ("hw_present", "0")]);
        module(&root, 2, &[("control", "1"), ("hw_present", "1"), ("power_on", "0"), ("power_good", "0")]);
        let sfps = [port(0), port(1), port(2)];
        // Ready already, as if its 3 seconds were over: the task thread's
        // one-second steps could otherwise miss the 3.5 the loop allows.
        task.mark_ready(2);
        let begin = std::time::Instant::now();
        initialize_sfp_modules(&sfps).unwrap();
        assert!(begin.elapsed() >= Duration::from_secs_f64(3.5));
        let states: Vec<&str> = sfps.iter().map(|s| s.get_state().unwrap()).collect();
        assert_eq!(states, [STATE_FW_CONTROL, STATE_NOT_PRESENT, STATE_POWER_BAD]);
        task.reset();
        // Without a reset there is nothing to wait for; a module START does
        // not move is reported unstable.
        let unstable = port(1);
        set_state(&unstable, STATE_POWERED_ON);
        let sfps = [port(0), unstable];
        let begin = std::time::Instant::now();
        initialize_sfp_modules(&sfps).unwrap();
        assert!(begin.elapsed() < Duration::from_secs(2));
        assert_eq!(sfps[1].get_state().unwrap(), STATE_POWERED_ON);
        // A ready port past the list's end: Python's IndexError.
        let sfps = [port(1)];
        task.mark_ready(4);
        module(&root, 3, &[("control", "1"), ("hw_present", "1"), ("power_on", "1"), ("hw_reset", "0")]);
        let resetting = port(3);
        resetting.on_event(EVENT_START).unwrap();
        let e = initialize_sfp_modules(&sfps).unwrap_err();
        assert_eq!(e.str(), "list index out of range");
        task.reset();
    }
}
