# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Nexthop-specific counterpart to sonic_py_common.device_info."""

import glob
import os
import syslog

from nexthop_utils import eeprom_utils
from sonic_py_common import device_info

ONIE_DEVICE_VERSION_KEY = "onie_device_version"
ONIE_TLV_CODE_DEVICE_VERSION = 0x26
# Instantiated from ACPI by the kernel, so present before PDDF.
CPU_CARD_IDPROM_GLOB = "/sys/bus/i2c/drivers/at24/i2c-PRP0001:*/eeprom"
# On tmpfs, so the IDPROM is read from the device once per boot.
CPU_CARD_IDPROM_CACHE = "/var/run/platform_cache/cpucard_idprom"
# A Device Version that was never programmed is, by definition, version 0.
UNPROGRAMMED_DEVICE_VERSION = 0


def get_machine_conf_value(key: str) -> str | None:
    """Value of `key` in machine.conf, or None if the file or key is absent."""
    machine_info = device_info.get_machine_info()
    if machine_info:
        return machine_info.get(key)
    return None


def read_cpu_card_idprom() -> tuple[eeprom_utils.Eeprom, bytearray]:
    """The CPU card IDPROM's Eeprom and its contents.

    Reads the cache when it exists, which any user can do, and otherwise the device, which needs root.
    Raises an exception if the IDPROM cannot be read.
    """
    paths = glob.glob(CPU_CARD_IDPROM_GLOB)
    if not paths:
        raise FileNotFoundError(f"No CPU card IDPROM matches {CPU_CARD_IDPROM_GLOB}")
    if len(paths) > 1:
        raise RuntimeError(f"Expected one CPU card IDPROM matching {CPU_CARD_IDPROM_GLOB}, found {sorted(paths)}")
    path = paths[0]
    eeprom = eeprom_utils.Eeprom(path, start=0, status="", ro=True)
    if os.path.isfile(CPU_CARD_IDPROM_CACHE):
        # Not set_cache_name(): that opens the device for a lock, which only the cache write needs.
        eeprom.cache_name = CPU_CARD_IDPROM_CACHE
        try:
            return eeprom, _read_tlvinfo(eeprom, path)
        except Exception:
            if not eeprom.cache_update_needed:  # bad data came from the cache
                os.remove(CPU_CARD_IDPROM_CACHE)
            raise
    os.makedirs(os.path.dirname(CPU_CARD_IDPROM_CACHE), exist_ok=True)
    eeprom.set_cache_name(CPU_CARD_IDPROM_CACHE)
    try:
        data = _read_tlvinfo(eeprom, path)
        try:
            eeprom.update_cache(data)
        except OSError as e:
            syslog.syslog(syslog.LOG_WARNING, f"Failed to cache the CPU card IDPROM: {e}")
    finally:
        # set_cache_name() holds a lock that only update_cache() releases.
        if eeprom.lock_file is not None:
            eeprom.lock_file.close()
    return eeprom, data


def _read_tlvinfo(eeprom: eeprom_utils.Eeprom, path: str) -> bytearray:
    try:
        data = eeprom.read_eeprom()
    except (OSError, RuntimeError) as e:
        raise RuntimeError(f"{path}: {e}") from e
    if not eeprom.is_valid_tlvinfo_header(data):
        raise RuntimeError(f"{path}: no TlvInfo header")
    if not eeprom.is_checksum_valid(data)[0]:
        raise RuntimeError(f"{path}: TlvInfo CRC mismatch")
    return data


def get_cpu_card_idprom_field(code: int) -> bytearray:
    """Raw value of ONIE TLV `code` in the CPU card IDPROM.

    Raises an exception if the field cannot be read from the IDPROM.
    """
    eeprom, data = read_cpu_card_idprom()
    found, tlv = eeprom.get_tlv_field(data, code)
    if not found or tlv is None:
        raise RuntimeError(f"TLV 0x{code:02x} not found in the CPU card IDPROM")
    return tlv[2]


def get_device_version(skip_machine_conf: bool = False) -> int:
    """The CPU card IDPROM Device Version (ONIE TLV 0x26).

    Args:
        skip_machine_conf: if True, don't read machine.conf and only read the
            IDPROM directly.

    Raises an exception if the Device Version cannot be read from the IDPROM.
    """
    if not skip_machine_conf:
        value = get_machine_conf_value(ONIE_DEVICE_VERSION_KEY)
        if value is not None and value.isdigit():
            return int(value)
        syslog.syslog(
            syslog.LOG_INFO,
            f"No numeric {ONIE_DEVICE_VERSION_KEY} in {device_info.MACHINE_CONF_PATH} (got {value!r}), "
            "ONIE may not support it yet; reading the CPU card IDPROM",
        )

    value = get_cpu_card_idprom_field(ONIE_TLV_CODE_DEVICE_VERSION)
    if len(value) != 1:
        raise ValueError(f"Device Version TLV 0x{ONIE_TLV_CODE_DEVICE_VERSION:02x} has length {len(value)}, expected 1")
    return value[0]
