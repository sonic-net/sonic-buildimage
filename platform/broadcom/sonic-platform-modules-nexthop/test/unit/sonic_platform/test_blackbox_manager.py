#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for BlackBoxManager write dedup and logged-record lookup."""

import pytest


@pytest.fixture
def manager_module():
    """Loads the module after conftest.py has injected deps."""
    from sonic_platform import blackbox_manager

    yield blackbox_manager


def _make_manager(manager_module, tmp_path, *, dcdc_filter=True, psu_filter=False):
    """Builds a BlackBoxManager backed by real temp-dir loggers for two sources.

    Both log_filter values are configurable, mirroring pd-plugin.json; tests set
    them per case rather than assuming a fixed record count or filter setting.
    """
    plugin = {
        "BLACKBOX": {
            "dcdc": {"log_path": str(tmp_path / "dcdc"), "discard_duplicate_logs": dcdc_filter},
            "psu": {"log_path": str(tmp_path / "psu"), "discard_duplicate_logs": psu_filter},
        }
    }
    return manager_module.BlackBoxManager(None, plugin, devices={"dcdc": [], "psu": []})


def _payloads(manager, source):
    """Stored snapshot payloads for a source, oldest-first (skip markers dropped)."""
    snapshots, _, _ = manager.read_all_blackbox_logs(source)
    return [s.payload for s in snapshots if hasattr(s, "payload")]


class TestWriteBlackboxLog:
    def test_filtered_skips_duplicate_records(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        assert m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa"]}) is True

        written = m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa"]})

        assert written is False
        assert _payloads(m, "dcdc") == [{"DCDC0:raa228234": ["aa"]}]

    def test_filtered_logs_only_new_records(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa"]})

        written = m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa", "bb"]})

        assert written is True
        assert _payloads(m, "dcdc") == [
            {"DCDC0:raa228234": ["aa"]},
            {"DCDC0:raa228234": ["bb"]},
        ]

    def test_filtered_multi_record_device_does_not_oscillate(self, manager_module, tmp_path):
        # A filtered source with a MULTI-record device (log_filter is configurable,
        # so this is a valid config). Once every record has been logged, repeated
        # identical polls must be no-ops -- no record may be re-logged.
        m = _make_manager(manager_module, tmp_path)
        assert m.write_blackbox_log("dcdc", {"D:raa228234": ["aa"]}) is True
        assert m.write_blackbox_log("dcdc", {"D:raa228234": ["aa", "bb"]}) is True

        for _ in range(5):
            assert m.write_blackbox_log("dcdc", {"D:raa228234": ["aa", "bb"]}) is False

        # Every record logged exactly once across the two snapshots.
        assert _payloads(m, "dcdc") == [
            {"D:raa228234": ["aa"]},
            {"D:raa228234": ["bb"]},
        ]

    def test_filtered_multi_device_does_not_oscillate(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa"], "DCDC1:raa228234": ["bb"]})
        assert m.write_blackbox_log(
            "dcdc", {"DCDC0:raa228234": ["aa"], "DCDC1:raa228234": ["cc"]}
        ) is True

        for _ in range(5):
            assert m.write_blackbox_log(
                "dcdc", {"DCDC0:raa228234": ["aa"], "DCDC1:raa228234": ["cc"]}
            ) is False

        assert _payloads(m, "dcdc") == [
            {"DCDC0:raa228234": ["aa"], "DCDC1:raa228234": ["bb"]},
            {"DCDC1:raa228234": ["cc"]},
        ]

    def test_filtered_relogs_record_after_it_ages_out(self, manager_module, tmp_path):
        # Dedup is bounded by what remains in the log; a record only re-logs if it
        # is no longer present in any retained snapshot. With nothing aging out
        # here, an old record stays suppressed.
        m = _make_manager(manager_module, tmp_path)
        m.write_blackbox_log("dcdc", {"D:raa228234": ["aa"]})
        m.write_blackbox_log("dcdc", {"D:raa228234": ["aa", "bb"]})
        assert m.write_blackbox_log("dcdc", {"D:raa228234": ["aa"]}) is False

    def test_unfiltered_always_writes(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)

        assert m.write_blackbox_log("psu", {"PSU1:murata": ["aa"]}) is True
        assert m.write_blackbox_log("psu", {"PSU1:murata": ["aa"]}) is True

        assert _payloads(m, "psu") == [
            {"PSU1:murata": ["aa"]},
            {"PSU1:murata": ["aa"]},
        ]

    def test_multi_record_source_can_be_unfiltered(self, manager_module, tmp_path):
        # A multi-record source with log_filter False writes the full payload every
        # time (dedup disabled) regardless of record count.
        m = _make_manager(manager_module, tmp_path)
        m.write_blackbox_log("psu", {"PSU1:murata": ["aa", "bb"]})
        m.write_blackbox_log("psu", {"PSU1:murata": ["aa", "bb"]})
        assert _payloads(m, "psu") == [
            {"PSU1:murata": ["aa", "bb"]},
            {"PSU1:murata": ["aa", "bb"]},
        ]

    def test_empty_payload_not_written(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        assert m.write_blackbox_log("dcdc", {}) is False
        assert _payloads(m, "dcdc") == []

    def test_unknown_source_not_written(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        assert m.write_blackbox_log("bogus", {"X:y": ["aa"]}) is False


class TestLoggedRecords:
    def test_returns_all_logged_records_per_device(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa"], "DCDC1:raa228234": ["bb"]})
        m.write_blackbox_log("dcdc", {"DCDC0:raa228234": ["aa", "cc"], "DCDC1:raa228234": ["bb"]})

        assert m._logged_records("dcdc") == {
            "DCDC0:raa228234": {"aa", "cc"},
            "DCDC1:raa228234": {"bb"},
        }

    def test_empty_when_no_logs(self, manager_module, tmp_path):
        m = _make_manager(manager_module, tmp_path)
        assert m._logged_records("dcdc") == {}
