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


//! `sonic_platform/db_table_helper.py`: STATE_DB and APPL_DB tables, one set
//! per thread.
//!
//! Python connects one pair per front-end namespace and then takes the
//! default namespace's tables -- which a multi-ASIC system's front-end
//! namespaces do not include, so there every use raises `KeyError('')`
//! (and the helper stays uninitialised, to raise again next time).  On a
//! single-ASIC system the default namespace is the only one.

use std::cell::RefCell;
use std::sync::Arc;

use platform_base::db::Db;
use platform_base::py::{PyErr, PyResult};

pub const TRANSCEIVER_DOM_TEMPERATURE_TABLE: &str = "TRANSCEIVER_DOM_TEMPERATURE";
pub const TRANSCEIVER_DOM_THRESHOLD_TABLE: &str = "TRANSCEIVER_DOM_THRESHOLD";
pub const TRANSCEIVER_INFO_TABLE: &str = "TRANSCEIVER_INFO";
pub const TEMPERATURE_INFO_TABLE: &str = "TEMPERATURE_INFO";

/// The thread's `DbTableHelper`: its two connections, once initialised.
struct Helper {
    state: Arc<Db>,
    appl: Arc<Db>,
}

thread_local! {
    static HELPER: RefCell<Option<Arc<Helper>>> = const { RefCell::new(None) };
}

fn helper() -> PyResult<Arc<Helper>> {
    if let Some(h) = HELPER.with(|h| h.borrow().clone()) {
        return Ok(h);
    }
    if platform_base::multi_asic::is_multi_asic()? {
        return Err(PyErr::key_error(""));
    }
    let h = Arc::new(Helper {
        state: Arc::new(Db::connect("STATE_DB")?),
        appl: Arc::new(Db::connect("APPL_DB")?),
    });
    HELPER.with(|slot| *slot.borrow_mut() = Some(h.clone()));
    Ok(h)
}

/// `Table(db, table).hget(key, field)`: `(present, value)`.
pub fn table_hget(table: &str, key: &str, field: &str) -> PyResult<(bool, Option<String>)> {
    let h = helper()?;
    let v = h.state.hget(&h.state.key(table, key), field)?;
    Ok((v.is_some(), v))
}

/// `get_appl_db(namespace).exists(key)`: only the default namespace has a
/// database here, so any other is the `KeyError` Python's dict lookup raises
/// on a single-ASIC system.
pub fn appl_db_exists(namespace: &str, key: &str) -> PyResult<bool> {
    let h = helper()?;
    if !namespace.is_empty() {
        return Err(PyErr::key_error(namespace));
    }
    h.appl.exists(key)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, STATE_DB};

    const APPL_DB: i64 = 0;

    #[test]
    fn tables_are_state_db_s_and_appl_db_is_the_default_namespace_s() {
        let root = Root::new();
        let redis = FakeRedis::get();
        redis.install(&root);
        redis.flush();
        assert_eq!(table_hget(TRANSCEIVER_INFO_TABLE, "Ethernet0", "type").unwrap(), (false, None));
        redis.hset(STATE_DB, "TRANSCEIVER_INFO|Ethernet0", &[("type", "QSFP56")]);
        assert_eq!(table_hget(TRANSCEIVER_INFO_TABLE, "Ethernet0", "type").unwrap(), (true, Some("QSFP56".into())));
        assert!(!appl_db_exists("", "PORT_TABLE:Ethernet0").unwrap());
        redis.hset(APPL_DB, "PORT_TABLE:Ethernet0", &[("admin_status", "up")]);
        assert!(appl_db_exists("", "PORT_TABLE:Ethernet0").unwrap());
        let e = appl_db_exists("asic0", "PORT_TABLE:Ethernet0").unwrap_err();
        assert_eq!((e.exc.name(), e.str().as_str()), ("KeyError", "'asic0'"));
    }

    /// The default namespace's tables are not among a multi-ASIC system's
    /// front-end namespaces: `KeyError('')`, every time.
    #[test]
    fn a_multi_asic_system_has_no_default_namespace() {
        let root = Root::new();
        root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
        for _ in 0..2 {
            let e = table_hget(TEMPERATURE_INFO_TABLE, "x", "temperature").unwrap_err();
            assert_eq!((e.exc.name(), e.str().as_str()), ("KeyError", "''"));
        }
        let e = appl_db_exists("", "x").unwrap_err();
        assert_eq!(e.exc.name(), "KeyError");
    }
}
