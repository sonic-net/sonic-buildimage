#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import importlib
import os
import pytest
import sys

from unittest.mock import patch

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True


@pytest.fixture
def adm1266_rtc_sync_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    # For files without .py extension, we need to use SourceFileLoader explicitly
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    adm1266_rtc_sync_path = os.path.join(TEST_DIR, "../../../common/utils/adm1266_rtc_sync")
    loader = importlib.machinery.SourceFileLoader("adm1266_rtc_sync", adm1266_rtc_sync_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    adm1266_rtc_sync_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(adm1266_rtc_sync_module)

    yield adm1266_rtc_sync_module


class TestAdm1266RtcSync:
    """Test class for adm1266_rtc_sync functionality."""

    def test_main_ok_with_no_dpms(self, adm1266_rtc_sync_module):
        # Given
        with (
            patch.object(adm1266_rtc_sync_module, "load_pd_plugin_config", return_value={}),
            patch.object(adm1266_rtc_sync_module, "load_pddf_device_config", return_value={}),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0

    def test_main_ok_with_non_adm1266_dpm(self, adm1266_rtc_sync_module):
        # Given
        with (
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm1": {"type": "unknown"},
                        "test-dpm2": {"type": "unknown"},
                    }
                },
            ),
            patch.object(adm1266_rtc_sync_module, "load_pddf_device_config", return_value={}),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0

    def test_main_ok_with_adm1266_dpm_and_rtc_epoch_offset_path(self, adm1266_rtc_sync_module):
        # Given
        written_value = None

        def mock_open_func(path, mode):
            nonlocal written_value
            if mode == "w" and "rtc_epoch_offset" in path:
                # Mock file object for writing
                class MockFile:
                    def write(self, value):
                        nonlocal written_value
                        written_value = value

                    def __enter__(self):
                        return self

                    def __exit__(self, *args):
                        pass

                return MockFile()
            # For other files, use the real open
            return open(path, mode)

        with (
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm": {
                            "type": "adm1266",
                            "dpm": "DPM1",
                        },
                    }
                },
            ),
            patch.object(
                adm1266_rtc_sync_module,
                "load_pddf_device_config",
                return_value={
                    "DPM1": {"i2c": {"topo_info": {"parent_bus": "0x7", "dev_addr": "0x41"}}}
                },
            ),
            patch("builtins.open", side_effect=mock_open_func),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0
            assert written_value == "1704067200"

    def test_main_fail_with_missing_dpm_field(self, adm1266_rtc_sync_module):
        # Given
        with (
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm": {
                            "type": "adm1266",
                            # Missing "dpm" field
                        },
                    }
                },
            ),
            patch.object(
                adm1266_rtc_sync_module,
                "load_pddf_device_config",
                return_value={
                    "DPM1": {"i2c": {"topo_info": {"parent_bus": "0x7", "dev_addr": "0x41"}}}
                },
            ),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 1

    def test_main_fail_when_some_dpms_fail(self, adm1266_rtc_sync_module):
        # Given
        written_value = None

        def mock_open_func(path, mode):
            nonlocal written_value
            if mode == "w" and "rtc_epoch_offset" in path:
                # Mock file object for writing
                class MockFile:
                    def write(self, value):
                        nonlocal written_value
                        written_value = value

                    def __enter__(self):
                        return self

                    def __exit__(self, *args):
                        pass

                return MockFile()
            # For other files, use the real open
            return open(path, mode)

        with (
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm": {
                            "type": "adm1266",
                            "dpm": "DPM1",
                        },
                        # Missing "dpm" field for test-dpm2
                        "test-dpm2": {
                            "type": "adm1266",
                        },
                    }
                },
            ),
            patch.object(
                adm1266_rtc_sync_module,
                "load_pddf_device_config",
                return_value={
                    "DPM1": {"i2c": {"topo_info": {"parent_bus": "0x7", "dev_addr": "0x41"}}}
                },
            ),
            patch("builtins.open", side_effect=mock_open_func),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 1
