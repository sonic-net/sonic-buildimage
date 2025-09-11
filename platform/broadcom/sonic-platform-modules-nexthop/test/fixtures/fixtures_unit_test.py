#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test fixtures for sonic_platform tests.
This file provides common fixtures that can be used across all test modules.
"""

import os
import sys
import importlib.util
from unittest.mock import Mock
import pytest


class PddfChassisMock:
    """Mock implementation of PddfChassis for testing."""
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


@pytest.fixture
def mock_pddf_data():
    """Fixture providing mock PDDF data for tests."""
    data_mock = Mock()
    data_mock.data = {'PLATFORM': {'num_nexthop_fpga_asic_temp_sensors': 0}}
    return data_mock


@pytest.fixture
def chassis(mock_pddf_data):
    """
    Fixture providing a Chassis instance for testing.
    This fixture loads the chassis module directly to avoid package import issues.
    """
    # Set up the specific PddfChassis mock with our test implementation
    pddf_chassis_mock = Mock()
    pddf_chassis_mock.PddfChassis = PddfChassisMock
    sys.modules['sonic_platform_pddf_base.pddf_chassis'] = pddf_chassis_mock
    
    # Load the chassis module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    chassis_path = os.path.join(test_dir, "../../common/sonic_platform/chassis.py")
    
    spec = importlib.util.spec_from_file_location("chassis", chassis_path)
    chassis_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(chassis_module)
    
    return chassis_module.Chassis(pddf_data=mock_pddf_data)


@pytest.fixture
def mock_sfps(chassis):
    """
    Fixture providing mock SFP objects for testing.
    Creates a list of mock SFPs and attaches them to the chassis.
    """
    NUM_TEST_SFPS = 32
    mock_sfps = []

    for i in range(1, NUM_TEST_SFPS + 1):
        sfp_mock = Mock()
        sfp_mock.get_name = Mock(return_value=f"Ethernet{i}")
        sfp_mock.get_position_in_parent = Mock(return_value=str(i))
        mock_sfps.append(sfp_mock)

    chassis.get_all_sfps = Mock(return_value=mock_sfps)
    return mock_sfps


@pytest.fixture
def pid_params():
    """Fixture providing default PID controller parameters for testing."""
    return {
        "dt": 5,
        "setpoint": 85,
        "Kp": 1.0,
        "Ki": 1.0,
        "Kd": 1.0,
        "min_speed": 30,
        "max_speed": 100
    }


@pytest.fixture
def pid_controller(pid_params):
    """
    Fixture providing a FanPIDController instance for testing.
    This fixture loads the thermal_actions module directly to avoid package import issues.
    """
    # Load the thermal_actions module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    thermal_actions_path = os.path.join(test_dir, "../../common/sonic_platform/thermal_actions.py")

    spec = importlib.util.spec_from_file_location("thermal_actions", thermal_actions_path)
    thermal_actions_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(thermal_actions_module)

    return thermal_actions_module.FanPIDController(**pid_params)


@pytest.fixture
def nexthop_eeprom_utils():
    """
    Fixture providing nexthop.eeprom_utils module for testing.
    This fixture loads the module directly to avoid package import issues.
    """
    # Load the eeprom_utils module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    eeprom_utils_path = os.path.join(test_dir, "../../common/nexthop/eeprom_utils.py")

    spec = importlib.util.spec_from_file_location("eeprom_utils", eeprom_utils_path)
    eeprom_utils_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(eeprom_utils_module)

    return eeprom_utils_module


@pytest.fixture
def nexthop_fpga_lib():
    """
    Fixture providing nexthop.fpga_lib module for testing.
    This fixture loads the module directly to avoid package import issues.
    """
    # Load the fpga_lib module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    fpga_lib_path = os.path.join(test_dir, "../../common/nexthop/fpga_lib.py")

    spec = importlib.util.spec_from_file_location("fpga_lib", fpga_lib_path)
    fpga_lib_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fpga_lib_module)

    return fpga_lib_module


@pytest.fixture
def nexthop_led_control():
    """
    Fixture providing nexthop.led_control module for testing.
    This fixture loads the module directly to avoid package import issues.
    """
    # Import the mock setup function
    from .mock_imports_unit_tests import setup_sonic_platform_mocks

    # Set up mocks before loading the module
    setup_sonic_platform_mocks()

    # Load the led_control module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    led_control_path = os.path.join(test_dir, "../../common/nexthop/led_control.py")

    spec = importlib.util.spec_from_file_location("led_control", led_control_path)
    led_control_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(led_control_module)

    return led_control_module
