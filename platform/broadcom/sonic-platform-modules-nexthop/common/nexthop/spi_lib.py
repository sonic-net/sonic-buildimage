# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Library to perform common SPI device operation
"""

import os

from dataclasses import dataclass
from nexthop_utils.platform_utils import run_cmd
from nexthop.pddf_loader import load_pddf_device_config

SPI_DEV_DIR = "/sys/bus/spi/devices"

@dataclass
class SpiDeviceInfo:
    controller_name: str | None
    controller_idx: int | None
    component_name: str | None
    device_cs: int | None
    

def spi_device_to_component(device_name) -> str | None: 
    """Find the component name associated with a spi device

    :param device_name: the spi device name
    :return: component name if found, otherwise None
    """
    if "ASIC_BOOT_FLASH" in device_name:
        component_name = device_name.replace("ASIC_BOOT_FLASH", "ASIC_PCIE")
    elif "CPUCARD" in device_name or "SWITCHCARD" in device_name or "MEZZCARD" in device_name:
        component_name = device_name.replace("CONFIG_FLASH", "FPGA")
    # TODO: Add MGMT PHY support
    else:
        component_name = None
    
    # TODO: Add validation against platform_components.json OR platform.json
    return component_name

def component_to_spi_device(component_name) -> str | None:
    """Find the spi device name associated with a component

    :param device_name: the component name
    :return: component name if found, otherwise None
    """
    if "FPGA" in component_name:
        device_name = component_name.replace("FPGA", "CONFIG_FLASH")
    elif "ASIC_PCIE" in component_name:
        device_name = component_name.replace("PCIE", "BOOT_FLASH")
    # TODO: Add MGMT PHY support
    else:
        device_name = None
        
    # TODO: Add validation against platform_component.json OR platform.json
    return device_name


def get_spi_device_info(device_name, pddf_config=None) -> SpiDeviceInfo | None:
    """  Get SPI device info for a given device name """
    if pddf_config is None:
        pddf_config = load_pddf_device_config()
    
    spi_device_obj = pddf_config.get(device_name)
    if not spi_device_obj:
        return None
    
    component_name = spi_device_to_component(device_name)

    device_cs = spi_device_obj["dev_attr"]["chip_select"]
    controller_name = spi_device_obj["dev_info"]["device_parent"]
    controller_obj = pddf_config.get(controller_name)
    if not controller_obj:
        controller_idx = None
        controller_name = None
    else:
        controller_idx = controller_obj["dev_attr"]["spi_controller_idx"]
    return SpiDeviceInfo(
        controller_name = controller_name,
        controller_idx = controller_idx,
        component_name = component_name,
        device_cs = device_cs
    )

def get_mtd_device_path(spi_device_name: str) -> str:
    """
    Get the mtd device /dev/mtdX name for a given SPI device name
    
    Args:
        spi_device_name: SPI device name (e.g. ASIC_BOOT_FLASH)

    Returns:
        mtd device name (e.g. /dev/mtd0) if found
    
    Raises:
        Exception if not able to find the mtd device path
    """
    spi_device_info = get_spi_device_info(spi_device_name)
    if spi_device_info is None or spi_device_info.controller_idx is None or spi_device_info.device_cs is None:
        raise Exception(f"Failed to get SPI device info for {spi_device_name}")
    expected_spi_name = f"spi{spi_device_info.controller_idx}.{spi_device_info.device_cs}"
    spi_dev_path = os.path.join(SPI_DEV_DIR, expected_spi_name)
    if not os.path.isdir(spi_dev_path):
        raise Exception(f"SPI device {expected_spi_name} not found in {SPI_DEV_DIR}")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")
    if not os.path.isdir(mtd_dev_dir):
        raise Exception(f"mtd directory {mtd_dev_dir} not created for {expected_spi_name}")
    entries = [e for e in os.listdir(mtd_dev_dir) if not e.endswith("ro")]
    if len(entries) != 1:
        raise Exception(f"Expected exactly one mtd device under {mtd_dev_dir}, found: {entries}")
    mtd_dev = os.path.join("/dev", entries[0])
    if not os.path.exists(mtd_dev):
        raise Exception(f"mtd device {mtd_dev} not found in /dev")
    return mtd_dev
