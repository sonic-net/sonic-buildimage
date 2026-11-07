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



//! `sonic_platform/component.py`: the firmware components.
//!
//! In a container the chassis builds none of the host's components (ONIE,
//! SSD, BIOS, CPLDs); the BMC's is the one that appears there, once the BMC
//! has been asked for.  On the host, [`host_component_list`] builds the rest.
//!
//! Firmware moves through the tools the Python runs -- the ONIE and SSD
//! update scripts, cpldupdate, mst, dmidecode, tar -- with the same argv, the
//! same terminal (their output is the user's), and the same exceptions when
//! they fail; and through ONIE's own file system, mounted for the duration
//! of a version read.  The CPLD images come in MPFA archives: a tarball with
//! a `metadata.ini` that configparser reads, ported here with its errors.

use std::collections::HashSet;
use std::io::{Read, Write};
use std::os::fd::{FromRawFd, OwnedFd};
use std::os::unix::fs::{FileTypeExt, MetadataExt};
use std::os::unix::process::CommandExt;
use std::process::{Command, Stdio};
use std::sync::{Arc, Mutex};
use std::thread::sleep;
use std::time::Duration;

use platform_base::py::{codec, py_int, splitlines, str_repr, strip, Exc, PyErr, PyResult, PyVal};
use platform_base::redfish_client::is_exception;
use platform_base::{device_info, io, subprocess, BaseClass, PyObject};
use regex::Regex;

use crate::bmc::{self, Bmc};
use crate::device_data;
use crate::dpuctlplat::BootProg;
use crate::utils::{self, Log};

pub const FW_AUTO_INSTALLED: i128 = 1;
pub const FW_AUTO_UPDATED: i128 = 2;
pub const FW_AUTO_SCHEDULED: i128 = 3;
pub const FW_AUTO_ERR_BOOT_TYPE: i128 = -1;
pub const FW_AUTO_ERR_IMAGE: i128 = -2;
pub const FW_AUTO_ERR_UNKNOWN: i128 = -3;

/// `print(msg)`.
pub fn print(msg: &str) {
    let _ = writeln!(std::io::stdout(), "{msg}");
}

/// `os.path.splitext(path)[1]`: from the last dot of the last component,
/// unless that component is all dots up to it.
pub fn splitext_ext(path: &str) -> &str {
    let base_start = path.rfind('/').map_or(0, |i| i + 1);
    let base = &path[base_start..];
    match base.rfind('.') {
        Some(i) if base[..i].chars().any(|c| c != '.') => &base[i..],
        _ => "",
    }
}

/// `str(list_of_str)`.
fn list_str(items: &[&str]) -> String {
    PyVal::List(items.iter().map(|s| PyVal::str(*s)).collect()).py_repr()
}

/// `Component._check_file_validity(image_path)`.
fn check_file_validity(image_path: &str, name: &str, image_ext_name: Option<&[&str]>) -> bool {
    if !io::is_file(image_path) {
        print(&format!("ERROR: File {image_path} doesn't exist or is not a file"));
        return false;
    }
    if let Some(exts) = image_ext_name {
        if !exts.contains(&splitext_ext(image_path)) {
            print(&format!(
                "ERROR: Extend name of file {image_path} is wrong. Image for {name} should have extend name {}",
                list_str(exts)
            ));
            return false;
        }
    }
    true
}

/// `ComponentBMC`.
pub struct ComponentBmc {
    bmc: Option<Arc<Bmc>>,
}

const BMC_COMPONENT_NAME: &str = "BMC";
const BMC_COMPONENT_DESCRIPTION: &str = "BMC - Baseboard Management Controller";
const BMC_FIRMWARE_EXTENSION: [&str; 1] = [".fwpkg"];
const BMC_FW_UPDATE: &str = "/usr/bin/bmc_fw_update.py";

impl ComponentBmc {
    /// `ComponentBMC()`: holds the BMC singleton, or None.
    pub fn new() -> PyResult<ComponentBmc> {
        Ok(ComponentBmc { bmc: bmc::get_instance()? })
    }

    /// `install_firmware(image_path)`: hw-management's updater, in its own
    /// session, talking to the terminal directly.
    fn install(&self, image_path: &str) -> bool {
        if !check_file_validity(image_path, BMC_COMPONENT_NAME, Some(&BMC_FIRMWARE_EXTENSION)) {
            print(&format!("Invalid firmware image path: {image_path}"));
            return false;
        }
        print(&format!("Starting BMC firmware update, path={image_path}"));
        match check_call(&[BMC_FW_UPDATE, image_path], true) {
            Ok(()) => {
                print("Successfully updated BMC firmware, and restarted BMC");
                true
            }
            Err(e) if is_called_process_error(&e) => {
                print("Failed to update BMC firmware");
                false
            }
            Err(e) => {
                utils::log_error(&format!("Exception occurred during BMC firmware update: {}", e.str()));
                print(&format!("Exception occurred during BMC firmware update: {}", e.str()));
                false
            }
        }
    }
}

impl PyObject for ComponentBmc {
    fn py_class(&self) -> &'static str {
        "ComponentBMC"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ComponentBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(BMC_COMPONENT_NAME))
    }
    fn get_description(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(BMC_COMPONENT_DESCRIPTION))
    }
    fn get_firmware_version(&self) -> PyResult<PyVal> {
        match &self.bmc {
            None => Ok(PyVal::str("N/A")),
            Some(b) => b.get_version(),
        }
    }
    fn get_available_firmware_version(&self, _image_path: &str) -> PyResult<PyVal> {
        Err(PyErr::not_implemented_msg("BMC component doesn't support available firmware version query"))
    }
    fn get_firmware_update_notification(&self, _image_path: &str) -> PyResult<PyVal> {
        Ok(PyVal::str("BMC will be automatically restarted to complete BMC firmware update"))
    }
    /// The switch's boot type is beside the point -- the BMC restarts --
    /// but anything but a cold boot is still skipped.
    fn auto_update_firmware(&self, image_path: &str, boot_type: &str) -> PyResult<PyVal> {
        if !io::exists(image_path) {
            return Ok(PyVal::Int(FW_AUTO_ERR_IMAGE));
        }
        if boot_type != "cold" {
            return Ok(PyVal::Int(FW_AUTO_ERR_BOOT_TYPE));
        }
        if !self.install(image_path) {
            return Ok(PyVal::Int(FW_AUTO_ERR_UNKNOWN));
        }
        Ok(PyVal::Int(FW_AUTO_INSTALLED))
    }
    fn install_firmware(&self, image_path: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.install(image_path)))
    }
    fn update_firmware(&self, image_path: &str) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.install(image_path)))
    }
}

/// `BMC._get_component_list()`.
pub fn bmc_component_list() -> PyResult<Vec<PyVal>> {
    crate::imports::component();
    Ok(vec![PyVal::Obj(Arc::new(ComponentBmc::new()?))])
}

// -- os and subprocess, as component.py reaches them ------------------------

/// What `Popen()` refuses before anything runs: an argument the locale's
/// codec cannot encode, or one with a NUL in it.
fn argv_check(argv: &[&str]) -> PyResult<()> {
    for a in argv {
        codec::encode_check(codec::locale(), a)?;
        if a.contains('\0') {
            return Err(PyErr::value_error("embedded null byte"));
        }
    }
    Ok(())
}

fn is_called_process_error(e: &PyErr) -> bool {
    e.exc.name() == "CalledProcessError"
}

/// `except RuntimeError`: it and its subclasses.
fn is_runtime_error(e: &PyErr) -> bool {
    matches!(e.exc, Exc::RuntimeError | Exc::NotImplementedError)
}

/// The program `Popen` runs for `argv0`: `argv0` itself -- a bare name
/// looked for on PATH.  In this crate's tests, under an io root, it is
/// looked for beneath the root instead -- an absolute path there, a bare
/// name in its /usr/bin -- so the test's stand-ins run, and nothing real.
pub(crate) fn program_path(argv0: &str) -> std::path::PathBuf {
    #[cfg(test)]
    {
        if io::real("/") != std::path::Path::new("/") {
            return if argv0.starts_with('/') { io::real(argv0) } else { io::real(&format!("/usr/bin/{argv0}")) };
        }
    }
    argv0.into()
}

fn program(argv0: &str) -> Command {
    Command::new(program_path(argv0))
}

/// `subprocess.check_call(argv)`: the command talks to the terminal; with
/// `start_new_session`, from a session of its own.
fn check_call(argv: &[&str], new_session: bool) -> PyResult<()> {
    argv_check(argv)?;
    let mut cmd = program(argv[0]);
    cmd.args(&argv[1..]);
    if new_session {
        // SAFETY: setsid() between fork and exec, as start_new_session.
        unsafe {
            cmd.pre_exec(|| {
                libc::setsid();
                Ok(())
            });
        }
    }
    let st = cmd.status().map_err(|e| PyErr::from_io(&e, Some(argv[0])))?;
    match subprocess::returncode(st) {
        0 => Ok(()),
        rc => Err(subprocess::called_process_error(argv, rc)),
    }
}

/// `subprocess.check_output(argv, stderr=STDOUT, universal_newlines=True)`:
/// both streams through one pipe, decoded before the exit status is looked
/// at.
fn check_output_merged(argv: &[&str]) -> PyResult<String> {
    argv_check(argv)?;
    let mut fds = [0; 2];
    // SAFETY: a pipe whose two ends become owned descriptors at once.
    if unsafe { libc::pipe2(fds.as_mut_ptr(), libc::O_CLOEXEC) } != 0 {
        return Err(PyErr::from_io(&std::io::Error::last_os_error(), None));
    }
    let (r, w) = unsafe { (OwnedFd::from_raw_fd(fds[0]), OwnedFd::from_raw_fd(fds[1])) };
    let mut child = {
        let w2 = w.try_clone().map_err(|e| PyErr::from_io(&e, None))?;
        let mut cmd = program(argv[0]);
        cmd.args(&argv[1..]).stdout(Stdio::from(w)).stderr(Stdio::from(w2));
        cmd.spawn().map_err(|e| PyErr::from_io(&e, Some(argv[0])))?
        // The Command, and the write ends with it, go here.
    };
    let mut out = Vec::new();
    let _ = std::fs::File::from(r).read_to_end(&mut out);
    let st = child.wait().map_err(|e| PyErr::from_io(&e, None))?;
    let text = subprocess::text(&out)?;
    match subprocess::returncode(st) {
        0 => Ok(text),
        rc => Err(subprocess::called_process_error(argv, rc)),
    }
}

/// `subprocess.check_output(argv)`: bytes; stdin and stderr the
/// terminal's.
fn check_output_bytes(argv: &[&str]) -> PyResult<Vec<u8>> {
    argv_check(argv)?;
    let out = program(argv[0])
        .args(&argv[1..])
        .stdin(Stdio::inherit())
        .stderr(Stdio::inherit())
        .output()
        .map_err(|e| PyErr::from_io(&e, Some(argv[0])))?;
    match subprocess::returncode(out.status) {
        0 => Ok(out.stdout),
        rc => Err(subprocess::called_process_error(argv, rc)),
    }
}

/// `subprocess.run(argv, stdout=PIPE, stderr=DEVNULL, timeout=timeout)`'s
/// stdout, whatever the exit status.  Past the timeout -- counted until the
/// command has closed its stdout and exited -- it is killed and reaped, and
/// `TimeoutExpired` is raised.
fn run_stdout(argv: &[&str], timeout: Duration) -> PyResult<Vec<u8>> {
    argv_check(argv)?;
    let mut child = program(argv[0])
        .args(&argv[1..])
        .stdin(Stdio::inherit())
        .stdout(Stdio::piped())
        .stderr(Stdio::null())
        .spawn()
        .map_err(|e| PyErr::from_io(&e, Some(argv[0])))?;
    let deadline = std::time::Instant::now() + timeout;
    let mut out = Vec::new();
    let mut pipe = child.stdout.take();
    let done = loop {
        let left = deadline.saturating_duration_since(std::time::Instant::now());
        if left.is_zero() {
            break false;
        }
        let Some(p) = pipe.as_mut() else {
            match child.try_wait() {
                Ok(Some(_)) => break true,
                Ok(None) => sleep(Duration::from_millis(5)),
                Err(e) => return Err(PyErr::from_io(&e, None)),
            }
            continue;
        };
        let mut pfd = libc::pollfd { fd: std::os::fd::AsRawFd::as_raw_fd(p), events: libc::POLLIN, revents: 0 };
        // SAFETY: one valid pollfd.
        if unsafe { libc::poll(&mut pfd, 1, left.as_millis().clamp(1, i32::MAX as u128) as i32) } > 0 {
            let mut buf = [0u8; 4096];
            match p.read(&mut buf) {
                Ok(0) | Err(_) => pipe = None,
                Ok(n) => out.extend_from_slice(&buf[..n]),
            }
        }
    };
    if done {
        return Ok(out);
    }
    let _ = child.kill();
    let _ = child.wait();
    Err(PyErr::other(
        "TimeoutExpired",
        format!("Command '{}' timed out after {} seconds", list_str(argv), timeout.as_secs()),
    ))
}

/// `sonic_py_common.general.check_output_pipe(cmd0, *args)`: a shell-less
/// pipeline; the last command's output, decoded, unless a command -- the
/// first that failed, in order -- exited non-zero.
fn check_output_pipe(cmds: &[&[&str]]) -> PyResult<String> {
    let mut children: Vec<std::process::Child> = Vec::new();
    for (i, argv) in cmds.iter().enumerate() {
        argv_check(argv)?;
        let mut cmd = program(argv[0]);
        cmd.args(&argv[1..]).stdout(Stdio::piped());
        if i > 0 {
            if let Some(prev) = children.last_mut().and_then(|c| c.stdout.take()) {
                cmd.stdin(Stdio::from(prev));
            }
        }
        children.push(cmd.spawn().map_err(|e| PyErr::from_io(&e, Some(argv[0])))?);
    }
    let mut out = Vec::new();
    if let Some(mut so) = children.last_mut().and_then(|c| c.stdout.take()) {
        let _ = so.read_to_end(&mut out);
    }
    let text = subprocess::text(&out)?;
    for (child, argv) in children.iter_mut().zip(cmds) {
        let st = child.wait().map_err(|e| PyErr::from_io(&e, None))?;
        let rc = subprocess::returncode(st);
        if rc != 0 {
            return Err(subprocess::called_process_error(argv, rc));
        }
    }
    Ok(text)
}

/// An `OSError` naming two files, as `os.symlink` raises it.
fn os_error2(e: &std::io::Error, a: &str, b: &str) -> PyErr {
    let base = PyErr::from_io(e, None);
    let msg = format!("{}: {} -> {}", base.str(), str_repr(a), str_repr(b));
    PyErr::new(base.exc.clone(), vec![PyVal::Str(msg)])
}

fn lexists(path: &str) -> bool {
    std::fs::symlink_metadata(io::real(path)).is_ok()
}

fn mkdir(path: &str, mode: u32) -> PyResult<()> {
    use std::os::unix::fs::DirBuilderExt;
    std::fs::DirBuilder::new().mode(mode).create(io::real(path)).map_err(|e| PyErr::from_io(&e, Some(path)))
}

/// `os.path.ismount(path)`: on another device than its parent, or its own
/// parent.
fn ismount(path: &str) -> bool {
    let Ok(s1) = std::fs::symlink_metadata(io::real(path)) else {
        return false;
    };
    if s1.file_type().is_symlink() {
        return false;
    }
    let parent = io::join(path, "..");
    let s2 = match std::fs::symlink_metadata(io::real(&parent)) {
        Ok(s) => s,
        Err(_) => match std::fs::symlink_metadata(io::real(&io::realpath(&parent))) {
            Ok(s) => s,
            Err(_) => return false,
        },
    };
    s1.dev() != s2.dev() || s1.ino() == s2.ino()
}

fn basename(path: &str) -> &str {
    path.rfind('/').map_or(path, |i| &path[i + 1..])
}

/// `shutil.copyfile(src, dst)`.
fn copyfile(src: &str, dst: &str) -> PyResult<()> {
    if let (Ok(a), Ok(b)) = (std::fs::metadata(io::real(src)), std::fs::metadata(io::real(dst))) {
        if a.dev() == b.dev() && a.ino() == b.ino() {
            return Err(PyErr::other(
                "SameFileError",
                format!("{} and {} are the same file", str_repr(src), str_repr(dst)),
            ));
        }
    }
    for f in [src, dst] {
        if std::fs::metadata(io::real(f)).is_ok_and(|m| m.file_type().is_fifo()) {
            return Err(PyErr::other("SpecialFileError", format!("`{f}` is a named pipe")));
        }
    }
    let mut fsrc = std::fs::File::open(io::real(src)).map_err(|e| PyErr::from_io(&e, Some(src)))?;
    if fsrc.metadata().is_ok_and(|m| m.is_dir()) {
        return Err(PyErr::os_error(libc::EISDIR, Some(src)));
    }
    let mut fdst = match std::fs::File::create(io::real(dst)) {
        Ok(f) => f,
        Err(e) if e.raw_os_error() == Some(libc::EISDIR) && !io::exists(dst) => {
            return Err(PyErr::new(
                Exc::OSError("FileNotFoundError"),
                vec![PyVal::Str(format!("Directory does not exist: {dst}"))],
            ))
        }
        Err(e) => return Err(PyErr::from_io(&e, Some(dst))),
    };
    std::io::copy(&mut fsrc, &mut fdst).map_err(|e| PyErr::from_io(&e, None))?;
    Ok(())
}

/// `tempfile._RandomNameSequence`: eight of these, at random.
fn random_name() -> String {
    const CHARS: &[u8] = b"abcdefghijklmnopqrstuvwxyz0123456789_";
    let mut buf = [0u8; 8];
    // SAFETY: getrandom into a buffer of ours.
    let n = unsafe { libc::getrandom(buf.as_mut_ptr().cast(), buf.len(), 0) };
    if n != buf.len() as isize {
        let t = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map_or(0, |d| d.as_nanos() as u64);
        buf = (t ^ u64::from(std::process::id())).to_le_bytes();
    }
    buf.iter().map(|b| CHARS[usize::from(*b) % CHARS.len()] as char).collect()
}

/// `posixpath.normpath` of an absolute path.
fn normpath(path: &str) -> String {
    let mut parts: Vec<&str> = Vec::new();
    for p in path.split('/') {
        match p {
            "" | "." => {}
            ".." => {
                parts.pop();
            }
            p => parts.push(p),
        }
    }
    format!("/{}", parts.join("/"))
}

/// `os.path.abspath(path)`.
fn abspath(path: &str) -> String {
    if path.starts_with('/') {
        normpath(path)
    } else {
        let cwd = std::env::current_dir().map(|p| p.to_string_lossy().into_owned()).unwrap_or_default();
        normpath(&format!("{cwd}/{path}"))
    }
}

/// `tempfile.gettempdir()`: the first candidate a file can be made in,
/// found once.
fn gettempdir() -> PyResult<String> {
    static DIR: Mutex<Option<String>> = Mutex::new(None);
    if let Some(d) = DIR.lock().ok().and_then(|d| d.clone()) {
        return Ok(d);
    }
    let mut dirlist: Vec<String> = Vec::new();
    for env in ["TMPDIR", "TEMP", "TMP"] {
        if let Ok(v) = std::env::var(env) {
            if !v.is_empty() {
                dirlist.push(v);
            }
        }
    }
    dirlist.extend(["/tmp", "/var/tmp", "/usr/tmp"].map(String::from));
    dirlist.push(std::env::current_dir().map(|p| p.to_string_lossy().into_owned()).unwrap_or_else(|_| ".".into()));
    for dir in &dirlist {
        let dir = if dir == "." { dir.clone() } else { abspath(dir) };
        for _ in 0..100 {
            let filename = io::join(&dir, &random_name());
            match std::fs::OpenOptions::new().read(true).write(true).create_new(true).open(io::real(&filename)) {
                Ok(mut f) => {
                    let _ = f.write_all(b"blat");
                    drop(f);
                    let _ = std::fs::remove_file(io::real(&filename));
                    if let Ok(mut d) = DIR.lock() {
                        *d = Some(dir.clone());
                    }
                    return Ok(dir);
                }
                Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => continue,
                Err(_) => break,
            }
        }
    }
    Err(PyErr::new(
        Exc::OSError("FileNotFoundError"),
        vec![
            PyVal::Int(i128::from(libc::ENOENT)),
            PyVal::Str(format!(
                "No usable temporary directory found in {}",
                PyVal::List(dirlist.iter().map(|d| PyVal::str(d.as_str())).collect()).py_repr()
            )),
        ],
    ))
}

/// `tempfile.mkdtemp(prefix=prefix)`.
fn mkdtemp(prefix: &str) -> PyResult<String> {
    let dir = gettempdir()?;
    for _ in 0..10000 {
        let file = io::join(&dir, &format!("{prefix}{}", random_name()));
        match mkdir(&file, 0o700) {
            Ok(()) => return Ok(abspath(&file)),
            Err(e) if e.exc.name() == "FileExistsError" => continue,
            Err(e) => return Err(e),
        }
    }
    Err(PyErr::new(
        Exc::OSError("FileExistsError"),
        vec![PyVal::Int(i128::from(libc::EEXIST)), PyVal::str("No usable temporary directory name found")],
    ))
}

/// `str.zfill(width)`: zeros after any sign.
fn zfill(s: &str, width: usize) -> String {
    let n = s.chars().count();
    if n >= width {
        return s.to_string();
    }
    let fill = "0".repeat(width - n);
    match s.chars().next() {
        Some(c @ ('+' | '-')) => format!("{c}{fill}{}", &s[1..]),
        _ => format!("{fill}{s}"),
    }
}

/// `logger.log_notice(msg, also_print_to_console=True)`: syslog, then the
/// terminal -- NOTICE passes the logger's threshold, so both, always.
fn notice_print(msg: &str) {
    utils::log_notice(msg);
    print(msg);
}

fn warning_print(msg: &str) {
    utils::log_warning(msg);
    print(msg);
}

/// `Component._read_generic_file(filename, len, ignore_errors)`: up to
/// `len` characters; an unreadable file is None if errors are ignored, a
/// RuntimeError otherwise.
fn read_generic_file(filename: &str, len: usize, ignore_errors: bool) -> PyResult<Option<String>> {
    match io::read_text(filename) {
        Ok(t) => Ok(Some(t.chars().take(len).collect())),
        Err(e) if e.exc.is_os_error() => {
            if ignore_errors {
                Ok(None)
            } else {
                Err(PyErr::runtime_error(format!("Failed to read file {filename} due to {}", e.repr())))
            }
        }
        Err(e) => Err(e),
    }
}

// -- configparser ------------------------------------------------------------

/// `configparser.ConfigParser()` after `read(path)`: the defaults, strict,
/// `#` and `;` comments on lines of their own, `=` and `:` as delimiters,
/// option names lower-cased, `%` interpolation on `get`.
struct Ini {
    defaults: Vec<(String, String)>,
    sections: Vec<(String, Vec<(String, String)>)>,
}

const DEFAULTSECT: &str = "DEFAULT";

/// A section's options while being read: each value's lines.
type RawOptions = Vec<(String, Vec<String>)>;

fn ini_err(class: &'static str, msg: String) -> PyErr {
    PyErr::other(class, msg)
}

fn is_space(c: char) -> bool {
    platform_base::py::is_py_space(c)
}

impl Ini {
    /// `cp.read(path)`: a file that cannot be opened is skipped.
    fn read(path: &str) -> PyResult<Ini> {
        let mut ini = Ini { defaults: Vec::new(), sections: Vec::new() };
        let text = match io::read_text(path) {
            Ok(t) => t,
            Err(e) if e.exc.is_os_error() => return Ok(ini),
            Err(e) => return Err(e),
        };
        ini.parse(&text, path)?;
        Ok(ini)
    }

    /// `_read_inner`, with the values joined as `_join_multiline_values`
    /// does.
    fn parse(&mut self, text: &str, fpname: &str) -> PyResult<()> {
        // Where values are built: None for DEFAULT, else a section index.
        enum Cur {
            None,
            Default,
            Section(usize),
        }
        let mut raw_defaults: RawOptions = Vec::new();
        let mut raw_sections: Vec<(String, RawOptions)> = Vec::new();
        let mut added: HashSet<String> = HashSet::new();
        let mut cur = Cur::None;
        let mut sectname = String::new();
        let mut optname: Option<String> = None;
        let mut indent_level = 0usize;
        let mut errors: Vec<(usize, String)> = Vec::new();
        let mut lines: Vec<&str> = text.split_inclusive('\n').collect();
        if text.is_empty() {
            lines.clear();
        }
        // The option map `cursect` names.
        fn opts<'a>(
            cur: &Cur,
            defaults: &'a mut RawOptions,
            sections: &'a mut [(String, RawOptions)],
        ) -> Option<&'a mut RawOptions> {
            match cur {
                Cur::Default => Some(defaults),
                Cur::Section(k) => sections.get_mut(*k).map(|s| &mut s.1),
                Cur::None => None,
            }
        }
        for (i, line) in lines.iter().enumerate() {
            let lineno = i + 1;
            let stripped = line.trim_matches(is_space);
            let clean = if stripped.starts_with('#') || stripped.starts_with(';') { "" } else { stripped };
            if clean.is_empty() {
                // A blank line belongs to the value being read; a comment
                // line does not.
                let has_comments = stripped != clean;
                if let (false, Some(o)) = (has_comments, optname.as_ref().filter(|o| !o.is_empty())) {
                    if let Some(v) = opts(&cur, &mut raw_defaults, &mut raw_sections).and_then(|m| m.iter_mut().find(|(k, _)| k == o)) {
                        v.1.push(String::new());
                    }
                }
                continue;
            }
            let cur_indent = line.chars().position(|c| !is_space(c)).unwrap_or(0);
            // A continuation line?
            if !matches!(cur, Cur::None) && optname.as_ref().is_some_and(|o| !o.is_empty()) && cur_indent > indent_level {
                let o = optname.clone().unwrap_or_default();
                if let Some(v) = opts(&cur, &mut raw_defaults, &mut raw_sections).and_then(|m| m.iter_mut().find(|(k, _)| *k == o)) {
                    v.1.push(clean.to_string());
                }
                continue;
            }
            indent_level = cur_indent;
            // A section header: `\[(?P<header>.+)\]`, matched from the start.
            let header = clean.strip_prefix('[').and_then(|rest| {
                let k = rest.rfind(']')?;
                (k >= 1).then(|| rest[..k].to_string())
            });
            if header.is_none() && matches!(cur, Cur::None) {
                return Err(ini_err(
                    "MissingSectionHeaderError",
                    format!(
                        "File contains no section headers.\nfile: {}, line: {lineno}\n{}",
                        str_repr(fpname),
                        str_repr(line)
                    ),
                ));
            }
            if let Some(h) = header {
                sectname = h.clone();
                if let Some(k) = raw_sections.iter().position(|(n, _)| *n == h) {
                    if added.contains(&format!("S{h}")) {
                        return Err(ini_err(
                            "DuplicateSectionError",
                            format!(
                                "While reading from {} [line {lineno:2}]: section {} already exists",
                                str_repr(fpname),
                                str_repr(&h)
                            ),
                        ));
                    }
                    cur = Cur::Section(k);
                    added.insert(format!("S{h}"));
                } else if h == DEFAULTSECT {
                    cur = Cur::Default;
                } else {
                    raw_sections.push((h.clone(), Vec::new()));
                    cur = Cur::Section(raw_sections.len() - 1);
                    added.insert(format!("S{h}"));
                }
                optname = None;
                continue;
            }
            // An option line: `(?P<option>.*?)\s*(?P<vi>=|:)\s*(?P<value>.*)$`.
            indent_level = cur_indent;
            let Some(d) = clean.find(['=', ':']) else {
                errors.push((lineno, line.to_string()));
                continue;
            };
            let opt = clean[..d].trim_end_matches(is_space);
            let val = clean[d + 1..].trim_matches(is_space).to_string();
            if opt.is_empty() {
                errors.push((lineno, line.to_string()));
            }
            let o = opt.trim_end_matches(is_space).to_lowercase();
            let key = format!("O{sectname}\u{0}{o}");
            if added.contains(&key) {
                return Err(ini_err(
                    "DuplicateOptionError",
                    format!(
                        "While reading from {} [line {lineno:2}]: option {} in section {} already exists",
                        str_repr(fpname),
                        str_repr(&o),
                        str_repr(&sectname)
                    ),
                ));
            }
            added.insert(key);
            if let Some(m) = opts(&cur, &mut raw_defaults, &mut raw_sections) {
                match m.iter_mut().find(|(k, _)| *k == o) {
                    Some(v) => v.1 = vec![val],
                    None => m.push((o.clone(), vec![val])),
                }
            }
            optname = Some(o);
        }
        let join = |v: RawOptions| -> Vec<(String, String)> {
            v.into_iter().map(|(k, l)| (k, l.join("\n").trim_end_matches(is_space).to_string())).collect()
        };
        self.defaults = join(raw_defaults);
        self.sections = raw_sections.into_iter().map(|(n, o)| (n, join(o))).collect();
        if let Some((first, rest)) = errors.split_first() {
            let mut msg = format!("Source contains parsing errors: {}", str_repr(fpname));
            for (lineno, line) in std::iter::once(first).chain(rest) {
                msg.push_str(&format!("\n\t[line {lineno:2}]: {}", str_repr(line)));
            }
            return Err(ini_err("ParsingError", msg));
        }
        Ok(())
    }

    fn section(&self, name: &str) -> Option<&Vec<(String, String)>> {
        self.sections.iter().find(|(n, _)| n == name).map(|(_, o)| o)
    }

    /// `has_option(section, option)`.
    fn has_option(&self, section: &str, option: &str) -> bool {
        let o = option.to_lowercase();
        let in_defaults = self.defaults.iter().any(|(k, _)| *k == o);
        if section.is_empty() || section == DEFAULTSECT {
            return in_defaults;
        }
        match self.section(section) {
            None => false,
            Some(s) => s.iter().any(|(k, _)| *k == o) || in_defaults,
        }
    }

    /// The section's options over the defaults, as `_unify_values` chains
    /// them.
    fn lookup(&self, section: &str, key: &str) -> Option<String> {
        self.section(section)
            .and_then(|s| s.iter().find(|(k, _)| k == key))
            .or_else(|| self.defaults.iter().find(|(k, _)| k == key))
            .map(|(_, v)| v.clone())
    }

    /// `get(section, option)`, interpolated.
    fn get(&self, section: &str, option: &str) -> PyResult<String> {
        if self.section(section).is_none() && section != DEFAULTSECT {
            return Err(ini_err("NoSectionError", format!("No section: {}", str_repr(section))));
        }
        let o = option.to_lowercase();
        let Some(value) = self.lookup(section, &o) else {
            return Err(ini_err(
                "NoOptionError",
                format!("No option {} in section: {}", str_repr(&o), str_repr(section)),
            ));
        };
        let mut accum = String::new();
        self.interpolate(&o, &mut accum, &value, section, 1)?;
        Ok(accum)
    }

    /// `BasicInterpolation._interpolate_some`.
    fn interpolate(&self, option: &str, accum: &mut String, rest: &str, section: &str, depth: usize) -> PyResult<()> {
        let rawval = self.lookup(section, option).unwrap_or_else(|| rest.to_string());
        if depth > 10 {
            return Err(ini_err(
                "InterpolationDepthError",
                format!(
                    "Recursion limit exceeded in value substitution: option {} in section {} contains an interpolation key which cannot be substituted in 10 steps. Raw value: {}",
                    str_repr(option),
                    str_repr(section),
                    str_repr(&rawval)
                ),
            ));
        }
        let keycre = Regex::new(r"^%\(([^)]+)\)s").map_err(|e| PyErr::runtime_error(e.to_string()))?;
        let mut rest = rest.to_string();
        while !rest.is_empty() {
            let Some(p) = rest.find('%') else {
                accum.push_str(&rest);
                return Ok(());
            };
            if p > 0 {
                accum.push_str(&rest[..p]);
                rest = rest[p..].to_string();
            }
            let c: String = rest.chars().skip(1).take(1).collect();
            if c == "%" {
                accum.push('%');
                rest = rest[2..].to_string();
            } else if c == "(" {
                let Some(m) = keycre.captures(&rest) else {
                    return Err(ini_err(
                        "InterpolationSyntaxError",
                        format!("bad interpolation variable reference {}", str_repr(&rest)),
                    ));
                };
                let var = m.get(1).map_or("", |g| g.as_str()).to_lowercase();
                let end = m.get(0).map_or(0, |g| g.end());
                rest = rest[end..].to_string();
                let Some(v) = self.lookup(section, &var) else {
                    return Err(ini_err(
                        "InterpolationMissingOptionError",
                        format!(
                            "Bad value substitution: option {} in section {} contains an interpolation key {} which is not a valid option name. Raw value: {}",
                            str_repr(option),
                            str_repr(section),
                            str_repr(&var),
                            str_repr(&rawval)
                        ),
                    ));
                };
                if v.contains('%') {
                    self.interpolate(option, accum, &v, section, depth + 1)?;
                } else {
                    accum.push_str(&v);
                }
            } else {
                return Err(ini_err(
                    "InterpolationSyntaxError",
                    format!("'%' must be followed by '%' or '(', found: {}", str_repr(&rest)),
                ));
            }
        }
        Ok(())
    }
}

// -- MPFAManager --------------------------------------------------------------

const MPFA_EXTENSION: &str = ".mpfa";

/// An extracted MPFA: its directory and its metadata.
struct Mpfa {
    contents: String,
    metadata: Ini,
}

/// `with MPFAManager(path) as mpfa: body(mpfa)`: extracted on the way in --
/// a failure there leaves whatever it extracted behind, as `__exit__` does
/// not run -- and removed on the way out, whose failure is the one raised.
fn with_mpfa<T>(mpfa_path: &str, body: impl FnOnce(&Mpfa) -> PyResult<T>) -> PyResult<T> {
    if !io::is_file(mpfa_path) {
        return Err(PyErr::runtime_error(format!("MPFA doesn't exist: path={mpfa_path}")));
    }
    if splitext_ext(mpfa_path) != MPFA_EXTENSION {
        return Err(PyErr::runtime_error(format!("MPFA doesn't have valid extension: path={mpfa_path}")));
    }
    let contents = mkdtemp("mpfa-")?;
    check_call(&["tar", "xzf", mpfa_path, "-C", &contents], false)?;
    let metadata_path = io::join(&contents, "metadata.ini");
    if !io::is_file(&metadata_path) {
        return Err(PyErr::runtime_error(format!("MPFA metadata doesn't exist: path={metadata_path}")));
    }
    let metadata = Ini::read(&metadata_path)?;
    let mpfa = Mpfa { contents, metadata };
    let r = body(&mpfa);
    if io::exists(&mpfa.contents) {
        check_call(&["rm", "-rf", &mpfa.contents], false)?;
    }
    r
}

// -- ONIEUpdater --------------------------------------------------------------

const ONIE_FW_UPDATE: &str = "/usr/bin/mlnx-onie-fw-update.sh";
const ONIE_VERSION_PARSE_PATTERN: &str =
    r"([0-9]{4})\.([0-9]{2})-([0-9]+)\.([0-9]+)\.([0-9]+)-?(rc[0-9]+)?-?(dev)?-([0-9]+)";
const ONIE_VERSION_BASE_PARSE_PATTERN: &str = r"([0-9]+)\.([0-9]+)\.([0-9]+)";
const ONIE_VERSION_REQUIRED: &str = "5.2.0016";
const ONIE_VERSION_ATTR: &str = "onie_version";
const ONIE_NO_PENDING_UPDATES_ATTR: &str = "No pending firmware updates present";
const PLATFORM_ALWAYS_SUPPORT_UPGRADE: [&str; 1] = ["x86_64-nvidia_sn2201-r0"];
const FS_MOUNTPOINT: &str = "/mnt/onie-fs";
const ONIE_PATH: &str = "/lib/onie";

/// `ONIEUpdater`: firmware staged for ONIE to install on the next boot, and
/// ONIE's version, read from its file system.
struct OnieUpdater {
    platform: Option<String>,
}

impl OnieUpdater {
    fn new() -> PyResult<OnieUpdater> {
        Ok(OnieUpdater { platform: device_info::get_platform()? })
    }

    /// `__add_prefix(image_path)`: a copy under /tmp whose name orders it
    /// among ONIE's pending updates -- a ROM last -- made on every call.  A
    /// capsule is staged where it is.
    fn add_prefix(image_path: &str) -> PyResult<String> {
        if image_path.ends_with(".cab") {
            return Ok(image_path.to_string());
        }
        let rename_path = if !image_path.contains(".rom") {
            format!("/tmp/00-{}", basename(image_path))
        } else {
            format!("/tmp/99-{}", basename(image_path))
        };
        copyfile(image_path, &rename_path)?;
        Ok(rename_path)
    }

    fn mount_onie_fs() -> PyResult<&'static str> {
        if lexists(ONIE_PATH) || io::exists(FS_MOUNTPOINT) {
            Self::umount_onie_fs()?;
        }
        let fs_path = check_output_pipe(&[&["fdisk", "-l"], &["grep", "ONIE boot"], &["awk", "{print $1}"]])?;
        let fs_path = fs_path.trim_end_matches('\n');
        mkdir(FS_MOUNTPOINT, 0o777)?;
        check_call(&["mount", "-n", "-r", "-t", "ext4", fs_path, FS_MOUNTPOINT], false)?;
        let fs_onie_path = io::join(FS_MOUNTPOINT, "onie/tools/lib/onie");
        std::os::unix::fs::symlink(&fs_onie_path, io::real(ONIE_PATH)).map_err(|e| os_error2(&e, &fs_onie_path, ONIE_PATH))?;
        Ok(FS_MOUNTPOINT)
    }

    fn umount_onie_fs() -> PyResult<()> {
        if io::is_link(ONIE_PATH) {
            std::fs::remove_file(io::real(ONIE_PATH)).map_err(|e| PyErr::from_io(&e, Some(ONIE_PATH)))?;
        }
        if ismount(FS_MOUNTPOINT) {
            check_call(&["umount", "-rf", FS_MOUNTPOINT], false)?;
        }
        if io::exists(FS_MOUNTPOINT) {
            std::fs::remove_dir(io::real(FS_MOUNTPOINT)).map_err(|e| PyErr::from_io(&e, Some(FS_MOUNTPOINT)))?;
        }
        Ok(())
    }

    fn stage_update(image_path: &str) -> PyResult<()> {
        let rename_path = Self::add_prefix(image_path)?;
        check_call(&[ONIE_FW_UPDATE, "add", &rename_path], false).map_err(|e| {
            if is_called_process_error(&e) {
                PyErr::runtime_error(format!("Failed to stage firmware update: {}", e.str()))
            } else {
                e
            }
        })
    }

    fn unstage_update(image_path: &str) -> PyResult<()> {
        let rename_path = Self::add_prefix(image_path)?;
        check_call(&[ONIE_FW_UPDATE, "remove", basename(&rename_path)], false).map_err(|e| {
            if is_called_process_error(&e) {
                PyErr::runtime_error(format!("Failed to unstage firmware update: {}", e.str()))
            } else {
                e
            }
        })
    }

    fn trigger_update(allow_reboot: bool) -> PyResult<()> {
        let cmd: &[&str] =
            if allow_reboot { &[ONIE_FW_UPDATE, "update"] } else { &[ONIE_FW_UPDATE, "update", "--no-reboot"] };
        check_call(cmd, false).map_err(|e| {
            if is_called_process_error(&e) {
                PyErr::runtime_error(format!("Failed to trigger firmware update: {}", e.str()))
            } else {
                e
            }
        })
    }

    fn show_pending() -> PyResult<String> {
        match check_output_merged(&[ONIE_FW_UPDATE, "show-pending"]) {
            Ok(o) => Ok(o.trim_end_matches('\n').to_string()),
            Err(e) if is_called_process_error(&e) => {
                Err(PyErr::runtime_error(format!("Failed to get pending firmware updates: {}", e.str())))
            }
            Err(e) => Err(e),
        }
    }

    fn is_update_staged(image_path: &str) -> PyResult<bool> {
        let output = Self::show_pending()?;
        let rename_path = Self::add_prefix(image_path)?;
        let base = basename(&rename_path);
        Ok(splitlines(&output).iter().any(|l| l.starts_with(base)))
    }

    /// `parse_onie_version(version, is_base)`: major, minor and release --
    /// all the callers take.
    fn parse_onie_version(version: &str, is_base: bool) -> PyResult<(String, String, String)> {
        let pattern = if is_base { ONIE_VERSION_BASE_PARSE_PATTERN } else { ONIE_VERSION_PARSE_PATTERN };
        let re = Regex::new(pattern).map_err(|e| PyErr::runtime_error(e.to_string()))?;
        let Some(m) = re.captures(version) else {
            return Err(PyErr::runtime_error(format!(
                "Failed to parse ONIE version: pattern={pattern}, version={version}"
            )));
        };
        let g = |i: usize| m.get(i).map_or(String::new(), |x| x.as_str().to_string());
        Ok(if is_base { (g(1), g(2), g(3)) } else { (g(3), g(4), g(5)) })
    }

    /// `get_onie_version()`: grub-machine.cfg's `onie_version=`, with ONIE's
    /// file system mounted for the read and unmounted whatever happened.
    fn get_onie_version() -> PyResult<String> {
        let r = (|| -> PyResult<String> {
            let fs_mountpoint = Self::mount_onie_fs()?;
            let machine_conf_path = io::join(fs_mountpoint, "onie/grub/grub-machine.cfg");
            let text = io::read_text(&machine_conf_path)?;
            let mut version = None;
            for line in text.split_inclusive('\n') {
                if line.starts_with(ONIE_VERSION_ATTR) {
                    let items: Vec<&str> = line.trim_end_matches('\n').split('=').collect();
                    if items.len() != 2 {
                        return Err(PyErr::runtime_error(format!("Failed to parse ONIE info: line={line}")));
                    }
                    version = Some(items[1].to_string());
                    break;
                }
            }
            version.ok_or_else(|| PyErr::runtime_error("Failed to parse ONIE version"))
        })();
        Self::umount_onie_fs()?;
        r
    }

    /// `get_onie_firmware_info(image_path)`: what the image says of itself
    /// when bash runs it with `-q -i`, as `key=value` lines.
    fn get_onie_firmware_info(image_path: &str) -> PyResult<Vec<(String, String)>> {
        let r = (|| -> PyResult<Vec<(String, String)>> {
            Self::mount_onie_fs()?;
            let output = match check_output_merged(&["/bin/bash", image_path, "-q", "-i"]) {
                Ok(o) => o.trim_end_matches('\n').to_string(),
                Err(e) if is_called_process_error(&e) => {
                    return Err(PyErr::runtime_error(format!("Failed to get ONIE firmware info: {}", e.str())))
                }
                Err(e) => return Err(e),
            };
            let mut info: Vec<(String, String)> = Vec::new();
            for line in splitlines(&output) {
                let items: Vec<&str> = line.split('=').collect();
                if items.len() != 2 {
                    return Err(PyErr::runtime_error(format!("Failed to parse ONIE firmware info: line={line}")));
                }
                match info.iter_mut().find(|(k, _)| k == items[0]) {
                    Some(e) => e.1 = items[1].to_string(),
                    None => info.push((items[0].to_string(), items[1].to_string())),
                }
            }
            Ok(info)
        })();
        Self::umount_onie_fs()?;
        r
    }

    /// `__get_pending_updates()`: the names in show-pending's table.
    fn get_pending_updates() -> PyResult<Vec<String>> {
        let output = Self::show_pending()?;
        if output.contains(ONIE_NO_PENDING_UPDATES_ATTR) {
            return Ok(Vec::new());
        }
        let mut names = Vec::new();
        for line in splitlines(&output) {
            let Some((name, _)) = line.split_once('|') else {
                continue;
            };
            let name = strip(name);
            if name.is_empty() || name == "Name" {
                continue;
            }
            names.push(name.to_string());
        }
        Ok(names)
    }

    /// `__clear_pending_updates()`: every stale pending update removed, or
    /// the update stopped before anything is staged.
    fn clear_pending_updates() -> PyResult<()> {
        let pending = Self::get_pending_updates()?;
        if pending.is_empty() {
            return Ok(());
        }
        notice_print(&format!(
            "Found stale firmware update(s) in the ONIE pending directory: {}. \
             ONIE runs every pending file as an installer on the next update-mode \
             reboot, so these would break the update and can leave the switch stuck \
             in ONIE; removing them before staging the new image (firmware update \
             history and rollback backup are preserved).",
            pending.join(", ")
        ));
        let mut removed = Vec::new();
        let mut failed = Vec::new();
        for name in &pending {
            match check_call(&[ONIE_FW_UPDATE, "remove", name], false) {
                Ok(()) => removed.push(name.as_str()),
                Err(e) if is_called_process_error(&e) => {
                    failed.push(name.as_str());
                    utils::log_warning(&format!("Failed to clear pending firmware update '{name}': {}", e.str()));
                }
                Err(e) => return Err(e),
            }
        }
        if !removed.is_empty() {
            notice_print(&format!("Removed stale pending firmware update(s): {}", removed.join(", ")));
        }
        if !failed.is_empty() {
            return Err(PyErr::runtime_error(format!(
                "Failed to clear stale pending firmware update(s): {}",
                failed.join(", ")
            )));
        }
        Ok(())
    }

    /// `update_firmware(image_path, allow_reboot)`: on any failure, the
    /// image is taken back out of ONIE's queue if it got in -- and a failure
    /// of that is what is raised instead.
    fn update_firmware(image_path: &str, allow_reboot: bool) -> PyResult<()> {
        let r = (|| -> PyResult<()> {
            if allow_reboot {
                Self::clear_pending_updates()?;
            }
            Self::stage_update(image_path)?;
            Self::trigger_update(allow_reboot)
        })();
        if let Err(e) = r {
            if Self::is_update_staged(image_path)? {
                Self::unstage_update(image_path)?;
            }
            return Err(e);
        }
        Ok(())
    }

    /// `is_non_onie_firmware_update_supported()`: ONIE new enough to install
    /// firmware other than its own -- always so on the platforms in
    /// `PLATFORM_ALWAYS_SUPPORT_UPGRADE`.
    fn is_non_onie_firmware_update_supported(&self) -> PyResult<bool> {
        if self.platform.as_deref().is_some_and(|p| PLATFORM_ALWAYS_SUPPORT_UPGRADE.contains(&p)) {
            return Ok(true);
        }
        let current = Self::get_onie_version()?;
        let (ma1, mi1, re1) = Self::parse_onie_version(&current, false)?;
        let version1 = py_int(&format!("{ma1}{mi1}{re1}"))?;
        let (ma2, mi2, re2) = Self::parse_onie_version(ONIE_VERSION_REQUIRED, true)?;
        let version2 = py_int(&format!("{ma2}{mi2}{re2}"))?;
        Ok(version1 >= version2)
    }
}

// -- the host's components ----------------------------------------------------

/// Which class of component.py.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Kind {
    Onie,
    Ssd,
    Bios,
    BiosSn2201,
    Cpld,
    CpldSn2201,
    CpldSn4280,
    FpgaDpu,
}

impl Kind {
    fn is_cpld(self) -> bool {
        matches!(self, Kind::Cpld | Kind::CpldSn2201 | Kind::CpldSn4280 | Kind::FpgaDpu)
    }
}

const SSD_FW_UPDATE: &str = "/usr/bin/mlnx-ssd-fw-update.sh";
const ASIC_DETECT_SCRIPT: &str = "/usr/bin/asic_detect/asic_detect.sh";
const AUX_PWR_CYCLE_FILE: &str = "/var/run/hw-management/system/aux_pwr_cycle";
const WAIT_FOR_DPU_OS_RUN_TIMEOUT: i128 = 300;
const DPU_BOOT_PROGRESS_POLL_INTERVAL: i128 = 5;

/// A component of component.py other than the BMC's.
pub struct HostComponent {
    kind: Kind,
    /// `self.idx`, for the CPLD family.
    idx: i128,
    name: String,
    onie_updater: Option<OnieUpdater>,
    /// `_dpus_to_monitor`, once the FPGA's pre-update hook has run.
    dpus_to_monitor: Mutex<Option<Vec<(String, BootProg)>>>,
}

impl HostComponent {
    fn new(kind: Kind, idx: i128) -> PyResult<HostComponent> {
        let name = match kind {
            Kind::Onie => "ONIE".to_string(),
            Kind::Ssd => "SSD".to_string(),
            Kind::Bios | Kind::BiosSn2201 => "BIOS".to_string(),
            Kind::FpgaDpu => format!("DPU{idx}_FPGA"),
            _ => format!("CPLD{idx}"),
        };
        let onie_updater = matches!(kind, Kind::Onie | Kind::Bios).then(OnieUpdater::new).transpose()?;
        Ok(HostComponent { kind, idx, name, onie_updater, dpus_to_monitor: Mutex::new(None) })
    }

    fn description(&self) -> &'static str {
        match self.kind {
            Kind::Onie => "ONIE - Open Network Install Environment",
            Kind::Ssd => "SSD - Solid-State Drive",
            Kind::Bios | Kind::BiosSn2201 => "BIOS - Basic Input/Output System",
            Kind::FpgaDpu => "FPGA - Field-Programmable Gate Array",
            _ => "CPLD - Complex Programmable Logic Device",
        }
    }

    /// `self.image_ext_name`.
    fn image_ext_name(&self) -> Option<&'static [&'static str]> {
        match self.kind {
            Kind::Onie | Kind::BiosSn2201 => None,
            Kind::Ssd => Some(&[".pkg"]),
            Kind::Bios => Some(&[".rom", ".cab"]),
            _ => Some(&[".vme"]),
        }
    }

    fn onie(&self) -> &OnieUpdater {
        static NONE: OnieUpdater = OnieUpdater { platform: None };
        self.onie_updater.as_ref().unwrap_or(&NONE)
    }

    fn check_file_validity(&self, image_path: &str) -> bool {
        check_file_validity(image_path, &self.name, self.image_ext_name())
    }

    // ONIE --------------------------------------------------------------

    fn onie_install(&self, image_path: &str, allow_reboot: bool) -> PyResult<bool> {
        if !self.check_file_validity(image_path) {
            return Ok(false);
        }
        print(&format!("INFO: Staging {} firmware update with ONIE updater", self.name));
        match OnieUpdater::update_firmware(image_path, allow_reboot) {
            Ok(()) => Ok(true),
            Err(e) if is_exception(&e) => {
                print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                Ok(false)
            }
            Err(e) => Err(e),
        }
    }

    // SSD ---------------------------------------------------------------

    fn ssd_install(&self, image_path: &str, allow_reboot: bool) -> PyResult<bool> {
        if !self.check_file_validity(image_path) {
            return Ok(false);
        }
        let update = [SSD_FW_UPDATE, "-y", "-u", "-i", image_path];
        let install = [SSD_FW_UPDATE, "--no-power-cycle", "-y", "-u", "-i", image_path];
        let cmd: &[&str] = if allow_reboot { &update } else { &install };
        print(&format!("INFO: Installing {} firmware update", self.name));
        match check_call(cmd, false) {
            Ok(()) => Ok(true),
            Err(e) if is_called_process_error(&e) => {
                print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                Ok(false)
            }
            Err(e) => Err(e),
        }
    }

    /// The SSD tool's `-q -i image` report.
    fn ssd_firmware_info(&self, image_path: &str) -> PyResult<String> {
        match check_output_merged(&[SSD_FW_UPDATE, "-q", "-i", image_path]) {
            Ok(o) => Ok(o.trim_end_matches('\n').to_string()),
            Err(e) if is_called_process_error(&e) => {
                Err(PyErr::runtime_error(format!("Failed to get {} firmware info: {}", self.name, e.str())))
            }
            Err(e) => Err(e),
        }
    }

    fn ssd_update_notification(&self, image_path: &str) -> PyResult<PyVal> {
        let output = self.ssd_firmware_info(image_path)?;
        let mut power_cycle_required = None;
        let mut upgrade_required = None;
        for line in splitlines(&output) {
            if line.starts_with("Power Cycle Required") {
                power_cycle_required = Some(field(line)?);
            }
            if line.starts_with("Upgrade Required") {
                upgrade_required = Some(field(line)?);
            }
        }
        if !upgrade_required.as_deref().is_some_and(|u| u == "yes" || u == "no") {
            return Err(PyErr::runtime_error(format!("Failed to parse {} firmware upgrade status", self.name)));
        }
        if upgrade_required.as_deref() == Some("no") {
            return Ok(PyVal::None);
        }
        if !power_cycle_required.as_deref().is_some_and(|u| u == "yes" || u == "no") {
            return Err(PyErr::runtime_error(format!("Failed to parse {} firmware power policy", self.name)));
        }
        if power_cycle_required.as_deref() == Some("yes") {
            return Ok(PyVal::Str(format!(
                "Immediate power cycle is required to complete {} firmware update",
                self.name
            )));
        }
        Ok(PyVal::None)
    }

    // BIOS --------------------------------------------------------------

    fn bios_install(&self, image_path: &str, allow_reboot: bool) -> PyResult<bool> {
        if !self.onie().is_non_onie_firmware_update_supported()? {
            print(&format!("ERROR: ONIE {ONIE_VERSION_REQUIRED} or later is required"));
            return Ok(false);
        }
        if !self.check_file_validity(image_path) {
            return Ok(false);
        }
        print(&format!("INFO: Staging {} firmware update with ONIE updater", self.name));
        match OnieUpdater::update_firmware(image_path, allow_reboot) {
            Ok(()) => Ok(true),
            Err(e) if is_exception(&e) => {
                utils::log_warning(&format!("Failed to update {} firmware: {}", self.name, e.str()));
                print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                Ok(false)
            }
            Err(e) => Err(e),
        }
    }

    /// The `dmidecode` the BIOS version comes from.
    fn dmidecode(&self, argv: &[&str]) -> PyResult<String> {
        match check_output_merged(argv) {
            Ok(o) => Ok(o.trim_end_matches('\n').to_string()),
            Err(e) if is_called_process_error(&e) => {
                Err(PyErr::runtime_error(format!("Failed to get {} version: {}", self.name, e.str())))
            }
            Err(e) => Err(e),
        }
    }

    // CPLD --------------------------------------------------------------

    /// `_mst_context()`: mst started around `body`, and stopped after it
    /// whatever it did; a failed stop is only logged.
    fn with_mst<T>(&self, body: impl FnOnce() -> PyResult<T>) -> PyResult<T> {
        notice_print(&format!("{}: mst start begin", self.name));
        if let Err(e) = check_call(&["/usr/bin/mst", "start"], false) {
            if is_called_process_error(&e) {
                utils::log_error(&format!("Failed to manage {} mst: {}", self.name, e.str()));
            }
            return Err(e);
        }
        let r = body();
        notice_print(&format!("{}: mst stop begin", self.name));
        match check_call(&["/usr/bin/mst", "stop"], false) {
            Ok(()) => {}
            Err(e) if is_called_process_error(&e) => {
                utils::log_error(&format!("Failed to stop {} mst: {}", self.name, e.str()))
            }
            Err(e) => return Err(e),
        }
        r
    }

    /// `__get_mst_device()`: the ASIC's mst cr-space node.
    fn get_mst_device(&self) -> PyResult<String> {
        let out = match check_output_bytes(&[ASIC_DETECT_SCRIPT, "-m"]) {
            Ok(o) => o,
            Err(e) if is_called_process_error(&e) => {
                return Err(PyErr::runtime_error(format!("Failed to get {} mst device: {}", self.name, e.str())))
            }
            Err(e) => return Err(e),
        };
        let out = codec::decode(codec::Codec::Utf8, &out)?;
        let out = strip(&out);
        if out.is_empty() {
            return Err(PyErr::runtime_error(format!("Failed to get {} mst device: empty device node", self.name)));
        }
        Ok(out.to_string())
    }

    /// `_is_spc1_asic()`: asic_detect says `spc1`; no script, no answer in 30
    /// seconds or no way to run it is "no".
    fn is_spc1_asic() -> PyResult<bool> {
        if !io::is_file(ASIC_DETECT_SCRIPT) {
            return Ok(false);
        }
        match run_stdout(&[ASIC_DETECT_SCRIPT], Duration::from_secs(30)) {
            Ok(out) => Ok(strip(&subprocess::text(&out)?) == "spc1"),
            Err(e) if e.exc.is_os_error() || e.exc.name() == "TimeoutExpired" => Ok(false),
            Err(e) => Err(e),
        }
    }

    /// A cpldupdate run, `_install_firmware`'s common ending.
    fn cpldupdate(&self, cmd: &[&str], image_path: &str) -> PyResult<bool> {
        print(&format!("INFO: Installing {} firmware update: path={image_path}", self.name));
        match check_call(cmd, false) {
            Ok(()) => Ok(true),
            Err(e) if is_called_process_error(&e) => {
                print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                Ok(false)
            }
            Err(e) => Err(e),
        }
    }

    /// `_install_firmware(image_path)`, per class: only the base class looks
    /// at the file first, and only it knows SPC1, which cpldupdate reaches
    /// through the mst device.
    fn cpld_install(&self, image_path: &str) -> PyResult<bool> {
        match self.kind {
            Kind::CpldSn2201 => {
                self.cpldupdate(&["cpldupdate", "--gpio", image_path, "--uncustomized", "--print-progress"], image_path)
            }
            Kind::CpldSn4280 => self.cpldupdate(&["cpldupdate", "--gpio", "--print-progress", image_path], image_path),
            Kind::FpgaDpu => {
                print(&format!("INFO: Installing {} firmware update: path={image_path}", self.name));
                let cmd = ["cpldupdate", "--cpld_chain", "2", "--gpio", "--print-progress", image_path];
                match self.with_mst(|| check_call(&cmd, false)) {
                    Ok(()) => Ok(true),
                    Err(e) if is_called_process_error(&e) => {
                        print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                        Ok(false)
                    }
                    Err(e) => Err(e),
                }
            }
            _ => {
                if !self.check_file_validity(image_path) {
                    return Ok(false);
                }
                if Self::is_spc1_asic()? {
                    let r = self.with_mst(|| {
                        let mst_dev = self.get_mst_device()?;
                        notice_print(&format!("{}: mst device = {mst_dev}", self.name));
                        let cmd = ["cpldupdate", "--dev", mst_dev.as_str(), "--print-progress", image_path];
                        print(&format!("INFO: Installing {} firmware update: path={image_path}", self.name));
                        notice_print(&format!("{}: cpldupdate begin: {}", self.name, cmd.join(" ")));
                        check_call(&cmd, false)?;
                        notice_print(&format!("{}: cpldupdate done", self.name));
                        Ok(())
                    });
                    return match r {
                        Ok(()) => Ok(true),
                        Err(e) if is_runtime_error(&e) => {
                            print(&format!("ERROR: {}", e.str()));
                            Ok(false)
                        }
                        Err(e) if is_called_process_error(&e) => {
                            print(&format!("ERROR: Failed to update {} firmware: {}", self.name, e.str()));
                            Ok(false)
                        }
                        Err(e) => Err(e),
                    };
                }
                self.cpldupdate(&["cpldupdate", "--gpio", "--print-progress", image_path], image_path)
            }
        }
    }

    /// `install_firmware(image_path)`: an MPFA's burn file, or the file.
    fn cpld_install_firmware(&self, image_path: &str) -> PyResult<bool> {
        if !image_path.contains(MPFA_EXTENSION) {
            return self.cpld_install(image_path);
        }
        with_mpfa(image_path, |mpfa| {
            if !mpfa.metadata.has_option("firmware", "burn") {
                return Err(PyErr::runtime_error(format!("Failed to get {} burn firmware", self.name)));
            }
            let burn = mpfa.metadata.get("firmware", "burn")?;
            print(&format!("INFO: Processing {} burn file: firmware install", self.name));
            self.cpld_install(&io::join(&mpfa.contents, &burn))
        })
    }

    /// `update_firmware(image_path)`: burn, then -- on a BMC platform -- an
    /// aux power cycle, or the refresh file, between the hooks.
    fn cpld_update_firmware(&self, image_path: &str) -> PyResult<()> {
        print("INFO: Running pre-update hook");
        self.pre_update_hook()?;
        let refreshed = with_mpfa(image_path, |mpfa| -> PyResult<bool> {
            if !mpfa.metadata.has_option("firmware", "burn") {
                return Err(PyErr::runtime_error(format!("Failed to get {} burn firmware", self.name)));
            }
            let burn = mpfa.metadata.get("firmware", "burn")?;
            print(&format!("INFO: Processing {} burn file: firmware install", self.name));
            if !self.cpld_install(&io::join(&mpfa.contents, &burn))? {
                return Ok(false);
            }
            if device_data::is_platform_with_bmc()? {
                print(&format!(
                    "INFO: Burning {} firmware completed. Running aux power cycle to complete firmware update",
                    self.name
                ));
                utils::write_file(AUX_PWR_CYCLE_FILE, &"1", true, Log::Error)?;
                return Ok(false);
            }
            if !mpfa.metadata.has_option("firmware", "refresh") {
                return Err(PyErr::runtime_error(format!("Failed to get {} refresh firmware", self.name)));
            }
            let refresh = mpfa.metadata.get("firmware", "refresh")?;
            print(&format!("INFO: Processing {} refresh file: firmware update", self.name));
            self.cpld_install(&io::join(&mpfa.contents, &refresh))
        })?;
        if refreshed {
            print("INFO: Running post-update hook");
            self.post_update_hook()?;
        }
        Ok(())
    }

    /// `_pre_update_hook()`: the FPGA's notes the DPUs running now.
    fn pre_update_hook(&self) -> PyResult<()> {
        if self.kind != Kind::FpgaDpu {
            return Ok(());
        }
        let mut running = Vec::new();
        for dpu_index in 0..device_data::get_dpu_count()? {
            let dpu_name = format!("dpu{dpu_index}");
            let ctl = BootProg::printing(&dpu_name)?;
            if ctl.in_os_run() {
                running.push((dpu_name, ctl));
            }
        }
        if let Ok(mut g) = self.dpus_to_monitor.lock() {
            *g = Some(running);
        }
        Ok(())
    }

    /// `_post_update_hook()`: the FPGA's waits for those DPUs to be back,
    /// then reboots the switch.
    fn post_update_hook(&self) -> PyResult<()> {
        if self.kind != Kind::FpgaDpu {
            return Ok(());
        }
        // `getattr(self, '_dpus_to_monitor', {})`.
        let ctls: Vec<(String, BootProg)> =
            self.dpus_to_monitor.lock().ok().and_then(|g| g.clone()).unwrap_or_default();
        if !ctls.is_empty() {
            let mut names: Vec<&str> = ctls.iter().map(|(n, _)| n.as_str()).collect();
            names.sort();
            notice_print(&format!(
                "{} refresh done; waiting up to {WAIT_FOR_DPU_OS_RUN_TIMEOUT}s for DPU(s) {} to reach OS_RUN",
                self.name,
                names.join(", ")
            ));
            utils::wait_until(
                || ctls.iter().all(|(_, c)| c.in_os_run()),
                WAIT_FOR_DPU_OS_RUN_TIMEOUT as f64,
                DPU_BOOT_PROGRESS_POLL_INTERVAL as f64,
            );
            let mut not_running: Vec<&str> =
                ctls.iter().filter(|(_, c)| !c.in_os_run()).map(|(n, _)| n.as_str()).collect();
            not_running.sort();
            if !not_running.is_empty() {
                warning_print(&format!(
                    "{} refresh: DPU(s) {} did not reach OS_RUN; rebooting anyway",
                    self.name,
                    not_running.join(", ")
                ));
            }
        }
        check_call(&["/usr/local/bin/reboot"], true).map_err(|e| {
            if is_called_process_error(&e) {
                PyErr::runtime_error(format!("{} host reboot failed: {}", self.name, e.str()))
            } else {
                e
            }
        })
    }

    /// `get_firmware_version()` of the CPLD family:
    /// `CPLD<part>_REV<major><minor>`, zero-padded, from hw-management.
    fn cpld_firmware_version(&self) -> PyResult<PyVal> {
        let (pn, ver, min, fmt) = if self.kind == Kind::FpgaDpu {
            (
                format!("/var/run/hw-management/dpu{}/system/fpga1_pn", self.idx),
                format!("/var/run/hw-management/dpu{}/system/fpga1_version", self.idx),
                format!("/var/run/hw-management/dpu{}/system/fpga1_version_min", self.idx),
                "FPGA",
            )
        } else {
            (
                format!("/var/run/hw-management/system/cpld{}_pn", self.idx),
                format!("/var/run/hw-management/system/cpld{}_version", self.idx),
                format!("/var/run/hw-management/system/cpld{}_version_min", self.idx),
                "CPLD",
            )
        };
        let part_number = read_generic_file(&pn, 6, true)?;
        let version = read_generic_file(&ver, 2, false)?.unwrap_or_default();
        let version_minor = read_generic_file(&min, 2, true)?;
        let part_number = part_number.unwrap_or_else(|| "0".into());
        let version_minor = version_minor.unwrap_or_else(|| "0".into());
        Ok(PyVal::Str(format!(
            "{fmt}{}_REV{}{}",
            zfill(part_number.trim_end_matches('\n'), 6),
            zfill(version.trim_end_matches('\n'), 2),
            zfill(version_minor.trim_end_matches('\n'), 2)
        )))
    }

    /// `Component.auto_update_firmware`: an install for the next cold boot.
    fn base_auto_update(&self, image_path: &str, boot_type: &str) -> PyResult<PyVal> {
        if !io::exists(image_path) {
            return Ok(PyVal::Int(FW_AUTO_ERR_IMAGE));
        }
        if boot_type != "cold" {
            return Ok(PyVal::Int(FW_AUTO_ERR_BOOT_TYPE));
        }
        let installed = match self.kind {
            Kind::Onie => self.onie_install(image_path, false)?,
            Kind::Bios => self.bios_install(image_path, false)?,
            // ComponentBase.install_firmware takes no allow_reboot.
            _ => {
                return Err(PyErr::type_error(
                    "ComponentBase.install_firmware() got an unexpected keyword argument 'allow_reboot'",
                ))
            }
        };
        if !installed {
            return Ok(PyVal::Int(FW_AUTO_ERR_UNKNOWN));
        }
        Ok(PyVal::Int(FW_AUTO_INSTALLED))
    }
}

/// `line.split(':')[1].lstrip(' \t')`.
fn field(line: &str) -> PyResult<String> {
    line.split(':')
        .nth(1)
        .map(|v| v.trim_start_matches([' ', '\t']).to_string())
        .ok_or_else(|| PyErr::index_error("list index out of range"))
}

impl PyObject for HostComponent {
    fn py_class(&self) -> &'static str {
        match self.kind {
            Kind::Onie => "ComponentONIE",
            Kind::Ssd => "ComponentSSD",
            Kind::Bios => "ComponentBIOS",
            Kind::BiosSn2201 => "ComponentBIOSSN2201",
            Kind::Cpld => "ComponentCPLD",
            Kind::CpldSn2201 => "ComponentCPLDSN2201",
            Kind::CpldSn4280 => "ComponentCPLDSN4280",
            Kind::FpgaDpu => "ComponenetFPGADPU",
        }
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ComponentBase
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name.clone()))
    }
    fn get_description(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(self.description()))
    }
    fn get_firmware_version(&self) -> PyResult<PyVal> {
        match self.kind {
            Kind::Onie => OnieUpdater::get_onie_version().map(PyVal::Str),
            Kind::Ssd => {
                let output = match check_output_merged(&[SSD_FW_UPDATE, "-q"]) {
                    Ok(o) => o.trim_end_matches('\n').to_string(),
                    Err(e) if is_called_process_error(&e) => {
                        return Err(PyErr::runtime_error(format!("Failed to get {} info: {}", self.name, e.str())))
                    }
                    Err(e) => return Err(e),
                };
                for line in splitlines(&output) {
                    if line.starts_with("Firmware Version") {
                        return field(line).map(PyVal::Str);
                    }
                }
                Err(PyErr::runtime_error(format!("Failed to parse {} version", self.name)))
            }
            Kind::Bios => self.dmidecode(&["dmidecode", "--oem-string", "1"]).map(PyVal::Str),
            Kind::BiosSn2201 => {
                let output = self.dmidecode(&["dmidecode", "-t0"])?;
                let re = Regex::new("Version: (.*)").map_err(|e| PyErr::runtime_error(e.to_string()))?;
                Ok(PyVal::Str(match re.captures(&output).and_then(|c| c.get(1)) {
                    Some(m) => m.as_str().to_string(),
                    None => "Unknown version".to_string(),
                }))
            }
            _ => self.cpld_firmware_version(),
        }
    }
    fn get_available_firmware_version(&self, image_path: &str) -> PyResult<PyVal> {
        match self.kind {
            Kind::Onie => {
                let info = OnieUpdater::get_onie_firmware_info(image_path)?;
                match info.into_iter().find(|(k, _)| k == "image_version") {
                    Some((_, v)) => Ok(PyVal::Str(v)),
                    None => Err(PyErr::runtime_error(format!("Failed to get {} available firmware version", self.name))),
                }
            }
            Kind::Ssd => {
                let output = self.ssd_firmware_info(image_path)?;
                let mut current = None;
                let mut available = None;
                let mut upgrade_required = None;
                for line in splitlines(&output) {
                    if line.starts_with("Firmware Version") {
                        current = Some(field(line)?);
                    }
                    if line.starts_with("Available Firmware Version") {
                        available = Some(field(line)?);
                    }
                    if line.starts_with("Upgrade Required") {
                        upgrade_required = Some(field(line)?);
                    }
                }
                if !upgrade_required.as_deref().is_some_and(|u| u == "yes" || u == "no") {
                    return Err(PyErr::runtime_error(format!("Failed to parse {} firmware upgrade status", self.name)));
                }
                if upgrade_required.as_deref() == Some("no") {
                    return current.map(PyVal::Str).ok_or_else(|| {
                        PyErr::runtime_error(format!("Failed to parse {} current firmware version", self.name))
                    });
                }
                available.map(PyVal::Str).ok_or_else(|| {
                    PyErr::runtime_error(format!("Failed to parse {} available firmware version", self.name))
                })
            }
            Kind::Bios => Err(PyErr::not_implemented_msg(format!(
                "{} component doesn't support firmware version query",
                self.name
            ))),
            Kind::BiosSn2201 => Err(PyErr::not_implemented()),
            _ => with_mpfa(image_path, |mpfa| {
                if !mpfa.metadata.has_option("version", &self.name) {
                    return Err(PyErr::runtime_error(format!("Failed to get {} available firmware version", self.name)));
                }
                mpfa.metadata.get("version", &self.name).map(PyVal::Str)
            }),
        }
    }
    fn get_firmware_update_notification(&self, image_path: &str) -> PyResult<PyVal> {
        match self.kind {
            Kind::Onie | Kind::Bios => Ok(PyVal::Str(format!(
                "Immediate cold reboot is required to complete {} firmware update",
                self.name
            ))),
            Kind::Ssd => self.ssd_update_notification(image_path),
            Kind::BiosSn2201 => Ok(PyVal::None),
            _ => Ok(PyVal::Str(if splitext_ext(basename(image_path)) == ".vme" {
                format!(
                    "Power cycle (with 30 sec delay) or refresh image is required to complete {} firmware update",
                    self.name
                )
            } else {
                format!("Immediate power cycle is required to complete {} firmware update", self.name)
            })),
        }
    }
    fn install_firmware(&self, image_path: &str) -> PyResult<PyVal> {
        let ok = match self.kind {
            Kind::Onie => self.onie_install(image_path, true)?,
            Kind::Ssd => self.ssd_install(image_path, true)?,
            Kind::Bios => self.bios_install(image_path, true)?,
            Kind::BiosSn2201 => return Err(PyErr::not_implemented()),
            _ => self.cpld_install_firmware(image_path)?,
        };
        Ok(PyVal::Bool(ok))
    }
    /// The install, its result dropped -- the CPLDs' is a flow of its own.
    fn update_firmware(&self, image_path: &str) -> PyResult<PyVal> {
        match self.kind {
            Kind::Onie => {
                self.onie_install(image_path, true)?;
            }
            Kind::Ssd => {
                self.ssd_install(image_path, true)?;
            }
            Kind::Bios => {
                self.bios_install(image_path, true)?;
            }
            Kind::BiosSn2201 => return Err(PyErr::not_implemented()),
            _ => self.cpld_update_firmware(image_path)?,
        }
        Ok(PyVal::None)
    }
    fn auto_update_firmware(&self, image_path: &str, boot_type: &str) -> PyResult<PyVal> {
        match self.kind {
            // An update needing no reboot is done now; one needing a cold
            // boot waits for it.
            Kind::Ssd => {
                if !io::exists(image_path) {
                    return Ok(PyVal::Int(FW_AUTO_ERR_IMAGE));
                }
                let reboot_required = match self.ssd_update_notification(image_path) {
                    Ok(v) => !v.is_none(),
                    Err(e) if is_runtime_error(&e) => return Ok(PyVal::Int(FW_AUTO_ERR_UNKNOWN)),
                    Err(e) => return Err(e),
                };
                if !reboot_required {
                    self.ssd_install(image_path, true)?;
                    return Ok(PyVal::Int(FW_AUTO_UPDATED));
                }
                if boot_type != "cold" {
                    return Ok(PyVal::Int(FW_AUTO_ERR_BOOT_TYPE));
                }
                Ok(PyVal::Int(FW_AUTO_SCHEDULED))
            }
            k if k.is_cpld() => {
                if !io::exists(image_path) {
                    return Ok(PyVal::Int(FW_AUTO_ERR_IMAGE));
                }
                if boot_type != "cold" {
                    return Ok(PyVal::Int(FW_AUTO_ERR_BOOT_TYPE));
                }
                if !self.cpld_install_firmware(image_path)? {
                    return Ok(PyVal::Int(FW_AUTO_ERR_UNKNOWN));
                }
                Ok(PyVal::Int(FW_AUTO_SCHEDULED))
            }
            _ => self.base_auto_update(image_path, boot_type),
        }
    }
}

/// `ComponentCPLD.get_component_list()` for the class: one per CPLD (or
/// DPU) that hw-management counts -- in one character of its file.
fn cpld_component_list(kind: Kind) -> PyResult<Vec<PyVal>> {
    let file = if kind == Kind::FpgaDpu {
        "/var/run/hw-management/config/dpu_num"
    } else {
        "/var/run/hw-management/config/cpld_num"
    };
    let n = read_generic_file(file, 1, false)?.unwrap_or_default();
    let count = py_int(n.trim_end_matches('\n'))?;
    (1..=count).map(|i| Ok(PyVal::Obj(Arc::new(HostComponent::new(kind, i)?)))).collect()
}

/// `DeviceDataManager.get_bios_component()`.
fn get_bios_component() -> PyResult<PyVal> {
    let kind = if device_data::get_platform_name()?.as_deref() == Some("x86_64-nvidia_sn2201-r0") {
        Kind::BiosSn2201
    } else {
        Kind::Bios
    };
    Ok(PyVal::Obj(Arc::new(HostComponent::new(kind, 0)?)))
}

/// `DeviceDataManager.get_cpld_component_list()`.
fn get_cpld_component_list() -> PyResult<Vec<PyVal>> {
    if device_data::is_simx_platform()? {
        return Ok(Vec::new());
    }
    match device_data::get_platform_name()?.as_deref() {
        Some("x86_64-nvidia_sn2201-r0") => cpld_component_list(Kind::CpldSn2201),
        Some("x86_64-nvidia_sn4280-r0") => {
            let mut v = cpld_component_list(Kind::CpldSn4280)?;
            v.extend(cpld_component_list(Kind::FpgaDpu)?);
            Ok(v)
        }
        _ => cpld_component_list(Kind::Cpld),
    }
}

/// `Chassis.initialize_components()` on the host, before the BMC: ONIE,
/// SSD, BIOS and the CPLDs, each appended as it is built -- so a CPLD list
/// that cannot be read leaves the first three in `list`, as Python leaves
/// them in `_component_list`.
pub fn host_component_list_into(list: &mut Vec<PyVal>) -> PyResult<()> {
    crate::imports::component();
    list.push(PyVal::Obj(Arc::new(HostComponent::new(Kind::Onie, 0)?)));
    list.push(PyVal::Obj(Arc::new(HostComponent::new(Kind::Ssd, 0)?)));
    list.push(get_bios_component()?);
    let cplds = get_cpld_component_list()?;
    list.extend(cplds);
    Ok(())
}

/// [`host_component_list_into`] into a new list.
#[cfg(test)]
pub fn host_component_list() -> PyResult<Vec<PyVal>> {
    let mut v = Vec::new();
    host_component_list_into(&mut v)?;
    Ok(v)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn zfill_and_ini_read_as_python() {
        assert_eq!(zfill("5", 2), "05");
        assert_eq!(zfill("-5", 3), "-05");
        assert_eq!(zfill("123", 2), "123");
        let mut ini = Ini { defaults: Vec::new(), sections: Vec::new() };
        ini.parse("[version]\nCPLD1 = CPLD000085_REV0700\n[firmware]\nburn=b.vme\n  more\nrefresh: %(burn)s.r\n", "m.ini").unwrap();
        assert!(ini.has_option("version", "cpld1"));
        assert_eq!(ini.get("firmware", "burn").unwrap(), "b.vme\nmore");
        assert_eq!(ini.get("firmware", "refresh").unwrap(), "b.vme\nmore.r");
        let mut bad = Ini { defaults: Vec::new(), sections: Vec::new() };
        assert_eq!(
            bad.parse("x=1\n", "m.ini").unwrap_err().str(),
            "File contains no section headers.\nfile: 'm.ini', line: 1\n'x=1\\n'"
        );
    }

    /// `COMP_SCRIPT`/`COMP_OUT`: the native side of the sandbox component
    /// harness (driver.py), step for step.
    #[test]
    fn component_differential() {
        let (Ok(script), Ok(out)) = (std::env::var("COMP_SCRIPT"), std::env::var("COMP_OUT")) else {
            return;
        };
        let steps = platform_base::json::load(&script).unwrap();
        let lines = super::scenario_tests::run_steps(&steps, &[]);
        std::fs::write(out, lines.concat()).unwrap();
    }

    #[test]
    fn splitext_takes_python_s_extension() {
        assert_eq!(splitext_ext("/a/b/fw.fwpkg"), ".fwpkg");
        assert_eq!(splitext_ext("/a/b.c/fw"), "");
        assert_eq!(splitext_ext(".bashrc"), "");
        assert_eq!(splitext_ext("..x"), "");
        assert_eq!(splitext_ext("a..x"), ".x");
        assert_eq!(splitext_ext("x."), ".");
    }
}

/// The component scenarios, against Python's answers.
#[cfg(test)]
#[path = "component_tests.rs"]
mod scenario_tests;
