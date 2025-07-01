#!/usr/bin/env python

import copy
import os
import pytest
import sys
import time

from unittest.mock import Mock, patch

NUM_TEST_SFPS = 32
# Xcvr presence states used by xcvrd
XCVR_INSERTED = "1"
XCVR_REMOVED = "0"


class PddfChassisMock:
    platform_inventory = {}
    platform_inventory['num_components'] = 0

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        # Initialize required attributes that the Chassis class expects
        self._thermal_list = []
        self._sfp_list = []
        self._watchdog = None
        self._eeprom = Mock()
        self._eeprom.modelstr = Mock(return_value="Test Model")
        self.plugin_data = {'REBOOT_CAUSE': {'reboot_cause_file': '/tmp/test_reboot_cause'}}

    def get_all_sfps(self):
        return self._sfp_list

    def set_system_led(self, led_name, color):
        return True

    def get_system_led(self, led_name):
        return "green"

class SfpTestHelper(object):
    def __init__(self):
        self.sfp_state = {}
        self.change_events = {}

    def update_state(self, mock_sfps, state):
        assert state in [XCVR_INSERTED, XCVR_REMOVED]
        for mock_sfp in mock_sfps:
            port_num = mock_sfp.get_position_in_parent()
            if self.sfp_state.get(port_num) != state:
                self.change_events[port_num] = state
            self.sfp_state[port_num] = state
            mock_sfp.get_presence = Mock(return_value=(state == XCVR_INSERTED))

    def get_change_events(self):
        change_events = copy.deepcopy(self.change_events)
        self.change_events.clear()
        return (True, {'sfp': change_events})

@pytest.fixture
def chassis():
    # Mock out dependency modules
    pddf_chassis_mock = Mock()
    pddf_chassis_mock.PddfChassis = PddfChassisMock
    sys.modules['sonic_platform_pddf_base.pddf_chassis'] = pddf_chassis_mock
    sys.modules['sonic_platform.component'] = Mock()
    sys.modules['sonic_platform.thermal'] = Mock()
    # Import the module under test
    cwd = os.path.dirname(os.path.realpath(__file__))
    sys.path.append(os.path.join(cwd, "../"))
    from chassis import Chassis
    data_mock = Mock()
    data_mock.data = {'PLATFORM': {'num_nexthop_fpga_asic_temp_sensors': 0}}
    return Chassis(pddf_data=data_mock)

@pytest.fixture
def mock_sfps(chassis):
    mock_sfps = []
    for i in range(1, NUM_TEST_SFPS + 1):
        sfp_mock = Mock()
        sfp_mock.get_name = Mock(return_value=f"Ethernet{i}")
        sfp_mock.get_position_in_parent = Mock(return_value=str(i))
        mock_sfps.append(sfp_mock)
    chassis.get_all_sfps = Mock(return_value=mock_sfps)
    return mock_sfps

def test_get_change_event(chassis, mock_sfps):
    sfp_test_helper = SfpTestHelper()

    # Initial call test - expect change in all SFPs
    inserted_sfps = mock_sfps[0:NUM_TEST_SFPS//2]
    removed_sfps = mock_sfps[NUM_TEST_SFPS//2:]
    sfp_test_helper.update_state(inserted_sfps, XCVR_INSERTED)
    sfp_test_helper.update_state(removed_sfps, XCVR_REMOVED)
    assert chassis.get_change_event() == sfp_test_helper.get_change_events()

    # Small timeout test - no change
    assert chassis.get_change_event(timeout=1) == sfp_test_helper.get_change_events()

    # Large timeout test - no change
    start_time = time.monotonic()
    with patch('time.sleep') as mock_sleep, patch('time.monotonic') as mock_monotonic:
        mock_monotonic.side_effect = [start_time + i for i in range(20)]
        assert chassis.get_change_event(timeout=10 * 1000) == sfp_test_helper.get_change_events()

    # Final change test - a subset of SFPs
    sfp_test_helper.update_state(inserted_sfps[0:NUM_TEST_SFPS//4], XCVR_REMOVED)
    sfp_test_helper.update_state(removed_sfps[0:NUM_TEST_SFPS//4], XCVR_INSERTED)
    assert chassis.get_change_event() == sfp_test_helper.get_change_events()
