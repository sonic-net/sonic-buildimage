#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Read the CF2 mgmt-switch config version from the
config EEPROM. Writes to the FPGA and mounts a PDDF subtree
"""

import os
import subprocess
import sys

from nexthop.fpga_lib import (
    name_to_bdf,
    write_32,
    read_32,
    overwrite_field,
)

VERSION_BYTE_0 = 0x90
VERSION_BYTE_1 = 0x83
# EEPROM Version is stored on two discountinious bytes
EEPROM_DEVICE_NAME = "MGMT-SWCH-EEPROM"
CACHE_FILE = "/tmp/mgmt_switch_version"

# Have to access EEPROM through cpu FPGA, must write bit to specfic addr
FPGA_DEVICE = "CPUCARD_FPGA"
FPGA_MGMT_EEPROM_TOGGLE_ADDR = 0xC
FPGA_MGMT_EEPROM_TOGGLE_BIT = (8, 8)


def _cpu_fpga_mgmt_toggle(value: int) -> None:
    """Sets a bit on the CPUCARD_FPGA to enable/disable the i2c path to the mgmt-switch EEPROM."""
    bdf = name_to_bdf(FPGA_DEVICE)
    if bdf is None:
        raise RuntimeError(f"could not resolve PCI BDF for FPGA {FPGA_DEVICE}")
    old_value = read_32(bdf, FPGA_MGMT_EEPROM_TOGGLE_ADDR)
    reg_value = overwrite_field(old_value, FPGA_MGMT_EEPROM_TOGGLE_BIT, value)
    write_32(bdf, FPGA_MGMT_EEPROM_TOGGLE_ADDR, reg_value)

PDDF_PLUGINS_PATH = "/usr/share/sonic/platform/plugins"


def _pddfparse():
    """Return a PddfParse instance, importing it from the platform plugins path."""
    if PDDF_PLUGINS_PATH not in sys.path:
        sys.path.append(PDDF_PLUGINS_PATH)
    import pddfparse
    return pddfparse.PddfParse()


def _eeprom_sysfs_path(pddf, device_name: str) -> str:
    """Resolve the PDDF I2C EEPROM sysfs 'eeprom' path via pddfparse."""
    path = pddf.get_path(device_name, "eeprom")
    if not path:
        raise RuntimeError(f"no eeprom path for {device_name} in pddf-device.json")
    return path


def _read_word(path: str, addr: int) -> int:
    """Read a byte from the given sysfs path and return it as an integer"""
    with open(path, "rb") as f:
        f.seek(addr)
        data = f.read(1)
    return int.from_bytes(data, "big")


def _read_cache(path: str = CACHE_FILE) -> str | None:
    """Return the cached version string from path, or None if absent/empty."""
    try:
        with open(path) as f:
            v = f.read().strip()
            if v:
                return v
    except OSError:
        pass
    return None


def _write_cache(version: str, path: str = CACHE_FILE) -> None:
    """Persist version string to path; world-readable so non-root show commands can use it."""
    with open(path, "w") as f:
        f.write(version)
    os.chmod(path, 0o644)



def _read_version() -> tuple[int,int]:
    """Return the version byte and the vlan version byte

    Enable the FPGA-gated i2c path and create the PDDF subtree around the read, then
    tear both down again
    """
    pddf = _pddfparse()
    _cpu_fpga_mgmt_toggle(1)
    pddf.create_subtree(EEPROM_DEVICE_NAME)
    try:
        dev = _eeprom_sysfs_path(pddf, EEPROM_DEVICE_NAME)
        version = _read_word(dev, VERSION_BYTE_0)
        vlan_version = _read_word(dev, VERSION_BYTE_1)
        return version, vlan_version
    finally:
        pddf.delete_subtree(EEPROM_DEVICE_NAME)
        _cpu_fpga_mgmt_toggle(0)


def main() -> int:
    if os.geteuid() != 0:
        sys.exit(subprocess.call(["sudo", sys.argv[0]] + sys.argv[1:]))

    vlan = "--vlan" in sys.argv[1:]
    cache_file = CACHE_FILE + ("_vlan" if vlan else "")
    cached = _read_cache(cache_file)
    if cached is not None:
        print(cached)
        return 0

    version, vlan_version = _read_version()
    result = "0." + str(vlan_version if vlan else version)
    _write_cache(result, cache_file)
    print(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
