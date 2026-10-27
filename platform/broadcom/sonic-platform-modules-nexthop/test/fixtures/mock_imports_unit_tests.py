#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Dependency mocking utilities for platform unit tests.

This module provides functions to set up mocks for dependencies
that are not available in test environments.
"""

import contextlib
import importlib.util
import os
import sys
import types

from unittest.mock import MagicMock, Mock, patch
from fixtures.fake_swsscommon import fake_swsscommon_modules

# sonic_platform_base and sonic_platform_pddf_base are mocked wholesale below and are not
# pip-installed in the unit test environment, so submodules whose real behaviour we depend
# on are loaded from their source trees instead.
_SONIC_PLATFORM_COMMON = os.path.normpath(os.path.join(
    os.path.dirname(__file__), "..", "..", "..", "..", "..",
    "src", "sonic-platform-common"))
_PDDF_BASE = os.path.normpath(os.path.join(
    os.path.dirname(__file__), "..", "..", "..", "..", "pddf", "platform-api-pddf-base"))


def _load_real_module(module_name, relative_path, root=_SONIC_PLATFORM_COMMON):
    """Load a module from a source tree (sonic-platform-common by default) under the given name."""
    path = os.path.join(root, relative_path)
    if not os.path.exists(path):
        raise RuntimeError(
            f"cannot find {relative_path} at {path}; unit tests need the "
            "sonic-buildimage source tree around this platform package")
    spec = importlib.util.spec_from_file_location(module_name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SHARED_THERMAL_PACKAGE = "sonic_platform_base.sonic_thermal_control"

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
    "sonic_platform_base.sonic_thermal_control.pid_thermal_manager_base",

    # SONiC common
    "sonic_py_common",
    "sonic_py_common.logger",

    # EEPROM
    "sonic_eeprom",
    "sonic_eeprom.eeprom_tlvinfo",

    # Other
    "portconfig",
]


def mock_smbus2():
    """Set up mock for smbus2 module.

    smbus2 is a hardware (I2C) dependency not available in test environments.
    """
    return {
        "smbus2": MagicMock(),
    }


def mock_syslog_modules():
    """Set up mocks for syslogger."""

    class MockSysLogger:
        # Methods as mocks so tests can assert calls
        log_notice = Mock()
        log_info = Mock()
        log_error = Mock()
        log_warning = Mock()
        log_debug = Mock()
        log = Mock()
        logger = Mock()

        def __init__(self, *args, **kwargs):
            pass

    syslogger = MagicMock()
    syslogger.SysLogger = MockSysLogger
    
    logger = MagicMock()
    logger.Logger = MockSysLogger

    syslog = MagicMock()
    syslog.SYSLOG_IDENTIFIER_THERMAL = "nh_thermal"
    syslog.NhLoggerMixin = MockSysLogger

    return {
        "sonic_py_common.syslogger": syslogger,
        "sonic_py_common.logger": logger,
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

    thermal_condition_base = Mock()
    thermal_condition_base.ThermalPolicyConditionBase = type(
        "ThermalPolicyConditionBase", (object,), {})

    watchdog_base = Mock()
    watchdog_base.WatchdogBase = type("WatchdogBase", (object,), {})

    led_control_base = Mock()
    led_control_base.LedControlBase = type("LedControlBase", (object,), {})

    interface_mock = Mock()
    interface_mock.backplane_prefix.return_value = "Ethernet-BP"
    interface_mock.inband_prefix.return_value = "Ethernet-IB"
    interface_mock.recirc_prefix.return_value = "Ethernet-Rec"

    return {
        "sonic_platform_base.sonic_thermal_control.thermal_json_object": thermal_json_object,
        "sonic_platform_base.sonic_thermal_control.thermal_info_base": thermal_info_base,
        "sonic_platform_base.sonic_thermal_control.thermal_action_base": thermal_action_base,
        "sonic_platform_base.sonic_thermal_control.thermal_condition_base": thermal_condition_base,
        "sonic_platform_base.watchdog_base": watchdog_base,
        "sonic_led.led_control_base": led_control_base,
        "sonic_py_common.interface": interface_mock,
    }


def load_real_thermal_base(dependencies):
    """Load the real ThermalBase and PddfAsicThermal, and a fake PddfThermal built on
    ThermalBase as the real one is.

    ThermalBase provides default PID hooks, so the platform's thermals are only right if
    their own hooks win over it in the method resolution order. A fake ThermalBase would
    hide that.
    """
    device_base = _load_real_module(
        "sonic_platform_base.device_base", os.path.join("sonic_platform_base", "device_base.py"))
    # thermal_base does "from . import device_base", which reads it off the mocked package.
    dependencies["sonic_platform_base"].device_base = device_base
    with patch.dict(sys.modules, dependencies):
        thermal_base = _load_real_module(
            "sonic_platform_base.thermal_base", os.path.join("sonic_platform_base", "thermal_base.py"))
        sys.modules["sonic_platform_base.thermal_base"] = thermal_base
        pddf_asic_thermal = _load_real_module(
            "sonic_platform_pddf_base.pddf_asic_thermal",
            os.path.join("sonic_platform_pddf_base", "pddf_asic_thermal.py"), root=_PDDF_BASE)

    pddf_thermal = Mock()
    pddf_thermal.PddfThermal = type("PddfThermal", (thermal_base.ThermalBase,), {})

    return {
        "sonic_platform_base.device_base": device_base,
        "sonic_platform_base.thermal_base": thermal_base,
        "sonic_platform_pddf_base.pddf_thermal": pddf_thermal,
        "sonic_platform_pddf_base.pddf_asic_thermal": pddf_asic_thermal,
    }


@contextlib.contextmanager
def real_thermal_control_package():
    """Make the whole real sonic_thermal_control package importable inside the block.

    The mocks above leave thermal_json_object and thermal_manager_base fake, so no policy
    type is ever registered and no policy file could be loaded. Inside this block the package
    and every submodule are the real ones, freshly loaded, so the type registry starts empty
    each time. On exit sys.modules is restored, mocks included.
    """
    package_dir = os.path.join(
        _SONIC_PLATFORM_COMMON, "sonic_platform_base", "sonic_thermal_control")
    init_path = os.path.join(package_dir, "__init__.py")
    if not os.path.exists(init_path):
        raise RuntimeError(
            f"cannot find {init_path}; unit tests need the "
            "sonic-platform-common source tree alongside this platform package")
    with patch.dict(sys.modules):
        for name in [n for n in sys.modules
                     if n == SHARED_THERMAL_PACKAGE or n.startswith(SHARED_THERMAL_PACKAGE + ".")]:
            del sys.modules[name]
        # A real package with a real __path__, so its submodules and their relative imports
        # resolve to the source tree rather than to attributes of the mocked parent.
        spec = importlib.util.spec_from_file_location(
            SHARED_THERMAL_PACKAGE, init_path, submodule_search_locations=[package_dir])
        package = importlib.util.module_from_spec(spec)
        sys.modules[SHARED_THERMAL_PACKAGE] = package
        spec.loader.exec_module(package)
        yield package


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
    results.update(mock_smbus2())
    results.update(mock_syslog_modules())
    results.update(fake_some_base_modules())
    results.update(fake_swsscommon_modules())
    results.update(load_real_thermal_base(results))
    return results

