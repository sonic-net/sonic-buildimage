#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform chassis module.
These tests run in isolation from the SONiC environment and can be executed directly using pytest:
python -m pytest test/unit/sonic_platform/test_chassis.py -v
"""

import os
import pytest
import sys
import tempfile

from fixtures.test_helpers_common import mock_pddf_data
from unittest.mock import patch, Mock


class MockPddfChassis:
    """Mock implementation of PddfChassis for testing."""

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        self._thermal_list = []
        self.plugin_data = pddf_plugin_data


@pytest.fixture
def mock_pddf_chassis():
    """Injects and returns a mock PddfChassis for testing."""
    pddf_chassis = Mock()
    pddf_chassis.PddfChassis = MockPddfChassis
    with patch.dict(sys.modules, {"sonic_platform_pddf_base.pddf_chassis": pddf_chassis}):
        yield pddf_chassis.PddfChassis


@pytest.fixture
def chassis_module(mock_pddf_chassis):
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import chassis

    yield chassis


def _create_temp_file(content: str) -> str:
    """
    Creates a temporary file, under a temporary directory.
    Args:
        content: content to write to the temporary file.
    Returns:
        Path to the created file
    """
    root = tempfile.mkdtemp()
    filepath = os.path.join(root, 'reboot-cause.txt')
    with open(filepath, 'w+') as file:
        file.write(content)
    return filepath


class TestChassis:
    """Test class for Chassis functionality."""

    def test_chassis_basic_functionality(self, chassis_module):
        """Test basic chassis functionality."""
        # Test that chassis object was created successfully
        chassis = chassis_module.Chassis()
        assert chassis is not None

        # Test that get_change_event method exists and is callable
        assert hasattr(chassis, "get_change_event")
        assert callable(getattr(chassis, "get_change_event"))

    def test_chassis_get_watchdog(self, chassis_module):
        chassis = chassis_module.Chassis(
            pddf_data=mock_pddf_data({
                "WATCHDOG": {
                    "dev_info": {"device_parent": "FAKE_MULTIFPGAPCIE1"},
                    "dev_attr": {
                        "event_driven_power_cycle_control_reg_offset": "0x28",
                        "watchdog_counter_reg_offset": "0x1E0",
                    },
                },
                "FAKE_MULTIFPGAPCIE1": {
                    "dev_info": {"device_bdf": "FAKE_ADDR"},
                },
            })
        )
        actual_watchdog = chassis.get_watchdog()
        assert actual_watchdog.fpga_pci_addr == "FAKE_ADDR"
        assert actual_watchdog.event_driven_power_cycle_control_reg_offset == 0x28
        assert actual_watchdog.watchdog_counter_reg_offset == 0x1E0

    def test_chassis_get_watchdog_pddf_data_is_empty(self, chassis_module):
        # Initiailize chasis with an empty pddf_data
        chassis = chassis_module.Chassis(pddf_data=mock_pddf_data({}))

        assert chassis.get_watchdog() is None

    def test_chassis_get_watchdog_no_watchdog_presence_in_pddf_data(self, chassis_module):
        # Initiailize chasis with an empty pddf_data
        chassis = chassis_module.Chassis(pddf_data=mock_pddf_data({"device": {}}))

        assert chassis.get_watchdog() is None

    def test_chassis_get_reboot_cause_sw_reboot(self, chassis_module):
        EXPECTED_SW_REBOOT_CAUSE = "reboot"
        EXPECTED_MINOR_CAUSES = "System powered off due to software disabling data plane power, System powered off due to software disabling data plane power, System powered off due to software disabling data plane power"

        # Given
        reboot_cause_filepath = _create_temp_file(
            f"User issued '{EXPECTED_SW_REBOOT_CAUSE}' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        chassis_module.adm1266.get_reboot_cause = Mock(
            return_value=("Power Loss", EXPECTED_MINOR_CAUSES)
        )

        # When
        chassis = chassis_module.Chassis(
            pddf_plugin_data={
                "REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}
            },
        )
        # Then
        assert chassis.get_reboot_cause() == (
            EXPECTED_SW_REBOOT_CAUSE,
            EXPECTED_MINOR_CAUSES,
        )

    def test_chassis_get_reboot_cause_sw_kernel_panic(self, chassis_module):
        # Given
        reboot_cause_filepath = _create_temp_file(
            f"Kernel Panic [Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        chassis_module.adm1266.get_reboot_cause = Mock(return_value=None)

        # When
        chassis = chassis_module.Chassis(
            pddf_plugin_data={
                "REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}
            },
        )

        # Then
        assert chassis.get_reboot_cause() == (
            "Kernel Panic",
            "",
        )

    def test_chassis_get_reboot_cause_hw(self, chassis_module):
        EXPECTED_HW_CAUSE = "Power Loss"
        EXPECTED_HW_MINOR_CAUSE = "System powered off due to loss of input power on both PSUs, System powered off due to software disabling data plane power"

        # Given
        reboot_cause_filepath = _create_temp_file("")
        chassis_module.adm1266.get_reboot_cause = Mock(
            return_value=(
                EXPECTED_HW_CAUSE,
                EXPECTED_HW_MINOR_CAUSE,
            )
        )

        # When
        chassis = chassis_module.Chassis(
            pddf_plugin_data={
                "REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}
            },
        )

        # Then
        assert chassis.get_reboot_cause() == (
            EXPECTED_HW_CAUSE,
            EXPECTED_HW_MINOR_CAUSE,
        )

    def test_chassis_get_reboot_cause_unknown(self, chassis_module):
        # Given
        reboot_cause_filepath = _create_temp_file("unknown")
        chassis_module.adm1266.get_reboot_cause = Mock(return_value=None)

        # When
        chassis = chassis_module.Chassis(
            pddf_plugin_data={
                "REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}
            },
        )

        # Then
        assert chassis.get_reboot_cause() == ("Unknown", "Unknown")
