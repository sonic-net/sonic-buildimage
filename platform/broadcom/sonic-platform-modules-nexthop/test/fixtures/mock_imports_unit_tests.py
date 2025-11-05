#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Dependency mocking utilities for platform unit tests.

This module provides functions to set up mocks for dependencies
that are not available in test environments.
"""

import types

from unittest.mock import Mock
from fixtures.fake_swsscommon import fake_swsscommon_modules


MOCK_MODULES = [
    # PDDF
    "sonic_platform_pddf_base",
    "sonic_platform_pddf_base.pddf_chassis",
    "sonic_platform_pddf_base.pddf_platform",
    "sonic_platform_pddf_base.pddf_component",
    "sonic_platform_pddf_base.pddf_sfp",
    "sonic_platform_pddf_base.pddf_psu",
    "sonic_platform_pddf_base.pddf_fan",
    "sonic_platform_pddf_base.pddf_fan_drawer",

    # Platform base
    "sonic_platform_base",
    "sonic_platform_base.chassis_base",
    "sonic_platform_base.component_base",
    "sonic_platform_base.sfp_base",
    "sonic_platform_base.psu_base",
    "sonic_platform_base.fan_base",
    "sonic_platform_base.fan_drawer_base",

    # Thermal control
    "sonic_platform_base.sonic_thermal_control",
    "sonic_platform_base.sonic_thermal_control.thermal_condition_base",
    "sonic_platform_base.sonic_thermal_control.thermal_manager_base",

    # SONiC common
    "sonic_py_common",
    "sonic_py_common.logger",

    # EEPROM
    "sonic_eeprom",
    "sonic_eeprom.eeprom_tlvinfo",

    # Other
    "portconfig",
]


def mock_syslog_modules():
    """Set up mocks for syslogger."""

    class MockSysLogger:
        # Methods as mocks so tests can assert calls
        log_info = Mock()
        log_error = Mock()
        log_warning = Mock()
        log_debug = Mock()
        log = Mock()

        def __init__(self, *args, **kwargs):
            pass

    syslogger = Mock()
    syslogger.SysLogger = MockSysLogger

    syslog = Mock()
    syslog.SYSLOG_IDENTIFIER_THERMAL = "nh_thermal"
    syslog.NhLoggerMixin = MockSysLogger

    return {
        "sonic_py_common.syslogger": syslogger,
        "sonic_platform.syslog": syslog,
    }


def fake_some_base_modules():
    """Returns fake bases for some modules that their derived classes are required to be real.

    Otherwise, classes will be `Mock` when they extend from the mocked base class.
    And it may cause "metaclass conflict" error.
    """
    # Mock the thermal_json_object decorator
    def mock_thermal_json_object(name):
        def decorator(cls):
            return cls
        return decorator

    thermal_json_object = Mock()
    thermal_json_object.thermal_json_object = mock_thermal_json_object

    thermal_info_base = Mock()
    thermal_info_base.ThermalPolicyInfoBase = type("ThermalPolicyInfoBase", (object,), {})

    thermal_action_base = Mock()
    thermal_action_base.ThermalPolicyActionBase = type("ThermalPolicyActionBase", (object,), {})

    thermal_base = Mock()
    thermal_base.ThermalBase = type("ThermalBase", (object,), {})

    watchdog_base = Mock()
    watchdog_base.WatchdogBase = type("WatchdogBase", (object,), {})

    pddf_thermal = Mock()
    pddf_thermal.PddfThermal = type("PddfThermal", (object,), {})

    led_control_base = Mock()
    led_control_base.LedControlBase = type("LedControlBase", (object,), {})

    return {
        "sonic_platform_base.sonic_thermal_control.thermal_json_object": thermal_json_object,
        "sonic_platform_base.sonic_thermal_control.thermal_info_base": thermal_info_base,
        "sonic_platform_base.sonic_thermal_control.thermal_action_base": thermal_action_base,
        "sonic_platform_base.thermal_base": thermal_base,
        "sonic_platform_base.watchdog_base": watchdog_base,
        "sonic_platform_pddf_base.pddf_thermal": pddf_thermal,
        "sonic_led.led_control_base": led_control_base,
    }


def dependencies_dict() -> dict[str, types.ModuleType]:
    """Returns a dictionary of mocked/faked dependencies for unit tests.

    When loading any module from sonic_platform, the __init__.py tries to import
    all its submodules. This means that all dependencies must be available when
    running unit tests. This function provides a complete list of all required
    dependencies.
    """
    results = {}
    for module in MOCK_MODULES:
        results[module] = Mock()
    results.update(mock_syslog_modules())
    results.update(fake_some_base_modules())
    results.update(fake_swsscommon_modules())
    return results

