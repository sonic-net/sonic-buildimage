#!/usr/bin/env python3

"""
PDDF config extraction utilities
This module provides functions to extract information from PDDF configuration
"""

import json
import re

from dataclasses import dataclass
from enum import Enum

PDDF_DEVICE_JSON_PATH = "/usr/share/sonic/platform/pddf/pddf-device.json"


class FpgaDeviceName(str, Enum):
    CPU_CARD = "CPUCARD_FPGA"
    SWITCHCARD = "SWITCHCARD_FPGA"


@dataclass
class FpgaDevAttrs:
    pwr_cycle_reg_offset: int
    pwr_cycle_enable_word: int


def load_pddf_device_config():
    """Load and parse pddf-device.json configuration. Raises exception on error."""
    with open(PDDF_DEVICE_JSON_PATH, "r") as f:
        config = json.load(f)
    return config


def extract_fpga_attrs(config, fpga_types):
    """Extract fpga attrs information from PDDF config. Raise exception on error."""
    fpga_attrs = {}
    for device_name, device_config in config.items():
        if not isinstance(device_config, dict) or "dev_info" not in device_config:
            continue

        dev_info = device_config["dev_info"]
        if "device_type" not in dev_info or dev_info["device_type"] != "MULTIFPGAPCIE":
            continue

        device_name = dev_info["device_name"]
        if device_name not in fpga_types:
            continue

        # Exceptions thrown here since FPGAs should have these attributes
        # past this point.
        dev_attrs = device_config["dev_attr"]

        fpga_attrs[device_name] = FpgaDevAttrs(
            pwr_cycle_reg_offset=int(dev_attrs["pwr_cycle_reg_offset"], 16),
            pwr_cycle_enable_word=int(dev_attrs["pwr_cycle_enable_word"], 16),
        )

    return fpga_attrs


def extract_xcvr_list(config):
    """Extract transceiver information for initialization from PDDF config"""
    xcvr_list = []

    port_ctrl_pattern = re.compile(r"^PORT\d+-CTRL$")

    for device_name, device_config in config.items():
        if not isinstance(device_config, dict):
            continue

        # Check if device name matches PORT\d+-CTRL pattern
        if not port_ctrl_pattern.match(device_name):
            continue

        # Check if device has i2c section
        if "i2c" not in device_config:
            continue

        i2c_config = device_config["i2c"]

        # Check if attr_list exists and contains required attributes
        attr_list = i2c_config.get("attr_list", [])
        has_reset = False
        has_lpmode = False

        for attr in attr_list:
            if isinstance(attr, dict):
                attr_name = attr.get("attr_name", "")
                if attr_name == "xcvr_reset":
                    has_reset = True
                elif attr_name == "xcvr_lpmode":
                    has_lpmode = True

        # Only include devices that have both required attributes
        if not (has_reset and has_lpmode):
            continue

        # Extract bus and address from topo_info
        topo_info = i2c_config.get("topo_info", {})
        bus = topo_info.get("parent_bus")
        addr = topo_info.get("dev_addr")

        if bus and addr:
            xcvr_list.append(
                {
                    "name": device_name,
                    "bus": int(bus, 16),
                    "addr": f"{int(addr, 16):04x}",
                }
            )

    return xcvr_list
