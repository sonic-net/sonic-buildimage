#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform chassis module.
These tests run in isolation from the SONiC environment and can be executed directly using pytest:
python -m pytest test/unit/sonic_platform/test_chassis.py -v
"""

import datetime
import pytest
import sys

from fixtures.test_helpers_common import mock_pddf_data, temp_file
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


@pytest.fixture
def reboot_cause_manager_module():
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import reboot_cause_manager

    yield reboot_cause_manager


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
            pddf_data=mock_pddf_data(
                {
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
                }
            )
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

    def test_chassis_get_reboot_cause_sw(self, chassis_module):
        EXPECTED_SW_REBOOT_CAUSE = "reboot"

        from sonic_platform import reboot_cause_manager

        # Given
        with (
            patch.object(
                chassis_module.Chassis, "REBOOT_CAUSE_NON_HARDWARE", "Non-Hardware", create=True
            ),
            patch.object(
                reboot_cause_manager.RebootCauseManager,
                "summarize_reboot_causes",
                Mock(
                    return_value=[
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.SOFTWARE,
                            source="SW",
                            timestamp=datetime.datetime(
                                2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                            ),
                            cause=EXPECTED_SW_REBOOT_CAUSE,
                            description="User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]",
                            chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                        )
                    ]
                ),
            ),
        ):
            # When
            chassis = chassis_module.Chassis(pddf_plugin_data={"DUMMY": "dummy"})
            # Then
            assert chassis.get_reboot_cause() == ("Non-Hardware", "")

    def test_chassis_get_reboot_cause_hw(self, chassis_module):
        from sonic_platform import reboot_cause_manager

        # Given
        with (
            patch.object(
                chassis_module.Chassis, "REBOOT_CAUSE_POWER_LOSS", "Power Loss", create=True
            ),
            patch.object(
                reboot_cause_manager.RebootCauseManager,
                "summarize_reboot_causes",
                Mock(
                    return_value=[
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.HARDWARE,
                            source="test-dpm",
                            timestamp=datetime.timedelta(seconds=70),
                            cause="PSU_VIN_LOSS",
                            description="Both PSUs lost input power",
                            chassis_reboot_cause_category="REBOOT_CAUSE_POWER_LOSS",
                        )
                    ]
                ),
            ),
            temp_file(content="") as sw_reboot_cause_filepath,
        ):
            # When
            chassis = chassis_module.Chassis(
                pddf_plugin_data={
                    "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                }
            )
            # Then
            assert chassis.get_reboot_cause() == (
                "Power Loss",
                "Both PSUs lost input power, time: 70.000000s after power-on, src: test-dpm",
            )
            with open(sw_reboot_cause_filepath, "r") as file:
                assert file.read() == ""

    def test_chassis_get_reboot_cause_hw_reboot_one_more_time(self, chassis_module):
        from sonic_platform import reboot_cause_manager

        # Given
        with (
            patch.object(
                chassis_module.Chassis, "REBOOT_CAUSE_POWER_LOSS", "Power Loss", create=True
            ),
            patch.object(chassis_module.Chassis, "REBOOT_CAUSE_WATCHDOG", "Watchdog", create=True),
            patch.object(
                reboot_cause_manager.RebootCauseManager,
                "summarize_reboot_causes",
                Mock(
                    return_value=[
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.HARDWARE,
                            source="test-dpm",
                            timestamp=datetime.timedelta(seconds=70),
                            cause="PSU_VIN_LOSS",
                            description="Both PSUs lost input power",
                            chassis_reboot_cause_category="REBOOT_CAUSE_POWER_LOSS",
                        ),
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.HARDWARE,
                            source="test-dpm",
                            timestamp=datetime.datetime(
                                2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                            ),
                            cause="WATCHDOG",
                            description="FPGA watchdog expired",
                            chassis_reboot_cause_category="REBOOT_CAUSE_WATCHDOG",
                        ),
                    ]
                ),
            ),
            temp_file(content="") as sw_reboot_cause_filepath,
        ):
            # When
            chassis = chassis_module.Chassis(
                pddf_plugin_data={
                    "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                }
            )
            # Then
            assert chassis.get_reboot_cause() == (
                "Power Loss",
                "Both PSUs lost input power, time: 70.000000s after power-on, src: test-dpm",
            )
            with open(sw_reboot_cause_filepath, "r") as file:
                assert (
                    file.read()
                    == "System rebooted 1 more time: Watchdog (FPGA watchdog expired, time: 2025-10-02 23:22:56 UTC, src: test-dpm)"
                )

    def test_chassis_get_reboot_cause_sw_and_hw(self, chassis_module):
        from sonic_platform import reboot_cause_manager

        # Given
        with (
            patch.object(chassis_module.Chassis, "REBOOT_CAUSE_WATCHDOG", "Watchdog", create=True),
            patch.object(
                chassis_module.Chassis,
                "REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER",
                "Thermal Overload: Other",
                create=True,
            ),
            patch.object(
                reboot_cause_manager.RebootCauseManager,
                "summarize_reboot_causes",
                Mock(
                    return_value=[
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.SOFTWARE,
                            source="SW",
                            timestamp=datetime.datetime(
                                2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                            ),
                            cause="Kernel Panic",
                            description="Kernel Panic [Time: Thu Oct  2 11:22:56 PM UTC 2025]",
                            chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                        ),
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.HARDWARE,
                            source="test-dpm",
                            timestamp=datetime.datetime(
                                2025, 10, 3, 23, 22, 56, tzinfo=datetime.timezone.utc
                            ),
                            cause="WATCHDOG",
                            description="FPGA watchdog expired",
                            chassis_reboot_cause_category="REBOOT_CAUSE_WATCHDOG",
                        ),
                        reboot_cause_manager.RebootCause(
                            type=reboot_cause_manager.RebootCause.Type.HARDWARE,
                            source="test-dpm",
                            timestamp=datetime.datetime(
                                2025, 10, 4, 23, 22, 56, tzinfo=datetime.timezone.utc
                            ),
                            cause="OVER_TEMP",
                            description="Switch card exceeded overtemperature",
                            chassis_reboot_cause_category="REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER",
                        ),
                    ]
                ),
            ),
            temp_file(content="") as sw_reboot_cause_filepath,
        ):
            # When
            chassis = chassis_module.Chassis(
                pddf_plugin_data={
                    "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                }
            )
            # Then
            assert chassis.get_reboot_cause() == (
                "Kernel Panic",
                "time: 2025-10-02 23:22:56 UTC, src: SW",
            )
            with open(sw_reboot_cause_filepath, "r") as file:
                assert (
                    file.read()
                    == "System rebooted 2 more times: Watchdog (FPGA watchdog expired, time: 2025-10-03 23:22:56 UTC, src: test-dpm); Thermal Overload: Other (Switch card exceeded overtemperature, time: 2025-10-04 23:22:56 UTC, src: test-dpm)"
                )

    def test_chassis_get_reboot_cause_unknown(self, chassis_module):
        # Given
        with patch.object(
            chassis_module.RebootCauseManager,
            "summarize_reboot_causes",
            Mock(return_value=[]),
        ):
            # When
            chassis = chassis_module.Chassis(pddf_plugin_data={"DUMMY": "dummy"})
            # Then
            assert chassis.get_reboot_cause() == ("Unknown", "")

    def test_chassis_get_reboot_cause_unknown_hw(self, chassis_module, reboot_cause_manager_module):
        # Given
        with patch.object(
            chassis_module.RebootCauseManager,
            "summarize_reboot_causes",
            Mock(return_value=[reboot_cause_manager_module.UNKNOWN_HW_REBOOT_CAUSE]),
        ):
            # When
            chassis = chassis_module.Chassis(pddf_plugin_data={"DUMMY": "dummy"})
            # Then
            assert chassis.get_reboot_cause() == ("REBOOT_CAUSE_HARDWARE_OTHER", "unknown, time: unknown, src: unknown")
