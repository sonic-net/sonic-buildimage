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
        self.plugin_data = {'REBOOT_CAUSE': {'reboot_cause_file': '/tmp/test_reboot_cause'}}

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

class DpmInfoMock:
    def __init__(self):
        # Carelessly made up stuff - Not accurate for any HW platform

        self.dpm_signals = {
            1 : 2,  # PDIO bit 0 - Fault code bit 2
            14 : 0, # PDIO bit 13 - Fault code bit 0
            15 : 1  # PDIO bit 14 - Fault code bit 1
        }
        self.dpm_table = {
            0 : "",
            1 : "THERMTRIP_L: CPU has exceeded Tdie,shutdown",
            2 : "CPU_PWR_CYC_REQ",
            3 : "BMC_PWR_CYC_REQ",
            4 : "FPGA_PWR_CYC_REQ",
            5 : "Switch Card CP power bad"
        }
        self.power_fault_cause = {
            0: ("PSU_VIN_LOSS",     "Both PSUs lost input power"),  # PDIO1 (0)
            1: ("OVER_TEMP",        "Switch card temp sensor OT)"), # PDIO2 (1)
            2: ("CPU_PWR_BAD",      "CPU card power bad"),          # PDIO3 (2)
            3: ("WACHDOG",          "FPGA watchdog expired"),       # PDIO4 (3)
            4: ("ASIC_OT",          "ASIC MAX_TEMP exceeded OT threshold"), # PDIO5 (4)
            5: ("NO_FAN_PRSNT",     "All 4 fans have same ID=0xf"),         # PDIO6 (5)
            6: ("CMD_PWR_CYC",      "Software commanded power cycle"),      # PDIO7 (6)
            7: ("DP_PWR_ON",        "P2 only: from shift chain; not used on P1"), # PDIO8 (7)
            9: ("FPGA_CMD_PCYC",    "FPGA commanded power cycle"),                # PDIO10 (9)
            10:("CMD_ASIC_PWR_OFF", "FPGA command ASIC power off"),               # PDIO11 (10)
        }

        self.vp_to_pdio_desc = {
                5: { "pdio": 2, "rail": "POS0V75_S5" },  # VP6  -> PDIO3
                6: { "pdio": 3, "rail": "POS1V8_S5" },   # VP7  -> PDIO4
                7: { "pdio": 4, "rail": "POS3V3_S5" },   # VP8  -> PDIO5
                8: { "pdio": 6, "rail": "POS1V1_S0" },   # VP9  -> PDIO7
                9: { "pdio": 7, "rail": "POS0V78_S0" },  # VP10 -> PDIO8
                10: { "pdio": 8, "rail": "POS0V75_S0" }, # VP11 -> PDIO9
                11: { "pdio": 9, "rail": "POS1V8_S0" },  # VP12 -> PDIO10
                12: { "pdio": 10, "rail": "POS3V3_S0" }, # VP13 -> PDIO11
        }

        self.vh_to_pdio_desc = {
                4: { "pdio": 5, "rail": "POS5V0_S0" },   # VH4_UV (bit 4) -> PDIO6 (bit 5)
        }
        self._create_nvmem_path()

    def get_vp_to_pdio_desc(self):
        return self.vp_to_pdio_desc

    def get_vh_to_pdio_desc(self):
        return self.vh_to_pdio_desc

    def get_dpm_signals(self):
        return self.dpm_signals

    def get_dpm_table(self):
        return self.dpm_table

    def get_power_fault_cause(self):
        return self.power_fault_cause

    def get_nvmem_path(self):
        return self.nvmem_path

    def get_name(self):
        return "dpm-mock"

    def _create_nvmem_path(self):
        """Create temporary file with binary data and return path"""
        nvmem_file = tempfile.NamedTemporaryFile(delete=False)
        nvmem_file.close()
        self.nvmem_path = nvmem_file.name

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
        os.path.dirname(__file__),

        json_file = os.path.join(os.path.dirname(__file__), "adm1266_test_spec.json")
        data, records, causes = process_input(json_file)
        self.blackbox_input = data
        self.expected_records = records
        self.expected_causes = causes

        self.dpm_info = DpmInfoMock()

        # Load the adm1266 module directly from file path
        test_dir = os.path.dirname(os.path.realpath(__file__))
        adm1266_path = os.path.join(test_dir, "../../common/sonic_platform/adm1266.py")

        spec = importlib.util.spec_from_file_location("adm1266", adm1266_path)
        adm1266_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(adm1266_module)
        self.adm = adm1266_module.Adm1266(self.dpm_info)

        self._setup_nvmem_file(data)

    def _setup_nvmem_file(self, binary_data):
        """Populate nvmem file with binary data """
        with open(self.dpm_info.get_nvmem_path(), 'wb') as nvmem_file:
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

    def get_reboot_causes(self):
        return self.adm.get_reboot_causes()

    def parse_blackbox(self, data):
        return self.adm.parse_blackbox(data)

    def get_reboot_cause(self):
        return self.adm.get_reboot_cause()

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
