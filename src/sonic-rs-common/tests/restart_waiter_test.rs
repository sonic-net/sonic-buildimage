use sonic_rs_common::restart_waiter::{is_advanced_boot_in_progress, is_boot_done};
use swss_common::KeyOpFieldValues;

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
