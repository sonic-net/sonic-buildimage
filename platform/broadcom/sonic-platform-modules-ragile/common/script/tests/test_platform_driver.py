#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for platform_driver.py — focused on removeDev and addDev
"""

import os
import sys
import pytest
from unittest.mock import patch, mock_open, MagicMock, call

# Mock platform-specific imports
import types
mock_config = types.ModuleType('platform_config')
mock_config.GLOBALCONFIG = {"DRIVERLISTS": [], "DEVS": [], "OPTOE": []}
mock_config.WARM_UPGRADE_STARTED_FLAG = "/tmp/.warm_upgrade_started"
mock_config.WARM_UPG_FLAG = "/tmp/.warm_upg_flag"
sys.modules['platform_config'] = mock_config

# Mock click
sys.modules['click'] = MagicMock()

import platform_driver as pd


class TestRemoveDev:
    """Tests for removeDev()"""

    @patch('os.path.exists', return_value=True)
    def test_removes_existing_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            pd.removeDev(10, 0x56)
        m.assert_called_once_with("/sys/bus/i2c/devices/i2c-10/delete_device", 'w')
        m().write.assert_called_once_with("0x56\n")

    @patch('os.path.exists', return_value=False)
    def test_skips_nonexistent_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            pd.removeDev(10, 0x56)
        m.assert_not_called()

    @patch('os.path.exists', return_value=True)
    def test_correct_devpath_format(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            pd.removeDev(5, 0x48)
        mock_exists.assert_called_with("/sys/bus/i2c/devices/5-0048")


class TestAddDev:
    """Tests for addDev()"""

    @patch('os.path.exists')
    def test_adds_new_device(self, mock_exists):
        # pdevpath exists, devpath does not
        mock_exists.side_effect = lambda p: p.endswith("/")
        m = mock_open()
        with patch("builtins.open", m):
            pd.addDev("tmp75", 10, 0x48)
        m.assert_called_once_with("/sys/bus/i2c/devices/i2c-10/new_device", 'w')
        m().write.assert_called_once_with("tmp75 0x48\n")

    @patch('os.path.exists', return_value=True)
    def test_skips_existing_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            pd.addDev("tmp75", 10, 0x48)
        m.assert_not_called()

    @patch('time.sleep')
    @patch('os.path.exists')
    def test_lm75_sleeps_before_add(self, mock_exists, mock_sleep):
        mock_exists.side_effect = lambda p: p.endswith("/")
        m = mock_open()
        with patch("builtins.open", m):
            pd.addDev("lm75", 10, 0x48)
        mock_sleep.assert_any_call(0.1)

    @patch('os.path.exists')
    def test_waits_for_parent_bus(self, mock_exists):
        # Simulate parent bus appearing after 3 attempts
        call_count = [0]
        def side_effect(path):
            if path.endswith("/"):
                call_count[0] += 1
                return call_count[0] >= 3
            return False  # devpath doesn't exist

        mock_exists.side_effect = side_effect
        m = mock_open()
        with patch("builtins.open", m), \
             patch('time.sleep'):
            pd.addDev("tmp75", 10, 0x48)
        m.assert_called_once()


class TestCheckDriver:
    """Tests for check_driver()"""

    @patch.object(pd, 'log_os_system', return_value=(0, "3"))
    def test_driver_loaded(self, mock_log):
        assert pd.check_driver() is True

    @patch.object(pd, 'log_os_system', return_value=(0, "0"))
    def test_driver_not_loaded(self, mock_log):
        assert pd.check_driver() is False

    @patch.object(pd, 'log_os_system', return_value=(1, ""))
    def test_command_fails(self, mock_log):
        assert pd.check_driver() is False


class TestChecksignaldriver:
    """Tests for checksignaldriver()"""

    @patch.object(pd, 'log_os_system', return_value=(0, "1"))
    def test_module_loaded(self, mock_log):
        assert pd.checksignaldriver("wb_io_dev") is True

    @patch.object(pd, 'log_os_system', return_value=(0, "0"))
    def test_module_not_loaded(self, mock_log):
        assert pd.checksignaldriver("wb_io_dev") is False

    @patch.object(pd, 'log_os_system', return_value=(1, ""))
    def test_command_fails(self, mock_log):
        assert pd.checksignaldriver("wb_io_dev") is False


class TestAddRemoveDriver:
    """Tests for adddriver() and removedriver()"""

    @patch.object(pd, 'log_os_system')
    @patch.object(pd, 'checksignaldriver', return_value=False)
    def test_adddriver_loads_module(self, mock_check, mock_log):
        pd.adddriver("wb_io_dev", 0)
        mock_log.assert_called_once_with("modprobe wb_io_dev")

    @patch.object(pd, 'log_os_system')
    @patch.object(pd, 'checksignaldriver', return_value=True)
    def test_adddriver_skips_loaded_module(self, mock_check, mock_log):
        pd.adddriver("wb_io_dev", 0)
        mock_log.assert_not_called()

    @patch.object(pd, 'log_os_system')
    @patch.object(pd, 'checksignaldriver', return_value=True)
    def test_removedriver_removes_module(self, mock_check, mock_log):
        pd.removedriver("wb_io_dev", 0)
        mock_log.assert_called_once_with("rmmod -f wb_io_dev")

    @patch.object(pd, 'log_os_system')
    @patch.object(pd, 'checksignaldriver', return_value=False)
    def test_removedriver_skips_unloaded_module(self, mock_check, mock_log):
        pd.removedriver("wb_io_dev", 0)
        mock_log.assert_not_called()
