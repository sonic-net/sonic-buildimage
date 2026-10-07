#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Read the BCM53134 mgmt-switch config version from the Microwire EEPROM (93LC86C)
via the PDDF-instantiated spidev node.  READ-ONLY: only issues the Microwire READ opcode.

Intended to be called by read-mgmt-switch-version.service (as root, after pddf-platform-init)
which populates CACHE_FILE.  Subsequent non-root calls (e.g. from show platform firmware
status) read the cache directly without needing elevated privileges.
"""

import ctypes
import fcntl
import os
import struct
import sys
import syslog

from nexthop.pddf_loader import load_pddf_device_config
from nexthop.spi_lib import create_spi_subtree, delete_spi_subtree, get_spidev_path

VERSION_WORD = 0x7F
SPI_DEVICE_NAME = "MGMT-SWITCH-EEPROM"
CACHE_FILE = "/tmp/mgmt_switch_version"
VERSION_UNKNOWN = "unknown"      # EEPROM erased (word 0x7F = 0xFFFF)
VERSION_UNAVAILABLE = "N/A"      # service has not populated cache yet

SPI_IOC_MAGIC = ord("k")
_IOC_WRITE = 1 << 30
_SPI_IOC_TRANSFER_SIZE = 32  # sizeof(struct spi_ioc_transfer)


def _IOW(nr: int, size: int) -> int:
    """Encode a Linux _IOW ioctl request code (write direction, spidev magic)."""
    return _IOC_WRITE | (size << 16) | (SPI_IOC_MAGIC << 8) | nr


WR_MODE = _IOW(1, 1)
WR_BPW = _IOW(3, 1)
WR_SPEED = _IOW(4, 4)


def _MSG(n: int) -> int:
    """Return the SPI_IOC_MESSAGE ioctl code for n simultaneous transfers."""
    return _IOW(0, n * _SPI_IOC_TRANSFER_SIZE)


def _spi_xfer(fd: int, tx: list[int]) -> bytes:
    """Issue a full-duplex SPI transfer via SPI_IOC_MESSAGE ioctl; return rx bytes."""
    txb = bytes(tx)
    rxb = bytearray(len(txb))
    ta = (ctypes.c_char * len(txb)).from_buffer_copy(txb)
    ra = (ctypes.c_char * len(rxb)).from_buffer(rxb)
    # spi_ioc_transfer: tx_buf rx_buf len speed_hz delay bits_per_word cs_change tx_nbits rx_nbits word_delay pad
    tr = struct.pack(
        "QQIIHBBBBBB",
        ctypes.addressof(ta),  # tx_buf
        ctypes.addressof(ra),  # rx_buf
        len(txb),              # len
        1_000_000,             # speed_hz
        0,                     # delay_usecs
        8,                     # bits_per_word
        0, 0, 0, 0, 0,         # cs_change, tx_nbits, rx_nbits, word_delay_usecs, pad
    )
    fcntl.ioctl(fd, _MSG(1), tr)
    return bytes(ra)


def _read_word(fd: int, addr: int) -> int:
    """Issue a Microwire READ for the given 10-bit address; return the 16-bit data word."""
    # Microwire READ frame for 93LC86C (x16, 10-bit address):
    #   bits [7:5] = 0b111  dummy — FPGA masks these 3 clock pulses
    #   bit  [4]   = 1      start bit
    #   bits [3:2] = 0b10   READ opcode
    #   bits [1:0] = A9:A8  upper address bits
    # Followed by A7:A0, then 12 zero bytes to clock out the response.
    tx = [0b111_1_10_00 | ((addr >> 8) & 3), addr & 0xFF] + [0] * 12
    rx = _spi_xfer(fd, tx)
    # rx[0] is during the command byte (discard); rx[1:] contains:
    #   bits [0:8]  = A7:A0 addr echo
    #   bit  [8]    = dummy 0
    #   bits [9:25] = 16-bit data word MSB-first
    #   bits [26:]  = discard
    # MSB-first: bit[0] = MSB of rx[1]
    n = int.from_bytes(rx[1:], "big")
    return (n >> (len(rx[1:]) * 8 - 25)) & 0xFFFF


def _read_cache() -> str | None:
    """Return the cached version string from CACHE_FILE, or None if absent/empty."""
    try:
        with open(CACHE_FILE) as f:
            v = f.read().strip()
            if v:
                return v
    except OSError:
        pass
    return None


def _write_cache(version: str) -> None:
    """Persist version string to CACHE_FILE; world-readable so non-root show commands can use it."""
    with open(CACHE_FILE, "w") as f:
        f.write(version)
    os.chmod(CACHE_FILE, 0o644)


def _read_version(dev: str) -> int:
    """Open spidev at dev, configure SPI parameters, and return the raw VERSION_WORD value."""
    fd = os.open(dev, os.O_RDWR)
    try:
        fcntl.ioctl(fd, WR_MODE, b"\x00")  # SPI mode 0
        fcntl.ioctl(fd, WR_BPW, b"\x08")   # 8 bits/word
        fcntl.ioctl(fd, WR_SPEED, struct.pack("I", 1_000_000))
        return _read_word(fd, VERSION_WORD)
    finally:
        os.close(fd)


def main() -> int:
    cached = _read_cache()
    if cached is not None:
        print(cached)
        return 0

    if os.geteuid() != 0:
        syslog.syslog(
            syslog.LOG_WARNING,
            "read_mgmt_switch_version: cache not populated and running as non-root; "
            "mgmt-switch-version.service may not have run yet",
        )
        print(VERSION_UNAVAILABLE)
        return 0

    syslog.syslog(
        syslog.LOG_INFO, "read_mgmt_switch_version: reading BCM53134 EEPROM via SPI"
    )
    pddf_config = load_pddf_device_config()
    try:
        spidev_path = get_spidev_path(SPI_DEVICE_NAME, pddf_config)
    except RuntimeError:
        syslog.syslog(
            syslog.LOG_INFO,
            f"read_mgmt_switch_version: {SPI_DEVICE_NAME} not in PDDF config, skipping",
        )
        return 0
    create_spi_subtree(SPI_DEVICE_NAME, pddf_config, wait_for_path=spidev_path)
    try:
        version = _read_version(spidev_path)
    finally:
        delete_spi_subtree(SPI_DEVICE_NAME, pddf_config)

    result = VERSION_UNKNOWN if version == 0xFFFF else str(version)
    syslog.syslog(syslog.LOG_INFO, f"read_mgmt_switch_version: version={result}")
    _write_cache(result)
    print(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
