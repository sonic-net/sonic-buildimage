use std::time::{Duration, Instant};

use swss_common::{
    DbConnector, Exception, KeyOpFieldValues, KeyOperation, SelectResult, SubscriberStateTable,
};

use crate::device_info::{StateDBConnector, StateDBTrait};

const WARM_RESTART_ENABLE_TABLE: &str = "WARM_RESTART_ENABLE_TABLE";
const RESTART_KEY: &str = "system";
const RESTART_ENABLE_FIELD: &str = "enable";

/// Default wait of swss::RestartWaiter::waitAdvancedBootDone.
pub const DEFAULT_MAX_WAIT: Duration = Duration::from_secs(180);
/// The pause after a failed read, so that a lasting failure is not read again in a busy loop until the wait ends.
const FAILED_READ_PAUSE: Duration = Duration::from_secs(1);

/// Port of swss::RestartWaiter::waitAdvancedBootDone: if a warm or fast boot is in progress, wait until
/// WARM_RESTART_ENABLE_TABLE|system enable turns "false" or `max_wait` passes. Returns false on timeout. A failed read
/// goes to `on_error`, and the wait goes on, as swss::RedisTableWaiter logs it and goes on.
pub fn wait_advanced_boot_done(
    max_wait: Duration,
    on_error: impl FnMut(&Exception),
) -> Result<bool, Exception> {
    let state_db = StateDBConnector::new()?;
    if !is_advanced_boot_in_progress(&state_db)? {
        return Ok(true);
    }

    let db = DbConnector::new_named("STATE_DB", false, 0)?;
    let table = SubscriberStateTable::new(db, WARM_RESTART_ENABLE_TABLE, None, None)?;
    Ok(wait_for_boot_done(&table, max_wait, on_error))
}

/// The updates of a subscribed table; a trait so that the wait can be tested without Redis.
pub trait TableUpdates {
    /// Waits up to `timeout` for updates.
    fn read_data(&self, timeout: Duration) -> Result<SelectResult, Exception>;
    fn pops(&self) -> Result<Vec<KeyOpFieldValues>, Exception>;
}

impl TableUpdates for SubscriberStateTable {
    fn read_data(&self, timeout: Duration) -> Result<SelectResult, Exception> {
        SubscriberStateTable::read_data(self, timeout, true)
    }

    fn pops(&self) -> Result<Vec<KeyOpFieldValues>, Exception> {
        SubscriberStateTable::pops(self)
    }
}

/// Waits up to `max_wait` for the update that ends the boot; false on timeout or a signal.
pub fn wait_for_boot_done(
    updates: &impl TableUpdates,
    max_wait: Duration,
    mut on_error: impl FnMut(&Exception),
) -> bool {
    let start = Instant::now();
    while let Some(remaining) = max_wait
        .checked_sub(start.elapsed())
        .filter(|d| !d.is_zero())
    {
        let read = updates
            .read_data(remaining)
            .and_then(|result| match result {
                SelectResult::Data => Ok(Some(updates.pops()?.iter().any(is_boot_done))),
                SelectResult::Signal => Ok(None),
                SelectResult::Timeout => Ok(Some(false)),
            });
        match read {
            Ok(Some(true)) => return true,
            Ok(Some(false)) => {}
            Ok(None) => return false,
            Err(e) => {
                on_error(&e);
                std::thread::sleep(FAILED_READ_PAUSE.min(max_wait.saturating_sub(start.elapsed())));
            }
        }
    }
    false
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
