#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the sonic_platform chassis module.
These tests run in isolation from the SONiC environment and can be executed directly using pytest:
python -m pytest test/unit/sonic_platform/test_chassis.py -v
"""

import copy
import time

from unittest.mock import patch, Mock

# Test constants
NUM_TEST_SFPS = 32

# Xcvr presence states used by xcvrd
XCVR_INSERTED = "1"
XCVR_REMOVED = "0"


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
        assert hasattr(chassis, 'get_change_event')
        assert callable(getattr(chassis, 'get_change_event'))
