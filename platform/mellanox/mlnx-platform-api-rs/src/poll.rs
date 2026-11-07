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



//! The pieces of Python's `select.poll` and `open()` the SFP change events
//! are made of.
//!
//! `select.poll()` keeps its fds in a dict and builds the pollfd array from
//! it, so simultaneous events come back in registration order -- a
//! re-registered fd keeps its place, an unregistered one leaves.  An fd here
//! is a sysfs attribute opened in text mode, read whole after a seek to 0,
//! as SDK attributes signal with POLLPRI.

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::os::fd::{AsRawFd, RawFd};

use platform_base::io;
use platform_base::py::{codec, PyErr, PyResult};

/// `select.POLLERR | select.POLLPRI`.
pub const POLLERR_PRI: i16 = libc::POLLERR | libc::POLLPRI;

/// `open(path)`: a text-mode file.
pub struct SysFile {
    pub name: String,
    file: File,
}

impl SysFile {
    /// `open(path)`, with the exception Python raises.
    pub fn open(path: &str) -> PyResult<SysFile> {
        let file = File::open(io::real(path)).map_err(|e| PyErr::from_io(&e, Some(path)))?;
        Ok(SysFile { name: path.to_string(), file })
    }

    pub fn fileno(&self) -> RawFd {
        self.file.as_raw_fd()
    }

    /// `f.seek(0)`.
    pub fn seek0(&mut self) -> PyResult<()> {
        self.file.seek(SeekFrom::Start(0)).map(|_| ()).map_err(|e| PyErr::from_io(&e, None))
    }

    /// `f.read()`: the rest of the file, decoded by the locale's codec.
    pub fn read(&mut self) -> PyResult<String> {
        let mut b = Vec::new();
        self.file.read_to_end(&mut b).map_err(|e| PyErr::from_io(&e, None))?;
        codec::decode_text(&b)
    }
}

/// `select.poll()`.
#[derive(Default)]
pub struct PollSet {
    /// The registration dict: fd and event mask, in insertion order.
    fds: Vec<(RawFd, i16)>,
}

impl PollSet {
    /// `p.register(fd, mask)`: a known fd keeps its place.
    pub fn register(&mut self, fd: RawFd, mask: i16) {
        match self.fds.iter_mut().find(|(f, _)| *f == fd) {
            Some(e) => e.1 = mask,
            None => self.fds.push((fd, mask)),
        }
    }

    /// `p.unregister(fd)`: `KeyError` for an fd that is not there.
    pub fn unregister(&mut self, fd: RawFd) -> PyResult<()> {
        match self.fds.iter().position(|(f, _)| *f == fd) {
            Some(i) => {
                self.fds.remove(i);
                Ok(())
            }
            None => Err(PyErr::key_error(platform_base::py::PyVal::Int(i128::from(fd)))),
        }
    }

    /// `p.poll(timeout_ms)`: `(fd, revents)` for each fd with events, in
    /// registration order.  The timeout is rounded up to a millisecond.
    pub fn poll(&self, timeout_ms: f64) -> PyResult<Vec<(RawFd, i16)>> {
        let mut pfds: Vec<libc::pollfd> =
            self.fds.iter().map(|&(fd, events)| libc::pollfd { fd, events, revents: 0 }).collect();
        let ms = if timeout_ms < 0.0 { -1 } else { timeout_ms.ceil().min(i32::MAX as f64) as i32 };
        // A test's events, which a plain file never raises, without waiting.
        #[cfg(test)]
        let injected = inject::take(&self.fds);
        #[cfg(test)]
        let ms = if injected.is_empty() { ms } else { 0 };
        loop {
            // SAFETY: pfds is a valid array of pollfd for its length.
            let rc = unsafe { libc::poll(pfds.as_mut_ptr(), pfds.len() as libc::nfds_t, ms) };
            if rc >= 0 {
                break;
            }
            let e = std::io::Error::last_os_error();
            // PEP 475: EINTR is retried (with the timeout recomputed, which
            // a sub-second wait does not notice).
            if e.raw_os_error() != Some(libc::EINTR) {
                return Err(PyErr::from_io(&e, None));
            }
        }
        #[cfg(test)]
        for (fd, revents) in injected {
            if let Some(p) = pfds.iter_mut().find(|p| p.fd == fd) {
                p.revents |= revents;
            }
        }
        Ok(pfds.iter().filter(|p| p.revents != 0).map(|p| (p.fd, p.revents)).collect())
    }
}

/// The SDK's POLLPRI, for tests: a regular file standing in for a sysfs
/// attribute never raises it, so a test queues one for a path and the next
/// poll that has the path open reports it (as the sandbox's LD_PRELOAD shim
/// did for whole daemons).
#[cfg(test)]
pub mod inject {
    use std::os::fd::RawFd;
    use std::path::PathBuf;
    use std::sync::Mutex;

    static PENDING: Mutex<Vec<(PathBuf, i16)>> = Mutex::new(Vec::new());

    fn pending() -> std::sync::MutexGuard<'static, Vec<(PathBuf, i16)>> {
        PENDING.lock().unwrap_or_else(|e| e.into_inner())
    }

    /// The next poll of `path` (as the code names it) reports `revents`.
    pub fn event(path: &str, revents: i16) {
        let real = super::io::real(path);
        let real = std::fs::canonicalize(&real).unwrap_or(real);
        pending().push((real, revents));
    }

    /// Forget events no poll took.
    pub fn clear() {
        pending().clear();
    }

    /// The queued events of the files `fds` have open, taken off the queue.
    pub(super) fn take(fds: &[(RawFd, i16)]) -> Vec<(RawFd, i16)> {
        let mut pending = pending();
        let mut out = Vec::new();
        for &(fd, _) in fds {
            let Ok(target) = std::fs::read_link(format!("/proc/self/fd/{fd}")) else {
                continue;
            };
            if let Some(i) = pending.iter().position(|(p, _)| *p == target) {
                out.push((fd, pending.remove(i).1));
            }
        }
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;
    use std::os::fd::{FromRawFd, OwnedFd};

    fn pipe() -> (OwnedFd, OwnedFd) {
        let mut fds = [0; 2];
        // SAFETY: a two-int array for pipe() to fill.
        assert_eq!(unsafe { libc::pipe(fds.as_mut_ptr()) }, 0);
        // SAFETY: two fresh descriptors, each owned once.
        unsafe { (OwnedFd::from_raw_fd(fds[0]), OwnedFd::from_raw_fd(fds[1])) }
    }

    #[test]
    fn a_sysfs_file_opens_seeks_and_reads_as_python_s_open_does() {
        let root = Root::new();
        let e = SysFile::open("/sys/module/sx_core/asic0/module0/present").err().unwrap();
        assert_eq!(
            (e.exc.name(), e.str()),
            ("FileNotFoundError", "[Errno 2] No such file or directory: '/sys/module/sx_core/asic0/module0/present'".to_string())
        );
        root.write("/sys/module/sx_core/asic0/module0/present", "1\n");
        let mut f = SysFile::open("/sys/module/sx_core/asic0/module0/present").unwrap();
        assert_eq!(f.name, "/sys/module/sx_core/asic0/module0/present");
        assert!(f.fileno() > 2);
        assert_eq!(f.read().unwrap(), "1\n");
        assert_eq!(f.read().unwrap(), "");
        f.seek0().unwrap();
        assert_eq!(f.read().unwrap(), "1\n");
        // A directory opens, and reading it is Python's IsADirectoryError.
        std::fs::create_dir_all(root.real("/sys/module/sx_core/asic0/module1/present")).unwrap();
        let mut d = SysFile::open("/sys/module/sx_core/asic0/module1/present").unwrap();
        assert_eq!(d.read().unwrap_err().exc.name(), "IsADirectoryError");
        // A pipe cannot seek: ESPIPE.
        std::fs::create_dir_all(root.real("/tmp")).unwrap();
        let fifo = std::ffi::CString::new(root.real("/tmp/fifo").to_string_lossy().into_owned()).unwrap();
        // SAFETY: a NUL-terminated path.
        assert_eq!(unsafe { libc::mkfifo(fifo.as_ptr(), 0o600) }, 0);
        let _writer = std::fs::OpenOptions::new().read(true).write(true).open(root.real("/tmp/fifo")).unwrap();
        let mut p = SysFile::open("/tmp/fifo").unwrap();
        assert_eq!(p.seek0().unwrap_err().str(), "[Errno 29] Illegal seek");
    }

    #[test]
    fn a_poll_set_keeps_registration_order_and_reports_in_it() {
        let (r1, w1) = pipe();
        let (r2, w2) = pipe();
        let (r1, r2) = (r1.as_raw_fd(), r2.as_raw_fd());
        let mut p = PollSet::default();
        p.register(r2, libc::POLLIN);
        p.register(r1, libc::POLLIN);
        // Registering again keeps the place, with the new mask.
        p.register(r2, libc::POLLIN | libc::POLLPRI);
        assert_eq!(p.fds, [(r2, libc::POLLIN | libc::POLLPRI), (r1, libc::POLLIN)]);
        assert!(p.poll(0.0).unwrap().is_empty());
        // SAFETY: a byte into each pipe's write end.
        unsafe {
            libc::write(w1.as_raw_fd(), b"x".as_ptr().cast(), 1);
            libc::write(w2.as_raw_fd(), b"x".as_ptr().cast(), 1);
        }
        assert_eq!(p.poll(-1.0).unwrap(), [(r2, libc::POLLIN), (r1, libc::POLLIN)]);
        assert_eq!(p.poll(0.4).unwrap().len(), 2);
        p.unregister(r2).unwrap();
        assert_eq!(p.poll(0.0).unwrap(), [(r1, libc::POLLIN)]);
        assert_eq!(p.unregister(r2).unwrap_err().str(), r2.to_string());
        assert_eq!(p.unregister(r2).unwrap_err().exc.name(), "KeyError");
    }

    #[test]
    fn a_regular_file_raises_no_priority_event_unless_a_test_injects_one() {
        let root = Root::new();
        root.write("/sys/module/sx_core/asic0/module3/present", "0\n");
        let f = SysFile::open("/sys/module/sx_core/asic0/module3/present").unwrap();
        let mut p = PollSet::default();
        p.register(f.fileno(), POLLERR_PRI);
        assert!(p.poll(1.0).unwrap().is_empty());
        inject::event("/sys/module/sx_core/asic0/module3/present", libc::POLLPRI);
        assert_eq!(p.poll(1000.0).unwrap(), [(f.fileno(), libc::POLLPRI)]);
        assert!(p.poll(0.0).unwrap().is_empty());
        inject::event("/sys/module/sx_core/asic0/module3/present", libc::POLLPRI);
        inject::clear();
        assert!(p.poll(0.0).unwrap().is_empty());
        // An fd that is not open has no path to match: poll says POLLNVAL.
        let mut q = PollSet::default();
        q.register(1_000_000, POLLERR_PRI);
        inject::event("/sys/module/sx_core/asic0/module3/present", libc::POLLPRI);
        assert_eq!(q.poll(0.0).unwrap(), [(1_000_000, libc::POLLNVAL)]);
        inject::clear();
    }
}
