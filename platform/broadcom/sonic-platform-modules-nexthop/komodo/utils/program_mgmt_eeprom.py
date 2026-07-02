# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import mmap
import time

BYTE_SIZE = 4
ENDIAN = 'little'

class MgmtSwitchEepromProgrammer:
    """
    A class for programming the BCM53134 management ethernet switch EEPROM.

    To create an instance of the class:
      import program_mgmt_eeprom
      e = program_mgmt_eeprom.MgmtSwitchEepromProgrammer()

    To read individual EEPROM addresses: 
      e.init_spi_master()
      e.read_word(<address>)

    To write individual EEPROM addresses:
      e.init_spi_master()
      e.stage_write_enable()
      e.launch_write_operation()
      e.read_received_data()
      e.write_word(<address>, <byte1>, <byte0>)

    To bring up the management interface (eth0):
      e.program_all()
    """
    
    def __init__(self, offset=0xb0c00000):
        self.init_mmap(offset)

    def init_mmap(self, offset):
        size=0x100000
        with open('/dev/mem', 'r+b') as f:
            mm = mmap.mmap(f.fileno(), size, access=mmap.ACCESS_WRITE, offset=offset)
        self.mm = mm

    def read_reg(self, offset):
        time.sleep(0.05)
        value = int.from_bytes(self.mm[offset:offset+BYTE_SIZE], ENDIAN)
        print("READ " + hex(offset) + " : " + hex(value))

    def write_reg(self, offset, value):
        time.sleep(0.05)
        self.mm[offset:offset+BYTE_SIZE] = value.to_bytes(BYTE_SIZE, ENDIAN)
        print("WRITE " + hex(offset) + " : " + hex(value))

    def read_status_reg(self):
        self.read_reg(0x50864) # read status register
        self.read_reg(0x50874) # read Tx FIFO occupancy register
        self.read_reg(0x50878) # read Rx FIFO occupancy register

    def slave_select_mgmt_eeprom(self):
        self.write_reg(0x50870, 0xFFFFFFFE) # bit 3

    def slave_deselect_mgmt_eeprom(self):
        self.write_reg(0x50870, 0xFFFFFFFF)

    def init_spi_master(self):
        self.write_reg(0x0c, 0x06) # MSWT EEPROM select, disable write protect
        self.write_reg(0x00068, 0x00000003) # mask first 3 SPI_CLK out of 8 per Mwire spec
        self.write_reg(0x00080, 0xA0000000)
        self.write_reg(0x00080, 0xA000000F) # skip every other write/read, arm logic analyzer
        self.write_reg(0x50840, 0x0000000A) # soft reset SPI master
        self.write_reg(0x50860, 0x000001E6) # inhibit Tx, manual CS_N, Tx/Rx FIFO reset, master, enable
        self.write_reg(0x50860, 0x00000186) # deassert Tx/Rx FIFO reset

    def stage_write_enable(self):
        self.slave_deselect_mgmt_eeprom()
        self.write_reg(0x50860, 0x00000186) # disable Tx output 
        self.write_reg(0x50868, 0x000000f3) # write Tx FIFO, {3'b0, opcode[12:8]} = 0x13
        self.write_reg(0x50868, 0x00000000) # write Tx FIFO, opcode[7:0] = 0x00

    def launch_read_operation(self):
        self.slave_select_mgmt_eeprom()
        self.write_reg(0x00068, 0x00000013) # special read bit 
        self.write_reg(0x50860, 0x00000096) # enable Tx output, launch transaction

    def launch_write_operation(self):
        self.slave_select_mgmt_eeprom()
        self.write_reg(0x50860, 0x00000086) # enable Tx output, launch transaction

    def stage_word_read(self, address):
        self.slave_deselect_mgmt_eeprom()

        self.write_reg(0x50860, 0x00000196) # disable Tx output 

        self.write_reg(0x50868, 0xf0) # start bit (1) + opcode (read: 10)
        self.write_reg(0x50868, address << 1)
        self.write_reg(0x50868, 0x00)
        self.write_reg(0x50868, 0x00)

    def read_received_data(self):
        self.slave_deselect_mgmt_eeprom()

        self.write_reg(0x00068, 0x00000003) # reset special read bit 
        self.write_reg(0x50860, 0x00000196) # disable Tx output 

        self.read_reg(0x5086c) # read Rx FIFO, expect dummy byte 0, discard
        self.read_reg(0x5086c) # read Rx FIFO, expect dummy byte 1, discard
        self.read_reg(0x5086c) # read data [15:8] 
        self.read_reg(0x5086c) # read data [7:0]

    def stage_word_write(self, address, byte1, byte0):
        self.slave_deselect_mgmt_eeprom()

        self.write_reg(0x50860, 0x00000186) # disable Tx output 

        # The EEPROM is a Microwire 93LC86C (16Kb) -- the word size is 16-bits
        self.write_reg(0x50868, 0xf4)    # start bit (1) + opcode (write: 01)
        self.write_reg(0x50868, address) # address
        self.write_reg(0x50868, byte1)   # data [15:8]
        self.write_reg(0x50868, byte0)   # data [7:0]

    def write_word(self, address, byte1, byte0):
        self.stage_word_write(address, byte1, byte0)
        time.sleep(0.1)
        self.launch_write_operation()
        time.sleep(0.1)
        self.read_received_data()
        time.sleep(0.1)

    def write_read_word(self, address, byte1, byte0):
        self.stage_word_write(address, byte1, byte0)
        time.sleep(0.1)
        self.launch_write_operation()
        time.sleep(0.1)
        self.read_received_data()
        time.sleep(0.1)

        self.stage_word_read(address)
        time.sleep(0.1)
        self.launch_read_operation()
        time.sleep(0.1)
        self.read_received_data()

    def read_word(self, address):
        self.stage_word_read(address)
        time.sleep(0.1)
        self.launch_read_operation()
        time.sleep(0.1)
        self.read_received_data()

    def program_all(self):
        # Initial setup
        self.init_spi_master()
        self.stage_write_enable()
        self.launch_write_operation()
        self.read_received_data()

        # Program the EEPROM contents
        # Configure Port 5 (SGMII) as 1G + AN disabled
        self.write_word(0x0, 0xa8, 0x20)
        self.write_word(0x1, 0xff, 0x01)
        self.write_word(0x2, 0x00, 0xe6)
        self.write_word(0x3, 0x00, 0x01)
        self.write_word(0x4, 0x00, 0x01)
        self.write_word(0x5, 0xff, 0x01)
        self.write_word(0x6, 0x00, 0x14)
        self.write_word(0x7, 0x3e, 0x01)
        self.write_word(0x8, 0x80, 0x00)
        self.write_word(0x9, 0x20, 0x01)
        self.write_word(0xa, 0x0c, 0x2f)
        self.write_word(0xb, 0x3e, 0x01)
        self.write_word(0xc, 0x83, 0x00)
        self.write_word(0xd, 0x20, 0x01)
        self.write_word(0xe, 0x01, 0x0d)
        self.write_word(0xf, 0x3e, 0x01)
        self.write_word(0x10, 0x84, 0x70)
        self.write_word(0x11, 0x26, 0x01)
        self.write_word(0x12, 0x12, 0x51)
        self.write_word(0x13, 0x3e, 0x01)
        self.write_word(0x14, 0x83, 0x40)
        self.write_word(0x15, 0x34, 0x01)
        self.write_word(0x16, 0x00, 0x03)
        self.write_word(0x17, 0x3e, 0x01)
        self.write_word(0x18, 0x80, 0x00)
        self.write_word(0x19, 0x00, 0x01)
        self.write_word(0x1a, 0x01, 0x40)
        self.write_word(0x1b, 0x20, 0x01)
        self.write_word(0x1c, 0x2c, 0x2f)
        self.write_word(0x1d, 0xff, 0x01)
        self.write_word(0x1e, 0x00, 0x00)
        self.write_word(0x1f, 0x5d, 0x01)
        self.write_word(0x20, 0x00, 0x4b)

    def stage_erase_all(self):
        self.slave_deselect_mgmt_eeprom()
        self.write_reg(0x50860, 0x00000186) # disable Tx output 
        self.write_reg(0x50868, 0x000000f2) # write Tx FIFO, {3'b0, opcode[12:8]} = 0x12
        self.write_reg(0x50868, 0x00000000) # write Tx FIFO, opcode[7:0] = 0x00
