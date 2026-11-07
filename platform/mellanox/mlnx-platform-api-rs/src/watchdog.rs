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



//! `sonic_platform/watchdog.py`: the main hardware watchdog, found by its
//! sysfs identity and driven with the watchdog ioctls.
//!
//! The device is opened only for an ioctl -- on first `arm` or `disarm` --
//! and stays open as long as the object, as Python's `_watchdog` handle does.
//! Opening it is what starts the kernel's timer, so nothing else here opens it.

use std::fs::OpenOptions;
use std::os::fd::{AsRawFd, OwnedFd};
use std::sync::Mutex;

use platform_base::py::{float_repr, PyErr, PyResult, PyVal};
use platform_base::{io, BaseClass, PyObject};

use crate::device_data;
use crate::utils::{self, Log};

const IO_READ: u32 = 0x8000_0000;
const IO_READ_WRITE: u32 = 0xC000_0000;
const IO_SIZE_INT: u32 = 0x0004_0000;
const IO_TYPE_WATCHDOG: u32 = (b'W' as u32) << 8;
const WDR_INT: u32 = IO_READ | IO_SIZE_INT | IO_TYPE_WATCHDOG;
const WDWR_INT: u32 = IO_READ_WRITE | IO_SIZE_INT | IO_TYPE_WATCHDOG;

const WDIOC_SETOPTIONS: u32 = 4 | WDR_INT;
const WDIOC_KEEPALIVE: u32 = 5 | WDR_INT;
const WDIOC_SETTIMEOUT: u32 = 6 | WDWR_INT;

const WDIOS_DISABLECARD: i32 = 0x0001;
const WDIOS_ENABLECARD: i32 = 0x0002;

const WD_MLNX_MAIN_IDENTITY: &str = "mlx-wdt-main";
const WD_SYSFS_PATH: &str = "/sys/class/watchdog/";
const WD_COMMON_ERROR: i128 = -1;

const STATE_FILE: &str = "/run/hw-management/watchdog/main/state";
const TIMEOUT_FILE: &str = "/run/hw-management/watchdog/main/timeout";
const TIMELEFT_FILE: &str = "/run/hw-management/watchdog/main/timeleft";
const TIMESTAMP_DIR: &str = "/tmp/nvidia";
/// `WatchdogType1.TIMESTAMP_FILE`.
const TIMESTAMP_FILE: &str = "/tmp/nvidia/watchdog_timestamp";

/// `WatchdogType1` keeps the time it was armed in a file, since it cannot
/// ask the hardware for the time left; `WatchdogType2` can.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Type {
    Type1,
    Type2,
}

pub struct Watchdog {
    kind: Type,
    watchdog_path: String,
    /// `self._watchdog`: the handle, once an ioctl needed it.
    handle: Mutex<Option<OwnedFd>>,
    /// `self.timeout`.
    timeout: Mutex<i128>,
}

/// `time.monotonic()`.
fn monotonic() -> f64 {
    let mut ts = libc::timespec { tv_sec: 0, tv_nsec: 0 };
    // SAFETY: a valid timespec to fill.
    unsafe { libc::clock_gettime(libc::CLOCK_MONOTONIC, &mut ts) };
    ts.tv_sec as f64 + ts.tv_nsec as f64 * 1e-9
}

impl Watchdog {
    /// `WatchdogImplBase.__init__`: the timeout hw-management reports.
    fn new(kind: Type, wd_device_path: String) -> Watchdog {
        Watchdog {
            kind,
            watchdog_path: wd_device_path,
            handle: Mutex::new(None),
            timeout: Mutex::new(utils::read_int(TIMEOUT_FILE, 0, Log::Error)),
        }
    }

    /// `ioctl(self.watchdog, request, arg)`, opening the device first if no
    /// ioctl has yet; a failed open is tried again by the next one.
    fn ioctl(&self, request: u32, arg: Option<&mut i32>) -> PyResult<()> {
        let mut h = utils::lock(&self.handle);
        if h.is_none() {
            let f = OpenOptions::new()
                .write(true)
                .open(io::real(&self.watchdog_path))
                .map_err(|e| PyErr::from_io(&e, Some(&self.watchdog_path)))?;
            *h = Some(f.into());
        }
        #[cfg(test)]
        if let Some(d) = utils::lock(&tests::FAKE_DRIVER).as_mut() {
            return d.ioctl(request, arg);
        }
        let fd = h.as_ref().map_or(-1, |f| f.as_raw_fd());
        let ptr = arg.map_or(std::ptr::null_mut(), |a| a as *mut i32);
        // SAFETY: the watchdog requests read or write one int, which `ptr`
        // points at when they take an argument.
        let rc = unsafe { libc::ioctl(fd, request as _, ptr) };
        if rc < 0 {
            return Err(PyErr::from_io(&std::io::Error::last_os_error(), None));
        }
        Ok(())
    }

    fn setoptions(&self, option: i32) -> PyResult<()> {
        let mut req = option;
        self.ioctl(WDIOC_SETOPTIONS, Some(&mut req))
    }

    /// `_settimeout(seconds)`: the timeout the driver settled on.
    fn settimeout(&self, seconds: i128) -> PyResult<i128> {
        let mut req = u32::try_from(seconds)
            .map_err(|_| PyErr::other("OverflowError", "unsigned int is greater than maximum"))?
            as i32;
        self.ioctl(WDIOC_SETTIMEOUT, Some(&mut req))?;
        Ok(i128::from(req as u32))
    }

    /// `WatchdogImplBase.arm`: -1 for seconds out of range or an ioctl that
    /// fails; the timeout armed with otherwise.
    fn arm_impl(&self, seconds: i128) -> PyResult<i128> {
        // `seconds < 0 or seconds > max_period`: the maximum is asked for
        // only when the first test fails.
        if seconds < 0 {
            return Ok(WD_COMMON_ERROR);
        }
        let max = device_data::get_watchdog_max_period()?
            .ok_or_else(|| PyErr::type_error("'>' not supported between instances of 'int' and 'NoneType'"))?;
        if seconds > max {
            return Ok(WD_COMMON_ERROR);
        }
        let r = (|| -> PyResult<i128> {
            let timeout = *utils::lock(&self.timeout);
            if timeout != seconds {
                let t = self.settimeout(seconds)?;
                *utils::lock(&self.timeout) = t;
            }
            if self.armed() {
                self.ioctl(WDIOC_KEEPALIVE, None)?;
            } else {
                self.setoptions(WDIOS_ENABLECARD)?;
            }
            Ok(*utils::lock(&self.timeout))
        })();
        match r {
            Ok(t) => Ok(t),
            Err(e) if e.exc.is_os_error() => Ok(WD_COMMON_ERROR),
            Err(e) => Err(e),
        }
    }

    fn armed(&self) -> bool {
        utils::read_str(STATE_FILE, "", Log::Error) == "active"
    }
}

impl PyObject for Watchdog {
    fn py_class(&self) -> &'static str {
        match self.kind {
            Type::Type1 => "WatchdogType1",
            Type::Type2 => "WatchdogType2",
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::WatchdogBase
    }
    /// Type 1 also notes when it was armed -- whether arming worked or not.
    fn arm(&self, seconds: i128) -> PyResult<PyVal> {
        let ret = self.arm_impl(seconds)?;
        if self.kind == Type::Type1 {
            std::fs::create_dir_all(io::real(TIMESTAMP_DIR)).map_err(|e| PyErr::from_io(&e, Some(TIMESTAMP_DIR)))?;
            utils::write_file(TIMESTAMP_FILE, &float_repr(monotonic()), false, Log::Error)?;
        }
        Ok(PyVal::Int(ret))
    }
    fn disarm(&self) -> PyResult<PyVal> {
        let mut disarmed = false;
        if self.armed() {
            match self.setoptions(WDIOS_DISABLECARD) {
                Ok(()) => disarmed = true,
                Err(e) if e.exc.is_os_error() => {}
                Err(e) => return Err(e),
            }
        }
        Ok(PyVal::Bool(disarmed))
    }
    fn is_armed(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.armed()))
    }
    fn get_remaining_time(&self) -> PyResult<PyVal> {
        if !self.armed() {
            return Ok(PyVal::Int(WD_COMMON_ERROR));
        }
        Ok(PyVal::Int(match self.kind {
            Type::Type2 => utils::read_int(TIMELEFT_FILE, 0, Log::Error),
            Type::Type1 => {
                let arm_timestamp = utils::read_float(TIMESTAMP_FILE, 0.0, Log::Error);
                let timeout = *utils::lock(&self.timeout);
                platform_base::py::int_of_float(timeout as f64 - (monotonic() - arm_timestamp))?
            }
        }))
    }
}

/// `is_mlnx_wd_main(dev)`: an unreadable identity is not it; one that does
/// not decode raises.
fn is_mlnx_wd_main(dev: &str) -> PyResult<bool> {
    match io::read_text(&format!("{WD_SYSFS_PATH}/{dev}/identity")) {
        Ok(t) => Ok(platform_base::py::strip(&t) == WD_MLNX_MAIN_IDENTITY),
        Err(e) if e.exc.is_os_error() => Ok(false),
        Err(e) => Err(e),
    }
}

/// `get_watchdog()`: the main watchdog, once hw-management has had up to 10
/// seconds to show its state; None when no device is it.
pub fn get_watchdog() -> PyResult<Option<Watchdog>> {
    crate::imports::watchdog();
    utils::wait_until(|| io::exists(STATE_FILE), 10.0, 1.0);
    let mut main = None;
    for device in io::listdir("/dev/")? {
        if device.starts_with("watchdog") && is_mlnx_wd_main(&device)? {
            main = Some(device);
            break;
        }
    }
    let Some(name) = main else {
        return Ok(None);
    };
    let kind = if io::exists(&format!("{WD_SYSFS_PATH}/{name}/timeleft")) { Type::Type2 } else { Type::Type1 };
    Ok(Some(Watchdog::new(kind, format!("/dev/{name}"))))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    /// The driver, for a test: each ioctl logged, then failed with `errno`
    /// if one is set; SETTIMEOUT answered with `timeout` if one is set (the
    /// driver's own rounding), else with the timeout asked for.
    #[derive(Default)]
    pub(super) struct FakeDriver {
        log: Vec<String>,
        errno: Option<i32>,
        timeout: Option<i32>,
    }

    pub(super) static FAKE_DRIVER: Mutex<Option<FakeDriver>> = Mutex::new(None);

    impl FakeDriver {
        pub(super) fn ioctl(&mut self, request: u32, arg: Option<&mut i32>) -> PyResult<()> {
            let name = match request {
                WDIOC_SETOPTIONS => "SETOPTIONS",
                WDIOC_KEEPALIVE => "KEEPALIVE",
                WDIOC_SETTIMEOUT => "SETTIMEOUT",
                _ => "?",
            };
            self.log.push(match &arg {
                Some(a) => format!("{name} {a}"),
                None => name.to_string(),
            });
            if let Some(errno) = self.errno {
                return Err(PyErr::from_io(&std::io::Error::from_raw_os_error(errno), None));
            }
            if let (WDIOC_SETTIMEOUT, Some(t), Some(a)) = (request, self.timeout, arg) {
                *a = t;
            }
            Ok(())
        }
    }

    /// The driver in place, with nothing logged yet.
    fn driver(errno: Option<i32>, timeout: Option<i32>) {
        *FAKE_DRIVER.lock().unwrap() = Some(FakeDriver { log: Vec::new(), errno, timeout });
    }

    fn log() -> Vec<String> {
        FAKE_DRIVER.lock().unwrap().as_mut().map(|d| std::mem::take(&mut d.log)).unwrap_or_default()
    }

    fn no_driver() {
        *FAKE_DRIVER.lock().unwrap() = None;
    }

    /// hw-management's state and timeout, and the device nodes.
    fn bench(state: &str, timeout: &str) -> Root {
        let root = Root::new();
        root.write(STATE_FILE, state);
        root.write(TIMEOUT_FILE, timeout);
        root.write("/dev/watchdog0", "");
        root.write("/dev/watchdog1", "");
        root
    }

    #[test]
    fn the_main_watchdog_is_found_by_its_identity() {
        let root = bench("inactive", "30");
        // No device names itself mlx-wdt-main.
        root.write("/sys/class/watchdog/watchdog0/identity", "other\n");
        assert!(get_watchdog().unwrap().is_none());
        root.write("/sys/class/watchdog/watchdog1/identity", "mlx-wdt-main\n");
        let wd = get_watchdog().unwrap().unwrap();
        assert_eq!((wd.py_class(), wd.watchdog_path.as_str(), wd.kind), ("WatchdogType1", "/dev/watchdog1", Type::Type1));
        assert_eq!(wd.base_class(), BaseClass::WatchdogBase);
        root.write("/sys/class/watchdog/watchdog1/timeleft", "7\n");
        let wd = get_watchdog().unwrap().unwrap();
        assert_eq!(wd.py_class(), "WatchdogType2");
        // An identity that does not decode is the decoder's error, whatever
        // order the devices are listed in.
        root.write("/sys/class/watchdog/watchdog1/identity", "other\n");
        std::fs::write(root.real("/sys/class/watchdog/watchdog0/identity"), b"\xff\n").unwrap();
        assert_eq!(get_watchdog().err().map(|e| e.exc.name()), Some("UnicodeDecodeError"));
    }

    #[test]
    fn arming_sets_the_timeout_only_when_it_changes() {
        let root = bench("inactive", "30");
        let wd = Watchdog::new(Type::Type2, "/dev/watchdog1".into());
        driver(None, None);
        // The timeout hw-management reports: no SETTIMEOUT, and the card on.
        assert_eq!(wd.arm(30).unwrap(), PyVal::Int(30));
        assert_eq!(log(), ["SETOPTIONS 2"]);
        // Armed already: a new timeout, as the driver settles it, then a
        // keepalive.
        root.write(STATE_FILE, "active");
        driver(None, Some(32));
        assert_eq!(wd.arm(31).unwrap(), PyVal::Int(32));
        assert_eq!(log(), ["SETTIMEOUT 31", "KEEPALIVE"]);
        assert_eq!(wd.arm(32).unwrap(), PyVal::Int(32));
        assert_eq!(log(), ["KEEPALIVE"]);
        // Out of range: nothing asked of the driver.
        assert_eq!(wd.arm(-1).unwrap(), PyVal::Int(-1));
        assert_eq!(wd.arm(65536).unwrap(), PyVal::Int(-1));
        assert!(log().is_empty());
        // The driver failing is -1.
        driver(Some(libc::EIO), None);
        assert_eq!(wd.arm(10).unwrap(), PyVal::Int(-1));
        assert_eq!(log(), ["SETTIMEOUT 10"]);
        no_driver();
    }

    #[test]
    fn a_platform_s_max_period_bounds_the_timeout() {
        let root = bench("inactive", "0");
        root.write(platform_base::device_info::MACHINE_CONF_PATH, "onie_platform=x86_64-mlnx_msn2100-r0\n");
        let wd = Watchdog::new(Type::Type2, "/dev/watchdog1".into());
        driver(None, None);
        if !crate::testing::platform_from_env() {
            assert_eq!(wd.arm(33).unwrap(), PyVal::Int(-1));
            assert!(log().is_empty());
        }
        assert_eq!(wd.arm(32).unwrap(), PyVal::Int(32));
        assert_eq!(log(), ["SETTIMEOUT 32", "SETOPTIONS 2"]);
        no_driver();
    }

    #[test]
    fn a_device_that_cannot_be_opened_or_driven_is_minus_one() {
        let root = bench("active", "30");
        no_driver();
        // No device node: the open fails, and is tried again next time.
        let wd = Watchdog::new(Type::Type2, "/dev/watchdog9".into());
        assert_eq!(wd.arm(10).unwrap(), PyVal::Int(-1));
        assert_eq!(wd.disarm().unwrap(), PyVal::Bool(false));
        // A plain file for a device: every ioctl is ENOTTY.
        let wd = Watchdog::new(Type::Type2, "/dev/watchdog1".into());
        assert_eq!(wd.arm(10).unwrap(), PyVal::Int(-1));
        assert_eq!(wd.disarm().unwrap(), PyVal::Bool(false));
        drop(root);
    }

    #[test]
    fn disarming_turns_an_armed_card_off() {
        let root = bench("inactive", "30");
        let wd = Watchdog::new(Type::Type2, "/dev/watchdog1".into());
        driver(None, None);
        assert_eq!(wd.disarm().unwrap(), PyVal::Bool(false));
        assert!(log().is_empty());
        root.write(STATE_FILE, "active");
        assert_eq!(wd.disarm().unwrap(), PyVal::Bool(true));
        assert_eq!(log(), ["SETOPTIONS 1"]);
        driver(Some(libc::EIO), None);
        assert_eq!(wd.disarm().unwrap(), PyVal::Bool(false));
        no_driver();
    }

    #[test]
    fn the_time_left_is_hw_management_s_or_counted_from_the_arming() {
        let root = bench("inactive", "30");
        root.write(TIMELEFT_FILE, "17");
        let t2 = Watchdog::new(Type::Type2, "/dev/watchdog1".into());
        let t1 = Watchdog::new(Type::Type1, "/dev/watchdog1".into());
        assert_eq!(t2.get_remaining_time().unwrap(), PyVal::Int(-1));
        assert_eq!(t1.get_remaining_time().unwrap(), PyVal::Int(-1));
        assert_eq!(t2.is_armed().unwrap(), PyVal::Bool(false));
        root.write(STATE_FILE, "active");
        assert_eq!(t2.is_armed().unwrap(), PyVal::Bool(true));
        assert_eq!(t2.get_remaining_time().unwrap(), PyVal::Int(17));
        // Type 1 notes when it was armed -- even when arming failed -- and
        // counts down from there.
        driver(Some(libc::EIO), None);
        assert_eq!(t1.arm(30).unwrap(), PyVal::Int(-1));
        let stamp = root.read(TIMESTAMP_FILE).unwrap();
        assert!(stamp.parse::<f64>().is_ok(), "{stamp}");
        let left = t1.get_remaining_time().unwrap();
        assert!(matches!(left, PyVal::Int(29..=30)), "{}", left.py_repr());
        // No timestamp yet reads as 0.0: the whole uptime has gone by.
        root.write(TIMESTAMP_FILE, "garbage");
        assert!(matches!(t1.get_remaining_time().unwrap(), PyVal::Int(n) if n < 30));
        no_driver();
    }
}
