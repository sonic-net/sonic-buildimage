#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test script for nh_reboot_cause utility.
This script sets up the necessary mocks and imports to test the CLI tool.
"""

import base64
import importlib.util
import json
import os
import sys
import tempfile
from unittest.mock import Mock

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True

def setup_test_environment():
    """Set up mocks and load modules using the fixture pattern."""

    # Mock sonic_platform_base.chassis_base that adm1266.py imports
    chassis_base_mock = Mock()
    chassis_base_mock.ChassisBase = Mock()
    chassis_base_mock.ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER = "Unknown"
    chassis_base_mock.ChassisBase.REBOOT_CAUSE_POWER_LOSS = "Power Loss"
    chassis_base_mock.ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload"
    chassis_base_mock.ChassisBase.REBOOT_CAUSE_WATCHDOG = "Watchdog"
    sys.modules["sonic_platform_base"] = Mock()
    sys.modules["sonic_platform_base.chassis_base"] = chassis_base_mock

    # Get the test directory (we're in test/unit/sonic_platform/)
    test_dir = os.path.dirname(os.path.realpath(__file__))

    # Load dpm module directly from file path
    dpm_path = os.path.join(test_dir, "../../../common/sonic_platform/dpm.py")
    spec = importlib.util.spec_from_file_location("sonic_platform.dpm", dpm_path)
    dpm_module = importlib.util.module_from_spec(spec)
    sys.modules["sonic_platform.dpm"] = dpm_module
    spec.loader.exec_module(dpm_module)

    # Load adm1266 module directly from file path
    adm1266_path = os.path.join(test_dir, "../../../common/sonic_platform/adm1266.py")
    spec = importlib.util.spec_from_file_location("sonic_platform.adm1266", adm1266_path)
    adm1266_module = importlib.util.module_from_spec(spec)
    sys.modules["sonic_platform.adm1266"] = adm1266_module
    spec.loader.exec_module(adm1266_module)

    # Now load the nh_reboot_cause utility (file has no .py extension)
    nh_reboot_cause_path = os.path.join(test_dir, "../../../common/utils/nh_reboot_cause")

    # For files without .py extension, we need to use SourceFileLoader explicitly
    from importlib.machinery import SourceFileLoader
    loader = SourceFileLoader("nh_reboot_cause", nh_reboot_cause_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    nh_reboot_cause_module = importlib.util.module_from_spec(spec)
    sys.modules["nh_reboot_cause"] = nh_reboot_cause_module
    spec.loader.exec_module(nh_reboot_cause_module)

    return nh_reboot_cause_module, dpm_module


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


def test_show_current(capsys):
    """Test showing current reboot-cause."""
    nh_reboot_cause_module, dpm_module = setup_test_environment()

    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = dpm_module.SystemDPMLogHistory.HISTORY_DIR
        dpm_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

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
            dpm_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir


def test_show_history(capsys):
    """Test showing reboot-cause history."""
    nh_reboot_cause_module, dpm_module = setup_test_environment()

    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = dpm_module.SystemDPMLogHistory.HISTORY_DIR
        dpm_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

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
            dpm_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir


def test_cli_help():
    """Test that the CLI command is properly configured with click."""
    nh_reboot_cause_module, _ = setup_test_environment()

    cli = nh_reboot_cause_module.reboot_cause

    # Verify the command has help text
    assert cli.help is not None, "CLI command should have help text"
    assert "reboot-cause" in cli.help.lower(), "Help text should mention reboot-cause"

    # Verify --history option exists
    has_history_option = any(param.name == 'history' for param in cli.params)
    assert has_history_option, "CLI should have --history option"


def test_unsupported_dpm_type(capsys):
    """Test that unsupported DPM types are rejected."""
    nh_reboot_cause_module, dpm_module = setup_test_environment()

    with tempfile.TemporaryDirectory() as tmpdir:
        original_history_dir = dpm_module.SystemDPMLogHistory.HISTORY_DIR
        dpm_module.SystemDPMLogHistory.HISTORY_DIR = tmpdir

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
            dpm_module.SystemDPMLogHistory.HISTORY_DIR = original_history_dir

