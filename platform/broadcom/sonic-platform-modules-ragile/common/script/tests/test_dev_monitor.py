#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for dev_monitor.py — focused on removeDev and addDev
"""

import os
import sys
import pytest
from unittest.mock import patch, mock_open, MagicMock, call

# Mock platform-specific imports
mock_config = MagicMock()
mock_config.DEV_MONITOR_PARAM = {}
sys.modules['platform_config'] = mock_config
sys.modules['platform_util'] = MagicMock()
sys.modules['click'] = MagicMock()

import dev_monitor as dm


class TestDevMonitorRemoveDev:
    """Tests for DevMonitor.removeDev()"""

    def setup_method(self):

        self.monitor = dm.DevMonitor.__new__(dm.DevMonitor)
        self.monitor.logger = MagicMock()

    @patch('os.path.exists', return_value=True)
    def test_removes_existing_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.removeDev(10, 0x56)
        m.assert_called_once_with("/sys/bus/i2c/devices/i2c-10/delete_device", 'w')
        m().write.assert_called_once_with("0x56\n")

    @patch('os.path.exists', return_value=False)
    def test_skips_nonexistent_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.removeDev(10, 0x56)
        m.assert_not_called()

    @patch('os.path.exists', return_value=True)
    def test_correct_devpath_format(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.removeDev(5, 0x48)
        mock_exists.assert_called_with("/sys/bus/i2c/devices/5-0048")


class TestDevMonitorAddDev:
    """Tests for DevMonitor.addDev()"""

    def setup_method(self):

        self.monitor = dm.DevMonitor.__new__(dm.DevMonitor)
        self.monitor.logger = MagicMock()

    @patch('os.path.exists', return_value=False)
    def test_adds_new_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.addDev("tmp75", 10, 0x48)
        m.assert_called_once_with("/sys/bus/i2c/devices/i2c-10/new_device", 'w')
        m().write.assert_called_once_with("tmp75 0x48\n")

    @patch('os.path.exists', return_value=True)
    def test_skips_existing_device(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.addDev("tmp75", 10, 0x48)
        m.assert_not_called()

    @patch('time.sleep')
    @patch('os.path.exists', return_value=False)
    def test_lm75_sleeps_before_add(self, mock_exists, mock_sleep):
        m = mock_open()
        with patch("builtins.open", m):
            self.monitor.addDev("lm75", 10, 0x48)
        mock_sleep.assert_called_once_with(0.1)

    @patch('os.path.exists', return_value=False)
    def test_non_lm75_no_sleep(self, mock_exists):
        m = mock_open()
        with patch("builtins.open", m), \
             patch('time.sleep') as mock_sleep:
            self.monitor.addDev("tmp75", 10, 0x48)
        mock_sleep.assert_not_called()
