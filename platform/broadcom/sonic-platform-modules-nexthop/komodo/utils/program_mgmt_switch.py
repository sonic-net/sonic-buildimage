#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import mmap
import os
import subprocess
import time

"""
This utility programs the BCM53134 management switch with the below
komodo_mwire_v2 configuration:

    - Configure Port 5 (SGMII) as 1G + AN disabled
    - Enable forwarding to reserved multicast addresses
    - Configures LED function

It programs the MDIO registers and EEPROM with the same configuration.
No reboot is required since the MDIO changes take effect immediately.
The same configuration will persist across reboots due to the EEPROM.
"""

BYTE_SIZE = 4
ENDIAN = "little"
MDIO_CTRL = 0x00F0
PHY_ADDRESS = 0x1E


class FpgaController:

    def __init__(self):
        self.pci_address = self._get_pci_address()
        self.pci_path = f"/sys/bus/pci/devices/{self.pci_address}"
        offset, size = self._get_pci_resource_info()
        self.init_mmap(offset, size)

    def _get_pci_address(self):
        command = "setpci -s 00:02.1 0x19.b | xargs printf '0000:%s:00.0'"
        try:
            return subprocess.check_output(command, shell=True, text=True).strip()
        except subprocess.CalledProcessError as e:
            print(f"Error executing command: {e}")

    def _get_pci_resource_info(self):
        resource_path = os.path.join(self.pci_path, "resource")
        try:
            with open(resource_path, "r") as f:
                # Read the first line for "Region 0"
                line = f.readline().split()
                start_addr = int(line[0], 16)
                end_addr = int(line[1], 16)
                size = end_addr - start_addr + 1
                return start_addr, size
        except FileNotFoundError:
            raise RuntimeError(f"PCI device {self.pci_path} not found.")

    def init_mmap(self, offset, size):
        # Using /dev/mem requires root privileges
        with open("/dev/mem", "r+b") as f:
            self.mm = mmap.mmap(
                f.fileno(), size, access=mmap.ACCESS_WRITE, offset=offset
            )
        print(f"Mapped FPGA at {hex(offset)} with size {hex(size)}")

    def read_reg(self, offset):
        time.sleep(0.001)
        value = int.from_bytes(self.mm[offset : offset + BYTE_SIZE], ENDIAN)

    def write_reg(self, offset, value):
        time.sleep(0.001)
        self.mm[offset : offset + BYTE_SIZE] = value.to_bytes(BYTE_SIZE, ENDIAN)

    def read_status_reg(self):
        self.read_reg(0x50864)  # read status register
        self.read_reg(0x50874)  # read Tx FIFO occupancy register
        self.read_reg(0x50878)  # read Rx FIFO occupancy register

    def slave_select_mgmt_eeprom(self):
        self.write_reg(0x50870, 0xFFFFFFFE)  # bit 3

    def slave_deselect_mgmt_eeprom(self):
        self.write_reg(0x50870, 0xFFFFFFFF)

    def init_spi_master(self):
        self.write_reg(0x0C, 0x06)  # MSWT EEPROM select, disable write protect
        self.write_reg(
            0x00068, 0x00000003
        )  # mask first 3 SPI_CLK out of 8 per Mwire spec
        self.write_reg(0x00080, 0xA0000000)
        self.write_reg(
            0x00080, 0xA000000F
        )  # skip every other write/read, arm logic analyzer
        self.write_reg(0x50840, 0x0000000A)  # soft reset SPI master
        self.write_reg(
            0x50860, 0x000001E6
        )  # inhibit Tx, manual CS_N, Tx/Rx FIFO reset, master, enable
        self.write_reg(0x50860, 0x00000186)  # deassert Tx/Rx FIFO reset

    def stage_write_enable(self):
        self.slave_deselect_mgmt_eeprom()
        self.write_reg(0x50860, 0x00000186)  # disable Tx output
        self.write_reg(
            0x50868, 0x000000F3
        )  # write Tx FIFO, {3'b0, opcode[12:8]} = 0x13
        self.write_reg(0x50868, 0x00000000)  # write Tx FIFO, opcode[7:0] = 0x00

    def launch_read_operation(self):
        self.slave_select_mgmt_eeprom()
        self.write_reg(0x00068, 0x00000013)  # special read bit
        self.write_reg(0x50860, 0x00000096)  # enable Tx output, launch transaction

    def launch_write_operation(self):
        self.slave_select_mgmt_eeprom()
        self.write_reg(0x50860, 0x00000086)  # enable Tx output, launch transaction

    def read_received_data(self):
        self.slave_deselect_mgmt_eeprom()

        self.write_reg(0x00068, 0x00000003)  # reset special read bit
        self.write_reg(0x50860, 0x00000196)  # disable Tx output

        self.read_reg(0x5086C)  # read Rx FIFO, expect dummy byte 0, discard
        self.read_reg(0x5086C)  # read Rx FIFO, expect dummy byte 1, discard
        self.read_reg(0x5086C)  # read data [15:8]
        self.read_reg(0x5086C)  # read data [7:0]

    def stage_word_write(self, address, byte1, byte0):
        self.slave_deselect_mgmt_eeprom()

        self.write_reg(0x50860, 0x00000186)  # disable Tx output

        # The EEPROM is a Microwire 93LC86C (16Kb) -- the word size is 16-bits
        self.write_reg(0x50868, 0xF4)  # start bit (1) + opcode (write: 01)
        self.write_reg(0x50868, address)  # address
        self.write_reg(0x50868, byte1)  # data [15:8]
        self.write_reg(0x50868, byte0)  # data [7:0]

    def write_eeprom_word(self, address, byte1, byte0):
        self.stage_word_write(address, byte1, byte0)
        time.sleep(0.001)
        self.launch_write_operation()
        time.sleep(0.001)
        self.read_received_data()
        time.sleep(0.001)

    def program_eeprom(self):
        print("Programming the BCM53134 EEPROM via SPI...")

        # Initial setup
        self.init_spi_master()
        self.stage_write_enable()
        self.launch_write_operation()
        self.read_received_data()

        self.write_eeprom_word(0x0, 0xA8, 0x26)
        self.write_eeprom_word(0x1, 0xFF, 0x01)
        self.write_eeprom_word(0x2, 0x00, 0xE6)
        self.write_eeprom_word(0x3, 0x00, 0x01)
        self.write_eeprom_word(0x4, 0x00, 0x01)
        self.write_eeprom_word(0x5, 0xFF, 0x01)
        self.write_eeprom_word(0x6, 0x00, 0x14)
        self.write_eeprom_word(0x7, 0x3E, 0x01)
        self.write_eeprom_word(0x8, 0x80, 0x00)
        self.write_eeprom_word(0x9, 0x20, 0x01)
        self.write_eeprom_word(0xA, 0x0C, 0x2F)
        self.write_eeprom_word(0xB, 0x3E, 0x01)
        self.write_eeprom_word(0xC, 0x83, 0x00)
        self.write_eeprom_word(0xD, 0x20, 0x01)
        self.write_eeprom_word(0xE, 0x01, 0x0D)
        self.write_eeprom_word(0xF, 0x3E, 0x01)
        self.write_eeprom_word(0x10, 0x84, 0x70)
        self.write_eeprom_word(0x11, 0x26, 0x01)
        self.write_eeprom_word(0x12, 0x12, 0x51)
        self.write_eeprom_word(0x13, 0x3E, 0x01)
        self.write_eeprom_word(0x14, 0x83, 0x40)
        self.write_eeprom_word(0x15, 0x34, 0x01)
        self.write_eeprom_word(0x16, 0x00, 0x03)
        self.write_eeprom_word(0x17, 0x3E, 0x01)
        self.write_eeprom_word(0x18, 0x80, 0x00)
        self.write_eeprom_word(0x19, 0x00, 0x01)
        self.write_eeprom_word(0x1A, 0x01, 0x40)
        self.write_eeprom_word(0x1B, 0x20, 0x01)
        self.write_eeprom_word(0x1C, 0x2C, 0x2F)
        self.write_eeprom_word(0x1D, 0xFF, 0x01)
        self.write_eeprom_word(0x1E, 0x00, 0x00)
        self.write_eeprom_word(0x1F, 0x5D, 0x01)
        self.write_eeprom_word(0x20, 0x00, 0x4B)
        self.write_eeprom_word(0x21, 0x10, 0x01)
        self.write_eeprom_word(0x22, 0x03, 0x0A)
        self.write_eeprom_word(0x23, 0x12, 0x01)
        self.write_eeprom_word(0x24, 0x03, 0x0A)
        self.write_eeprom_word(0x25, 0x2F, 0x01)
        self.write_eeprom_word(0x26, 0x00, 0x00)

        print("EEPROM programming complete!")

    def mdio_write(self, addr, data):
        if not (0 <= addr <= 0x1F):
            raise ValueError("addr out of range")

        if not (0 <= data <= 0xFFFF):
            raise ValueError("data out of range")

        val = 0x80000000  # MDIO write
        phy_addr = PHY_ADDRESS << 24
        val |= phy_addr

        phy_reg_addr = addr << 16
        val |= phy_reg_addr
        val |= data

        self.write_reg(MDIO_CTRL, val)
        self.write_reg(MDIO_CTRL, 0x0)

    def mdio_write_reg(self, page, offset, data):
        val1 = page << 8
        val1 |= 0x0001
        self.mdio_write(16, val1)

        # For 32-bit writes, split data into two 16-bit MDIO writes
        # Write lower 16 bits to register 24
        self.mdio_write(24, data & 0xFFFF)

        # If data is larger than 16 bits, write upper 16 bits to register 25
        if data > 0xFFFF:
            self.mdio_write(25, (data >> 16) & 0xFFFF)

        val2 = 0x0001
        val2 |= offset << 8
        self.mdio_write(17, val2)

    def program_mdio_reg(self):
        print("Programming BCM53134 directly via MDIO...")

        self.write_reg(0x0C, 0x04)  # MSWT EEPROM deselect, select MDIO

        self.mdio_write_reg(0xE6, 0x00, 0x01)
        self.mdio_write_reg(0x14, 0x3E, 0x8000)
        self.mdio_write_reg(0x14, 0x20, 0x0C2F)
        self.mdio_write_reg(0x14, 0x3E, 0x8300)
        self.mdio_write_reg(0x14, 0x20, 0x010D)
        self.mdio_write_reg(0x14, 0x3E, 0x8470)
        self.mdio_write_reg(0x14, 0x26, 0x1251)
        self.mdio_write_reg(0x14, 0x3E, 0x8340)
        self.mdio_write_reg(0x14, 0x34, 0x0003)
        self.mdio_write_reg(0x14, 0x3E, 0x8000)
        self.mdio_write_reg(0x14, 0x00, 0x0140)
        self.mdio_write_reg(0x14, 0x20, 0x2C2F)
        self.mdio_write_reg(0x00, 0x5D, 0x4B)
        self.mdio_write_reg(0x00, 0x10, 0x030A)
        self.mdio_write_reg(0x00, 0x12, 0x030A)
        self.mdio_write_reg(0x00, 0x2F, 0x0)

        self.write_reg(0x0C, 0x06)  # MSWT EEPROM select

        print("MDIO programming complete!")


def main():
    c = FpgaController()
    c.program_eeprom()
    c.program_mdio_reg()


if __name__ == "__main__":
    main()
