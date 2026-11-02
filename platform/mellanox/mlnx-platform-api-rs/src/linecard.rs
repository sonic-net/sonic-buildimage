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



//! `sonic_platform/module.py`'s `Module`: a line card of a modular chassis.
//!
//! Its thermals and SFPs are built while the card is powered, and dropped
//! when its power state changes or -- powered -- when the slot's sequence
//! number in STATE_DB does, which is the card having been swapped.  The
//! sequence number comes over redis-py's own connection, TCP to
//! localhost:6379, not the database config.

use std::sync::{Arc, Mutex, MutexGuard};

use platform_base::db::Db;
use platform_base::py::{PyResult, PyVal};
use platform_base::{BaseClass, PyObject};

use crate::device_data;
use crate::sfp::Sfp;
use crate::utils::{self, Log};
use crate::vpd_parser::VpdParser;

const STATE_ACTIVATED: i128 = 1;
const STATE_DEACTIVATED: i128 = 0;
const STATE_DB: i64 = 6;

/// `Module.redis_client`: one per process, connected on first use; a
/// failed command leaves the next one to connect again, as redis-py's pool.
static REDIS: Mutex<Option<Db>> = Mutex::new(None);

struct State {
    /// `self.seq_no`: 0 until read, then the string STATE_DB holds -- or 0
    /// again when it cannot be read.
    seq_no: PyVal,
    current_state: i128,
    thermal_list: Vec<PyVal>,
    sfp_list: Vec<Option<Arc<Sfp>>>,
    sfp_initialized_count: usize,
    sfp_count: i128,
}

pub struct LinecardModule {
    slot_id: i128,
    /// `VpdParser('/run/hw-management/lc{}/eeprom/vpd_parsed')`: the slot is
    /// never formatted in, so the file is never there.
    vpd_parser: VpdParser,
    /// `self.lock`, with the state it guards.
    state: Mutex<State>,
}

impl LinecardModule {
    /// `Module(slot_id)`.
    pub fn new(slot_id: i128) -> LinecardModule {
        crate::module::import();
        LinecardModule {
            slot_id,
            vpd_parser: VpdParser::new("/run/hw-management/lc{}/eeprom/vpd_parsed"),
            state: Mutex::new(State {
                seq_no: PyVal::Int(0),
                current_state: STATE_DEACTIVATED,
                thermal_list: Vec::new(),
                sfp_list: Vec::new(),
                sfp_initialized_count: 0,
                sfp_count: 0,
            }),
        }
    }

    fn name(&self) -> String {
        format!("LINE-CARD{}", self.slot_id)
    }

    fn lock(&self) -> MutexGuard<'_, State> {
        utils::lock(&self.state)
    }

    /// `_get_seq_no()`: the stripped string, or 0 for anything that fails --
    /// a missing key included, whose None has no `decode`.
    fn get_seq_no(&self) -> PyVal {
        let key = format!("MODULAR_CHASSIS_SLOT|{}", self.slot_id);
        let mut conn = utils::lock(&REDIS);
        if conn.is_none() {
            *conn = Db::connect_addr("localhost", 6379, STATE_DB).ok();
        }
        let r = conn.as_ref().map(|db| db.hget(&key, "seq_no"));
        match r {
            Some(Ok(Some(v))) => PyVal::str(platform_base::py::strip(&v)),
            Some(Ok(None)) => PyVal::Int(0),
            Some(Err(_)) => {
                *conn = None;
                PyVal::Int(0)
            }
            None => PyVal::Int(0),
        }
    }

    /// `_check_state()`.
    fn check_state(&self, st: &mut State) {
        let seq_no = self.get_seq_no();
        let state = utils::read_int(&format!("/run/hw-management/system/lc{}_powered", self.slot_id), 0, Log::Off);
        // A new power state, or -- powered -- a new card in the slot.
        if state != st.current_state || (!seq_no.py_eq(&st.seq_no) && state == STATE_ACTIVATED) {
            Self::re_init(st);
        }
        st.current_state = state;
        st.seq_no = seq_no;
    }

    /// `_re_init()`.  (It clears `_sfp_count`, which nothing reads; the
    /// count kept is `sfp_count`.)
    fn re_init(st: &mut State) {
        st.thermal_list = Vec::new();
        st.sfp_list = Vec::new();
    }

    /// `initialize_thermals()`.
    fn initialize_thermals(&self, st: &mut State) {
        self.check_state(st);
        if st.current_state == STATE_ACTIVATED && st.thermal_list.is_empty() {
            crate::imports::thermal();
            st.thermal_list = crate::thermal::initialize_linecard_thermals(&self.name(), self.slot_id);
        }
    }

    /// `get_num_sfps()`.
    fn num_sfps(&self, st: &mut State) -> i128 {
        if st.sfp_count == 0 {
            st.sfp_count = device_data::get_linecard_sfp_count(self.slot_id);
        }
        st.sfp_count
    }

    /// `get_num_sfps()`, for ModularChassis's sum.
    pub fn get_num_sfps(&self) -> PyResult<i128> {
        let mut st = self.lock();
        Ok(self.num_sfps(&mut st))
    }

    fn create_sfp_object(&self, st: &State, index: i128) -> PyResult<Arc<Sfp>> {
        crate::imports::sfp();
        Sfp::new_linecard(index, self.slot_id, st.sfp_count, &self.name())
    }

    /// `initialize_sfps()`.
    fn initialize_sfps(&self, st: &mut State) -> PyResult<()> {
        self.check_state(st);
        if st.current_state != STATE_ACTIVATED {
            return Ok(());
        }
        if st.sfp_list.is_empty() {
            let count = self.num_sfps(st);
            for index in 0..count.max(0) {
                let s = self.create_sfp_object(st, index)?;
                st.sfp_list.push(Some(s));
            }
            st.sfp_initialized_count = usize::try_from(count).unwrap_or(0);
        } else if st.sfp_initialized_count != st.sfp_list.len() {
            for index in 0..st.sfp_list.len() {
                if st.sfp_list[index].is_none() {
                    let s = self.create_sfp_object(st, index as i128)?;
                    st.sfp_list[index] = Some(s);
                }
            }
            st.sfp_initialized_count = st.sfp_list.len();
        }
        Ok(())
    }

    /// `get_all_sfps()`.
    pub fn get_all_sfps(&self) -> PyResult<Vec<Option<Arc<Sfp>>>> {
        let mut st = self.lock();
        self.initialize_sfps(&mut st)?;
        Ok(st.sfp_list.clone())
    }

    fn sys_int(&self, what: &str) -> i128 {
        utils::read_int(&format!("/run/hw-management/system/lc{}_{what}", self.slot_id), 0, Log::Error)
    }
}

impl PyObject for LinecardModule {
    fn py_class(&self) -> &'static str {
        "Module"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ModuleBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name()))
    }
    fn get_model(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.vpd_parser.get_model()))
    }
    fn get_serial(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.vpd_parser.get_serial()))
    }
    fn get_revision(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.vpd_parser.get_revision()))
    }
    fn get_type(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("LINE-CARD"))
    }
    fn get_slot(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.slot_id))
    }
    fn get_presence(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.sys_int("present") == 1))
    }
    fn get_position_in_parent(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.slot_id))
    }
    fn is_replaceable(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(true))
    }
    /// Active, else present, else empty -- each its own read, each logging
    /// its own failure.
    fn get_oper_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(if self.sys_int("active") == 1 {
            "Online"
        } else if self.sys_int("present") == 1 {
            "Present"
        } else if self.sys_int("present") == 0 {
            "Empty"
        } else {
            "Fault"
        }))
    }
    fn get_all_thermals(&self) -> PyResult<PyVal> {
        let mut st = self.lock();
        self.initialize_thermals(&mut st);
        Ok(PyVal::List(st.thermal_list.clone()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_d::{platform, platform_settable};
    use crate::testing::Root;
    use platform_base::testing::FakeRedis;
    use std::io::{Read, Write};

    const SYS: &str = "/run/hw-management/system";

    fn card(root: &Root) -> LinecardModule {
        platform(root, "x86_64-nvidia_sn4800-r0", r#"{"chassis": {}}"#);
        LinecardModule::new(1)
    }

    /// The line card's STATE_DB connection, to the fake server.
    fn redis_for_cards() -> &'static FakeRedis {
        let redis = FakeRedis::get();
        *REDIS.lock().unwrap() = Some(Db::connect_addr("127.0.0.1", redis.port(), STATE_DB).unwrap());
        redis
    }

    fn names(v: &[PyVal]) -> Vec<String> {
        v.iter()
            .map(|t| match t {
                PyVal::Obj(o) => o.get_name().map(|n| n.py_str()).unwrap_or_default(),
                _ => String::new(),
            })
            .collect()
    }

    #[test]
    fn a_line_card_reads_its_slot_files() {
        let root = Root::new();
        let lc = card(&root);
        assert_eq!(
            [lc.get_name(), lc.get_type(), lc.get_slot(), lc.get_position_in_parent(), lc.is_replaceable()].map(|v| v.unwrap().py_repr()),
            ["'LINE-CARD1'", "'LINE-CARD'", "1", "1", "True"]
        );
        assert_eq!((lc.py_class(), lc.base_class()), ("Module", BaseClass::ModuleBase));
        // The VPD path keeps its `{}`: no file, and N/A.
        assert_eq!(lc.get_model().unwrap().py_repr(), "'N/A'");
        root.write("/run/hw-management/lc{}/eeprom/vpd_parsed", "PN_VPD_FIELD: MSN4800-LC\nSN_VPD_FIELD: SN1\nREV_VPD_FIELD: A1\n");
        assert_eq!(
            [lc.get_model(), lc.get_serial(), lc.get_revision()].map(|v| v.unwrap().py_repr()),
            ["'MSN4800-LC'", "'SN1'", "'A1'"]
        );
        let status = |active: &str, present: &str| {
            root.write(&format!("{SYS}/lc1_active"), active);
            root.write(&format!("{SYS}/lc1_present"), present);
            lc.get_oper_status().unwrap().py_str()
        };
        assert_eq!(status("1", "0"), "Online");
        assert_eq!(status("0", "1"), "Present");
        assert_eq!(status("0", "0"), "Empty");
        assert_eq!(status("0", "2"), "Fault");
        assert_eq!(lc.get_presence().unwrap().py_repr(), "False");
        root.write(&format!("{SYS}/lc1_present"), "1");
        assert_eq!(lc.get_presence().unwrap().py_repr(), "True");
    }

    #[test]
    fn thermals_follow_the_power_state_and_the_card_in_the_slot() {
        let root = Root::new();
        let lc = card(&root);
        let redis = redis_for_cards();
        redis.hset(STATE_DB, "MODULAR_CHASSIS_SLOT|1", &[("seq_no", " 7 ")]);
        // Unpowered: none.
        assert!(lc.get_all_thermals().unwrap().iter_items().unwrap().is_empty());
        root.write(&format!("{SYS}/lc1_powered"), "1");
        root.write("/run/hw-management/lc1/config/gearbox_counter", "2");
        let t = lc.get_all_thermals().unwrap().iter_items().unwrap();
        assert_eq!(names(&t), ["LINE-CARD1 Gearbox 1 Temp", "LINE-CARD1 Gearbox 2 Temp"]);
        assert_eq!(lc.lock().seq_no.py_repr(), "'7'");
        // The same card: kept, though there are more gearboxes now.
        root.write("/run/hw-management/lc1/config/gearbox_counter", "3");
        assert_eq!(lc.get_all_thermals().unwrap().iter_items().unwrap().len(), 2);
        // Another card in the slot: built again.
        redis.hset(STATE_DB, "MODULAR_CHASSIS_SLOT|1", &[("seq_no", "8")]);
        assert_eq!(lc.get_all_thermals().unwrap().iter_items().unwrap().len(), 3);
        // No seq_no at all reads as 0 -- a change -- and powered off clears.
        redis.hdel(STATE_DB, "MODULAR_CHASSIS_SLOT|1", "seq_no");
        assert_eq!(lc.get_all_thermals().unwrap().iter_items().unwrap().len(), 3);
        assert_eq!(lc.lock().seq_no.py_repr(), "0");
        root.write(&format!("{SYS}/lc1_powered"), "0");
        assert!(lc.get_all_thermals().unwrap().iter_items().unwrap().is_empty());
    }

    #[test]
    fn a_failed_seq_no_read_is_0_and_drops_the_connection() {
        let root = Root::new();
        let lc = card(&root);
        // A server that answers SELECT and fails the HGET.
        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let port = listener.local_addr().unwrap().port();
        let server = std::thread::spawn(move || {
            let (mut s, _) = listener.accept().unwrap();
            let mut buf = [0u8; 512];
            let _ = s.read(&mut buf);
            s.write_all(b"+OK\r\n").unwrap();
            let _ = s.read(&mut buf);
            s.write_all(b"-ERR boom\r\n").unwrap();
        });
        *REDIS.lock().unwrap() = Some(Db::connect_addr("127.0.0.1", port, STATE_DB).unwrap());
        assert_eq!(lc.get_seq_no().py_repr(), "0");
        assert!(REDIS.lock().unwrap().is_none());
        server.join().unwrap();
        drop(root);
    }

    #[test]
    fn sfps_are_built_while_the_card_is_powered() {
        if !platform_settable() {
            return;
        }
        let root = Root::new();
        let lc = card(&root);
        let redis = redis_for_cards();
        redis.hset(STATE_DB, "MODULAR_CHASSIS_SLOT|1", &[("seq_no", "1")]);
        root.write("/run/hw-management/config/hotplug_linecards", "8");
        root.write("/run/hw-management/lc1/config/module_counter", "2");
        assert!(lc.get_all_sfps().unwrap().is_empty());
        root.write(&format!("{SYS}/lc1_powered"), "1");
        let sfps = lc.get_all_sfps().unwrap();
        assert_eq!(sfps.len(), 2);
        assert_eq!(sfps.iter().flatten().map(|s| s.sdk_index).collect::<Vec<_>>(), [0, 1]);
        // Asked again: the same objects.
        let again = lc.get_all_sfps().unwrap();
        assert!(Arc::ptr_eq(sfps[1].as_ref().unwrap(), again[1].as_ref().unwrap()));
        // A port left unbuilt is built on the next ask.
        {
            let mut st = lc.lock();
            st.sfp_list[1] = None;
            st.sfp_initialized_count = 1;
        }
        assert!(lc.get_all_sfps().unwrap().iter().all(Option::is_some));
        // No line cards: the index cannot be worked out.
        root.write(&format!("{SYS}/lc1_powered"), "0");
        lc.get_all_sfps().unwrap();
        root.write(&format!("{SYS}/lc1_powered"), "1");
        root.write("/run/hw-management/config/hotplug_linecards", "0");
        crate::utils::forget_caches();
        let e = lc.get_all_sfps().err().unwrap();
        assert_eq!((e.exc.name(), e.str()), ("ZeroDivisionError", "integer modulo by zero".to_string()));
    }
}
