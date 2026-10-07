# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Clause 22 MDIO over the Nexthop FPGA MDIO master, plus the Marvell multi-chip
SMI indirection used by the LinkStreet management switches (88E6341,
88E6191X).

The FPGA block is two 32-bit registers: control at +0x0 and read-data at +0x4.
A transaction is armed by the rising edge of the read or write bit, so every
transaction is followed by a zero write to re-arm the master. There is no busy
bit, only read-data-valid.

Transport is injected as read32/write32 callables so the protocol can be unit
tested without hardware; `fpga_register_io` builds them from fpga_lib.
"""

import time
from typing import Callable

from nexthop import fpga_lib

# --- FPGA Clause 22 MDIO master block ---
MDIO_CTRL_OFFSET = 0x0
MDIO_READ_DATA_OFFSET = 0x4
MDIO_CTRL_WRITE = 1 << 31
MDIO_CTRL_READ = 1 << 30
MDIO_CTRL_PHY_ADDR_SHIFT = 24
MDIO_CTRL_REG_ADDR_SHIFT = 16
MDIO_READ_DATA_VALID = 1 << 31
MDIO_DATA_MASK = 0xFFFF
MDIO_ADDR_MAX = 0x1F

# --- Marvell multi-chip SMI bridge registers ---
SMI_COMMAND_REG = 0x00
SMI_DATA_REG = 0x01
SMI_BUSY = 1 << 15
SMI_CMD_HEADER = 0x9000  # SMIBusy | SMIMode=Clause22
SMI_OP_WRITE = 0x1
SMI_OP_READ = 0x2
SMI_OP_SHIFT = 10
SMI_DEVICE_SHIFT = 5

# --- Marvell switch internal registers (family-identical across 88E6341/88E6191X) ---
GLOBAL2_DEVICE = 0x1C
PORT_REG_SWITCH_ID = 0x03
SWITCH_ID_PRODUCT_MASK = 0xFFF0
G2_REG_SCRATCH_MISC = 0x1A
SCRATCH_POINTER_SHIFT = 8
SCRATCH_DATA_MASK = 0xFF
SCRATCH_BYTE0_INDEX = 0x00
SCRATCH_BYTE1_INDEX = 0x01

Read32 = Callable[[int], int]
Write32 = Callable[[int, int], None]


class MdioTimeoutError(TimeoutError):
    """FPGA MDIO master did not assert read-data-valid."""


class SmiTimeoutError(TimeoutError):
    """Marvell SMIBusy did not clear."""


class SmiNoDeviceError(RuntimeError):
    """Nothing answers at the bridge address (undriven bus reads all ones)."""


def fpga_register_io(fpga_name: str, base_offset: int, pddf_config=None) -> tuple[Read32, Write32]:
    """Return read32/write32 for a register block at `base_offset` on a named PDDF FPGA."""
    bdf = fpga_lib.name_to_bdf(fpga_name, pddf_config)
    if bdf is None:
        raise RuntimeError(f"could not resolve PCI BDF for FPGA {fpga_name}")

    def read32(offset: int) -> int:
        return fpga_lib.read_32(bdf, base_offset + offset)

    def write32(offset: int, value: int) -> None:
        fpga_lib.write_32(bdf, base_offset + offset, value)

    return read32, write32


def _check_5bit(name: str, value: int) -> None:
    if not 0 <= value <= MDIO_ADDR_MAX:
        raise ValueError(f"{name} 0x{value:X} out of range 0..0x{MDIO_ADDR_MAX:X}")


class FpgaMdioC22Master:
    def __init__(
        self,
        read32: Read32,
        write32: Write32,
        timeout_s: float = 0.1,
        poll_s: float = 0.0001,
        sleep=time.sleep,
        clock=time.monotonic,
    ):
        self._read32 = read32
        self._write32 = write32
        self._timeout_s = timeout_s
        self._poll_s = poll_s
        self._sleep = sleep
        self._clock = clock

    def _control_word(self, cmd: int, phy: int, reg: int, data: int = 0) -> int:
        _check_5bit("phy address", phy)
        _check_5bit("register", reg)
        return cmd | (phy << MDIO_CTRL_PHY_ADDR_SHIFT) | (reg << MDIO_CTRL_REG_ADDR_SHIFT) | (data & MDIO_DATA_MASK)

    def read(self, phy: int, reg: int) -> int:
        self._write32(MDIO_CTRL_OFFSET, self._control_word(MDIO_CTRL_READ, phy, reg))
        try:
            deadline = self._clock() + self._timeout_s
            while True:
                value = self._read32(MDIO_READ_DATA_OFFSET)
                if value & MDIO_READ_DATA_VALID:
                    return value & MDIO_DATA_MASK
                if self._clock() >= deadline:
                    raise MdioTimeoutError(
                        f"MDIO read phy=0x{phy:02X} reg=0x{reg:02X}: read-data-valid not set within {self._timeout_s}s"
                    )
                self._sleep(self._poll_s)
        finally:
            # A skipped re-arm leaves the master armed and every later read times out.
            self._write32(MDIO_CTRL_OFFSET, 0)

    def write(self, phy: int, reg: int, data: int) -> None:
        if not 0 <= data <= MDIO_DATA_MASK:
            raise ValueError(f"data 0x{data:X} exceeds 16 bits")
        self._write32(MDIO_CTRL_OFFSET, self._control_word(MDIO_CTRL_WRITE, phy, reg, data))
        self._write32(MDIO_CTRL_OFFSET, 0)


class MarvellSmi:
    """Multi-chip addressing mode: all internal devices reached via the bridge PHY's
    SMI Command/Data register pair."""

    def __init__(
        self,
        master: FpgaMdioC22Master,
        bridge_phy: int,
        timeout_s: float = 1.0,
        poll_s: float = 0.001,
        sleep=time.sleep,
        clock=time.monotonic,
    ):
        _check_5bit("bridge phy", bridge_phy)
        self._master = master
        self._bridge = bridge_phy
        self._timeout_s = timeout_s
        self._poll_s = poll_s
        self._sleep = sleep
        self._clock = clock

    @staticmethod
    def _command(op: int, device: int, reg: int) -> int:
        _check_5bit("device", device)
        _check_5bit("register", reg)
        return SMI_CMD_HEADER | (op << SMI_OP_SHIFT) | (device << SMI_DEVICE_SHIFT) | reg

    def _wait_ready(self, device: int, reg: int) -> None:
        deadline = self._clock() + self._timeout_s
        while True:
            command = self._master.read(self._bridge, SMI_COMMAND_REG)
            if command == MDIO_DATA_MASK:
                raise SmiNoDeviceError(f"no device at MDIO address 0x{self._bridge:02X}")
            if not command & SMI_BUSY:
                return
            if self._clock() >= deadline:
                raise SmiTimeoutError(
                    f"SMIBusy did not clear within {self._timeout_s}s (device=0x{device:02X} reg=0x{reg:02X})"
                )
            self._sleep(self._poll_s)

    def read(self, device: int, reg: int) -> int:
        self._wait_ready(device, reg)
        self._master.write(self._bridge, SMI_COMMAND_REG, self._command(SMI_OP_READ, device, reg))
        self._wait_ready(device, reg)
        return self._master.read(self._bridge, SMI_DATA_REG)

    def write(self, device: int, reg: int, value: int) -> None:
        self._wait_ready(device, reg)
        self._master.write(self._bridge, SMI_DATA_REG, value)
        self._master.write(self._bridge, SMI_COMMAND_REG, self._command(SMI_OP_WRITE, device, reg))
        self._wait_ready(device, reg)

    def read_switch_id(self, port0_device: int) -> int:
        """Product number from the per-port Switch Identifier register, revision nibble masked."""
        return self.read(port0_device, PORT_REG_SWITCH_ID) & SWITCH_ID_PRODUCT_MASK

    def read_scratch_byte(self, index: int) -> int:
        """Global2 Scratch & Misc: pointer-only write (Update=0), then read the data byte."""
        self.write(GLOBAL2_DEVICE, G2_REG_SCRATCH_MISC, index << SCRATCH_POINTER_SHIFT)
        return self.read(GLOBAL2_DEVICE, G2_REG_SCRATCH_MISC) & SCRATCH_DATA_MASK

    def read_config_version(self) -> str:
        """EEPROM config version stamped into the scratch bytes, as "<major>.<minor>"."""
        major = self.read_scratch_byte(SCRATCH_BYTE0_INDEX)
        minor = self.read_scratch_byte(SCRATCH_BYTE1_INDEX)
        return f"{major}.{minor}"
