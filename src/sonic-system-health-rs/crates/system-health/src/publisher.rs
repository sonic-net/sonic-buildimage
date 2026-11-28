//! STATE_DB SYSTEM_HEALTH_INFO, written anew every cycle.

use anyhow::Result;
use indexmap::IndexMap;

use crate::db::StateDb;
use crate::manager::CheckResult;

pub const SYSTEM_HEALTH_TABLE_NAME: &str = "SYSTEM_HEALTH_INFO";

/// The message of every Not OK object, and `summary`.
pub fn health_fields(result: &CheckResult) -> IndexMap<String, String> {
    let mut fields = IndexMap::new();
    for objects in result.stats.values() {
        for (name, object) in objects {
            if !object.is_ok() {
                fields.insert(name.clone(), object.message.clone());
            }
        }
    }
    fields.insert("summary".to_string(), result.summary.clone());
    fields
}

/// Deletes the table, then writes the new fields, as Python does.
pub fn publish(db: &dyn StateDb, result: &CheckResult) -> Result<()> {
    let values: Vec<(String, String)> = health_fields(result).into_iter().collect();
    db.del(SYSTEM_HEALTH_TABLE_NAME)?;
    db.hmset(SYSTEM_HEALTH_TABLE_NAME, &values)
}

pub fn clear(db: &dyn StateDb) -> Result<()> {
    db.del(SYSTEM_HEALTH_TABLE_NAME)
}
