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


//! `sonic_platform/vpd_parser.py`: `key: value` VPD files, re-read when their
//! modification time changes.

use std::sync::Mutex;
use std::time::SystemTime;

use platform_base::io;

use crate::utils::{self, Log};

pub const SN_VPD_FIELD: &str = "SN_VPD_FIELD";
pub const PN_VPD_FIELD: &str = "PN_VPD_FIELD";
pub const REV_VPD_FIELD: &str = "REV_VPD_FIELD";
pub const MFR_VPD_FIELD: &str = "MFR_NAME";

struct State {
    data: Vec<(String, String)>,
    mtime: Option<SystemTime>,
}

/// `VpdParser(file_path)`.
pub struct VpdParser {
    file: String,
    state: Mutex<State>,
}

impl VpdParser {
    pub fn new(file: &str) -> VpdParser {
        crate::imports::vpd_parser();
        VpdParser { file: file.to_string(), state: Mutex::new(State { data: Vec::new(), mtime: None }) }
    }

    /// `_get_data()`: False, and no data, when the file is not there.
    fn get_data(&self) -> bool {
        let mut st = utils::lock(&self.state);
        if !io::exists(&self.file) {
            st.data.clear();
            return false;
        }
        match std::fs::metadata(io::real(&self.file)).and_then(|m| m.modified()) {
            Ok(mtime) => {
                if Some(mtime) != st.mtime {
                    st.mtime = Some(mtime);
                    st.data = utils::read_key_value(&self.file, Log::Error, ":");
                }
                true
            }
            Err(_) => {
                st.data.clear();
                false
            }
        }
    }

    fn get(&self, key: &str) -> Option<String> {
        let st = utils::lock(&self.state);
        st.data.iter().find(|(k, _)| k == key).map(|(_, v)| v.clone())
    }

    fn field(&self, key: &str, what: &str, log: Log) -> String {
        if self.get_data() && self.get(key).is_none() {
            log.emit(&format!("Fail to read {what}: No key {key} in VPD {}", self.file));
            return "N/A".into();
        }
        self.get(key).unwrap_or_else(|| "N/A".into())
    }

    pub fn get_model(&self) -> String {
        self.field(PN_VPD_FIELD, "model number", Log::Error)
    }

    pub fn get_serial(&self) -> String {
        self.field(SN_VPD_FIELD, "serial number", Log::Error)
    }

    pub fn get_revision(&self) -> String {
        self.field(REV_VPD_FIELD, "revision", Log::Error)
    }

    pub fn get_entry_value(&self, key: &str) -> String {
        self.field(key, "vpd info", Log::Warning)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::testing::Root;

    /// Python's tests/mock_psu_vpd.
    const MOCK_PSU_VPD: &str = "SN_VPD_FIELD: MT1946X07684\nEFT_REV: 1\nPN_VPD_FIELD: MTEF-PSF-AC-C\n\
        REV_VPD_FIELD: A3\nMFG_DATE_FIELD: 1B94BF\nMFR_NAME: DELTA\nFEED: AC/DC\nCAPACITY: 1100\nMAX_RPM:23000\nMIN_RPM:4600\n";

    #[test]
    fn fields_come_from_the_vpd_file() {
        let root = Root::new();
        let v = VpdParser::new("/x/psu1_vpd");
        // No file: N/A, quietly.
        assert_eq!(v.get_model(), "N/A");
        assert_eq!(v.get_entry_value(MFR_VPD_FIELD), "N/A");
        root.write("/x/psu1_vpd", MOCK_PSU_VPD);
        assert_eq!(v.get_model(), "MTEF-PSF-AC-C");
        assert_eq!(v.get_serial(), "MT1946X07684");
        assert_eq!(v.get_revision(), "A3");
        assert_eq!(v.get_entry_value(MFR_VPD_FIELD), "DELTA");
        assert_eq!(v.get_entry_value("MAX_RPM"), "23000");
        // A key the file lacks: N/A, logged.
        assert_eq!(v.get_entry_value("NOPE"), "N/A");
        // A file that loses its keys.
        let w = VpdParser::new("/x/other");
        root.write("/x/other", "OTHER: 1\n");
        assert_eq!((w.get_model(), w.get_serial(), w.get_revision()), ("N/A".into(), "N/A".into(), "N/A".into()));
        // Re-read only when the modification time changes; gone, forgotten.
        std::fs::remove_file(root.real("/x/psu1_vpd")).unwrap();
        assert_eq!(v.get_serial(), "N/A");
    }

    #[test]
    fn the_file_is_re_read_only_when_its_mtime_changes() {
        let root = Root::new();
        root.write("/x/file", "SN_VPD_FIELD: 1\n");
        let v = VpdParser::new("/x/file");
        assert_eq!(v.get_serial(), "1");
        let mtime = std::fs::metadata(root.real("/x/file")).unwrap().modified().unwrap();
        // Same mtime: the old data, even though the content changed.
        root.write("/x/file", "SN_VPD_FIELD: 2\n");
        let f = std::fs::File::options().write(true).open(root.real("/x/file")).unwrap();
        f.set_modified(mtime).unwrap();
        assert_eq!(v.get_serial(), "1");
    }
}
