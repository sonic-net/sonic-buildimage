#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Read the CF2 mgmt-switch config version.

Every CF2 CPU card keeps the switch config in an EEPROM behind an FPGA-gated i2c
path. Legacy CPU card carries an 88E6341 whose version sits at fixed offsets in
that image. Latest CPU cards carry an 88E6191X whose image stamps the version into the
switch's Global2 scratch bytes; the version is recovered by decoding those writes
out of the image, or with --mdio by reading the scratch bytes live over the CPU FPGA
MDIO master. The EEPROM path is the default because when the BMC is the platform
owner it alone may drive the 88E6191X MDIO. The CPU card is identified from the ONIE
platform string revision field.

Any failure is logged to syslog and reported as N/A so callers such as
`show platform firmware status` keep their output intact.
"""

import contextlib
import fcntl
import os
import re
import subprocess
import sys
import syslog

from sonic_py_common import device_info

from nexthop.fpga_lib import (
    name_to_bdf,
    write_32,
    read_32,
    overwrite_field,
)
from nexthop import mdio_lib

# --- 88E6341: version bytes in the config EEPROM ---
VERSION_BYTE_0 = 0x90
VERSION_BYTE_1 = 0x83
# EEPROM Version is stored on two discountinious bytes
EEPROM_DEVICE_NAME = "MGMT-SWCH-EEPROM"
CACHE_FILE = "/tmp/mgmt_switch_version"
VERSION_UNAVAILABLE = "N/A"

# Have to access EEPROM through cpu FPGA, must write bit to specfic addr
FPGA_DEVICE = "CPUCARD_FPGA"
FPGA_MGMT_EEPROM_TOGGLE_ADDR = 0xC
FPGA_MGMT_EEPROM_TOGGLE_BIT = (8, 8)

# --- 88E6191X: version in switch scratch bytes over FPGA MDIO ---
FPGA_MDIO_MASTER_OFFSET = 0xF0
MV88E6191X_BRIDGE_PHY = 0x1E
MV88E6191X_PORT0_DEVICE = 0x00
MV88E6191X_SWITCH_ID = 0x1920
LOCK_FILE = "/run/lock/mgmt_switch_version.lock"

# The 88E6191X image is Z80 code; the version is written to Global2 Scratch & Misc as
#   LD HL,<Update|pointer<<8|data> ; PUSH HL ; LD HL,0x1A1C ; PUSH HL ; CALL
# with scratch byte 0 = major and byte 1 = minor.
MV88E6191X_IMAGE_SIZE = 4096
SCRATCH_UPDATE = 1 << 15
SCRATCH_WRITE_RE = re.compile(
    rb"\x21(?P<value>..)\xe5\x21"
    + bytes([mdio_lib.GLOBAL2_DEVICE, mdio_lib.G2_REG_SCRATCH_MISC])
    + rb"\xe5\xcd",
    re.DOTALL,
)

# Prototype platform naming: x86_64-nexthop_<sku>-r<S><CC><B>, CC = CPU card type + HW revision.
PLATFORM_REVISION_RE = re.compile(r"-r(\d)(\d\d)(\d)$")
# Legacy CF2 CPU card code - uses 88E6341.
CPU_CARD_88E6341 = 2


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



@contextlib.contextmanager
def _hardware_lock():
    """Serialize concurrent readers: the EEPROM path creates and deletes the PDDF i2c
    device and flips the FPGA gate, and the MDIO master is a single shared block."""
    os.makedirs(os.path.dirname(LOCK_FILE), exist_ok=True)
    with open(LOCK_FILE, "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


def _with_config_eeprom(read):
    """Call read(sysfs_path) with the FPGA-gated i2c path enabled and the PDDF subtree
    created, then tear both down again."""
    with _hardware_lock():
        pddf = _pddfparse()
        _cpu_fpga_mgmt_toggle(1)
        pddf.create_subtree(EEPROM_DEVICE_NAME)
        try:
            return read(_eeprom_sysfs_path(pddf, EEPROM_DEVICE_NAME))
        finally:
            pddf.delete_subtree(EEPROM_DEVICE_NAME)
            _cpu_fpga_mgmt_toggle(0)


def _read_88e6341_version() -> tuple[int, int]:
    """Return the version byte and the vlan version byte from the 88E6341 config EEPROM."""
    return _with_config_eeprom(lambda dev: (_read_word(dev, VERSION_BYTE_0), _read_word(dev, VERSION_BYTE_1)))


def _read_88e6191x_image() -> bytes:
    def read(dev):
        with open(dev, "rb") as f:
            return f.read(MV88E6191X_IMAGE_SIZE)

    return _with_config_eeprom(read)


def _decode_88e6191x_version(image: bytes) -> str:
    """Return "<major>.<minor>" from the scratch-byte writes in an 88E6191X config image."""
    scratch = {}
    for m in SCRATCH_WRITE_RE.finditer(image):
        value = int.from_bytes(m.group("value"), "little")
        if value & SCRATCH_UPDATE:
            scratch[(value >> 8) & 0x7F] = value & 0xFF
    if mdio_lib.SCRATCH_BYTE0_INDEX not in scratch or mdio_lib.SCRATCH_BYTE1_INDEX not in scratch:
        raise ValueError("no version stamp found in 88E6191X config EEPROM image")
    return f"{scratch[mdio_lib.SCRATCH_BYTE0_INDEX]}.{scratch[mdio_lib.SCRATCH_BYTE1_INDEX]}"


def _has_88e6191x(platform: str | None) -> bool:
    """Legacy CF2 CPU card carries an 88E6341; Latest uses 88E6191X.

    A GA-scheme platform string (-r<n>) does not encode the CPU card and always
    denotes current hardware.
    """
    m = PLATFORM_REVISION_RE.search(platform or "")
    return m is None or int(m.group(2)) != CPU_CARD_88E6341


def _make_smi() -> mdio_lib.MarvellSmi:
    read32, write32 = mdio_lib.fpga_register_io(FPGA_DEVICE, FPGA_MDIO_MASTER_OFFSET)
    return mdio_lib.MarvellSmi(mdio_lib.FpgaMdioC22Master(read32, write32), MV88E6191X_BRIDGE_PHY)


def _read_88e6191x_version_mdio() -> str:
    """Return "<major>.<minor>" from the live 88E6191X scratch bytes.

    Raises SmiNoDeviceError or TimeoutError when the MDIO path is unavailable (when the
    BMC is the platform owner the bus is muxed to it and reads all ones) and RuntimeError
    when the device answering at the bridge address is not an 88E6191X.
    """
    with _hardware_lock():
        smi = _make_smi()
        switch_id = smi.read_switch_id(MV88E6191X_PORT0_DEVICE)
        if switch_id != MV88E6191X_SWITCH_ID:
            raise RuntimeError(
                f"unexpected switch id 0x{switch_id:04X} at MDIO 0x{MV88E6191X_BRIDGE_PHY:02X}, "
                f"expected 88E6191X 0x{MV88E6191X_SWITCH_ID:04X}"
            )
        return smi.read_config_version()


def _read_version(vlan: bool, mdio: bool) -> str | None:
    """Return the version string to report, or None if it cannot be read right now."""
    if not _has_88e6191x(device_info.get_platform(config_db=None)):
        version, vlan_version = _read_88e6341_version()
        return "0." + str(vlan_version if vlan else version)
    if not mdio:
        return _decode_88e6191x_version(_read_88e6191x_image())
    try:
        return _read_88e6191x_version_mdio()
    except (mdio_lib.SmiNoDeviceError, TimeoutError) as e:
        syslog.syslog(
            syslog.LOG_WARNING,
            f"MDIO unavailable ({e}); when the BMC is the platform owner only it can read the 88E6191X",
        )
        return None


def _describe_failure(e: Exception) -> str:
    """Map the exceptions the read paths can raise to a message that says which stage failed."""
    if isinstance(e, ImportError):
        return f"pddfparse not importable, PDDF platform plugins missing ({e})"
    if isinstance(e, ValueError):
        return f"config EEPROM image has no recognizable version stamp ({e})"
    if isinstance(e, FileNotFoundError):
        return f"EEPROM sysfs node missing, PDDF device creation failed ({e})"
    if isinstance(e, OSError):
        return f"FPGA or i2c access failed ({e})"
    if isinstance(e, RuntimeError):
        return f"platform config error ({e})"
    return f"unexpected {type(e).__name__} ({e})"


def _main() -> int:
    args = sys.argv[1:]
    vlan = "--vlan" in args
    cache_file = CACHE_FILE + ("_vlan" if vlan else "")
    if "--no-cache" not in args:
        cached = _read_cache(cache_file)
        if cached is not None:
            print(cached)
            return 0

    if os.geteuid() != 0:
        return subprocess.call(["sudo", sys.argv[0]] + sys.argv[1:])

    result = _read_version(vlan, mdio="--mdio" in args)
    if result is None:
        print(VERSION_UNAVAILABLE)
        return 0
    try:
        _write_cache(result, cache_file)
    except OSError as e:
        syslog.syslog(syslog.LOG_WARNING, f"could not write version cache {cache_file}: {e}")
    print(result)
    return 0


def main() -> int:
    """Never let an error reach stdout: callers such as fwutil parse the printed version."""
    try:
        return _main()
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, f"cannot read mgmt switch version: {_describe_failure(e)}")
        print(VERSION_UNAVAILABLE)
        return 0


if __name__ == "__main__":
    sys.exit(main())
