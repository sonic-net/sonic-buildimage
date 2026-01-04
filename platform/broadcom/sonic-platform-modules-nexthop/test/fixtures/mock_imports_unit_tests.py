#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Dependency mocking utilities for platform unit tests.

This module provides functions to set up mocks for dependencies
that are not available in test environments.
"""

import sys
from unittest.mock import Mock
import functools


MOCK_MODULES = [
    # PDDF
    'sonic_platform_pddf_base',
    'sonic_platform_pddf_base.pddf_chassis',
    'sonic_platform_pddf_base.pddf_platform', 
    'sonic_platform_pddf_base.pddf_component',
    'sonic_platform_pddf_base.pddf_sfp',
    'sonic_platform_pddf_base.pddf_psu',
    'sonic_platform_pddf_base.pddf_thermal',
    'sonic_platform_pddf_base.pddf_fan',
    'sonic_platform_pddf_base.pddf_fan_drawer',

    # Sonic platform
    'sonic_platform',
    'sonic_platform.platform',
    
    # Platform base
    'sonic_platform_base',
    'sonic_platform_base.thermal_base',
    'sonic_platform_base.chassis_base', 
    'sonic_platform_base.component_base',
    'sonic_platform_base.sfp_base',
    'sonic_platform_base.psu_base',
    'sonic_platform_base.fan_base',
    'sonic_platform_base.fan_drawer_base',
    'sonic_platform_base.watchdog_base',
    
    # Thermal control
    'sonic_platform_base.sonic_thermal_control',
    'sonic_platform_base.sonic_thermal_control.thermal_manager_base',
    'sonic_platform_base.sonic_thermal_control.thermal_action_base',
    'sonic_platform_base.sonic_thermal_control.thermal_json_object',
    
    # SONiC common
    'sonic_py_common',
    'sonic_py_common.logger',
    
    # EEPROM
    'sonic_eeprom',
    'sonic_eeprom.eeprom_tlvinfo',
    
    # swsscommon
    'swsscommon',
    'swsscommon.swsscommon',

    # LED control
    'sonic_led',
    'sonic_led.led_control_base',

    # Other
    'portconfig',
]

def setup_sonic_platform_mocks():
    """
    Set up mock modules for all SONiC platform dependencies.
    This should be called before importing any sonic_platform modules in tests.
    """
    for module in MOCK_MODULES:
        sys.modules[module] = Mock()

    # Mock the thermal module specifically to avoid import chains
    thermal_mock = Mock()
    thermal_mock.NexthopFpgaAsicThermal = Mock()
    sys.modules['sonic_platform.thermal'] = thermal_mock

    # Mock LedControlBase as a proper base class that can be inherited from
    led_control_base_mock = Mock()
    led_control_base_mock.LedControlBase = type('LedControlBase', (object,), {})
    sys.modules['sonic_led.led_control_base'] = led_control_base_mock

