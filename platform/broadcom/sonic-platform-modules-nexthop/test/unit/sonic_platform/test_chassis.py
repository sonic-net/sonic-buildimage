#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform chassis module.
These tests run in isolation from the SONiC environment and can be executed directly using pytest:
python -m pytest test/unit/sonic_platform/test_chassis.py -v
"""

import copy
import os
import sys
import tempfile
import time

from unittest.mock import patch, Mock

# Test constants
NUM_TEST_SFPS = 32

# Xcvr presence states used by xcvrd
XCVR_INSERTED = "1"
XCVR_REMOVED = "0"

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


class SfpTestHelper:
    """
    Helper class for managing SFP state changes in tests.
    Provides utilities for simulating SFP insertion/removal events.
    """
    
    def __init__(self):
        self.sfp_state = {}
        self.change_events = {}

    def update_state(self, mock_sfps, state):
        """
        Update the state of mock SFPs and track change events.
        
        Args:
            mock_sfps: List of mock SFP objects
            state: New state (XCVR_INSERTED or XCVR_REMOVED)
        """
        assert state in [XCVR_INSERTED, XCVR_REMOVED]
        
        for mock_sfp in mock_sfps:
            port_num = mock_sfp.get_position_in_parent()
            if self.sfp_state.get(port_num) != state:
                self.change_events[port_num] = state
            self.sfp_state[port_num] = state
            mock_sfp.get_presence = Mock(return_value=(state == XCVR_INSERTED))

    def get_change_events(self):
        """
        Get and clear the current change events.
        
        Returns:
            Tuple of (success, events_dict) matching chassis.get_change_event() format
        """
        change_events = copy.deepcopy(self.change_events)
        self.change_events.clear()
        return (True, {'sfp': change_events})

    def clear_events(self):
        """Clear all tracked change events."""
        self.change_events.clear()

    def get_current_state(self, port_num):
        """Get the current state of a specific port."""
        return self.sfp_state.get(port_num)

    def set_all_inserted(self, mock_sfps):
        """Set all SFPs to inserted state."""
        self.update_state(mock_sfps, XCVR_INSERTED)

    def set_all_removed(self, mock_sfps):
        """Set all SFPs to removed state."""
        self.update_state(mock_sfps, XCVR_REMOVED)


class TestChassis:
    """Test class for Chassis functionality."""

    def test_get_change_event_initial_state(self, chassis, mock_sfps):
        """Test initial SFP change event detection."""
        sfp_test_helper = SfpTestHelper()

        # Initial call test - expect change in all SFPs
        inserted_sfps = mock_sfps[0:NUM_TEST_SFPS//2]
        removed_sfps = mock_sfps[NUM_TEST_SFPS//2:]

        sfp_test_helper.update_state(inserted_sfps, XCVR_INSERTED)
        sfp_test_helper.update_state(removed_sfps, XCVR_REMOVED)

        result = chassis.get_change_event()
        expected = sfp_test_helper.get_change_events()

        assert result == expected

    def test_get_change_event_no_change_short_timeout(self, chassis, mock_sfps):
        """Test change event detection with short timeout and no changes."""
        sfp_test_helper = SfpTestHelper()

        # Set initial state
        sfp_test_helper.update_state(mock_sfps, XCVR_INSERTED)
        chassis.get_change_event()  # Clear initial events
        sfp_test_helper.get_change_events()  # Clear helper events

        # Small timeout test - no change
        result = chassis.get_change_event(timeout=1)
        expected = sfp_test_helper.get_change_events()

        assert result == expected

    def test_get_change_event_no_change_long_timeout(self, chassis, mock_sfps):
        """Test change event detection with long timeout and no changes."""
        sfp_test_helper = SfpTestHelper()

        # Set initial state
        sfp_test_helper.update_state(mock_sfps, XCVR_INSERTED)
        chassis.get_change_event()  # Clear initial events
        sfp_test_helper.get_change_events()  # Clear helper events

        # Large timeout test - no change (mock time to avoid actual waiting)
        start_time = time.monotonic()
        with patch('time.sleep') as mock_sleep, patch('time.monotonic') as mock_monotonic:
            mock_monotonic.side_effect = [start_time + i for i in range(20)]
            result = chassis.get_change_event(timeout=10 * 1000)
            expected = sfp_test_helper.get_change_events()

            assert result == expected

    def test_get_change_event_partial_change(self, chassis, mock_sfps):
        """Test change event detection with partial SFP state changes."""
        sfp_test_helper = SfpTestHelper()

        # Set initial state
        inserted_sfps = mock_sfps[0:NUM_TEST_SFPS//2]
        removed_sfps = mock_sfps[NUM_TEST_SFPS//2:]

        sfp_test_helper.update_state(inserted_sfps, XCVR_INSERTED)
        sfp_test_helper.update_state(removed_sfps, XCVR_REMOVED)
        chassis.get_change_event()  # Clear initial events
        sfp_test_helper.get_change_events()  # Clear helper events

        # Final change test - a subset of SFPs
        sfp_test_helper.update_state(inserted_sfps[0:NUM_TEST_SFPS//4], XCVR_REMOVED)
        sfp_test_helper.update_state(removed_sfps[0:NUM_TEST_SFPS//4], XCVR_INSERTED)

        result = chassis.get_change_event()
        expected = sfp_test_helper.get_change_events()

        assert result == expected

    def test_get_change_event_all_inserted(self, chassis, mock_sfps):
        """Test change event when all SFPs are inserted."""
        sfp_test_helper = SfpTestHelper()

        sfp_test_helper.set_all_inserted(mock_sfps)
        result = chassis.get_change_event()
        expected = sfp_test_helper.get_change_events()

        assert result == expected

    def test_get_change_event_all_removed(self, chassis, mock_sfps):
        """Test change event when all SFPs are removed."""
        sfp_test_helper = SfpTestHelper()

        sfp_test_helper.set_all_removed(mock_sfps)
        result = chassis.get_change_event()
        expected = sfp_test_helper.get_change_events()

        assert result == expected

    def test_chassis_basic_functionality(self, chassis):
        """Test basic chassis functionality."""
        # Test that chassis object was created successfully
        assert chassis is not None

        # Test that get_change_event method exists and is callable
        assert hasattr(chassis, "get_change_event")
        assert callable(getattr(chassis, "get_change_event"))

    def test_chassis_get_watchdog(self, chassis):
        actual_watchdog = chassis.get_watchdog()
        assert actual_watchdog.fpga_pci_addr == "FAKE_ADDR"
        assert actual_watchdog.event_driven_power_cycle_control_reg_offset == 0x28
        assert actual_watchdog.watchdog_counter_reg_offset == 0x1E0

    def test_chassis_get_watchdog_pddf_data_is_empty(self, chassis):
        # Re-initiailize chasis with an empty pddf_data
        chassis.__init__(pddf_data={})

        assert chassis.get_watchdog() is None

    def test_chassis_get_watchdog_no_watchdog_presence_in_pddf_data(self, chassis):
        # Re-initiailize chasis with an empty pddf_data
        mock_pddf_data = Mock()
        mock_pddf_data.data = {"device": {}}
        chassis.__init__(pddf_data=mock_pddf_data)

        assert chassis.get_watchdog() is None

    def test_chassis_get_reboot_cause_sw_reboot(self, chassis):
        EXPECTED_SW_REBOOT_CAUSE = "reboot"
        EXPECTED_MINOR_CAUSES = "System powered off due to software disabling data plane power, System powered off due to software disabling data plane power, System powered off due to software disabling data plane power"

        # Given
        reboot_cause_filepath = _create_temp_file(
            f"User issued '{EXPECTED_SW_REBOOT_CAUSE}' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        chassis.__init__(
            pddf_data={},
            pddf_plugin_data={
                "REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}
            },
        )

        # When
        mock_adm1266 = sys.modules["sonic_platform"].adm1266
        mock_adm1266.get_reboot_cause.return_value = (
            "Power Loss",
            EXPECTED_MINOR_CAUSES,
        )

        # Then
        assert chassis.get_reboot_cause() == (
            EXPECTED_SW_REBOOT_CAUSE,
            EXPECTED_MINOR_CAUSES,
        )

    def test_chassis_get_reboot_cause_sw_kernel_panic(self, chassis):
        # Given
        reboot_cause_filepath = _create_temp_file(
            f"Kernel Panic [Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        chassis.__init__(
            pddf_data={},
            pddf_plugin_data={"REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}},
        )

        # When
        mock_adm1266 = sys.modules["sonic_platform"].adm1266
        mock_adm1266.get_reboot_cause.return_value = None

        # Then
        assert chassis.get_reboot_cause() == (
            "Kernel Panic",
            "",
        )

    def test_chassis_get_reboot_cause_hw(self, chassis):
        EXPECTED_HW_CAUSE = "Power Loss"
        EXPECTED_HW_MINOR_CAUSE = "System powered off due to loss of input power on both PSUs, System powered off due to software disabling data plane power"

        # Given
        reboot_cause_filepath = _create_temp_file("")
        chassis.__init__(
            pddf_data={},
            pddf_plugin_data={"REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}},
        )

        # When
        mock_adm1266 = sys.modules["sonic_platform"].adm1266
        mock_adm1266.get_reboot_cause.return_value = (
            EXPECTED_HW_CAUSE,
            EXPECTED_HW_MINOR_CAUSE,
        )

        # Then
        assert chassis.get_reboot_cause() == (
            EXPECTED_HW_CAUSE,
            EXPECTED_HW_MINOR_CAUSE,
        )

    def test_chassis_get_reboot_cause_unknown(self, chassis):
        # Given
        reboot_cause_filepath = _create_temp_file("unknown")
        chassis.__init__(
            pddf_data={},
            pddf_plugin_data={"REBOOT_CAUSE": {"reboot_cause_file": reboot_cause_filepath}},
        )

        # When
        mock_adm1266 = sys.modules["sonic_platform"].adm1266
        mock_adm1266.get_reboot_cause.return_value = None

        # Then
        assert chassis.get_reboot_cause() == ("Unknown", "Unknown")
