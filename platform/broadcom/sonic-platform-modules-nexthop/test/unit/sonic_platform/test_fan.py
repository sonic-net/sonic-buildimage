#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform fan module.
These tests run in isolation from the SONiC environment using pytest:
python -m pytest test/unit/sonic_platform/test_fan.py -v
"""

import importlib.util
import pytest
import os
import sys

from fixtures.fake_swsscommon import FakeDBConnector, FakeTable, setup_fake_swsscommon
from unittest.mock import Mock, patch, call


@pytest.fixture(scope="session", autouse=True)
def setup_fan_unit_tests():
    setup_fake_swsscommon()
    yield


class MockPddfFan:
    """Mock implemnentation of PddfFan for testing."""

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


@pytest.fixture(scope="session")
def mock_pddf_fan():
    """Fixture providing a mock PddfFan instance for testing."""
    return MockPddfFan


@pytest.fixture(scope="session")
def fan(mock_pddf_fan):
    """
    Fixture providing a Fan instance for testing.
    This fixture loads the fan module directly to avoid package import issues.
    """
    # Set up the PddfFan mock
    pddf_fan_module = Mock()
    pddf_fan_module.PddfFan = mock_pddf_fan
    sys.modules["sonic_platform_pddf_base.pddf_fan"] = pddf_fan_module

    # Load the fan module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    fan_path = os.path.join(test_dir, "../../../common/sonic_platform/fan.py")

    spec = importlib.util.spec_from_file_location("fan", fan_path)
    fan_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fan_module)

    return fan_module.Fan(tray_idx=0, fan_idx=0)


class TestFan:
    """Test class for Fan functionality."""

    def test_get_presence(self, fan, mock_pddf_fan):
        """Test get_presence."""
        mock_pddf_fan.get_presence.return_value = True
        assert fan.get_presence() is True

        mock_pddf_fan.get_presence.return_value = False
        assert fan.get_presence() is False

    def test_get_model_for_present_non_psu_fan(self, fan, mock_pddf_fan):
        """Test get_model for present non-PSU fan."""
        mock_pddf_fan.get_presence.return_value = True
        assert fan.get_model() == "FAN-80G1-F"

    def test_get_model_for_non_present_fan(self, fan, mock_pddf_fan):
        """Test get_model for non-present non-PSU fan."""
        mock_pddf_fan.get_presence.return_value = False
        assert fan.get_model() == "N/A"

    def test_fan_init_ok_when_db_conn_fails(self, mock_pddf_fan):
        """Test Fan initialization is ok when DB connection fails."""
        # Given
        with patch.object(
            FakeDBConnector,
            "__init__",
            Mock(side_effect=RuntimeError)
        ):
            # When
            test_fan = fan.__wrapped__(mock_pddf_fan)
            # Then
            assert test_fan._state_fan_tbl is None

    def test_get_max_speed_default(self, fan):
        """Test get_max_speed when it hasn't been set."""
        assert fan.get_max_speed() == fan._DEFAULT_MAX_SPEED

    def test_set_max_speed(self, fan):
        """Test set_max_speed writes data to STATE_DB."""
        fan.set_max_speed(60.99)
        assert (
            FakeTable._global_db["STATE_DB"]["FAN_INFO"][fan.get_name()]["max_speed"]
            == "60.99"
        )

    def test_set_and_get_max_speed(self, fan):
        """Test setting and getting max speed."""
        fan.set_max_speed(60.99)
        assert fan.get_max_speed() == 60.99

    def test_set_speed_is_clamped_by_max_speed(self, fan, mock_pddf_fan):
        """Test set_speed is clamped by the previously set max_speed."""
        # Given
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

    def test_set_and_get_max_speed_when_db_conn_fails(self, mock_pddf_fan):
        """Test set_max_speed and get_max_speed change nothing when DB connection fails."""
        # Given
        with patch.object(
            FakeDBConnector,
            "__init__",
            Mock(side_effect=RuntimeError)
        ):
            test_fan = fan.__wrapped__(mock_pddf_fan)
            # When/Then
            assert test_fan.set_max_speed(60.99) == False
            assert test_fan.get_max_speed() == test_fan._DEFAULT_MAX_SPEED

    def test_set_and_get_max_speed_when_db_conn_resumes(self, mock_pddf_fan):
        """Test set_max_speed and get_max_speed working when DB connection resumes."""
        # Given - Inject a failed DB connection
        old_init = FakeDBConnector.__init__
        FakeDBConnector.__init__ = Mock(side_effect=RuntimeError)
        test_fan = fan.__wrapped__(mock_pddf_fan)
        assert test_fan._state_fan_tbl is None

        # When - Revive the DB connector
        FakeDBConnector.__init__ = old_init

        # Then - Perform set/get max speed should work
        test_fan.set_max_speed(60.99)
        assert test_fan._state_fan_tbl is not None
        assert (
            FakeTable._global_db["STATE_DB"]["FAN_INFO"][test_fan.get_name()]["max_speed"]
            == "60.99"
        )
        assert test_fan.get_max_speed() == 60.99
