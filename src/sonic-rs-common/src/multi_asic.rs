use swss_common::{DbConnector, Table};

use crate::device_info::DeviceInfo;

const CHASSIS_STATE_DB: &str = "CHASSIS_STATE_DB";
const CHASSIS_FABRIC_ASIC_INFO_TABLE: &str = "CHASSIS_FABRIC_ASIC_TABLE";
const ASIC_NAME_PREFIX: &str = "asic";

/// ASIC ids present on the device, as sonic_py_common.multi_asic.get_asic_presence_list.
pub fn asic_presence_list(info: &DeviceInfo) -> Vec<u32> {
    asic_presence_list_with(info, fabric_asic_keys)
}

/// Same as [asic_presence_list], with the CHASSIS_FABRIC_ASIC_TABLE key reader injected.
pub fn asic_presence_list_with(
    info: &DeviceInfo,
    fabric_asic_keys: impl FnOnce() -> Vec<String>,
) -> Vec<u32> {
    let num_asics = info.num_asics();
    if num_asics <= 1 || !info.is_supervisor() {
        return (0..num_asics).collect();
    }
    let expected = info.expected_asic_list();
    if !expected.is_empty() {
        return expected;
    }
    fabric_asic_keys()
        .iter()
        .filter_map(|name| name.strip_prefix(ASIC_NAME_PREFIX)?.parse().ok())
        .collect()
}

fn fabric_asic_keys() -> Vec<String> {
    DbConnector::new_named(CHASSIS_STATE_DB, false, 0)
        .and_then(|db| Table::new(db, CHASSIS_FABRIC_ASIC_INFO_TABLE))
        .and_then(|table| table.get_keys())
        .unwrap_or_default()
}
