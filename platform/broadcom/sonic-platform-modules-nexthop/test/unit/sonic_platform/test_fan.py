#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform fan module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/sonic_platform/test_fan.py -v
"""

import pytest
import sys

from fixtures.fake_swsscommon import FakeTable
from unittest.mock import Mock, patch, call


class MockPddfFan:
    """Mock implementation of PddfFan for testing."""

    # mock methods
    get_presence = Mock()
    set_speed = Mock(return_value=True)

    def __init__(
        self,
        tray_idx,
        fan_idx,
        pddf_data,
        pddf_plugin_data,
        is_psu_fan,
        psu_index,
    ):
        self.tray_idx = tray_idx + 1
        self.fan_index = fan_idx + 1
        self.pddf_data = pddf_data
        self.pddf_plugin_data = pddf_plugin_data
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.fans_psu_index = psu_index

    def get_name(self):
        return f"Fantray{self.tray_idx}_{self.fan_index}"


@pytest.fixture
def mock_pddf_fan():
    """Injects and returns a mock PddfFan for testing."""
    pddf_fan = Mock()
    pddf_fan.PddfFan = MockPddfFan
    with patch.dict(sys.modules, {"sonic_platform_pddf_base.pddf_fan": pddf_fan}):
        yield pddf_fan.PddfFan


@pytest.fixture
def fan_module(mock_pddf_fan):
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import fan

    yield fan


class TestFan:
    """Test class for Fan functionality."""

    def test_get_presence(self, mock_pddf_fan, fan_module):
        """Test get_presence."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        mock_pddf_fan.get_presence.return_value = True
        assert fan.get_presence() is True

        mock_pddf_fan.get_presence.return_value = False
        assert fan.get_presence() is False

    def test_get_model_for_present_non_psu_fan(self, mock_pddf_fan, fan_module):
        """Test get_model for present non-PSU fan."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        mock_pddf_fan.get_presence.return_value = True
        assert fan.get_model() == "FAN-80G1-F"

    def test_get_model_for_non_present_fan(self, mock_pddf_fan, fan_module):
        """Test get_model for non-present non-PSU fan."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        mock_pddf_fan.get_presence.return_value = False
        assert fan.get_model() == "N/A"

    def test_fan_init_ok_when_db_conn_fails(self, fan_module):
        """Test Fan initialization is ok when DB connection fails."""
        # Given
        DBConnector = sys.modules["swsscommon"].swsscommon.DBConnector
        with patch.object(DBConnector, "__init__", Mock(side_effect=RuntimeError)):
            # When
            fan = fan_module.Fan(tray_idx=0, fan_idx=0)
            # Then
            assert fan._state_fan_tbl is None

    def test_get_max_speed_default(self, fan_module):
        """Test get_max_speed when it hasn't been set."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)
        assert fan.get_max_speed() == fan._DEFAULT_MAX_SPEED

    def test_set_max_speed(self, fan_module):
        """Test set_max_speed writes data to STATE_DB."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        fan.set_max_speed(60.99)
        assert (
            FakeTable._global_db["STATE_DB"]["FAN_INFO"][fan.get_name()]["max_speed"]
            == "60.99"
        )

    def test_set_and_get_max_speed(self, fan_module):
        """Test setting and getting max speed."""
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        fan.set_max_speed(60.99)
        assert fan.get_max_speed() == 60.99

    def test_set_speed_is_clamped_by_max_speed(self, mock_pddf_fan, fan_module):
        """Test set_speed is clamped by the previously set max_speed."""
        # Given
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)
        fan.set_max_speed(60)

        # When
        fan.set_speed(61)
        fan.set_speed(100)
        fan.set_speed(59)
        fan.set_speed(30)

        # Then
        mock_pddf_fan.set_speed.assert_has_calls(
            [
                # Clamped to max speed
                call(fan, 60),
                call(fan, 60),
                # Within max speed
                call(fan, 59),
                call(fan, 30),
            ],
            any_order=False,
        )

    def test_set_and_get_max_speed_when_db_conn_fails(self, fan_module):
        """Test set_max_speed and get_max_speed change nothing when DB connection fails."""
        # Given
        DBConnector = sys.modules["swsscommon"].swsscommon.DBConnector
        with patch.object(DBConnector, "__init__", Mock(side_effect=RuntimeError)):
            fan = fan_module.Fan(tray_idx=0, fan_idx=0)
            # When/Then
            assert fan.set_max_speed(60.99) == False
            assert fan.get_max_speed() == fan._DEFAULT_MAX_SPEED

    def test_set_and_get_max_speed_when_db_conn_resumes(self, fan_module):
        """Test set_max_speed and get_max_speed working when DB connection resumes."""
        # Given - Inject a failed DB connection
        DBConnector = sys.modules["swsscommon"].swsscommon.DBConnector
        with patch.object(DBConnector, "__init__", Mock(side_effect=RuntimeError)):
            fan = fan_module.Fan(tray_idx=0, fan_idx=0)
            assert fan._state_fan_tbl is None

        # When - Revive the DB connector
        # Then - Perform set/get max speed should work
        fan.set_max_speed(60.99)
        assert fan._state_fan_tbl is not None
        assert (
            FakeTable._global_db["STATE_DB"]["FAN_INFO"][fan.get_name()][
                "max_speed"
            ]
            == "60.99"
        )
        assert fan.get_max_speed() == 60.99
