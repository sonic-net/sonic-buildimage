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

//! Mellanox hw-management independent-mode thermal updater.
//!
//! Ports `thermal_updater.py`.  The hw-management file layout it writes
//! through lives in the `hw-management-rs` crate.
//!
//! Background thread that:
//!  1. Resumes `hw-management-tc` (writes "0" to the `suspend` file).
//!  2. Periodically reads ASIC temperature from STATE_DB TEMPERATURE_INFO and
//!     writes it to hw-management sysfs so that `hw-management-tc` can still
//!     react in independent mode.
//!  3. Periodically reads transceiver temperatures from STATE_DB
//!     TRANSCEIVER_DOM_TEMPERATURE / TRANSCEIVER_DOM_THRESHOLD and writes them
//!     to hw-management sysfs module thermal files.
//!  4. On cancellation, writes "1" to `suspend`, which stops `hw-management-tc`
//!     and pins fans at maximum PWM.

use std::collections::HashMap;
use std::sync::mpsc;
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

use swss_common::{ConfigDBConnector, DbConnector, Table};

use hw_management_rs::HwMgmt;

use crate::utils;

// ── Constants matching Python ─────────────────────────────────────────────────

const TC_SUSPEND_FILE:  &str = "/run/hw-management/config/suspend";

/// Values for `TC_SUSPEND_FILE`.  hw-management-tc reads an absent or `0` file
/// as *not* suspended and keeps driving fans; `1` makes it stop and pin fans at
/// maximum PWM.
const TC_RESUME:  &str = "0";
const TC_SUSPEND: &str = "1";
const TC_CONFIG_FILE:   &str = "/run/hw-management/config/tc_config.json";

/// millidegrees — multiply °C by this before writing to sysfs.
const TEMP_SCALE: i64 = 1000;

/// Default ASIC *warning* threshold in millidegrees (= 105 °C).
///
/// It lands in `asic_temp_crit`, because this call site passes it as the
/// library's `critical_threshold` — the same swap the Python caller makes.
/// The library maps straight through; see `hw_management_rs`.
const ASIC_CRIT_FILE_VALUE: i64 = 105_000;

/// Default ASIC *critical* threshold in millidegrees (= 120 °C).
/// Lands in `asic_temp_emergency`, passed as the library's `warning_threshold`.
const ASIC_EMERGENCY_FILE_VALUE: i64 = 120_000;

/// Sentinel for "ASIC temperature not ready yet".
/// MUST be the empty string — hw-management-tc distinguishes "" from "0".
const ASIC_NOT_READY: &str = "";

/// Error-read sentinel: written when the DB read itself fails.
const ERR_THERMAL: i64 = 254_000;

const DEFAULT_ASIC_INTERVAL_MS:   u64 = 1_000;
const DEFAULT_MODULE_INTERVAL_MS: u64 = 10_000;

/// Floor for a configured interval, in milliseconds.
///
/// A sub-second `poll_time` truncates to zero, and a zero interval makes the
/// feed loop sleep for nothing: `recv_timeout` returns at once, every deadline
/// is always due, and the thread issues STATE_DB reads and sysfs writes without
/// pause.  Python spins on the same input (`utils.py:525-539` re-queues an
/// already-due event at the same timestamp), so this is a deliberate deviation
/// rather than identity — an unbounded write rate is not a behaviour worth
/// reproducing.
///
/// The floor has to be a cadence the feed can actually sustain, not merely
/// non-zero: at 1 ms it would still be a redis read and a set of sysfs writes
/// every millisecond, which is the thing being prevented.  The shipped
/// configurations ask for 3 s and 20 s, three orders of magnitude away, so no
/// real input meets the floor.
const MIN_INTERVAL_MS: u64 = 100;

// ── Public API ────────────────────────────────────────────────────────────────

/// Configuration for the ThermalUpdater background thread.
#[derive(Debug)]
pub struct ThermalUpdaterConfig {
    /// Number of ASICs on this platform.
    pub asic_count: usize,
    /// ASIC STATE_DB keys: `["ASIC"]` for single-ASIC, `["ASIC0", "ASIC1"]`
    /// for multi-ASIC.  Matches the names written by TemperatureUpdater.
    pub asic_names: Vec<String>,
    /// Number of transceiver module slots (from hw-management config).
    pub module_count: usize,
    /// How often to push ASIC temperatures to sysfs.
    pub asic_interval: Duration,
    /// How often to push module temperatures to sysfs.
    pub module_interval: Duration,
    /// How often to push DPU temperatures, on a SmartSwitch.
    pub dpu_interval: Duration,
}

impl ThermalUpdaterConfig {
    /// Build the configuration for this platform, reading hw-management
    /// counter files and `tc_config.json`.
    pub fn from_platform(asic_count: usize, asic_names: Vec<String>) -> Self {
        // The module count lives in hw-management's tree, so read it through
        // hw-management's own accessor rather than re-encoding the path here.
        let module_count = HwMgmt::new().get_module_count().unwrap_or(0) as usize;

        let tc = parse_tc_config();

        Self {
            asic_count,
            asic_names,
            module_count,
            asic_interval:   Duration::from_millis(tc.asic_ms),
            module_interval: Duration::from_millis(tc.module_ms),
            dpu_interval:    Duration::from_secs_f64(tc.dpu_secs),
        }
    }
}

/// Handle to the running ThermalUpdater background thread.
///
/// Drop the sender or call `cancel()` to stop the thread and restore
/// `hw-management-tc`.
pub struct ThermalUpdaterHandle {
    /// Drop this to signal the thread to stop.
    cancel_tx: Option<mpsc::Sender<()>>,
    thread:    Option<JoinHandle<()>>,
}

impl ThermalUpdaterHandle {
    /// Signal the background thread to stop and wait for it to exit.
    pub fn cancel(mut self) {
        drop(self.cancel_tx.take());
        if let Some(t) = self.thread.take() {
            let _ = t.join();
        }
    }
}

impl Drop for ThermalUpdaterHandle {
    fn drop(&mut self) {
        drop(self.cancel_tx.take());
        if let Some(t) = self.thread.take() {
            let _ = t.join();
        }
    }
}

/// Clear the thermal data this daemon feeds, on the way out.
///
/// Python registers this with `atexit` when the updater is constructed
/// (`thermal_updater.py:64-81`, `:92`), so it runs on every interpreter exit —
/// normal, signal-driven and unhandled exception alike.  Without it the tree
/// keeps the last values written, and hw-management-tc goes on acting on
/// temperatures nothing is refreshing.
///
/// Not registered on a liquid-cooled platform, where Python returns before
/// constructing the updater at all (`thermal_manager.py:43-46`).
fn clean_thermal_data(hw: &HwMgmt, asic_count: usize, module_count: usize) {
    for asic_index in 0..asic_count {
        hw.thermal_data_clean_asic(asic_index as i64);
    }
    // Python returns here when there are no modules, so the counter is left
    // alone rather than written as 0.
    if module_count == 0 {
        return;
    }
    hw.module_data_set_module_counter(module_count as i64);
    for module_index in 1..=module_count {
        hw.thermal_data_clean_module(0, module_index as i64);
    }
}

/// Put hw-management back from a panic hook, before the process dies.
///
/// The release profile sets `panic = "abort"`, so a panic unwinds nothing and
/// `ThermalUpdaterHandle`'s `Drop` never runs.  Without this the daemon would
/// die leaving the tree exactly as the last cycle wrote it, and tc would keep
/// driving fans from an ASIC temperature nothing is refreshing any more.
///
/// **This is a deliberate deviation, not identity.** Python's main loop is
/// `while thermal_control.run(): pass` followed by `deinit()` with no
/// `try`/`finally` (`thermalctld:1595-1601`), so an unhandled exception skips
/// the suspend write and leaves `suspend` at `0`; only the `atexit` cleanup
/// runs, which is why tc there sees cleared data rather than stale data.  This
/// hook does both — clear *and* suspend — which stops tc instead of handing it
/// the error sentinel.  Both are safe; they are not the same.
///
/// Install it only where the feed actually starts.  On a liquid-cooled platform
/// `suspend` is never written at all, and a panic there must not start writing
/// it.
pub fn install_panic_hook(asic_count: usize, module_count: usize) {
    install_panic_hook_with(TC_SUSPEND_FILE, HwMgmt::new(), asic_count, module_count);
}

fn install_panic_hook_with(
    path: &'static str,
    hw: HwMgmt,
    asic_count: usize,
    module_count: usize,
) {
    let previous = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| {
        utils::write_sysfs_log(path, TC_SUSPEND);
        clean_thermal_data(&hw, asic_count, module_count);
        // Chain rather than replace: the panic message is the only record of
        // why the daemon died, and swallowing it would trade one hazard for
        // another.
        previous(info);
    }));
}

/// Spawn the ThermalUpdater background thread.
///
/// The thread opens its own STATE_DB and CONFIG_DB connections so it has no
/// shared state with the main daemon thread.
pub fn start_thermal_updater(cfg: ThermalUpdaterConfig, dpu_ids: Vec<u32>) -> ThermalUpdaterHandle {
    let (tx, rx) = mpsc::channel::<()>();

    let thread = thread::Builder::new()
        .name("thermal-updater".to_string())
        .spawn(move || thermal_updater_main(cfg, dpu_ids, rx))
        .expect("failed to spawn thermal-updater thread");

    ThermalUpdaterHandle {
        cancel_tx: Some(tx),
        thread:    Some(thread),
    }
}

// ── Thread body ───────────────────────────────────────────────────────────────

fn thermal_updater_main(
    cfg: ThermalUpdaterConfig,
    dpu_ids: Vec<u32>,
    cancel_rx: mpsc::Receiver<()>,
) {
    // Resume hw-management-tc: from here on we feed it temperatures, and it is
    // supposed to drive fans from them.  Python does the same in start().
    // Getting this backwards suspends the fan loop for exactly the period it is
    // being fed, and hands it stale temperatures once it resumes.
    utils::write_sysfs_log(TC_SUSPEND_FILE, TC_RESUME);
    log::info!(
        "ThermalUpdater started: {} ASIC(s), {} modules, \
         asic_interval={}ms, module_interval={}ms",
        cfg.asic_count,
        cfg.module_count,
        cfg.asic_interval.as_millis(),
        cfg.module_interval.as_millis(),
    );

    // Open our own DB connections (not shared with the main thread).
    let state_db_conn = match DbConnector::new_named("STATE_DB", false, 0) {
        Ok(c) => c,
        Err(e) => {
            log::error!("ThermalUpdater: cannot open STATE_DB: {e:?}");
            // No feed will ever start, so leave tc suspended rather than
            // driving fans from temperatures nothing is refreshing.
            utils::write_sysfs_log(TC_SUSPEND_FILE, TC_SUSPEND);
            return;
        }
    };
    let temp_table = match Table::new(state_db_conn, "TEMPERATURE_INFO") {
        Ok(t) => t,
        Err(e) => {
            log::error!("ThermalUpdater: cannot open TEMPERATURE_INFO: {e:?}");
            utils::write_sysfs_log(TC_SUSPEND_FILE, TC_SUSPEND);
            return;
        }
    };

    // Module tables (optional — if module_count == 0 we skip these).
    let module_tables: Option<(Table, Table, Table)> = if cfg.module_count > 0 {
        let build = || -> Result<(Table, Table, Table), Box<dyn std::error::Error>> {
            let c1 = DbConnector::new_named("STATE_DB", false, 0)?;
            let c2 = DbConnector::new_named("STATE_DB", false, 0)?;
            let c3 = DbConnector::new_named("STATE_DB", false, 0)?;
            let dom_temp  = Table::new(c1, "TRANSCEIVER_DOM_TEMPERATURE")?;
            let dom_thr   = Table::new(c2, "TRANSCEIVER_DOM_THRESHOLD")?;
            // Manufacturer and model, published alongside module temperature.
            let xcvr_info = Table::new(c3, "TRANSCEIVER_INFO")?;
            Ok((dom_temp, dom_thr, xcvr_info))
        };
        match build() {
            Ok(t) => Some(t),
            Err(e) => {
                log::warn!("ThermalUpdater: cannot open transceiver tables: {e}; \
                            module updates disabled");
                None
            }
        }
    } else {
        None
    };

    // The hw-management tree these writes land in.
    let hw = HwMgmt::new();

    // On a SmartSwitch the DPU feed shares this thread at its own cadence,
    // which is what SmartswitchThermalUpdater does by scheduling a second
    // timer rather than a second thread.
    let dpus = (!dpu_ids.is_empty()).then(|| crate::dpu::DpuUpdater::new(dpu_ids));

    let modules = module_tables
        .as_ref()
        .map(|(a, b, c)| (a as &dyn HashReader, b as &dyn HashReader, c as &dyn HashReader));

    let mut feeds = Feeds {
        hw: &hw,
        cfg: &cfg,
        temp: &temp_table,
        modules,
        // PORT.index -> logical port name.  The index is 1-based, so it is
        // keyed by module_num rather than sdk_index, and it refills lazily.
        port_map: PortMap::new(),
        // Presence per module, so that a module going absent is cleared once
        // on the transition rather than rewritten every cycle, as Python does.
        module_present: HashMap::new(),
        dpus,
    };

    let mut deadlines = Deadlines::starting_at(Instant::now());

    loop {
        let wait = deadlines.wait(Instant::now(), feeds.dpus.is_some());

        // Block until cancellation or the next deadline.  Any other outcome is
        // a timeout or a spurious wakeup, which just means "go round again".
        if let Err(mpsc::RecvTimeoutError::Disconnected) = cancel_rx.recv_timeout(wait) {
            break;
        }

        let due = deadlines.take_due(Instant::now(), &cfg, feeds.dpus.is_some());
        feeds.run(&due);
    }

    suspend_and_clean(TC_SUSPEND_FILE, &hw, &cfg);
}

/// What the updater does on its way out, on every exit path.
///
/// Suspending is the load-bearing half: nothing refreshes the fed temperatures
/// once this thread stops, so `hw-management-tc` has to stop too — it pins fans
/// at maximum PWM rather than acting on readings that will never move again.
/// Requirement 1c names every exit path, and this is the one taken on a normal
/// stop; the panic hook covers the one that is not.
///
/// The order is Python's: `deinit()` writes suspend, and the `atexit` handler
/// clears the data as the interpreter shuts down.  Clearing first would leave
/// tc reading empty files for as long as the two steps take.
fn suspend_and_clean(suspend_path: &str, hw: &HwMgmt, cfg: &ThermalUpdaterConfig) {
    utils::write_sysfs_log(suspend_path, TC_SUSPEND);
    clean_thermal_data(hw, cfg.asic_count, cfg.module_count);
    log::info!("ThermalUpdater stopped; hw-management-tc suspended and thermal data cleared");
}

// ── ASIC update ───────────────────────────────────────────────────────────────

/// When each feed is next due.
///
/// Three cadences share one thread, which is what `SmartswitchThermalUpdater`
/// does by scheduling a second timer rather than a second thread.
#[derive(Debug)]
struct Deadlines {
    asic: Instant,
    module: Instant,
    dpu: Instant,
}

/// Which feeds one pass should run.
#[derive(Debug, Default, PartialEq, Eq)]
struct Due {
    asic: bool,
    module: bool,
    dpu: bool,
}

/// Longest this thread will block, however far off the next feed is.
///
/// The cap costs a wakeup five times a second and buys a shutdown that does not
/// have to wait out a module interval — on a platform whose modules poll every
/// ten seconds, stopping the daemon would otherwise leave hw-management-tc
/// running on stale temperatures for most of that.
const MAX_BLOCK: Duration = Duration::from_millis(200);

impl Deadlines {
    /// Every feed is due immediately, so the first pass fills hw-management in
    /// before tc has a chance to act on an empty tree.
    fn starting_at(now: Instant) -> Self {
        Self { asic: now, module: now, dpu: now }
    }

    /// How long to block before the next feed is due.
    ///
    /// A platform with no DPUs must not be woken by the DPU deadline at all,
    /// which is what `Duration::MAX` stands in for — its deadline is never
    /// rearmed, so consulting it would pin the wait at zero and spin.
    fn wait(&self, now: Instant, has_dpus: bool) -> Duration {
        self.asic
            .saturating_duration_since(now)
            .min(self.module.saturating_duration_since(now))
            .min(if has_dpus {
                self.dpu.saturating_duration_since(now)
            } else {
                Duration::MAX
            })
            .min(MAX_BLOCK)
    }

    /// Which feeds are due, rearming each one it reports.
    ///
    /// Rearming from `now` rather than from the old deadline means a feed that
    /// ran late does not then run twice in quick succession to catch up: these
    /// are sensor reads, and there is nothing to catch up on.
    fn take_due(&mut self, now: Instant, cfg: &ThermalUpdaterConfig, has_dpus: bool) -> Due {
        let mut due = Due::default();
        if now >= self.asic {
            due.asic = true;
            self.asic = now + cfg.asic_interval;
        }
        if now >= self.module {
            due.module = true;
            self.module = now + cfg.module_interval;
        }
        if has_dpus && now >= self.dpu {
            due.dpu = true;
            self.dpu = now + cfg.dpu_interval;
        }
        due
    }
}

/// Everything one pass writes through, and the state it carries between passes.
struct Feeds<'a> {
    hw: &'a HwMgmt,
    cfg: &'a ThermalUpdaterConfig,
    temp: &'a dyn HashReader,
    /// `None` where the platform has no modules, or where the transceiver
    /// tables could not be opened — in which case the module feed is off
    /// rather than writing zeros over real readings.
    modules: Option<(&'a dyn HashReader, &'a dyn HashReader, &'a dyn HashReader)>,
    port_map: PortMap,
    module_present: HashMap<usize, bool>,
    dpus: Option<crate::dpu::DpuUpdater>,
}

impl Feeds<'_> {
    fn run(&mut self, due: &Due) {
        if due.asic {
            update_asic(self.hw, self.cfg, self.temp);
        }
        if due.module {
            if let Some((dom_temp, dom_thr, xcvr_info)) = self.modules {
                update_modules(
                    self.hw,
                    self.cfg,
                    dom_temp,
                    dom_thr,
                    xcvr_info,
                    &mut self.port_map,
                    &mut self.module_present,
                );
            }
        }
        if due.dpu {
            if let Some(d) = self.dpus.as_mut() {
                d.update();
            }
        }
    }
}

/// The one operation this crate performs on a STATE_DB table: read one field
/// of one key.
///
/// All three outcomes are distinguished by the callers and none may be folded
/// into another.  `Ok(None)` is a module or ASIC that STATE_DB does not carry
/// yet, which writes hw-management's not-ready value; `Err` is a *failed read*,
/// which writes the warning threshold as the temperature together with a fault
/// code so tc drives fans up rather than trusting a plausible zero
/// ([`read_asic_temp`]).  A trait returning `Option` would lose that split.
///
/// The trait is declared here rather than taken from `platform-traits` so that
/// it can be implemented for `swss_common::Table` — a local trait may be
/// implemented for a foreign type, the reverse may not — and so the
/// vendor-facing contract stays free of a swss-common dependency.
pub trait HashReader {
    fn hget(&self, key: &str, field: &str) -> Result<Option<String>, String>;
}

impl HashReader for Table {
    fn hget(&self, key: &str, field: &str) -> Result<Option<String>, String> {
        match Table::hget(self, key, field) {
            // A value that is not UTF-8 reads as empty, which the callers
            // already treat as "not ready" — the same as Python's
            // `.get(field, '')`.
            Ok(Some(v)) => Ok(Some(v.to_str().unwrap_or("").to_string())),
            Ok(None) => Ok(None),
            Err(e) => Err(format!("{e:?}")),
        }
    }
}

fn update_asic(hw: &HwMgmt, cfg: &ThermalUpdaterConfig, temp_table: &dyn HashReader) {
    for (i, asic_name) in cfg.asic_names.iter().enumerate() {
        let (temp_str, fault) = read_asic_temp(temp_table, asic_name);

        // The un-indexed "asic*" alias for index 0 and the "asic{n}*" files are
        // both hw-management's business, so the library writes them.
        hw.thermal_data_set_asic(
            i as i64,
            &temp_str,
            // warning_threshold -> asic_temp_emergency
            &ASIC_EMERGENCY_FILE_VALUE.to_string(),
            // critical_threshold -> asic_temp_crit
            &ASIC_CRIT_FILE_VALUE.to_string(),
            &fault.to_string(),
        );
    }
}

/// Read ASIC temperature from STATE_DB and return (sysfs_value_str, fault).
///
/// hw-management naming convention (inverted vs. threshold semantics):
///   `asic_temp_crit`      = 105 000 (WARNING threshold, the LOWER value)
///   `asic_temp_emergency` = 120 000 (CRITICAL threshold, the HIGHER value)
fn read_asic_temp(temp_table: &dyn HashReader, asic_name: &str) -> (String, i64) {
    match temp_table.hget(asic_name, "temperature") {
        Ok(Some(raw)) => {
            let s = raw.trim().to_string();
            if s == "N/A" || s.is_empty() {
                (ASIC_NOT_READY.to_string(), 0)
            } else {
                match s.parse::<f64>() {
                    Ok(celsius) => {
                        let millis = (celsius * TEMP_SCALE as f64) as i64;
                        if millis == 0 {
                            (ASIC_NOT_READY.to_string(), 0)
                        } else {
                            (millis.to_string(), 0)
                        }
                    }
                    Err(_) => (ASIC_NOT_READY.to_string(), 0),
                }
            }
        }
        Ok(None) => (ASIC_NOT_READY.to_string(), 0),
        Err(_)   => (ERR_THERMAL.to_string(), ERR_THERMAL),
    }
}

// ── Module update ─────────────────────────────────────────────────────────────

#[allow(clippy::too_many_arguments)]
fn update_modules(
    hw: &HwMgmt,
    cfg: &ThermalUpdaterConfig,
    dom_temp: &dyn HashReader,
    dom_thr:  &dyn HashReader,
    xcvr_info: &dyn HashReader,
    port_map: &mut PortMap,
    present_state: &mut HashMap<usize, bool>,
) {
    for sdk_index in 0..cfg.module_count {
        let module_num = sdk_index + 1; // hw-management is 1-based

        // A module with no logical port has no reading: Python's
        // `_get_data_from_db` returns `(False, None)` and the temperature comes
        // out 0 (`sfp.py:1685-1700`, `:1738-1742`).  Guessing a name from the
        // index instead would publish some other port's temperature into this
        // module's files, which hw-management-tc then acts on.
        let port = match port_map.get(module_num) {
            Some(p) => p.to_string(),
            None => String::new(),
        };
        let readable = !port.is_empty();

        let temp_celsius = readable.then(|| read_optional_float(dom_temp, &port, "temperature")).flatten();
        let warn_celsius = readable.then(|| read_optional_float(dom_thr,  &port, "temphighwarning")).flatten();
        let crit_celsius = readable.then(|| read_optional_float(dom_thr,  &port, "temphighalarm")).flatten();

        let temp_val = temp_celsius
            .map(|v| (v * TEMP_SCALE as f64) as i64)
            .unwrap_or(0);
        // Same swap as the ASIC path: the DOM warning threshold is passed as
        // critical_threshold so that it lands in module{n}_temp_crit, and the
        // DOM alarm threshold as warning_threshold so it lands in _emergency.
        let crit_file_val = warn_celsius
            .map(|v| (v * TEMP_SCALE as f64) as i64)
            .unwrap_or(0);
        let emergency_file_val = crit_celsius
            .map(|v| (v * TEMP_SCALE as f64) as i64)
            .unwrap_or(0);
        let fault = if temp_celsius.is_none() { 1 } else { 0 };

        // A module with no DOM temperature row is absent.  Python drives this
        // from the SFP object; the DOM row is the DB-side equivalent.
        let present = temp_celsius.is_some();
        let was_present = present_state.insert(sdk_index, present);

        if present {
            hw.thermal_data_set_module(
                0,
                module_num as i64,
                &temp_val.to_string(),
                &emergency_file_val.to_string(),
                &crit_file_val.to_string(),
                &fault.to_string(),
            );
            hw.vendor_data_set_module(0, module_num as i64, &read_vendor_info(xcvr_info, &port));
        } else if was_present != Some(false) {
            // Only on the transition to absent: hw-management-tc wants zeros in
            // every temperature file, and the vendor data cleared.  Rewriting
            // this every cycle would differ from Python.
            hw.thermal_data_set_module(0, module_num as i64, "0", "0", "0", "0");
            hw.vendor_data_set_module(
                0,
                module_num as i64,
                &[
                    ("manufacturer".to_string(), String::new()),
                    ("part_number".to_string(), String::new()),
                ],
            );
        }
    }
}

/// Manufacturer and model for a port, from `TRANSCEIVER_INFO`.
///
/// Python reads the same two fields and passes them under the keys
/// `manufacturer` and `part_number`, which hw-management renames to
/// `Manufacturer` and `PN` in the eeprom file.
fn read_vendor_info(xcvr_info: &dyn HashReader, port: &str) -> Vec<(String, String)> {
    let get = |field: &str| -> String {
        xcvr_info
            .hget(port, field)
            .ok()
            .flatten()
            .map(|v| v.trim().to_string())
            .unwrap_or_default()
    };
    vec![
        ("manufacturer".to_string(), get("manufacturer")),
        ("part_number".to_string(), get("model")),
    ]
}

/// Read an optional float field from a DB table.
///
/// Returns `None` when the key/field is absent or the value is "N/A" / "None".
fn read_optional_float(table: &dyn HashReader, key: &str, field: &str) -> Option<f64> {
    match table.hget(key, field) {
        Ok(Some(v)) => {
            let s = v.trim().to_string();
            if s == "N/A" || s == "None" || s.is_empty() {
                None
            } else {
                s.parse::<f64>().ok()
            }
        }
        _ => None,
    }
}

// ── Port map ──────────────────────────────────────────────────────────────────

/// APPL_DB key that says port configuration has finished.
const PORT_CONFIG_DONE: &str = "PORT_TABLE:PortConfigDone";

/// The two databases behind the port map.
///
/// Behind a trait because what is worth driving is the *policy* — rebuild only
/// once port configuration has finished, add and never replace, and no more
/// often than the gap below — none of which is about redis.
pub trait PortSource {
    /// APPL_DB's `PortConfigDone`.  False where the database could not be
    /// opened, which keeps the map empty rather than rebuilding from a
    /// half-populated CONFIG_DB.
    fn config_done(&self) -> bool;

    /// `PORT` as (name, index) pairs.  An error drops whatever connection the
    /// implementation holds, so the next rebuild reconnects.
    fn ports(&mut self) -> Result<Vec<(String, String)>, String>;
}

/// The real source: APPL_DB for the ready signal, CONFIG_DB for the table.
struct RedisPorts {
    appl_db: Option<DbConnector>,
    config_db: Option<ConfigDBConnector>,
}

impl PortSource for RedisPorts {
    fn config_done(&self) -> bool {
        self.appl_db
            .as_ref()
            .and_then(|db| db.exists(PORT_CONFIG_DONE).ok())
            .unwrap_or(false)
    }

    fn ports(&mut self) -> Result<Vec<(String, String)>, String> {
        let result = (|| -> Result<Vec<(String, String)>, Box<dyn std::error::Error>> {
            // One connector for the life of the thread.  Python builds a fresh
            // one per rebuild; holding it is unobservable and saves a connect
            // on a path that can run every minute.
            if self.config_db.is_none() {
                let cfg = ConfigDBConnector::new(false, None)?;
                cfg.connect(true, false)?;
                self.config_db = Some(cfg);
            }
            let cfg = self.config_db.as_ref().expect("just set");
            Ok(cfg
                .get_table("PORT")?
                .into_iter()
                .map(|(name, fields)| {
                    let index = fields
                        .get("index")
                        .and_then(|v| v.to_str().ok())
                        .unwrap_or_default()
                        .to_string();
                    (name, index)
                })
                .collect())
        })();

        result.map_err(|e| {
            // Drop the connector so the next rebuild makes a new one.  Holding
            // it across a failure would freeze the port map for the life of the
            // thread once redis restarted; Python builds a fresh connector per
            // rebuild and recovers by construction.
            self.config_db = None;
            format!("{e}")
        })
    }
}

/// `PORT.index` → logical port name.
///
/// `PORT.index` is the 1-based physical port number, so a caller looks it up
/// with `module_num` (`sdk_index + 1`) and not with `sdk_index` — which is what
/// Python does through `self.index = sfp_index + 1` (`sfp.py:335`, `:1676`).
///
/// Python keeps the map on the SFP class and fills it in *lazily*: a lookup
/// that misses checks APPL_DB's `PortConfigDone` and rebuilds only once port
/// configuration has finished (`sfp.py:1675-1683`).  Both halves of that matter
/// and are reproduced here:
///
/// * **Rebuild on a miss.** thermalctld can start before CONFIG_DB carries any
///   port, and a map built once at start-up would then stay empty for the life
///   of the daemon.
/// * **Add, never replace** (`sfp.py:1672-1673`). A breakout that remaps an
///   index leaves Python on the name it already had, so replacing here would
///   move a module onto a port Python never moves it to.
struct PortMap {
    map: HashMap<usize, String>,
    source: Box<dyn PortSource>,
    /// When the last rebuild ran.  An index `PORT` never publishes is a
    /// permanent miss, so without this a platform with more module slots than
    /// configured ports rebuilds once per unmapped slot per cycle, forever.
    last_rebuild: Option<Instant>,
}

/// Shortest gap between two rebuilds.
///
/// A deviation: Python reaches for the map only after the SFP reports itself
/// present, so an empty cage never asks and its rebuild storm is bounded by the
/// modules actually fitted.  Presence here comes from the DOM row, which is
/// downstream of the very lookup being rate-limited, so the gap is what bounds
/// it instead.  It is six module cycles, far below any rate at which port
/// configuration changes.
const PORT_MAP_REBUILD_INTERVAL: Duration = Duration::from_secs(60);

impl PortMap {
    fn new() -> Self {
        let appl_db = match DbConnector::new_named("APPL_DB", false, 0) {
            Ok(c) => Some(c),
            Err(e) => {
                log::warn!("ThermalUpdater: cannot open APPL_DB: {e:?}; \
                            the port map will not be rebuilt");
                None
            }
        };
        Self::from_source(Box::new(RedisPorts { appl_db, config_db: None }))
    }

    /// A map over any source, with the start-up rebuild already run.
    fn from_source(source: Box<dyn PortSource>) -> Self {
        let mut this = Self { map: HashMap::new(), source, last_rebuild: None };
        this.rebuild();
        this
    }

    /// The logical port for a 1-based `PORT.index`, rebuilding if the index is
    /// not known yet, port configuration has finished, and the last rebuild is
    /// old enough.
    fn get(&mut self, index: usize) -> Option<&str> {
        let due = rebuild_due(self.last_rebuild, Instant::now());
        if !self.map.contains_key(&index) && due && self.port_config_done() {
            // Arm the gap here rather than inside `rebuild`, so only a rebuild
            // that already passed `port_config_done` counts.  The constructor's
            // rebuild runs before port configuration has necessarily finished
            // and would otherwise arm the gap against an empty map, leaving
            // every module without a port for a minute — the exact window the
            // lazy refill exists to close.
            self.last_rebuild = Some(Instant::now());
            self.rebuild();
        }
        self.map.get(&index).map(String::as_str)
    }

    /// A map with known contents and no database behind it.
    ///
    /// `appl_db` being `None` makes `port_config_done()` false, so a lookup
    /// that misses never tries to rebuild — the miss is the answer.
    #[cfg(test)]
    fn preloaded(pairs: &[(usize, &str)]) -> Self {
        struct Nothing;
        impl PortSource for Nothing {
            fn config_done(&self) -> bool {
                false
            }
            fn ports(&mut self) -> Result<Vec<(String, String)>, String> {
                Ok(Vec::new())
            }
        }
        Self {
            map: pairs.iter().map(|(i, p)| (*i, p.to_string())).collect(),
            source: Box::new(Nothing),
            last_rebuild: None,
        }
    }

    fn port_config_done(&self) -> bool {
        self.source.config_done()
    }

    fn rebuild(&mut self) {
        match self.source.ports() {
            Ok(rows) => {
                let mut map = std::mem::take(&mut self.map);
                merge_ports(&mut map, rows);
                self.map = map;
            }
            Err(e) => {
                log::warn!("ThermalUpdater: failed to build port map from CONFIG_DB: {e}");
            }
        }
    }
}

/// Whether a rebuild is allowed yet.  The first one always is.
fn rebuild_due(last: Option<Instant>, now: Instant) -> bool {
    last.is_none_or(|t| now.saturating_duration_since(t) >= PORT_MAP_REBUILD_INTERVAL)
}

/// Fold `(port name, index text)` rows into the map, adding indices it does not
/// already carry and leaving the ones it does — Python's
/// `if index not in cls.port_mapping` (`sfp.py:1672-1673`).
fn merge_ports<I: IntoIterator<Item = (String, String)>>(
    map: &mut HashMap<usize, String>,
    rows: I,
) {
    for (port_name, index) in rows {
        let Ok(idx) = index.trim().parse::<usize>() else { continue };
        map.entry(idx).or_insert(port_name);
    }
}

// ── tc_config.json parser ──────────────────────────────────────────────────────

/// The three poll intervals `tc_config.json` carries, in the units each
/// consumer wants them in.
#[derive(Debug, PartialEq)]
pub struct TcIntervals {
    pub asic_ms:   u64,
    pub module_ms: u64,
    pub dpu_secs:  f64,
}

impl Default for TcIntervals {
    fn default() -> Self {
        Self {
            asic_ms:   DEFAULT_ASIC_INTERVAL_MS,
            module_ms: DEFAULT_MODULE_INTERVAL_MS,
            dpu_secs:  crate::dpu::DEFAULT_DPU_POLL_SECS,
        }
    }
}

/// Does Python's `re.match(r'asic\d*', key)` match this key?
///
/// At most one key matches in practice: all 37 shipped `tc_config_*.json` carry
/// exactly one `asic`-prefixed key (`asic\d*`) and one `module`-prefixed key
/// (`module\d+`).  That matters because `serde_json::Map` is a `BTreeMap`
/// without the `preserve_order` feature, so iteration here is sorted where
/// Python's `dict` is in insertion order — if a file ever carried two matching
/// keys the two implementations could pick different ones.  The tests pin the
/// sorted-order behaviour so the divergence is at least defined.
///
/// The keys under `dev_parameters` are regex *sources* — `asic\d*`,
/// `module\d+`, `dpu\d+_module` — and Python matches a pattern against that
/// literal text rather than against an expanded device name
/// (`thermal_updater.py:94-102`).  `\d*` accepts zero characters, so the ASIC
/// pattern matches its own key.
fn key_matches_asic(key: &str) -> bool {
    key.starts_with("asic")
}

/// Does Python's `re.match(r'module\d+', key)` match this key?
///
/// Note the asymmetry with the ASIC rule: `\d+` needs a real digit, and the
/// character after `module` in the shipped key is a backslash, so this does
/// **not** match on any platform shipping today and the module interval falls
/// back to its default.  Reproducing that is deliberate — a Rust daemon that
/// "fixed" it would poll modules at a cadence the Python daemon never used.
fn key_matches_module(key: &str) -> bool {
    key.strip_prefix("module")
        .is_some_and(|rest| rest.starts_with(|c: char| c.is_ascii_digit()))
}

/// Python's literal `dev_parameters` key for the DPU module sensor — a plain
/// dictionary lookup, not a regex match (`smartswitch_thermal_updater.py:70`).
const DPU_MODULE_KEY: &str = r"dpu\d+_module";

fn parse_tc_config() -> TcIntervals {
    parse_tc_config_from(TC_CONFIG_FILE)
}

/// Read the poll intervals out of `tc_config.json`.
///
/// Python treats a missing file, missing `dev_parameters`, a missing key and a
/// falsy `poll_time` all the same way: keep the default.  Two guards here have
/// no Python counterpart, and both exist because Python degrades where Rust
/// would not survive: the finite/upper-bound test, because `Duration` panics on
/// a value Python would merely schedule badly, and [`MIN_INTERVAL_MS`].
fn parse_tc_config_from(path: &str) -> TcIntervals {
    let defaults = TcIntervals::default();

    let Ok(text) = std::fs::read_to_string(path) else {
        log::info!("{path} does not exist, use default polling interval");
        return defaults;
    };

    let v: serde_json::Value = match serde_json::from_str(&text) {
        Ok(v) => v,
        Err(e) => {
            log::warn!("ThermalUpdater: failed to parse {path}: {e}; using defaults");
            return defaults;
        }
    };

    let Some(dev_parameters) = v.get("dev_parameters").and_then(|d| d.as_object()) else {
        log::warn!("dev_parameters not configured or empty, using default intervals");
        return defaults;
    };

    // `poll_time` is seconds, and is a JSON number in every shipped file but a
    // string in some; Python's `int()` takes either.  Note the order: Python
    // tests the *raw* value for truthiness and only then casts, so 0.5 counts as
    // configured and becomes 0 — truncating first would call it unset instead.
    let poll_time = |value: &serde_json::Value| -> Option<f64> {
        let secs = match value.get("poll_time")? {
            serde_json::Value::Number(n) => n.as_f64()?,
            serde_json::Value::String(s) => s.trim().parse().ok()?,
            _ => return None,
        };
        // The upper bound is not cosmetic: asic_ms and module_ms saturate in the
        // float-to-u64 cast, but dpu_secs goes straight into
        // `Duration::from_secs_f64`, which panics outside its range.
        (secs.is_finite() && secs > 0.0 && secs <= u32::MAX as f64).then_some(secs)
    };
    // Python's `int()` truncates to whole seconds before the value is used, so
    // a poll_time of 3.7 schedules at 3 there and would schedule at 3.7 here.
    let seconds = |value: &serde_json::Value| poll_time(value).map(f64::trunc);
    let first_match = |pred: fn(&str) -> bool| {
        dev_parameters
            .iter()
            .find(|(k, _)| pred(k))
            .and_then(|(_, v)| seconds(v))
    };

    TcIntervals {
        asic_ms: first_match(key_matches_asic)
            .map(|s| ((s * 1000.0) as u64).max(MIN_INTERVAL_MS))
            .unwrap_or(defaults.asic_ms),
        module_ms: first_match(key_matches_module)
            .map(|s| ((s * 1000.0) as u64).max(MIN_INTERVAL_MS))
            .unwrap_or(defaults.module_ms),
        // Python halves this one: the updater runs at twice the sensor's own
        // rate so a reading is never missed.
        dpu_secs: dev_parameters
            .get(DPU_MODULE_KEY)
            .and_then(seconds)
            .map(|s| (s / 2.0).max(MIN_INTERVAL_MS as f64 / 1000.0))
            .unwrap_or(defaults.dpu_secs),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use std::sync::Arc;
    use std::sync::atomic::{AtomicBool, Ordering};

    fn json(body: &str) -> tempfile::NamedTempFile {
        let mut f = tempfile::NamedTempFile::new().unwrap();
        f.write_all(body.as_bytes()).unwrap();
        f
    }

    /// The shape hw-management actually ships (`tc_config_sn5640.json`): the
    /// keys are regex sources, not device names.
    const SHIPPED: &str = r#"{
        "dev_parameters": {
            "asic\\d*":      { "poll_time": 3 },
            "module\\d+":    { "poll_time": 20 },
            "sensor_amb":    { "poll_time": 30 }
        }
    }"#;

    #[test]
    fn the_asic_interval_comes_from_the_shipped_config() {
        let f = json(SHIPPED);
        let tc = parse_tc_config_from(f.path().to_str().unwrap());
        assert_eq!(tc.asic_ms, 3_000, "the file says 3s, not the 1s default");
    }

    /// Python's `module\d+` pattern does not match the key `module\d+`, so the
    /// module interval falls back to its default even though the file carries
    /// one.  Matching that is the point: polling modules every 20s here would
    /// be a cadence the Python daemon never ran at.
    #[test]
    fn the_module_interval_falls_back_as_pythons_regex_does() {
        let f = json(SHIPPED);
        let tc = parse_tc_config_from(f.path().to_str().unwrap());
        assert_eq!(tc.module_ms, DEFAULT_MODULE_INTERVAL_MS);
    }

    #[test]
    fn a_literal_module_key_would_match() {
        let f = json(r#"{"dev_parameters": {"module1": {"poll_time": 20}}}"#);
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()).module_ms, 20_000);
    }

    #[test]
    fn the_dpu_interval_is_half_its_poll_time() {
        let f = json(r#"{"dev_parameters": {"dpu\\d+_module": {"poll_time": 3}}}"#);
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()).dpu_secs, 1.5);
    }

    #[test]
    fn a_missing_file_or_key_leaves_the_defaults() {
        assert_eq!(parse_tc_config_from("/nonexistent/tc_config.json"), TcIntervals::default());
        let f = json(r#"{"platform": "sn5640"}"#);
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()), TcIntervals::default());
        let f = json("not json at all");
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()), TcIntervals::default());
    }

    /// Python's `if poll_time:` treats 0 as unset, which is also what keeps a
    /// zero interval from turning the feed loop into a spin.
    #[test]
    fn a_zero_or_negative_poll_time_is_unset() {
        for body in [
            r#"{"dev_parameters": {"asic\\d*": {"poll_time": 0}}}"#,
            r#"{"dev_parameters": {"asic\\d*": {"poll_time": -5}}}"#,
            r#"{"dev_parameters": {"asic\\d*": {}}}"#,
        ] {
            let f = json(body);
            assert_eq!(
                parse_tc_config_from(f.path().to_str().unwrap()).asic_ms,
                DEFAULT_ASIC_INTERVAL_MS,
                "{body}"
            );
        }
    }

    /// The DPU interval is the one that becomes a `Duration` directly, so an
    /// absurd poll_time would panic in `from_secs_f64` at startup where the
    /// ASIC and module paths merely saturate their integer cast.
    #[test]
    fn an_overflowing_poll_time_is_unset() {
        let f = json(r#"{"dev_parameters": {"dpu\\d+_module": {"poll_time": 1e300}}}"#);
        let tc = parse_tc_config_from(f.path().to_str().unwrap());
        assert_eq!(tc.dpu_secs, crate::dpu::DEFAULT_DPU_POLL_SECS);
        // And the value it would have produced must be constructible.
        let _ = Duration::from_secs_f64(tc.dpu_secs);
    }

    #[test]
    fn a_poll_time_string_is_accepted_like_pythons_int() {
        let f = json(r#"{"dev_parameters": {"asic\\d*": {"poll_time": "4"}}}"#);
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()).asic_ms, 4_000);
    }

    /// The panic hook is process-global and cargo runs a crate's tests in
    /// parallel threads, so any test that installs one must hold this and put
    /// back what it found.  Without it a panic anywhere else in the binary
    /// lands in this test's temporary directory.
    static HOOK_LOCK: std::sync::Mutex<()> = std::sync::Mutex::new(());

    /// `panic = "abort"` skips unwinding, so `ThermalUpdaterHandle::drop` never
    /// runs and this hook is the only thing that puts tc back.  Uses
    /// `catch_unwind` with the test profile, which does unwind — the hook runs
    /// either way; what differs is only whether the process survives it.
    #[test]
    fn the_panic_hook_suspends_tc_and_keeps_the_previous_hook() {
        // A poisoned lock only means an earlier hook test panicked; the state
        // it guards is reinstalled below either way.
        let _guard = HOOK_LOCK.lock().unwrap_or_else(|e| e.into_inner());
        let original = std::panic::take_hook();
        let dir = tempfile::tempdir().unwrap();
        let path: &'static str = Box::leak(
            dir.path().join("suspend").to_string_lossy().into_owned().into_boxed_str(),
        );

        let hw_dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(hw_dir.path().join("config")).unwrap();
        std::fs::write(hw_dir.path().join("config/module_counter"), "2").unwrap();
        let hw = HwMgmt::with_base(hw_dir.path());
        // Something for the cleanup to remove.
        hw.thermal_data_set_asic(0, "45000", "105000", "120000", "0");

        let previous_ran = Arc::new(AtomicBool::new(false));
        let flag = previous_ran.clone();
        std::panic::set_hook(Box::new(move |_| flag.store(true, Ordering::SeqCst)));

        install_panic_hook_with(path, hw, 1, 2);
        let _ = std::panic::catch_unwind(|| panic!("forced"));
        // Restore what was installed before this test, not the default.
        std::panic::set_hook(original);

        assert_eq!(
            std::fs::read_to_string(path).unwrap(),
            "1\n",
            "tc must be left suspended"
        );
        assert!(
            !hw_dir.path().join("thermal/asic").exists(),
            "the thermal data must be cleared, as Python's atexit handler does"
        );
        assert!(
            previous_ran.load(Ordering::SeqCst),
            "the previous hook must still run, or the panic message is lost"
        );
    }

    /// Python returns before touching the counter when there are no modules,
    /// so a module-less platform must not have it written as 0.
    #[test]
    fn the_module_counter_is_left_alone_when_there_are_no_modules() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("config")).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        clean_thermal_data(&hw, 1, 0);
        assert!(!dir.path().join("config/module_counter").exists());
    }

    /// Python casts with `int()` before scheduling, so a fractional poll_time
    /// schedules at the whole second below it.
    #[test]
    fn a_fractional_poll_time_truncates_as_pythons_int_does() {
        let f = json(r#"{"dev_parameters": {"asic\\d*": {"poll_time": 3.7}}}"#);
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()).asic_ms, 3_000);
    }

    /// A sub-second poll_time truncates to zero, and a zero interval would make
    /// the feed loop spin.  Python spins here; this does not.
    #[test]
    fn a_sub_second_poll_time_is_floored_not_zero() {
        let f = json(r#"{"dev_parameters": {"asic\\d*": {"poll_time": 0.0005}}}"#);
        let tc = parse_tc_config_from(f.path().to_str().unwrap());
        assert_eq!(tc.asic_ms, MIN_INTERVAL_MS, "0 ms would be a spin");
        assert!(!Duration::from_millis(tc.asic_ms).is_zero());

        let f = json(r#"{"dev_parameters": {"dpu\\d+_module": {"poll_time": 0.5}}}"#);
        let tc = parse_tc_config_from(f.path().to_str().unwrap());
        assert!(Duration::from_secs_f64(tc.dpu_secs) >= Duration::from_millis(MIN_INTERVAL_MS));
    }

    fn rows(pairs: &[(&str, &str)]) -> Vec<(String, String)> {
        pairs.iter().map(|(a, b)| (a.to_string(), b.to_string())).collect()
    }

    /// A permanently unmapped index — a module slot `PORT` never publishes —
    /// misses on every cycle, so without the gap it would rebuild the whole
    /// map, once per such slot, for the life of the daemon.
    #[test]
    fn a_rebuild_is_rate_limited_after_the_first() {
        let t0 = Instant::now();
        assert!(rebuild_due(None, t0), "the first rebuild is always due");
        assert!(!rebuild_due(Some(t0), t0), "a second one immediately after is not");
        assert!(!rebuild_due(Some(t0), t0 + PORT_MAP_REBUILD_INTERVAL / 2));
        assert!(rebuild_due(Some(t0), t0 + PORT_MAP_REBUILD_INTERVAL));
    }

    /// The constructor rebuilds before port configuration has necessarily
    /// finished.  Arming the gap on that rebuild would leave every module
    /// without a port for a minute, which is the window the lazy refill exists
    /// to close.
    #[test]
    fn the_startup_rebuild_does_not_arm_the_rate_limit() {
        let t0 = Instant::now();
        // `last_rebuild` stays None until a gated rebuild runs, so the first
        // refill after PortConfigDone appears is due immediately.
        assert!(rebuild_due(None, t0));
        assert!(rebuild_due(None, t0 + Duration::from_millis(1)));
    }

    /// Two matching keys cannot occur in any shipped file, but if one ever
    /// carried them the choice must be defined rather than incidental:
    /// `serde_json::Map` is a `BTreeMap` here, so the sorted-first key wins.
    #[test]
    fn overlapping_keys_resolve_in_sorted_order() {
        let f = json(
            r#"{"dev_parameters": {"asic1": {"poll_time": 7}, "asic0": {"poll_time": 5}}}"#,
        );
        assert_eq!(parse_tc_config_from(f.path().to_str().unwrap()).asic_ms, 5_000);
    }

    #[test]
    fn the_port_map_is_keyed_by_the_one_based_index() {
        let mut m = HashMap::new();
        merge_ports(&mut m, rows(&[("Ethernet0", "1"), ("Ethernet4", "2")]));
        assert_eq!(m.get(&1).map(String::as_str), Some("Ethernet0"));
        assert_eq!(m.get(&2).map(String::as_str), Some("Ethernet4"));
        assert_eq!(m.get(&0), None, "index 0 does not exist in PORT");
    }

    /// Python adds indices it does not have and leaves the ones it does, so a
    /// breakout that remaps an index keeps the name already in the map.
    /// Replacing here would move a module onto a port Python never moves it to.
    #[test]
    fn a_rebuild_adds_but_never_replaces() {
        let mut m = HashMap::new();
        merge_ports(&mut m, rows(&[("Ethernet0", "1")]));
        merge_ports(&mut m, rows(&[("Ethernet0/1", "1"), ("Ethernet8", "3")]));
        assert_eq!(m.get(&1).map(String::as_str), Some("Ethernet0"), "not replaced");
        assert_eq!(m.get(&3).map(String::as_str), Some("Ethernet8"), "added");
    }

    #[test]
    fn a_row_without_a_usable_index_is_skipped() {
        let mut m = HashMap::new();
        merge_ports(&mut m, rows(&[("Ethernet0", ""), ("Ethernet4", "n/a"), ("Ethernet8", " 3 ")]));
        assert_eq!(m.len(), 1);
        assert_eq!(m.get(&3).map(String::as_str), Some("Ethernet8"));
    }

    #[test]
    fn the_key_matchers_follow_pythons_re_match() {
        assert!(key_matches_asic(r"asic\d*"));
        assert!(key_matches_asic("asic0"));
        assert!(!key_matches_asic("sensor_amb"));
        // `\d+` needs a real digit; the shipped key has a backslash there.
        assert!(!key_matches_module(r"module\d+"));
        assert!(key_matches_module("module1"));
        assert!(!key_matches_module("module"));
    }

    // ── A STATE_DB table that is a HashMap ────────────────────────────────

    /// The equivalent of Python's `tests/mock_swsscommon.Table`: a dictionary
    /// standing in for redis, so the read paths can be driven without one.
    #[derive(Default)]
    struct FakeTable {
        rows: std::collections::HashMap<(String, String), String>,
        /// When set, every read fails — the case that must not be confused
        /// with an absent key.
        fail: bool,
    }

    impl FakeTable {
        fn with(pairs: &[(&str, &str, &str)]) -> Self {
            let mut t = Self::default();
            for (k, f, v) in pairs {
                t.rows.insert((k.to_string(), f.to_string()), v.to_string());
            }
            t
        }
        fn failing() -> Self {
            Self { fail: true, ..Default::default() }
        }
    }

    impl HashReader for FakeTable {
        fn hget(&self, key: &str, field: &str) -> Result<Option<String>, String> {
            if self.fail {
                return Err("redis is down".into());
            }
            Ok(self.rows.get(&(key.to_string(), field.to_string())).cloned())
        }
    }

    // ── read_asic_temp ────────────────────────────────────────────────────

    /// A live reading is scaled to hw-management's millidegrees and carries no
    /// fault.
    #[test]
    fn a_live_asic_reading_is_scaled_to_millidegrees() {
        let t = FakeTable::with(&[("ASIC", "temperature", "45.5")]);
        assert_eq!(read_asic_temp(&t, "ASIC"), ("45500".to_string(), 0));
    }

    /// Not-ready writes an empty string, never `0`: tc takes its recoverable
    /// sensor-error path on empty, and would latch a false emergency on a
    /// plausible-looking zero.
    #[test]
    fn an_absent_or_unusable_asic_reading_writes_the_not_ready_value() {
        for row in [
            FakeTable::default(),                                         // key absent
            FakeTable::with(&[("ASIC", "temperature", "N/A")]),
            FakeTable::with(&[("ASIC", "temperature", "  ")]),
            FakeTable::with(&[("ASIC", "temperature", "not a number")]),
            FakeTable::with(&[("ASIC", "temperature", "0")]),            // scales to 0
        ] {
            assert_eq!(read_asic_temp(&row, "ASIC"), (ASIC_NOT_READY.to_string(), 0));
        }
    }

    /// A *failed read* is a different case from a missing one, and the only one
    /// that writes a fault: assume hot and let tc drive the fans up.  Folding
    /// this into not-ready is the mistake the three-valued trait exists to
    /// prevent.
    #[test]
    fn a_failed_asic_read_reports_a_fault_and_not_the_not_ready_value() {
        let t = FakeTable::failing();
        assert_eq!(
            read_asic_temp(&t, "ASIC"),
            (ERR_THERMAL.to_string(), ERR_THERMAL)
        );
    }

    /// Whitespace around the value is Python's `.strip()`.
    #[test]
    fn an_asic_reading_is_trimmed_before_parsing() {
        let t = FakeTable::with(&[("ASIC", "temperature", " 45.5\n")]);
        assert_eq!(read_asic_temp(&t, "ASIC"), ("45500".to_string(), 0));
    }

    /// Each ASIC is read under its own key, so a multi-ASIC box does not feed
    /// ASIC0's temperature to ASIC1.
    #[test]
    fn each_asic_is_read_under_its_own_key() {
        let t = FakeTable::with(&[
            ("ASIC0", "temperature", "40.0"),
            ("ASIC1", "temperature", "50.0"),
        ]);
        assert_eq!(read_asic_temp(&t, "ASIC0").0, "40000");
        assert_eq!(read_asic_temp(&t, "ASIC1").0, "50000");
    }

    // ── read_optional_float ───────────────────────────────────────────────

    #[test]
    fn an_optional_float_parses_and_trims() {
        let t = FakeTable::with(&[("Ethernet0", "temperature", " 36.5 ")]);
        assert_eq!(read_optional_float(&t, "Ethernet0", "temperature"), Some(36.5));
    }

    /// `N/A` and `None` are the two strings the transceiver tables use for a
    /// value they do not have; both mean absent, not zero.
    #[test]
    fn the_placeholder_strings_read_as_absent() {
        for v in ["N/A", "None", ""] {
            let t = FakeTable::with(&[("Ethernet0", "temperature", v)]);
            assert_eq!(read_optional_float(&t, "Ethernet0", "temperature"), None, "{v:?}");
        }
    }

    /// Unlike the ASIC path, a failed read here is simply absent: the module
    /// feed has no fault code to write.
    #[test]
    fn a_failed_or_missing_optional_float_is_absent() {
        assert_eq!(read_optional_float(&FakeTable::failing(), "Ethernet0", "temperature"), None);
        assert_eq!(read_optional_float(&FakeTable::default(), "Ethernet0", "temperature"), None);
    }

    // ── read_vendor_info ──────────────────────────────────────────────────

    /// `part_number` comes from the `model` field, not from a field of that
    /// name — the rename is Python's and a straight-through mapping would
    /// publish an empty part number.
    #[test]
    fn the_part_number_is_read_from_the_model_field() {
        let t = FakeTable::with(&[
            ("Ethernet0", "manufacturer", "NVIDIA"),
            ("Ethernet0", "model", "MMA1T00-VS"),
        ]);
        assert_eq!(
            read_vendor_info(&t, "Ethernet0"),
            vec![
                ("manufacturer".to_string(), "NVIDIA".to_string()),
                ("part_number".to_string(), "MMA1T00-VS".to_string()),
            ]
        );
    }

    /// Both fields are always published, empty where the table has nothing, so
    /// the file hw-management writes keeps its shape.
    #[test]
    fn vendor_info_publishes_both_fields_even_when_empty() {
        let got = read_vendor_info(&FakeTable::default(), "Ethernet0");
        assert_eq!(got.len(), 2);
        assert!(got.iter().all(|(_, v)| v.is_empty()));
    }

    // ── update_asic / update_modules against a hw-management tree ─────────

    /// A tree with the counters `HwMgmt`'s index checks require.
    fn hw_tree(asics: usize, modules: usize) -> (tempfile::TempDir, HwMgmt) {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("config")).unwrap();
        std::fs::create_dir_all(dir.path().join("thermal")).unwrap();
        std::fs::create_dir_all(dir.path().join("eeprom")).unwrap();
        std::fs::write(dir.path().join("config/asic_num"), asics.to_string()).unwrap();
        std::fs::write(dir.path().join("config/module_counter"), modules.to_string()).unwrap();
        let hw = HwMgmt::with_base(dir.path());
        (dir, hw)
    }

    fn thermal_file(dir: &tempfile::TempDir, name: &str) -> Option<String> {
        std::fs::read_to_string(dir.path().join("thermal").join(name))
            .ok()
            .map(|s| s.trim().to_string())
    }

    /// The intervals play no part in these cases — the feed functions are
    /// called directly rather than through the loop that schedules them.
    fn cfg(asic_names: &[&str], module_count: usize) -> ThermalUpdaterConfig {
        ThermalUpdaterConfig {
            asic_count: asic_names.len().max(1),
            asic_names: asic_names.iter().map(|s| s.to_string()).collect(),
            module_count,
            asic_interval: Duration::from_secs(1),
            module_interval: Duration::from_secs(1),
            dpu_interval: Duration::from_secs(1),
        }
    }

    /// The threshold files are written with the names swapped: the *warning*
    /// value lands in `_temp_crit` and the *critical* one in `_temp_emergency`.
    /// Neither name can be trusted on its own; the pair is the mapping.
    #[test]
    fn the_asic_feed_writes_the_thresholds_into_the_inverted_file_names() {
        let (dir, hw) = hw_tree(1, 0);
        let t = FakeTable::with(&[("ASIC", "temperature", "45.5")]);
        update_asic(&hw, &cfg(&["ASIC"], 0), &t);

        assert_eq!(thermal_file(&dir, "asic1").as_deref(), Some("45500"));
        assert_eq!(
            thermal_file(&dir, "asic1_temp_crit").as_deref(),
            Some(ASIC_CRIT_FILE_VALUE.to_string().as_str()),
            "the warning value lands in _temp_crit"
        );
        assert_eq!(
            thermal_file(&dir, "asic1_temp_emergency").as_deref(),
            Some(ASIC_EMERGENCY_FILE_VALUE.to_string().as_str()),
            "the critical value lands in _temp_emergency"
        );
        assert_eq!(thermal_file(&dir, "asic1_temp_fault").as_deref(), Some("0"));
    }

    /// Index 0 also writes the un-indexed aliases, which is what a
    /// single-ASIC platform's tc reads.
    #[test]
    fn the_first_asic_also_writes_the_unindexed_alias() {
        let (dir, hw) = hw_tree(1, 0);
        let t = FakeTable::with(&[("ASIC", "temperature", "45.5")]);
        update_asic(&hw, &cfg(&["ASIC"], 0), &t);
        assert_eq!(thermal_file(&dir, "asic").as_deref(), Some("45500"));
    }

    /// A failed read writes the fault code alongside the temperature, so tc
    /// takes its error path instead of trusting the value.
    #[test]
    fn a_failed_asic_read_reaches_the_fault_file() {
        let (dir, hw) = hw_tree(1, 0);
        update_asic(&hw, &cfg(&["ASIC"], 0), &FakeTable::failing());
        assert_eq!(thermal_file(&dir, "asic1").as_deref(), Some(ERR_THERMAL.to_string().as_str()));
        assert_eq!(
            thermal_file(&dir, "asic1_temp_fault").as_deref(),
            Some(ERR_THERMAL.to_string().as_str())
        );
    }

    /// Each ASIC lands in its own files.
    #[test]
    fn a_multi_asic_platform_feeds_each_asic_separately() {
        let (dir, hw) = hw_tree(2, 0);
        let t = FakeTable::with(&[
            ("ASIC0", "temperature", "40.0"),
            ("ASIC1", "temperature", "50.0"),
        ]);
        update_asic(&hw, &cfg(&["ASIC0", "ASIC1"], 0), &t);
        assert_eq!(thermal_file(&dir, "asic1").as_deref(), Some("40000"));
        assert_eq!(thermal_file(&dir, "asic2").as_deref(), Some("50000"));
    }

    // ── Modules ───────────────────────────────────────────────────────────

    /// A present module publishes its temperature and both thresholds, with the
    /// same file-name inversion as the ASIC path, plus its vendor data.
    #[test]
    fn a_present_module_is_fed_its_temperature_thresholds_and_vendor_data() {
        let (dir, hw) = hw_tree(1, 1);
        let dom_temp = FakeTable::with(&[("Ethernet0", "temperature", "36.5")]);
        let dom_thr = FakeTable::with(&[
            ("Ethernet0", "temphighwarning", "70.0"),
            ("Ethernet0", "temphighalarm", "80.0"),
        ]);
        let xcvr = FakeTable::with(&[
            ("Ethernet0", "manufacturer", "NVIDIA"),
            ("Ethernet0", "model", "MMA1T00-VS"),
        ]);
        let mut pm = PortMap::preloaded(&[(1, "Ethernet0")]);
        let mut seen = HashMap::new();
        update_modules(&hw, &cfg(&[], 1), &dom_temp, &dom_thr, &xcvr, &mut pm, &mut seen);

        assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("36500"));
        assert_eq!(thermal_file(&dir, "module1_temp_crit").as_deref(), Some("70000"));
        assert_eq!(thermal_file(&dir, "module1_temp_emergency").as_deref(), Some("80000"));
        assert_eq!(thermal_file(&dir, "module1_temp_fault").as_deref(), Some("0"));

        let vpd = std::fs::read_to_string(dir.path().join("eeprom/module1_data")).unwrap();
        assert!(vpd.contains("NVIDIA"), "{vpd:?}");
        assert!(vpd.contains("MMA1T00-VS"), "{vpd:?}");
    }

    /// A module with no logical port is not fed at all.  Guessing a name from
    /// the index would publish a neighbour's temperature into this module's
    /// files, which tc then acts on.
    #[test]
    fn a_module_with_no_port_is_not_fed_a_neighbours_reading() {
        let (dir, hw) = hw_tree(1, 1);
        let dom_temp = FakeTable::with(&[("Ethernet0", "temperature", "36.5")]);
        let mut pm = PortMap::preloaded(&[]);
        let mut seen = HashMap::new();
        update_modules(
            &hw, &cfg(&[], 1), &dom_temp, &FakeTable::default(),
            &FakeTable::default(), &mut pm, &mut seen,
        );
        // The absent-transition write happens, but with zeros — never 36500.
        assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("0"));
    }

    /// Going absent clears the files once, and only once: rewriting the zeros
    /// every cycle would differ from Python.
    #[test]
    fn the_absent_transition_is_written_once_and_not_repeated() {
        let (dir, hw) = hw_tree(1, 1);
        let empty = FakeTable::default();
        let mut pm = PortMap::preloaded(&[(1, "Ethernet0")]);
        let mut seen = HashMap::new();

        update_modules(&hw, &cfg(&[], 1), &empty, &empty, &empty, &mut pm, &mut seen);
        assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("0"));

        // Remove the file; a second pass must not put it back.
        std::fs::remove_file(dir.path().join("thermal/module1_temp_input")).unwrap();
        update_modules(&hw, &cfg(&[], 1), &empty, &empty, &empty, &mut pm, &mut seen);
        assert_eq!(thermal_file(&dir, "module1_temp_input"), None);
    }

    /// The module files are indexed 1-based while the loop counts from zero, so
    /// module 0's reading has to land in `module1_*`.
    #[test]
    fn the_module_files_are_one_based() {
        let (dir, hw) = hw_tree(1, 2);
        let dom_temp = FakeTable::with(&[("Ethernet4", "temperature", "36.5")]);
        let mut pm = PortMap::preloaded(&[(2, "Ethernet4")]);
        let mut seen = HashMap::new();
        update_modules(
            &hw, &cfg(&[], 2), &dom_temp, &FakeTable::default(),
            &FakeTable::default(), &mut pm, &mut seen,
        );
        assert_eq!(thermal_file(&dir, "module2_temp_input").as_deref(), Some("36500"));
    }

    // ── The schedule ──────────────────────────────────────────────────────

    fn intervals(asic_ms: u64, module_ms: u64, dpu_ms: u64) -> ThermalUpdaterConfig {
        ThermalUpdaterConfig {
            asic_count: 1,
            asic_names: vec!["ASIC".to_string()],
            module_count: 1,
            asic_interval: Duration::from_millis(asic_ms),
            module_interval: Duration::from_millis(module_ms),
            dpu_interval: Duration::from_millis(dpu_ms),
        }
    }

    /// Every feed is due on the first pass, so hw-management is filled in
    /// before tc has a chance to act on an empty tree.
    #[test]
    fn the_first_pass_runs_every_feed() {
        let t0 = Instant::now();
        let mut d = Deadlines::starting_at(t0);
        assert_eq!(
            d.take_due(t0, &intervals(1000, 10_000, 5_000), true),
            Due { asic: true, module: true, dpu: true }
        );
    }

    /// Each feed rearms to its own interval, so a fast ASIC feed does not drag
    /// the module feed along with it.
    #[test]
    fn each_feed_rearms_on_its_own_interval() {
        let t0 = Instant::now();
        let cfg = intervals(1000, 10_000, 5_000);
        let mut d = Deadlines::starting_at(t0);
        d.take_due(t0, &cfg, true);

        // One ASIC interval later, only the ASIC feed is due.
        let t1 = t0 + Duration::from_millis(1000);
        assert_eq!(d.take_due(t1, &cfg, true), Due { asic: true, ..Default::default() });

        // At five seconds the DPU joins it; the modules still have not.
        let t2 = t0 + Duration::from_millis(5000);
        assert_eq!(
            d.take_due(t2, &cfg, true),
            Due { asic: true, module: false, dpu: true }
        );

        // At ten, all three.
        let t3 = t0 + Duration::from_millis(10_000);
        assert_eq!(d.take_due(t3, &cfg, true), Due { asic: true, module: true, dpu: true });
    }

    /// A feed that ran late rearms from *now*, not from the deadline it missed,
    /// so it does not then fire repeatedly to catch up.  These are sensor
    /// reads: there is nothing to catch up on.
    #[test]
    fn a_late_feed_does_not_fire_twice_to_catch_up() {
        let t0 = Instant::now();
        let cfg = intervals(1000, 10_000, 5_000);
        let mut d = Deadlines::starting_at(t0);
        d.take_due(t0, &cfg, false);

        // Ten ASIC intervals pass in one go.
        let late = t0 + Duration::from_millis(10_000);
        assert!(d.take_due(late, &cfg, false).asic);
        // Half an interval later it is not due again.
        assert!(!d.take_due(late + Duration::from_millis(500), &cfg, false).asic);
        assert!(d.take_due(late + Duration::from_millis(1000), &cfg, false).asic);
    }

    /// A platform with no DPUs never reports the DPU feed due, however long the
    /// thread has been running — its deadline is never rearmed, so a schedule
    /// that consulted it would report it due on every pass.
    #[test]
    fn a_platform_without_dpus_never_runs_the_dpu_feed() {
        let t0 = Instant::now();
        let cfg = intervals(1000, 10_000, 5_000);
        let mut d = Deadlines::starting_at(t0);
        for n in [0, 1000, 60_000] {
            let due = d.take_due(t0 + Duration::from_millis(n), &cfg, false);
            assert!(!due.dpu, "at {n} ms");
        }
    }

    /// The wait is the nearest deadline, capped so that a cancellation is
    /// noticed promptly even when every interval is long.
    #[test]
    fn the_wait_is_the_nearest_deadline_under_the_cap() {
        let t0 = Instant::now();
        let cfg = intervals(50, 10_000, 5_000);
        let mut d = Deadlines::starting_at(t0);
        d.take_due(t0, &cfg, true);

        // The ASIC feed is nearest, at 50 ms — under the cap.
        assert_eq!(d.wait(t0, true), Duration::from_millis(50));

        // Once every interval is long, the cap takes over.
        let cfg = intervals(10_000, 10_000, 10_000);
        let mut d = Deadlines::starting_at(t0);
        d.take_due(t0, &cfg, true);
        assert_eq!(d.wait(t0, true), MAX_BLOCK);
    }

    /// A deadline already in the past is a zero wait, not a negative one that
    /// would panic or saturate to a very long sleep.
    #[test]
    fn a_missed_deadline_waits_no_time_at_all() {
        let t0 = Instant::now();
        let d = Deadlines::starting_at(t0);
        assert_eq!(d.wait(t0 + Duration::from_secs(60), true), Duration::ZERO);
    }

    /// Without DPUs the DPU deadline must not enter the wait: it is never
    /// rearmed, so it is permanently in the past and would pin the wait at zero
    /// and spin the thread.
    #[test]
    fn the_dpu_deadline_does_not_pin_the_wait_on_a_platform_without_dpus() {
        let t0 = Instant::now();
        let cfg = intervals(10_000, 10_000, 10_000);
        let mut d = Deadlines::starting_at(t0);
        d.take_due(t0, &cfg, false); // asic and module rearm; dpu does not

        let later = t0 + Duration::from_millis(100);
        assert_eq!(d.wait(later, false), MAX_BLOCK, "the stale DPU deadline is ignored");
        assert_eq!(d.wait(later, true), Duration::ZERO, "and would not be, with DPUs");
    }

    // ── The dispatch ──────────────────────────────────────────────────────

    /// Only the feeds a pass reports due are run.  Running one that is not due
    /// costs a sysfs write per sensor per pass on a thread that wakes five
    /// times a second.
    #[test]
    fn only_the_due_feeds_write() {
        let (dir, hw) = hw_tree(1, 1);
        let temp = FakeTable::with(&[("ASIC", "temperature", "45.0")]);
        let dom = FakeTable::with(&[("Ethernet0", "temperature", "36.5")]);
        let cfg = cfg(&["ASIC"], 1);

        let mut feeds = Feeds {
            hw: &hw,
            cfg: &cfg,
            temp: &temp,
            modules: Some((&dom, &dom, &dom)),
            port_map: PortMap::preloaded(&[(1, "Ethernet0")]),
            module_present: HashMap::new(),
            dpus: None,
        };

        feeds.run(&Due { asic: true, ..Default::default() });
        assert_eq!(thermal_file(&dir, "asic1").as_deref(), Some("45000"));
        assert!(thermal_file(&dir, "module1_temp_input").is_none(), "not due");

        feeds.run(&Due { module: true, ..Default::default() });
        assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("36500"));
    }

    /// A platform whose transceiver tables could not be opened runs no module
    /// feed at all, rather than writing zeros over readings hw-management-tc is
    /// acting on.
    #[test]
    fn no_transceiver_tables_means_no_module_feed() {
        let (dir, hw) = hw_tree(1, 1);
        let temp = FakeTable::default();
        let cfg = cfg(&["ASIC"], 1);

        let mut feeds = Feeds {
            hw: &hw,
            cfg: &cfg,
            temp: &temp,
            modules: None,
            port_map: PortMap::preloaded(&[(1, "Ethernet0")]),
            module_present: HashMap::new(),
            dpus: None,
        };
        feeds.run(&Due { asic: true, module: true, dpu: true });

        assert!(thermal_file(&dir, "module1_temp_input").is_none());
        assert!(thermal_file(&dir, "asic1").is_some(), "the ASIC feed still ran");
    }

    /// Module presence is carried between passes, which is what makes the
    /// absent transition fire once rather than every pass.
    #[test]
    fn presence_is_remembered_across_passes() {
        let (dir, hw) = hw_tree(1, 1);
        let temp = FakeTable::default();
        let present = FakeTable::with(&[("Ethernet0", "temperature", "36.5")]);
        let absent = FakeTable::default();
        let cfg = cfg(&["ASIC"], 1);

        {
            let mut feeds = Feeds {
                hw: &hw,
                cfg: &cfg,
                temp: &temp,
                modules: Some((&present, &present, &present)),
                port_map: PortMap::preloaded(&[(1, "Ethernet0")]),
                module_present: HashMap::new(),
                dpus: None,
            };
            feeds.run(&Due { module: true, ..Default::default() });
            assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("36500"));

            // The module goes away: the transition clears the files once.
            feeds.modules = Some((&absent, &absent, &absent));
            feeds.run(&Due { module: true, ..Default::default() });
            assert_eq!(thermal_file(&dir, "module1_temp_input").as_deref(), Some("0"));

            std::fs::remove_file(dir.path().join("thermal/module1_temp_input")).unwrap();
            feeds.run(&Due { module: true, ..Default::default() });
            assert!(
                thermal_file(&dir, "module1_temp_input").is_none(),
                "the second absent pass writes nothing"
            );
        }
    }

    // ── The port map's rebuild policy ─────────────────────────────────────

    use std::cell::{Cell, RefCell};
    use std::rc::Rc;

    /// A source a test can change *after* construction, which is the real
    /// scenario: thermalctld can start before CONFIG_DB carries any port.
    #[derive(Clone, Default)]
    struct FakePorts {
        done: Rc<Cell<bool>>,
        rows: Rc<RefCell<Vec<(String, String)>>>,
        fail: Rc<Cell<bool>>,
        calls: Rc<Cell<usize>>,
    }

    impl FakePorts {
        fn set_rows(&self, pairs: &[(&str, &str)]) {
            *self.rows.borrow_mut() =
                pairs.iter().map(|(a, b)| (a.to_string(), b.to_string())).collect();
        }
    }

    impl PortSource for FakePorts {
        fn config_done(&self) -> bool {
            self.done.get()
        }
        fn ports(&mut self) -> Result<Vec<(String, String)>, String> {
            self.calls.set(self.calls.get() + 1);
            if self.fail.get() {
                return Err("CONFIG_DB is down".to_string());
            }
            Ok(self.rows.borrow().clone())
        }
    }

    /// The map is built once at start-up, so a daemon that starts after
    /// CONFIG_DB is populated has its ports immediately.
    #[test]
    fn the_map_is_built_once_at_construction() {
        let f = FakePorts::default();
        f.set_rows(&[("Ethernet0", "1"), ("Ethernet4", "2")]);
        let mut pm = PortMap::from_source(Box::new(f.clone()));

        assert_eq!(f.calls.get(), 1);
        assert_eq!(pm.get(1), Some("Ethernet0"));
        assert_eq!(pm.get(2), Some("Ethernet4"));
    }

    /// A miss does not rebuild until port configuration has finished.  Reading
    /// a half-populated CONFIG_DB would fix a module onto whatever port existed
    /// at that moment, and the map never replaces what it already has.
    #[test]
    fn a_miss_does_not_rebuild_before_port_configuration_finishes() {
        let f = FakePorts::default(); // empty CONFIG_DB, PortConfigDone unset
        let mut pm = PortMap::from_source(Box::new(f.clone()));
        assert_eq!(f.calls.get(), 1, "the start-up rebuild, against nothing");

        f.set_rows(&[("Ethernet0", "1")]); // ports appear...
        assert_eq!(pm.get(1), None, "...but configuration has not finished");
        assert_eq!(f.calls.get(), 1, "so no rebuild");
    }

    /// Once it has, the next miss picks the ports up — this is the window the
    /// lazy refill exists to close, and the constructor's rebuild deliberately
    /// does not arm the rate limit against it.
    #[test]
    fn the_first_miss_after_configuration_finishes_picks_the_ports_up() {
        let f = FakePorts::default();
        let mut pm = PortMap::from_source(Box::new(f.clone()));

        f.set_rows(&[("Ethernet0", "1")]);
        f.done.set(true);
        assert_eq!(pm.get(1), Some("Ethernet0"), "the miss rebuilt and found it");
        assert_eq!(f.calls.get(), 2);
    }

    /// An index `PORT` never publishes is a permanent miss, and without the
    /// gap a platform with more module slots than ports would rebuild once per
    /// unmapped slot per cycle, forever.
    #[test]
    fn a_permanent_miss_is_rate_limited() {
        let f = FakePorts::default();
        f.done.set(true);
        f.set_rows(&[("Ethernet0", "1")]);
        let mut pm = PortMap::from_source(Box::new(f.clone()));

        assert_eq!(pm.get(9), None);
        let after_first = f.calls.get();
        assert_eq!(pm.get(9), None);
        assert_eq!(pm.get(9), None);
        assert_eq!(f.calls.get(), after_first, "the gap holds off the repeats");
    }

    /// A rebuild adds and never replaces, so a breakout that remaps an index
    /// leaves the daemon on the name it already had — moving a module onto a
    /// port Python never moves it to would publish a neighbour's temperature
    /// into files hw-management-tc acts on.
    #[test]
    fn a_rebuild_never_moves_a_module_that_already_has_a_port() {
        let f = FakePorts::default();
        f.done.set(true);
        f.set_rows(&[("Ethernet0", "1")]);
        let mut pm = PortMap::from_source(Box::new(f.clone()));
        assert_eq!(pm.get(1), Some("Ethernet0"));

        // CONFIG_DB remaps index 1, and a miss elsewhere forces a rebuild.
        f.set_rows(&[("Ethernet8", "1")]);
        assert_eq!(pm.get(9), None);
        assert_eq!(pm.get(1), Some("Ethernet0"), "still the name it had");
    }

    /// A failed rebuild is logged and leaves the map as it was: a redis blip
    /// must not empty the port map and take every module's temperature with it.
    #[test]
    fn a_failed_rebuild_keeps_the_map_it_had() {
        let f = FakePorts::default();
        f.done.set(true);
        f.set_rows(&[("Ethernet0", "1")]);
        let mut pm = PortMap::from_source(Box::new(f.clone()));
        assert_eq!(pm.get(1), Some("Ethernet0"));

        f.fail.set(true);
        assert_eq!(pm.get(9), None);
        assert_eq!(pm.get(1), Some("Ethernet0"), "the map survived the failure");
    }

    // ── The way out ───────────────────────────────────────────────────────

    /// A normal stop suspends hw-management-tc and clears what it was being
    /// fed.  This is requirement 1c's main case — the panic hook covers the
    /// other one — and leaving `suspend` at 0 hands tc an ASIC temperature
    /// nothing refreshes.
    #[test]
    fn stopping_suspends_tc_and_clears_the_data_it_was_fed() {
        let (dir, hw) = hw_tree(1, 2);
        // Something for it to clear.
        for f in ["asic1", "module1_temp_input", "module2_temp_input"] {
            std::fs::write(dir.path().join("thermal").join(f), "45000").unwrap();
        }
        let suspend = dir.path().join("config/suspend");

        suspend_and_clean(&suspend.to_string_lossy(), &hw, &cfg(&["ASIC"], 2));

        assert_eq!(
            std::fs::read_to_string(&suspend).unwrap().trim(),
            "1",
            "tc is suspended"
        );
        for f in ["asic1", "module1_temp_input", "module2_temp_input"] {
            assert!(thermal_file(&dir, f).is_none(), "{f} was left behind");
        }
    }

    /// A platform with no modules leaves the module counter alone rather than
    /// writing 0, which is where Python returns.
    #[test]
    fn a_platform_with_no_modules_does_not_write_a_zero_counter() {
        let (dir, hw) = hw_tree(1, 0);
        let before = std::fs::read_to_string(dir.path().join("config/module_counter")).unwrap();
        suspend_and_clean(
            &dir.path().join("config/suspend").to_string_lossy(),
            &hw,
            &cfg(&["ASIC"], 0),
        );
        assert_eq!(
            std::fs::read_to_string(dir.path().join("config/module_counter")).unwrap(),
            before
        );
    }

    /// Suspending must not depend on the clearing succeeding: an unwritable
    /// hw-management tree still has to stop tc.
    #[test]
    fn suspending_happens_even_when_there_is_nothing_to_clear() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(dir.path().join("config")).unwrap();
        let hw = HwMgmt::with_base(dir.path().join("nonexistent"));
        let suspend = dir.path().join("config/suspend");

        suspend_and_clean(&suspend.to_string_lossy(), &hw, &cfg(&["ASIC"], 2));
        assert_eq!(std::fs::read_to_string(&suspend).unwrap().trim(), "1");
    }
}
