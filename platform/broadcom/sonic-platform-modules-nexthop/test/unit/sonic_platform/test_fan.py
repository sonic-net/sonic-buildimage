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
        # Given a fan
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        # When the PDDF fan reports it present
        mock_pddf_fan.get_presence.return_value = True
        # Then the fan is present
        assert fan.get_presence() is True

        # When the PDDF fan reports it absent
        mock_pddf_fan.get_presence.return_value = False
        # Then the fan is absent
        assert fan.get_presence() is False

    def test_get_model_for_present_non_psu_fan(self, mock_pddf_fan, fan_module):
        """Test get_model for present non-PSU fan."""
        # Given a non-PSU fan
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        # When it is present
        mock_pddf_fan.get_presence.return_value = True
        # Then its model is reported
        assert fan.get_model() == "FAN-80G1-F"

    def test_get_model_for_non_present_fan(self, mock_pddf_fan, fan_module):
        """Test get_model for non-present non-PSU fan."""
        # Given a non-PSU fan
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)

        # When it is not present
        mock_pddf_fan.get_presence.return_value = False
        # Then its model is N/A
        assert fan.get_model() == "N/A"

    def test_set_speed_passes_every_speed_through(self, mock_pddf_fan, fan_module):
        """The fan ceiling is the PID policy's max_speed now; set_speed does not clamp."""
        # Given a fan
        fan = fan_module.Fan(tray_idx=0, fan_idx=0)
        mock_pddf_fan.set_speed.reset_mock()

        # When speeds above and below the old 75% default cap are set
        fan.set_speed(100)
        fan.set_speed(30)

        # Then each reaches the PDDF fan unchanged
        # (Fan no longer overrides set_speed, so the class-level mock is called unbound.)
        assert mock_pddf_fan.set_speed.call_args_list == [call(100), call(30)]
