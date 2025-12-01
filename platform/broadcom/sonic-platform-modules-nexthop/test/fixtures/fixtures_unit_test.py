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
import tempfile

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
        self.plugin_data = pddf_plugin_data or {'REBOOT_CAUSE': {'reboot_cause_file': '/tmp/test_reboot_cause'}}

    def get_all_sfps(self):
        return self._sfp_list

    def set_system_led(self, led_name, color):
        return True

    def get_system_led(self, led_name):
        return "green"

def process_input(json_file):
    """Load a JSON test spec and return (blackbox_data, expected_records, expected_causes).

    The JSON must contain:
      - hexdump_lines: array of hexdump lines (strings)
    Optionally:
      - expected_records: list[dict] of raw numeric expectations
      - expected_causes: list[dict] of rendered string expectations
    """
    import json

    def parse_hexdump_lines(lines):
        bb = bytearray()
        hexchars = set("0123456789abcdefABCDEF")
        for line in lines:
            for tok in line.split():
                if len(tok) == 2 and all(c in hexchars for c in tok):
                    bb.append(int(tok, 16))
        return bytes(bb)

    with open(json_file, 'r') as f:
        spec = json.load(f)

    if 'hexdump_lines' not in spec:
        raise ValueError('JSON must include hexdump_lines')
    blackbox_data = parse_hexdump_lines(spec['hexdump_lines'])
    expected_records = spec.get('expected_blackbox_records')
    expected_causes = spec.get('expected_reboot_causes')

    return blackbox_data, expected_records, expected_causes

class Adm1266PlatformSpecMock:
    pddf_plugin_data = {
        "DPM": {
            "dpm-mock": {
                "dpm_signal_to_fault_cause": [
                    {
                        "pdio_mask": "0x0001",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0001",
                        "gpio_value": "0x0000",
                        "hw_cause": "PSU_VIN_LOSS",
                        "hw_desc": "Both PSUs lost input power",
                        "summary": "PSU input power lost",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
                    },
                    {
                        "pdio_mask": "0x0002",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0002",
                        "gpio_value": "0x0000",
                        "hw_cause": "OVER_TEMP",
                        "hw_desc": "Switch card temp sensor OT",
                        "summary": "Temperature exceeded threshold",
                        "reboot_cause": "REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER"
                    },
                    {
                        "pdio_mask": "0x0004",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0004",
                        "gpio_value": "0x0000",
                        "hw_cause": "CPU_PWR_BAD",
                        "hw_desc": "CPU card power bad",
                        "summary": "CPU power failure",
                        "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER"
                    },
                    {
                        "pdio_mask": "0x0008",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0008",
                        "gpio_value": "0x0000",
                        "hw_cause": "WATCHDOG",
                        "hw_desc": "FPGA watchdog expired",
                        "summary": "Watchdog timeout",
                        "reboot_cause": "REBOOT_CAUSE_WATCHDOG"
                    },
                    {
                        "pdio_mask": "0x0010",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0010",
                        "gpio_value": "0x0000",
                        "hw_cause": "ASIC_OT",
                        "hw_desc": "ASIC MAX_TEMP exceeded OT threshold",
                        "summary": "ASIC overtemperature",
                        "reboot_cause": "REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC"
                    },
                    {
                        "pdio_mask": "0x0020",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0020",
                        "gpio_value": "0x0000",
                        "hw_cause": "NO_FAN_PRSNT",
                        "hw_desc": "All 4 fans have same ID=0xf",
                        "summary": "No fans present",
                        "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER"
                    },
                    {
                        "pdio_mask": "0x0040",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0040",
                        "gpio_value": "0x0000",
                        "hw_cause": "CMD_PWR_CYC",
                        "hw_desc": "Software commanded power cycle",
                        "summary": "Software power cycle",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
                    },
                    {
                        "pdio_mask": "0x0080",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0080",
                        "gpio_value": "0x0000",
                        "hw_cause": "DP_PWR_ON",
                        "hw_desc": "P2 only: from shift chain; not used on P1",
                        "summary": "DP power on",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
                    },
                    {
                        "pdio_mask": "0x0100",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0100",
                        "gpio_value": "0x0000",
                        "hw_cause": "FPGA_CMD_PCYC",
                        "hw_desc": "FPGA commanded power cycle",
                        "summary": "FPGA power cycle",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
                    },
                    {
                        "pdio_mask": "0x0200",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0200",
                        "gpio_value": "0x0000",
                        "hw_cause": "CMD_ASIC_PWR_OFF",
                        "hw_desc": "FPGA command ASIC power off",
                        "summary": "ASIC power off",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS"
                    }
                ],
                "vpx_to_rail_desc": {
                    "6": "POS0V75_S5",
                    "7": "POS1V8_S5",
                    "8": "POS3V3_S5",
                    "9": "POS1V1_S0",
                    "10": "POS0V78_S0",
                    "11": "POS0V75_S0",
                    "12": "POS1V8_S0",
                    "13": "POS3V3_S0",
                },
                "vhx_to_rail_desc": {
                    "5": "POS5V0_S0"
                }
            }
        }
    }

    def __init__(self):
        # Create temporary nvmem file
        nvmem_file = tempfile.NamedTemporaryFile(delete=False)
        nvmem_file.close()
        nvmem_path = nvmem_file.name
        Adm1266PlatformSpecMock.pddf_plugin_data["DPM"]["dpm-mock"]["nvmem_path"] = nvmem_path
        self.nvmem_path = nvmem_path

        """
        Create a Adm1266PlatformSpec instance using mock data for testing.
        """
        test_dir = os.path.dirname(os.path.realpath(__file__))
        adm1266_platform_spec_path = os.path.join(test_dir, "../../common/sonic_platform/adm1266_platform_spec.py")
        spec = importlib.util.spec_from_file_location("adm1266_platform_spec", adm1266_platform_spec_path)
        adm1266_platform_spec_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(adm1266_platform_spec_module)
        adm1266_platform_spec = adm1266_platform_spec_module.Adm1266PlatformSpec("dpm-mock", Adm1266PlatformSpecMock.pddf_plugin_data)

        # Store cleanup info for later
        self.adm1266_platform_spec = adm1266_platform_spec

    def __del__(self):
        """Clean up temporary file"""
        if os.path.exists(self.nvmem_path):
            os.unlink(self.nvmem_path)


class Adm1266Mock:
    """
    Mock implementation of ADM1266 for unit testing.
    Reads test data from provided file paths.
    """
    def __init__(self):
        test_dir = os.path.dirname(__file__)

        json_file = os.path.join(test_dir, "adm1266_test_spec.json")
        data, records, causes = process_input(json_file)
        self.blackbox_input = data
        self.expected_records = records
        self.expected_causes = causes

        self.adm1266_platform_spec_mock = Adm1266PlatformSpecMock()

        # SET UP MOCKS BEFORE LOADING adm1266.py
        # Mock sonic_platform_base.chassis_base that adm1266.py imports
        chassis_base_mock = Mock()
        chassis_base_mock.ChassisBase = Mock()
        sys.modules["sonic_platform_base.chassis_base"] = chassis_base_mock

        # Mock sonic_platform.adm1266_platform_spec so the import works
        adm1266_platform_spec_mock = Mock()
        adm1266_platform_spec_mock.Adm1266PlatformSpec = lambda name, pddf_data: self.adm1266_platform_spec_mock.adm1266_platform_spec
        sys.modules["sonic_platform.adm1266_platform_spec"] = adm1266_platform_spec_mock

        # Mock SystemDPMLogHistory to avoid file system operations
        dpm_history = Mock()
        dpm_history.save = Mock()
        sys.modules["sonic_platform.dpm"] = dpm_history

        # NOW load the adm1266 module directly from file path
        adm1266_path = os.path.join(test_dir, "../../common/sonic_platform/adm1266.py")
        spec = importlib.util.spec_from_file_location("adm1266", adm1266_path)
        adm1266_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(adm1266_module)

        self.adm = adm1266_module.Adm1266(self.adm1266_platform_spec_mock.adm1266_platform_spec)
        self.adm_get_reboot_cause = adm1266_module.get_reboot_cause

        # Set up path to test PDDF plugin file
        # Use absolute path in the container
        self.test_pddf_path = "/sonic/device/nexthop/x86_64-nexthop_5010-r0/pddf/pd-plugin.json"

        # Write the test blackbox data to the nvmem file
        self._setup_nvmem_file(data)

    def _setup_nvmem_file(self, binary_data):
        """Populate nvmem file with binary data"""
        with open(self.adm1266_platform_spec_mock.nvmem_path, 'wb') as nvmem_file:
            nvmem_file.write(binary_data)

    def get_blackbox_input(self):
        return self.blackbox_input

    def get_expected_records(self):
        return self.expected_records

    def get_expected_causes(self):
        return self.expected_causes

    def read_blackbox(self):
        return self.adm.read_blackbox()

    def get_blackbox_records(self):
        return self.adm.get_blackbox_records()

    def parse_blackbox(self, data):
        return self.adm._parse_blackbox(data)

    def get_reboot_cause(self):
        return self.adm_get_reboot_cause(self.test_pddf_path)

    def clear_blackbox(self):
        self.adm.clear_blackbox()
        self.blackbox_cleared = True


class WatchdogBaseMock:
    """Mock of WatchdogBase for testing."""

    def arm(self, seconds):
        raise NotImplementedError

    def disarm(self):
        raise NotImplementedError

    def is_armed(self):
        raise NotImplementedError

    def get_remaining_time(self):
        raise NotImplementedError


class WatchdogMock(WatchdogBaseMock):
    def __init__(
        self,
        fpga_pci_addr: str,
        event_driven_power_cycle_control_reg_offset: int,
        watchdog_counter_reg_offset: int,
    ):
        self.fpga_pci_addr: str = fpga_pci_addr
        self.event_driven_power_cycle_control_reg_offset: int = event_driven_power_cycle_control_reg_offset
        self.watchdog_counter_reg_offset: int = watchdog_counter_reg_offset


@pytest.fixture
def mock_pddf_data():
    """Fixture providing mock PDDF data for tests."""
    data_mock = Mock()
    data_mock.data = {
        "PLATFORM": {"num_nexthop_fpga_asic_temp_sensors": 0},
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
    sys.modules["sonic_platform_pddf_base.pddf_chassis"] = pddf_chassis_mock

    watchdog_mock = Mock()
    watchdog_mock.Watchdog = WatchdogMock
    sys.modules["sonic_platform.watchdog"] = watchdog_mock

    # Load the chassis module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    chassis_path = os.path.join(test_dir, "../../common/sonic_platform/chassis.py")

    spec = importlib.util.spec_from_file_location("chassis", chassis_path)
    chassis_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(chassis_module)

    return chassis_module.Chassis(pddf_data=mock_pddf_data)

@pytest.fixture
def watchdog(mock_pddf_data):
    """
    Fixture providing a Watchdog instance for testing.
    """
    # Set up the specific WatchdogBase mock with our test implementation
    watchdog_base_mock = Mock()
    watchdog_base_mock.WatchdogBase = WatchdogBaseMock
    sys.modules["sonic_platform_base.watchdog_base"] = watchdog_base_mock

    # Load the module directly from file path
    test_dir = os.path.dirname(os.path.realpath(__file__))
    watchdog_path = os.path.join(test_dir, "../../common/sonic_platform/watchdog.py")

    spec = importlib.util.spec_from_file_location("watchdog", watchdog_path)
    watchdog_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(watchdog_module)

    return watchdog_module.Watchdog(
        fpga_pci_addr="FAKE_FPGA_PCI_ADDR",
        event_driven_power_cycle_control_reg_offset=0x28,
        watchdog_counter_reg_offset=0x1E0,
    )


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
        "max_speed": 100,
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
