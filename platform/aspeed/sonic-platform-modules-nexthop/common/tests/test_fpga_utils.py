"""Unit tests for the spidev-backed BmcFpgaCtrl framing in fpga_utils.py.

These run on any host (no FPGA hardware required): the spidev ioctl and the
sysfs device discovery are stubbed, so only the pure framing/parsing logic and
the spi_ioc_transfer marshalling are exercised.
"""

import ctypes
import os
import sys

import pytest

# fpga_utils.py lives in ../scripts and is a plain script, not a package.
_SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), os.pardir, "scripts")
sys.path.insert(0, os.path.abspath(_SCRIPTS_DIR))

import fpga_utils  # noqa: E402


def _bare_ctrl():
    """A BmcFpgaCtrl instance without running the hardware-touching __new__."""
    return object.__new__(fpga_utils.BmcFpgaCtrl)


def test_spi_ioc_transfer_is_32_bytes():
    # spidev's struct spi_ioc_transfer is 32 bytes on LP64.
    assert ctypes.sizeof(fpga_utils._SpiIocTransfer) == 32


def test_spi_ioc_message_number():
    # _IOW('k', 0, sizeof(spi_ioc_transfer)) == 0x40206B00.
    assert fpga_utils._spi_ioc_message(1) == 0x40206B00


def test_read_frame_and_parse():
    ctrl = _bare_ctrl()
    captured = {}

    def fake_transfer(tx):
        captured["tx"] = tx
        # FPGA returns the 32-bit value big-endian in the data slot (offset 9).
        rx = bytearray(len(tx))
        rx[9:13] = (0xDEADBEEF).to_bytes(4, "big")
        return bytes(rx)

    ctrl._transfer = fake_transfer

    value = ctrl.read_reg(0x50600)

    assert captured["tx"] == bytes([0x02, 0x00, 0x05, 0x06, 0x00]) + bytes(8)
    assert len(captured["tx"]) == 13
    assert value == 0xDEADBEEF


def test_read_frame_default_ta_len():
    # A bare instance inherits the class-default TA length (4 bytes).
    assert _bare_ctrl().ta_len == fpga_utils._FPGA_TA_LEN == 4


def test_read_frame_overridden_ta_len():
    ctrl = _bare_ctrl()
    ctrl.ta_len = 8  # override (e.g. from --ta-len)
    captured = {}

    def fake_transfer(tx):
        captured["tx"] = tx
        # Data now sits after CMD(1)+ADDR(4)+TA(8) = offset 13.
        rx = bytearray(len(tx))
        rx[13:17] = (0xDEADBEEF).to_bytes(4, "big")
        return bytes(rx)

    ctrl._transfer = fake_transfer

    value = ctrl.read_reg(0x50600)

    assert captured["tx"] == bytes([0x02, 0x00, 0x05, 0x06, 0x00]) + bytes(12)
    assert len(captured["tx"]) == 17  # 1 + 4 + 8 + 4
    assert value == 0xDEADBEEF


def test_write_frame():
    ctrl = _bare_ctrl()
    captured = {}

    def fake_transfer(tx):
        captured["tx"] = tx
        return bytes(len(tx))

    ctrl._transfer = fake_transfer

    ctrl.write_reg(0x000C, 0x12345678)

    assert captured["tx"] == bytes(
        [0x01, 0x00, 0x00, 0x00, 0x0C, 0x12, 0x34, 0x56, 0x78]
    )
    assert len(captured["tx"]) == 9


def test_transfer_ioctl_roundtrip(monkeypatch):
    """Exercise the real _transfer: verify the spi_ioc_transfer is marshalled
    correctly and that full-duplex RX data is read back from the buffer."""
    ctrl = _bare_ctrl()
    ctrl._fd = 123  # arbitrary; ioctl is stubbed below

    seen = {}

    def fake_ioctl(fd, request, arg):
        seen["fd"] = fd
        seen["request"] = request
        seen["tx"] = ctypes.string_at(arg.tx_buf, arg.len)
        seen["len"] = arg.len
        seen["bits"] = arg.bits_per_word
        # Emulate the FPGA: place the data big-endian at offset 9.
        resp = bytearray(arg.len)
        resp[9:13] = (0xCAFEBABE).to_bytes(4, "big")
        ctypes.memmove(arg.rx_buf, bytes(resp), arg.len)
        return 0

    monkeypatch.setattr(fpga_utils.fcntl, "ioctl", fake_ioctl)

    value = ctrl.read_reg(0x12345678)

    assert seen["fd"] == 123
    assert seen["request"] == 0x40206B00
    assert seen["len"] == 13
    assert seen["bits"] == 8
    assert seen["tx"] == bytes([0x02, 0x12, 0x34, 0x56, 0x78]) + bytes(8)
    assert value == 0xCAFEBABE


def test_find_spidev_maps_sysfs_name(monkeypatch):
    ctrl = _bare_ctrl()
    monkeypatch.setattr(
        fpga_utils.glob,
        "glob",
        lambda p: ["/sys/bus/platform/devices/14010000.spi/spi_master/spi3/spi3.0"]
        if "spi_master" in p
        else [],
    )
    assert ctrl._find_spidev() == "/dev/spidev3.0"


def test_find_spidev_raises_when_absent(monkeypatch):
    ctrl = _bare_ctrl()
    monkeypatch.setattr(fpga_utils.glob, "glob", lambda p: [])
    with pytest.raises(FileNotFoundError):
        ctrl._find_spidev()
