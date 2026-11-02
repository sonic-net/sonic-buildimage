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


//! Fixtures for the DPU, line card and thermal updater tests: a platform
//! under a test root, and the lines a `SysLogger` sends to its `/dev/log`.

use std::os::unix::net::UnixDatagram;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use crate::testing::Root;

/// The platform `name`, its `platform.json`, and hw-management's helper
/// module, which the thermal and SFP imports need.
pub fn platform(root: &Root, name: &str, platform_json: &str) {
    root.write("/host/machine.conf", &format!("onie_platform={name}\n"));
    root.write("/usr/share/sonic/platform/platform.json", platform_json);
    crate::hw_mgmt::testing::install_independent_mode_update(root);
}

/// Whether machine.conf names the platform: a `$PLATFORM` in the
/// environment wins over it, and a test that needs one particular platform
/// cannot run under another (see [`crate::testing::platform_from_env`]).
pub fn platform_settable() -> bool {
    !crate::testing::platform_from_env()
}

/// The root's `/dev/log`, where every `SysLogger` sends, drained as it
/// fills: a datagram socket holds only `net.unix.max_dgram_qlen` messages
/// (10 in a new network namespace) before a sender blocks.
pub struct Syslog {
    lines: Arc<Mutex<Vec<String>>>,
    idle: Arc<AtomicU64>,
    stop: Arc<AtomicBool>,
    reader: Option<std::thread::JoinHandle<()>>,
}

impl Syslog {
    pub fn listen(root: &Root) -> Syslog {
        let path = root.real("/dev/log");
        std::fs::create_dir_all(path.parent().expect("dev")).expect("dev");
        let _ = std::fs::remove_file(&path);
        let sock = UnixDatagram::bind(&path).expect("syslog socket");
        sock.set_read_timeout(Some(Duration::from_millis(5))).expect("timeout");
        let lines: Arc<Mutex<Vec<String>>> = Arc::default();
        let idle: Arc<AtomicU64> = Arc::default();
        let stop: Arc<AtomicBool> = Arc::default();
        let (l, i, st) = (lines.clone(), idle.clone(), stop.clone());
        let reader = std::thread::spawn(move || {
            let mut buf = [0u8; 8192];
            while !st.load(Ordering::Relaxed) {
                match sock.recv(&mut buf) {
                    Ok(n) => {
                        if let Some(line) = parse(&buf[..n]) {
                            l.lock().expect("lines").push(line);
                        }
                    }
                    Err(_) => {
                        i.fetch_add(1, Ordering::SeqCst);
                    }
                }
            }
        });
        Syslog { lines, idle, stop, reader: Some(reader) }
    }

    /// The lines sent so far, each `LEVEL message`, once the reader has
    /// found the socket empty twice since asked.
    pub fn take(&self) -> Vec<String> {
        let start = self.idle.load(Ordering::SeqCst);
        while self.idle.load(Ordering::SeqCst) < start + 2 {
            std::thread::sleep(Duration::from_millis(1));
        }
        std::mem::take(&mut *self.lines.lock().expect("lines"))
    }
}

impl Drop for Syslog {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Relaxed);
        if let Some(r) = self.reader.take() {
            let _ = r.join();
        }
    }
}

/// `<pri>name[pid]: message` as `LEVEL message`.
fn parse(datagram: &[u8]) -> Option<String> {
    let line = String::from_utf8_lossy(datagram).trim_end_matches('\0').to_string();
    let (pri, rest) = line.strip_prefix('<').and_then(|l| l.split_once('>'))?;
    let level = match pri.parse::<i32>().unwrap_or(0) % 8 {
        3 => "ERROR",
        4 => "WARNING",
        5 => "NOTICE",
        _ => "OTHER",
    };
    let msg = rest.split_once("]: ").map_or(rest, |(_, m)| m);
    Some(format!("{level} {msg}"))
}

/// The fake Redis under `root`, its config naming CHASSIS_STATE_DB (13) as
/// well as APPL_DB, CONFIG_DB and STATE_DB.
pub fn redis_with_chassis_db(root: &Root) -> &'static platform_base::testing::FakeRedis {
    let redis = platform_base::testing::FakeRedis::get();
    redis.install(root);
    root.write(
        platform_base::db::DB_CONFIG_FILE,
        &format!(
            r#"{{"INSTANCES": {{"redis": {{"hostname": "127.0.0.1", "port": {}, "unix_socket_path": "/var/run/redis/redis.sock"}}}},
"DATABASES": {{"APPL_DB": {{"id": 0, "separator": ":", "instance": "redis"}},
"CONFIG_DB": {{"id": 4, "separator": "|", "instance": "redis"}},
"STATE_DB": {{"id": 6, "separator": "|", "instance": "redis"}},
"CHASSIS_STATE_DB": {{"id": 13, "separator": "|", "instance": "redis"}}}}}}"#,
            redis.port()
        ),
    );
    redis
}
