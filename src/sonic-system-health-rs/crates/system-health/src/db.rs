//! Redis access behind traits, so the checks run against in-memory tables in tests.

use std::collections::{BTreeMap, HashMap};
use std::sync::{Arc, Mutex};

use anyhow::Result;
use swss_common::{ConfigDBConnector, SonicV2Connector};

pub type FieldMap = HashMap<String, String>;
/// Table entries by key, sorted.
pub type TableData = BTreeMap<String, FieldMap>;

pub trait StateDb {
    fn keys(&self, pattern: &str) -> Result<Vec<String>>;
    fn get(&self, key: &str, field: &str) -> Result<Option<String>>;
    fn get_all(&self, key: &str) -> Result<FieldMap>;
    fn hmset(&self, key: &str, values: &[(String, String)]) -> Result<()>;
    fn exists(&self, key: &str) -> Result<bool>;
    fn del(&self, key: &str) -> Result<()>;
    fn delete_all_by_pattern(&self, pattern: &str) -> Result<()>;
}

pub trait ConfigDb {
    fn get_table(&self, table: &str) -> Result<TableData>;
}

const STATE_DB: &str = "STATE_DB";

/// The first connection error of a process's Redis calls. The connections do not all reconnect (ConfigDBConnector's
/// reads never do), and a restarted Redis has lost what healthd wrote, so healthd exits on one and systemd restarts it
/// with new connections. Python's daemon did not recover: its main loop ended on the error, and its sysmonitor thread
/// kept the process up, so it was not restarted either.
#[derive(Clone, Default)]
pub struct RedisErrors(Arc<Mutex<Option<String>>>);

impl RedisErrors {
    /// Passes `result` on. An error is kept, if it is the first, unless `fails_again` (the same call on a new
    /// connection) is true: then the data is at fault, a key of the wrong type for instance, not the connection, and
    /// the error is only returned, as Python reported it in the results and went on.
    pub fn check<T, E>(&self, result: Result<T, E>, fails_again: impl FnOnce() -> bool) -> Result<T>
    where
        E: std::error::Error + Send + Sync + 'static,
    {
        result.map_err(|e| {
            if !fails_again() {
                let mut first = self.0.lock().unwrap_or_else(|e| e.into_inner());
                first.get_or_insert_with(|| e.to_string());
            }
            e.into()
        })
    }

    pub fn first(&self) -> Option<String> {
        self.0.lock().unwrap_or_else(|e| e.into_inner()).clone()
    }
}

pub struct SwssStateDb {
    v2: SonicV2Connector,
    errors: RedisErrors,
}

impl SwssStateDb {
    /// Connects, waiting for Redis as Python does; the errors of later calls go to `errors`.
    pub fn connect(errors: RedisErrors) -> Result<Self> {
        let v2 = SonicV2Connector::new(true, None)?;
        v2.connect(STATE_DB, true)?;
        Ok(Self { v2, errors })
    }

    /// Whether `call` fails on a new connection too; false when none can be made.
    fn fails_again<T>(call: impl FnOnce(&SonicV2Connector) -> swss_common::Result<T>) -> bool {
        let fresh = || -> swss_common::Result<SonicV2Connector> {
            let v2 = SonicV2Connector::new(true, None)?;
            v2.connect(STATE_DB, false)?;
            Ok(v2)
        };
        fresh().is_ok_and(|v2| call(&v2).is_err())
    }
}

impl StateDb for SwssStateDb {
    fn keys(&self, pattern: &str) -> Result<Vec<String>> {
        let call = |v2: &SonicV2Connector| v2.keys(STATE_DB, Some(pattern), false);
        self.errors.check(call(&self.v2), || Self::fails_again(call))
    }

    fn get(&self, key: &str, field: &str) -> Result<Option<String>> {
        let call = |v2: &SonicV2Connector| v2.get(STATE_DB, key, field, false);
        self.errors.check(call(&self.v2), || Self::fails_again(call))
    }

    fn get_all(&self, key: &str) -> Result<FieldMap> {
        let call = |v2: &SonicV2Connector| v2.get_all(STATE_DB, key, false);
        Ok(self
            .errors
            .check(call(&self.v2), || Self::fails_again(call))?
            .into_iter()
            .map(|(k, v)| (k, v.to_string_lossy().into_owned()))
            .collect())
    }

    fn hmset(&self, key: &str, values: &[(String, String)]) -> Result<()> {
        let call =
            |v2: &SonicV2Connector| v2.hmset(STATE_DB, key, values.iter().map(|(f, v)| (f.as_str(), v.as_str())));
        self.errors.check(call(&self.v2), || Self::fails_again(call))
    }

    fn exists(&self, key: &str) -> Result<bool> {
        let call = |v2: &SonicV2Connector| v2.exists(STATE_DB, key);
        self.errors.check(call(&self.v2), || Self::fails_again(call))
    }

    fn del(&self, key: &str) -> Result<()> {
        let call = |v2: &SonicV2Connector| v2.del(STATE_DB, key, false);
        self.errors.check(call(&self.v2), || Self::fails_again(call))?;
        Ok(())
    }

    fn delete_all_by_pattern(&self, pattern: &str) -> Result<()> {
        let call = |v2: &SonicV2Connector| v2.delete_all_by_pattern(STATE_DB, pattern);
        self.errors.check(call(&self.v2), || Self::fails_again(call))
    }
}

pub struct SwssConfigDb {
    conn: ConfigDBConnector,
    errors: RedisErrors,
}

impl SwssConfigDb {
    /// Connects, waiting for Redis as Python does; the errors of later calls go to `errors`.
    pub fn connect(errors: RedisErrors) -> Result<Self> {
        let conn = ConfigDBConnector::new(true, None)?;
        conn.connect(true, false)?;
        Ok(Self { conn, errors })
    }

    /// Whether `call` fails on a new connection too; false when none can be made.
    fn fails_again<T>(call: impl FnOnce(&ConfigDBConnector) -> swss_common::Result<T>) -> bool {
        let fresh = || -> swss_common::Result<ConfigDBConnector> {
            let conn = ConfigDBConnector::new(true, None)?;
            conn.connect(false, false)?;
            Ok(conn)
        };
        fresh().is_ok_and(|conn| call(&conn).is_err())
    }
}

impl ConfigDb for SwssConfigDb {
    fn get_table(&self, table: &str) -> Result<TableData> {
        let call = |conn: &ConfigDBConnector| conn.get_table(table);
        Ok(self
            .errors
            .check(call(&self.conn), || Self::fails_again(call))?
            .into_iter()
            .map(|(key, fields)| {
                let fields = fields
                    .into_iter()
                    .map(|(f, v)| (f, v.to_string_lossy().into_owned()))
                    .collect();
                (key, fields)
            })
            .collect())
    }
}

/// In-memory databases for tests and dry runs.
pub mod memory {
    use std::cell::RefCell;

    use super::*;

    #[derive(Default)]
    pub struct MemoryStateDb {
        pub data: RefCell<BTreeMap<String, FieldMap>>,
    }

    impl MemoryStateDb {
        pub fn set(&self, key: &str, fields: &[(&str, &str)]) {
            let mut data = self.data.borrow_mut();
            let entry = data.entry(key.to_string()).or_default();
            for (f, v) in fields {
                entry.insert(f.to_string(), v.to_string());
            }
        }

        pub fn entry(&self, key: &str) -> Option<FieldMap> {
            self.data.borrow().get(key).cloned()
        }
    }

    /// Redis glob with `*` only, which is all the checks use.
    pub fn glob_match(pattern: &str, key: &str) -> bool {
        let parts: Vec<&str> = pattern.split('*').collect();
        if parts.len() == 1 {
            return pattern == key;
        }
        let (first, last) = (parts[0], parts[parts.len() - 1]);
        if !key.starts_with(first) || key.len() < first.len() + last.len() || !key.ends_with(last) {
            return false;
        }
        let mut rest = &key[first.len()..key.len() - last.len()];
        for part in &parts[1..parts.len() - 1] {
            match rest.find(part) {
                Some(pos) => rest = &rest[pos + part.len()..],
                None => return false,
            }
        }
        true
    }

    impl StateDb for MemoryStateDb {
        fn keys(&self, pattern: &str) -> Result<Vec<String>> {
            Ok(self
                .data
                .borrow()
                .keys()
                .filter(|k| glob_match(pattern, k))
                .cloned()
                .collect())
        }

        fn get(&self, key: &str, field: &str) -> Result<Option<String>> {
            Ok(self.data.borrow().get(key).and_then(|f| f.get(field)).cloned())
        }

        fn get_all(&self, key: &str) -> Result<FieldMap> {
            Ok(self.data.borrow().get(key).cloned().unwrap_or_default())
        }

        fn hmset(&self, key: &str, values: &[(String, String)]) -> Result<()> {
            let mut data = self.data.borrow_mut();
            let entry = data.entry(key.to_string()).or_default();
            for (f, v) in values {
                entry.insert(f.clone(), v.clone());
            }
            Ok(())
        }

        fn exists(&self, key: &str) -> Result<bool> {
            Ok(self.data.borrow().contains_key(key))
        }

        fn del(&self, key: &str) -> Result<()> {
            self.data.borrow_mut().remove(key);
            Ok(())
        }

        fn delete_all_by_pattern(&self, pattern: &str) -> Result<()> {
            self.data.borrow_mut().retain(|k, _| !glob_match(pattern, k));
            Ok(())
        }
    }

    #[derive(Default)]
    pub struct MemoryConfigDb {
        pub tables: RefCell<BTreeMap<String, TableData>>,
        /// Tables whose read fails, as a Redis error would.
        pub failing: RefCell<Vec<String>>,
    }

    impl MemoryConfigDb {
        pub fn set(&self, table: &str, key: &str, fields: &[(&str, &str)]) {
            let mut tables = self.tables.borrow_mut();
            let entry = tables
                .entry(table.to_string())
                .or_default()
                .entry(key.to_string())
                .or_default();
            for (f, v) in fields {
                entry.insert(f.to_string(), v.to_string());
            }
        }
    }

    impl ConfigDb for MemoryConfigDb {
        fn get_table(&self, table: &str) -> Result<TableData> {
            if self.failing.borrow().iter().any(|t| t == table) {
                anyhow::bail!("cannot read {table}");
            }
            Ok(self.tables.borrow().get(table).cloned().unwrap_or_default())
        }
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        fn failed(text: &str) -> Result<i32, std::io::Error> {
            Err(std::io::Error::other(text.to_string()))
        }

        #[test]
        fn the_first_connection_error_is_kept() {
            let errors = RedisErrors::default();
            assert_eq!(
                errors
                    .check(Ok::<_, std::io::Error>(1), || panic!("not retried"))
                    .unwrap(),
                1
            );
            assert_eq!(errors.first(), None);
            // The same call works on a new connection: the connection was lost.
            assert_eq!(
                errors
                    .clone()
                    .check(failed("Broken pipe"), || false)
                    .unwrap_err()
                    .to_string(),
                "Broken pipe"
            );
            assert!(errors.check(failed("Connection refused"), || false).is_err());
            assert_eq!(errors.first().as_deref(), Some("Broken pipe"));
        }

        #[test]
        fn an_error_of_the_data_is_not_kept() {
            // The same call fails on a new connection too: a key of the wrong type, not the connection.
            let errors = RedisErrors::default();
            let wrong_kind = "Operation against a key holding the wrong kind of value";
            let e = errors.check(failed(wrong_kind), || true).unwrap_err();
            assert_eq!(e.to_string(), wrong_kind);
            assert_eq!(errors.first(), None);
        }

        #[test]
        fn glob() {
            assert!(glob_match("FAN_INFO*", "FAN_INFO|fan1"));
            assert!(glob_match("TEMPERATURE_INFO|ASIC*", "TEMPERATURE_INFO|ASIC"));
            assert!(!glob_match("TEMPERATURE_INFO|ASIC*", "TEMPERATURE_INFO|CPU"));
            assert!(glob_match("ALL_SERVICE_STATUS|*", "ALL_SERVICE_STATUS|swss"));
            assert!(glob_match("SYSTEM_HEALTH_INFO", "SYSTEM_HEALTH_INFO"));
            assert!(glob_match("a*b*c", "aXbYc"));
            assert!(!glob_match("a*b*c", "aXc"));
        }
    }
}
