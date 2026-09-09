//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// Apache-2.0
//

//! Power distribution board readings, mirroring `pdb.py`.
//!
//! A PDB is published by `psud` alongside the PSUs under its own key template
//! (`PDB {n}`), through the same accessor names — which is why it shares
//! [`PsuInfo`] rather than getting a struct of its own.
//!
//! Two things separate it from a PSU.  Its current and power readings carry a
//! **scale factor in a sibling file**, applied before the divisor, because the
//! hot-swap controller's units vary by part.  And its status LED is not per
//! device: every PDB reports the one aggregate `led_power` file verbatim,
//! bypassing the capability/colour machinery in `led.rs` entirely.

use platform_traits::{PowerEntityKind, PsuInfo};

use crate::utils;

const CURRENT_SCALE_DIVISOR: f64 = 1000.0;
const POWER_SCALE_DIVISOR: f64 = 1_000_000.0;
const TEMP_SCALE: f64 = 1000.0;
/// Python's `LED_DISPLAY_NA`.
const LED_DISPLAY_NA: &str = "N/A";

/// Read one PDB.  `index` is 1-based, as Python's `pdb_index + 1`.
pub fn read_pdb(base: &str, index: usize) -> PsuInfo {
    let env = format!("{base}/environment/pdb_hotswap{index}");
    let thermal = format!("{base}/thermal/pdb_hotswap{index}_temp1");

    // `get_status()` and `get_powergood_status()` are the same read; presence is
    // hard-coded True, so the guard in front of it never fires.
    let power_good = utils::read_int(&format!("{base}/system/pdb{index}_pwr_status")) == Some(1);

    // The temperature comes from a Thermal built in __init__, and only if the
    // input file existed *then*.  Reading it directly here is equivalent while
    // the tree is stable and skips the discovery step.
    let temperature = utils::read_float(&format!("{thermal}_input")).map(|v| v / TEMP_SCALE);

    let input_voltage = read_plain(&format!("{env}_in1_input"), 1000.0);
    let input_current = read_scaled(
        &format!("{env}_curr1_input"),
        &format!("{env}_curr1_scale"),
        CURRENT_SCALE_DIVISOR,
    );
    let input_power = read_scaled(
        &format!("{env}_power1_input"),
        &format!("{env}_power1_scale"),
        POWER_SCALE_DIVISOR,
    );

    PsuInfo {
        name: format!("PDB {index}"),
        kind: PowerEntityKind::Pdb,
        position_in_parent: index as u32,
        // `get_presence()` returns True unconditionally: a PDB is not a
        // field-replaceable part on these platforms.
        presence: true,
        power_good,
        is_replaceable: false,
        // 'N/A' literals in Python.
        model: None,
        serial: None,
        revision: None,
        status_led: Some(power_led(base)),

        // `get_voltage`/`get_current`/`get_power` are aliases of the input
        // readings (`pdb.py:149-156`) — a PDB has no separate output side, so
        // both halves publish the same numbers rather than one half publishing
        // `N/A`.
        voltage: input_voltage,
        current: input_current,
        power: input_power,
        input_voltage,
        input_current,
        input_power,
        temperature,
        // Not overridden: `PdbBase.get_temperature_high_threshold` and the
        // voltage limits all raise, so Python publishes `N/A`.
        temperature_high_threshold: None,
        voltage_high_threshold: None,
        voltage_low_threshold: None,
        maximum_supplied_power: read_plain(&format!("{env}_power1_max"), POWER_SCALE_DIVISOR),
        power_warning_suppress_threshold: None,
        power_critical_threshold: None,
    }
}

/// `_read_scaled_sensor`: absent input file means no reading at all, and the
/// scale file defaults to 1.0 when missing.
///
/// Note the asymmetry with the PSU path: here a missing *input* file yields
/// `None`, because Python checks `os.path.exists` before reading.  On a PSU the
/// same situation yields `0.0`.
fn read_scaled(input: &str, scale: &str, divisor: f64) -> Option<f64> {
    if !utils::exists(input) {
        return None;
    }
    let val = utils::read_int(input).unwrap_or(0) as f64;
    let factor = if utils::exists(scale) {
        utils::read_float(scale).unwrap_or(1.0)
    } else {
        1.0
    };
    Some(val * factor / divisor)
}

/// The `os.path.exists` guard plus a plain divide, for the files with no scale.
fn read_plain(input: &str, divisor: f64) -> Option<f64> {
    utils::exists(input).then(|| utils::read_int(input).unwrap_or(0) as f64 / divisor)
}

/// `get_status_led`: the aggregate power LED, verbatim.
///
/// Unlike every other LED on the platform this one is read as text and not
/// mapped through the capability list, so it can report colours — and blink
/// suffixes — that `led.rs` would collapse.  `none` and an empty file both mean
/// `N/A`.
fn power_led(base: &str) -> String {
    match utils::read_string(&format!("{base}/led/led_power")) {
        Some(t) if !t.eq_ignore_ascii_case("none") => t,
        _ => LED_DISPLAY_NA.to_string(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn tree(files: &[(&str, &str)]) -> tempfile::TempDir {
        let dir = tempfile::tempdir().unwrap();
        for sub in ["thermal", "environment", "system", "led"] {
            std::fs::create_dir_all(dir.path().join(sub)).unwrap();
        }
        for (name, body) in files {
            std::fs::write(dir.path().join(name), body).unwrap();
        }
        dir
    }

    fn read(dir: &tempfile::TempDir, index: usize) -> PsuInfo {
        read_pdb(&dir.path().to_string_lossy(), index)
    }

    /// A PDB is not field-replaceable and is always reported present, so
    /// `get_status()`'s presence guard never fires.
    #[test]
    fn a_pdb_is_always_present_and_never_replaceable() {
        let info = read(&tree(&[]), 1);
        assert!(info.presence);
        assert!(!info.is_replaceable);
        assert_eq!(info.kind, PowerEntityKind::Pdb);
        assert_eq!(info.name, "PDB 1");
    }

    #[test]
    fn powergood_comes_from_the_system_status_file() {
        assert!(!read(&tree(&[("system/pdb1_pwr_status", "0\n")]), 1).power_good);
        assert!(read(&tree(&[("system/pdb1_pwr_status", "1\n")]), 1).power_good);
    }

    /// The scale factor sits in a sibling file and is applied *before* the
    /// divisor.  Ignoring it would publish a number out by whatever the
    /// hot-swap controller's units happen to be.
    #[test]
    fn current_and_power_apply_the_scale_file() {
        let dir = tree(&[
            ("environment/pdb_hotswap1_curr1_input", "15400\n"),
            ("environment/pdb_hotswap1_curr1_scale", "2.0\n"),
            ("environment/pdb_hotswap1_power1_input", "302600000\n"),
            ("environment/pdb_hotswap1_power1_scale", "0.5\n"),
        ]);
        let info = read(&dir, 1);
        assert_eq!(info.input_current, Some(30.8));
        assert_eq!(info.input_power, Some(151.3));
        assert_eq!(info.power, Some(151.3));
    }

    /// A missing scale file means a factor of 1.0, not a missing reading.
    #[test]
    fn a_missing_scale_file_defaults_to_one() {
        let dir = tree(&[("environment/pdb_hotswap1_curr1_input", "15400\n")]);
        assert_eq!(read(&dir, 1).input_current, Some(15.4));
    }

    /// Unlike a PSU, whose readers publish `0.0` when the file is gone, a PDB
    /// checks `os.path.exists` first — so an absent input file is `N/A`.
    #[test]
    fn an_absent_input_file_is_absent_not_zero() {
        let info = read(&tree(&[]), 1);
        assert_eq!(info.input_current, None);
        assert_eq!(info.input_power, None);
        assert_eq!(info.input_voltage, None);
        assert_eq!(info.maximum_supplied_power, None);
    }

    /// A PDB has no separate output side: `get_voltage`/`get_current`/
    /// `get_power` alias the input readings rather than reporting `N/A`.
    #[test]
    fn the_output_readings_alias_the_input_ones() {
        let dir = tree(&[
            ("environment/pdb_hotswap1_in1_input", "12000\n"),
            ("environment/pdb_hotswap1_curr1_input", "15400\n"),
            ("environment/pdb_hotswap1_power1_input", "302600000\n"),
        ]);
        let info = read(&dir, 1);
        assert_eq!(info.voltage, info.input_voltage);
        assert_eq!(info.current, info.input_current);
        assert_eq!(info.power, info.input_power);
        assert_eq!(info.voltage, Some(12.0));
    }

    #[test]
    fn the_temperature_carries_the_thousandth_scale() {
        let dir = tree(&[("thermal/pdb_hotswap1_temp1_input", "42125\n")]);
        assert_eq!(read(&dir, 1).temperature, Some(42.125));
    }

    /// Not overridden on Mellanox, so Python publishes `N/A` for all of them.
    #[test]
    fn a_pdb_publishes_no_thresholds() {
        let dir = tree(&[("thermal/pdb_hotswap1_temp1_max", "70000\n")]);
        let info = read(&dir, 1);
        assert_eq!(info.temperature_high_threshold, None);
        assert_eq!(info.voltage_high_threshold, None);
        assert_eq!(info.voltage_low_threshold, None);
        assert_eq!(info.power_critical_threshold, None);
    }

    /// The power LED is read as text and published verbatim — it never goes
    /// through the capability/primary-colour mapping the other LEDs use, so it
    /// can report a colour `led.rs` would collapse.
    #[test]
    fn the_power_led_is_reported_verbatim() {
        let dir = tree(&[("led/led_power", "orange_blink\n")]);
        assert_eq!(read(&dir, 1).status_led.as_deref(), Some("orange_blink"));
    }

    #[test]
    fn an_empty_or_none_power_led_is_not_available() {
        assert_eq!(
            read(&tree(&[("led/led_power", "none\n")]), 1).status_led.as_deref(),
            Some("N/A")
        );
        assert_eq!(
            read(&tree(&[("led/led_power", "\n")]), 1).status_led.as_deref(),
            Some("N/A")
        );
        assert_eq!(read(&tree(&[]), 1).status_led.as_deref(), Some("N/A"));
    }

    /// The files are indexed by the 1-based PDB number, so PDB 2 must not read
    /// PDB 1's tree.
    #[test]
    fn each_pdb_reads_its_own_files() {
        let dir = tree(&[
            ("environment/pdb_hotswap1_in1_input", "12000\n"),
            ("environment/pdb_hotswap2_in1_input", "48000\n"),
        ]);
        assert_eq!(read(&dir, 1).input_voltage, Some(12.0));
        assert_eq!(read(&dir, 2).input_voltage, Some(48.0));
        assert_eq!(read(&dir, 2).name, "PDB 2");
    }
}
