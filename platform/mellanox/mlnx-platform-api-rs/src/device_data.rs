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


//! `sonic_platform/device_data.py`: the per-platform table and
//! `DeviceDataManager`.
//!
//! Every answer here is `@read_only_cache()`d in Python -- cached once it is
//! truthy, recomputed while it is not -- and so it is here; a platform with no
//! fans re-globs every call, as it does in Python.

use platform_base::py::{PyResult, PyVal};
use platform_base::{device_info, io, json};

use crate::utils::{self, Log, ReadOnlyCache};

pub const DEFAULT_WD_PERIOD: i128 = 65535;

/// One platform's entry in `DEVICE_DATA`.
#[derive(Default)]
pub struct PlatformData {
    /// `thermal.capability`, by temperature file name.
    pub thermal_capability: Option<&'static [(&'static str, bool)]>,
    /// `watchdog.max_period`.
    pub watchdog_max_period: Option<i128>,
    /// `sfp.max_port_per_line_card`.
    pub max_port_per_line_card: Option<i128>,
    /// `sfp.fw_control_ports`.
    pub fw_control_ports: Option<&'static [i128]>,
    /// Whether the entry has a `thermal` dict at all, and an `sfp` one.
    pub has_thermal: bool,
    pub has_sfp: bool,
    pub has_watchdog: bool,
}

/// `DEVICE_DATA[platform]`, or None for a platform the table does not list.
pub fn device_data(platform: &str) -> Option<PlatformData> {
    const NO_COMEX: &[(&str, bool)] = &[("comex_amb", false)];
    const NO_CPU_PACK_COMEX: &[(&str, bool)] = &[("cpu_pack", false), ("comex_amb", false)];
    const LD: &[(&str, bool)] = &[("port_amb", false), ("fan_amb", false), ("comex_amb", false)];
    let thermal = |cap: &'static [(&'static str, bool)]| PlatformData {
        thermal_capability: Some(cap),
        has_thermal: true,
        ..Default::default()
    };
    Some(match platform {
        "x86_64-mlnx_msn2700-r0" => thermal(NO_COMEX),
        // `minimum_table` is in the Python table too; nothing reads it.
        "x86_64-mlnx_msn2700a1-r0" => thermal(&[("comex_amb", true)]),
        "x86_64-mlnx_msn2740-r0" => thermal(NO_CPU_PACK_COMEX),
        "x86_64-mlnx_msn2100-r0" | "x86_64-mlnx_msn2010-r0" => PlatformData {
            watchdog_max_period: Some(32),
            has_watchdog: true,
            ..thermal(NO_CPU_PACK_COMEX)
        },
        "x86_64-mlnx_msn2410-r0" => thermal(NO_COMEX),
        "x86_64-mlnx_msn4700_simx-r0" => thermal(&[("cpu_pack", false)]),
        "x86_64-mlnx_msn3700-r0" | "x86_64-mlnx_msn3700c-r0" | "x86_64-mlnx_msn3800-r0"
        | "x86_64-mlnx_msn4700-r0" | "x86_64-mlnx_msn4410-r0" | "x86_64-mlnx_msn3420-r0"
        | "x86_64-mlnx_msn4600c-r0" | "x86_64-mlnx_msn4600-r0" => PlatformData::default(),
        "x86_64-nvidia_sn4280-r0" => thermal(NO_COMEX),
        "x86_64-nvidia_sn4800-r0" => PlatformData {
            max_port_per_line_card: Some(16),
            has_sfp: true,
            ..thermal(NO_COMEX)
        },
        "x86_64-nvidia_sn2201-r0" => thermal(&[("comex_amb", false), ("cpu_amb", true)]),
        "x86_64-nvidia_sn5400-r0" => PlatformData {
            fw_control_ports: Some(&[64, 65]),
            has_sfp: true,
            ..thermal(&[("comex_amb", false), ("pch_temp", true)])
        },
        "x86_64-nvidia_sn5600-r0" => PlatformData {
            fw_control_ports: Some(&[64]),
            has_sfp: true,
            ..thermal(&[("comex_amb", false), ("pch_temp", true)])
        },
        "x86_64-nvidia_sn5600_simx-r0" => thermal(NO_CPU_PACK_COMEX),
        "x86_64-nvidia_sn5610n-r0" | "x86_64-nvidia_sn5640-r0" => PlatformData {
            fw_control_ports: Some(&[64, 65]),
            has_sfp: true,
            ..thermal(NO_COMEX)
        },
        "x86_64-nvidia_sn5640_simx-r0" | "x86_64-nvidia_sn4280_simx-r0" => thermal(NO_CPU_PACK_COMEX),
        "x86_64-nvidia_sn6600_ld-r0" | "x86_64-nvidia_sn6810_ld-r0"
        | "x86_64-nvidia_sn6810_ld_simx-r0" => thermal(LD),
        _ => return None,
    })
}

const HW: &str = "/run/hw-management";

/// `DeviceDataManager.get_platform_name()`.
pub fn get_platform_name() -> PyResult<Option<String>> {
    static C: ReadOnlyCache<Option<String>> = ReadOnlyCache::new();
    C.try_get(device_info::get_platform)
}

/// `is_simx_platform()`: `platform_name and 'simx' in platform_name`.
pub fn is_simx_platform() -> PyResult<bool> {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.try_get(|| Ok(get_platform_name()?.is_some_and(|p| !p.is_empty() && p.contains("simx"))))
}

/// `get_fan_drawer_sysfs_count()`.
pub fn get_fan_drawer_sysfs_count() -> usize {
    static C: ReadOnlyCache<usize> = ReadOnlyCache::new();
    C.get(|| utils::glob(&format!("{HW}/thermal/fan*_status")).len())
}

/// `get_fan_drawer_count()`.
pub fn get_fan_drawer_count() -> usize {
    static C: ReadOnlyCache<usize> = ReadOnlyCache::new();
    C.get(|| {
        let n = get_fan_drawer_sysfs_count();
        if n == 0 {
            0
        } else if is_fan_hotswapable() {
            n
        } else {
            1
        }
    })
}

/// `get_fan_count()`.
pub fn get_fan_count() -> usize {
    static C: ReadOnlyCache<usize> = ReadOnlyCache::new();
    C.get(|| utils::glob(&format!("{HW}/thermal/fan*_speed_get")).len())
}

/// `is_fan_hotswapable()`.
pub fn is_fan_hotswapable() -> bool {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.get(|| utils::read_int(&format!("{HW}/config/hotplug_fans"), 0, Log::Error) > 0)
}

/// `get_psu_count()`.
pub fn get_psu_count() -> i128 {
    static C: ReadOnlyCache<i128> = ReadOnlyCache::new();
    C.get(|| {
        let n = utils::read_int(&format!("{HW}/config/hotplug_psus"), 0, Log::Error);
        if n > 0 {
            n
        } else {
            utils::glob(&format!("{HW}/config/psu*_i2c_addr")).len() as i128
        }
    })
}

/// `is_psu_hotswapable()`.
pub fn is_psu_hotswapable() -> bool {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.get(|| utils::read_int(&format!("{HW}/config/hotplug_psus"), 0, Log::Error) > 0)
}

/// `get_pdb_count()`: from the `/var/run` spelling, silently.
pub fn get_pdb_count() -> i128 {
    static C: ReadOnlyCache<i128> = ReadOnlyCache::new();
    C.get(|| utils::read_int("/var/run/hw-management/config/hotplug_pdbs", 0, Log::Off))
}

/// `get_sfp_count()`: `len(platform.json['chassis']['sfps'])`, raising as
/// Python's subscripts raise.
pub fn get_sfp_count() -> PyResult<i128> {
    static C: ReadOnlyCache<i128> = ReadOnlyCache::new();
    C.try_get(|| {
        let dir = device_info::get_path_to_platform_dir()?;
        let data = utils::load_json_file(&io::join(&dir, "platform.json"), Log::Error)
            .unwrap_or_default();
        let sfps = json::index(json::index(&data, "chassis")?, "sfps")?;
        Ok(platform_base::py::ops::len(sfps)? as i128)
    })
}

/// `get_linecard_sfp_count(lc_index)`.
pub fn get_linecard_sfp_count(lc_index: i128) -> i128 {
    utils::read_int(&format!("{HW}/lc{lc_index}/config/module_counter"), 0, Log::Off)
}

/// `get_gearbox_count(sysfs_folder)`.
pub fn get_gearbox_count(sysfs_folder: &str) -> i128 {
    utils::read_int(&io::join(sysfs_folder, "gearbox_counter"), 0, Log::Off)
}

/// `get_cpu_thermal_count()`: note the *relative* pattern, which Python
/// resolves against the working directory -- `/` for a pmon daemon.
pub fn get_cpu_thermal_count() -> usize {
    static C: ReadOnlyCache<usize> = ReadOnlyCache::new();
    C.get(|| utils::glob("run/hw-management/thermal/cpu_core[!_]").len())
}

/// `get_thermal_capability()`.
pub fn get_thermal_capability() -> PyResult<Option<&'static [(&'static str, bool)]>> {
    let Some(data) = get_platform_name()?.and_then(|p| device_data(&p)) else {
        return Ok(None);
    };
    if !data.has_thermal {
        return Ok(None);
    }
    Ok(data.thermal_capability)
}

/// `get_linecard_count()`.
pub fn get_linecard_count() -> i128 {
    static C: ReadOnlyCache<i128> = ReadOnlyCache::new();
    C.get(|| utils::read_int(&format!("{HW}/config/hotplug_linecards"), 0, Log::Off))
}

/// `get_linecard_max_port_count()`.
pub fn get_linecard_max_port_count() -> PyResult<i128> {
    Ok(get_platform_name()?
        .and_then(|p| device_data(&p))
        .and_then(|d| d.max_port_per_line_card)
        .unwrap_or(0))
}

/// `get_platform_dpus_data()`: platform.json's `DPUS`, or None.  Raises as
/// Python does when the platform directory cannot be found or the file
/// cannot be read (`json_data.get` on None).
pub fn get_platform_dpus_data() -> PyResult<Option<PyVal>> {
    static C: ReadOnlyCache<Option<PyVal>> = ReadOnlyCache::new();
    C.try_get(|| {
        let dir = device_info::get_path_to_platform_dir()?;
        // `json_data.get('DPUS', None)`: a file that will not load is None,
        // and neither it nor a top level other than an object has `get`.
        let data = utils::load_json_file(&io::join(&dir, "platform.json"), Log::Error).unwrap_or(PyVal::None);
        match &data {
            PyVal::Dict(_) => Ok(json::get(&data, "DPUS").cloned()),
            other => Err(platform_base::PyErr::missing(&other.type_name(), "get")),
        }
    })
}

/// `get_dpu_count()`.
pub fn get_dpu_count() -> PyResult<usize> {
    static C: ReadOnlyCache<usize> = ReadOnlyCache::new();
    C.try_get(|| {
        Ok(match get_platform_dpus_data()? {
            Some(d) if d.truthy() => platform_base::py::ops::len(&d)?,
            _ => 0,
        })
    })
}

/// `is_module_host_management_mode()`.
pub fn is_module_host_management_mode() -> PyResult<bool> {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.try_get(|| {
        let asic_id = if is_multi_asic_platform()? { Some(0) } else { None };
        let dir = utils::get_path_to_hwsku_directory(asic_id)?;
        let data = utils::read_key_value(&io::join(&dir, "sai.profile"), Log::Error, "=");
        Ok(data.iter().any(|(k, v)| k == "SAI_INDEPENDENT_MODULE_MODE" && v == "1"))
    })
}

/// `is_platform_with_bmc()`.
pub fn is_platform_with_bmc() -> PyResult<bool> {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.try_get(|| Ok(device_info::is_switch_host()? && device_info::get_bmc_data().is_some_and(|d| d.truthy())))
}

/// `get_watchdog_max_period()`.
pub fn get_watchdog_max_period() -> PyResult<Option<i128>> {
    let Some(data) = get_platform_name()?.and_then(|p| device_data(&p)) else {
        return Ok(Some(DEFAULT_WD_PERIOD));
    };
    if !data.has_watchdog {
        return Ok(Some(DEFAULT_WD_PERIOD));
    }
    Ok(data.watchdog_max_period)
}

/// `get_always_fw_control_ports()`.
pub fn get_always_fw_control_ports() -> PyResult<Option<&'static [i128]>> {
    let Some(data) = get_platform_name()?.and_then(|p| device_data(&p)) else {
        return Ok(None);
    };
    if !data.has_sfp {
        return Ok(None);
    }
    Ok(data.fw_control_ports)
}

/// `get_asic_count()`.
pub fn get_asic_count() -> PyResult<i128> {
    static C: ReadOnlyCache<i128> = ReadOnlyCache::new();
    C.try_get(device_info::get_num_npus)
}

/// `is_multi_asic_platform()`.
pub fn is_multi_asic_platform() -> PyResult<bool> {
    Ok(get_asic_count()? > 1)
}

/// `is_spc1()`.
pub fn is_spc1() -> PyResult<bool> {
    Ok(get_platform_name()?
        .is_some_and(|p| p == "x86_64-mlnx_msn2700-r0" || p == "x86_64-mlnx_msn2700a1-r0"))
}

/// `check_sysfs_access(path)`: exists, and is a directory or yields a byte.
pub fn check_sysfs_access(path: &str) -> bool {
    if !io::exists(path) {
        return false;
    }
    if io::is_dir(path) {
        return true;
    }
    use std::io::Read;
    let Ok(mut f) = std::fs::File::open(io::real(path)) else {
        return false;
    };
    let mut b = [0u8; 1];
    f.read(&mut b).is_ok()
}

/// `wait_sysfs_ready(modules_count, timeout=300, interval=1)`.
pub fn wait_sysfs_ready(modules_count: i128) -> PyResult<bool> {
    let mut nodes = vec!["present", "status", "statuserror"];
    if is_module_host_management_mode()? {
        nodes.extend(["control", "power_on"]);
    }
    let mut conditions: Vec<Box<dyn FnMut() -> bool>> = Vec::new();
    for idx in 0..modules_count.max(0) {
        for node in &nodes {
            let path = format!("/sys/module/sx_core/asic0/module{idx}/{node}");
            conditions.push(Box::new(move || check_sysfs_access(&path)));
        }
    }
    Ok(utils::wait_until_conditions(conditions, 300.0, 1.0))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use crate::utils::forget_caches;
    use crate::utils::tests::{hwsku, platform, platform_dir};

    /// `DEVICE_DATA`, as Python's table has it: thermal capability, watchdog
    /// period, SFP line card ports and FW-controlled ports.
    #[test]
    fn the_table_is_pythons() {
        type Row = (&'static str, Option<&'static [(&'static str, bool)]>, Option<i128>, Option<i128>, Option<&'static [i128]>);
        let rows: &[Row] = &[
            ("x86_64-mlnx_msn2700-r0", Some(&[("comex_amb", false)]), None, None, None),
            ("x86_64-mlnx_msn2700a1-r0", Some(&[("comex_amb", true)]), None, None, None),
            ("x86_64-mlnx_msn2740-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-mlnx_msn2100-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), Some(32), None, None),
            ("x86_64-mlnx_msn2410-r0", Some(&[("comex_amb", false)]), None, None, None),
            ("x86_64-mlnx_msn2010-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), Some(32), None, None),
            ("x86_64-mlnx_msn4700_simx-r0", Some(&[("cpu_pack", false)]), None, None, None),
            ("x86_64-mlnx_msn3700-r0", None, None, None, None),
            ("x86_64-mlnx_msn3700c-r0", None, None, None, None),
            ("x86_64-mlnx_msn3800-r0", None, None, None, None),
            ("x86_64-mlnx_msn4700-r0", None, None, None, None),
            ("x86_64-mlnx_msn4410-r0", None, None, None, None),
            ("x86_64-mlnx_msn3420-r0", None, None, None, None),
            ("x86_64-mlnx_msn4600c-r0", None, None, None, None),
            ("x86_64-mlnx_msn4600-r0", None, None, None, None),
            ("x86_64-nvidia_sn4280-r0", Some(&[("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn4800-r0", Some(&[("comex_amb", false)]), None, Some(16), None),
            ("x86_64-nvidia_sn2201-r0", Some(&[("comex_amb", false), ("cpu_amb", true)]), None, None, None),
            ("x86_64-nvidia_sn5400-r0", Some(&[("comex_amb", false), ("pch_temp", true)]), None, None, Some(&[64, 65])),
            ("x86_64-nvidia_sn5600-r0", Some(&[("comex_amb", false), ("pch_temp", true)]), None, None, Some(&[64])),
            ("x86_64-nvidia_sn5600_simx-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn5610n-r0", Some(&[("comex_amb", false)]), None, None, Some(&[64, 65])),
            ("x86_64-nvidia_sn5640-r0", Some(&[("comex_amb", false)]), None, None, Some(&[64, 65])),
            ("x86_64-nvidia_sn5640_simx-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn6600_ld-r0", Some(&[("port_amb", false), ("fan_amb", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn4280_simx-r0", Some(&[("cpu_pack", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn6810_ld-r0", Some(&[("port_amb", false), ("fan_amb", false), ("comex_amb", false)]), None, None, None),
            ("x86_64-nvidia_sn6810_ld_simx-r0", Some(&[("port_amb", false), ("fan_amb", false), ("comex_amb", false)]), None, None, None),
        ];
        for (p, cap, wd, lc, fw) in rows {
            let d = device_data(p).unwrap();
            assert_eq!(d.thermal_capability, *cap, "{p}");
            assert_eq!(d.has_thermal, cap.is_some(), "{p}");
            assert_eq!(d.watchdog_max_period, *wd, "{p}");
            assert_eq!(d.has_watchdog, wd.is_some(), "{p}");
            assert_eq!(d.max_port_per_line_card, *lc, "{p}");
            assert_eq!(d.fw_control_ports, *fw, "{p}");
            assert_eq!(d.has_sfp, lc.is_some() || fw.is_some(), "{p}");
        }
        assert!(device_data("x86_64-nvidia_sn9999-r0").is_none());
    }

    /// `check` on each platform in turn, each under its own root; nothing
    /// when `$PLATFORM` overrides the name.
    fn on(name: &str, check: impl FnOnce()) {
        let root = Root::new();
        if platform(&root, name).as_deref() == Some(name) {
            check();
        }
    }

    #[test]
    fn answers_from_the_platform_name() {
        on("x86_64-mlnx_msn2700-r0", || {
            assert_eq!(get_platform_name().unwrap().as_deref(), Some("x86_64-mlnx_msn2700-r0"));
            assert!(!is_simx_platform().unwrap());
            assert!(is_spc1().unwrap());
            assert_eq!(get_thermal_capability().unwrap(), Some(&[("comex_amb", false)][..]));
            assert_eq!(get_watchdog_max_period().unwrap(), Some(DEFAULT_WD_PERIOD));
            assert_eq!(get_always_fw_control_ports().unwrap(), None);
            assert_eq!(get_linecard_max_port_count().unwrap(), 0);
        });
        on("x86_64-mlnx_msn2100-r0", || {
            assert_eq!(get_watchdog_max_period().unwrap(), Some(32));
            assert!(!is_spc1().unwrap());
        });
        on("x86_64-mlnx_msn2700a1-r0", || assert!(is_spc1().unwrap()));
        on("x86_64-nvidia_sn5600-r0", || assert_eq!(get_always_fw_control_ports().unwrap(), Some(&[64][..])));
        on("x86_64-nvidia_sn4800-r0", || {
            assert_eq!(get_linecard_max_port_count().unwrap(), 16);
            assert_eq!(get_always_fw_control_ports().unwrap(), None);
        });
        on("x86_64-mlnx_msn3420-r0", || {
            assert_eq!(get_thermal_capability().unwrap(), None);
            assert_eq!(get_linecard_max_port_count().unwrap(), 0);
            assert_eq!(get_watchdog_max_period().unwrap(), Some(DEFAULT_WD_PERIOD));
        });
        on("x86_64-nvidia_sn5640_simx-r0", || assert!(is_simx_platform().unwrap()));
        on("x86_64-nvidia_unknown-r0", || {
            assert_eq!(get_thermal_capability().unwrap(), None);
            assert_eq!(get_watchdog_max_period().unwrap(), Some(DEFAULT_WD_PERIOD));
            assert_eq!(get_always_fw_control_ports().unwrap(), None);
            assert_eq!(get_linecard_max_port_count().unwrap(), 0);
        });
    }

    #[test]
    fn without_a_platform_name_nothing_is_known() {
        let _root = Root::new();
        if device_info::get_platform().unwrap().is_some() {
            return;
        }
        assert_eq!(get_platform_name().unwrap(), None);
        assert!(!is_simx_platform().unwrap() && !is_spc1().unwrap());
        assert_eq!(get_thermal_capability().unwrap(), None);
        assert_eq!(get_watchdog_max_period().unwrap(), Some(DEFAULT_WD_PERIOD));
        assert_eq!(get_always_fw_control_ports().unwrap(), None);
    }

    #[test]
    fn counts_come_from_hw_management() {
        let root = Root::new();
        // Nothing there: zeros, and recomputed every time.
        assert_eq!(get_fan_drawer_count(), 0);
        assert_eq!(get_fan_count(), 0);
        assert_eq!(get_psu_count(), 0);
        assert!(!is_fan_hotswapable() && !is_psu_hotswapable());
        assert_eq!(get_pdb_count(), 0);
        assert_eq!(get_linecard_count(), 0);
        assert_eq!(get_cpu_thermal_count(), 0);
        for i in 1..=4 {
            root.write(&format!("{HW}/thermal/fan{i}_status"), "1");
            root.write(&format!("{HW}/thermal/fan{i}_speed_get"), "1000");
        }
        root.write(&format!("{HW}/thermal/fan5_speed_get"), "1000");
        // Fixed fans: one drawer.
        assert_eq!(get_fan_drawer_sysfs_count(), 4);
        assert_eq!(get_fan_drawer_count(), 1);
        assert_eq!(get_fan_count(), 5);
        root.write(&format!("{HW}/config/hotplug_fans"), "4");
        forget_caches();
        assert!(is_fan_hotswapable());
        assert_eq!(get_fan_drawer_count(), 4);
        // Fixed PSUs: counted by their i2c addresses.
        root.write(&format!("{HW}/config/psu1_i2c_addr"), "0x59");
        root.write(&format!("{HW}/config/psu2_i2c_addr"), "0x58");
        assert_eq!(get_psu_count(), 2);
        assert!(!is_psu_hotswapable());
        root.write(&format!("{HW}/config/hotplug_psus"), "3");
        forget_caches();
        assert_eq!(get_psu_count(), 3);
        assert!(is_psu_hotswapable());
        root.write("/var/run/hw-management/config/hotplug_pdbs", "2");
        assert_eq!(get_pdb_count(), 2);
        root.write(&format!("{HW}/config/hotplug_linecards"), "8");
        assert_eq!(get_linecard_count(), 8);
        root.write(&format!("{HW}/lc3/config/module_counter"), "16");
        assert_eq!(get_linecard_sfp_count(3), 16);
        assert_eq!(get_linecard_sfp_count(4), 0);
        root.write("/x/gearbox_counter", "2");
        assert_eq!(get_gearbox_count("/x"), 2);
        assert_eq!(get_gearbox_count("/y"), 0);
        // `cpu_core[!_]`, relative to the working directory: the root here.
        for f in ["cpu_core0", "cpu_core1", "cpu_core_max", "cpu_pack"] {
            root.write(&format!("run/hw-management/thermal/{f}"), "1");
        }
        assert_eq!(get_cpu_thermal_count(), 2);
    }

    #[test]
    fn platform_json_gives_sfp_and_dpu_counts() {
        let root = Root::new();
        // No platform at all: `os.path.join(..., None)`.
        if device_info::get_platform().unwrap().is_none() {
            assert_eq!(get_sfp_count().unwrap_err().exc.name(), "TypeError");
            assert_eq!(get_platform_dpus_data().unwrap_err().exc.name(), "TypeError");
            assert_eq!(get_dpu_count().unwrap_err().exc.name(), "TypeError");
        }
        let Some(_) = platform(&root, "x86_64-nvidia_sn4280-r0") else { return };
        let dir = platform_dir(&root);
        // No platform.json: load_json_file's None, subscripted or `.get`.
        forget_caches();
        assert_eq!(get_sfp_count().unwrap_err().exc.name(), "TypeError");
        let e = get_platform_dpus_data().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("AttributeError", "'NoneType' object has no attribute 'get'".to_string()));
        root.write(&format!("{dir}/platform.json"), r#"{"chassis": {"name": "x"}}"#);
        forget_caches();
        assert_eq!(get_sfp_count().unwrap_err().str(), "'sfps'");
        assert_eq!(get_platform_dpus_data().unwrap(), None);
        assert_eq!(get_dpu_count().unwrap(), 0);
        root.write(
            &format!("{dir}/platform.json"),
            r#"{"chassis": {"sfps": [1, 2, 3]}, "DPUS": {"dpu0": {"midplane_interface": "dpu0"}, "dpu1": {}, "dpu2": {}}}"#,
        );
        forget_caches();
        assert_eq!(get_sfp_count().unwrap(), 3);
        assert_eq!(get_dpu_count().unwrap(), 3);
        let dpus = get_platform_dpus_data().unwrap().unwrap();
        assert_eq!(dpus.iter_items().unwrap().len(), 3);
        // Cached once truthy.
        root.write(&format!("{dir}/platform.json"), "{}");
        assert_eq!(get_sfp_count().unwrap(), 3);
        assert_eq!(get_dpu_count().unwrap(), 3);
        // What `len()` and `.get` make of values that are not what is meant.
        root.write(&format!("{dir}/platform.json"), r#"{"chassis": {"sfps": null}, "DPUS": 2}"#);
        forget_caches();
        let err = |e: platform_base::PyErr| (e.exc.name().to_string(), e.str());
        assert_eq!(err(get_sfp_count().unwrap_err()), ("TypeError".into(), "object of type 'NoneType' has no len()".into()));
        assert_eq!(err(get_dpu_count().unwrap_err()), ("TypeError".into(), "object of type 'int' has no len()".into()));
        root.write(&format!("{dir}/platform.json"), "[1]");
        forget_caches();
        assert_eq!(err(get_platform_dpus_data().unwrap_err()), ("AttributeError".into(), "'list' object has no attribute 'get'".into()));
    }

    #[test]
    fn module_host_management_mode_is_in_sai_profile() {
        let root = Root::new();
        let Some(_) = platform(&root, "x86_64-nvidia_sn5600-r0") else { return };
        let dir = platform_dir(&root);
        let sku = hwsku(&root);
        assert_eq!(get_asic_count().unwrap(), 1);
        assert!(!is_multi_asic_platform().unwrap());
        assert!(!is_module_host_management_mode().unwrap());
        root.write(&format!("{dir}/{sku}/sai.profile"), "SAI_INDEPENDENT_MODULE_MODE=0\n");
        assert!(!is_module_host_management_mode().unwrap());
        root.write(&format!("{dir}/{sku}/sai.profile"), "SAI_KEY=1\nSAI_INDEPENDENT_MODULE_MODE=1\n");
        assert!(is_module_host_management_mode().unwrap());
        // A multi-ASIC platform reads ASIC 0's.
        root.write(&format!("{dir}/asic.conf"), "NUM_ASIC=2\n");
        forget_caches();
        assert_eq!(get_asic_count().unwrap(), 2);
        assert!(is_multi_asic_platform().unwrap());
        assert!(!is_module_host_management_mode().unwrap());
        root.write(&format!("{dir}/{sku}/0/sai.profile"), "SAI_INDEPENDENT_MODULE_MODE=1\n");
        assert!(is_module_host_management_mode().unwrap());
        // asic.conf without num_asic: Python's UnboundLocalError.
        root.write(&format!("{dir}/asic.conf"), "OTHER=1\n");
        forget_caches();
        assert_eq!(get_asic_count().unwrap_err().exc.name(), "UnboundLocalError");
        assert_eq!(is_module_host_management_mode().unwrap_err().exc.name(), "UnboundLocalError");
    }

    #[test]
    fn a_bmc_platform_is_a_switch_host_with_bmc_data() {
        let root = Root::new();
        assert!(!is_platform_with_bmc().unwrap());
        root.write("/usr/share/sonic/platform/platform_env.conf", "switch_host=1\n");
        assert!(!is_platform_with_bmc().unwrap());
        root.write(device_info::GLOBAL_BMC_DATA_FILE, r#"{"bmc_addr": "169.254.0.1"}"#);
        assert!(is_platform_with_bmc().unwrap());
    }

    /// A conf file that will not read raises, as Python's reading of it does,
    /// and nothing is cached: the next call reads it again.
    #[test]
    fn a_conf_file_that_will_not_read_raises_and_is_not_cached() {
        let root = Root::new();
        let env = "/usr/share/sonic/platform/platform_env.conf";
        root.write(env, "");
        std::fs::write(root.real(env), b"switch_host=1\xff\n").unwrap();
        root.write(device_info::GLOBAL_BMC_DATA_FILE, r#"{"bmc_addr": "169.254.0.1"}"#);
        assert_eq!(is_platform_with_bmc().unwrap_err().exc.name(), "UnicodeDecodeError");
        root.write(env, "switch_host=1\n");
        assert!(is_platform_with_bmc().unwrap());
        let simx = "x86_64-nvidia_sn5640_simx-r0";
        if platform(&root, simx).as_deref() != Some(simx) {
            return;
        }
        std::fs::write(root.real(device_info::MACHINE_CONF_PATH), b"onie_platform=\xff\n").unwrap();
        assert_eq!(get_platform_name().unwrap_err().exc.name(), "UnicodeDecodeError");
        assert_eq!(is_simx_platform().unwrap_err().exc.name(), "UnicodeDecodeError");
        root.write(device_info::MACHINE_CONF_PATH, &format!("onie_platform={simx}\n"));
        assert!(is_simx_platform().unwrap());
    }

    #[test]
    fn sysfs_nodes_are_ready_when_readable() {
        let root = Root::new();
        assert!(!check_sysfs_access("/x/missing"));
        std::fs::create_dir_all(root.real("/x/dir")).unwrap();
        assert!(check_sysfs_access("/x/dir"));
        root.write("/x/file", "1");
        assert!(check_sysfs_access("/x/file"));
        // A node that exists but cannot be opened (a dangling-to-dir mode is
        // not available unprivileged): a broken link is not there at all.
        std::os::unix::fs::symlink("/nowhere", root.real("/x/broken")).unwrap();
        assert!(!check_sysfs_access("/x/broken"));
        let Some(_) = platform(&root, "x86_64-nvidia_sn5600-r0") else { return };
        let dir = platform_dir(&root);
        let sku = hwsku(&root);
        for m in 0..2 {
            for n in ["present", "status", "statuserror"] {
                root.write(&format!("/sys/module/sx_core/asic0/module{m}/{n}"), "1");
            }
        }
        assert!(wait_sysfs_ready(2).unwrap());
        assert!(wait_sysfs_ready(0).unwrap());
        // Host management also waits for control and power_on.
        root.write(&format!("{dir}/{sku}/sai.profile"), "SAI_INDEPENDENT_MODULE_MODE=1\n");
        forget_caches();
        for m in 0..2 {
            for n in ["control", "power_on"] {
                root.write(&format!("/sys/module/sx_core/asic0/module{m}/{n}"), "0");
            }
        }
        assert!(wait_sysfs_ready(2).unwrap());
    }
}
