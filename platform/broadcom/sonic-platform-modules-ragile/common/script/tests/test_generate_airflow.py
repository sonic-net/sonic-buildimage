#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
"""
Unit tests for generate_airflow.py
"""

import os
import sys
import json
import pytest
from unittest.mock import patch, mock_open, MagicMock

# Mock platform-specific imports before importing the module under test
sys.modules['platform_config'] = MagicMock()
sys.modules['platform_util'] = MagicMock()
sys.modules['eepromutil'] = MagicMock()
sys.modules['eepromutil.fru'] = MagicMock()
sys.modules['eepromutil.fantlv'] = MagicMock()

# Now we can import
import generate_airflow as ga


class TestGetBoardAirFlow:
    """Tests for get_board_air_flow()"""

    def test_all_zeros_returns_na(self):
        with patch.object(ga, 'airflow_error'):
            result = ga.get_board_air_flow(0, 0, 0, 0)
        assert result == "N/A"

    def test_more_intake_fans_returns_intake(self):
        result = ga.get_board_air_flow(3, 1, 0, 0)
        assert result == "intake"

    def test_more_exhaust_fans_returns_exhaust(self):
        result = ga.get_board_air_flow(1, 3, 0, 0)
        assert result == "exhaust"

    def test_equal_fans_more_intake_psus_returns_intake(self):
        result = ga.get_board_air_flow(2, 2, 2, 1)
        assert result == "intake"

    def test_equal_fans_more_exhaust_psus_returns_exhaust(self):
        result = ga.get_board_air_flow(2, 2, 1, 2)
        assert result == "exhaust"

    def test_all_equal_returns_intake(self):
        result = ga.get_board_air_flow(2, 2, 2, 2)
        assert result == "intake"

    def test_only_fans_intake(self):
        result = ga.get_board_air_flow(4, 0, 0, 0)
        assert result == "intake"

    def test_only_fans_exhaust(self):
        result = ga.get_board_air_flow(0, 4, 0, 0)
        assert result == "exhaust"

    def test_only_psus_intake(self):
        result = ga.get_board_air_flow(0, 0, 2, 0)
        assert result == "intake"

    def test_only_psus_exhaust(self):
        result = ga.get_board_air_flow(0, 0, 0, 2)
        assert result == "exhaust"

    def test_fans_override_psus(self):
        result = ga.get_board_air_flow(3, 1, 0, 4)
        assert result == "intake"


class TestGetModelFru:
    """Tests for get_model_fru()"""

    def test_success(self):
        device = {"name": "FAN1", "area": "boardInfoArea", "field": "boardPartNumber"}
        mock_fru = MagicMock()
        mock_area = MagicMock()
        mock_area.boardPartNumber = "M1HFAN-I-F"
        mock_fru.boardInfoArea = mock_area

        with patch.object(ga, 'ipmifru', return_value=mock_fru):
            status, model = ga.get_model_fru(device, b'\x00' * 256)
        assert status is True
        assert model == "M1HFAN-I-F"

    def test_invalid_area(self):
        device = {"name": "FAN1", "area": "nonExistentArea", "field": "boardPartNumber"}
        mock_fru = MagicMock()
        mock_fru.nonExistentArea = None

        with patch.object(ga, 'ipmifru', return_value=mock_fru):
            status, msg = ga.get_model_fru(device, b'\x00' * 256)
        assert status is False
        assert "area config error" in msg

    def test_invalid_field(self):
        device = {"name": "FAN1", "area": "boardInfoArea", "field": "nonExistentField"}
        mock_fru = MagicMock()
        mock_area = MagicMock(spec=[])
        mock_fru.boardInfoArea = mock_area

        with patch.object(ga, 'ipmifru', return_value=mock_fru):
            status, msg = ga.get_model_fru(device, b'\x00' * 256)
        assert status is False
        assert "get model error" in msg

    def test_exception(self):
        device = {"name": "FAN1", "area": "boardInfoArea", "field": "boardPartNumber"}

        with patch.object(ga, 'ipmifru', side_effect=Exception("decode error")):
            status, msg = ga.get_model_fru(device, b'\x00' * 256)
        assert status is False
        assert "decode error" in msg


class TestGetModelFantlv:
    """Tests for get_model_fantlv()"""

    def test_success(self):
        device = {"name": "FAN1", "field": "Model"}
        mock_tlv = MagicMock()
        mock_tlv.decode.return_value = [{"name": "Model", "value": "FAN-INTAKE-001"}]

        with patch.object(ga, 'fan_tlv', return_value=mock_tlv):
            status, model = ga.get_model_fantlv(device, b'\x00' * 256)
        assert status is True
        assert model == "FAN-INTAKE-001"

    def test_empty_decode(self):
        device = {"name": "FAN1", "field": "Model"}
        mock_tlv = MagicMock()
        mock_tlv.decode.return_value = []

        with patch.object(ga, 'fan_tlv', return_value=mock_tlv):
            status, msg = ga.get_model_fantlv(device, b'\x00' * 256)
        assert status is False
        assert "decode fantlv eeprom info error" in msg

    def test_field_not_found(self):
        device = {"name": "FAN1", "field": "Model"}
        mock_tlv = MagicMock()
        mock_tlv.decode.return_value = [{"name": "Serial", "value": "123"}]

        with patch.object(ga, 'fan_tlv', return_value=mock_tlv):
            status, msg = ga.get_model_fantlv(device, b'\x00' * 256)
        assert status is False
        assert "not found" in msg

    def test_exception(self):
        device = {"name": "FAN1", "field": "Model"}
        mock_tlv = MagicMock()
        mock_tlv.decode.side_effect = Exception("tlv error")

        with patch.object(ga, 'fan_tlv', return_value=mock_tlv):
            status, msg = ga.get_model_fantlv(device, b'\x00' * 256)
        assert status is False
        assert "tlv error" in msg


class TestGetDeviceModele:
    """Tests for get_device_modele()"""

    def test_unsupported_e2_type(self):
        device = {"name": "FAN1", "e2_type": "unknown"}
        status, msg = ga.get_device_modele(device)
        assert status is False
        assert "unsupport e2_type" in msg

    def test_eeprom_read_failure(self):
        device = {"name": "FAN1", "e2_type": "fru", "e2_path": "/dev/fake", "e2_size": 256}

        with patch.object(ga, 'dev_file_read', return_value=(False, "read error")):
            status, msg = ga.get_device_modele(device)
        assert status is False
        assert "eeprom read error" in msg

    def test_fru_type_calls_get_model_fru(self):
        device = {"name": "FAN1", "e2_type": "fru", "e2_path": "/dev/fake", "e2_size": 256,
                  "area": "boardInfoArea", "field": "boardPartNumber"}

        with patch.object(ga, 'dev_file_read', return_value=(True, b'\x00' * 256)), \
             patch.object(ga, 'byteTostr', return_value="eeprom_data"), \
             patch.object(ga, 'get_model_fru', return_value=(True, "MODEL-A")) as mock_fru:
            status, model = ga.get_device_modele(device)
        assert status is True
        assert model == "MODEL-A"
        mock_fru.assert_called_once()

    def test_fantlv_type_calls_get_model_fantlv(self):
        device = {"name": "FAN1", "e2_type": "fantlv", "e2_path": "/dev/fake", "e2_size": 256,
                  "field": "Model"}

        with patch.object(ga, 'dev_file_read', return_value=(True, b'\x00' * 256)), \
             patch.object(ga, 'byteTostr', return_value="eeprom_data"), \
             patch.object(ga, 'get_model_fantlv', return_value=(True, "MODEL-B")) as mock_fantlv:
            status, model = ga.get_device_modele(device)
        assert status is True
        assert model == "MODEL-B"
        mock_fantlv.assert_called_once()

    def test_default_e2_size(self):
        device = {"name": "FAN1", "e2_type": "fru", "e2_path": "/dev/fake",
                  "area": "boardInfoArea", "field": "boardPartNumber"}

        with patch.object(ga, 'dev_file_read', return_value=(True, b'\x00' * 256)) as mock_read, \
             patch.object(ga, 'byteTostr', return_value="data"), \
             patch.object(ga, 'get_model_fru', return_value=(True, "MODEL")):
            ga.get_device_modele(device)
        mock_read.assert_called_once_with("/dev/fake", 0, 256)


class TestDebugInit:
    """Tests for debug_init()"""

    def test_sets_debuglevel_from_file(self):
        with patch("builtins.open", mock_open(read_data="3")):
            ga.debug_init()
        assert ga.debuglevel == 3

    def test_defaults_to_zero_on_missing_file(self):
        with patch("builtins.open", side_effect=FileNotFoundError):
            ga.debug_init()
        assert ga.debuglevel == 0

    def test_defaults_to_zero_on_invalid_content(self):
        with patch("builtins.open", mock_open(read_data="abc")):
            ga.debug_init()
        assert ga.debuglevel == 0


class TestGenerateAirflow:
    """Tests for generate_airflow()"""

    def setup_method(self):
        self.air_flow_conf = {
            "fans": [
                {"name": "FAN1", "e2_type": "fru", "e2_path": "/dev/fan1",
                 "e2_size": 256, "area": "boardInfoArea", "field": "boardPartNumber",
                 "decode": "fan_airflow"},
                {"name": "FAN2", "e2_type": "fru", "e2_path": "/dev/fan2",
                 "e2_size": 256, "area": "boardInfoArea", "field": "boardPartNumber",
                 "decode": "fan_airflow"},
            ],
            "psus": [
                {"name": "PSU1", "e2_type": "fru", "e2_path": "/dev/psu1",
                 "e2_size": 256, "area": "boardInfoArea", "field": "boardPartNumber",
                 "decode": "psu_airflow"},
            ],
            "fan_airflow": {
                "intake": ["MODEL-INTAKE"],
                "exhaust": ["MODEL-EXHAUST"],
            },
            "psu_airflow": {
                "intake": ["PSU-INTAKE"],
                "exhaust": ["PSU-EXHAUST"],
            },
        }

    @patch.object(ga, 'AIRFLOW_RESULT_FILE', '/tmp/test_airflow_result.json')
    def test_all_intake(self):
        ga.AIR_FLOW_CONF = self.air_flow_conf

        with patch.object(ga, 'get_device_modele') as mock_model, \
             patch('os.system'):
            mock_model.side_effect = [
                (True, "MODEL-INTAKE"),
                (True, "MODEL-INTAKE"),
                (True, "PSU-INTAKE"),
            ]
            ga.generate_airflow()

        with open('/tmp/test_airflow_result.json', 'r') as f:
            result = json.load(f)

        assert result["FAN1"]["airflow"] == "intake"
        assert result["FAN2"]["airflow"] == "intake"
        assert result["PSU1"]["airflow"] == "intake"
        assert result["board"] == "intake"
        os.remove('/tmp/test_airflow_result.json')

    @patch.object(ga, 'AIRFLOW_RESULT_FILE', '/tmp/test_airflow_result.json')
    def test_mixed_airflow(self):
        ga.AIR_FLOW_CONF = self.air_flow_conf

        with patch.object(ga, 'get_device_modele') as mock_model, \
             patch('os.system'):
            mock_model.side_effect = [
                (True, "MODEL-INTAKE"),
                (True, "MODEL-EXHAUST"),
                (True, "PSU-INTAKE"),
            ]
            ga.generate_airflow()

        with open('/tmp/test_airflow_result.json', 'r') as f:
            result = json.load(f)

        assert result["FAN1"]["airflow"] == "intake"
        assert result["FAN2"]["airflow"] == "exhaust"
        assert result["board"] == "intake"
        os.remove('/tmp/test_airflow_result.json')

    @patch.object(ga, 'AIRFLOW_RESULT_FILE', '/tmp/test_airflow_result.json')
    def test_fan_eeprom_read_failure(self):
        ga.AIR_FLOW_CONF = self.air_flow_conf

        with patch.object(ga, 'get_device_modele') as mock_model, \
             patch('os.system'):
            mock_model.side_effect = [
                (False, "read error"),
                (True, "MODEL-EXHAUST"),
                (True, "PSU-EXHAUST"),
            ]
            ga.generate_airflow()

        with open('/tmp/test_airflow_result.json', 'r') as f:
            result = json.load(f)

        assert result["FAN1"]["model"] == "N/A"
        assert result["FAN1"]["airflow"] == "N/A"
        assert result["board"] == "exhaust"
        os.remove('/tmp/test_airflow_result.json')

    @patch.object(ga, 'AIRFLOW_RESULT_FILE', '/tmp/test_airflow_result.json')
    def test_unknown_model(self):
        ga.AIR_FLOW_CONF = self.air_flow_conf

        with patch.object(ga, 'get_device_modele') as mock_model, \
             patch('os.system'):
            mock_model.side_effect = [
                (True, "UNKNOWN-MODEL"),
                (True, "UNKNOWN-MODEL"),
                (True, "UNKNOWN-MODEL"),
            ]
            ga.generate_airflow()

        with open('/tmp/test_airflow_result.json', 'r') as f:
            result = json.load(f)

        assert result["FAN1"]["airflow"] == "N/A"
        assert result["board"] == "N/A"
        os.remove('/tmp/test_airflow_result.json')

    @patch.object(ga, 'AIRFLOW_RESULT_FILE', '/tmp/test_airflow_subdir/test_airflow.json')
    def test_creates_output_directory(self):
        ga.AIR_FLOW_CONF = {"fans": [], "psus": []}

        ga.generate_airflow()

        assert os.path.exists('/tmp/test_airflow_subdir/test_airflow.json')
        os.remove('/tmp/test_airflow_subdir/test_airflow.json')
        os.rmdir('/tmp/test_airflow_subdir')


class TestLogging:
    """Tests for logging functions"""

    @patch('syslog.openlog')
    @patch('syslog.syslog')
    def test_airflow_info(self, mock_syslog, mock_openlog):
        ga.airflow_info("test message")
        mock_openlog.assert_called_once_with("AIRFLOW", ga.syslog.LOG_PID)
        mock_syslog.assert_called_once_with(ga.syslog.LOG_INFO, "test message")

    @patch('syslog.openlog')
    @patch('syslog.syslog')
    def test_airflow_error(self, mock_syslog, mock_openlog):
        ga.airflow_error("error message")
        mock_openlog.assert_called_once_with("AIRFLOW", ga.syslog.LOG_PID)
        mock_syslog.assert_called_once_with(ga.syslog.LOG_ERR, "error message")

    @patch('syslog.openlog')
    @patch('syslog.syslog')
    def test_airflow_debug_when_enabled(self, mock_syslog, mock_openlog):
        ga.debuglevel = ga.AIRFLOWDEBUG
        ga.airflow_debug("debug msg")
        mock_syslog.assert_called_once()
        ga.debuglevel = 0

    @patch('syslog.syslog')
    def test_airflow_debug_when_disabled(self, mock_syslog):
        ga.debuglevel = 0
        ga.airflow_debug("debug msg")
        mock_syslog.assert_not_called()
