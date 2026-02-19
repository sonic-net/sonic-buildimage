#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test helpers for ADM1266 testing.

This module contains common utilities used by both unit and integration tests
for ADM1266 functionality. It helps avoid code duplication between test files.
"""

import os
import tempfile


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

    with open(json_file, "r") as f:
        spec = json.load(f)

    if "hexdump_lines" not in spec:
        raise ValueError("JSON must include hexdump_lines")
    blackbox_data = parse_hexdump_lines(spec["hexdump_lines"])
    expected_records = spec.get("expected_blackbox_records")
    expected_causes = spec.get("expected_reboot_causes")

    return blackbox_data, expected_records, expected_causes


class Adm1266TestMixin:
    """
    Mixin class that provides ADM1266 test helper methods to test classes.

    Test classes can inherit from this mixin to get access to all the helper methods
    as instance methods, which is convenient for test organization.
    """

    # Set up path to test PDDF plugin file
    # Use absolute path in the container
    TEST_PDDF_PATH = "/sonic/device/nexthop/x86_64-nexthop_5010-r0/pddf/pd-plugin.json"

    EXCERPT_PDDF_PLUGIN_DATA = {
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
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                    },
                    {
                        "pdio_mask": "0x0002",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0002",
                        "gpio_value": "0x0000",
                        "hw_cause": "OVER_TEMP",
                        "hw_desc": "Switch card temp sensor OT",
                        "summary": "Temperature exceeded threshold",
                        "reboot_cause": "REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER",
                    },
                    {
                        "pdio_mask": "0x0004",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0004",
                        "gpio_value": "0x0000",
                        "hw_cause": "CPU_PWR_BAD",
                        "hw_desc": "CPU card power bad",
                        "summary": "CPU power failure",
                        "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER",
                    },
                    {
                        "pdio_mask": "0x0008",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0008",
                        "gpio_value": "0x0000",
                        "hw_cause": "WATCHDOG",
                        "hw_desc": "FPGA watchdog expired",
                        "summary": "Watchdog timeout",
                        "reboot_cause": "REBOOT_CAUSE_WATCHDOG",
                    },
                    {
                        "pdio_mask": "0x0010",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0010",
                        "gpio_value": "0x0000",
                        "hw_cause": "ASIC_OT",
                        "hw_desc": "ASIC MAX_TEMP exceeded OT threshold",
                        "summary": "ASIC overtemperature",
                        "reboot_cause": "REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC",
                    },
                    {
                        "pdio_mask": "0x0020",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0020",
                        "gpio_value": "0x0000",
                        "hw_cause": "NO_FAN_PRSNT",
                        "hw_desc": "All 4 fans have same ID=0xf",
                        "summary": "No fans present",
                        "reboot_cause": "REBOOT_CAUSE_HARDWARE_OTHER",
                    },
                    {
                        "pdio_mask": "0x0040",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0040",
                        "gpio_value": "0x0000",
                        "hw_cause": "CMD_PWR_CYC",
                        "hw_desc": "Software commanded power cycle",
                        "summary": "Software power cycle",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                    },
                    {
                        "pdio_mask": "0x0080",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0080",
                        "gpio_value": "0x0000",
                        "hw_cause": "DP_PWR_ON",
                        "hw_desc": "P2 only: from shift chain; not used on P1",
                        "summary": "DP power on",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                    },
                    {
                        "pdio_mask": "0x0100",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0100",
                        "gpio_value": "0x0000",
                        "hw_cause": "FPGA_CMD_PCYC",
                        "hw_desc": "FPGA commanded power cycle",
                        "summary": "FPGA power cycle",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                    },
                    {
                        "pdio_mask": "0x0200",
                        "gpio_mask": "0x0000",
                        "pdio_value": "0x0200",
                        "gpio_value": "0x0000",
                        "hw_cause": "CMD_ASIC_PWR_OFF",
                        "hw_desc": "FPGA command ASIC power off",
                        "summary": "ASIC power off",
                        "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                    },
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
                "vhx_to_rail_desc": {"5": "POS5V0_S0"},
            }
        }
    }
    adm1266_platform_spec = None

    def setup_class(self):
        # Load test data from JSON spec.
        TEST_DIR = os.path.dirname(os.path.realpath(__file__))
        json_file = os.path.join(TEST_DIR, "adm1266_test_spec.json")
        data, records, causes = process_input(json_file)
        self.blackbox_input = data
        self.expected_records = records
        self.expected_causes = causes

    def setup_method(self):
        # Prepare nvmem_path and pddf_plugin_data
        nvmem_file = tempfile.NamedTemporaryFile(delete=False)
        nvmem_file.write(self.blackbox_input)
        nvmem_file.close()
        self.nvmem_path = nvmem_file.name
        self.pddf_plugin_data = self.EXCERPT_PDDF_PLUGIN_DATA
        self.pddf_plugin_data["DPM"]["dpm-mock"]["nvmem_path"] = self.nvmem_path

        # Delayed initialization until runtime, when all dependencies are properly patched.
        self.adm1266_platform_spec = None

    def teardown_method(self):
        """Clean up temporary file"""
        if self.nvmem_path and os.path.exists(self.nvmem_path):
            os.unlink(self.nvmem_path)

    def get_nvmem_path(self):
        return self.nvmem_path

    def get_blackbox_input(self):
        return self.blackbox_input

    def get_expected_records(self):
        return self.expected_records

    def get_expected_causes(self):
        return self.expected_causes

    def get_pddf_plugin_data(self):
        return self.pddf_plugin_data

    def get_fake_adm1266_platform_spec(self):
        if self.adm1266_platform_spec is None:
            # Do the import here, as all dependencies should already be patched at this point.
            from sonic_platform.adm1266_platform_spec import Adm1266PlatformSpec

            self.adm1266_platform_spec = Adm1266PlatformSpec("dpm-mock", self.pddf_plugin_data)
        return self.adm1266_platform_spec

    def get_test_pddf_path(self):
        return self.TEST_PDDF_PATH
