use std::cell::RefCell;
use std::collections::VecDeque;
use std::time::{Duration, Instant};

use sonic_rs_common::restart_waiter::{
    is_advanced_boot_in_progress, is_boot_done, wait_for_boot_done, TableUpdates,
};
use swss_common::{Exception, KeyOpFieldValues, SelectResult};

mod mock_state_db;
use mock_state_db::MockStateDbConnector;

#[test]
fn advanced_boot_in_progress() {
    let mut db = MockStateDbConnector::new();
    assert!(!is_advanced_boot_in_progress(&db).unwrap());
    db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "false");
    assert!(!is_advanced_boot_in_progress(&db).unwrap());
    db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "TRUE");
    assert!(is_advanced_boot_in_progress(&db).unwrap());
}

#[test]
fn boot_done_update() {
    assert!(is_boot_done(&KeyOpFieldValues::set(
        "system",
        [("enable", "false")]
    )));
    assert!(is_boot_done(&KeyOpFieldValues::set(
        "system",
        [("enable", "False")]
    )));
    assert!(!is_boot_done(&KeyOpFieldValues::set(
        "system",
        [("enable", "true")]
    )));
    assert!(!is_boot_done(&KeyOpFieldValues::set(
        "swss",
        [("enable", "false")]
    )));
    assert!(!is_boot_done(&KeyOpFieldValues::set(
        "system",
        [("other", "false")]
    )));
    assert!(!is_boot_done(&KeyOpFieldValues::del("system")));
}

/// Reads the given results in order, then `last` on every read; the updates read are the end of the boot.
struct FakeUpdates {
    reads: RefCell<VecDeque<Result<SelectResult, Exception>>>,
    last: Result<SelectResult, Exception>,
}

impl TableUpdates for FakeUpdates {
    fn read_data(&self, _timeout: Duration) -> Result<SelectResult, Exception> {
        self.reads
            .borrow_mut()
            .pop_front()
            .unwrap_or_else(|| self.last.clone())
    }

    fn pops(&self) -> Result<Vec<KeyOpFieldValues>, Exception> {
        Ok(vec![KeyOpFieldValues::set("system", [("enable", "false")])])
    }
}

/// As swss::RedisTableWaiter: a failed read is reported, and the wait goes on.
#[test]
fn a_failed_read_does_not_end_the_wait() {
    let updates = FakeUpdates {
        reads: RefCell::new(VecDeque::from([Err(Exception::new("readData error"))])),
        last: Ok(SelectResult::Data),
    };
    let mut errors = Vec::new();
    assert!(wait_for_boot_done(&updates, Duration::from_secs(10), |e| {
        errors.push(e.message().to_string())
    }));
    assert_eq!(errors, ["readData error"]);
}

/// Reads that keep failing end the wait at its end, without spinning in between.
#[test]
fn reads_that_keep_failing_wait_until_the_end() {
    let updates = FakeUpdates {
        reads: RefCell::new(VecDeque::new()),
        last: Err(Exception::new("readData error")),
    };
    let mut errors = 0;
    let start = Instant::now();
    assert!(!wait_for_boot_done(
        &updates,
        Duration::from_millis(300),
        |_| { errors += 1 }
    ));
    assert!(start.elapsed() >= Duration::from_millis(300));
    assert_eq!(errors, 1);
}
