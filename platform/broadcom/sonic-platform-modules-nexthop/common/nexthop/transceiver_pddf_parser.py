#!/usr/bin/env python3

"""
PDDF transceiver extraction utilities
This module provides functions to extract transceiver information from PDDF configuration
"""

import re


def extract_xcvr_list(config):
    """Extract transceiver information for initialization from PDDF config"""
    xcvr_list = []

    if not config:
        return xcvr_list

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
                {"name": device_name, "bus": int(bus, 16), "addr": f"{int(addr, 16):04x}"}
            )

    return xcvr_list
