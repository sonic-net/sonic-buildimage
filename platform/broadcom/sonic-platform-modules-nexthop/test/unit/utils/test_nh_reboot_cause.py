#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test script for nh_reboot_cause utility.
This script sets up the necessary mocks and imports to test the CLI tool.
"""

import base64
import importlib
import json
import os
import pytest
import sys
import tempfile

from unittest.mock import Mock, patch

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True


class MockChassisBase:
    REBOOT_CAUSE_HARDWARE_OTHER = "Unknown"
    REBOOT_CAUSE_POWER_LOSS = "Power Loss"
    REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload"
    REBOOT_CAUSE_WATCHDOG = "Watchdog"

    def __init__(self, *args, **kwargs):
        pass

@pytest.fixture
def mock_chassis_base():
    """Injects and returns a mock ChassisBase for testing."""
    chassis_base = Mock()
    chassis_base.ChassisBase = MockChassisBase
    with patch.dict(sys.modules, {"sonic_platform_base.chassis_base": chassis_base}):
        yield chassis_base.ChassisBase


@pytest.fixture
def nh_reboot_cause_module(mock_chassis_base):
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    # For files without .py extension, we need to use SourceFileLoader explicitly
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    nh_reboot_cause_path = os.path.join(TEST_DIR, "../../../common/utils/nh_reboot_cause")
    loader = importlib.machinery.SourceFileLoader("nh_reboot_cause", nh_reboot_cause_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    nh_reboot_cause_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(nh_reboot_cause_module)

    yield nh_reboot_cause_module


def create_test_data():
    """Create test DPM fault records in the correct envelope format.

    The 'raw' field must be a base64-encoded string (as stored by Serializer)
    representing 64 bytes of blackbox data.
    """
    # Create a minimal 64-byte raw record (all zeros) and encode it
    raw_bytes = bytes(64)
    raw_encoded = 'base64:' + base64.b64encode(raw_bytes).decode('ascii')

    records = [
        {
            "dpm_name": "test-dpm-1",
            "fault_uid": "0x1234",
            "power_loss": "Yes",
            "timestamp": "2025-01-15 10:30:45",
            "dpm_fault": "PSU input power lost",
            "raw": raw_encoded,
        },
        {
            "dpm_name": "test-dpm-2",
            "fault_uid": "0x5678",
            "power_loss": "No",
            "timestamp": "2025-01-15 10:30:46",
            "dpm_fault": "Watchdog timeout",
            "raw": raw_encoded,
        }
    ]

    return {
        "dpm_type": "adm1266",
        "gen_time": "2025_01_15_10_30_45",
        "schema_version": 1,
        "records_json": records
    }


def test_show_current(nh_reboot_cause_module, capsys):
    """Test showing current reboot-cause."""
    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR
        nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

        try:
            # Create test data file
            test_data = create_test_data()
            timestamp = "2025_01_15_10_30_45"
            test_file = os.path.join(tmpdir, f"reboot-cause-{timestamp}.json")

            with open(test_file, 'w') as f:
                json.dump(test_data, f)

            # Create symlink to latest
            prev_link = os.path.join(tmpdir, "previous-reboot-cause.json")
            os.symlink(test_file, prev_link)

            # Test show_current and verify output
            nh_reboot_cause_module.show_current()
            captured = capsys.readouterr()

            assert "test-dpm-1" in captured.out, "Expected DPM name in output"
            assert "test-dpm-2" in captured.out, "Expected second DPM name in output"
            assert "0x1234" in captured.out, "Expected fault UID in output"
            assert "Unsupported DPM type" not in captured.out, "Should not show DPM type error"

        finally:
            nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir


def test_show_history(nh_reboot_cause_module, capsys):
    """Test showing reboot-cause history."""
    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR
        nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

        try:
            # Create multiple test data files
            test_data = create_test_data()
            timestamps = ["2025_01_15_10_30_45", "2025_01_15_11_45_30", "2025_01_15_14_20_15"]

            for ts in timestamps:
                test_file = os.path.join(tmpdir, f"reboot-cause-{ts}.json")
                with open(test_file, 'w') as f:
                    json.dump(test_data, f)

            # Create symlink to latest
            latest_file = os.path.join(tmpdir, f"reboot-cause-{timestamps[-1]}.json")
            prev_link = os.path.join(tmpdir, "previous-reboot-cause.json")
            os.symlink(latest_file, prev_link)

            # Test show_history and verify output
            nh_reboot_cause_module.show_history()
            captured = capsys.readouterr()

            assert captured.out.count("Logs recorded at") == len(timestamps), \
                f"Expected {len(timestamps)} history entries"
            assert "test-dpm-1" in captured.out, "Expected DPM name in history output"

        finally:
            nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir


def test_cli_help(nh_reboot_cause_module):
    """Test that the CLI command is properly configured with click."""
    cli = nh_reboot_cause_module.reboot_cause

    # Verify the command has help text
    assert cli.help is not None, "CLI command should have help text"
    assert "reboot-cause" in cli.help.lower(), "Help text should mention reboot-cause"

    # Verify --history option exists
    has_history_option = any(param.name == 'history' for param in cli.params)
    assert has_history_option, "CLI should have --history option"


def test_unsupported_dpm_type(nh_reboot_cause_module, capsys):
    """Test that unsupported DPM types are rejected."""
    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR
        nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

        try:
            # Create test data with wrong DPM type
            test_data = create_test_data()
            test_data["dpm_type"] = "unknown_dpm"
            timestamp = "2025_01_15_10_30_45"
            test_file = os.path.join(tmpdir, f"reboot-cause-{timestamp}.json")

            with open(test_file, 'w') as f:
                json.dump(test_data, f)

            prev_link = os.path.join(tmpdir, "previous-reboot-cause.json")
            os.symlink(test_file, prev_link)

            # Test show_current with wrong DPM type
            nh_reboot_cause_module.show_current()
            captured = capsys.readouterr()

            assert "Unsupported DPM type" in captured.out, "Should show DPM type error"
            assert "unknown_dpm" in captured.out, "Should mention the unsupported DPM type"

        finally:
            nh_reboot_cause_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir

