use std::time::{Duration, Instant};

use swss_common::{
    DbConnector, KeyOpFieldValues, KeyOperation, SelectResult, SubscriberStateTable,
};

use crate::device_info::{StateDBConnector, StateDBTrait};

const WARM_RESTART_ENABLE_TABLE: &str = "WARM_RESTART_ENABLE_TABLE";
const RESTART_KEY: &str = "system";
const RESTART_ENABLE_FIELD: &str = "enable";

/// Default wait of swss::RestartWaiter::waitAdvancedBootDone.
pub const DEFAULT_MAX_WAIT: Duration = Duration::from_secs(180);

/// Port of swss::RestartWaiter::waitAdvancedBootDone: if a warm or fast boot is in progress, wait until
/// WARM_RESTART_ENABLE_TABLE|system enable turns "false" or `max_wait` passes. Returns false on timeout.
pub fn wait_advanced_boot_done(max_wait: Duration) -> Result<bool, swss_common::Exception> {
    let state_db = StateDBConnector::new()?;
    if !is_advanced_boot_in_progress(&state_db)? {
        return Ok(true);
    }

    let db = DbConnector::new_named("STATE_DB", false, 0)?;
    let table = SubscriberStateTable::new(db, WARM_RESTART_ENABLE_TABLE, None, None)?;
    let start = Instant::now();
    while let Some(remaining) = max_wait
        .checked_sub(start.elapsed())
        .filter(|d| !d.is_zero())
    {
        match table.read_data(remaining, true)? {
            SelectResult::Data => {
                if table.pops()?.iter().any(is_boot_done) {
                    return Ok(true);
                }
            }
            SelectResult::Signal => return Ok(false),
            SelectResult::Timeout => {}
        }
    }
    Ok(false)
}

pub fn is_advanced_boot_in_progress<T: StateDBTrait>(
    state_db: &T,
) -> Result<bool, swss_common::Exception> {
    let key = format!("{WARM_RESTART_ENABLE_TABLE}|{RESTART_KEY}");
    Ok(state_db
        .hget(&key, RESTART_ENABLE_FIELD)?
        .is_some_and(|v| v.to_string_lossy().to_lowercase() == "true"))
}

/// True for the update that ends the boot: `system` set with `enable` "false" (any case).
pub fn is_boot_done(kfv: &KeyOpFieldValues) -> bool {
    kfv.operation == KeyOperation::Set
        && kfv.key == RESTART_KEY
        && kfv
            .field_values
            .get(RESTART_ENABLE_FIELD)
            .is_some_and(|v| v.to_string_lossy().to_lowercase() == "false")
}
