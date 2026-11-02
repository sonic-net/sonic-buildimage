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


//! `sonic_platform/inotify_helper.py`: wait for hw-management to write a
//! value to a file.

use std::cell::RefCell;
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::sync::OnceLock;
use std::time::{Instant, SystemTime};

use platform_base::io;
use platform_base::logger::SysLogger;
use platform_base::py::{PyErr, PyResult};

use crate::utils::{self, Log};

/// The module's `logger = SysLogger("mlnx-platform-api")`.
fn logger() -> &'static SysLogger {
    static L: OnceLock<SysLogger> = OnceLock::new();
    L.get_or_init(|| SysLogger::new(Some("mlnx-platform-api")))
}

/// `InotifyHelper(file_path)`.
pub struct InotifyHelper {
    path: String,
    notifier: RefCell<Inotify>,
}

impl InotifyHelper {
    /// `inotify.adapters.Inotify()` raises its own InotifyError when there is
    /// no descriptor to be had, so Python's "INOTIFY adapter error!" branch,
    /// which tests the object for truth, is never taken.
    pub fn new(path: &str) -> PyResult<InotifyHelper> {
        let notifier = Inotify::new()?;
        if !io::exists(path) {
            logger().log_error(&format!("{path} does not exist"));
            return Err(PyErr::os_error(libc::ENOENT, Some(path)));
        }
        Ok(InotifyHelper { path: path.to_string(), notifier: RefCell::new(notifier) })
    }

    /// `wait_watch(timeout, expected_value)`: the value once a write leaves
    /// it there -- checked on each write, until `timeout` seconds pass
    /// without one -- and once more at the end; None if never.
    pub fn wait_watch(&self, timeout: f64, expected: i128) -> PyResult<Option<i128>> {
        let mut notifier = self.notifier.borrow_mut();
        notifier.add_watch(&self.path, libc::IN_CLOSE_WRITE)?;
        let hit = notifier.event_gen(timeout, |_| {
            let v = utils::read_int_raise(&self.path, Log::Error)?;
            Ok((v == expected).then_some(v))
        })?;
        if hit.is_some() {
            return Ok(hit);
        }
        let v = utils::read_int_raise(&self.path, Log::Error)?;
        Ok(if v == expected { Some(v) } else { None })
    }
}

/// How long ago a monotonic instant was, as Python prints `end - start`.
pub fn elapsed_repr(start: Instant) -> String {
    platform_base::py::float_repr(start.elapsed().as_secs_f64())
}

/// `inotify.constants.MASK_LOOKUP`, in its dict's order: `0o0004000`
/// (IN_NONBLOCK) and IN_MOVE_SELF are the same key, so the second name
/// sits in the first one's place.
const MASK_LOOKUP: [(u32, &str); 21] = [
    (0o2000000, "IN_CLOEXEC"),
    (0o0004000, "IN_MOVE_SELF"),
    (0x1, "IN_ACCESS"),
    (0x2, "IN_MODIFY"),
    (0x4, "IN_ATTRIB"),
    (0x8, "IN_CLOSE_WRITE"),
    (0x10, "IN_CLOSE_NOWRITE"),
    (0x20, "IN_OPEN"),
    (0x40, "IN_MOVED_FROM"),
    (0x80, "IN_MOVED_TO"),
    (0x100, "IN_CREATE"),
    (0x200, "IN_DELETE"),
    (0x400, "IN_DELETE_SELF"),
    (0x2000, "IN_UNMOUNT"),
    (0x4000, "IN_Q_OVERFLOW"),
    (0x8000, "IN_IGNORED"),
    (0x0100_0000, "IN_ONLYDIR"),
    (0x0200_0000, "IN_DONT_FOLLOW"),
    (0x2000_0000, "IN_MASK_ADD"),
    (0x4000_0000, "IN_ISDIR"),
    (0x8000_0000, "IN_ONESHOT"),
];

/// `Inotify._get_event_names(mask)`.
fn event_names(mut mask: u32) -> Vec<&'static str> {
    let mut names = Vec::new();
    for (bit, name) in MASK_LOOKUP {
        if mask & bit != 0 {
            names.push(name);
            mask -= bit;
            if mask == 0 {
                break;
            }
        }
    }
    names
}

fn names_repr(names: &[&str]) -> String {
    utils::py_list_repr(names)
}

/// `inotify.adapters`' debug lines: seen only once the root logger has a
/// handler (the liquid_cooling import's `basicConfig`).
fn debug(msg: &str) {
    platform_base::logger::py_log("inotify.adapters", platform_base::logger::PyLevel::Debug, msg);
}

/// `InotifyError`: the libc errno is part of the message.
fn inotify_error(call_result: i32) -> PyErr {
    let e = std::io::Error::last_os_error();
    let errno = e.raw_os_error().unwrap_or(0);
    let s = e.to_string();
    let msg = s.split(" (os error ").next().unwrap_or("").to_string();
    PyErr::other(
        "InotifyError",
        format!("Call failed (should not be -1): ({call_result}) ERRNO=({errno}) [{msg}]"),
    )
}

/// One event `event_gen` yields: `(header, type_names, path, filename)`.
pub(crate) struct Event {
    pub type_names: Vec<&'static str>,
    pub path: String,
    pub filename: String,
}

/// `inotify.adapters.Inotify` (0.2.12): one descriptor, its watches, and
/// the bytes of an event not yet read whole.
pub(crate) struct Inotify {
    fd: OwnedFd,
    /// `__watches`: path to descriptor, in the order they were added.
    watches: Vec<(String, i32)>,
    buffer: Vec<u8>,
}

impl Inotify {
    /// `Inotify()`.
    pub(crate) fn new() -> PyResult<Inotify> {
        // SAFETY: no arguments; the result is checked.  Close-on-exec, which
        // ctypes' call leaves off: a firmware tool spawned from Rust does not
        // close what it inherits the way `subprocess` does.
        let raw = unsafe { libc::inotify_init1(libc::IN_CLOEXEC) };
        if raw < 0 {
            return Err(inotify_error(raw));
        }
        // SAFETY: a fresh descriptor this struct now owns.
        let fd = unsafe { OwnedFd::from_raw_fd(raw) };
        debug(&format!("Inotify handle is ({raw})."));
        Ok(Inotify { fd, watches: Vec::new(), buffer: Vec::new() })
    }

    /// `add_watch(path, mask)`: a path already watched is warned about and
    /// left as it is.
    pub(crate) fn add_watch(&mut self, path: &str, mask: u32) -> PyResult<()> {
        debug(&format!("Adding watch: [{path}]"));
        if self.watches.iter().any(|(p, _)| p == path) {
            platform_base::logger::py_log(
                "inotify.adapters",
                platform_base::logger::PyLevel::Warning,
                &format!("Path already being watched: [{path}]"),
            );
            return Ok(());
        }
        let c = std::ffi::CString::new(io::real(path).to_string_lossy().into_owned())
            .map_err(|_| PyErr::value_error("embedded null byte"))?;
        // SAFETY: valid descriptor and NUL-terminated path.
        let wd = unsafe { libc::inotify_add_watch(self.fd.as_raw_fd(), c.as_ptr(), mask) };
        if wd < 0 {
            return Err(inotify_error(wd));
        }
        debug(&format!("Added watch ({wd}): [{path}]"));
        self.watches.push((path.to_string(), wd));
        Ok(())
    }

    /// `event_gen(timeout_s=timeout, yield_nones=False)` with its consumer
    /// as `on_event`, which ends the generator by answering `Some`.  Each
    /// round blocks a whole second; the generator stops after the round in
    /// which `timeout` seconds have passed since the last event -- not
    /// since it started -- so a busy directory keeps it going.  A terminal
    /// event (IN_Q_OVERFLOW, IN_UNMOUNT) raises TerminalEventException.
    pub(crate) fn event_gen<R>(
        &mut self,
        timeout: f64,
        mut on_event: impl FnMut(&Event) -> PyResult<Option<R>>,
    ) -> PyResult<Option<R>> {
        // `time.time()`, as Python measures it.
        let mut last_hit = SystemTime::now();
        loop {
            let mut pfd = libc::pollfd { fd: self.fd.as_raw_fd(), events: libc::POLLIN, revents: 0 };
            // SAFETY: one valid pollfd.
            let n = unsafe { libc::poll(&mut pfd, 1, 1000) };
            if n < 0 {
                let e = std::io::Error::last_os_error();
                if e.raw_os_error() != Some(libc::EINTR) {
                    return Err(PyErr::from_io(&e, None));
                }
            } else if n > 0 {
                debug(&format!("Events received from epoll: {}", names_repr(&event_names(pfd.revents as u32))));
                for event in self.handle_events()? {
                    last_hit = SystemTime::now();
                    for name in &event.type_names {
                        if *name == "IN_Q_OVERFLOW" || *name == "IN_UNMOUNT" {
                            return Err(PyErr::other("TerminalEventException", *name));
                        }
                    }
                    if let Some(r) = on_event(&event)? {
                        return Ok(Some(r));
                    }
                }
            }
            if last_hit.elapsed().map(|d| d.as_secs_f64()).unwrap_or(0.0) > timeout {
                return Ok(None);
            }
        }
    }

    /// `_handle_inotify_event`: one read of up to 1024 bytes, parsed into
    /// as many whole events as it completes.  An event for a descriptor that
    /// is not a watch -- IN_Q_OVERFLOW comes with -1 -- is dropped unseen,
    /// its name never decoded.
    fn handle_events(&mut self) -> PyResult<Vec<Event>> {
        let mut buf = [0u8; 1024];
        // SAFETY: reading into a buffer of the given size.
        let got = unsafe { libc::read(self.fd.as_raw_fd(), buf.as_mut_ptr().cast(), buf.len()) };
        if got < 0 {
            return Err(PyErr::from_io(&std::io::Error::last_os_error(), None));
        }
        let mut out = Vec::new();
        if got == 0 {
            return Ok(out);
        }
        self.buffer.extend_from_slice(&buf[..got as usize]);
        loop {
            if self.buffer.len() < 16 {
                debug("Not enough bytes for a header.");
                return Ok(out);
            }
            let word = |i: usize| u32::from_ne_bytes([self.buffer[i], self.buffer[i + 1], self.buffer[i + 2], self.buffer[i + 3]]);
            let (wd, mask, len) = (word(0) as i32, word(4), word(12) as usize);
            let type_names = event_names(mask);
            debug(&format!("Events received in stream: {}", names_repr(&type_names)));
            if self.buffer.len() < 16 + len {
                return Ok(out);
            }
            let raw = self.buffer[16..16 + len].to_vec();
            let rest = self.buffer.split_off(16 + len);
            self.buffer = rest;
            if let Some((path, _)) = self.watches.iter().find(|(_, w)| *w == wd) {
                let end = raw.iter().rposition(|&b| b != 0).map_or(0, |i| i + 1);
                let filename = platform_base::py::codec::decode(platform_base::py::codec::Codec::Utf8, &raw[..end])?;
                out.push(Event { type_names, path: path.clone(), filename });
            }
            if self.buffer.len() < 16 {
                return Ok(out);
            }
        }
    }
}

impl Drop for Inotify {
    /// `Inotify.__del__`.
    fn drop(&mut self) {
        debug("Cleaning-up inotify.");
    }
}

/// `InotifyEventHelper(dir_path, file_names)`: the directory watched for
/// files appearing, going and being written.
pub struct InotifyEventHelper {
    notifier: Inotify,
    dir_path: String,
    file_names: Vec<String>,
}

impl InotifyEventHelper {
    pub fn new(dir_path: &str, file_names: &[String]) -> PyResult<InotifyEventHelper> {
        let mut notifier = Inotify::new()?;
        let mask = libc::IN_CREATE | libc::IN_DELETE | libc::IN_MODIFY | libc::IN_MOVED_TO | libc::IN_MOVED_FROM | libc::IN_CLOSE_WRITE;
        notifier.add_watch(dir_path, mask)?;
        Ok(InotifyEventHelper { notifier, dir_path: dir_path.to_string(), file_names: file_names.to_vec() })
    }

    /// `wait_for_events(timeout_ms)`: `event_gen` blocks a second at a time
    /// and stops once `timeout_ms` has passed since the last event, so it
    /// takes at least a second.  The paths come back in the order they were
    /// first seen (Python's set of them iterates in hash order).
    pub fn wait_for_events(&mut self, timeout_ms: f64) -> PyResult<Vec<String>> {
        let mut changed: Vec<String> = Vec::new();
        let (dir_path, file_names) = (&self.dir_path, &self.file_names);
        self.notifier.event_gen(timeout_ms / 1000.0, |e| {
            if e.path == *dir_path && file_names.contains(&e.filename) {
                let p = io::join(&e.path, &e.filename);
                if !changed.contains(&p) {
                    changed.push(p);
                }
            }
            Ok(None::<()>)
        })?;
        Ok(changed)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use std::time::Duration;

    /// Writes `value` to the real `path` after `delay_ms`, from another thread.
    fn write_later(path: std::path::PathBuf, value: &'static str, delay_ms: u64) -> std::thread::JoinHandle<()> {
        std::thread::spawn(move || {
            std::thread::sleep(Duration::from_millis(delay_ms));
            std::fs::write(path, value).unwrap();
        })
    }

    #[test]
    fn a_watched_file_must_exist() {
        let _root = Root::new();
        let e = InotifyHelper::new("/run/hw-management/system/dpu1_ready").err().unwrap();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("FileNotFoundError", "[Errno 2] No such file or directory: '/run/hw-management/system/dpu1_ready'".to_string())
        );
    }

    #[test]
    fn wait_watch_answers_the_value_once_a_write_leaves_it_or_none() {
        let root = Root::new();
        let path = "/run/hw-management/system/dpu1_ready";
        root.write(path, "0\n");
        let helper = InotifyHelper::new(path).unwrap();
        // A write of the expected value ends the wait early.
        let begin = Instant::now();
        let writer = write_later(root.real(path), "1\n", 100);
        assert_eq!(helper.wait_watch(5.0, 1).unwrap(), Some(1));
        assert!(begin.elapsed() < Duration::from_secs(3));
        writer.join().unwrap();
        // A write of another value does not; the last check finds it.
        let writer = write_later(root.real(path), "0\n", 50);
        assert_eq!(helper.wait_watch(0.3, 1).unwrap(), None);
        writer.join().unwrap();
        // Already there and never written: the last check.
        root.write(path, "1\n");
        assert_eq!(helper.wait_watch(0.1, 1).unwrap(), Some(1));
        // Gone by then: its read raises.
        std::fs::remove_file(root.real(path)).unwrap();
        assert_eq!(helper.wait_watch(0.1, 1).unwrap_err().exc.name(), "FileNotFoundError");
    }

    #[test]
    fn event_names_follow_inotify_s_mask_lookup() {
        assert_eq!(event_names(0x8), ["IN_CLOSE_WRITE"]);
        assert_eq!(event_names(0x4000_0100), ["IN_CREATE", "IN_ISDIR"]);
        assert_eq!(event_names(0o4000), ["IN_MOVE_SELF"]);
        assert!(event_names(0).is_empty());
        assert_eq!(names_repr(&event_names(0x2 | 0x8)), "['IN_MODIFY', 'IN_CLOSE_WRITE']");
        assert!(elapsed_repr(Instant::now()).parse::<f64>().is_ok());
    }

    #[test]
    fn a_missing_directory_is_an_inotify_error() {
        let _root = Root::new();
        let e = InotifyEventHelper::new("/var/run/hw-management/config", &["asic1_ready".to_string()]).err().unwrap();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("InotifyError", "Call failed (should not be -1): (-1) ERRNO=(2) [No such file or directory]".to_string())
        );
    }

    #[test]
    fn the_watched_names_written_in_the_directory_come_back_once() {
        let root = Root::new();
        let dir = "/var/run/hw-management/config";
        root.write(&format!("{dir}/asic1_ready"), "1\n");
        let names = vec!["asic1_ready".to_string(), "asic2_ready".to_string()];
        let mut helper = InotifyEventHelper::new(dir, &names).unwrap();
        // Nothing written: a second's wait, nothing back.
        let begin = Instant::now();
        assert!(helper.wait_for_events(100.0).unwrap().is_empty());
        assert!(begin.elapsed() >= Duration::from_millis(900));
        // asic2 written twice and an unwatched file once: asic2, once.
        let real = root.real(dir);
        let writer = std::thread::spawn(move || {
            std::thread::sleep(Duration::from_millis(50));
            std::fs::write(real.join("other_ready"), "1\n").unwrap();
            std::fs::write(real.join("asic2_ready"), "0\n").unwrap();
            std::fs::write(real.join("asic2_ready"), "1\n").unwrap();
        });
        assert_eq!(helper.wait_for_events(100.0).unwrap(), [format!("{dir}/asic2_ready")]);
        writer.join().unwrap();
        drop(helper);
    }
}
