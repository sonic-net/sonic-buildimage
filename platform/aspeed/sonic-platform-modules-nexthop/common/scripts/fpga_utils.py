#!/usr/bin/python3

import argparse
import ctypes
import fcntl
import glob
import logging
import os
import time
from dataclasses import dataclass, field
from enum import Enum
from typing import Literal

logging.TRACE = 5


class CustomLogger(logging.Logger):
    def trace(self, msg, *args, **kwargs):
        if self.isEnabledFor(logging.TRACE):
            self._log(logging.TRACE, msg, args, **kwargs)


logging.setLoggerClass(CustomLogger)
logging.addLevelName(logging.TRACE, "TRACE")
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger(__name__)

# FPGA SPI bridge framing (big-endian on the wire). Reads clock out the
# command + address, a fixed turnaround (TA) gap, then read back the data:
#   Read:  TX [0x02][ADDR 4B][TA 4B][-- 4B --]  RX [-- 9B --][DATA 4B]
#   Write: TX [0x01][ADDR 4B][DATA 4B]
_FPGA_CMD_READ = 0x02
_FPGA_CMD_WRITE = 0x01
_FPGA_ADDR_LEN = 4
_FPGA_DATA_LEN = 4
_FPGA_TA_LEN = 4

# spidev ioctl plumbing (linux/spi/spidev.h).
_SPI_IOC_MAGIC = ord("k")


class _SpiIocTransfer(ctypes.Structure):
    _fields_ = [
        ("tx_buf", ctypes.c_uint64),
        ("rx_buf", ctypes.c_uint64),
        ("len", ctypes.c_uint32),
        ("speed_hz", ctypes.c_uint32),
        ("delay_usecs", ctypes.c_uint16),
        ("bits_per_word", ctypes.c_uint8),
        ("cs_change", ctypes.c_uint8),
        ("tx_nbits", ctypes.c_uint8),
        ("rx_nbits", ctypes.c_uint8),
        ("word_delay_usecs", ctypes.c_uint8),
        ("pad", ctypes.c_uint8),
    ]


def _spi_ioc_message(n: int) -> int:
    # _IOW(SPI_IOC_MAGIC, 0, char[sizeof(struct) * n])
    size = ctypes.sizeof(_SpiIocTransfer) * n
    return (1 << 30) | (size << 16) | (_SPI_IOC_MAGIC << 8) | 0


class BmcFpgaCtrl:
    _instance = None
    _SPI_CONTROLLER = "14010000.spi"

    # Read turnaround length (bytes). Class default; overridable per-instance
    # via __new__ (e.g. the --ta-len CLI flag).
    ta_len = _FPGA_TA_LEN

    def __new__(cls, ta_len: int = _FPGA_TA_LEN):
        if cls._instance is None:
            instance = super().__new__(cls)
            instance._dev_path = instance._find_spidev()
            instance._fd = os.open(instance._dev_path, os.O_RDWR)
            instance.ta_len = ta_len
            cls._instance = instance
        return cls._instance

    def _find_spidev(self) -> str:
        # The FPGA peripheral is chip-select 0 on the 14010000.spi controller.
        patterns = [
            f"/sys/bus/platform/devices/{self._SPI_CONTROLLER}/spi_master/spi*/spi*.0",
            f"/sys/bus/platform/devices/{self._SPI_CONTROLLER}/spi_controller/spi*/spi*.0",
        ]
        matches = sorted(m for p in patterns for m in glob.glob(p))
        if not matches:
            raise FileNotFoundError(
                f"FPGA spidev not found under {self._SPI_CONTROLLER}; "
                "run start-fpga-driver first"
            )
        name = os.path.basename(matches[0])  # e.g. "spi0.0"
        return "/dev/spidev" + name[len("spi") :]

    def __del__(self):
        try:
            os.close(self._fd)
        except (OSError, AttributeError):
            pass

    def _transfer(self, tx: bytes) -> bytes:
        length = len(tx)
        tx_buf = ctypes.create_string_buffer(tx, length)
        rx_buf = ctypes.create_string_buffer(length)
        xfer = _SpiIocTransfer(
            tx_buf=ctypes.addressof(tx_buf),
            rx_buf=ctypes.addressof(rx_buf),
            len=length,
            bits_per_word=8,
        )
        fcntl.ioctl(self._fd, _spi_ioc_message(1), xfer)
        return rx_buf.raw

    def read_reg(self, reg) -> int:
        tx = (
            bytes([_FPGA_CMD_READ])
            + reg.to_bytes(_FPGA_ADDR_LEN, "big")
            + bytes(self.ta_len)
            + bytes(_FPGA_DATA_LEN)
        )
        rx = self._transfer(tx)
        data_off = 1 + _FPGA_ADDR_LEN + self.ta_len
        value = int.from_bytes(rx[data_off : data_off + _FPGA_DATA_LEN], "big")
        logger.trace("READ " + hex(reg) + " : " + hex(value))
        return value

    def write_reg(self, reg, value) -> None:
        tx = (
            bytes([_FPGA_CMD_WRITE])
            + reg.to_bytes(_FPGA_ADDR_LEN, "big")
            + value.to_bytes(_FPGA_DATA_LEN, "big")
        )
        self._transfer(tx)
        logger.trace("WRITE " + hex(reg) + " : " + hex(value))


class BmcFpgaReg:
    def __init__(self, addr, twice_read=False):
        self._addr = addr
        self._ctrl = BmcFpgaCtrl()
        self._twice_read = twice_read

    def read(self) -> int:
        if self._twice_read:
            self._ctrl.read_reg(self._addr)
        return self._ctrl.read_reg(self._addr)

    def write(self, value: int) -> None:
        self._ctrl.write_reg(self._addr, value)

    def _write_mask(self, value: int, mask: int):
        new_value = (self.read() & ~mask) | (value & mask)
        self.write(new_value)

    def write_bits(self, bits: dict[int, int]):
        mask = 0
        value = 0
        for bit, bit_val in bits.items():
            mask |= 1 << bit
            if bit_val:
                value |= 1 << bit
        self._write_mask(value, mask)


# ============================================================================
# Codeurposed from Diags Shell
#
# SPI OpCode / Device Configuration
# ============================================================================


class FourByteAddrMode(Enum):
    NONE = "none"  # 3-byte addressing, no action needed
    EN4B = "en4b"  # Send EN4B (0xB7) during init, use standard opcodes
    WREN_EN4B = "wren_en4b"  # Send WREN then EN4B (Micron/ST)
    NATIVE_4B = "native_4b"  # Use SpiOpCodes4B, no mode switch


@dataclass
class SpiOpCodes:
    read: int = 0x03  # READ data from memory
    write: int = 0x02  # WRITE / PAGE_PROGRAM
    write_enable: int = 0x06  # WREN - enable write latch
    write_disable: int = 0x04  # WRDI - disable write latch
    read_status: int = 0x05  # RDSR - read status register
    read_id: int = 0x9F  # RDID - read JEDEC device ID
    sector_erase: int | None = 0x20  # set automatically by SpiDeviceConfig.__post_init__
    chip_erase: int | None = 0xC7  # e.g., 0xC7 for full chip erase
    enter_4byte_mode: int | None = 0xB7  # EN4B
    exit_4byte_mode: int | None = 0xE9  # EX4B


@dataclass
class SpiOpCodes4B(SpiOpCodes):
    read: int = 0x13  # READ 4B
    write: int = 0x12  # PAGE_PROGRAM 4B
    sector_erase: int | None = 0xDC  # set automatically by SpiDeviceConfig.__post_init__


# Maps sector size in bytes → standard NOR flash sector erase opcode (3-byte addr).
_SECTOR_ERASE_OPCODES: dict[int, int] = {
    4 * 1024: 0x20,  # Sub-sector erase (SE)
    32 * 1024: 0x52,  # Half-block erase (HBE)
    64 * 1024: 0xD8,  # Block erase (BE)
}

# Maps sector size in bytes → 4-byte address erase opcode.
_SECTOR_ERASE_OPCODES_4B: dict[int, int] = {
    4 * 1024: 0x21,  # Sub-sector erase 4B (SE_4B)
    32 * 1024: 0x5C,  # Half-block erase 4B (HBE_4B)
    64 * 1024: 0xDC,  # Block erase 4B (BE_4B)
    128 * 1024: 0xDC,  # Same opcode, larger erase unit
}


@dataclass
class SpiDeviceConfig:
    device_name: str = "Unknown"
    full_size: int | None = None
    sector_size: int | None = None
    page_size: int = 256
    address_width: int = 3
    word_size: int = 1
    chip_select: int = 0
    opcodes: SpiOpCodes = field(default_factory=SpiOpCodes)
    four_byte_addr_mode: FourByteAddrMode = FourByteAddrMode.NONE
    clock_phase_read: bool = False
    clock_phase_write: bool = False
    clock_polarity: bool = False
    needs_write_enable: bool = True
    id_length: int = 9

    def __post_init__(self) -> None:
        """Auto-derive 4-byte addressing mode and sector_erase opcode."""
        # Auto-derive four_byte_addr_mode when address_width > 3 and mode
        # was not explicitly set by the vendor entry.
        if self.address_width > 3 and self.four_byte_addr_mode is FourByteAddrMode.NONE:
            if isinstance(self.opcodes, SpiOpCodes4B):
                self.four_byte_addr_mode = FourByteAddrMode.NATIVE_4B
            else:
                # Default to EN4B — safe for Macronix, Winbond, and most parts.
                # Micron/ST entries override to WREN_EN4B explicitly.
                self.four_byte_addr_mode = FourByteAddrMode.EN4B

        # Promote to native 4B opcodes when mode is NATIVE_4B and opcodes
        # are still the default 3-byte set.
        if self.four_byte_addr_mode is FourByteAddrMode.NATIVE_4B and type(self.opcodes) is SpiOpCodes:
            self.opcodes = SpiOpCodes4B()

        # Derive sector_erase opcode from sector_size.
        if self.sector_size is None:
            self.opcodes.sector_erase = None
        else:
            lookup = _SECTOR_ERASE_OPCODES_4B if isinstance(self.opcodes, SpiOpCodes4B) else _SECTOR_ERASE_OPCODES
            opcode = lookup.get(self.sector_size)
            if opcode is not None:
                self.opcodes.sector_erase = opcode


class SpiFlashHam:
    # SPI Master IP constants
    _SOFT_RESET_KEY: int = 0x0000_000A
    _CS_ALL_DEASSERTED: int = 0xFFFF_FFFF
    _DEFAULT_FIFO_DEPTH: int = 256  # Xilinx AXI SPI FIFO depth (bytes)

    # Polling constants
    _DEFAULT_POLL_TIMEOUT_SECONDS: float = 1.0
    _POLL_INTERVAL_SECONDS: float = 0.0001  # 100µs between polls
    _WRITE_COMPLETE_TIMEOUT_SECONDS: float = 5.0

    def __init__(self, offset: int, device_config: SpiDeviceConfig):
        self.cli_name = "SPI Flash Ham"
        self.fifo_depth = 16

        def reg(rel, twice=False):
            return BmcFpgaReg(offset + rel, twice)

        self.device_config = device_config

        self.software_reset = reg(0x040, True)

        self.control = reg(0x060, True) 
        self.status = reg(0x064, True)

        self.transmit_fifo = reg(0x068)
        self.receive_fifo = reg(0x06C)

        self.slave_select = reg(0x070, True)

        self.tx_fifo_occupancy = reg(0x074, True)
        self.rx_fifo_occupancy = reg(0x078, True)

    def __del__(self):
        # Ensure we leave the hardware in a clean state (CS deasserted, master idle).
        self.deselect_slave()
        self.control.write(0)  # Clear control register to disable master and reset config
        self._init_direction = None

    # ========================================================================
    # Low-Level SPI Primitives
    # ========================================================================

    def reset(self) -> None:
        logger.debug(f"{self.cli_name}: SPI master soft reset (reset_key=0x{self._SOFT_RESET_KEY:08X})")
        self.software_reset.write(self._SOFT_RESET_KEY)
        self._init_direction = None

    # Tracks the last direction init_master was called with, so we can skip
    # redundant re-initialisation on consecutive same-direction operations.
    # None means the master has not been initialised yet (or was explicitly reset).
    _init_direction: bool | None = None

    def init_master(self, read_not_write: bool = True, force: bool = False) -> None:
        if not force and self._init_direction is not None:
            # Skip if same direction, or if clock phases are identical (direction
            # doesn't matter).
            if (
                self._init_direction == read_not_write
                or self.device_config.clock_phase_read == self.device_config.clock_phase_write
            ):
                return

        # Step 1: Soft reset
        self.reset()

        # Step 2: Configure with FIFO reset asserted
        # Set: inhibit, manual CS, FIFO resets, master mode, enable
        # All other bits default to 0 (loopback, CPOL, CPHA, LSB-first)
        logger.debug(
            f"{self.cli_name}: Configuring SPI control register (inhibit + manual CS + FIFO reset + master + enable)"
        )
        self.control.write(0x000_01E6)
        logger.debug(f"{self.cli_name}: SPI control register = 0x{self.control.read():08X}")

        # Step 3: Deassert FIFO resets
        logger.debug(f"{self.cli_name}: Deasserting FIFO resets")
        self.control.write_bits({5: 0, 6: 0})
        logger.debug(f"{self.cli_name}: SPI control register = 0x{self.control.read():08X}")

        # Step 4: Verify hardware is responding
        status_value = self.status.read()
        slave_select_value = self.slave_select.read()
        control_value = self.control.read()
        tx_occupancy = self.tx_fifo_occupancy.read()
        rx_occupancy = self.rx_fifo_occupancy.read()
        logger.debug(
            f"{self.cli_name}: Post-init verification: "
            f"status=0x{status_value:08X}, slave_select=0x{slave_select_value:08X}, "
            f"control=0x{control_value:08X}, "
            f"tx_occupancy={tx_occupancy}, rx_occupancy={rx_occupancy}"
        )

        self._init_direction = read_not_write

    def select_slave(self) -> None:
        cs_mask = self._CS_ALL_DEASSERTED & ~(1 << self.device_config.chip_select)
        logger.debug(
            f"{self.cli_name}: select slave cs={self.device_config.chip_select} (slave_select=0x{cs_mask:08X})"
        )
        self.slave_select.write(cs_mask)

    def deselect_slave(self) -> None:
        self.slave_select.write(self._CS_ALL_DEASSERTED)

    def poll_tx_empty(self, timeout_seconds: float | None = None) -> None:
        timeout_seconds = timeout_seconds or self._DEFAULT_POLL_TIMEOUT_SECONDS
        deadline = time.monotonic() + timeout_seconds
        poll_count = 0
        while True:
            # if self.status.tx_fifo_empty.raw != 0:
            if self.status.read() & 0x0000_0004 != 0:
                break

            poll_count += 1
            if poll_count <= 5 or poll_count % 100 == 0:
                status = self.status.read()
                control = self.control.read()
                tx_occupancy = self.tx_fifo_occupancy.read()
                logger.debug(
                    f"{self.cli_name}: poll_tx_empty iteration {poll_count}: "
                    f"status=0x{status:08X}, control=0x{control:08X}, "
                    f"tx_occupancy={tx_occupancy}"
                )
            if time.monotonic() > deadline:
                final_status = self.status.read()
                final_control = self.control.read()
                final_tx_occupancy = self.tx_fifo_occupancy.read()
                raise Exception(
                    f"{self.cli_name}: TX FIFO not empty after {timeout_seconds}s. "
                    f"Final: status=0x{final_status:08X}, control=0x{final_control:08X}, "
                    f"tx_occupancy={final_tx_occupancy}"
                )
            time.sleep(self._POLL_INTERVAL_SECONDS)

    def transfer(self, tx_data: bytes) -> bytes:
        logger.debug(
            f"{self.cli_name}: SPI transfer cs={self.device_config.chip_select} "
            f"tx_length={len(tx_data)} "
            f"tx_data=0x{tx_data[:16].hex()}{'...' if len(tx_data) > 16 else ''}"
        )

        fifo_depth = self.fifo_depth
        rx_data = bytearray()
        offset = 0

        # Assert CS — stays asserted for the entire transfer.
        self.select_slave()

        while offset < len(tx_data):
            chunk_size = min(fifo_depth, len(tx_data) - offset)

            # Ensure inhibit is set before filling the FIFO.
            # self.control.rmw(master_transaction_inhibit=1)
            self.control.write_bits({8: 1})

            # Fill TX FIFO with this chunk.
            for i in range(chunk_size):
                # self.transmit_fifo.mw(tx_data=tx_data[offset + i])
                self.transmit_fifo.write(tx_data[offset + i])

            # Start clocking (clear inhibit).
            # self.control.write_bits(master_transaction_inhibit=0)
            self.control.write_bits({8: 0})

            # Wait for the chunk to clock out.
            self.poll_tx_empty()

            # Pause clock before draining RX (CS stays asserted).
            # self.control.write_bits(master_transaction_inhibit=1)
            self.control.write_bits({8: 1})

            # Drain RX FIFO — one byte clocked in for each byte clocked out.
            self.receive_fifo.read() # Unsure why this is needed
            for _ in range(chunk_size):
                rx_data.append(self.receive_fifo.read() & 0xFF)

            offset += chunk_size

        # Deassert CS.
        self.deselect_slave()

        logger.debug(f"{self.cli_name}: SPI transfer complete rx_length={len(rx_data)}")
        return bytes(rx_data)

    # ========================================================================
    # SPI Opcode Helper Methods
    # ========================================================================

    def read_identification(self) -> bytes:
        self.init_master(read_not_write=True)

        opcode = self.device_config.opcodes.read_id
        num_id_bytes = self.device_config.id_length
        tx = bytes([opcode]) + bytes(num_id_bytes)

        rx = self.transfer(tx)
        device_id = rx[1:]  # Skip opcode echo
        logger.debug(f"{self.cli_name}: RDID cs={self.device_config.chip_select} id=0x{device_id.hex()}")
        return device_id

    def send_write_enable(self) -> None:
        logger.debug(
            f"{self.cli_name}: WREN cs={self.device_config.chip_select} "
            f"opcode=0x{self.device_config.opcodes.write_enable:02X}"
        )
        self.transfer(bytes([self.device_config.opcodes.write_enable]))

    def send_write_disable(self) -> None:
        logger.debug(
            f"{self.cli_name}: WRDI cs={self.device_config.chip_select} "
            f"opcode=0x{self.device_config.opcodes.write_disable:02X}"
        )
        self.transfer(bytes([self.device_config.opcodes.write_disable]))

    def read_status_register(self) -> int:
        self.init_master(read_not_write=True)

        opcode = self.device_config.opcodes.read_status
        tx = bytes([opcode, 0x00])

        rx = self.transfer(tx)
        status = rx[1]
        logger.debug(f"{self.cli_name}: RDSR cs={self.device_config.chip_select} status=0x{status:02X}")
        return status

    def enter_4byte_mode(self, wren_first: bool = False) -> None:
        opcode = self.device_config.opcodes.enter_4byte_mode
        if opcode is None:
            raise Exception(f"{self.cli_name}: device does not support EN4B")
        self.init_master(read_not_write=False)
        if wren_first:
            self.send_write_enable()
        self.transfer(bytes([opcode]))
        logger.debug(f"{self.cli_name}: EN4B cs={self.device_config.chip_select} (wren_first={wren_first})")

    def wait_write_complete(self, timeout_seconds: float | None = None) -> None:
        timeout_seconds = timeout_seconds or self._WRITE_COMPLETE_TIMEOUT_SECONDS
        deadline = time.monotonic() + timeout_seconds
        while True:
            status = self.read_status_register()
            if (status & 0x01) == 0:  # WIP bit clear
                return
            if time.monotonic() > deadline:
                raise Exception(
                    f"{self.cli_name}: Write still in progress after "
                    f"{timeout_seconds}s (status=0x{status:02X}, "
                    f"cs={self.device_config.chip_select})"
                )
            time.sleep(self._POLL_INTERVAL_SECONDS)

    # ========================================================================
    # Erase Operations
    # ========================================================================

    def bulk_erase(self) -> None:
        if self.device_config.opcodes.chip_erase is None:
            raise Exception(f"{self.cli_name}: Bulk erase not supported on this device")

        self.init_master(read_not_write=False)

        if self.device_config.needs_write_enable:
            self.send_write_enable()

        opcode = self.device_config.opcodes.chip_erase
        logger.debug(f"{self.cli_name}: CHIP_ERASE cs={self.device_config.chip_select} opcode=0x{opcode:02X}")
        self.transfer(bytes([opcode]))
        self.wait_write_complete(timeout_seconds=300.0)

    # ========================================================================
    # SPI Read/Write Transactions (address-based)
    # ========================================================================

    def _build_address_bytes(self, address: int) -> bytes:
        return address.to_bytes(self.device_config.address_width, byteorder="big")

    def _spi_read_bytes(self, address: int, length: int) -> bytes:
        self.init_master(read_not_write=True)

        opcode = self.device_config.opcodes.read
        addr_bytes = self._build_address_bytes(address)
        header = bytes([opcode]) + addr_bytes
        tx = header + bytes(length)  # Dummy bytes to clock in data
        rx = self.transfer(tx)

        return rx[len(header) :]  # Strip opcode + address echo

    def _spi_write_bytes(self, address: int, data: bytes) -> None:
        self.init_master(read_not_write=False)

        offset = 0
        while offset < len(data):
            # Calculate page-aligned chunk size
            page_size_int = self.device_config.page_size
            page_offset = (address + offset) % page_size_int
            chunk_size = min(
                page_size_int - page_offset,
                len(data) - offset,
            )
            chunk = data[offset : offset + chunk_size]
            chunk_addr = address + offset

            # Issue write enable if required
            if self.device_config.needs_write_enable:
                self.send_write_enable()

            # Build and send write transaction
            opcode = self.device_config.opcodes.write
            addr_bytes = self._build_address_bytes(chunk_addr)
            tx = bytes([opcode]) + addr_bytes + chunk
            logger.debug(
                f"{self.cli_name}: SPI write cs={self.device_config.chip_select} "
                f"address=0x{chunk_addr:04X} length={len(chunk)}"
            )
            self.transfer(tx)

            # Wait for write to complete
            self.wait_write_complete()

            offset += chunk_size

    # ========================================================================
    # BaseHam Interface Implementation
    # ========================================================================

    def read_block(self, address: int, delay: float = 0) -> bytes:
        if delay:
            time.sleep(delay)
        return self._spi_read_bytes(address, self.device_config.page_size)

    def write_block(
        self,
        address: int,
        data: bytes,
        byte_order: Literal["big", "little"] = "little",
        delay: float = 0,
    ) -> None:
        # byte_order is unused: SPI sends data as-is (present for BaseHam API compatibility)
        _ = byte_order
        if delay:
            time.sleep(delay)
        self._spi_write_bytes(address, data)


class BmbaFpgaSpiFlash:
    _DEVICE_IDENTITY: SpiDeviceConfig = SpiDeviceConfig(
        device_name="Macronix MX25L25635E",
        full_size=32 * 1024 * 1024,
        sector_size=64 * 1024,
        address_width=4,
        four_byte_addr_mode=FourByteAddrMode.NATIVE_4B,
    )

    def __init__(self):
        self.mux_grabbed = False
        self.device_identity = self._DEVICE_IDENTITY

        self.ham = SpiFlashHam(0x50600, self.device_identity)

        # Enter 4-byte address mode if required by the device.
        mode = self.device_identity.four_byte_addr_mode
        if mode is FourByteAddrMode.EN4B:
            self.ham.enter_4byte_mode()
        elif mode is FourByteAddrMode.WREN_EN4B:
            self.ham.enter_4byte_mode(wren_first=True)

    @property
    def jedec_id(self) -> dict[str, int | bytes]:
        raw_id = self.ham.read_identification()  # type: ignore

        # Parse continuation bytes and manufacturer ID
        continuation_count = 0
        manufacturer_id = 0
        device_id_start_index = 0

        for index, byte_value in enumerate(raw_id):
            if byte_value == 0x7F:
                continuation_count += 1
            else:
                manufacturer_id = byte_value
                device_id_start_index = index + 1
                break

        # Extract device ID bytes (remaining bytes after manufacturer ID)
        device_id_bytes = raw_id[device_id_start_index:]

        # Parse memory type and capacity (standard JEDEC format)
        mem_type = device_id_bytes[0] if len(device_id_bytes) > 0 else 0
        capacity = device_id_bytes[1] if len(device_id_bytes) > 1 else 0

        return {
            "manufacturer_id": manufacturer_id,
            "continuation_bytes": continuation_count,
            "mem_type": mem_type,
            "capacity": capacity,
        }

    def read_flash(self, length: int | None = None) -> bytes:
        if length is None:
            length = self.device_identity.full_size
        self.mux_grabbed = True
        try:
            page_size_int = self.device_identity.page_size
            num_pages = (length + page_size_int - 1) // page_size_int
            data = bytearray()
            for page_index in range(num_pages):
                address = page_index * page_size_int
                data += self.ham.read_block(address)
            return bytes(data[:length])
        finally:
            self.mux_grabbed = False

    def erase_flash(self, confirm: bool = False) -> None:
        if confirm:
            logger.info(f"Flash: erasing entire device")
            self.mux_grabbed = True
            self.write_protected = False
            try:
                opcodes = self.ham.device_config.opcodes
                if opcodes.chip_erase is not None:
                    logger.info(f"Flash: bulk erase")
                    self.ham.bulk_erase()
                else:
                    logger.warning(
                        f"Flash: device does not support erase — writing directly "
                        f"(expected for byte-addressable devices such as FRAM)"
                    )
            finally:
                self.write_protected = True
                self.mux_grabbed = False
        else:
            logger.info(f"Flash: erase not confirmed — would erase entire device")

    def write_flash(self, data: bytes) -> None:
        full_size_int = self.device_identity.full_size
        if len(data) > full_size_int:
            raise ValueError(
                f"Flash: data size {len(data)} bytes exceeds device size {self.device_identity.full_size} bytes"
            )
        self.mux_grabbed = True
        self.write_protected = False
        logger.info(f"Flash: writing {len(data)} bytes to flash")
        try:
            page_size_int = self.device_identity.page_size
            num_pages = (len(data) + page_size_int - 1) // page_size_int
            for page_index in range(num_pages):
                address = page_index * page_size_int
                page_data = data[address : address + page_size_int]
                self.ham.write_block(address, page_data)
        finally:
            self.write_protected = True
            self.mux_grabbed = False

    def verify_flash(self, file_path: str, read_entire_flash: bool = False) -> None:
        with open(file_path, "rb") as f:
            expected = f.read()
        if len(expected) > self.device_identity.full_size:
            raise ValueError(
                f"Flash: file size {len(expected)} bytes exceeds device size {self.device_identity.full_size}"
            )
        read_len = None if read_entire_flash else len(expected)
        logger.info(
            f"Flash: reading back "
            f"{'entire flash' if read_entire_flash else f'{len(expected)} bytes'} "
            f"for verification"
        )
        actual = self.read_flash(length=read_len)
        actual_region = actual[: len(expected)]
        if actual_region != expected:
            for offset, (a, e) in enumerate(zip(actual_region, expected)):
                if a != e:
                    raise ValueError(
                        f"Flash: verify failed at offset 0x{offset:06X}: read 0x{a:02X}, expected 0x{e:02X}"
                    )
        logger.info(f"Flash: verify passed")

    def program_device(self, file_path: str) -> None:
        with open(file_path, "rb") as f:
            image = f.read()
        full_size_int = self.device_identity.full_size
        if len(image) > full_size_int:
            raise ValueError(
                f"Flash: file size {len(image)} bytes is too large for "
                f"device size {self.device_identity.full_size}"
            )

        # --- Erase ---
        self.erase_flash(confirm=True)

        # --- Write ---
        self.write_flash(image)

        # --- Verify ---
        self.verify_flash(file_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="FPGA SPI flash utility")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--fpga", action="store_true", help="Read/write FPGA registers")
    group.add_argument("--flash", action="store_true", help="Read/write SPI Flash")

    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument("-r", metavar="ADDRESS", type=lambda x: int(x, 0), help="Read a byte at ADDRESS (hex or decimal)")
    group.add_argument("-w", nargs=2, metavar=("ADDRESS", "VALUE"), help="Write VALUE to ADDRESS (hex or decimal)")
    group.add_argument("--jedec", action="store_true", help="Read and print the device JEDEC ID")
    group.add_argument("--program", metavar="FILE", help="Program the device with the specified file")
    group.add_argument("--verify", metavar="FILE", help="Verify the device against the specified file")
    group.add_argument("--dump", metavar="FILE", help="Dump the entire flash contents")

    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument("--loglevel", choices=["trace", "debug", "info"], default="info", help="Set the logging level")

    parser.add_argument(
        "--ta-len",
        type=int,
        default=_FPGA_TA_LEN,
        help=f"FPGA read turnaround length in bytes (default: {_FPGA_TA_LEN})",
    )
    args = parser.parse_args()

    match args.loglevel.upper():
        case "TRACE":
            log_level = logging.TRACE
        case "DEBUG":
            log_level = logging.DEBUG
        case "INFO":
            log_level = logging.INFO
        case _: # Should not happen due to argparse choices
            pass

    logger.setLevel(level=log_level)

    # Initialise the singleton with the chosen TA length so both the --fpga
    # path and the lazily-constructed --flash path share it.
    fpga = BmcFpgaCtrl(args.ta_len)

    if args.fpga:
        if args.r is not None:
            value = fpga.read_reg(args.r)
            print(f"0x{args.r:08X}: 0x{value:08X}")
        elif args.w is not None:
            address = int(args.w[0], 0)
            value = int(args.w[1], 0)
            fpga.write_reg(address, value)
            print(f"Wrote 0x{value:08X} to 0x{address:08X}")

            read_value = fpga.read_reg(address)
            print(f"Read back 0x{read_value:08X} from 0x{address:08X}")

    if args.flash:
        flash = BmbaFpgaSpiFlash()
        if args.jedec:
            print(flash.jedec_id)
        elif args.r is not None:
            value = flash.ham._spi_read_bytes(args.r, 1)[0]
            print(f"0x{args.r:08X}: 0x{value:02X}")
        elif args.w is not None:
            address = int(args.w[0], 0)
            value = int(args.w[1], 0)
            num_bytes = max(1, (value.bit_length() + 7) // 8)
            data = value.to_bytes(num_bytes, byteorder='big')
            flash.ham._spi_write_bytes(address, data)
            print(f"Wrote 0x{value:0{num_bytes*2}X} to 0x{address:08X}")

            read_back = flash.ham._spi_read_bytes(address, num_bytes)
            read_value = int.from_bytes(read_back, byteorder='big')
            print(f"Read back 0x{read_value:0{num_bytes*2}X} from 0x{address:08X}")
        elif args.program is not None:
            flash.program_device(args.program)
        elif args.verify is not None:
            flash.verify_flash(args.verify)
        elif args.dump is not None:
            data = flash.read_flash()
            with open(args.dump, "wb") as f:
                f.write(data)
            print(f"Dumped flash contents to {args.dump}")

    