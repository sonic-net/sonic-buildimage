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


//! `sonic_platform/dpuctlplat.py`: one DPU's power, reset and PCI control
//! through hw-management's sysfs, and the handshakes that go with them.
//!
//! `DpuModule` turns verbosity on, so every operation is timed and logged,
//! and its boot progress is watched for the duration.  Python watches from a
//! forked `multiprocessing.Process`; this watches from a thread, so those
//! lines carry the daemon's own pid where Python's carry the child's.

use std::io::Write;
use std::os::fd::AsRawFd;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread::sleep;
use std::time::{Duration, Instant};

use platform_base::io;
use platform_base::logger::SysLogger;
use platform_base::py::{ops, py_int, PyErr, PyResult, PyVal};
use platform_base::redfish_client::is_exception;

use crate::device_data;
use crate::inotify_helper::{elapsed_repr, InotifyHelper};
use crate::utils::{self, Log};

const HW_BASE: &str = "/var/run/hw-management/";
const EVENT_BASE: &str = "/var/run/hw-management/events/";
const SYSTEM_BASE: &str = "/var/run/hw-management/system/";
pub const PCI_DEV_BASE: &str = "/sys/bus/pci/devices/";
const MLX5_CORE_DRIVER_PATH: &str = "/sys/bus/pci/drivers/mlx5_core/";
const MLX5_CORE_UNBIND_PATH: &str = "/sys/bus/pci/drivers/mlx5_core/unbind";
const MLX5_CORE_BIND_PATH: &str = "/sys/bus/pci/drivers/mlx5_core/bind";
// How long each handshake may take, and the pause between retries.  A
// test's DPU has answered before it is asked or never answers, so a test
// waits a fraction of a second where a switch waits minutes.
#[cfg(not(test))]
const WAIT_FOR_SHTDN: f64 = 120.0;
#[cfg(not(test))]
const WAIT_FOR_DPU_READY: f64 = 180.0;
#[cfg(not(test))]
const WAIT_FOR_PCI_BINDABLE: f64 = 10.0;
#[cfg(not(test))]
const PAUSE: Duration = Duration::from_secs(1);
#[cfg(test)]
const WAIT_FOR_SHTDN: f64 = 0.05;
#[cfg(test)]
const WAIT_FOR_DPU_READY: f64 = 0.05;
#[cfg(test)]
const WAIT_FOR_PCI_BINDABLE: f64 = 0.3;
#[cfg(test)]
const PAUSE: Duration = Duration::from_millis(1);
const PCI_POLL_INTERVAL: f64 = 0.1;
const CLR: &str = "0";
const SET: &str = "1";
pub const BOOT_PROG_RST: i128 = 0;
/// How long Python's forked watcher takes to install its SIGTERM handler.
const WATCHER_STARTUP: Duration = Duration::from_millis(5);

/// `boot_prog_map`.
fn boot_prog_name(v: i128) -> &'static str {
    match v {
        0 => "Reset/Boot-ROM",
        1 => "BL2 (from ATF image on eMMC partition)",
        2 => "BL31 (from ATF image on eMMC partition)",
        3 => "UEFI (from UEFI image on eMMC partition)",
        4 => "OS Starting",
        5 => "OS is running",
        6 => "Low-Power Standby",
        7 => "FW Update in progress",
        8 => "OS Crash Dump in progress",
        9 => "OS Crash Dump is complete",
        10 => "FW Fault Crash Dump in progress",
        11 => "FW Fault Crash Dump is complete",
        15 => "Software is inactive",
        _ => "N/A",
    }
}

/// `BootProgEnum.OS_RUN`.
pub const BOOT_PROG_OS_RUN: i128 = 5;

/// The module's `logger = SysLogger("dpuctl_plat")`, made at import.
static LOGGER: OnceLock<SysLogger> = OnceLock::new();

/// `import dpuctlplat`.
pub fn import() {
    crate::imports::dpuctlplat();
    LOGGER.get_or_init(|| SysLogger::new(Some("dpuctl_plat")));
}

fn logger() -> &'static SysLogger {
    LOGGER.get_or_init(|| SysLogger::new(Some("dpuctl_plat")))
}

/// `time.strftime("%Y-%m-%d %H:%M:%S")`.
fn local_time() -> String {
    // SAFETY: time and localtime_r on values of ours.
    let mut tm: libc::tm = unsafe { std::mem::zeroed() };
    let t = unsafe { libc::time(std::ptr::null_mut()) };
    unsafe { libc::localtime_r(&t, &mut tm) };
    format!(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    )
}

/// Where a DPU's lines go: the two ways `setup_logger` sets them up.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LogSink {
    /// `setup_logger(use_notice_level=True)`, as `DpuModule` calls it: info
    /// and debug at notice level, errors and warnings at theirs.
    Syslog,
    /// `setup_logger(use_print=True)`, as the FPGA update calls it: every
    /// line printed with the time, then sent at notice level.
    Print,
}

/// A DPU's boot progress file and where its lines go: what
/// `read_boot_prog()` and the `log_*` methods need, shared by `DpuCtlPlat`,
/// its watcher and the FPGA update, so that they read and log alike.
#[derive(Clone)]
pub struct BootProg {
    dpu_name: String,
    path: String,
    sink: LogSink,
}

impl BootProg {
    fn new(dpu_name: &str, hw_name: &str, sink: LogSink) -> BootProg {
        BootProg {
            dpu_name: dpu_name.to_string(),
            path: format!("{HW_BASE}{hw_name}/system/boot_progress"),
            sink,
        }
    }

    /// `DpuCtlPlat(dpu_name)` after `setup_logger(use_print=True)`, of which
    /// the FPGA update reads only the boot progress.  `get_hwmgmt_name()`'s
    /// `int(dpu_name[3:])` raises as Python's does.
    pub fn printing(dpu_name: &str) -> PyResult<BootProg> {
        import();
        let n = py_int(dpu_name.get(3..).unwrap_or(""))?;
        Ok(BootProg::new(dpu_name, &format!("{}{}", dpu_name.get(..3).unwrap_or(""), n + 1), LogSink::Print))
    }

    fn emit(&self, level: Log, msg: &str) {
        let msg = format!("{}: {msg}", self.dpu_name);
        match self.sink {
            LogSink::Syslog => match level {
                Log::Error => logger().log_error(&msg),
                Log::Warning => logger().log_warning(&msg),
                _ => logger().log_notice(&msg),
            },
            LogSink::Print => {
                crate::component::print(&format!("[{}] {msg}", local_time()));
                logger().log_notice(&msg);
            }
        }
    }

    fn log_error(&self, msg: &str) {
        self.emit(Log::Error, msg);
    }

    fn log_warning(&self, msg: &str) {
        self.emit(Log::Warning, msg);
    }

    /// `log_info` and `log_debug`: both at notice level for either sink.
    fn log_info(&self, msg: &str) {
        self.emit(Log::Notice, msg);
    }

    /// `read_boot_prog()`: up to three attempts while the file answers ENXIO,
    /// a second apart; anything else raises at once.
    pub fn read(&self) -> PyResult<i128> {
        for attempt in 0..3 {
            match utils::read_int_raise(&self.path, Log::Off) {
                Ok(v) => return Ok(v),
                Err(e) => {
                    self.log_read_failure(&e, attempt);
                    if !(e.exc.is_os_error() && os_errno(&e) == Some(libc::ENXIO)) || attempt == 2 {
                        return Err(e);
                    }
                    sleep(PAUSE);
                }
            }
        }
        Err(PyErr::runtime_error("unreachable"))
    }

    /// `_log_boot_progress_read_failure(msg, attempt)`, the `log_func` the
    /// read was given: only what `read_from_file` catches reaches it --
    /// `OSError`, and `ValueError` with its subclasses.
    fn log_read_failure(&self, e: &PyErr, attempt: i32) {
        let value_error = utils::caught_value_error(e);
        if !(e.exc.is_os_error() || value_error) {
            return;
        }
        let msg = format!("Failed to read from file {} - {}", self.path, e.repr());
        let enxio = msg.contains(&format!("({},", libc::ENXIO));
        if enxio && attempt < 2 {
            self.log_warning(&format!("ENXIO - read unavailable for boot_progress, attempt {} of 3", attempt + 1));
            return;
        }
        self.log_error(&msg);
    }

    /// `_is_dpu_in_os_run(ctl)`: whatever goes wrong is "no".
    pub fn in_os_run(&self) -> bool {
        self.read().is_ok_and(|v| v == BOOT_PROG_OS_RUN)
    }
}

/// `DpuCtlPlat(dpu_name)`, set up as `DpuModule` sets it up: everything at
/// notice level, errors and warnings at theirs, verbose.
pub struct DpuCtlPlat {
    dpu_name: String,
    /// `_name`: hw-management's name, `dpu<N+1>`.
    pub hw_name: String,
    boot: BootProg,
    rst_path: String,
    pwr_path: String,
    pwr_f_path: String,
    dpu_rdy_path: String,
    shtdn_ready_path: String,
    /// `pci_dev_path_map`: PCIE_INT and RSHIM_PCIE_INT, once resolved.
    pci_paths: Mutex<Option<(String, String)>>,
}

impl DpuCtlPlat {
    pub fn new(dpu_name: &str) -> DpuCtlPlat {
        import();
        let n = dpu_name.get(3..).and_then(|s| s.parse::<i128>().ok()).unwrap_or(0);
        let hw_name = format!("{}{}", dpu_name.get(..3).unwrap_or(""), n + 1);
        DpuCtlPlat {
            dpu_name: dpu_name.to_string(),
            boot: BootProg::new(dpu_name, &hw_name, LogSink::Syslog),
            rst_path: format!("{SYSTEM_BASE}{hw_name}_rst"),
            pwr_path: format!("{SYSTEM_BASE}{hw_name}_pwr"),
            pwr_f_path: format!("{SYSTEM_BASE}{hw_name}_pwr_force"),
            dpu_rdy_path: format!("{EVENT_BASE}{hw_name}_ready"),
            shtdn_ready_path: format!("{EVENT_BASE}{hw_name}_shtdn_ready"),
            hw_name,
            pci_paths: Mutex::new(None),
        }
    }

    fn log_error(&self, msg: &str) {
        self.boot.log_error(msg);
    }

    fn log_warning(&self, msg: &str) {
        self.boot.log_warning(msg);
    }

    fn log_info(&self, msg: &str) {
        self.boot.log_info(msg);
    }

    /// `read_boot_prog()`.
    pub fn read_boot_prog(&self) -> PyResult<i128> {
        self.boot.read()
    }
}

impl DpuCtlPlat {
    /// `read_force_power_path()`.
    pub fn read_force_power_path(&self) -> PyResult<i128> {
        utils::read_int_raise(&self.pwr_f_path, Log::Error)
    }

    /// `write_file(file_name, content, log_on_error)`: a failure is logged
    /// twice -- by utils, then here -- and raised with the DPU's name.
    fn write_file(&self, file: &str, content: &str, log_on_error: bool) -> PyResult<()> {
        self.log_info(&format!("Writing {content} to file {file}"));
        let log = if log_on_error { Log::Error } else { Log::Off };
        match utils::write_file(file, &content, true, log) {
            Ok(_) => Ok(()),
            Err(e) => {
                if log_on_error {
                    self.log_error(&format!("Failed to write {content} to file {file}"));
                }
                Err(PyErr::new(e.exc.clone(), vec![PyVal::Str(format!("{}:{}", self.dpu_name, e.str()))]))
            }
        }
    }

    /// `time_check_context(msg)`: logs how long `f` took.
    fn timed<T>(&self, msg: &str, f: impl FnOnce() -> PyResult<T>) -> PyResult<T> {
        let start = Instant::now();
        let r = f()?;
        self.log_info(&format!("Total time taken = {} for {msg}", elapsed_repr(start)));
        Ok(r)
    }

    /// `boot_prog_context()`: the boot progress watched while `f` runs.
    ///
    /// Python's watcher is a forked process that says it is shutting down
    /// when terminated -- if it got as far as installing its SIGTERM handler.
    /// An operation that returns at once beats it, and the line is missing;
    /// here, one shorter than `WATCHER_STARTUP` does without it.
    ///
    /// The context manager's `except Exception` yields a second time, which
    /// is what an exception from `f` lands in: it is logged as a failure to
    /// create the watcher, and `contextmanager` then raises
    /// `RuntimeError("generator didn't stop after throw()")` in its place.
    /// Python terminates the watcher only when that generator is collected,
    /// after the caller has the exception; here, before it is returned.  A
    /// watcher that cannot be started is the same log line, and `f` runs
    /// unwatched with its own answer.
    fn with_boot_prog_watch<T>(&self, f: impl FnOnce() -> PyResult<T>) -> PyResult<T> {
        let stop = Arc::new(AtomicBool::new(false));
        let say_bye = Arc::new(AtomicBool::new(true));
        let boot = self.boot.clone();
        let (st, bye) = (stop.clone(), say_bye.clone());
        let process = PROCESS_COUNTER.fetch_add(1, Ordering::Relaxed) + 1;
        let watcher = match std::thread::Builder::new().spawn(move || watch_boot_prog(&boot, process, &st, &bye)) {
            Ok(w) => w,
            Err(_) => {
                self.log_error("Exception occured during creating boot_prog_context manager!");
                return f();
            }
        };
        let start = Instant::now();
        let r = f();
        if start.elapsed() < WATCHER_STARTUP {
            say_bye.store(false, Ordering::Relaxed);
        }
        // `except Exception`: what is not an `Exception` gets past it as it is.
        let caught = r.as_ref().err().is_some_and(is_exception);
        if caught {
            self.log_error("Exception occured during creating boot_prog_context manager!");
        }
        stop.store(true, Ordering::Relaxed);
        let _ = watcher.join();
        if caught {
            return Err(PyErr::runtime_error("generator didn't stop after throw()"));
        }
        r
    }

    fn wait_for(&self, path: &str, timeout: f64, what: &str) -> PyResult<bool> {
        let helper = InotifyHelper::new(path)?;
        self.timed(what, || helper.wait_watch(timeout, 1)).map(|v| v.is_some())
    }

    /// `dpu_go_down()`: the `try` covers making the helper and waiting on
    /// it, and renames only these two classes -- not, say, inotify's own
    /// `InotifyError`.
    fn dpu_go_down(&self) -> PyResult<bool> {
        self.write_file(&self.rst_path, CLR, true)?;
        let r = (|| -> PyResult<bool> {
            let h = InotifyHelper::new(&self.shtdn_ready_path)?;
            Ok(self.timed("going down", || h.wait_watch(WAIT_FOR_SHTDN, 1))?.is_some())
        })();
        let ready = match r {
            Ok(ready) => ready,
            Err(e) if matches!(e.exc.name(), "FileNotFoundError" | "PermissionError") => {
                return Err(PyErr::new(e.exc.clone(), vec![PyVal::Str(format!("{}:{}", self.dpu_name, e.str()))]))
            }
            Err(e) => return Err(e),
        };
        if !ready {
            self.log_warning("Going Down Unsuccessful");
            return Ok(false);
        }
        Ok(true)
    }

    fn power_off(&self) -> PyResult<bool> {
        if !self.dpu_go_down()? {
            return self.power_off_force();
        }
        self.write_file(&self.pwr_path, CLR, true)?;
        self.log_info("Power Off complete");
        Ok(true)
    }

    fn power_off_force(&self) -> PyResult<bool> {
        self.write_file(&self.rst_path, CLR, true)?;
        self.write_file(&self.pwr_f_path, CLR, true)?;
        self.log_info("Force Power Off complete");
        Ok(true)
    }

    fn power_on_force(&self, count: i32, no_wait: bool) -> PyResult<bool> {
        if count < 4 {
            self.log_error(&format!("Failed Force Power on! Retry {}..", 4 - count));
        }
        self.write_file(&self.pwr_f_path, SET, true)?;
        self.write_file(&self.rst_path, SET, true)?;
        if no_wait {
            self.log_info("Exiting without checking result of reboot command");
            return Ok(true);
        }
        if !self.wait_for(&self.dpu_rdy_path, WAIT_FOR_DPU_READY, "power on force")? {
            if count > 1 {
                sleep(PAUSE);
                self.power_off_force()?;
                sleep(PAUSE);
                return self.power_on_force(count - 1, false);
            }
            self.log_error("Failed Force power on! Exiting");
            return Ok(false);
        }
        self.log_info("Force Power on Successful!");
        Ok(true)
    }

    fn power_on(&self) -> PyResult<bool> {
        self.write_file(&self.pwr_path, SET, true)?;
        self.write_file(&self.rst_path, SET, true)?;
        if !self.wait_for(&self.dpu_rdy_path, WAIT_FOR_DPU_READY, "power on")? {
            self.log_error("Failed power on! Trying Force Power on");
            self.power_off_force()?;
            sleep(PAUSE);
            return self.power_on_force(3, false);
        }
        self.log_info("Power on Successful!");
        Ok(true)
    }

    /// `get_pci_dev_path_map()`: PCIE_INT and RSHIM_PCIE_INT from
    /// platform.json, or the RuntimeError naming the one that is missing.
    pub fn pci_dev_paths(&self) -> PyResult<(String, String)> {
        let mut slot = utils::lock(&self.pci_paths);
        if let Some(p) = slot.as_ref() {
            return Ok(p.clone());
        }
        let mut out = Vec::new();
        for key in ["bus_info", "rshim_bus_info"] {
            let id = dpu_interface(&self.dpu_name, key)?;
            let Some(id) = id.filter(PyVal::truthy) else {
                return Err(PyErr::runtime_error(format!(
                    "Unable to obtain PCI device ID ({key}) for {} from platform.json",
                    self.dpu_name
                )));
            };
            out.push(io::join(PCI_DEV_BASE, &id.py_str()));
        }
        let paths = (out[0].clone(), out[1].clone());
        *slot = Some(paths.clone());
        Ok(paths)
    }

    /// `dpu_pci_remove()`: unbind the DPU's main function from mlx5_core.
    pub fn dpu_pci_remove(&self) -> bool {
        let r = (|| -> PyResult<()> {
            let (pci, rshim) = self.pci_dev_paths()?;
            let bdf = pci.rsplit('/').next().unwrap_or("").to_string();
            if io::exists(&io::join(&pci, "driver")) && io::exists(MLX5_CORE_UNBIND_PATH) {
                self.timed(&format!("pci unbind {pci}"), || self.write_file(MLX5_CORE_UNBIND_PATH, &bdf, true))?;
            } else {
                self.log_info(&format!(
                    "Skipping unbind for {pci}: driver not bound or mlx5_core unbind path missing"
                ));
            }
            self.log_info(&format!("Skip pci removal for SOC PCIE dev {rshim}"));
            Ok(())
        })();
        match r {
            Ok(()) => true,
            Err(e) => {
                self.log_error(&format!("Failed PCI Removal with error {}", e.str()));
                false
            }
        }
    }

    /// `_dpu_pci_bind(pci_dev_path, bdf)`: once its BARs are assigned.
    fn dpu_pci_bind(&self, pci: &str, bdf: &str) -> PyResult<()> {
        let resource = io::join(pci, "resource");
        let deadline = Instant::now() + Duration::from_secs_f64(WAIT_FOR_PCI_BINDABLE);
        loop {
            // `any(int(line.split()[0], 16) for line in f)`: line by line, the
            // first non-zero start is ready; a line that does not parse
            // raises first, which is caught as not ready yet.
            let ready = io::read_text(&resource).ok().is_some_and(|t| {
                for l in t.lines() {
                    let Some(w) = platform_base::py::split_ws(l).first().copied() else { return false };
                    match platform_base::py::py_int_base(w, 16) {
                        Ok(0) => {}
                        Ok(_) => return true,
                        Err(_) => return false,
                    }
                }
                false
            });
            if ready {
                break;
            }
            if Instant::now() >= deadline {
                self.log_warning(&format!(
                    "PCI device {pci} did not become bindable within {}s",
                    WAIT_FOR_PCI_BINDABLE as i64
                ));
                return Ok(());
            }
            sleep(Duration::from_secs_f64(PCI_POLL_INTERVAL));
        }
        let driver = io::join(pci, "driver");
        if io::exists(&driver) {
            self.log_info(&format!("Driver already bound for {pci}, skip bind"));
            return Ok(());
        }
        self.timed(&format!("pci bind {pci}"), || match self.write_file(MLX5_CORE_BIND_PATH, bdf, false) {
            Ok(()) => Ok(()),
            Err(e) if e.exc.is_os_error() => {
                if io::exists(&driver) && io::realpath(&driver) == io::realpath(MLX5_CORE_DRIVER_PATH) {
                    self.log_info(&format!("Driver bound concurrently for {pci}, skip bind"));
                    return Ok(());
                }
                self.log_error(&format!("Failed to bind mlx5_core for {pci}"));
                Err(e)
            }
            Err(e) => Err(e),
        })
    }

    /// `dpu_pci_scan()`: bind the main function back; check the SoC one is
    /// on the bus.
    pub fn dpu_pci_scan(&self) -> bool {
        let r = (|| -> PyResult<()> {
            let (pci, rshim) = self.pci_dev_paths()?;
            let bdf = pci.rsplit('/').next().unwrap_or("").to_string();
            if io::exists(&io::join(&pci, "driver")) {
                self.log_info(&format!("Driver already bound for {pci}, skip bind"));
            } else if io::exists(&pci) && io::exists(MLX5_CORE_BIND_PATH) {
                self.dpu_pci_bind(&pci, &bdf)?;
            } else if !io::exists(&pci) {
                self.log_warning(&format!("PCI device {pci} not found"));
            } else {
                self.log_warning(&format!(
                    "mlx5_core bind path {MLX5_CORE_BIND_PATH} not found, skipping bind for {pci}"
                ));
            }
            if !io::exists(&rshim) {
                self.log_warning(&format!("PCI device {rshim} not found"));
            }
            Ok(())
        })();
        match r {
            Ok(()) => true,
            Err(e) => {
                self.log_error(&format!("Failed to rescan with error {}", e.str()));
                false
            }
        }
    }

    /// `dpu_power_on(forced, skip_pre_post)`.
    pub fn dpu_power_on(&self, forced: bool, skip_pre_post: bool) -> PyResult<bool> {
        self.with_boot_prog_watch(|| {
            self.log_info(&format!("Power on with force = {}", if forced { "True" } else { "False" }));
            let rv = if self.read_boot_prog()? == crate::dpuctlplat::BOOT_PROG_OS_RUN {
                self.log_info("Skipping DPU power on as DPU is already powered on");
                true
            } else if forced {
                self.power_on_force(4, false)?
            } else if self.read_force_power_path()? == 0 {
                self.log_info("Power on with Force=True since power off force sysfs is cleared");
                self.power_on_force(4, false)?
            } else {
                self.power_on()?
            };
            if !skip_pre_post {
                self.dpu_pci_scan();
            }
            Ok(rv)
        })
    }

    /// `dpu_power_off(forced, skip_pre_post)`.
    pub fn dpu_power_off(&self, forced: bool, skip_pre_post: bool) -> PyResult<bool> {
        self.with_boot_prog_watch(|| {
            if !skip_pre_post {
                self.dpu_pci_remove();
            }
            self.log_info(&format!("Power off with force = {}", if forced { "True" } else { "False" }));
            if self.read_boot_prog()? == BOOT_PROG_RST {
                self.log_info("Skipping DPU power off as DPU is already powered off");
                return Ok(true);
            }
            if forced {
                return self.power_off_force();
            }
            self.power_off()
        })
    }

    fn reboot(&self, no_wait: bool) -> PyResult<bool> {
        if !self.dpu_go_down()? {
            self.power_off_force()?;
        }
        self.write_file(&self.rst_path, SET, true)?;
        if no_wait {
            self.log_info("Exiting without checking result of reboot command");
            return Ok(true);
        }
        if !self.wait_for(&self.dpu_rdy_path, WAIT_FOR_DPU_READY, "power on")? {
            self.power_off_force()?;
            sleep(PAUSE);
            return self.power_on_force(4, false);
        }
        Ok(true)
    }

    fn reboot_force(&self, no_wait: bool) -> PyResult<bool> {
        self.power_off_force()?;
        sleep(PAUSE);
        self.power_on_force(4, no_wait)
    }

    /// `dpu_reboot(forced, no_wait, skip_pre_post)`.
    pub fn dpu_reboot(&self, forced: bool, no_wait: bool, skip_pre_post: bool) -> PyResult<bool> {
        self.with_boot_prog_watch(|| {
            if !skip_pre_post {
                self.dpu_pci_remove();
            }
            self.log_info(&format!("Reboot with force = {}", if forced { "True" } else { "False" }));
            let rv = if forced { self.reboot_force(no_wait)? } else { self.reboot(no_wait)? };
            if !no_wait && !skip_pre_post {
                self.dpu_pci_scan();
            }
            if rv {
                self.log_info("Reboot Complete");
            }
            Ok(rv)
        })
    }
}

/// `DeviceDataManager.get_dpu_interface(dpu, interface)`:
/// `dpu_data.get(dpu, {}).get(interface)`, which raises AttributeError when
/// the DPUS table or the DPU's entry in it is not a dict.
pub fn dpu_interface(dpu: &str, key: &str) -> PyResult<Option<PyVal>> {
    const DPU_INTERFACE_VALUES: [&str; 4] = ["midplane_interface", "rshim_info", "bus_info", "rshim_bus_info"];
    let data = device_data::get_platform_dpus_data()?;
    let Some(d) = data.filter(PyVal::truthy) else {
        return Ok(None);
    };
    if !DPU_INTERFACE_VALUES.contains(&key) {
        return Ok(None);
    }
    let entry = ops::get(&d, dpu, PyVal::Dict(Vec::new()))?;
    Ok(Some(ops::get(&entry, key, PyVal::None)?).filter(|v| !v.is_none()))
}

/// `multiprocessing`'s process counter: the watcher's name is
/// `Process-<n>`, `n` counting every one started in this process.
static PROCESS_COUNTER: AtomicUsize = AtomicUsize::new(0);

/// `update_boot_prog_once(poll_var)` once the poll has answered: the read
/// is `read_boot_prog`'s, retries and lines included, and a change is
/// logged.  `dpu_boot_prog_update(read_value)` finds a 0 falsy and reads
/// again.
fn update_boot_prog_once(boot: &BootProg, state: &mut Option<i128>) -> PyResult<()> {
    let v = boot.read()?;
    if Some(v) == *state {
        return Ok(());
    }
    let v = if v != 0 {
        v
    } else {
        boot.read().inspect_err(|_| boot.log_error("Could not update boot_progress of DPU"))?
    };
    *state = Some(v);
    boot.log_info(&format!("The boot_progress status is changed to = {v} - {}", boot_prog_name(v)));
    Ok(())
}

/// `watch_boot_prog()`, in the watcher: log each change of the boot
/// progress until told to stop, then say so as the SIGTERM handler does.
///
/// Whatever a read raises ends the watch with an error line, and the child
/// exits before any SIGTERM, without its goodbye.  The file is opened
/// before the `try`: failing to, the child dies with its traceback on
/// stderr, and nothing in syslog.
fn watch_boot_prog(boot: &BootProg, process: usize, stop: &AtomicBool, say_bye: &AtomicBool) {
    let file = match std::fs::File::open(io::real(&boot.path)) {
        Ok(f) => f,
        Err(e) => {
            let e = PyErr::from_io(&e, Some(&boot.path));
            let _ = writeln!(
                std::io::stderr(),
                "Process Process-{process}:\nTraceback (most recent call last):\n{}: {}",
                e.exc.name(),
                e.str()
            );
            return;
        }
    };
    let mut state: Option<i128> = None;
    while !stop.load(Ordering::Relaxed) {
        let mut pfd = libc::pollfd { fd: file.as_raw_fd(), events: libc::POLLPRI, revents: 0 };
        // SAFETY: one valid pollfd.
        let n = unsafe { libc::poll(&mut pfd, 1, 100) };
        if n <= 0 {
            continue;
        }
        if let Err(e) = update_boot_prog_once(boot, &mut state) {
            boot.log_error(&format!("Error during watch_boot_progress: {}", e.str()));
            return;
        }
    }
    if say_bye.load(Ordering::Relaxed) {
        boot.log_info("Received termination signal, shutting down...");
    }
}

/// The errno an `OSError` carries as its first argument.
fn os_errno(e: &PyErr) -> Option<i32> {
    match e.args.first() {
        Some(platform_base::PyVal::Int(n)) => i32::try_from(*n).ok(),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_d::{platform, Syslog};
    use crate::testing::Root;

    const DPUS: &str = r#"{"DPUS": {"dpu0": {"bus_info": "0000:08:00.0", "rshim_bus_info": "0000:08:00.1", "midplane_interface": "dpu0"}}}"#;
    const BOOT: &str = "/var/run/hw-management/dpu1/system/boot_progress";
    const RST: &str = "/var/run/hw-management/system/dpu1_rst";
    const PWR: &str = "/var/run/hw-management/system/dpu1_pwr";
    const PWR_F: &str = "/var/run/hw-management/system/dpu1_pwr_force";
    const READY: &str = "/var/run/hw-management/events/dpu1_ready";
    const SHTDN: &str = "/var/run/hw-management/events/dpu1_shtdn_ready";
    const PCI: &str = "/sys/bus/pci/devices/0000:08:00.0";
    const RSHIM: &str = "/sys/bus/pci/devices/0000:08:00.1";

    /// DPU0 as hw-management shows it: booted to `boot`, the force-power
    /// switch at `pwr_f`, and what the ready and shutdown-ready events hold.
    fn dpu(root: &Root, boot: &str, pwr_f: &str, ready: &str, shtdn: &str) -> DpuCtlPlat {
        platform(root, "x86_64-nvidia_sn4280-r0", DPUS);
        root.write(BOOT, boot);
        root.write(PWR, "0");
        root.write(PWR_F, pwr_f);
        root.write(RST, "0");
        root.write(READY, ready);
        root.write(SHTDN, shtdn);
        DpuCtlPlat::new("dpu0")
    }

    /// The DPU's own lines, with how long an operation took left out.
    fn lines(log: &Syslog) -> Vec<String> {
        log.take()
            .into_iter()
            .filter(|l| l.contains("dpu0:"))
            .map(|l| match l.find("Total time taken = ") {
                Some(i) => format!("{}Total time taken for{}", &l[..i], l[i..].split_once(" for").map_or("", |(_, r)| r)),
                None => l,
            })
            .collect()
    }

    #[test]
    fn hw_management_numbers_the_dpu_from_one() {
        let d = DpuCtlPlat::new("dpu3");
        assert_eq!(d.hw_name, "dpu4");
        assert_eq!(d.boot.path, "/var/run/hw-management/dpu4/system/boot_progress");
        assert_eq!(d.rst_path, "/var/run/hw-management/system/dpu4_rst");
        assert_eq!(d.pwr_f_path, "/var/run/hw-management/system/dpu4_pwr_force");
        assert_eq!(d.dpu_rdy_path, "/var/run/hw-management/events/dpu4_ready");
        assert_eq!(d.shtdn_ready_path, "/var/run/hw-management/events/dpu4_shtdn_ready");
        // `boot_prog_map`, and N/A for what it lacks.
        let names: Vec<&str> = (0..=16).map(boot_prog_name).collect();
        assert_eq!(
            names,
            [
                "Reset/Boot-ROM",
                "BL2 (from ATF image on eMMC partition)",
                "BL31 (from ATF image on eMMC partition)",
                "UEFI (from UEFI image on eMMC partition)",
                "OS Starting",
                "OS is running",
                "Low-Power Standby",
                "FW Update in progress",
                "OS Crash Dump in progress",
                "OS Crash Dump is complete",
                "FW Fault Crash Dump in progress",
                "FW Fault Crash Dump is complete",
                "N/A",
                "N/A",
                "N/A",
                "Software is inactive",
                "N/A",
            ]
        );
    }

    #[test]
    fn boot_progress_retries_only_while_the_file_answers_enxio() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "0", "0");
        assert_eq!(d.read_boot_prog().unwrap(), 5);
        root.write(BOOT, "x");
        let e = d.read_boot_prog().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("ValueError", "invalid literal for int() with base 10: 'x'".to_string()));
        std::fs::remove_file(root.real(BOOT)).unwrap();
        assert_eq!(d.read_boot_prog().unwrap_err().exc.name(), "FileNotFoundError");
        // A socket answers open() with ENXIO.
        let _sock = std::os::unix::net::UnixListener::bind(root.real(BOOT)).unwrap();
        let e = d.read_boot_prog().unwrap_err();
        assert_eq!(e.str(), format!("[Errno 6] No such device or address: '{BOOT}'"));
        assert_eq!(
            lines(&log),
            [
                format!("ERROR dpu0: Failed to read from file {BOOT} - ValueError(\"invalid literal for int() with base 10: 'x'\")"),
                format!("ERROR dpu0: Failed to read from file {BOOT} - FileNotFoundError(2, 'No such file or directory')"),
                "WARNING dpu0: ENXIO - read unavailable for boot_progress, attempt 1 of 3".to_string(),
                "WARNING dpu0: ENXIO - read unavailable for boot_progress, attempt 2 of 3".to_string(),
                format!("ERROR dpu0: Failed to read from file {BOOT} - OSError(6, 'No such device or address')"),
            ]
        );
    }

    #[test]
    fn power_on_waits_for_the_dpu_to_say_it_is_ready() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "0", "1", "1", "0");
        assert!(d.dpu_power_on(false, true).unwrap());
        assert_eq!((root.read(PWR), root.read(RST)), (Some("1".into()), Some("1".into())));
        assert_eq!(
            lines(&log),
            [
                "NOTICE dpu0: Power on with force = False".to_string(),
                format!("NOTICE dpu0: Writing 1 to file {PWR}"),
                format!("NOTICE dpu0: Writing 1 to file {RST}"),
                "NOTICE dpu0: Total time taken for power on".to_string(),
                "NOTICE dpu0: Power on Successful!".to_string(),
                "NOTICE dpu0: Received termination signal, shutting down...".to_string(),
            ]
        );
    }

    #[test]
    fn power_on_falls_back_to_forcing_and_gives_up_after_three_retries() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "0", "1", "0", "0");
        assert!(!d.dpu_power_on(false, true).unwrap());
        let got: Vec<String> = lines(&log).into_iter().filter(|l| !l.contains("Writing") && !l.contains("Total time")).collect();
        assert_eq!(
            got,
            [
                "NOTICE dpu0: Power on with force = False",
                "ERROR dpu0: Failed power on! Trying Force Power on",
                "NOTICE dpu0: Force Power Off complete",
                "ERROR dpu0: Failed Force Power on! Retry 1..",
                "NOTICE dpu0: Force Power Off complete",
                "ERROR dpu0: Failed Force Power on! Retry 2..",
                "NOTICE dpu0: Force Power Off complete",
                "ERROR dpu0: Failed Force Power on! Retry 3..",
                "ERROR dpu0: Failed Force power on! Exiting",
                "NOTICE dpu0: Received termination signal, shutting down...",
            ]
        );
        assert_eq!((root.read(PWR_F), root.read(RST)), (Some("1".into()), Some("1".into())));
    }

    #[test]
    fn power_on_is_forced_when_asked_or_when_force_power_is_off() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "0", "1", "1", "0");
        assert!(d.dpu_power_on(true, true).unwrap());
        assert!(lines(&log).contains(&"NOTICE dpu0: Force Power on Successful!".to_string()));
        assert_eq!((root.read(PWR_F), root.read(PWR)), (Some("1".into()), Some("0".into())));
        root.write(PWR_F, "0");
        assert!(d.dpu_power_on(false, true).unwrap());
        let l = lines(&log);
        assert!(l.contains(&"NOTICE dpu0: Power on with Force=True since power off force sysfs is cleared".to_string()));
        assert!(l.contains(&"NOTICE dpu0: Force Power on Successful!".to_string()));
        // Already running: nothing written.
        root.write(BOOT, "5");
        root.write(PWR_F, "0");
        assert!(d.dpu_power_on(false, true).unwrap());
        assert!(lines(&log).contains(&"NOTICE dpu0: Skipping DPU power on as DPU is already powered on".to_string()));
        assert_eq!(root.read(PWR_F).as_deref(), Some("0"));
    }

    #[test]
    fn power_off_goes_down_gracefully_or_by_force() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "0", "1");
        root.write(PWR, "1");
        assert!(d.dpu_power_off(false, true).unwrap());
        assert_eq!((root.read(RST), root.read(PWR), root.read(PWR_F)), (Some("0".into()), Some("0".into()), Some("1".into())));
        let l = lines(&log);
        assert!(l.contains(&"NOTICE dpu0: Total time taken for going down".to_string()));
        assert!(l.contains(&"NOTICE dpu0: Power Off complete".to_string()));
        // The DPU never says it is ready to go down: forced.
        root.write(SHTDN, "0");
        root.write(PWR, "1");
        assert!(d.dpu_power_off(false, true).unwrap());
        assert_eq!((root.read(PWR), root.read(PWR_F)), (Some("1".into()), Some("0".into())));
        let l = lines(&log);
        assert!(l.contains(&"WARNING dpu0: Going Down Unsuccessful".to_string()));
        assert!(l.contains(&"NOTICE dpu0: Force Power Off complete".to_string()));
        // Forced from the start.
        root.write(PWR_F, "1");
        assert!(d.dpu_power_off(true, true).unwrap());
        assert_eq!(root.read(PWR_F).as_deref(), Some("0"));
        // Already off.
        root.write(BOOT, "0");
        assert!(d.dpu_power_off(false, true).unwrap());
        assert!(lines(&log).contains(&"NOTICE dpu0: Skipping DPU power off as DPU is already powered off".to_string()));
    }

    #[test]
    fn going_down_without_the_event_file_raises_with_the_dpu_named() {
        let root = Root::new();
        let d = dpu(&root, "5", "1", "0", "0");
        std::fs::remove_file(root.real(SHTDN)).unwrap();
        let e = d.dpu_go_down().unwrap_err();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("FileNotFoundError", format!("dpu0:[Errno 2] No such file or directory: '{SHTDN}'"))
        );
        // The file gone while it is waited on: renamed the same.
        root.write(SHTDN, "0");
        let path = root.real(SHTDN);
        let gone = std::thread::spawn(move || {
            sleep(Duration::from_millis(5));
            std::fs::remove_file(path).unwrap();
        });
        let e = d.dpu_go_down().unwrap_err();
        gone.join().unwrap();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("FileNotFoundError", format!("dpu0:[Errno 2] No such file or directory: '{SHTDN}'"))
        );
        // Another class keeps its own words.
        std::fs::create_dir_all(root.real(SHTDN)).unwrap();
        let e = d.dpu_go_down().unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("IsADirectoryError", format!("[Errno 21] Is a directory: '{SHTDN}'")));
    }

    /// Whatever an operation raises lands in the watcher's context manager,
    /// which logs it as its own failure and raises its generator's error.
    #[test]
    fn an_operation_that_raises_answers_the_context_manager_s_error() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "0", "0");
        std::fs::remove_dir_all(root.real("/var/run/hw-management/system")).unwrap();
        let e = d.dpu_power_off(true, true).unwrap_err();
        assert_eq!((e.exc.name(), e.str()), ("RuntimeError", "generator didn't stop after throw()".to_string()));
        let l = lines(&log);
        assert!(l.contains(&format!("ERROR dpu0: Failed to write 0 to file {RST}")));
        assert!(l.contains(&"ERROR dpu0: Exception occured during creating boot_prog_context manager!".to_string()));
        // The same from going down without its event file.
        drop((log, root));
        let root = Root::new();
        let d = dpu(&root, "5", "1", "0", "0");
        std::fs::remove_file(root.real(SHTDN)).unwrap();
        let e = d.dpu_reboot(false, true, true).unwrap_err();
        assert_eq!(e.str(), "generator didn't stop after throw()");
    }

    /// Without boot progress to open, the watcher dies at once -- its
    /// traceback on stderr, nothing in syslog -- and the operation runs.
    #[test]
    fn a_watcher_that_cannot_open_the_boot_progress_says_nothing_in_syslog() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "0", "1");
        std::fs::remove_file(root.real(BOOT)).unwrap();
        // Off by force: boot progress is read first, and raises.
        assert_eq!(d.dpu_power_off(true, true).unwrap_err().str(), "generator didn't stop after throw()");
        let l = lines(&log);
        assert!(!l.iter().any(|l| l.contains("watch_boot_progress") || l.contains("Received termination")), "{l:?}");
        assert!(l.contains(&format!("ERROR dpu0: Failed to read from file {BOOT} - FileNotFoundError(2, 'No such file or directory')")));
    }

    /// Each poll's read is `read_boot_prog`'s; a change is logged, the same
    /// value again is not, and an error goes back to the loop, which ends
    /// the watch on it.
    #[test]
    fn the_watcher_reads_as_read_boot_prog_does() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "0", "1", "1", "0");
        let mut state = None;
        update_boot_prog_once(&d.boot, &mut state).unwrap();
        update_boot_prog_once(&d.boot, &mut state).unwrap();
        root.write(BOOT, "5");
        update_boot_prog_once(&d.boot, &mut state).unwrap();
        assert_eq!(state, Some(5));
        root.write(BOOT, "x");
        let e = update_boot_prog_once(&d.boot, &mut state).unwrap_err();
        assert_eq!(e.str(), "invalid literal for int() with base 10: 'x'");
        assert_eq!(
            lines(&log),
            [
                "NOTICE dpu0: The boot_progress status is changed to = 0 - Reset/Boot-ROM".to_string(),
                "NOTICE dpu0: The boot_progress status is changed to = 5 - OS is running".to_string(),
                format!("ERROR dpu0: Failed to read from file {BOOT} - ValueError(\"invalid literal for int() with base 10: 'x'\")"),
            ]
        );
    }

    #[test]
    fn reboot_goes_down_comes_back_and_retries_by_force() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "1", "1");
        assert!(d.dpu_reboot(false, false, true).unwrap());
        assert_eq!(root.read(RST).as_deref(), Some("1"));
        let l = lines(&log);
        assert!(l.contains(&"NOTICE dpu0: Reboot with force = False".to_string()));
        assert!(l.contains(&"NOTICE dpu0: Reboot Complete".to_string()));
        // Not going down is forced down; not waiting returns at once.
        root.write(SHTDN, "0");
        assert!(d.dpu_reboot(false, true, true).unwrap());
        let l = lines(&log);
        assert!(l.contains(&"NOTICE dpu0: Force Power Off complete".to_string()));
        assert!(l.contains(&"NOTICE dpu0: Exiting without checking result of reboot command".to_string()));
        // Forced, without waiting.
        assert!(d.dpu_reboot(true, true, true).unwrap());
        assert_eq!((root.read(PWR_F), root.read(RST)), (Some("1".into()), Some("1".into())));
        // Forced, waiting: ready.
        assert!(d.dpu_reboot(true, false, true).unwrap());
        // Never ready: forced power on, four times, then no.
        lines(&log);
        root.write(READY, "0");
        root.write(SHTDN, "1");
        assert!(!d.dpu_reboot(false, false, true).unwrap());
        let l = lines(&log);
        assert!(l.contains(&"ERROR dpu0: Failed Force power on! Exiting".to_string()));
        assert!(!l.contains(&"NOTICE dpu0: Reboot Complete".to_string()));
    }

    #[test]
    fn pre_and_post_steps_unbind_and_bind_the_pci_function() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "1", "1");
        root.write(&format!("{PCI}/driver/x"), "");
        root.write(MLX5_CORE_UNBIND_PATH, "");
        root.write(&format!("{RSHIM}/x"), "");
        assert!(d.dpu_reboot(false, false, false).unwrap());
        assert_eq!(root.read(MLX5_CORE_UNBIND_PATH).as_deref(), Some("0000:08:00.0"));
        let l = lines(&log);
        assert!(l.contains(&format!("NOTICE dpu0: Skip pci removal for SOC PCIE dev {RSHIM}")));
        assert!(l.contains(&format!("NOTICE dpu0: Driver already bound for {PCI}, skip bind")));
        // Off: removed first.  On: scanned after.
        assert!(d.dpu_power_off(true, false).unwrap());
        root.write(BOOT, "0");
        assert!(d.dpu_power_on(true, false).unwrap());
        assert_eq!(d.pci_dev_paths().unwrap(), (PCI.to_string(), RSHIM.to_string()));
    }

    #[test]
    fn a_pci_function_is_bound_once_its_bars_are_assigned() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let d = dpu(&root, "5", "1", "1", "1");
        root.write(MLX5_CORE_BIND_PATH, "");
        root.write(&format!("{PCI}/resource"), "0x0000000000000000 0x0000000000000000 0x0\n0x00000000fa000000 0x00000000fbffffff 0x40200\n");
        assert!(d.dpu_pci_remove());
        assert!(lines(&log).contains(&format!(
            "NOTICE dpu0: Skipping unbind for {PCI}: driver not bound or mlx5_core unbind path missing"
        )));
        assert!(d.dpu_pci_scan());
        assert_eq!(root.read(MLX5_CORE_BIND_PATH).as_deref(), Some("0000:08:00.0"));
        let l = lines(&log);
        assert!(l.contains(&format!("NOTICE dpu0: Total time taken for pci bind {PCI}")));
        assert!(l.contains(&format!("WARNING dpu0: PCI device {RSHIM} not found")));
        // No BARs: given up on.
        root.write(MLX5_CORE_BIND_PATH, "");
        root.write(&format!("{PCI}/resource"), "0x0000000000000000 0x0000000000000000 0x0\n");
        assert!(d.dpu_pci_scan());
        assert_eq!(root.read(MLX5_CORE_BIND_PATH).as_deref(), Some(""));
        assert!(lines(&log).contains(&format!("WARNING dpu0: PCI device {PCI} did not become bindable within 0s")));
        // A line that does not parse raises before a later non-zero one is
        // read: still not bindable.
        root.write(&format!("{PCI}/resource"), "\n0x00000000fa000000 0x00000000fbffffff 0x40200\n");
        assert!(d.dpu_pci_scan());
        assert_eq!(root.read(MLX5_CORE_BIND_PATH).as_deref(), Some(""));
        // The bind failing.
        std::fs::remove_file(root.real(MLX5_CORE_BIND_PATH)).unwrap();
        std::fs::create_dir_all(root.real(MLX5_CORE_BIND_PATH)).unwrap();
        root.write(&format!("{PCI}/resource"), "0x00000000fa000000 0x00000000fbffffff 0x40200\n");
        assert!(!d.dpu_pci_scan());
        let l = lines(&log);
        assert!(l.contains(&format!("ERROR dpu0: Failed to bind mlx5_core for {PCI}")));
        assert!(l.contains(&format!(
            "ERROR dpu0: Failed to rescan with error dpu0:[Errno 21] Is a directory: '{MLX5_CORE_BIND_PATH}'"
        )));
        // Bound by the kernel while the BARs were awaited.
        root.write(&format!("{PCI}/driver/x"), "");
        d.dpu_pci_bind(PCI, "0000:08:00.0").unwrap();
        assert!(lines(&log).contains(&format!("NOTICE dpu0: Driver already bound for {PCI}, skip bind")));
        std::fs::remove_dir_all(root.real(&format!("{PCI}/driver"))).unwrap();
        // No bind path; no device.
        std::fs::remove_dir(root.real(MLX5_CORE_BIND_PATH)).unwrap();
        assert!(d.dpu_pci_scan());
        assert!(lines(&log).contains(&format!(
            "WARNING dpu0: mlx5_core bind path {MLX5_CORE_BIND_PATH} not found, skipping bind for {PCI}"
        )));
        std::fs::remove_dir_all(root.real(PCI)).unwrap();
        assert!(d.dpu_pci_scan());
        assert!(lines(&log).contains(&format!("WARNING dpu0: PCI device {PCI} not found")));
    }

    #[test]
    fn without_pci_ids_in_platform_json_pci_steps_fail() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        platform(&root, "x86_64-nvidia_sn4280-r0", r#"{"DPUS": {"dpu0": {"bus_info": "0000:08:00.0"}}}"#);
        let d = DpuCtlPlat::new("dpu0");
        let e = d.pci_dev_paths().unwrap_err();
        assert_eq!(e.str(), "Unable to obtain PCI device ID (rshim_bus_info) for dpu0 from platform.json");
        assert!(!d.dpu_pci_remove());
        assert!(!d.dpu_pci_scan());
        let l = lines(&log);
        assert!(l.contains(&"ERROR dpu0: Failed PCI Removal with error Unable to obtain PCI device ID (rshim_bus_info) for dpu0 from platform.json".to_string()));
        assert!(l.contains(&"ERROR dpu0: Failed to rescan with error Unable to obtain PCI device ID (rshim_bus_info) for dpu0 from platform.json".to_string()));
        assert_eq!(dpu_interface("dpu0", "bus_info").unwrap().map(|v| v.py_str()).as_deref(), Some("0000:08:00.0"));
        assert!(dpu_interface("dpu0", "midplane_interface").unwrap().is_none());
        assert!(dpu_interface("dpu7", "bus_info").unwrap().is_none());
        // Not one of `DpuInterfaceEnum`'s values.
        assert!(dpu_interface("dpu0", "bogus").unwrap().is_none());
        platform(&root, "x86_64-nvidia_sn4280-r0", r#"{"chassis": {}}"#);
        crate::utils::forget_caches();
        assert!(dpu_interface("dpu0", "bus_info").unwrap().is_none());
        let e = DpuCtlPlat::new("dpu0").pci_dev_paths().unwrap_err();
        assert_eq!(e.str(), "Unable to obtain PCI device ID (bus_info) for dpu0 from platform.json");
        // `.get` on what is not a dict.
        for (json, what) in [(r#"{"DPUS": {"dpu0": null}}"#, "NoneType"), (r#"{"DPUS": ["dpu0"]}"#, "list")] {
            platform(&root, "x86_64-nvidia_sn4280-r0", json);
            crate::utils::forget_caches();
            let e = dpu_interface("dpu0", "bus_info").unwrap_err();
            assert_eq!((e.exc.name(), e.str()), ("AttributeError", format!("'{what}' object has no attribute 'get'")));
        }
    }
}
