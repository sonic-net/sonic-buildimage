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

//! Platform capability table — port of Python's `device_data.py`.
//!
//! Each entry maps a full SONiC platform string
//! (e.g. `"x86_64-nvidia_sn5640-r0"`) to the set of thermal sensors that
//! actually exist on that board.  Unknown platforms fall back to the default
//! (conservative: assume all sensors are present).

use crate::utils;

// ── Capability struct ─────────────────────────────────────────────────────────

/// Per-platform thermal sensor capability flags.
///
/// A `false` value means that sysfs file does NOT exist on that SKU and the
/// corresponding `ThermalEntry` must be omitted from `discover_thermals()`.
///
/// This mirrors Python's `DeviceDataManager.get_thermal_capability()` dict.
#[derive(Debug, Clone)]
pub struct ThermalCapability {
    // ── Single ambient sensors ──────────────────────────────────────────────
    /// `port_amb` — ambient temperature at the port side.
    /// Almost always present; absent on liquid-cooled variants (SN6600-LD, SN6810-LD).
    pub port_amb: bool,
    /// `fan_amb` — ambient temperature at the fan side.
    pub fan_amb: bool,
    /// `comex_amb` — COMEX module ambient temperature.
    /// Absent on most SN4000 and all SN5000+ generations.
    pub comex_amb: bool,
    /// `cpu_pack` — CPU package temperature.
    /// Absent on low-power / simulation SKUs.
    pub cpu_pack: bool,
    /// `cpu_amb` — CPU board ambient temperature.
    /// Present only on a few SKUs (SN2201).
    pub cpu_amb: bool,
    /// `swb_amb` — switch board ambient temperature.
    /// Currently absent on all shipping SKUs in DEVICE_DATA.
    pub swb_amb: bool,
    /// `pch_temp` — Platform Controller Hub temperature.
    /// Present on SN5400, SN5600.
    pub pch_temp: bool,

    // ── ASIC default thresholds (in °C, integer — matching Python `int`) ────
    /// Written to STATE_DB as `"105"` (no decimal), NOT `"105.0"`.
    pub asic_warn_default: i64,
    /// Written to STATE_DB as `"120"`.
    pub asic_crit_default: i64,
}

impl Default for ThermalCapability {
    fn default() -> Self {
        Self {
            port_amb:          true,
            fan_amb:           true,
            comex_amb:         true,
            cpu_pack:          true,
            cpu_amb:           false,
            swb_amb:           false,
            pch_temp:          false,
            asic_warn_default: 105,
            asic_crit_default: 120,
        }
    }
}

// ── Capability lookup ─────────────────────────────────────────────────────────

/// Return the thermal capability for the given SONiC platform string.
///
/// Platform strings are matched exactly against the `DEVICE_DATA` table
/// (case-sensitive, as in Python).  Unknown platforms fall back to
/// `ThermalCapability::default()`.
pub fn get_thermal_capability(platform: &str) -> ThermalCapability {
    let d = ThermalCapability::default();
    match platform {
        // ── SN2000 generation ────────────────────────────────────────────────
        "x86_64-mlnx_msn2700-r0" => ThermalCapability { comex_amb: false, ..d },
        "x86_64-mlnx_msn2700a1-r0" => d, // comex_amb: True (same as default)
        "x86_64-mlnx_msn2740-r0" => ThermalCapability { cpu_pack: false, comex_amb: false, ..d },
        "x86_64-mlnx_msn2100-r0" => ThermalCapability { cpu_pack: false, comex_amb: false, ..d },
        "x86_64-mlnx_msn2410-r0" => ThermalCapability { comex_amb: false, ..d },
        "x86_64-mlnx_msn2010-r0" => ThermalCapability { cpu_pack: false, comex_amb: false, ..d },
        // ── SN3000 / SN4000 generation ───────────────────────────────────────
        "x86_64-mlnx_msn3700-r0" |
        "x86_64-mlnx_msn3700c-r0" |
        "x86_64-mlnx_msn3800-r0" |
        "x86_64-mlnx_msn4700-r0" |
        "x86_64-mlnx_msn4410-r0" |
        "x86_64-mlnx_msn3420-r0" |
        "x86_64-mlnx_msn4600c-r0" |
        "x86_64-mlnx_msn4600-r0" => d,
        "x86_64-mlnx_msn4700_simx-r0" => ThermalCapability { cpu_pack: false, ..d },
        // ── SN4000 NVIDIA-branded ────────────────────────────────────────────
        "x86_64-nvidia_sn4280-r0" => ThermalCapability { comex_amb: false, ..d },
        "x86_64-nvidia_sn4280_simx-r0" => ThermalCapability { cpu_pack: false, comex_amb: false, ..d },
        "x86_64-nvidia_sn4800-r0" => ThermalCapability { comex_amb: false, ..d },
        // ── SN2201 ──────────────────────────────────────────────────────────
        "x86_64-nvidia_sn2201-r0" => ThermalCapability {
            comex_amb: false,
            cpu_amb: true,
            ..d
        },
        // ── SN5000 generation ────────────────────────────────────────────────
        "x86_64-nvidia_sn5400-r0" => ThermalCapability {
            comex_amb: false,
            pch_temp: true,
            ..d
        },
        "x86_64-nvidia_sn5600-r0" => ThermalCapability {
            comex_amb: false,
            pch_temp: true,
            ..d
        },
        "x86_64-nvidia_sn5600_simx-r0" => ThermalCapability {
            cpu_pack: false,
            comex_amb: false,
            ..d
        },
        "x86_64-nvidia_sn5610n-r0" => ThermalCapability { comex_amb: false, ..d },
        "x86_64-nvidia_sn5640-r0" => ThermalCapability { comex_amb: false, ..d },
        "x86_64-nvidia_sn5640_simx-r0" => ThermalCapability {
            cpu_pack: false,
            comex_amb: false,
            ..d
        },
        // ── Liquid-cooled (LD) variants ───────────────────────────────────────
        "x86_64-nvidia_sn6600_ld-r0" |
        "x86_64-nvidia_sn6810_ld-r0" |
        "x86_64-nvidia_sn6810_ld_simx-r0" => ThermalCapability {
            port_amb:  false,
            fan_amb:   false,
            comex_amb: false,
            ..d
        },
        // ── Unknown: use conservative defaults ───────────────────────────────
        _ => d,
    }
}

// ── Hardware count helpers ─────────────────────────────────────────────────────

/// Number of ASICs, derived from `/sys/module/sx_core/asic*/temperature/input`.
/// Falls back to 1 when `sx_core` is not loaded (unit tests, early boot).
pub fn get_asic_count() -> usize {
    let n = utils::glob_count("/sys/module/sx_core/asic*/temperature/input");
    if n == 0 { 1 } else { n }
}

/// Whether this platform uses a multi-ASIC numbering scheme.
pub fn is_multi_asic() -> bool {
    get_asic_count() > 1
}

/// Number of power distribution boards, from `config/hotplug_pdbs`.
///
/// Only the liquid-cooled platforms have any: SN6600_LD and N6300_LD report 2.
/// A missing file means none, as Python's `default = 0` does.
pub fn get_pdb_count() -> usize {
    pdb_count_in(utils::HW_MGMT_CONFIG)
}

/// The same count under any config directory.
pub fn pdb_count_in(config: &str) -> usize {
    utils::read_int(&format!("{config}/hotplug_pdbs"))
        .unwrap_or(0)
        .max(0) as usize
}

/// Python's `is_psu_hotswapable()`: a platform with `hotplug_psus > 0` builds
/// `Psu` objects, one without builds `FixedPsu` (`chassis.py:192-200`).  The
/// two differ in far more than replaceability — a fixed PSU publishes no
/// electrical readings at all.
pub fn is_psu_hotswappable() -> bool {
    psu_hotswappable_in(utils::HW_MGMT_CONFIG)
}

/// The same answer under any config directory.
pub fn psu_hotswappable_in(config: &str) -> bool {
    utils::read_int(&format!("{config}/hotplug_psus")).unwrap_or(0) > 0
}

/// Number of gearbox thermal sensors.
pub fn get_gearbox_count() -> usize {
    gearbox_count_in(utils::HW_MGMT_THERMAL)
}

/// The same count under any thermal directory.
pub fn gearbox_count_in(thermal: &str) -> usize {
    utils::glob_count(&format!("{thermal}/gearbox*_temp_input"))
}

/// Number of SODIMM thermal sensors, resolved at runtime via glob.
///
/// Returns a sorted list of 1-based SODIMM indices to match Python's
/// `create_discrete_thermal()` ordering.
pub fn get_sodimm_indices() -> Vec<usize> {
    sodimm_indices_in(utils::HW_MGMT_THERMAL)
}

/// The same list under any thermal directory.
pub fn sodimm_indices_in(thermal: &str) -> Vec<usize> {
    let mut indices = Vec::new();
    if let Ok(entries) = glob::glob(&format!("{thermal}/sodimm*_temp_input")) {
        for entry in entries.flatten() {
            let fname = entry.file_name().unwrap_or_default().to_string_lossy().into_owned();
            // sodimm{N}_temp_input → extract N
            if let Some(rest) = fname.strip_prefix("sodimm") {
                if let Some(idx_str) = rest.strip_suffix("_temp_input") {
                    if let Ok(idx) = idx_str.parse::<usize>() {
                        indices.push(idx);
                    }
                }
            }
        }
    }
    indices.sort_unstable();
    indices
}

/// Number of PSUs.
///
/// `config/hotplug_psus` where the platform has hot-swappable PSUs, else the
/// number of `config/psu*_i2c_addr` files, which is what a fixed-PSU platform
/// exposes instead (`device_data.py:289-292`).  There is no default: inventing
/// one publishes PSU thermals and fans for hardware that may not be there.
pub fn get_psu_count() -> usize {
    psu_count_in(utils::HW_MGMT_CONFIG)
}

/// The same count under any config directory.
pub fn psu_count_in(config: &str) -> usize {
    let hotplug = utils::read_int(&format!("{config}/hotplug_psus")).unwrap_or(0);
    if hotplug > 0 {
        return hotplug as usize;
    }
    utils::glob_count(&format!("{config}/psu*_i2c_addr"))
}

/// Whether fan drawers are hot-swappable (pluggable drawer modules).
pub fn is_fan_hotswappable() -> bool {
    fan_hotswappable_in(utils::HW_MGMT_CONFIG)
}

/// The same answer under any config directory.
pub fn fan_hotswappable_in(config: &str) -> bool {
    utils::read_int(&format!("{config}/hotplug_fans"))
        .map(|v| v > 0)
        .unwrap_or(false)
}

/// Count fan drawers (= number of fan*_status files).
pub fn get_fan_drawer_count() -> usize {
    fan_drawer_count_in(utils::HW_MGMT_THERMAL)
}

/// The same count under any thermal directory.
pub fn fan_drawer_count_in(thermal: &str) -> usize {
    utils::glob_count(&format!("{thermal}/fan*_status"))
}

/// Count individual fans (= number of fan*_speed_get files).
pub fn get_fan_count() -> usize {
    fan_count_in(utils::HW_MGMT_THERMAL)
}

/// The same count under any thermal directory.
pub fn fan_count_in(thermal: &str) -> usize {
    utils::glob_count(&format!("{thermal}/fan*_speed_get"))
}

#[cfg(test)]
mod tests {
    use super::*;

    // ── get_thermal_capability ────────────────────────────────────────────

    /// Which sensors a platform has decides which STATE_DB rows exist, so an
    /// entry that falls through to the default silently publishes sensors the
    /// box does not have — every one of them reading `N/A`.
    #[test]
    fn an_unknown_platform_falls_back_to_the_defaults() {
        let d = ThermalCapability::default();
        let got = get_thermal_capability("x86_64-some_vendor_future-r0");
        assert_eq!(got.port_amb, d.port_amb);
        assert_eq!(got.comex_amb, d.comex_amb);
        assert_eq!(got.cpu_pack, d.cpu_pack);
    }

    /// The match is exact and case-sensitive, as Python's dict lookup is: a
    /// near-miss is an unknown platform, not a close one.
    #[test]
    fn the_platform_string_is_matched_exactly() {
        assert!(!get_thermal_capability("x86_64-nvidia_sn5640-r0").comex_amb);
        // Wrong case and a missing suffix both fall through to the default,
        // where comex_amb is true.
        assert!(get_thermal_capability("X86_64-NVIDIA_SN5640-R0").comex_amb);
        assert!(get_thermal_capability("x86_64-nvidia_sn5640").comex_amb);
    }

    /// COMEX is the sensor that most often differs, and it is absent across the
    /// whole SN5000 generation.
    #[test]
    fn the_sn5000_generation_has_no_comex_sensor() {
        for p in [
            "x86_64-nvidia_sn5400-r0",
            "x86_64-nvidia_sn5600-r0",
            "x86_64-nvidia_sn5610n-r0",
            "x86_64-nvidia_sn5640-r0",
        ] {
            assert!(!get_thermal_capability(p).comex_amb, "{p}");
        }
    }

    /// The SN3000/SN4000 group takes the defaults unchanged, which is what
    /// makes it a group.
    #[test]
    fn the_sn3000_and_sn4000_group_is_the_default_shape() {
        let d = ThermalCapability::default();
        for p in [
            "x86_64-mlnx_msn3700-r0",
            "x86_64-mlnx_msn3800-r0",
            "x86_64-mlnx_msn4700-r0",
            "x86_64-mlnx_msn4600-r0",
        ] {
            let got = get_thermal_capability(p);
            assert_eq!(got.comex_amb, d.comex_amb, "{p}");
            assert_eq!(got.cpu_pack, d.cpu_pack, "{p}");
        }
    }

    /// A simulated platform has no CPU package sensor: simx does not model one,
    /// and publishing it would give every simx run a permanently `N/A` row.
    #[test]
    fn simulated_platforms_have_no_cpu_package_sensor() {
        for p in [
            "x86_64-mlnx_msn4700_simx-r0",
            "x86_64-nvidia_sn4280_simx-r0",
            "x86_64-nvidia_sn5600_simx-r0",
            "x86_64-nvidia_sn5640_simx-r0",
        ] {
            assert!(!get_thermal_capability(p).cpu_pack, "{p}");
        }
    }

    /// A liquid-cooled variant has neither ambient sensor: there is no airflow
    /// to measure, and hw-management does not publish the files.
    #[test]
    fn a_liquid_cooled_variant_has_no_ambient_sensors() {
        for p in [
            "x86_64-nvidia_sn6600_ld-r0",
            "x86_64-nvidia_sn6810_ld-r0",
            "x86_64-nvidia_sn6810_ld_simx-r0",
        ] {
            let c = get_thermal_capability(p);
            assert!(!c.port_amb, "{p}");
            assert!(!c.fan_amb, "{p}");
            assert!(!c.comex_amb, "{p}");
        }
    }

    /// SN2201 is the only platform with a CPU board ambient sensor, and the
    /// PCH sensor belongs to SN5400/SN5600 alone.  Both are single-platform
    /// facts that a copy-paste into a neighbouring arm would silently break.
    #[test]
    fn the_single_platform_sensors_stay_on_their_platform() {
        assert!(get_thermal_capability("x86_64-nvidia_sn2201-r0").cpu_amb);
        assert!(!get_thermal_capability("x86_64-nvidia_sn5600-r0").cpu_amb);

        assert!(get_thermal_capability("x86_64-nvidia_sn5400-r0").pch_temp);
        assert!(get_thermal_capability("x86_64-nvidia_sn5600-r0").pch_temp);
        assert!(!get_thermal_capability("x86_64-nvidia_sn5640-r0").pch_temp);
    }

    /// The ASIC thresholds are Python `int`s, which is why STATE_DB carries
    /// `"105"` and not `"105.0"`.
    #[test]
    fn the_asic_thresholds_are_integers() {
        let c = get_thermal_capability("x86_64-nvidia_sn5640-r0");
        assert_eq!(c.asic_warn_default.to_string(), "105");
        assert_eq!(c.asic_crit_default.to_string(), "120");
        assert!(c.asic_warn_default < c.asic_crit_default);
    }

    // ── The device counts ─────────────────────────────────────────────────

    fn tree(files: &[&str]) -> tempfile::TempDir {
        let d = tempfile::tempdir().unwrap();
        for f in files {
            let p = d.path().join(f);
            std::fs::create_dir_all(p.parent().unwrap()).unwrap();
            std::fs::write(p, "").unwrap();
        }
        d
    }

    fn dir(d: &tempfile::TempDir) -> String {
        d.path().to_string_lossy().into_owned()
    }

    #[test]
    fn the_gearbox_and_fan_counts_are_the_number_of_files() {
        let d = tree(&[
            "gearbox0_temp_input", "gearbox1_temp_input",
            "fan1_status", "fan2_status",
            "fan1_speed_get", "fan2_speed_get", "fan3_speed_get", "fan4_speed_get",
        ]);
        assert_eq!(gearbox_count_in(&dir(&d)), 2);
        assert_eq!(fan_drawer_count_in(&dir(&d)), 2);
        assert_eq!(fan_count_in(&dir(&d)), 4, "four fans across two drawers");
    }

    #[test]
    fn an_empty_tree_counts_nothing() {
        let d = tree(&[]);
        assert_eq!(gearbox_count_in(&dir(&d)), 0);
        assert_eq!(fan_drawer_count_in(&dir(&d)), 0);
        assert_eq!(fan_count_in(&dir(&d)), 0);
        assert_eq!(pdb_count_in(&dir(&d)), 0);
    }

    /// The DIMM indices come back sorted numerically and need not be
    /// contiguous: slot 10 must not sort before slot 2, which a lexical sort
    /// over the file names would do.
    #[test]
    fn sodimm_indices_are_sorted_numerically_and_may_have_gaps() {
        let d = tree(&[
            "sodimm10_temp_input", "sodimm2_temp_input", "sodimm1_temp_input",
        ]);
        assert_eq!(sodimm_indices_in(&dir(&d)), vec![1, 2, 10]);
    }

    /// A file that is not a DIMM sensor is ignored rather than parsed into a
    /// bogus index.
    #[test]
    fn unrelated_files_are_not_taken_for_dimm_sensors() {
        let d = tree(&["sodimm_temp_input", "sodimmX_temp_input", "fan1_status"]);
        assert!(sodimm_indices_in(&dir(&d)).is_empty());
    }

    /// A hot-swappable platform counts PSUs from `hotplug_psus`; a fixed-PSU
    /// one has no such file and is counted from its i2c entries instead.
    /// Inventing a default either way would publish thermals and fans for
    /// hardware that may not be there.
    #[test]
    fn the_psu_count_has_two_sources_and_no_default() {
        let hot = tree(&["hotplug_psus"]);
        std::fs::write(hot.path().join("hotplug_psus"), "2").unwrap();
        assert_eq!(psu_count_in(&dir(&hot)), 2);
        assert!(psu_hotswappable_in(&dir(&hot)));

        let fixed = tree(&["psu1_i2c_addr", "psu2_i2c_addr", "psu3_i2c_addr"]);
        assert_eq!(psu_count_in(&dir(&fixed)), 3);
        assert!(!psu_hotswappable_in(&dir(&fixed)));

        let none = tree(&[]);
        assert_eq!(psu_count_in(&dir(&none)), 0);
    }

    /// `hotplug_psus` of zero means fixed PSUs, not "two sources disagree" —
    /// the count falls through to the i2c entries.
    #[test]
    fn a_zero_hotplug_count_falls_through_to_the_i2c_entries() {
        let d = tree(&["hotplug_psus", "psu1_i2c_addr", "psu2_i2c_addr"]);
        std::fs::write(d.path().join("hotplug_psus"), "0").unwrap();
        assert_eq!(psu_count_in(&dir(&d)), 2);
        assert!(!psu_hotswappable_in(&dir(&d)));
    }

    #[test]
    fn fan_hotswappability_needs_a_positive_count() {
        let d = tree(&["hotplug_fans"]);
        std::fs::write(d.path().join("hotplug_fans"), "4").unwrap();
        assert!(fan_hotswappable_in(&dir(&d)));

        std::fs::write(d.path().join("hotplug_fans"), "0").unwrap();
        assert!(!fan_hotswappable_in(&dir(&d)));

        assert!(!fan_hotswappable_in(&dir(&tree(&[]))), "no file at all");
    }

    /// A negative or unparsable PDB count is clamped rather than wrapping into
    /// a huge `usize`.
    #[test]
    fn a_negative_pdb_count_is_clamped_to_zero() {
        let d = tree(&["hotplug_pdbs"]);
        std::fs::write(d.path().join("hotplug_pdbs"), "-1").unwrap();
        assert_eq!(pdb_count_in(&dir(&d)), 0);

        std::fs::write(d.path().join("hotplug_pdbs"), "nonsense").unwrap();
        assert_eq!(pdb_count_in(&dir(&d)), 0);

        std::fs::write(d.path().join("hotplug_pdbs"), "1").unwrap();
        assert_eq!(pdb_count_in(&dir(&d)), 1);
    }
}
