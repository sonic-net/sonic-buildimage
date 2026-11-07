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


//! `sonic_platform/utils.py`.
//!
//! Every sysfs read the platform makes goes through `read_from_file`, and what
//! it does on failure is what the platform does on failure: log
//! `Failed to read from file <path> - <repr(e)>` through whichever log
//! function the caller passed -- `log_error` unless it said otherwise, so on
//! every call -- and answer the caller's default.  The text, the level and the
//! default are all observable, so they are reproduced here exactly.

use std::fmt::Display;
use std::process::{Command, Stdio};
use std::io::Write;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard, OnceLock, PoisonError};
use std::thread::{JoinHandle, ThreadId};
use std::time::{Duration, Instant};

use platform_base::logger::{self, LOG_DEBUG, LOG_ERR, LOG_INFO, LOG_NOTICE, LOG_WARNING};
use platform_base::py::{py_float, py_int, strip, Exc, PyErr, PyResult, PyVal};
use platform_base::{device_info, io, json};

/// A `log_func` argument: which `Logger` method, or `None`.
///
/// Every `Logger` in the package keeps the default NOTICE threshold, so a
/// method is as good as its level: `log_info` and `log_debug` say nothing.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Log {
    Error,
    Warning,
    Notice,
    Info,
    Debug,
    Off,
}

impl Log {
    pub fn emit(self, msg: &str) {
        let level = match self {
            Log::Error => LOG_ERR,
            Log::Warning => LOG_WARNING,
            Log::Notice => LOG_NOTICE,
            Log::Info => LOG_INFO,
            Log::Debug => LOG_DEBUG,
            Log::Off => return,
        };
        if level <= LOG_NOTICE {
            #[cfg(test)]
            tests::LOGGED.with(|l| l.borrow_mut().push(msg.to_string()));
            logger::syslog(level, msg);
        }
    }
}

/// `logger.log_error(msg)` and friends, on any of the package's loggers.
pub fn log_error(msg: &str) {
    Log::Error.emit(msg)
}
pub fn log_warning(msg: &str) {
    Log::Warning.emit(msg)
}
pub fn log_notice(msg: &str) {
    Log::Notice.emit(msg)
}
pub fn log_info(msg: &str) {
    Log::Info.emit(msg)
}
pub fn log_debug(msg: &str) {
    Log::Debug.emit(msg)
}

/// `read_from_file(path, target_type, default, raise_exception, log_func)`.
///
/// `convert` is the target type applied to the stripped text.  Only
/// `ValueError` and `IOError` (and their subclasses: `UnicodeDecodeError`,
/// `FileNotFoundError`) are caught, as in Python.
pub fn read_from_file<T>(
    path: &str,
    convert: impl FnOnce(&str) -> PyResult<T>,
    default: T,
    raise: bool,
    log: Log,
) -> PyResult<T> {
    let r = io::read_text(path).and_then(|text| convert(strip(&text)));
    match r {
        Ok(v) => Ok(v),
        Err(e) if e.exc.is_os_error() || caught_value_error(&e) => {
            log.emit(&format!("Failed to read from file {path} - {}", e.repr()));
            if raise {
                Err(e)
            } else {
                Ok(default)
            }
        }
        Err(e) => Err(e),
    }
}

/// `ValueError` and the subclasses of it a read or a write can raise.
pub(crate) fn caught_value_error(e: &PyErr) -> bool {
    matches!(e.exc, Exc::ValueError | Exc::Other("UnicodeDecodeError" | "UnicodeEncodeError"))
}

/// `read_str_from_file(path, default='', ...)`.
pub fn read_str(path: &str, default: &str, log: Log) -> String {
    read_from_file(path, |s| Ok(s.to_string()), default.to_string(), false, log)
        .unwrap_or_default()
}

/// `read_str_from_file(path, raise_exception=True)`.
pub fn read_str_raise(path: &str, log: Log) -> PyResult<String> {
    read_from_file(path, |s| Ok(s.to_string()), String::new(), true, log)
}

/// `read_int_from_file(path, default, ...)`.
pub fn read_int(path: &str, default: i128, log: Log) -> i128 {
    read_from_file(path, py_int, default, false, log).unwrap_or(default)
}

/// `read_int_from_file(path, raise_exception=True)`.
pub fn read_int_raise(path: &str, log: Log) -> PyResult<i128> {
    read_from_file(path, py_int, 0, true, log)
}

/// `read_float_from_file(path, default, ...)`.
pub fn read_float(path: &str, default: f64, log: Log) -> f64 {
    read_from_file(path, py_float, default, false, log).unwrap_or(default)
}

/// `read_float_from_file(path, None, ...)`.
pub fn read_float_opt(path: &str, log: Log) -> Option<f64> {
    read_from_file(path, |s| py_float(s).map(Some), None, false, log).unwrap_or(None)
}

/// `_key_value_converter(content, delimeter)`: `k, v = line.split(d, 1)`,
/// which raises `ValueError` on a line without the delimiter.
pub fn key_value(content: &str, delim: &str) -> PyResult<Vec<(String, String)>> {
    let mut out: Vec<(String, String)> = Vec::new();
    for line in py_splitlines(content) {
        let Some((k, v)) = line.split_once(delim) else {
            return Err(PyErr::value_error("not enough values to unpack (expected 2, got 1)"));
        };
        let (k, v) = (strip(k).to_string(), strip(v).to_string());
        match out.iter_mut().find(|(k2, _)| *k2 == k) {
            Some(e) => e.1 = v,
            None => out.push((k, v)),
        }
    }
    Ok(out)
}

/// `read_key_value_file(path, default={}, ..., delimeter=':')`.
pub fn read_key_value(path: &str, log: Log, delim: &str) -> Vec<(String, String)> {
    read_from_file(path, |s| key_value(s, delim), Vec::new(), false, log).unwrap_or_default()
}

/// `str.splitlines()`: without the line breaks, and none for a trailing one.
pub fn py_splitlines(s: &str) -> Vec<&str> {
    platform_base::py::splitlines(s)
}

/// `write_file(path, content, raise_exception, log_func)`: `str(content)`.
pub fn write_file(path: &str, content: &dyn Display, raise: bool, log: Log) -> PyResult<bool> {
    let text = content.to_string();
    match io::write_text(path, &text) {
        Ok(()) => Ok(true),
        Err(e) if e.exc.is_os_error() || caught_value_error(&e) => {
            log.emit(&format!("Failed to write {text} to file {path} - {}", e.repr()));
            if raise {
                Err(e)
            } else {
                Ok(false)
            }
        }
        Err(e) => Err(e),
    }
}

/// What `if not value:` in `read_only_cache` sees.
pub trait Truthy {
    fn truthy(&self) -> bool;
}
impl Truthy for bool {
    fn truthy(&self) -> bool {
        *self
    }
}
impl Truthy for i128 {
    fn truthy(&self) -> bool {
        *self != 0
    }
}
impl Truthy for usize {
    fn truthy(&self) -> bool {
        *self != 0
    }
}
impl Truthy for String {
    fn truthy(&self) -> bool {
        !self.is_empty()
    }
}
impl Truthy for PyVal {
    fn truthy(&self) -> bool {
        PyVal::truthy(self)
    }
}
impl<T: Truthy> Truthy for Option<T> {
    fn truthy(&self) -> bool {
        self.as_ref().is_some_and(Truthy::truthy)
    }
}
impl<T> Truthy for Vec<T> {
    fn truthy(&self) -> bool {
        !self.is_empty()
    }
}

/// The caches' generation: a value cached in an older one is forgotten.
/// Only a test moves it, when it puts another platform under the code; on a
/// switch it stays 0 and a cached value lives as long as the process.
static CACHE_GENERATION: AtomicU64 = AtomicU64::new(0);

/// Forget every `ReadOnlyCache`, for a test starting on a new platform.
#[cfg(test)]
pub fn forget_caches() {
    CACHE_GENERATION.fetch_add(1, Ordering::Relaxed);
}

/// `@read_only_cache()`: one value per function, kept only once it is truthy
/// -- a 0, a False, an empty list is computed again on every call.
pub struct ReadOnlyCache<T>(Mutex<Option<(u64, T)>>);

impl<T: Clone + Truthy> ReadOnlyCache<T> {
    pub const fn new() -> Self {
        ReadOnlyCache(Mutex::new(None))
    }

    pub fn get(&self, compute: impl FnOnce() -> T) -> T {
        let generation = CACHE_GENERATION.load(Ordering::Relaxed);
        if let Some(v) = self.kept(generation) {
            return v;
        }
        // Computed outside the lock, as Python computes it outside any lock:
        // two threads may both compute, and the later store wins.
        let v = compute();
        self.keep(generation, &v);
        v
    }

    /// [`get`](Self::get) for a function that can raise: the exception gets
    /// out and nothing is stored, as the decorator's assignment never runs.
    pub fn try_get(&self, compute: impl FnOnce() -> PyResult<T>) -> PyResult<T> {
        let generation = CACHE_GENERATION.load(Ordering::Relaxed);
        if let Some(v) = self.kept(generation) {
            return Ok(v);
        }
        let v = compute()?;
        self.keep(generation, &v);
        Ok(v)
    }

    fn kept(&self, generation: u64) -> Option<T> {
        let (g, v) = self.0.lock().unwrap_or_else(PoisonError::into_inner).clone()?;
        (g == generation && v.truthy()).then_some(v)
    }

    fn keep(&self, generation: u64, v: &T) {
        *self.0.lock().unwrap_or_else(PoisonError::into_inner) = Some((generation, v.clone()));
    }
}

impl<T: Clone + Truthy> Default for ReadOnlyCache<T> {
    fn default() -> Self {
        Self::new()
    }
}

/// `is_host()`.
pub fn is_host() -> bool {
    static C: ReadOnlyCache<bool> = ReadOnlyCache::new();
    C.get(|| !io::exists("/.dockerenv"))
}

/// `@default_return(value)`: any exception logged at debug, and `value`.
pub fn default_return<T>(value: T, r: PyResult<T>, method: &str) -> T {
    match r {
        Ok(v) => v,
        Err(e) => {
            log_debug(&format!("Faield to execute method {method} - {}", e.repr()));
            value
        }
    }
}

/// `run_command(command)`: stdout stripped, `None` if it could not run or
/// its output does not decode.
pub fn run_command(command: &[&str]) -> Option<String> {
    let (prog, args) = command.split_first()?;
    for a in command {
        platform_base::py::codec::encode_check(platform_base::py::codec::locale(), a).ok()?;
    }
    let out = Command::new(prog)
        .args(args)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
        .ok()?;
    platform_base::subprocess::text(&out.stdout).ok().map(|t| strip(&t).to_string())
}

/// `load_json_file(filename, log_func)`.
pub fn load_json_file(filename: &str, log: Log) -> Option<PyVal> {
    match io::read_text(filename) {
        Ok(text) => match json::loads(&text) {
            Ok(v) => Some(v),
            Err(_) => {
                log.emit("failed to decode Json file.");
                None
            }
        },
        Err(e) => {
            log.emit(&format!("error occurred while parsing json file: {}", e.str()));
            None
        }
    }
}

/// `get_path_to_hwsku_directory(asic_id)`.
pub fn get_path_to_hwsku_directory(asic_id: Option<i128>) -> PyResult<String> {
    let platform_path = device_info::get_path_to_platform_dir()?;
    let hwsku = device_info::get_hwsku().ok_or_else(device_info::join_none_error)?;
    let base = io::join(&platform_path, &hwsku);
    Ok(match asic_id {
        Some(id) => io::join(&base, &id.to_string()),
        None => base,
    })
}

/// `get_path_list_to_asic_hwsku_dir(num_of_asics)` (the second definition,
/// which is the one Python ends up with).
pub fn get_path_list_to_asic_hwsku_dir(num_of_asics: i128) -> PyResult<Vec<String>> {
    if num_of_asics == 1 {
        return Ok(vec![io::join(&get_path_to_hwsku_directory(None)?, "hwsku.json")]);
    }
    (0..num_of_asics.max(0))
        .map(|a| Ok(io::join(&get_path_to_hwsku_directory(Some(a))?, "hwsku.json")))
        .collect()
}

/// `_shutdown_event`: a `threading.Event`, set once the daemon is shutting
/// down (see `notify_shutdown`) and never cleared.
static SHUTDOWN_EVENT: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());

/// `_shutdown_event.wait(interval)`: true at once if the event is set,
/// otherwise once `interval` seconds have passed or it is set, whichever is
/// first.  Every caller passes a constant; the interval is clamped only so
/// that no value can make the conversion panic.
fn wait_interval(interval: f64) -> bool {
    let (set, cv) = &SHUTDOWN_EVENT;
    let timeout = Duration::from_secs_f64(interval.clamp(0.0, PY_WAIT_MAX_S));
    let (set, _) = cv.wait_timeout_while(lock(set), timeout, |set| !*set).unwrap_or_else(PoisonError::into_inner);
    *set
}

/// `get_shutdown_event().is_set()`.
pub fn shutdown_requested() -> bool {
    *lock(&SHUTDOWN_EVENT.0)
}

/// `_handle_shutdown_signal(...)`'s `_shutdown_event.set()`, which is what
/// the platform API's `notify_shutdown()` runs here (see `open`): every
/// readiness wait ends at once with false.  Any thread.
///
/// Only once `watch_shutdown_signals()` has armed the watch, as in Python,
/// where the event is set by nothing but the handler that installs.
pub fn notify_shutdown() {
    if !*lock(&SHUTDOWN_WATCH_INSTALLED) {
        return;
    }
    let (set, cv) = &SHUTDOWN_EVENT;
    *lock(set) = true;
    cv.notify_all();
}

/// `wait_until(predict, timeout, interval)`.
///
/// Ends with false as soon as the daemon is shutting down (see
/// `notify_shutdown`).
pub fn wait_until(mut predict: impl FnMut() -> bool, timeout: f64, interval: f64) -> bool {
    if predict() {
        return true;
    }
    let mut timeout = timeout;
    while timeout > 0.0 {
        if wait_interval(interval) {
            return false;
        }
        timeout -= interval;
        if predict() {
            return true;
        }
    }
    false
}

/// `wait_until_conditions(conditions, timeout, interval)`, which ends the
/// same way on shutdown.
pub fn wait_until_conditions(
    mut conditions: Vec<Box<dyn FnMut() -> bool>>,
    timeout: f64,
    interval: f64,
) -> bool {
    let mut timeout = timeout;
    while timeout > 0.0 {
        conditions.retain_mut(|c| !c());
        if conditions.is_empty() {
            return true;
        }
        if wait_interval(interval) {
            return false;
        }
        timeout -= interval;
    }
    false
}

/// `os.access(path, os.R_OK)`.
pub fn readable(path: &str) -> bool {
    std::fs::File::open(io::real(path)).is_ok()
}

/// A mutex's guard, poisoned or not.  A panic while one of these locks is
/// held leaves what it guards whole -- a cache, a list being filled, a
/// flag -- and Python, whose locks have no such state, carries on; so do we.
pub(crate) fn lock<T: ?Sized>(m: &Mutex<T>) -> MutexGuard<'_, T> {
    m.lock().unwrap_or_else(PoisonError::into_inner)
}

/// `os.path.dirname(path)`.
pub(crate) fn py_dirname(path: &str) -> &str {
    let head = &path[..path.rfind('/').map_or(0, |i| i + 1)];
    if !head.is_empty() && head.bytes().any(|b| b != b'/') {
        head.trim_end_matches('/')
    } else {
        head
    }
}

/// `os.path.basename(path)`.
pub(crate) fn py_basename(path: &str) -> &str {
    &path[path.rfind('/').map_or(0, |i| i + 1)..]
}

/// `wait_for_file_creation(path, timeout)`: inotify on the directory, as
/// Python does it -- so `timeout` counts from the last event in the
/// directory, not from the start, and a directory hw-management is still
/// filling keeps the wait going.  Any failure of the watch is logged and
/// the file checked once more.
pub fn wait_for_file_creation(path: &str, timeout: f64) -> bool {
    if readable(path) {
        return true;
    }
    let dir = py_dirname(path);
    let name = py_basename(path);
    if !io::exists(dir) {
        log_debug(&format!("Directory {dir} does not exist"));
        return false;
    }
    let watched = (|| -> PyResult<Option<()>> {
        let mut notifier = crate::inotify_helper::Inotify::new()?;
        notifier.add_watch(dir, libc::IN_CREATE | libc::IN_CLOSE_WRITE | libc::IN_MOVED_TO)?;
        notifier.event_gen(timeout, |e| {
            let kind = e.type_names.iter().any(|t| matches!(*t, "IN_CREATE" | "IN_CLOSE_WRITE" | "IN_MOVED_TO"));
            if e.filename == name && kind && readable(path) {
                log_info(&format!("File {path} created and readable"));
                return Ok(Some(()));
            }
            Ok(None)
        })
    })();
    match watched {
        Ok(Some(())) => return true,
        Ok(None) => {}
        Err(e) => log_error(&format!("Inotify error while waiting for {path}: {}", e.repr())),
    }
    readable(path)
}

pub const SYSFS_LABELS_READY_FILE: &str = "/var/run/hw-management/sysfs_labels_rdy";
pub const SYSFS_LABELS_READY_WAIT_TIMEOUT: f64 = 60.0;

/// `ensure_sysfs_labels_ready()`.
pub fn ensure_sysfs_labels_ready() -> bool {
    let path = SYSFS_LABELS_READY_FILE;
    if readable(path) {
        return true;
    }
    let dir = py_dirname(path).to_string();
    log_info(&format!("Sysfs labels ready file {path} not accessible, waiting for creation"));
    let deadline = Instant::now() + Duration::from_secs_f64(SYSFS_LABELS_READY_WAIT_TIMEOUT);
    let remaining = || deadline.saturating_duration_since(Instant::now()).as_secs_f64();
    if !wait_until(|| io::exists(&dir), remaining(), 1.0) {
        log_error(&format!("Parent directory {dir} not available after timeout"));
        return false;
    }
    if wait_for_file_creation(path, remaining()) {
        return true;
    }
    log_error(&format!("Sysfs labels ready file {path} not available after timeout"));
    false
}

/// `glob.glob(pattern)`, for patterns whose wildcards are all in the last
/// component -- which every pattern in the package is.  Names come back in
/// directory order, as Python's do; hidden files only match a pattern that
/// starts with a dot.
pub fn glob(pattern: &str) -> Vec<String> {
    let (dir, pat) = match pattern.rsplit_once('/') {
        Some((d, p)) => (if d.is_empty() { "/" } else { d }, p),
        None => (".", pattern),
    };
    let Ok(rd) = std::fs::read_dir(io::real(dir)) else {
        return Vec::new();
    };
    let mut out = Vec::new();
    for e in rd.flatten() {
        let name = e.file_name().to_string_lossy().into_owned();
        if name.starts_with('.') && !pat.starts_with('.') {
            continue;
        }
        if fnmatch(&name, pat) {
            out.push(if pattern.contains('/') {
                format!("{}/{name}", pattern.rsplit_once('/').map_or("", |(d, _)| d))
            } else {
                name
            });
        }
    }
    out
}

/// `fnmatch.fnmatchcase(name, pat)`: `*`, `?`, `[seq]` and `[!seq]`.
pub fn fnmatch(name: &str, pat: &str) -> bool {
    let n: Vec<char> = name.chars().collect();
    let p: Vec<char> = pat.chars().collect();
    fn go(n: &[char], p: &[char]) -> bool {
        match p.first() {
            None => n.is_empty(),
            Some('*') => (0..=n.len()).any(|k| go(&n[k..], &p[1..])),
            Some('?') => !n.is_empty() && go(&n[1..], &p[1..]),
            Some('[') => {
                let Some(end) = p.iter().skip(2).position(|c| *c == ']').map(|i| i + 2) else {
                    return n.first() == Some(&'[') && go(&n[1..], &p[1..]);
                };
                let Some(c) = n.first() else { return false };
                let (neg, set) = if p[1] == '!' { (true, &p[2..end]) } else { (false, &p[1..end]) };
                let mut hit = false;
                let mut i = 0;
                while i < set.len() {
                    if i + 2 < set.len() && set[i + 1] == '-' {
                        hit |= set[i] <= *c && *c <= set[i + 2];
                        i += 3;
                    } else {
                        hit |= set[i] == *c;
                        i += 1;
                    }
                }
                hit != neg && go(&n[1..], &p[end + 1..])
            }
            Some(ch) => n.first() == Some(ch) && go(&n[1..], &p[1..]),
        }
    }
    go(&n, &p)
}

/// `str(list_of_strings)`, as a logged command line reads.
pub fn py_list_repr(items: &[&str]) -> String {
    format!(
        "[{}]",
        items.iter().map(|s| platform_base::py::str_repr(s)).collect::<Vec<_>>().join(", ")
    )
}

/// `Popen.returncode`: the exit status, or minus the signal that killed it.
pub fn py_returncode(s: &std::process::ExitStatus) -> i32 {
    use std::os::unix::process::ExitStatusExt;
    s.code().or_else(|| s.signal().map(|sig| -sig)).unwrap_or(-1)
}

/// Python's `a // b` on integers: toward negative infinity.
pub fn floor_div(a: i128, b: i128) -> PyResult<i128> {
    if b == 0 {
        return Err(PyErr::new(
            Exc::ZeroDivisionError,
            vec![PyVal::str("integer division or modulo by zero")],
        ));
    }
    let q = a / b;
    Ok(if (a % b != 0) && ((a < 0) != (b < 0)) { q - 1 } else { q })
}

/// Python's `round(x)` on a float: to the nearest integer, halves to even.
pub fn round_half_even(x: f64) -> i128 {
    let r = x.round();
    let r = if (x - x.trunc()).abs() == 0.5 && (r as i128) % 2 != 0 {
        r - x.signum()
    } else {
        r
    };
    r as i128
}

/// `wait_until_conditions` over conditions that can raise: the first one
/// that does ends the wait with its exception, as in Python.
pub fn try_wait_until_conditions(
    mut conditions: Vec<Box<dyn FnMut() -> PyResult<bool>>>,
    timeout: f64,
    interval: f64,
) -> PyResult<bool> {
    let mut timeout = timeout;
    while timeout > 0.0 {
        let mut pending = Vec::new();
        for mut c in conditions.drain(..) {
            if !c()? {
                pending.push(c);
            }
        }
        if pending.is_empty() {
            return Ok(true);
        }
        conditions = pending;
        if wait_interval(interval) {
            return Ok(false);
        }
        timeout -= interval;
    }
    Ok(false)
}

/// `_extract_ports_index_by_type(port_type, num_of_asics)`: 0-based indexes
/// of the ports hwsku.json gives that `port_type`, or None.
fn extract_ports_index_by_type(port_type: &str, num_of_asics: i128) -> PyResult<Option<Vec<i128>>> {
    let platform_file = io::join(&device_info::get_path_to_platform_dir()?, "platform.json");
    if !io::exists(&platform_file) {
        return Ok(None);
    }
    let platform = load_json_file(&platform_file, Log::Error).unwrap_or(PyVal::None);
    let interfaces = json::index(&platform, "interfaces")?;
    let mut name_to_index: Vec<(String, String)> = Vec::new();
    for (key, value) in dict_items(interfaces, "items")? {
        if let Some(index_raw) = json::get(&value, "index") {
            let index = split_first(index_raw, ",")?;
            match name_to_index.iter_mut().find(|(k, _)| *k == key) {
                Some(e) => e.1 = index,
                None => name_to_index.push((key, index)),
            }
        }
    }
    if name_to_index.is_empty() {
        return Ok(None);
    }
    let mut hwsku: Vec<(String, PyVal)> = Vec::new();
    for hwsku_json in get_path_list_to_asic_hwsku_dir(num_of_asics)? {
        if !io::exists(&hwsku_json) {
            continue;
        }
        let data = load_json_file(&hwsku_json, Log::Error).unwrap_or(PyVal::None);
        for (k, v) in dict_items(json::index(&data, "interfaces")?, "items")? {
            match hwsku.iter_mut().find(|(hk, _)| *hk == k) {
                Some(e) => e.1 = v,
                None => hwsku.push((k, v)),
            }
        }
    }
    if hwsku.is_empty() {
        return Ok(None);
    }
    let mut out: Vec<i128> = Vec::new();
    for (key, value) in &hwsku {
        let Some((_, index)) = name_to_index.iter().find(|(k, _)| k == key) else {
            continue;
        };
        if json::get(value, "port_type").is_some_and(|t| matches!(t, PyVal::Str(s) if s == port_type)) {
            let i = py_int(index)? - 1;
            // `list(dict.fromkeys(...))`: first occurrence kept.
            if !out.contains(&i) {
                out.push(i);
            }
        }
    }
    Ok(if out.is_empty() { None } else { Some(out) })
}

/// `dict.items()` -- or `.keys()`, named by `method` -- on a JSON object;
/// anything else is the `AttributeError` for that method.
fn dict_items(v: &PyVal, method: &str) -> PyResult<Vec<(String, PyVal)>> {
    match v {
        PyVal::Dict(items) => Ok(items.iter().map(|(k, v)| (k.py_str(), v.clone())).collect()),
        other => Err(PyErr::missing(&other.type_name(), method)),
    }
}

/// `value.split(sep)[0]` on a JSON string.
fn split_first(v: &PyVal, sep: &str) -> PyResult<String> {
    match v {
        PyVal::Str(s) => Ok(s.split(sep).next().unwrap_or("").to_string()),
        other => Err(PyErr::missing(&other.type_name(), "split")),
    }
}

/// `extract_RJ45_ports_index(num_of_asics)`.
pub fn extract_rj45_ports_index(num_of_asics: i128) -> PyResult<Option<Vec<i128>>> {
    extract_ports_index_by_type("RJ45", num_of_asics)
}

/// `extract_cpo_ports_index(num_of_asics)`.
pub fn extract_cpo_ports_index(num_of_asics: i128) -> PyResult<Option<Vec<i128>>> {
    extract_ports_index_by_type("CPO", num_of_asics)
}

/// `extract_asic_id_map(num_of_asics)`: 0-based SFP index to ASIC id.
pub fn extract_asic_id_map(num_of_asics: i128) -> PyResult<Vec<(i128, i128)>> {
    let mut interface2asic: Vec<(String, i128)> = Vec::new();
    for (asic_id, hwsku_json) in get_path_list_to_asic_hwsku_dir(num_of_asics)?.iter().enumerate() {
        let data = load_json_file(hwsku_json, Log::Error).unwrap_or(PyVal::None);
        for (k, _) in dict_items(json::index(&data, "interfaces")?, "keys")? {
            match interface2asic.iter_mut().find(|(i, _)| *i == k) {
                Some(e) => e.1 = asic_id as i128,
                None => interface2asic.push((k, asic_id as i128)),
            }
        }
    }
    let platform_file = io::join(&device_info::get_path_to_platform_dir()?, "platform.json");
    let platform = load_json_file(&platform_file, Log::Error).unwrap_or(PyVal::None);
    let mut map: Vec<(i128, i128)> = Vec::new();
    for (interface, value) in dict_items(json::index(&platform, "interfaces")?, "items")? {
        if let Some(index_raw) = json::get(&value, "index") {
            let index = split_first(index_raw, ",")?;
            // `asic_id_map[int(index)-1] = interface2asic[inteface]`: the
            // right-hand side first, so an unknown interface is the KeyError
            // even when its index would not parse.
            let asic = interface2asic
                .iter()
                .find(|(i, _)| *i == interface)
                .map(|(_, a)| *a)
                .ok_or_else(|| PyErr::key_error(interface.as_str()))?;
            let index = py_int(&index)? - 1;
            match map.iter_mut().find(|(i, _)| *i == index) {
                Some(e) => e.1 = asic,
                None => map.push((index, asic)),
            }
        }
    }
    Ok(map)
}

/// The thread `threading.main_thread()` names: the one that first imported
/// the package, which is the one that first opened the platform.
static MAIN_THREAD: OnceLock<ThreadId> = OnceLock::new();

/// Called by `open()`: the first caller's thread is the main thread.
pub fn note_main_thread() {
    MAIN_THREAD.get_or_init(|| std::thread::current().id());
}

static SHUTDOWN_WATCH_INSTALLED: Mutex<bool> = Mutex::new(false);

/// `watch_shutdown_signals()`.
///
/// On the main thread Python installs SIGTERM/SIGINT handlers that set the
/// shutdown event.  Here the daemon owns the signals, so there is no handler
/// to install: the flag arms the watch, and the daemon's `notify_shutdown()`
/// on the platform API sets the event (see `notify_shutdown` here), as the
/// PyO3 bridge runs the Python handler it kept.  Off the main thread it says
/// so, on every call until the main thread has armed it.
pub fn watch_shutdown_signals() {
    let mut installed = SHUTDOWN_WATCH_INSTALLED.lock().unwrap_or_else(PoisonError::into_inner);
    if *installed {
        return;
    }
    if MAIN_THREAD.get() != Some(&std::thread::current().id()) {
        log_notice(
            "watch_shutdown_signals skipped: not in main thread, \
             readiness waits will not abort on shutdown until armed from the main thread",
        );
        return;
    }
    *installed = true;
}

/// `TimerEvent(interval, cb, repeat)`.
struct TimerEvent {
    interval: f64,
    cb: Box<dyn Fn() -> PyResult<()> + Send + Sync>,
    repeat: bool,
}

struct TimerState {
    /// `(timestamp, sequence, event)`, timestamps as `time.monotonic()`
    /// floats; the earliest runs first.
    queue: Vec<(f64, u64, Arc<TimerEvent>)>,
    seq: u64,
    stop: bool,
}

struct TimerShared {
    state: Mutex<TimerState>,
    cv: Condvar,
}

/// `utils.Timer`: a thread running scheduled callbacks, each repeating
/// `interval` seconds after the previous run finished.
///
/// A callback that raises kills the thread, as an uncaught exception does a
/// Python thread; `threading.excepthook`'s report goes to stderr, without the
/// traceback's frames.
pub struct Timer {
    shared: Arc<TimerShared>,
    thread: Mutex<Option<JoinHandle<()>>>,
    name: String,
}

static THREAD_COUNTER: AtomicU64 = AtomicU64::new(0);

/// `time.monotonic()`: seconds as a float, from an origin of our own -- only
/// differences are used.
fn monotonic() -> f64 {
    static ORIGIN: OnceLock<Instant> = OnceLock::new();
    ORIGIN.get_or_init(Instant::now).elapsed().as_secs_f64()
}

/// The longest `threading.Event.wait` takes: the timeout as nanoseconds in
/// an i64.  Beyond it the wait raises OverflowError.
const PY_WAIT_MAX_S: f64 = 9_223_372_036.854_775;

/// What `threading.excepthook` writes for an exception that ends a thread,
/// without the traceback's frames.
fn report_thread_death(name: &str, e: &PyErr) {
    let last = match e.str() {
        s if s.is_empty() => e.exc.name().to_string(),
        s => format!("{}: {s}", e.exc.name()),
    };
    let _ = writeln!(std::io::stderr(), "Exception in thread {name}:\nTraceback (most recent call last):\n{last}");
}

impl Default for Timer {
    fn default() -> Self {
        Timer::new()
    }
}

impl Timer {
    pub fn new() -> Timer {
        let n = THREAD_COUNTER.fetch_add(1, Ordering::Relaxed) + 1;
        Timer {
            shared: Arc::new(TimerShared {
                state: Mutex::new(TimerState { queue: Vec::new(), seq: 0, stop: false }),
                cv: Condvar::new(),
            }),
            thread: Mutex::new(None),
            name: format!("Thread-{n}"),
        }
    }

    /// `schedule(interval, cb, repeat=True, run_now=True)`.
    pub fn schedule(&self, interval: f64, cb: impl Fn() -> PyResult<()> + Send + Sync + 'static) {
        let ev = Arc::new(TimerEvent { interval, cb: Box::new(cb), repeat: true });
        Self::add(&self.shared, ev, true);
    }

    /// `add_timer_event(timer_event, run_now)`: a negative interval is in
    /// the past, and runs at once, as in Python.
    fn add(shared: &TimerShared, ev: Arc<TimerEvent>, run_now: bool) {
        let mut ts = monotonic();
        if !run_now {
            ts += ev.interval;
        }
        let mut st = shared.state.lock().unwrap_or_else(PoisonError::into_inner);
        st.seq += 1;
        let seq = st.seq;
        st.queue.push((ts, seq, ev));
        drop(st);
        shared.cv.notify_all();
    }

    /// `start()`.
    pub fn start(&self) -> PyResult<()> {
        let mut slot = self.thread.lock().unwrap_or_else(PoisonError::into_inner);
        if slot.is_some() {
            return Err(PyErr::runtime_error("threads can only be started once"));
        }
        let shared = self.shared.clone();
        let name = self.name.clone();
        let handle = std::thread::Builder::new()
            .spawn(move || Self::run(&shared, &name))
            .map_err(|_| PyErr::runtime_error("can't start new thread"))?;
        *slot = Some(handle);
        Ok(())
    }

    /// `stop()`: wakes the thread and waits for it, if it is still alive.
    pub fn stop(&self) {
        let handle = self.thread.lock().unwrap_or_else(PoisonError::into_inner).take();
        let Some(handle) = handle else {
            return;
        };
        if handle.is_finished() {
            let _ = handle.join();
            return;
        }
        self.shared.state.lock().unwrap_or_else(PoisonError::into_inner).stop = true;
        self.shared.cv.notify_all();
        let _ = handle.join();
    }

    fn run(shared: &TimerShared, name: &str) {
        loop {
            let ev = {
                let mut st = shared.state.lock().unwrap_or_else(PoisonError::into_inner);
                loop {
                    if st.stop {
                        return;
                    }
                    let next = st
                        .queue
                        .iter()
                        .enumerate()
                        .min_by(|(_, (a, sa, _)), (_, (b, sb, _))| a.total_cmp(b).then(sa.cmp(sb)))
                        .map(|(i, _)| i);
                    let Some(i) = next else {
                        st = shared.cv.wait(st).unwrap_or_else(PoisonError::into_inner);
                        continue;
                    };
                    let now = monotonic();
                    let ts = st.queue[i].0;
                    if ts > now {
                        // `self._wait_event.wait(self._min_timestamp - now)`.
                        if ts - now > PY_WAIT_MAX_S {
                            report_thread_death(name, &PyErr::other("OverflowError", "timestamp out of range for platform time_t"));
                            return;
                        }
                        st = shared
                            .cv
                            .wait_timeout(st, Duration::from_secs_f64(ts - now))
                            .map_or_else(|e| e.into_inner().0, |(g, _)| g);
                        continue;
                    }
                    break st.queue.swap_remove(i).2;
                }
            };
            if let Err(e) = (ev.cb)() {
                report_thread_death(name, &e);
                return;
            }
            if ev.repeat {
                Self::add(shared, ev, false);
            }
        }
    }
}

/// `natsort.natsorted`'s key for a plain string: text and numbers in turn,
/// numbers compared as numbers.
pub fn natsort_key(s: &str) -> Vec<NatPart> {
    let mut out = Vec::new();
    let mut chars = s.chars().peekable();
    let mut first = true;
    while let Some(&c) = chars.peek() {
        if c.is_ascii_digit() {
            if first {
                out.push(NatPart::Text(String::new()));
            }
            let mut d = String::new();
            while let Some(&c) = chars.peek().filter(|c| c.is_ascii_digit()) {
                d.push(c);
                chars.next();
            }
            out.push(NatPart::Num(d.parse().unwrap_or(u128::MAX)));
        } else {
            let mut t = String::new();
            while let Some(&c) = chars.peek().filter(|c| !c.is_ascii_digit()) {
                t.push(c);
                chars.next();
            }
            out.push(NatPart::Text(t));
        }
        first = false;
    }
    out
}

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum NatPart {
    Text(String),
    Num(u128),
}


#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use std::thread::sleep;

    thread_local! {
        /// What this thread logged at NOTICE or above, which is what reaches
        /// syslog.
        pub(crate) static LOGGED: std::cell::RefCell<Vec<String>> = const { std::cell::RefCell::new(Vec::new()) };
    }
    use crate::testing::Root;
    use platform_base::testing::{FakeRedis, CONFIG_DB};

    /// The platform `machine.conf` names, as `get_platform()` then reads it:
    /// `$PLATFORM`, if the environment sets it, wins -- so a caller checks.
    pub(crate) fn platform(root: &Root, name: &str) -> Option<String> {
        // Says the skip the callers' comparison makes under `$PLATFORM`.
        crate::testing::platform_from_env();
        root.write(device_info::MACHINE_CONF_PATH, &format!("onie_platform={name}\n"));
        device_info::get_platform().unwrap()
    }

    /// The platform's device directory, made, as `get_path_to_platform_dir()`
    /// finds it.
    pub(crate) fn platform_dir(root: &Root) -> String {
        let p = device_info::get_platform().unwrap().unwrap_or_default();
        std::fs::create_dir_all(root.real(&io::join(device_info::HOST_DEVICE_PATH, &p))).unwrap();
        device_info::get_path_to_platform_dir().unwrap()
    }

    /// CONFIG_DB with an hwsku; the answer is whatever the process cached
    /// first, so callers use what this returns.
    pub(crate) fn hwsku(root: &Root) -> String {
        let redis = FakeRedis::get();
        redis.install(root);
        redis.set(CONFIG_DB, "CONFIG_DB_INITIALIZED", "1");
        redis.hset(CONFIG_DB, "DEVICE_METADATA|localhost", &[("hwsku", "SKU-E")]);
        device_info::get_hwsku().unwrap()
    }

    fn err<T: std::fmt::Debug>(r: PyResult<T>) -> (String, String) {
        let e = r.unwrap_err();
        (e.exc.name().to_string(), e.str())
    }

    #[test]
    fn fnmatch_matches_what_python_matches() {
        assert!(fnmatch("fan1_status", "fan*_status"));
        assert!(!fnmatch("fan1_speed_get", "fan*_status"));
        assert!(fnmatch("cpu_core0", "cpu_core[!_]"));
        assert!(!fnmatch("cpu_core_", "cpu_core[!_]"));
        assert!(!fnmatch("cpu_core0_max", "cpu_core[!_]"));
        assert!(fnmatch("psu2_i2c_addr", "psu*_i2c_addr"));
    }

    #[test]
    fn python_arithmetic() {
        assert_eq!(floor_div(-7, 2).unwrap(), -4);
        assert_eq!(floor_div(100 * 6100, 13000).unwrap(), 46);
        assert_eq!(round_half_even(0.5), 0);
        assert_eq!(round_half_even(1.5), 2);
        assert_eq!(round_half_even(2.5), 2);
        assert_eq!(round_half_even(153.0 * 100.0 / 255.0), 60);
        assert_eq!(round_half_even(-2.5), -2);
    }

    #[test]
    fn natural_order_compares_numbers_as_numbers() {
        let mut ports = vec!["Ethernet8", "Ethernet16", "Ethernet0", "Ethernet2", "Ethernet100"];
        ports.sort_by_key(|p| natsort_key(p));
        assert_eq!(ports, vec!["Ethernet0", "Ethernet2", "Ethernet8", "Ethernet16", "Ethernet100"]);
        assert!(natsort_key("10a") > natsort_key("9b"));
    }

    #[test]
    fn a_timer_repeats_until_stopped_and_stop_waits_for_it() {
        let n = Arc::new(AtomicU64::new(0));
        let t = Timer::new();
        let c = n.clone();
        t.schedule(0.05, move || {
            c.fetch_add(1, Ordering::Relaxed);
            Ok(())
        });
        t.start().unwrap();
        assert!(t.start().is_err(), "a thread starts once");
        sleep(Duration::from_millis(300));
        t.stop();
        let seen = n.load(Ordering::Relaxed);
        assert!(seen >= 3, "ran {seen} times");
        sleep(Duration::from_millis(150));
        assert_eq!(n.load(Ordering::Relaxed), seen, "ran after stop");
    }

    #[test]
    fn a_callback_that_raises_ends_the_thread() {
        let n = Arc::new(AtomicU64::new(0));
        let t = Timer::new();
        let c = n.clone();
        t.schedule(0.01, move || {
            c.fetch_add(1, Ordering::Relaxed);
            Err(PyErr::missing("RJ45Port", "get_asic_index"))
        });
        t.start().unwrap();
        sleep(Duration::from_millis(100));
        assert_eq!(n.load(Ordering::Relaxed), 1);
        t.stop();
    }

    #[test]
    fn splitlines_and_key_value() {
        assert_eq!(py_splitlines("a\nb\r\nc"), vec!["a", "b", "c"]);
        assert_eq!(py_splitlines("a\n"), vec!["a"]);
        let kv = key_value("PN: X\nSN : Y:Z\n", ":").unwrap();
        assert_eq!(kv, vec![("PN".into(), "X".into()), ("SN".into(), "Y:Z".into())]);
        assert!(key_value("PN X", ":").is_err());
    }

    #[test]
    fn reads_answer_the_default_or_raise_as_python_does() {
        let root = Root::new();
        // A file that is not there: the default, or the IOError.
        assert_eq!(read_str("/x/missing", "default return", Log::Error), "default return");
        assert_eq!(err(read_str_raise("/x/missing", Log::Error)).0, "FileNotFoundError");
        assert_eq!(read_int("/x/missing", 100, Log::Error), 100);
        assert_eq!(err(read_int_raise("/x/missing", Log::Off)).0, "FileNotFoundError");
        assert_eq!(read_float("/x/missing", 2.5, Log::Error), 2.5);
        assert_eq!(read_float_opt("/x/missing", Log::Off), None);
        // Values are stripped and converted.
        root.write("/x/s", "  hello  ");
        assert_eq!(read_str("/x/s", "", Log::Error), "hello");
        assert_eq!(read_str_raise("/x/s", Log::Error).unwrap(), "hello");
        root.write("/x/i", "123 \n");
        assert_eq!(read_int("/x/i", 0, Log::Error), 123);
        assert_eq!(read_int_raise("/x/i", Log::Error).unwrap(), 123);
        root.write("/x/f", "2.5 ");
        assert_eq!(read_float("/x/f", 0.0, Log::Error), 2.5);
        assert_eq!(read_float_opt("/x/f", Log::Error), Some(2.5));
        // A ValueError is caught like an IOError.
        assert_eq!(read_int("/x/s", 7, Log::Warning), 7);
        assert_eq!(
            err(read_int_raise("/x/s", Log::Notice)),
            ("ValueError".into(), "invalid literal for int() with base 10: 'hello'".into())
        );
        // Bytes the locale's codec cannot decode: UnicodeDecodeError, a
        // ValueError, so the default -- when the codec is ASCII.
        std::fs::write(root.real("/x/bin"), b"\xff\xfe").unwrap();
        if platform_base::py::codec::locale() == platform_base::py::codec::Codec::Ascii {
            assert_eq!(read_str("/x/bin", "d", Log::Info), "d");
        }
        // Any other exception escapes, whatever raise_exception says.
        let r = read_from_file("/x/s", |_| Err::<(), _>(PyErr::type_error("t")), (), false, Log::Debug);
        assert_eq!(err(r).0, "TypeError");
    }

    #[test]
    fn write_file_writes_str_and_answers_false_or_raises() {
        let root = Root::new();
        assert!(!write_file("/x/w", &"  hello  ", false, Log::Error).unwrap());
        std::fs::create_dir_all(root.real("/x")).unwrap();
        assert!(write_file("/x/w", &"  hello  ", false, Log::Error).unwrap());
        assert_eq!(read_str("/x/w", "", Log::Error), "hello");
        assert!(write_file("/x/w", &123, true, Log::Error).unwrap());
        assert_eq!(read_int("/x/w", 0, Log::Error), 123);
        assert_eq!(err(write_file("/not/exist/file", &"123", true, Log::Off)).0, "FileNotFoundError");
    }

    #[test]
    fn key_value_files_parse_as_dicts() {
        let root = Root::new();
        root.write("/x/kv", "a:b\nc : d:e\na:z\n");
        assert_eq!(
            read_key_value("/x/kv", Log::Error, ":"),
            vec![("a".to_string(), "z".to_string()), ("c".to_string(), "d:e".to_string())]
        );
        root.write("/x/eq", "a=b");
        assert_eq!(read_key_value("/x/eq", Log::Error, "="), vec![("a".to_string(), "b".to_string())]);
        // A line without the delimiter is a ValueError: the default, {}.
        root.write("/x/bad", "a:b\nnodelim\n");
        assert!(read_key_value("/x/bad", Log::Error, ":").is_empty());
        assert!(read_key_value("/x/missing", Log::Error, ":").is_empty());
        assert_eq!(py_splitlines("a\x0bb\x0cc\x1cd\x1de\x1ef\rg"), vec!["a", "b", "c", "d", "e", "f", "g"]);
        assert!(py_splitlines("").is_empty());
    }

    #[test]
    fn every_log_level_and_truthiness() {
        for l in [Log::Error, Log::Warning, Log::Notice, Log::Info, Log::Debug, Log::Off] {
            l.emit("utils test");
        }
        log_error("e");
        log_warning("w");
        log_notice("n");
        log_info("i");
        log_debug("d");
        assert!(true.truthy() && !false.truthy());
        assert!(1i128.truthy() && !0i128.truthy());
        assert!(1usize.truthy() && !0usize.truthy());
        assert!(String::from("x").truthy() && !String::new().truthy());
        assert!(PyVal::Int(1).truthy() && !PyVal::None.truthy());
        assert!(Some(1i128).truthy() && !Some(0i128).truthy() && !None::<i128>.truthy());
        assert!(Vec::from([0]).truthy() && !Vec::<i32>::new().truthy());
    }

    #[test]
    fn a_read_only_cache_keeps_only_a_truthy_value() {
        let _root = Root::new();
        let c: ReadOnlyCache<i128> = ReadOnlyCache::default();
        assert_eq!(c.get(|| 0), 0);
        assert_eq!(c.get(|| 1000), 1000);
        assert_eq!(c.get(|| 10000), 1000);
        forget_caches();
        assert_eq!(c.get(|| 5), 5);
    }

    #[test]
    fn is_host_is_no_dockerenv() {
        let root = Root::new();
        assert!(is_host());
        root.write("/.dockerenv", "");
        assert!(is_host(), "a True is kept");
        forget_caches();
        assert!(!is_host());
    }

    #[test]
    fn default_return_and_run_command() {
        assert_eq!(default_return(100, Ok(1), "f"), 1);
        assert_eq!(default_return(100, Err(PyErr::runtime_error("")), "f"), 100);
        assert_eq!(run_command(&["sh", "-c", "echo ' hi '"]).as_deref(), Some("hi"));
        assert_eq!(run_command(&["not_a_command"]), None);
        assert_eq!(run_command(&[]), None);
        // Output the locale's codec cannot decode: None.
        assert_eq!(run_command(&["sh", "-c", "printf '\\377'"]), None);
        if platform_base::py::codec::locale() == platform_base::py::codec::Codec::Ascii {
            assert_eq!(run_command(&["echo", "\u{e9}"]), None);
        }
    }

    #[test]
    fn load_json_file_answers_none_on_any_failure() {
        let root = Root::new();
        assert_eq!(load_json_file("/x/some_file", Log::Error), None);
        root.write("/x/empty", "");
        assert_eq!(load_json_file("/x/empty", Log::Error), None);
        root.write("/x/j", r#"{"a": "b"}"#);
        let v = load_json_file("/x/j", Log::Off).unwrap();
        assert_eq!(json::get(&v, "a"), Some(&PyVal::str("b")));
    }

    #[test]
    fn waits_end_as_soon_as_their_conditions_hold() {
        assert!(wait_until(|| true, 1.0, 1.0));
        assert!(!wait_until(|| false, 0.2, 0.1));
        let mut n = 0;
        assert!(wait_until(
            || {
                n += 1;
                n > 2
            },
            1.0,
            0.01
        ));
        assert!(wait_until_conditions(vec![Box::new(|| true)], 1.0, 0.01));
        assert!(!wait_until_conditions(vec![Box::new(|| true), Box::new(|| false)], 0.05, 0.01));
        assert!(!wait_until_conditions(vec![], 0.0, 0.01));
        let mut m = 0;
        assert!(wait_until_conditions(
            vec![Box::new(move || {
                m += 1;
                m > 1
            })],
            1.0,
            0.01
        ));
        assert!(try_wait_until_conditions(vec![Box::new(|| Ok(true))], 1.0, 0.01).unwrap());
        assert!(!try_wait_until_conditions(vec![Box::new(|| Ok(false))], 0.05, 0.01).unwrap());
        let r = try_wait_until_conditions(vec![Box::new(|| Err(PyErr::runtime_error("boom")))], 1.0, 0.01);
        assert_eq!(err(r), ("RuntimeError".into(), "boom".into()));
    }

    #[test]
    fn a_file_is_waited_for_until_it_is_readable() {
        let root = Root::new();
        root.write("/x/ready", "1");
        assert!(readable("/x/ready"));
        assert!(wait_for_file_creation("/x/ready", 1.0));
        // No directory: no wait.
        assert!(!wait_for_file_creation("/nodir/file", 5.0));
        assert!(!wait_for_file_creation("/x/never", 0.2));
        let late = root.real("/x/late");
        let t = std::thread::spawn(move || {
            sleep(Duration::from_millis(150));
            std::fs::write(late, "1").unwrap();
        });
        assert!(wait_for_file_creation("/x/late", 5.0));
        t.join().unwrap();
        // The timeout counts from the directory's last event: other files
        // written every half second keep a 1.5-second wait going until the
        // file comes, 3 seconds in.
        let dir = root.real("/x");
        let t = std::thread::spawn(move || {
            for i in 0..6 {
                sleep(Duration::from_millis(500));
                std::fs::write(dir.join(format!("other{i}")), "1").unwrap();
            }
            std::fs::write(dir.join("slow"), "1").unwrap();
        });
        let begin = Instant::now();
        assert!(wait_for_file_creation("/x/slow", 1.5));
        assert!(begin.elapsed() >= Duration::from_secs(3));
        t.join().unwrap();
    }

    #[test]
    fn dirname_and_basename_split_as_posixpath_does() {
        for (p, d, b) in [
            ("/var/run/x", "/var/run", "x"),
            ("/x", "/", "x"),
            ("x", "", "x"),
            ("//x", "//", "x"),
            ("/a//b", "/a", "b"),
            ("/a/", "/a", ""),
        ] {
            assert_eq!((py_dirname(p), py_basename(p)), (d, b), "{p}");
        }
    }

    #[test]
    fn sysfs_labels_wait_for_the_directory_then_the_file() {
        let root = Root::new();
        let dir = root.real("/var/run/hw-management");
        let file = root.real(SYSFS_LABELS_READY_FILE);
        let t = std::thread::spawn(move || {
            sleep(Duration::from_millis(200));
            std::fs::create_dir_all(&dir).unwrap();
            sleep(Duration::from_millis(900));
            std::fs::write(file, "1").unwrap();
        });
        assert!(ensure_sysfs_labels_ready());
        t.join().unwrap();
        assert!(ensure_sysfs_labels_ready(), "there already");
    }

    #[test]
    fn glob_matches_in_the_last_component_in_directory_order() {
        let root = Root::new();
        for f in ["fan1_status", "fan2_status", ".fan3_status", "fan1_speed_get"] {
            root.write(&format!("/run/hw-management/thermal/{f}"), "1");
        }
        let mut got = glob("/run/hw-management/thermal/fan*_status");
        got.sort();
        assert_eq!(got, ["/run/hw-management/thermal/fan1_status", "/run/hw-management/thermal/fan2_status"]);
        assert_eq!(glob("/run/hw-management/thermal/.fan*"), ["/run/hw-management/thermal/.fan3_status"]);
        assert!(glob("/nowhere/*").is_empty());
        root.write("/top_file", "");
        assert_eq!(glob("/top_f?le"), ["/top_file"]);
        // A relative pattern is the working directory's: the root under a test.
        root.write("rel_a", "");
        assert_eq!(glob("rel_[a-c]"), ["rel_a"]);
        assert!(fnmatch("[x", "[x"));
        assert!(!fnmatch("y", "[x"));
        assert!(!fnmatch("", "[ab]"));
        assert!(!fnmatch("", "?"));
        assert!(fnmatch("b", "[!a]") && !fnmatch("a", "[!a]"));
    }


    #[test]
    fn python_reprs_and_return_codes() {
        assert_eq!(py_list_repr(&["i2cset", "-f", "a'b"]), r#"['i2cset', '-f', "a'b"]"#);
        let st = Command::new("sh").args(["-c", "exit 3"]).status().unwrap();
        assert_eq!(py_returncode(&st), 3);
        let st = Command::new("sh").args(["-c", "kill -9 $$"]).status().unwrap();
        assert_eq!(py_returncode(&st), -9);
        assert_eq!(err(floor_div(1, 0)), ("ZeroDivisionError".into(), "integer division or modulo by zero".into()));
        assert_eq!(floor_div(7, -2).unwrap(), -4);
        assert_eq!(floor_div(6, 3).unwrap(), 2);
        assert_eq!(round_half_even(3.5), 4);
        assert_eq!(round_half_even(0.4), 0);
        assert!(natsort_key("a99999999999999999999999999999999999999999") > natsort_key("a1"));
        assert_eq!(natsort_key(""), Vec::<NatPart>::new());
    }

    /// The `get_path_to_hwsku_directory` family over a device directory.
    #[test]
    fn hwsku_paths_join_platform_hwsku_and_asic() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0");
        let dir = platform_dir(&root);
        let sku = hwsku(&root);
        let base = format!("{dir}/{sku}");
        assert_eq!(get_path_to_hwsku_directory(None).unwrap(), base);
        assert_eq!(get_path_to_hwsku_directory(Some(1)).unwrap(), format!("{base}/1"));
        assert_eq!(get_path_list_to_asic_hwsku_dir(1).unwrap(), [format!("{base}/hwsku.json")]);
        assert_eq!(
            get_path_list_to_asic_hwsku_dir(2).unwrap(),
            [format!("{base}/0/hwsku.json"), format!("{base}/1/hwsku.json")]
        );
        assert!(get_path_list_to_asic_hwsku_dir(0).unwrap().is_empty());
    }

    /// `extract_RJ45_ports_index` / `extract_cpo_ports_index`, the cases of
    /// Python's test_utils.
    #[test]
    fn ports_of_a_type_come_from_platform_and_hwsku_json() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0");
        let dir = platform_dir(&root);
        let sku = hwsku(&root);
        let pj = format!("{dir}/platform.json");
        let hj = format!("{dir}/{sku}/hwsku.json");
        // Neither file.
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        // hwsku.json only.
        root.write(&hj, r#"{"interfaces": {"Ethernet0": {"port_type": "CPO"}}}"#);
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        // platform.json without an index.
        root.write(&pj, r#"{"interfaces": {"Ethernet0": {"lanes": "0"}}}"#);
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        // platform.json only.
        std::fs::remove_file(root.real(&hj)).unwrap();
        root.write(&pj, r#"{"interfaces": {"Ethernet0": {"index": "1", "lanes": "0"}}}"#);
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        // No port of the type.
        root.write(&hj, r#"{"interfaces": {"Ethernet0": {"port_type": "SFP"}}}"#);
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        root.write(&hj, r#"{"interfaces": {"Ethernet0": {"default_brkout_mode": "1x1000[100,10]", "port_type": "RJ45"}}}"#);
        assert_eq!(extract_rj45_ports_index(1).unwrap(), Some(vec![0]));
        assert_eq!(extract_cpo_ports_index(1).unwrap(), None);
        // Several; the index's first number; a port hwsku.json lacks.
        root.write(
            &pj,
            r#"{"interfaces": {"Ethernet0": {"index": "1,1,1,1"}, "Ethernet4": {"index": "5"},
                "Ethernet8": {"index": "9"}, "Ethernet12": {"index": "13"}, "Ethernet0": {"index": "1"}}}"#,
        );
        root.write(
            &hj,
            r#"{"interfaces": {"Ethernet0": {"port_type": "CPO"}, "Ethernet4": {"port_type": "CPO"},
                "Ethernet8": {"port_type": "CPO"}, "Ethernet16": {"port_type": "CPO"}, "Ethernet4": {"port_type": "CPO"}}}"#,
        );
        assert_eq!(extract_cpo_ports_index(1).unwrap(), Some(vec![0, 4, 8]));
        // Two ASICs: both hwsku.json files, merged.
        root.write(&format!("{dir}/{sku}/0/hwsku.json"), r#"{"interfaces": {"Ethernet0": {"port_type": "CPO"}}}"#);
        root.write(&format!("{dir}/{sku}/1/hwsku.json"), r#"{"interfaces": {"Ethernet8": {"port_type": "CPO"}}}"#);
        assert_eq!(extract_cpo_ports_index(2).unwrap(), Some(vec![0, 8]));
        // What Python's subscripts and methods raise on the wrong shapes.
        root.write(&pj, r#"{"interfaces": []}"#);
        assert_eq!(err(extract_cpo_ports_index(1)), ("AttributeError".into(), "'list' object has no attribute 'items'".into()));
        root.write(&pj, r#"{"interfaces": {"Ethernet0": {"index": 1}}}"#);
        assert_eq!(err(extract_cpo_ports_index(1)), ("AttributeError".into(), "'int' object has no attribute 'split'".into()));
        root.write(&pj, r#"{"chassis": {}}"#);
        assert_eq!(err(extract_cpo_ports_index(1)).0, "KeyError");
        root.write(&pj, r#"{"interfaces": {"Ethernet0": {"index": "x"}}}"#);
        root.write(&hj, r#"{"interfaces": {"Ethernet0": {"port_type": "CPO"}}}"#);
        assert_eq!(err(extract_cpo_ports_index(1)).0, "ValueError");
    }

    #[test]
    fn the_asic_of_each_port_is_the_hwsku_json_listing_it() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn5600-r0");
        let dir = platform_dir(&root);
        let sku = hwsku(&root);
        root.write(&format!("{dir}/{sku}/0/hwsku.json"), r#"{"interfaces": {"Ethernet0": {}, "Ethernet4": {}}}"#);
        root.write(&format!("{dir}/{sku}/1/hwsku.json"), r#"{"interfaces": {"Ethernet8": {}, "Ethernet4": {}}}"#);
        root.write(
            &format!("{dir}/platform.json"),
            r#"{"interfaces": {"Ethernet0": {"index": "1,1"}, "Ethernet4": {"index": "2"}, "Ethernet8": {"index": "3"},
                "Ethernet9": {"lanes": "1"}, "Ethernet0": {"index": "1"}}}"#,
        );
        assert_eq!(extract_asic_id_map(2).unwrap(), vec![(0, 0), (1, 1), (2, 1)]);
        root.write(&format!("{dir}/platform.json"), r#"{"interfaces": {"Ethernet12": {"index": "4"}}}"#);
        assert_eq!(err(extract_asic_id_map(2)), ("KeyError".into(), "'Ethernet12'".into()));
        root.write(&format!("{dir}/{sku}/hwsku.json"), r#"{"interfaces": []}"#);
        assert_eq!(err(extract_asic_id_map(1)).0, "AttributeError");
    }

    /// `notify_shutdown()` ends a wait in progress on another thread at
    /// once, with false, and every wait after it at its first interval --
    /// once the watch is armed, as Python's event is set only by the handler
    /// arming installs.  In a process of its own: the event ends every wait
    /// in the process.
    #[test]
    fn a_shutdown_ends_every_wait_at_once() {
        if !crate::testing::in_own_process("utils::tests::a_shutdown_ends_every_wait_at_once") {
            return;
        }
        notify_shutdown();
        assert!(!shutdown_requested(), "set before the watch was armed");
        note_main_thread();
        watch_shutdown_signals();

        let started = Instant::now();
        let waits = vec![
            std::thread::spawn(|| wait_until(|| false, 300.0, 60.0)),
            std::thread::spawn(|| wait_until_conditions(vec![Box::new(|| false)], 300.0, 60.0)),
            std::thread::spawn(|| try_wait_until_conditions(vec![Box::new(|| Ok(false))], 300.0, 60.0).unwrap()),
        ];
        sleep(Duration::from_millis(200));
        notify_shutdown();
        for w in waits {
            assert!(!w.join().unwrap());
        }
        assert!(shutdown_requested());
        // A condition that already holds still answers true, as in Python.
        assert!(wait_until(|| true, 300.0, 60.0));
        assert!(wait_until_conditions(vec![Box::new(|| true)], 300.0, 60.0));
        assert!(!wait_until(|| false, 300.0, 60.0));
        assert!(started.elapsed() < Duration::from_secs(10), "{:?}", started.elapsed());
    }

    #[test]
    fn shutdown_signals_are_watched_from_the_main_thread_only() {
        note_main_thread();
        let main = MAIN_THREAD.get() == Some(&std::thread::current().id());
        std::thread::spawn(watch_shutdown_signals).join().unwrap();
        watch_shutdown_signals();
        if main {
            assert!(*SHUTDOWN_WATCH_INSTALLED.lock().unwrap());
            watch_shutdown_signals();
        }
    }

    #[test]
    fn a_timer_runs_the_earliest_event_first_and_says_what_killed_it() {
        let order = Arc::new(Mutex::new(Vec::new()));
        let t = Timer::default();
        t.stop();
        let (a, b) = (order.clone(), order.clone());
        t.schedule(10.0, move || {
            a.lock().unwrap().push("a");
            Ok(())
        });
        t.schedule(10.0, move || {
            b.lock().unwrap().push("b");
            Err(PyErr::not_implemented())
        });
        t.start().unwrap();
        sleep(Duration::from_millis(100));
        assert_eq!(*order.lock().unwrap(), ["a", "b"]);
        t.stop();
        // Nothing scheduled: the thread waits until stopped.
        let idle = Timer::new();
        idle.start().unwrap();
        sleep(Duration::from_millis(20));
        idle.stop();
    }

    #[test]
    fn splitlines_breaks_where_python_s_str_splitlines_does() {
        assert_eq!(py_splitlines("a\u{85}b\u{2028}c\u{2029}d\x0be\r\nf\n"), ["a", "b", "c", "d", "e", "f"]);
    }

    #[test]
    fn a_write_the_locale_cannot_encode_is_a_caught_value_error() {
        let root = Root::new();
        root.write("/tmp/x/f", "");
        if platform_base::py::codec::locale() != platform_base::py::codec::Codec::Ascii {
            return;
        }
        assert!(!write_file("/tmp/x/f", &"\u{e9}", false, Log::Error).unwrap());
        let e = write_file("/tmp/x/f", &"\u{e9}", true, Log::Error).unwrap_err();
        assert_eq!(e.exc.name(), "UnicodeEncodeError");
    }
}
