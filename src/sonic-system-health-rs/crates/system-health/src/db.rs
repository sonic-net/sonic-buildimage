//! Redis access behind traits, so the checks run against in-memory tables in tests.

use std::collections::{BTreeMap, HashMap};

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

pub struct SwssStateDb {
    v2: SonicV2Connector,
}

impl SwssStateDb {
    pub fn connect() -> Result<Self> {
        let v2 = SonicV2Connector::new(true, None)?;
        v2.connect(STATE_DB, true)?;
        Ok(Self { v2 })
    }
}

impl StateDb for SwssStateDb {
    fn keys(&self, pattern: &str) -> Result<Vec<String>> {
        Ok(self.v2.keys(STATE_DB, Some(pattern), false)?)
    }

    fn get(&self, key: &str, field: &str) -> Result<Option<String>> {
        Ok(self.v2.get(STATE_DB, key, field, false)?)
    }

    fn get_all(&self, key: &str) -> Result<FieldMap> {
        Ok(self
            .v2
            .get_all(STATE_DB, key, false)?
            .into_iter()
            .map(|(k, v)| (k, v.to_string_lossy().into_owned()))
            .collect())
    }

    fn hmset(&self, key: &str, values: &[(String, String)]) -> Result<()> {
        Ok(self
            .v2
            .hmset(STATE_DB, key, values.iter().map(|(f, v)| (f.as_str(), v.as_str())))?)
    }

    fn exists(&self, key: &str) -> Result<bool> {
        Ok(self.v2.exists(STATE_DB, key)?)
    }

    fn del(&self, key: &str) -> Result<()> {
        self.v2.del(STATE_DB, key, false)?;
        Ok(())
    }

    fn delete_all_by_pattern(&self, pattern: &str) -> Result<()> {
        Ok(self.v2.delete_all_by_pattern(STATE_DB, pattern)?)
    }
}

pub struct SwssConfigDb {
    conn: ConfigDBConnector,
}

impl SwssConfigDb {
    pub fn connect() -> Result<Self> {
        let conn = ConfigDBConnector::new(true, None)?;
        conn.connect(true, false)?;
        Ok(Self { conn })
    }
}

impl ConfigDb for SwssConfigDb {
    fn get_table(&self, table: &str) -> Result<TableData> {
        Ok(self
            .conn
            .get_table(table)?
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
