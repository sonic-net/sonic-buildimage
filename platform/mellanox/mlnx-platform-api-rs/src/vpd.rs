//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// Apache-2.0
//

//! VPD parsing, mirroring `vpd_parser.py`.
//!
//! The file is a flat `KEY: value` list.  Python reaches it through
//! `utils.read_key_value_file()`, which splits the *stripped* file content into
//! lines and each line on its first `:`.  A line without one raises
//! `ValueError` inside the converter, `read_from_file` catches it and
//! substitutes the default — an **empty** dict.  So one malformed line does not
//! lose one entry, it loses the whole file, and every field falls back to
//! `N/A`.  That is reproduced here rather than tidied up: a parser that kept
//! the good lines would report a model number where Python reports `N/A`.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};
use std::time::SystemTime;

const PN_FIELD: &str = "PN_VPD_FIELD";
const SN_FIELD: &str = "SN_VPD_FIELD";
const REV_FIELD: &str = "REV_VPD_FIELD";
/// Manufacturer, read by the invalid-voltage workaround in `psu.rs`.
pub const MFR_FIELD: &str = "MFR_NAME";
/// PSU capacity in watts, read by the same workaround.
pub const CAPACITY_FIELD: &str = "CAPACITY";

/// Cached reader for one VPD file.
///
/// Python re-parses only when the file's mtime moves (`vpd_parser.py:43-47`);
/// a PSU's VPD is rewritten when the PSU is swapped, and `psud` asks for the
/// model every cycle.
#[derive(Debug)]
pub struct VpdParser {
    path: PathBuf,
    last_mtime: Option<SystemTime>,
    data: BTreeMap<String, String>,
}

impl VpdParser {
    pub fn new<P: AsRef<Path>>(path: P) -> Self {
        Self {
            path: path.as_ref().to_path_buf(),
            last_mtime: None,
            data: BTreeMap::new(),
        }
    }

    /// Python's `_get_data()`: refresh from disk if the mtime moved, and empty
    /// the cache when the file has gone away.
    fn refresh(&mut self) {
        let mtime = match std::fs::metadata(&self.path).and_then(|m| m.modified()) {
            Ok(m) => m,
            Err(_) => {
                // Python clears the cache here too, so a PSU pulled out stops
                // reporting the serial it had while it was in.
                self.data.clear();
                self.last_mtime = None;
                return;
            }
        };
        if self.last_mtime == Some(mtime) {
            return;
        }
        self.last_mtime = Some(mtime);
        self.data = std::fs::read_to_string(&self.path)
            .ok()
            .and_then(|s| parse(&s))
            .unwrap_or_default();
    }

    /// One entry, or `None` where Python returns the string `'N/A'`.
    pub fn get(&mut self, key: &str) -> Option<String> {
        self.refresh();
        self.data.get(key).cloned()
    }

    pub fn model(&mut self) -> Option<String> {
        self.get(PN_FIELD)
    }

    pub fn serial(&mut self) -> Option<String> {
        self.get(SN_FIELD)
    }

    pub fn revision(&mut self) -> Option<String> {
        self.get(REV_FIELD)
    }
}

/// `_key_value_converter` with `delimeter=':'`, over content Python has already
/// stripped.  `None` stands for the `ValueError` that empties the dict.
fn parse(content: &str) -> Option<BTreeMap<String, String>> {
    let mut out = BTreeMap::new();
    for line in content.trim().lines() {
        let (k, v) = line.split_once(':')?;
        out.insert(k.trim().to_string(), v.trim().to_string());
    }
    Some(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    fn write(dir: &tempfile::TempDir, body: &str) -> PathBuf {
        let p = dir.path().join("psu1_vpd");
        let mut f = std::fs::File::create(&p).unwrap();
        f.write_all(body.as_bytes()).unwrap();
        p
    }

    #[test]
    fn keys_and_values_are_stripped() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "PN_VPD_FIELD:  MTEF-PSF-AC-C  \n SN_VPD_FIELD :MT1919X00042\n");
        let mut v = VpdParser::new(p);
        assert_eq!(v.model().as_deref(), Some("MTEF-PSF-AC-C"));
        assert_eq!(v.serial().as_deref(), Some("MT1919X00042"));
    }

    /// A value may itself contain a colon; only the first one separates.
    #[test]
    fn only_the_first_colon_separates() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "MFR_NAME:DELTA:LLC\n");
        let mut v = VpdParser::new(p);
        assert_eq!(v.get(MFR_FIELD).as_deref(), Some("DELTA:LLC"));
    }

    /// The behaviour that matters: one bad line loses every field, because
    /// Python's converter raises and the default empty dict is substituted.
    #[test]
    fn one_malformed_line_empties_the_whole_file() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "PN_VPD_FIELD:MTEF\nthis line has no colon\nSN_VPD_FIELD:MT19\n");
        let mut v = VpdParser::new(p);
        assert_eq!(v.model(), None);
        assert_eq!(v.serial(), None);
    }

    #[test]
    fn a_missing_key_is_absent_not_an_error() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "PN_VPD_FIELD:MTEF\n");
        let mut v = VpdParser::new(p);
        assert_eq!(v.model().as_deref(), Some("MTEF"));
        assert_eq!(v.serial(), None);
        assert_eq!(v.revision(), None);
    }

    #[test]
    fn a_missing_file_reports_nothing() {
        let mut v = VpdParser::new("/nonexistent/psu1_vpd");
        assert_eq!(v.model(), None);
    }

    /// Python empties its cache when the file disappears, so a PSU that has
    /// been pulled stops reporting the serial it had while it was in.
    #[test]
    fn removing_the_file_clears_what_was_cached() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "SN_VPD_FIELD:MT1919X00042\n");
        let mut v = VpdParser::new(&p);
        assert_eq!(v.serial().as_deref(), Some("MT1919X00042"));
        std::fs::remove_file(&p).unwrap();
        assert_eq!(v.serial(), None);
    }

    /// Trailing blank lines survive the strip; an interior one would not, and
    /// that asymmetry is Python's.
    #[test]
    fn trailing_whitespace_does_not_break_the_file() {
        let d = tempfile::tempdir().unwrap();
        let p = write(&d, "\n\nPN_VPD_FIELD:MTEF\n\n\n");
        let mut v = VpdParser::new(p);
        assert_eq!(v.model().as_deref(), Some("MTEF"));
    }
}
