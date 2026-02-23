#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the pddf_config_parser module.
"""

import sys
import os
import json
import pytest
from unittest.mock import MagicMock


@pytest.fixture(scope="function", autouse=True)
def pddf_config_parser_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import pddf_config_parser

    yield pddf_config_parser

# Mock sonic_py_common if not available
try:
    import sonic_py_common
except ImportError:
    sys.modules["sonic_py_common"] = MagicMock()
    sys.modules["sonic_py_common.logger"] = MagicMock()

CWD = os.path.dirname(os.path.realpath(__file__))
BASE_PLATFORM_PDDF_PATH = "../../../../../../device/nexthop/{}/pddf"


def find_pddf_device_json(platform_variant):
    """Find the pddf-device.json file for the given platform variant."""
    # First try to find pddf-device.json.j2 which dynamically generates pddf-device.json at runtime.
    platform_pddf_path = os.path.join(
        CWD, BASE_PLATFORM_PDDF_PATH.format(platform_variant)
    )
    pddf_template_path = os.path.join(platform_pddf_path, "pddf-device.json.j2")
    # If all else fails, fallback to pddf-device.json
    fallback_path = os.path.join(platform_pddf_path, "pddf-device.json")
    if os.path.exists(pddf_template_path):
        return pddf_template_path
    else:
        return fallback_path


class TestExtractXcvrList:
    """Test class for 'extract_xcvr_list' function."""

    def test_extract_xcvr_list(self, pddf_config_parser_module):
        """Test extract_xcvr_list with a sample configuration."""
        # Sample PDDF config
        config = {
            "PORT1-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x0f", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                        {"attr_name": "xcvr_present"},
                    ],
                }
            },
            "PORT32-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x2e", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
            "PORT33-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x35", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
            "PORTMGMT-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x36", "dev_addr": "0x09"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
            "SOME_OTHER_DEVICE": {
                "i2c": {"topo_info": {"parent_bus": "0x01", "dev_addr": "0x50"}}
            },
        }

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then
        expected = [
            {"name": "PORT1-CTRL", "bus": 15, "addr": "0008"},
            {"name": "PORT32-CTRL", "bus": 46, "addr": "0008"},
            {"name": "PORT33-CTRL", "bus": 53, "addr": "0008"},
        ]

        assert len(xcvr_list) == 3
        for expected_xcvr in expected:
            assert expected_xcvr in xcvr_list

    def test_extract_xcvr_list_missing_required_attrs(self, pddf_config_parser_module):
        """Test that devices without both xcvr_reset and xcvr_lpmode are filtered out."""
        config = {
            "PORT1-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x0f", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"}
                        # Missing xcvr_lpmode
                    ],
                }
            },
            "PORT2-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x10", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_lpmode"}
                        # Missing xcvr_reset
                    ],
                }
            },
            "PORT3-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x11", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
        }

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - only PORT3-CTRL should be included
        assert len(xcvr_list) == 1
        assert xcvr_list[0]["name"] == "PORT3-CTRL"

    def test_extract_xcvr_list_non_port_ctrl_devices(self, pddf_config_parser_module):
        """Test that non-PORT*-CTRL devices are filtered out."""
        config = {
            "PORT1-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x0f", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
            "SOME_FAN": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x10", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
            "PORT_INVALID": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x11", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
        }

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - only PORT1-CTRL should be included
        assert len(xcvr_list) == 1
        assert xcvr_list[0]["name"] == "PORT1-CTRL"

    def test_extract_xcvr_list_missing_i2c_section(self, pddf_config_parser_module):
        """Test that devices without i2c section are filtered out."""
        config = {
            "PORT1-CTRL": {
                "some_other_section": {}
                # Missing i2c section
            },
            "PORT2-CTRL": {
                "i2c": {
                    "topo_info": {"parent_bus": "0x10", "dev_addr": "0x08"},
                    "attr_list": [
                        {"attr_name": "xcvr_reset"},
                        {"attr_name": "xcvr_lpmode"},
                    ],
                }
            },
        }

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - only PORT2-CTRL should be included
        assert len(xcvr_list) == 1
        assert xcvr_list[0]["name"] == "PORT2-CTRL"

    def test_extract_xcvr_list_empty_config(self, pddf_config_parser_module):
        """Test extract_xcvr_list with empty configuration."""
        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list({})

        # Then
        assert xcvr_list == []

    @pytest.mark.parametrize(
        "platform_variant",
        ["x86_64-nexthop_4010-r0", "x86_64-nexthop_4010-r1"],
    )
    def test_extract_xcvr_list_real_40x0_config(self, pddf_config_parser_module, platform_variant):
        """Test extract_xcvr_list with real NH-40x0 pddf-device.json configuration."""
        # Path to the real pddf-device.json file
        config_path = find_pddf_device_json(platform_variant)

        # Load the real configuration
        with open(config_path, "r") as f:
            config = json.load(f)

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - validate the results
        assert isinstance(xcvr_list, list)

        # NH-40x0 should have 64 OSFP transceivers
        assert len(xcvr_list) == 64

        # First port starts at bus 23
        xcvr_port1 = next(xcvr for xcvr in xcvr_list if xcvr["name"] == "PORT1-CTRL")
        assert xcvr_port1["bus"] == 23
        assert xcvr_port1["addr"] == "0008"

        # Verify all entries have unique names and bus numbers
        names = [xcvr["name"] for xcvr in xcvr_list]
        buses = [xcvr["bus"] for xcvr in xcvr_list]
        assert len(names) == len(set(names)), "All transceiver names should be unique"
        assert len(buses) == len(set(buses)), "All bus numbers should be unique"

    def test_extract_xcvr_list_real_4220_config(self, pddf_config_parser_module):
        """Test extract_xcvr_list with real NH-4220 pddf-device.json configuration."""
        # Path to the real pddf-device.json file
        config_path = find_pddf_device_json("x86_64-nexthop_4220-r0")

        # Load the real configuration
        with open(config_path, "r") as f:
            config = json.load(f)

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - validate the results
        assert isinstance(xcvr_list, list)

        # NH-4220 should have 64 OSFP transceivers
        assert len(xcvr_list) == 64

        # First port starts at bus 23
        xcvr_port1 = next(xcvr for xcvr in xcvr_list if xcvr["name"] == "PORT1-CTRL")
        assert xcvr_port1["bus"] == 24
        assert xcvr_port1["addr"] == "0008"

        # Verify all entries have unique names and bus numbers
        names = [xcvr["name"] for xcvr in xcvr_list]
        buses = [xcvr["bus"] for xcvr in xcvr_list]
        assert len(names) == len(set(names)), "All transceiver names should be unique"
        assert len(buses) == len(set(buses)), "All bus numbers should be unique"

    def test_extract_xcvr_list_real_5010_config(self, pddf_config_parser_module):
        """Test extract_xcvr_list with real NH-5010 pddf-device.json configuration."""
        # Path to the real pddf-device.json file
        config_path = find_pddf_device_json("x86_64-nexthop_5010-r0")

        # Load the real configuration
        with open(config_path, "r") as f:
            config = json.load(f)

        # When
        xcvr_list = pddf_config_parser_module.extract_xcvr_list(config)

        # Then - validate the results
        assert isinstance(xcvr_list, list)

        # NH-5010 should have 32 QSFP-DD, 32 QSFP112, and 2 QSFP28 transceivers
        assert len(xcvr_list) == 32 + 32 + 2

        # First QSFP-DD starts at bus 53
        xcvr_port1 = next(xcvr for xcvr in xcvr_list if xcvr["name"] == "PORT1-CTRL")
        assert xcvr_port1["bus"] == 53
        assert xcvr_port1["addr"] == "0008"
        # First QSFP112 starts at bus 18
        xcvr_port33 = next(xcvr for xcvr in xcvr_list if xcvr["name"] == "PORT33-CTRL")
        assert xcvr_port33["bus"] == 18
        assert xcvr_port33["addr"] == "0008"

        # Verify all entries have unique names and bus numbers
        names = [xcvr["name"] for xcvr in xcvr_list]
        buses = [xcvr["bus"] for xcvr in xcvr_list]
        assert len(names) == len(set(names)), "All transceiver names should be unique"
        assert len(buses) == len(set(buses)), "All bus numbers should be unique"


class TestExtractFpgaDevAttrs:

    @pytest.fixture(autouse=True)
    def setup(self, pddf_config_parser_module):
        self.FPGA_TYPES = (
            pddf_config_parser_module.FpgaDeviceName.CPU_CARD.value,
            pddf_config_parser_module.FpgaDeviceName.SWITCHCARD.value,
        )

    def test_extract_fpga_attrs_malformed_config(self, pddf_config_parser_module):
        bad_configs = []
        no_dev_attr_config = {
            "MULTIFPGAPCIE0": {
                "dev_info": {
                    "device_name": "CPUCARD_FPGA",
                    "device_type": "MULTIFPGAPCIE",
                }
            }
        }
        bad_configs.append(no_dev_attr_config)

        no_pwr_cycle_config_reg_config = {
            "MULTIFPGAPCIE0": {
                "dev_info": {
                    "device_name": "SWITCHCARD_FPGA",
                    "device_type": "MULTIFPGAPCIE",
                },
                "dev_attr": {
                    "pwr_cycle_enable_word": "0xDEADBEEF",
                },
            }
        }
        bad_configs.append(no_pwr_cycle_config_reg_config)

        for config in bad_configs:
            with pytest.raises(Exception):
                pddf_config_parser_module.extract_fpga_attrs(config, self.FPGA_TYPES)

    @pytest.mark.parametrize(
        "platform_variant",
        [
            "x86_64-nexthop_4010-r0",
            "x86_64-nexthop_4010-r1",
            "x86_64-nexthop_5010-r0",
        ],
    )
    def test_extract_fpga_attrs(self, pddf_config_parser_module, platform_variant):
        """Test extract_fpga_attrs with real NH pddf-device.json configuration."""
        # Path to the real pddf-device.json file
        config_path = find_pddf_device_json(platform_variant)

        # Load the real configuration
        with open(config_path, "r") as f:
            config = json.load(f)

        # When
        fpga_attrs = pddf_config_parser_module.extract_fpga_attrs(config, self.FPGA_TYPES)

        # Then
        assert fpga_attrs == {
            pddf_config_parser_module.FpgaDeviceName.CPU_CARD.value: pddf_config_parser_module.FpgaDevAttrs(
                pwr_cycle_reg_offset=0x8,
                pwr_cycle_enable_word=0xDEADBEEF,
            ),
            pddf_config_parser_module.FpgaDeviceName.SWITCHCARD.value: pddf_config_parser_module.FpgaDevAttrs(
                pwr_cycle_reg_offset=0x4,
                pwr_cycle_enable_word=0xDEADBEEF,
            ),
        }
