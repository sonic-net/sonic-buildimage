//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// Apache-2.0
//

//! PSU readings, mirroring `psu.py`.
//!
//! Python splits this in two.  `FixedPsu` is a soldered-in supply: it is always
//! present, reports no electrical values at all, and owns a per-unit LED.
//! `Psu` extends it for hot-swappable units, which read presence from sysfs,
//! publish the full electrical set, and share **one** LED between them.  The
//! chassis picks between them on `hotplug_psus` (`chassis.py:192-200`).
//!
//! Every reader takes the hw-management root as an argument rather than
//! reaching for a constant, so the tests below drive real trees.

use platform_traits::{PowerEntityKind, PsuInfo};

use crate::led::FanLeds;
use crate::utils;
use crate::vpd::{self, VpdParser};

/// Python's `PSU_PATH`.  `/var/run` is a symlink to `/run`, so this names the
/// same tree as [`crate::utils::HW_MGMT_THERMAL`]'s parent.
pub const HW_MGMT_BASE: &str = "/var/run/hw-management";

/// `Psu.PSU_SENSORS_CONF_UPDATER` and the two sensors.conf locations.
const PLATFORM_SENSORS_CONF: &str = "/usr/share/sonic/platform/sensors.conf";
const ETC_SENSORS_CONF: &str = "/etc/sensors.d/sensors.conf";
const PSU_SENSORS_CONF_UPDATER: &str = "/usr/share/sonic/platform/psu_sensors_conf_updater";

/// One PSU and the state Python keeps on its object between calls.
#[derive(Debug)]
pub struct Psu {
    base: String,
    /// 1-based, as Python's `self.index = psu_index + 1`.
    index: usize,
    hotswappable: bool,
    vpd: VpdParser,
    /// Python seeds `self.model` from the VPD in `__init__` and compares
    /// against it on every `get_model()` to decide whether the PSU has been
    /// swapped for a different one.
    last_model: Option<String>,
    /// Read once; the invalid-voltage workaround is scoped to four platforms.
    platform_name: String,
}

impl Psu {
    pub fn new(base: &str, index: usize, hotswappable: bool, platform_name: &str) -> Self {
        let mut vpd = VpdParser::new(format!("{base}/eeprom/psu{index}_vpd"));
        // Python reads the model once in __init__ so the first get_model() has
        // something to compare against and does not fire the rebuild.
        let last_model = vpd.model();
        Self {
            base: base.to_string(),
            index,
            hotswappable,
            vpd,
            last_model,
            platform_name: platform_name.to_string(),
        }
    }

    fn path(&self, rest: &str) -> String {
        format!("{}/{}", self.base, rest)
    }

    /// `thermal/psu{n}_pwr_status == 1`.  Both classes read the same file.
    fn power_good(&self) -> bool {
        utils::read_int(&self.path(&format!("thermal/psu{}_pwr_status", self.index))) == Some(1)
    }

    /// A fixed PSU cannot be absent; a hot-swappable one reads
    /// `thermal/psu{n}_status`.
    fn presence(&self) -> bool {
        if !self.hotswappable {
            return true;
        }
        utils::read_int(&self.path(&format!("thermal/psu{}_status", self.index))) == Some(1)
    }

    /// The output-voltage file, whose name differs across generations.
    ///
    /// Python caches the answer on the instance and never re-resolves it; this
    /// re-checks each cycle, which costs two `stat` calls and means a PSU
    /// swapped for one with the other layout is picked up.  The published value
    /// is the same either way while the platform does not change under us.
    fn voltage_path(&self) -> Option<String> {
        let out2 = self.path(&format!("power/psu{}_volt_out2", self.index));
        if utils::exists(&out2) {
            return Some(out2);
        }
        let plain = self.path(&format!("power/psu{}_volt", self.index));
        utils::exists(&plain).then_some(plain)
    }

    /// Python's `read_int_from_file(path) / divisor` under a powergood guard.
    ///
    /// The default matters: `read_int_from_file` falls back to **0**, so a PSU
    /// that is powered good but whose file is missing publishes `0.0`, not
    /// `N/A`.  Only the powergood check produces `None`.
    fn scaled(&self, rest: &str, divisor: f64) -> Option<f64> {
        self.power_good()
            .then(|| utils::read_int(&self.path(rest)).unwrap_or(0) as f64 / divisor)
    }

    /// Everything `psud` publishes for this PSU.
    ///
    /// Values are computed into locals first: several readers need `&mut self`
    /// for the VPD cache and the rest only `&self`, which cannot be mixed
    /// inside one struct literal.
    pub fn read(&mut self, leds: &FanLeds) -> PsuInfo {
        let n = self.index;
        let power_good = self.power_good();
        let presence = self.presence();
        let led_id = self.led_id();

        // A fixed PSU reports none of the electrical values — `psu.py:88-115`
        // and `:179-217` all `return None` — so the whole block is gated once
        // here rather than each reader returning None for its own reasons.
        let electrical = self.hotswappable;

        let model = self.model();
        let (serial, revision) = if electrical {
            (self.vpd.serial(), self.vpd.revision())
        } else {
            (None, None)
        };

        let (voltage, current, power, input_voltage, input_current) = if electrical {
            (
                self.voltage(),
                self.scaled(&format!("power/psu{n}_curr"), 1000.0),
                self.scaled(&format!("power/psu{n}_power"), 1_000_000.0),
                self.scaled(&format!("power/psu{n}_volt_in"), 1000.0),
                self.scaled(&format!("power/psu{n}_curr_in"), 1000.0),
            )
        } else {
            (None, None, None, None, None)
        };

        let (temperature, temp_high, max_supplied) = if electrical {
            (
                self.scaled(&format!("thermal/psu{n}_temp1"), 1000.0),
                self.scaled(&format!("thermal/psu{n}_temp1_max"), 1000.0),
                self.scaled(&format!("power/psu{n}_power_max"), 1_000_000.0),
            )
        } else {
            (None, None, None)
        };

        let (volt_high, volt_low) = if electrical {
            (self.voltage_threshold("max"), self.voltage_threshold("min"))
        } else {
            (None, None)
        };

        let (warning, critical) = if electrical {
            (self.power_threshold(false), self.power_threshold(true))
        } else {
            (None, None)
        };

        PsuInfo {
            name: format!("PSU {n}"),
            kind: PowerEntityKind::Psu,
            position_in_parent: n as u32,
            presence,
            power_good,
            is_replaceable: self.hotswappable,
            model,
            serial,
            revision,
            status_led: Some(leds.status(&led_id)),
            voltage,
            current,
            power,
            input_voltage,
            input_current,
            // `PsuBase` has no input-power accessor.
            input_power: None,
            temperature,
            temperature_high_threshold: temp_high,
            voltage_high_threshold: volt_high,
            voltage_low_threshold: volt_low,
            maximum_supplied_power: max_supplied,
            power_warning_suppress_threshold: warning,
            power_critical_threshold: critical,
        }
    }

    /// `FixedPsu` uses a per-unit LED, `Psu` the one shared by every PSU.
    ///
    /// The difference is not cosmetic: on a hot-swappable platform every PSU
    /// reports the *same* colour, because `Psu.get_shared_led()` builds one
    /// `PsuLed(None)` for the class (`psu.py:411-415`, `:117-120`).
    fn led_id(&self) -> String {
        if self.hotswappable {
            "psu".to_string()
        } else {
            format!("psu{}", self.index)
        }
    }

    fn voltage(&self) -> Option<f64> {
        let path = self.voltage_path()?;
        self.power_good()
            .then(|| utils::read_int(&path).unwrap_or(0) as f64 / 1000.0)
    }

    /// `get_voltage_high_threshold` / `get_voltage_low_threshold`.
    ///
    /// Gated on a capability file that names which limits the hardware
    /// advertises: the `_min` and `_max` files may exist and still be
    /// meaningless, so the capability text — not the file's presence — decides.
    fn voltage_threshold(&mut self, which: &str) -> Option<f64> {
        let volt = self.voltage_path()?;
        let cap_path = format!("{volt}_capability");
        let limit_path = format!("{volt}_{which}");
        if !utils::exists(&cap_path) || !utils::exists(&limit_path) || !self.power_good() {
            return None;
        }
        let capability = utils::read_string(&cap_path)?;
        if !capability.contains(which) {
            return None;
        }
        let raw = utils::read_int(&limit_path).unwrap_or(0);
        let platform = self.platform_name.clone();
        let raw = invalid_voltage_wa(raw, &limit_path, &platform, &mut self.vpd)?;
        // Python's `if max_voltage:` — a zero threshold is discarded, not
        // published as 0.0.
        (raw != 0).then(|| raw as f64 / 1000.0)
    }

    /// The ambient-temperature-derived power thresholds.
    ///
    /// `critical` picks between the two formulas in `psu.py:554-621`.  They
    /// share every read, differ in which ambient limit they compare against,
    /// and differ in one term:
    ///
    /// ```text
    /// warning:   cool -> capacity - slope*1000
    ///            hot  -> capacity - (1000 + amb - limit) * slope
    /// critical:  cool -> capacity
    ///            hot  -> capacity - (amb - limit) * slope
    /// ```
    ///
    /// `slope` is already multiplied by 1000 when read, so the `slope * 1000`
    /// in the warning's cool branch is a *second* factor of 1000.  Both clamp a
    /// negative result to zero and log.
    fn power_threshold(&self, critical: bool) -> Option<f64> {
        if !self.power_good() {
            return None;
        }
        let capacity_path = self.path(&format!("config/psu{}_power_capacity", self.index));
        if !utils::exists(&capacity_path) {
            return None;
        }
        // Every read below is Python's `read_int_from_file` with its default of
        // 0, so a tree missing the ambient files yields amb = 0 and takes the
        // `else` branch rather than failing.
        let read = |p: String| utils::read_int(&p).unwrap_or(0);

        let capacity = read(capacity_path);
        let limit = read(self.path(if critical {
            "config/amb_tmp_crit_limit"
        } else {
            "config/amb_tmp_warn_limit"
        }));
        let fan_amb = read(self.path("thermal/fan_amb"));
        let port_amb = read(self.path("thermal/port_amb"));
        let ambient = fan_amb.min(port_amb);
        let slope = read(self.path(&format!("config/psu{}_power_slope", self.index))) * 1000;

        let mut threshold = if ambient < limit {
            if critical {
                capacity
            } else {
                capacity - slope * 1000
            }
        } else if critical {
            capacity - (ambient - limit) * slope
        } else {
            capacity - (1000 + ambient - limit) * slope
        };

        if threshold <= 0 {
            log::warn!("Got negative PSU power threshold {threshold} for PSU {}", self.index);
            threshold = 0;
        }
        Some(threshold as f64 / 1_000_000.0)
    }

    /// `get_model`, including the side effect Python attaches to it.
    ///
    /// When the VPD model changes — a PSU was swapped for a different part —
    /// Python regenerates `/etc/sensors.d/sensors.conf` from the platform's
    /// copy and restarts `sensord`, because the sensor layout is per-PSU-model.
    /// It is triggered by a *read*, which is why it lives here rather than in
    /// a lifecycle hook.
    fn model(&mut self) -> Option<String> {
        if !self.hotswappable {
            // `FixedPsu.get_model()` is the literal 'N/A'.
            return None;
        }
        let current = self.vpd.model();
        match sensors_conf_action(
            current != self.last_model,
            PSU_SENSORS_CONF_UPDATER,
            PLATFORM_SENSORS_CONF,
            ETC_SENSORS_CONF,
        ) {
            SensorsConf::Skip | SensorsConf::NoSource => {}
            SensorsConf::Rebuild(src) => {
                if let Err(e) = rebuild_sensors_conf(&src) {
                    // Python leaves self.model unchanged so the next cycle
                    // retries, and returns the new value anyway.
                    log::error!("Failed to update PSU sensors configuration - {e}");
                    return current;
                }
            }
        }
        self.last_model = current.clone();
        current
    }
}

/// What `get_model` should do about `/etc/sensors.d/sensors.conf`.
#[derive(Debug, PartialEq)]
enum SensorsConf {
    /// The model did not change, or the platform ships no updater.
    Skip,
    /// The updater exists but there is no source file to rebuild from, so
    /// Python accepts the new model and moves on.
    NoSource,
    /// Run the updater against this source.
    Rebuild(String),
}

/// The decision half of `psu.py:309-341`, separated from running anything so
/// it can be exercised against a temporary tree.
///
/// The source is the *platform's* `sensors.conf` where one exists, not the
/// generated `/etc/sensors.d` copy — Python takes the platform file as the
/// source of truth precisely because the generated one may already be missing
/// the entries a previous rebuild dropped.
fn sensors_conf_action(model_changed: bool, updater: &str, platform_conf: &str, etc_conf: &str) -> SensorsConf {
    if !model_changed || !utils::exists(updater) {
        return SensorsConf::Skip;
    }
    let src = if std::path::Path::new(platform_conf).is_file() {
        platform_conf
    } else {
        etc_conf
    };
    if std::path::Path::new(src).is_file() {
        SensorsConf::Rebuild(src.to_string())
    } else {
        SensorsConf::NoSource
    }
}

/// Run the updater and restart `sensord`, as `psu.py:322-341` does.
fn rebuild_sensors_conf(src: &str) -> std::io::Result<()> {
    rebuild_sensors_conf_with(src, &mut |argv| run(argv))
}

/// The same, with the command execution supplied.
///
/// What is worth pinning is the command *text*: the updater is a shell
/// function that has to be sourced before it can be called, the source file is
/// the platform's copy rather than the generated one, and both paths are
/// quoted. Getting any of that wrong silently leaves `sensord` reading a
/// configuration for the PSU that was swapped out.
fn rebuild_sensors_conf_with(src: &str, run: &mut dyn FnMut(&[&str]) -> std::io::Result<()>) -> std::io::Result<()> {
    let script = format!(
        "source \"{PSU_SENSORS_CONF_UPDATER}\" && \
         update_psu_sensors_configuration \"{src}\" \"{ETC_SENSORS_CONF}\""
    );
    run(&["bash", "-c", &script])?;
    run(&["service", "sensord", "restart"])
}

fn run(argv: &[&str]) -> std::io::Result<()> {
    let status = std::process::Command::new(argv[0]).args(&argv[1..]).status()?;
    if status.success() {
        Ok(())
    } else {
        Err(std::io::Error::other(format!("command {argv:?} exited with {status}")))
    }
}

/// `InvalidPsuVolWA` (`psu.py:624-689`).
///
/// A known hardware fault makes some Delta 1100 W supplies report `127998` as
/// a voltage threshold on four older platforms.  The recovery is to run
/// `sensors -s`, which pokes the hardware into producing a real value, then
/// re-read once.
///
/// The three guards all `return threshold_value` — the *invalid* one — rather
/// than giving up, so a platform outside the affected list publishes 127.998 V.
/// That is reproduced: narrowing it to `None` would report `N/A` where Python
/// reports a number, on exactly the platforms nobody has looked at.
fn invalid_voltage_wa(value: i64, threshold_path: &str, platform_name: &str, vpd: &mut VpdParser) -> Option<i64> {
    const INVALID_VOLTAGE_VALUE: i64 = 127_998;
    const EXPECT_VENDOR_NAME: &str = "DELTA";
    const EXPECT_CAPACITY: &str = "1100";
    const EXPECT_PLATFORMS: [&str; 4] = [
        "x86_64-mlnx_msn3700-r0",
        "x86_64-mlnx_msn3700c-r0",
        "x86_64-mlnx_msn3800-r0",
        "x86_64-mlnx_msn4600c-r0",
    ];

    if value != INVALID_VOLTAGE_VALUE {
        return Some(value);
    }
    if !EXPECT_PLATFORMS.contains(&platform_name) {
        log::warn!("PSU threshold file {threshold_path} value {value}, but platform is {platform_name}");
        return Some(value);
    }
    // Python compares against 'N/A' — its stand-in for a field the VPD does not
    // carry — and lets that case through, so a PSU with no MFR_NAME still gets
    // the workaround.
    if let Some(vendor) = vpd.get(vpd::MFR_FIELD) {
        if vendor != EXPECT_VENDOR_NAME {
            log::warn!("PSU threshold file {threshold_path} value {value}, but its vendor is {vendor}");
            return Some(value);
        }
    }
    if let Some(capacity) = vpd.get(vpd::CAPACITY_FIELD) {
        if capacity != EXPECT_CAPACITY {
            log::warn!("PSU threshold file {threshold_path} value {value}, but its capacity is {capacity}");
            return Some(value);
        }
    }

    let _ = run(&["sensors", "-s"]);
    wait_set_done(threshold_path, INVALID_VOLTAGE_VALUE)
}

/// `InvalidPsuVolWA.wait_set_done` with `WAIT_TIME = 1`.
///
/// The loop reads once, and on a still-invalid value decrements to zero, sleeps
/// a second and falls out — so the sleep buys nothing and the second read never
/// happens.  Kept as written: shortening it would change how long a polling
/// cycle stalls on an affected box, which is the only observable difference.
fn wait_set_done(path: &str, invalid: i64) -> Option<i64> {
    match utils::read_int(path) {
        Some(v) if v != invalid => return Some(v),
        // `read_int_from_file` defaults to 0, which is not the invalid value,
        // so an unreadable file ends the wait immediately with 0.
        None => return Some(0),
        _ => {}
    }
    std::thread::sleep(std::time::Duration::from_secs(1));
    log::warn!("sensors -s does not recover PSU threshold sensor after 1 seconds");
    None
}

/// Build the chassis's PSU list once, in `psud`'s order.
///
/// The objects are kept rather than rebuilt each cycle because each carries a
/// VPD cache and the last model it saw.
pub fn discover(base: &str, count: usize, hotswappable: bool, platform_name: &str) -> Vec<Psu> {
    (1..=count)
        .map(|i| Psu::new(base, i, hotswappable, platform_name))
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A hw-management tree with the directories Python expects, plus whatever
    /// files a case needs.
    fn tree(files: &[(&str, &str)]) -> tempfile::TempDir {
        let dir = tempfile::tempdir().unwrap();
        for sub in ["thermal", "power", "config", "eeprom", "led", "system", "environment"] {
            std::fs::create_dir_all(dir.path().join(sub)).unwrap();
        }
        for (name, body) in files {
            std::fs::write(dir.path().join(name), body).unwrap();
        }
        dir
    }

    fn base_of(dir: &tempfile::TempDir) -> String {
        dir.path().to_string_lossy().into_owned()
    }

    /// An LED tree with no capability files, so `status()` answers `off` and
    /// these cases stay independent of the host.
    fn no_leds() -> (tempfile::TempDir, FanLeds) {
        let d = tempfile::tempdir().unwrap();
        let leds = FanLeds::with_path(d.path());
        (d, leds)
    }

    fn read_one(dir: &tempfile::TempDir, hotswappable: bool) -> PsuInfo {
        let (_l, leds) = no_leds();
        let mut psu = Psu::new(&base_of(dir), 1, hotswappable, "x86_64-mlnx_msn4700-r0");
        psu.read(&leds)
    }

    // ── FixedPsu vs Psu ───────────────────────────────────────────────────

    /// A soldered-in PSU is always present and publishes no electrical values
    /// at all — every reader in `FixedPsu` returns `None` regardless of what
    /// the files say.
    #[test]
    fn a_fixed_psu_is_present_and_reports_nothing_electrical() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("power/psu1_volt", "12000\n"),
            ("power/psu1_curr", "15400\n"),
        ]);
        let info = read_one(&dir, false);

        assert!(info.presence);
        assert!(info.power_good);
        assert!(!info.is_replaceable);
        assert_eq!(info.voltage, None);
        assert_eq!(info.current, None);
        assert_eq!(info.power, None);
        assert_eq!(info.model, None);
    }

    #[test]
    fn a_hotswappable_psu_reads_presence_from_sysfs() {
        let dir = tree(&[("thermal/psu1_status", "0\n"), ("thermal/psu1_pwr_status", "1\n")]);
        assert!(!read_one(&dir, true).presence);

        let dir = tree(&[("thermal/psu1_status", "1\n"), ("thermal/psu1_pwr_status", "1\n")]);
        assert!(read_one(&dir, true).presence);
    }

    // ── Electrical readings ───────────────────────────────────────────────

    #[test]
    fn the_electrical_values_carry_pythons_divisors() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("power/psu1_volt", "12100\n"),
            ("power/psu1_curr", "15400\n"),
            ("power/psu1_power", "302600000\n"),
            ("power/psu1_volt_in", "230000\n"),
            ("power/psu1_curr_in", "2500\n"),
            ("power/psu1_power_max", "1100000000\n"),
            ("thermal/psu1_temp1", "30125\n"),
            ("thermal/psu1_temp1_max", "70000\n"),
        ]);
        let info = read_one(&dir, true);

        assert_eq!(info.voltage, Some(12.1));
        assert_eq!(info.current, Some(15.4));
        assert_eq!(info.power, Some(302.6));
        assert_eq!(info.input_voltage, Some(230.0));
        assert_eq!(info.input_current, Some(2.5));
        assert_eq!(info.maximum_supplied_power, Some(1100.0));
        assert_eq!(info.temperature, Some(30.125));
        assert_eq!(info.temperature_high_threshold, Some(70.0));
    }

    /// Every electrical reader is gated on powergood, so a present PSU that is
    /// not delivering reports `N/A` rather than a stale number.
    #[test]
    fn a_psu_that_is_not_powered_good_reports_nothing() {
        let dir = tree(&[
            ("thermal/psu1_status", "1\n"),
            ("thermal/psu1_pwr_status", "0\n"),
            ("power/psu1_volt", "12100\n"),
            ("power/psu1_curr", "15400\n"),
        ]);
        let info = read_one(&dir, true);

        assert!(info.presence);
        assert!(!info.power_good);
        assert_eq!(info.voltage, None);
        assert_eq!(info.current, None);
        assert_eq!(info.temperature, None);
    }

    /// The fidelity case that is easy to get wrong: Python's
    /// `read_int_from_file` defaults to **0**, so a powered-good PSU whose file
    /// is missing publishes `0.0`, not `N/A`.  Only the powergood check yields
    /// `None`.
    #[test]
    fn an_unreadable_file_under_powergood_is_zero_not_absent() {
        let dir = tree(&[("thermal/psu1_pwr_status", "1\n")]);
        let info = read_one(&dir, true);

        assert_eq!(info.current, Some(0.0));
        assert_eq!(info.power, Some(0.0));
        assert_eq!(info.temperature, Some(0.0));
        // Voltage is the exception: its *path* is resolved by existence, and
        // neither candidate file is there, so there is nothing to read.
        assert_eq!(info.voltage, None);
    }

    /// `volt_out2` wins where both exist; the plain `volt` is the fallback.
    #[test]
    fn the_newer_voltage_file_wins() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("power/psu1_volt", "11000\n"),
            ("power/psu1_volt_out2", "12100\n"),
        ]);
        assert_eq!(read_one(&dir, true).voltage, Some(12.1));

        let dir = tree(&[("thermal/psu1_pwr_status", "1\n"), ("power/psu1_volt", "11000\n")]);
        assert_eq!(read_one(&dir, true).voltage, Some(11.0));
    }

    // ── Voltage thresholds ────────────────────────────────────────────────

    /// The capability *text* decides, not the presence of the min/max files:
    /// a platform that ships them without advertising them publishes `N/A`.
    #[test]
    fn voltage_thresholds_need_the_capability_to_name_them() {
        let files: &[(&str, &str)] = &[
            ("thermal/psu1_pwr_status", "1\n"),
            ("power/psu1_volt", "12000\n"),
            ("power/psu1_volt_max", "13000\n"),
            ("power/psu1_volt_min", "11000\n"),
        ];

        let dir = tree(files);
        let info = read_one(&dir, true);
        assert_eq!(info.voltage_high_threshold, None, "no capability file at all");

        let mut with_cap = files.to_vec();
        with_cap.push(("power/psu1_volt_capability", "min\n"));
        let dir = tree(&with_cap);
        let info = read_one(&dir, true);
        assert_eq!(info.voltage_low_threshold, Some(11.0));
        assert_eq!(info.voltage_high_threshold, None, "capability names min only");

        let mut both = files.to_vec();
        both.push(("power/psu1_volt_capability", "min max\n"));
        let dir = tree(&both);
        let info = read_one(&dir, true);
        assert_eq!(info.voltage_high_threshold, Some(13.0));
        assert_eq!(info.voltage_low_threshold, Some(11.0));
    }

    /// Python's `if max_voltage:` discards a zero threshold rather than
    /// publishing `0.0`.
    #[test]
    fn a_zero_voltage_threshold_is_discarded() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("power/psu1_volt", "12000\n"),
            ("power/psu1_volt_max", "0\n"),
            ("power/psu1_volt_capability", "max\n"),
        ]);
        assert_eq!(read_one(&dir, true).voltage_high_threshold, None);
    }

    /// The workaround's guards all return the *invalid* value, so a platform
    /// outside the affected list publishes 127.998 V.  Narrowing that to `N/A`
    /// would be an improvement Python does not make.
    #[test]
    fn the_invalid_voltage_survives_on_an_unaffected_platform() {
        let mut vpd = VpdParser::new("/nonexistent");
        let got = invalid_voltage_wa(127_998, "/some/path", "x86_64-mlnx_msn4700-r0", &mut vpd);
        assert_eq!(got, Some(127_998));
    }

    #[test]
    fn a_valid_voltage_passes_the_workaround_untouched() {
        let mut vpd = VpdParser::new("/nonexistent");
        let got = invalid_voltage_wa(12_000, "/some/path", "x86_64-mlnx_msn3700-r0", &mut vpd);
        assert_eq!(got, Some(12_000));
    }

    /// An affected platform whose PSU is not the affected part also returns the
    /// invalid value, without running anything.
    #[test]
    fn a_non_delta_psu_on_an_affected_platform_is_left_alone() {
        let dir = tree(&[("eeprom/psu1_vpd", "MFR_NAME:MURATA\nCAPACITY:1100\n")]);
        let mut vpd = VpdParser::new(dir.path().join("eeprom/psu1_vpd"));
        let got = invalid_voltage_wa(127_998, "/p", "x86_64-mlnx_msn3700-r0", &mut vpd);
        assert_eq!(got, Some(127_998));
    }

    // ── Power thresholds ──────────────────────────────────────────────────

    /// Below the ambient limit the critical threshold is the raw capacity and
    /// the warning threshold is one slope below it — where `slope` has already
    /// been multiplied by 1000 on read, so `slope * 1000` is a *second* factor.
    #[test]
    fn a_cool_box_uses_the_capacity_directly() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("config/psu1_power_capacity", "1100000000\n"),
            ("config/psu1_power_slope", "10\n"),
            ("config/amb_tmp_warn_limit", "45000\n"),
            ("config/amb_tmp_crit_limit", "50000\n"),
            ("thermal/fan_amb", "30000\n"),
            ("thermal/port_amb", "35000\n"),
        ]);
        let info = read_one(&dir, true);

        assert_eq!(info.power_critical_threshold, Some(1100.0));
        // 1_100_000_000 - 10*1000*1000 = 1_090_000_000
        assert_eq!(info.power_warning_suppress_threshold, Some(1090.0));
    }

    /// Above the limit both thresholds fall as the box heats up, and the
    /// ambient used is the *lower* of the two sensors.
    #[test]
    fn a_hot_box_derates_from_the_lower_ambient_sensor() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("config/psu1_power_capacity", "1100000000\n"),
            ("config/psu1_power_slope", "10\n"),
            ("config/amb_tmp_crit_limit", "50000\n"),
            ("thermal/fan_amb", "55000\n"),
            ("thermal/port_amb", "60000\n"),
        ]);
        // ambient = min(55000, 60000) = 55000; slope = 10*1000 = 10000
        // 1_100_000_000 - (55000-50000)*10000 = 1_050_000_000
        assert_eq!(read_one(&dir, true).power_critical_threshold, Some(1050.0));
    }

    /// A derate steep enough to go negative is clamped to zero, not published
    /// as a negative wattage.
    #[test]
    fn a_negative_threshold_is_clamped_to_zero() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            ("config/psu1_power_capacity", "1000\n"),
            ("config/psu1_power_slope", "1000\n"),
            ("config/amb_tmp_crit_limit", "50000\n"),
            ("thermal/fan_amb", "90000\n"),
            ("thermal/port_amb", "90000\n"),
        ]);
        assert_eq!(read_one(&dir, true).power_critical_threshold, Some(0.0));
    }

    /// Without the capacity file there is no formula to run, so the thresholds
    /// are absent rather than zero.
    #[test]
    fn no_capacity_file_means_no_power_thresholds() {
        let dir = tree(&[("thermal/psu1_pwr_status", "1\n")]);
        let info = read_one(&dir, true);
        assert_eq!(info.power_warning_suppress_threshold, None);
        assert_eq!(info.power_critical_threshold, None);
    }

    // ── Identity and LED ──────────────────────────────────────────────────

    #[test]
    fn the_vpd_supplies_model_serial_and_revision() {
        let dir = tree(&[
            ("thermal/psu1_pwr_status", "1\n"),
            (
                "eeprom/psu1_vpd",
                "PN_VPD_FIELD:MTEF-PSF-AC-C\nSN_VPD_FIELD:MT1919X00042\nREV_VPD_FIELD:A3\n",
            ),
        ]);
        let info = read_one(&dir, true);
        assert_eq!(info.model.as_deref(), Some("MTEF-PSF-AC-C"));
        assert_eq!(info.serial.as_deref(), Some("MT1919X00042"));
        assert_eq!(info.revision.as_deref(), Some("A3"));
    }

    /// Hot-swappable PSUs share one LED, so they all report the same colour;
    /// a fixed PSU has its own.  Getting this backwards would make every PSU
    /// report `off` on a platform whose shared LED is green.
    #[test]
    fn the_led_is_shared_only_when_the_psu_is_hot_swappable() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("led_psu_capability"), "green red\n").unwrap();
        std::fs::write(dir.path().join("led_psu_green"), "255\n").unwrap();
        std::fs::write(dir.path().join("led_psu_red"), "0\n").unwrap();
        std::fs::write(dir.path().join("led_psu2_capability"), "green red\n").unwrap();
        std::fs::write(dir.path().join("led_psu2_green"), "0\n").unwrap();
        std::fs::write(dir.path().join("led_psu2_red"), "255\n").unwrap();
        let leds = FanLeds::with_path(dir.path());

        let hw = tree(&[("thermal/psu2_pwr_status", "1\n")]);
        let base = base_of(&hw);

        let mut shared = Psu::new(&base, 2, true, "p");
        assert_eq!(shared.read(&leds).status_led.as_deref(), Some("green"));

        let mut fixed = Psu::new(&base, 2, false, "p");
        assert_eq!(fixed.read(&leds).status_led.as_deref(), Some("red"));
    }

    #[test]
    fn psus_are_named_and_numbered_from_one() {
        let dir = tree(&[]);
        let psus = discover(&base_of(&dir), 2, true, "p");
        let (_l, leds) = no_leds();
        let names: Vec<String> = psus.into_iter().map(|mut p| p.read(&leds).name).collect();
        assert_eq!(names, vec!["PSU 1", "PSU 2"]);
    }

    // ── sensors.conf rebuild decision ─────────────────────────────────────

    fn touch(dir: &tempfile::TempDir, name: &str) -> String {
        let p = dir.path().join(name);
        std::fs::write(&p, "").unwrap();
        p.to_string_lossy().into_owned()
    }

    /// The rebuild is triggered by a *model change*, not by every read — a PSU
    /// that stays put must not restart `sensord` every polling cycle.
    #[test]
    fn an_unchanged_model_rebuilds_nothing() {
        let d = tempfile::tempdir().unwrap();
        let updater = touch(&d, "updater");
        let platform = touch(&d, "platform.conf");
        assert_eq!(
            sensors_conf_action(false, &updater, &platform, "/nonexistent"),
            SensorsConf::Skip
        );
    }

    /// A platform that ships no updater script opts out entirely.
    #[test]
    fn no_updater_script_means_no_rebuild() {
        let d = tempfile::tempdir().unwrap();
        let platform = touch(&d, "platform.conf");
        assert_eq!(
            sensors_conf_action(true, "/nonexistent/updater", &platform, "/nonexistent"),
            SensorsConf::Skip
        );
    }

    /// The platform's own sensors.conf is preferred over the generated copy,
    /// because the generated one may already have entries missing.
    #[test]
    fn the_platform_conf_wins_over_the_generated_one() {
        let d = tempfile::tempdir().unwrap();
        let updater = touch(&d, "updater");
        let platform = touch(&d, "platform.conf");
        let etc = touch(&d, "etc.conf");
        assert_eq!(
            sensors_conf_action(true, &updater, &platform, &etc),
            SensorsConf::Rebuild(platform)
        );
    }

    #[test]
    fn the_generated_conf_is_the_fallback() {
        let d = tempfile::tempdir().unwrap();
        let updater = touch(&d, "updater");
        let etc = touch(&d, "etc.conf");
        assert_eq!(
            sensors_conf_action(true, &updater, "/nonexistent/platform.conf", &etc),
            SensorsConf::Rebuild(etc)
        );
    }

    /// With neither source present Python accepts the new model rather than
    /// retrying forever.
    #[test]
    fn no_source_file_at_all_is_accepted_not_retried() {
        let d = tempfile::tempdir().unwrap();
        let updater = touch(&d, "updater");
        assert_eq!(
            sensors_conf_action(true, &updater, "/nonexistent/a", "/nonexistent/b"),
            SensorsConf::NoSource
        );
    }

    // ── wait_set_done ─────────────────────────────────────────────────────

    /// A threshold that has recovered by the first read returns straight away,
    /// without the one-second stall.
    #[test]
    fn a_recovered_threshold_ends_the_wait_at_once() {
        let d = tempfile::tempdir().unwrap();
        let p = d.path().join("psu1_volt_max");
        std::fs::write(&p, "13000\n").unwrap();
        assert_eq!(wait_set_done(&p.to_string_lossy(), 127_998), Some(13_000));
    }

    /// Python reads through `read_int_from_file`, whose default is 0 — not the
    /// invalid value — so an unreadable file ends the wait immediately.
    #[test]
    fn an_unreadable_threshold_ends_the_wait_with_zero() {
        assert_eq!(wait_set_done("/nonexistent/threshold", 127_998), Some(0));
    }

    // ── The sensors.conf rebuild command ──────────────────────────────────

    /// Every argv the runner was handed, in order.
    type Commands = Rc<RefCell<Vec<Vec<String>>>>;

    fn record() -> (impl FnMut(&[&str]) -> std::io::Result<()>, Commands) {
        let log = Rc::new(RefCell::new(Vec::new()));
        let l = log.clone();
        let f = move |argv: &[&str]| -> std::io::Result<()> {
            l.borrow_mut().push(argv.iter().map(|s| s.to_string()).collect());
            Ok(())
        };
        (f, log)
    }

    use std::cell::RefCell;
    use std::rc::Rc;

    /// Two commands, in order: source the updater and call the shell function
    /// it defines, then restart `sensord` so it picks the new file up.
    /// Restarting without regenerating, or regenerating without restarting,
    /// both leave the daemon reading the old PSU's sensor layout.
    #[test]
    fn the_rebuild_sources_the_updater_then_restarts_sensord() {
        let (mut run, log) = record();
        rebuild_sensors_conf_with("/usr/share/sonic/platform/sensors.conf", &mut run).unwrap();

        let cmds = log.borrow().clone();
        assert_eq!(cmds.len(), 2);

        assert_eq!(cmds[0][0], "bash");
        assert_eq!(cmds[0][1], "-c");
        let script = &cmds[0][2];
        assert!(
            script.starts_with("source \""),
            "the function must be sourced: {script}"
        );
        assert!(script.contains(PSU_SENSORS_CONF_UPDATER), "{script}");
        assert!(script.contains("update_psu_sensors_configuration"), "{script}");
        assert!(
            script.contains("\"/usr/share/sonic/platform/sensors.conf\""),
            "the source is quoted: {script}"
        );
        assert!(script.contains(&format!("\"{ETC_SENSORS_CONF}\"")), "{script}");

        assert_eq!(cmds[1], ["service", "sensord", "restart"]);
    }

    /// A failing updater stops before the restart: restarting `sensord` against
    /// a file that was not regenerated is worse than not restarting at all.
    #[test]
    fn a_failing_updater_does_not_restart_sensord() {
        let log = Rc::new(RefCell::new(Vec::new()));
        let l = log.clone();
        let mut run = move |argv: &[&str]| -> std::io::Result<()> {
            l.borrow_mut().push(argv[0].to_string());
            Err(std::io::Error::other("updater failed"))
        };
        assert!(rebuild_sensors_conf_with("/src.conf", &mut run).is_err());
        assert_eq!(*log.borrow(), ["bash"], "sensord was never touched");
    }

    // ── The rest of the invalid-voltage workaround ────────────────────────

    /// The capacity guard: an affected platform with a Delta PSU that is not
    /// the 1100 W part is left alone, invalid value and all.
    #[test]
    fn a_delta_psu_of_the_wrong_capacity_is_left_alone() {
        let dir = tree(&[("eeprom/psu1_vpd", "MFR_NAME:DELTA\nCAPACITY:550\n")]);
        let mut vpd = VpdParser::new(dir.path().join("eeprom/psu1_vpd"));
        let got = invalid_voltage_wa(127_998, "/p", "x86_64-mlnx_msn3700-r0", &mut vpd);
        assert_eq!(got, Some(127_998));
    }

    /// The full path: an affected platform, a Delta 1100 W PSU, an invalid
    /// reading.  `sensors -s` is run — it fails here, and Python ignores its
    /// result too — and the threshold is re-read once.  A value that has
    /// recovered by then is the answer.
    #[test]
    fn the_workaround_rereads_the_threshold_after_poking_the_hardware() {
        let dir = tree(&[("eeprom/psu1_vpd", "MFR_NAME:DELTA\nCAPACITY:1100\n")]);
        let threshold = dir.path().join("power/psu1_volt_max");
        std::fs::write(&threshold, "13000\n").unwrap();

        let mut vpd = VpdParser::new(dir.path().join("eeprom/psu1_vpd"));
        let got = invalid_voltage_wa(
            127_998,
            &threshold.to_string_lossy(),
            "x86_64-mlnx_msn3700-r0",
            &mut vpd,
        );
        assert_eq!(got, Some(13_000), "the recovered value, not the invalid one");
    }
}
