#!/usr/bin/env python

import pytest
import sys
import copy
import time

from fixtures.fake_swsscommon import fake_swsscommon_modules
from fixtures.mock_imports_unit_tests import mock_syslog_modules
from fixtures.test_helpers_chassis import setup_patch_for_chassis_init
from unittest.mock import Mock, patch, create_autospec


# Test constants
NUM_TEST_SFPS = 32

# Xcvr presence states used by xcvrd
XCVR_INSERTED = "1"
XCVR_REMOVED = "0"


@pytest.fixture(scope="module")
def mock_unimportant_modules():
    """Mock modules that aren't important for integration testing."""
    modules = {}
    modules.update(mock_syslog_modules())
    modules.update(fake_swsscommon_modules())

    with patch.dict(sys.modules, modules):
        yield


@pytest.fixture(scope="module")
def chassis_module(mock_unimportant_modules):
    """Loads the module before all tests. This is to let conftest.py inject deps first."""
    from sonic_platform import chassis

    yield chassis


@pytest.fixture(scope="module")
def mock_sfps(mock_unimportant_modules):
    """
    Fixture providing autospec'ed mock SFP objects for testing.
    Creates a list of mock SFPs and attaches them to the chassis.
    """
    mock_sfps = []
    from sonic_platform_base.sfp_base import SfpBase

    for i in range(1, NUM_TEST_SFPS + 1):
        # autospec helps catch typos or changes in the real interface early.
        sfp_mock = create_autospec(SfpBase)
        sfp_mock.get_name.return_value = f"Ethernet{i}"
        sfp_mock.get_position_in_parent.return_value = str(i)
        mock_sfps.append(sfp_mock)

    return mock_sfps


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
        return (True, {"sfp": change_events})

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


class TestChassisSfpIntegration:
    """Test class for Chassis and SFP events functionality."""

    def test_get_change_event_initial_state(self, chassis_module, mock_sfps):
        """Test initial SFP change event detection."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            # Initial call test - expect change in all SFPs
            inserted_sfps = mock_sfps[0 : NUM_TEST_SFPS // 2]
            removed_sfps = mock_sfps[NUM_TEST_SFPS // 2 :]

            sfp_test_helper.update_state(inserted_sfps, XCVR_INSERTED)
            sfp_test_helper.update_state(removed_sfps, XCVR_REMOVED)

            result = chassis.get_change_event()
            expected = sfp_test_helper.get_change_events()

            assert result == expected

    def test_get_change_event_no_change_short_timeout(self, chassis_module, mock_sfps):
        """Test change event detection with short timeout and no changes."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            # Set initial state
            sfp_test_helper.update_state(mock_sfps, XCVR_INSERTED)
            chassis.get_change_event()  # Clear initial events
            sfp_test_helper.get_change_events()  # Clear helper events

            # Small timeout test - no change
            result = chassis.get_change_event(timeout=1)
            expected = sfp_test_helper.get_change_events()

            assert result == expected

    def test_get_change_event_no_change_long_timeout(self, chassis_module, mock_sfps):
        """Test change event detection with long timeout and no changes."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            # Set initial state
            sfp_test_helper.update_state(mock_sfps, XCVR_INSERTED)
            chassis.get_change_event()  # Clear initial events
            sfp_test_helper.get_change_events()  # Clear helper events

            # Large timeout test - no change (mock time to avoid actual waiting)
            start_time = time.monotonic()
            with patch("time.sleep") as mock_sleep, patch("time.monotonic") as mock_monotonic:
                mock_monotonic.side_effect = [start_time + i for i in range(20)]
                result = chassis.get_change_event(timeout=10 * 1000)
                expected = sfp_test_helper.get_change_events()

                assert result == expected

    def test_get_change_event_partial_change(self, chassis_module, mock_sfps):
        """Test change event detection with partial SFP state changes."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            # Set initial state
            inserted_sfps = mock_sfps[0 : NUM_TEST_SFPS // 2]
            removed_sfps = mock_sfps[NUM_TEST_SFPS // 2 :]

            sfp_test_helper.update_state(inserted_sfps, XCVR_INSERTED)
            sfp_test_helper.update_state(removed_sfps, XCVR_REMOVED)
            chassis.get_change_event()  # Clear initial events
            sfp_test_helper.get_change_events()  # Clear helper events

            # Final change test - a subset of SFPs
            sfp_test_helper.update_state(inserted_sfps[0 : NUM_TEST_SFPS // 4], XCVR_REMOVED)
            sfp_test_helper.update_state(removed_sfps[0 : NUM_TEST_SFPS // 4], XCVR_INSERTED)

            result = chassis.get_change_event()
            expected = sfp_test_helper.get_change_events()

            assert result == expected

    def test_get_change_event_all_inserted(self, chassis_module, mock_sfps):
        """Test change event when all SFPs are inserted."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            sfp_test_helper.set_all_inserted(mock_sfps)
            result = chassis.get_change_event()
            expected = sfp_test_helper.get_change_events()

            assert result == expected

    def test_get_change_event_all_removed(self, chassis_module, mock_sfps):
        """Test change event when all SFPs are removed."""
        sfp_test_helper = SfpTestHelper()
        with setup_patch_for_chassis_init(pddf_plugin_data={}):
            chassis = chassis_module.Chassis()
            chassis._sfp_list = mock_sfps

            sfp_test_helper.set_all_removed(mock_sfps)
            result = chassis.get_change_event()
            expected = sfp_test_helper.get_change_events()

            assert result == expected
