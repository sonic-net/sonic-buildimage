#!/usr/bin/env python3

"""
PDDF config extraction utilities
This module provides functions to extract information from PDDF configuration
"""

import json
import os
import re

from enum import Enum
from sonic_py_common import device_info

PDDF_DEVICE_JSON_PATH = "/usr/share/sonic/platform/pddf/pddf-device.json"
PD_PLUGIN_JSON_PATH = "/usr/share/sonic/platform/pddf/pd-plugin.json"


class FpgaDeviceName(str, Enum):
    CPU_CARD = "CPUCARD_FPGA"
    SWITCHCARD = "SWITCHCARD_FPGA"
    SWITCHCARD_0 = "SWITCHCARD_FPGA0"


def load_pddf_device_config():
    """Load and parse pddf-device.json configuration. Raises exception on error."""
    with open(PDDF_DEVICE_JSON_PATH, "r") as f:
        config = json.load(f)
    return config


def load_pd_plugin_config() -> dict:
    """Load and parse pd-plugin.json configuration. Raises exception on error."""
    with open(PD_PLUGIN_JSON_PATH, "r") as f:
        config = json.load(f)
    return config


def load_platform_json() -> dict:
    """
    Load and parse platform.json configuration file.

    Returns:
        Dictionary containing the platform.json configuration.

    Raises:
        FileNotFoundError: If platform.json file does not exist.
        json.JSONDecodeError: If the file contains invalid JSON.
        Exception: For other file reading errors.
    """
    platform_path = device_info.get_path_to_platform_dir()
    platform_json_file = os.path.join(platform_path, device_info.PLATFORM_JSON_FILE)

    with open(platform_json_file, 'r') as f:
        config = json.load(f)
    return config


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


FPGA_REBOOT_CONF_KEY = "FPGA_REBOOT_CONF"
SWITCH_HOST_KEY = "switch_host_pwrcyc"
BMC_IS_PLATFORM_MANAGER_KEY = "bmc_is_platform_manager"


def reboot_conf(config: dict) -> dict:
    conf = config.get(FPGA_REBOOT_CONF_KEY)
    if not conf:
        raise ValueError(f"No {FPGA_REBOOT_CONF_KEY} in PDDF configuration")
    return conf
