#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for monitor_fan.py
"""

import os
import sys
import time
import pytest
from unittest.mock import patch, MagicMock, PropertyMock

# Mock platform-specific imports
sys.modules['plat_hal'] = MagicMock()
sys.modules['plat_hal.interface'] = MagicMock()
sys.modules['plat_hal.baseutil'] = MagicMock()

import monitor_fan as mf


class TestInitLogger:
    """Tests for _init_logger()"""

    @patch('monitor_fan.RotatingFileHandler')
    @patch('subprocess.call')
    @patch('os.path.exists', return_value=False)
    def test_creates_log_directory_when_missing(self, mock_exists, mock_call, mock_handler):
        logger = mf._init_logger()
        mock_call.assert_any_call(["mkdir", "-p", os.path.dirname(mf.LOG_FILE)])
        mock_call.assert_any_call(["sync"])
        assert logger is not None

    @patch('monitor_fan.RotatingFileHandler')
    @patch('os.path.exists', return_value=True)
    def test_skips_mkdir_when_log_exists(self, mock_exists, mock_handler):
        with patch('subprocess.call') as mock_call:
            logger = mf._init_logger()
        mock_call.assert_not_called()
        assert logger is not None


class TestFan:
    """Tests for Fan class"""

    def setup_method(self):
        self.mock_interface = MagicMock()
        self.fan = mf.Fan("FAN1", self.mock_interface)

    def test_init(self):
        assert self.fan.name == "FAN1"
        assert self.fan.pre_present is False
        assert self.fan.pre_status is True
        assert self.fan.plugin_cnt == 0
        assert self.fan.plugout_cnt == 0

    def test_fan_dict_update_caches(self):
        self.mock_interface.get_fan_info.return_value = {"NAME": "FAN-A", "SN": "123"}
        self.fan.fan_dict_update()
        self.fan.fan_dict_update()  # second call should use cache
        self.mock_interface.get_fan_info.assert_called_once()

    def test_fan_dict_update_refreshes_after_interval(self):
        self.mock_interface.get_fan_info.return_value = {"NAME": "FAN-A", "SN": "123"}
        self.fan.fan_dict_update()
        self.fan.update_time = time.time() - 2  # simulate 2 seconds ago
        self.fan.fan_dict_update()
        assert self.mock_interface.get_fan_info.call_count == 2

    def test_get_model(self):
        self.mock_interface.get_fan_info.return_value = {"NAME": "FAN-MODEL-A", "SN": "123"}
        assert self.fan.get_model() == "FAN-MODEL-A"

    def test_get_serial(self):
        self.mock_interface.get_fan_info.return_value = {"NAME": "FAN-A", "SN": "SN12345"}
        assert self.fan.get_serial() == "SN12345"

    def test_get_presence(self):
        self.mock_interface.get_fan_presence.return_value = True
        assert self.fan.get_presence() is True
        self.mock_interface.get_fan_presence.assert_called_with("FAN1")

    def test_get_rotor_speed_normal(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 8000, "SpeedMax": 10000}
        }
        assert self.fan.get_rotor_speed("Rotor1") == 80

    def test_get_rotor_speed_over_100_capped(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 12000, "SpeedMax": 10000}
        }
        assert self.fan.get_rotor_speed("Rotor1") == 100

    def test_get_rotor_speed_string_value_returns_zero(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": "N/A", "SpeedMax": 10000}
        }
        assert self.fan.get_rotor_speed("Rotor1") == 0

    def test_get_rotor_speed_none_value_returns_zero(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": None, "SpeedMax": 10000}
        }
        assert self.fan.get_rotor_speed("Rotor1") == 0

    def test_get_rotor_speed_tolerance_normal(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Tolerance": 25}
        }
        assert self.fan.get_rotor_speed_tolerance("Rotor1") == 25

    def test_get_rotor_speed_tolerance_string_returns_default(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Tolerance": "N/A"}
        }
        assert self.fan.get_rotor_speed_tolerance("Rotor1") == 30

    def test_get_rotor_speed_tolerance_none_returns_default(self):
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Tolerance": None}
        }
        assert self.fan.get_rotor_speed_tolerance("Rotor1") == 30

    def test_get_target_speed(self):
        self.mock_interface.get_fan_speed_pwm.return_value = 75
        assert self.fan.get_target_speed() == 75

    def test_get_status_not_present(self):
        self.mock_interface.get_fan_presence.return_value = False
        assert self.fan.get_status() is False

    def test_get_status_speed_within_tolerance(self):
        self.mock_interface.get_fan_presence.return_value = True
        self.mock_interface.get_fan_rotor_number.return_value = 1
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 7500, "SpeedMax": 10000, "Tolerance": 30}
        }
        self.mock_interface.get_fan_speed_pwm.return_value = 75
        assert self.fan.get_status() is True

    def test_get_status_speed_too_high(self):
        self.mock_interface.get_fan_presence.return_value = True
        self.mock_interface.get_fan_rotor_number.return_value = 1
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 10000, "SpeedMax": 10000, "Tolerance": 10}
        }
        self.mock_interface.get_fan_speed_pwm.return_value = 50  # target 50%, actual 100%
        assert self.fan.get_status() is False

    def test_get_status_speed_too_low(self):
        self.mock_interface.get_fan_presence.return_value = True
        self.mock_interface.get_fan_rotor_number.return_value = 1
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 1000, "SpeedMax": 10000, "Tolerance": 10}
        }
        self.mock_interface.get_fan_speed_pwm.return_value = 80  # target 80%, actual 10%
        assert self.fan.get_status() is False

    def test_get_status_multiple_rotors(self):
        self.mock_interface.get_fan_presence.return_value = True
        self.mock_interface.get_fan_rotor_number.return_value = 2
        self.mock_interface.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 7500, "SpeedMax": 10000, "Tolerance": 30},
            "Rotor2": {"Speed": 7500, "SpeedMax": 10000, "Tolerance": 30},
        }
        self.mock_interface.get_fan_speed_pwm.return_value = 75
        assert self.fan.get_status() is True

    def test_get_direction(self):
        self.mock_interface.get_fan_info.return_value = {"NAME": "FAN-A", "SN": "123", "AirFlow": "intake"}
        assert self.fan.get_direction() == "intake"


class TestMonitorFan:
    """Tests for MonitorFan class"""

    def setup_method(self):
        self.mock_baseutil = MagicMock()
        self.mock_baseutil.get_monitor_config.return_value = {
            "monitor_fan_para": {
                "present_interval": 0.5,
                "status_interval": 5,
                "present_check_cnt": 3,
                "status_check_cnt": 3,
            }
        }
        with patch.object(mf, 'baseutil', self.mock_baseutil), \
             patch.object(mf, 'interface', MagicMock()), \
             patch.object(mf, '_init_logger', return_value=MagicMock()):
            self.monitor = mf.MonitorFan()

    def test_debug_init_debug_mode(self):
        with patch('os.path.exists', return_value=True):
            self.monitor.debug_init()
        self.monitor.logger.setLevel.assert_called_with(mf.logging.DEBUG)

    def test_debug_init_info_mode(self):
        with patch('os.path.exists', return_value=False):
            self.monitor.debug_init()
        self.monitor.logger.setLevel.assert_called_with(mf.logging.INFO)

    def test_fan_obj_init(self):
        self.monitor.int_case.get_fan_total_number.return_value = 3
        self.monitor.fan_obj_init()
        assert len(self.monitor.fan_obj_list) == 3
        assert self.monitor.fan_obj_list[0].name == "FAN1"
        assert self.monitor.fan_obj_list[2].name == "FAN3"

    def test_fan_airflow_check_match(self):
        fan_obj = MagicMock()
        fan_obj.name = "FAN1"
        fan_obj.get_direction.return_value = "intake"
        self.monitor.int_case.get_device_airflow.return_value = "intake"

        self.monitor.fan_airflow_check(fan_obj)
        self.monitor.logger.debug.assert_called()

    def test_fan_airflow_check_mismatch(self):
        fan_obj = MagicMock()
        fan_obj.name = "FAN1"
        fan_obj.get_direction.return_value = "intake"
        self.monitor.int_case.get_device_airflow.return_value = "exhaust"

        self.monitor.fan_airflow_check(fan_obj)
        self.monitor.logger.error.assert_called()

    def test_fan_plug_in_detected(self):
        fan_obj = mf.Fan("FAN1", MagicMock())
        fan_obj.pre_present = False
        fan_obj.int_case.get_fan_presence.return_value = True
        # Need 3 consecutive detections
        for _ in range(3):
            self.monitor.fan_plug_in_out_check(fan_obj)
        assert fan_obj.pre_present is True

    def test_fan_plug_out_detected(self):
        fan_obj = mf.Fan("FAN1", MagicMock())
        fan_obj.pre_present = True
        fan_obj.int_case.get_fan_presence.return_value = False
        for _ in range(3):
            self.monitor.fan_plug_in_out_check(fan_obj)
        assert fan_obj.pre_present is False

    def test_fan_plug_in_not_enough_count(self):
        fan_obj = mf.Fan("FAN1", MagicMock())
        fan_obj.pre_present = False
        fan_obj.int_case.get_fan_presence.return_value = True
        self.monitor.fan_plug_in_out_check(fan_obj)  # only 1 detection
        assert fan_obj.pre_present is False  # not yet confirmed

    def test_fan_status_normal_to_error(self):
        fan_obj = mf.Fan("FAN1", MagicMock())
        fan_obj.pre_status = True
        fan_obj.int_case.get_fan_presence.return_value = True
        fan_obj.int_case.get_fan_rotor_number.return_value = 1
        fan_obj.int_case.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 1000, "SpeedMax": 10000, "Tolerance": 10}
        }
        fan_obj.int_case.get_fan_speed_pwm.return_value = 80  # way off
        for _ in range(3):
            self.monitor.fan_status_check(fan_obj)
        assert fan_obj.pre_status is False

    def test_fan_status_error_to_normal(self):
        fan_obj = mf.Fan("FAN1", MagicMock())
        fan_obj.pre_status = False
        fan_obj.int_case.get_fan_presence.return_value = True
        fan_obj.int_case.get_fan_rotor_number.return_value = 1
        fan_obj.int_case.get_fan_info_rotor.return_value = {
            "Rotor1": {"Speed": 7500, "SpeedMax": 10000, "Tolerance": 30}
        }
        fan_obj.int_case.get_fan_speed_pwm.return_value = 75
        for _ in range(3):
            self.monitor.fan_status_check(fan_obj)
        assert fan_obj.pre_status is True

    def test_checkFanPresence_iterates_all(self):
        fan1 = MagicMock()
        fan2 = MagicMock()
        self.monitor.fan_obj_list = [fan1, fan2]
        with patch.object(self.monitor, 'fan_plug_in_out_check') as mock_check:
            self.monitor.checkFanPresence()
        assert mock_check.call_count == 2

    def test_checkFanStatus_iterates_all(self):
        fan1 = MagicMock()
        fan2 = MagicMock()
        self.monitor.fan_obj_list = [fan1, fan2]
        with patch.object(self.monitor, 'fan_status_check') as mock_check:
            self.monitor.checkFanStatus()
        assert mock_check.call_count == 2
