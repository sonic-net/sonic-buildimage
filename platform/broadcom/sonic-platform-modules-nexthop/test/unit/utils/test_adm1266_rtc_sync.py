#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import importlib
import os
import pytest
import sys

from fixtures.test_helpers_common import temp_file
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
        with patch.object(adm1266_rtc_sync_module, "load_pd_plugin_config", return_value={}):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0

    def test_main_ok_with_non_adm1266_dpm(self, adm1266_rtc_sync_module):
        # Given
        with patch.object(
            adm1266_rtc_sync_module,
            "load_pd_plugin_config",
            return_value={
                "DPM": {
                    "test-dpm1": {"type": "unknown"},
                    "test-dpm2": {"type": "unknown"},
                }
            },
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0

    def test_main_ok_with_adm1266_dpm_and_rtc_epoch_offset_path(self, adm1266_rtc_sync_module):
        # Given
        with (
            temp_file(content="") as rtc_epoch_offset_path,
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm": {
                            "type": "adm1266",
                            "nvmem_path": "/dummy/path",
                            "powerup_counter_path": "/dummy/path",
                            "rtc_epoch_offset_path": rtc_epoch_offset_path,
                        },
                    }
                },
            ),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 0
            with open(rtc_epoch_offset_path, "r") as file:
                assert file.read() == "1704067200"

    def test_main_fail_with_no_rtc_epoch_offset_path(self, adm1266_rtc_sync_module):
        # Given
        with patch.object(
            adm1266_rtc_sync_module,
            "load_pd_plugin_config",
            return_value={
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": "/dummy/path",
                        "powerup_counter_path": "/dummy/path",
                    },
                }
            },
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 1

    def test_main_fail_when_some_dpms_fail(self, adm1266_rtc_sync_module):
        # Given
        with (
            temp_file(content="") as rtc_epoch_offset_path,
            patch.object(
                adm1266_rtc_sync_module,
                "load_pd_plugin_config",
                return_value={
                    "DPM": {
                        "test-dpm": {
                            "type": "adm1266",
                            "nvmem_path": "/dummy/path",
                            "powerup_counter_path": "/dummy/path",
                            "rtc_epoch_offset_path": rtc_epoch_offset_path,
                        },
                        # No rtc_epoch_offset_path.
                        "test-dpm2": {
                            "type": "adm1266",
                            "nvmem_path": "/dummy/path",
                            "powerup_counter_path": "/dummy/path",
                        },
                    }
                },
            ),
        ):
            # When
            ret = adm1266_rtc_sync_module.main()
            # Then
            assert ret == 1
