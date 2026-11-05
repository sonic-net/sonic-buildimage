"""
file        qsfpdd.py
author      Broadcom OSD Firmware Team
date        09/22/2022
version  1.01 - added file directory to path in order to import same level modules

property $ Copyright: (c) 2022 Broadcom Limited All Rights Reserved $
           No portions of this material may be reproduced in any form
           without the written permission of:
           Broadcom Limited
           408 E. Plumeria Drive
           San Jose, California 95134
           United States
 All information contained in this document/file is Broadcom Limit company
 private proprietary, trade secret, and remains the property of Broadcom
 Limited. The intellectual and technical concepts contained herein are
 proprietary to Broadcom Limited and may be covered by U.S. and Foreign Patents,
 patents in process, and are protected by trade secret or copyright law.
 Dissemination of this information or reproduction of this material is strictly
 forbidden unless prior written permission is obtained from Broadcom Limited.

brief   This file includes all the CMIS compliant I2C driver functions

section
"""
import os
import sys
from array import array
from GS_timing import *
from micas_i2c import I2CException
from util import UtilObj as u
import micas_i2c as i2c_d
import logging
import math


logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler(sys.stdout))


"""
logger.debug("This is a debug message")
logger.info("This is an info message")
logger.warning("This is a warning message")
logger.error("This is an error message")
logger.critical("This is a critical message")
"""


class cmis_mcu_mezz_smb(object):
    mcu_bus_map = [24, 25, 26, 27, 28, 29, 30, 31]
    dev_addr = 0x50
    dev = None

    def __init__(self, twi_dev=None, sn='255',
                 oe_mcu=0, cmis_mcu_select_byte=0x70,
                 delay_ms=50,
                 retry_open_max=10, delay_after_open_ms=15, delay_between_retry_ms=5):
        if twi_dev:
            self.dev = twi_dev
        self.sn = sn
        self.cmis_mcu_select_byte = cmis_mcu_select_byte
        self.delay_ms = delay_ms
        self.oe_mcu = oe_mcu
        self.retry_open_max = retry_open_max
        self.delay_after_open_ms = delay_after_open_ms
        self.delay_between_retry_ms =delay_between_retry_ms
        return

    def __enter__(self):
        retry_open_max = self.retry_open_max
        if self.dev is None:
            self.dev = i2c_d.I2cHost(bus=self.mcu_bus_map[self.oe_mcu])
        self.connect_mcu(num=self.oe_mcu, verbose=True)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return

    def open(self):
        retry_open_max = self.retry_open_max
        if self.dev is None:
            self.dev = i2c_d.I2cHost(bus=self.mcu_bus_map[self.oe_mcu])
        self.connect_mcu(num=self.oe_mcu, verbose=True)

    def close(self):
        return self

    def set_device_address(self, dev_addr):
        self.dev_addr = dev_addr

    def set_page(self, page):
        self.reg_write(0x7F, [page])

    def set_bank(self, bank):
        self.reg_write(0x7E, [bank])

    def reg_read(self, offset, data, verbose=True):
        try:
            data_in = self.dev.reg_read(addr=0x50, offset=offset, n=len(data), verbose=verbose)
            for i in range(len(data)):
                data[i] = data_in[i]
        except I2CException:
            raise cmis_mcu_mezz_smb_Exception('Exception on reg_read, I2C Driver Error, I2C Client Device may have Nacked')       
        
    def reg_write(self, offset, data, verbose=True):
        try:
            self.dev.reg_write(addr=0x50, offset=offset, data=data, verbose=verbose)
        except I2CException:
            if verbose:
                print("reg_write offset: %02X data_size: %d" % (offset, len(data)))
            raise cmis_mcu_mezz_smb_Exception('Exception on reg_write, I2C Driver Error, I2C Client Device may have Nacked')

    def enter_password(self, pwd):
        self.reg_write(122, pwd)

    def connect_mcu(self, num, verbose=False):
        try:
            self.dev.set_bus(bus=self.mcu_bus_map[num])
            if verbose:
                # do nothing as this information is not needed at cli level for ssh query
                pass
        except Exception as err:
            logger.error(type(err))
            logger.error(str(err))

    def enter_debug_mode(self, retry=3, verbose=False):
        data = array('B', [0])
        self.reg_read(0x53, data)                               # read the current system mode
        if verbose:
            print("Requested Mode: %02X" % 0xCC)
            print("System Mode: %02X" % data[0])
        if data[0] == 0xCC:                                       # check if requested mode matches with the current system mode
            return True
        for i in range(retry):
            if verbose:
                print("Entering Pwd: %02X %02X %02X %02X" % (0xF4, 0xDE, 0xCA, 0xF3))
            self.reg_write(0x7A, [0xF4, 0xDE, 0xCA, 0xF3])                                     # enter password
            delay(1000)
            (ret, status) = self.poll_status(0x53, 0xCC, delay_ms=20, retry=100)
            if verbose:
                print('Retry: %d, Status: %02X' % (i, status))
            if ret:
                return True
        (ret, status) = self.poll_status(0x53, 0xCC, delay_ms=0, retry=1)
        if not ret:
            raise cmis_mcu_mezz_smb_Exception('Unable to enter service mode.')
        return ret

    def read_from_data_page(self, data_page, data):
        page = data_page
        rd_num = len(data)
        data_idx = 0
        while rd_num > 0:
            if rd_num > 128:
                rd_pg_byte = 128
            else:
                rd_pg_byte = rd_num
            rd_data = array('B', rd_pg_byte * [0])
            self.set_page(page)
            self.reg_read(0x80, rd_data)
            for i in range(rd_pg_byte):
                data[data_idx] = rd_data[i]
                data_idx += 1
            page += 1
            rd_num -= rd_pg_byte

    def write_to_data_page(self, data_page, data):
        page = data_page
        wr_num = len(data)
        data_idx = 0
        while wr_num > 0:
            if wr_num > 128:
                wr_pg_byte = 128
            else:
                wr_pg_byte = wr_num
            wr_data = array('B', wr_pg_byte * [0])
            for i in range(wr_pg_byte):
                wr_data[i] = data[data_idx]
                data_idx += 1
            self.set_page(page)
            self.reg_write(0x80, wr_data)
            wr_num -= wr_pg_byte
            page += 1

    @staticmethod
    def print_msa_page(offset, data_buffer):
        row_header = ['00', '01', '02', '03', '04', '05', '06', '07', '08', '09', '0A', '0B', '0C', '0D', '0E', '0F']
        col_header = ['00', '10', '20', '30', '40', '50', '60', '70']
        if offset > 127:
            col_header = ['80', '90', 'A0', 'B0', 'C0', 'D0', 'E0', 'F0']
            offset = (offset % 128)
        arr = [['-' for i in range(16)] for j in range(8)]
        idx = offset
        for data in data_buffer:
            x = idx % 16
            y = int(idx / 16)
            arr[y][x] = "{:02X}".format(data)
            idx = idx + 1

        row_format = "{:>4}" * (len(row_header) + 1)
        print(row_format.format("", *row_header))
        for col, row in zip(col_header, arr):
            print(row_format.format(col, *row))


    def read_msa_info(self, bank=0):
        msa_data = {}
        page_data = array('B', 128 * [0])
        self.set_page(0x00)
        self.reg_read(0x00, page_data)
        data16 = page_data[14] * 256 + page_data[15]
        data16 = u.twos_comp(data16, 16)
        msa_data['Temperature'] = data16 / 256
        data16 = page_data[16] * 256 + page_data[17]
        msa_data['Vcc'] = data16 * 0.0001

        print('\nBank: %d\n' % bank)
        self.set_bank(bank)

        self.set_page(0x11)
        self.reg_read(0x80, page_data)
        msa_data['TxLOS'] = bin(page_data[136 - 128])[2:].zfill(8)
        msa_data['TxLOL'] = bin(page_data[137 - 128])[2:].zfill(8)
        msa_data['RxLOS'] = bin(page_data[147 - 128])[2:].zfill(8)
        msa_data['RxLOL'] = bin(page_data[148 - 128])[2:].zfill(8)

        for i in range(4):
            data16 = page_data[154 - 128 + 2 * i] * 256 + page_data[155 - 128 + 2 * i]
            PmW = data16 * 0.0001
            Pdb = 10 * math.log(PmW, 10)
            msa_data['TxLOP%01d' % i] = '%.3fmW %.3fdBm' % (PmW, Pdb)
        for i in range(4):
            data16 = page_data[170 - 128 + 2 * i] * 256 + page_data[171 - 128 + 2 * i]
            msa_data['TxBias%01d' % i] = '%.1fmA' % (data16 * 0.002 * 2)
        for i in range(4):
            data16 = page_data[186 - 128 + 2 * i] * 256 + page_data[187 - 128 + 2 * i]
            PmW = data16 * 0.0001
            Pdb = 10 * math.log(PmW, 10)
            msa_data['RXLIP%01d' % i] = '%.3fmW %.3fdBm' % (PmW, Pdb)

        msa_data['LOP Hi Alarm'] = bin(page_data[139 - 128])[2:].zfill(8)
        msa_data['LOP Lo Alarm'] = bin(page_data[140 - 128])[2:].zfill(8)
        msa_data['LOP Hi Warn'] = bin(page_data[141 - 128])[2:].zfill(8)
        msa_data['LOP Lo Warn'] = bin(page_data[142 - 128])[2:].zfill(8)
        msa_data['Bias Hi Alarm'] = bin(page_data[143 - 128])[2:].zfill(8)
        msa_data['Bias Lo Alarm'] = bin(page_data[144 - 128])[2:].zfill(8)
        msa_data['Bias Hi Warn'] = bin(page_data[145 - 128])[2:].zfill(8)
        msa_data['Bias Lo Warn'] = bin(page_data[146 - 128])[2:].zfill(8)
        msa_data['LIP Hi Alarm'] = bin(page_data[149 - 128])[2:].zfill(8)
        msa_data['LIP Lo Alarm'] = bin(page_data[150 - 128])[2:].zfill(8)
        msa_data['LIP Hi Warn'] = bin(page_data[151 - 128])[2:].zfill(8)
        msa_data['LIP Lo Warn'] = bin(page_data[152 - 128])[2:].zfill(8)

        for item in msa_data:
            print(item + ':', msa_data[item])

    def read(self, msa_page, offset, size, bank=0, delay_ms=None, verbose=True):
        msa_page = int(msa_page)
        offset = int(offset)
        size = int(size)
        bank = int(bank)

        _rtn_data_buffer = []
        if msa_page > 0 and offset < 128:
            print('MSA pages greater than 0 only supports upper page reading.')
            return False, _rtn_data_buffer
        self.set_bank(bank)
        # print('Bank: %d' % bank)
        if offset < 128:
            remain_byte = 128 - offset
        else:
            remain_byte = 256 - offset
        if remain_byte <= size:
            read_byte = remain_byte
        else:
            read_byte = size
        data_buffer = array('B', read_byte * [0])
        self.set_page(msa_page)
        if delay_ms is not None:
            delay(delay_ms)
        self.reg_read(offset, data_buffer)

        _rtn_data_buffer = []
        if len(data_buffer) != size:
            return False, _rtn_data_buffer

        _rtn_str = ''
        for _byte in data_buffer:
            offset = str('{:02X}'.format(_byte))
            _rtn_str += offset

        if verbose:
            # do nothing as this information is not needed at cli level for ssh query
            #     print(f'MSA Page {hex(msa_page)}:')
            #     self.print_msa_page(offset, data_buffer)
            pass

        # This is the only information, and most important to print to the cli output
        print(_rtn_str)

        return True, data_buffer

    def i2c_read(self,msa_page, offset, size, bank=0, delay_ms=None, verbose=False):
        """
        This is a high level i2c_read wrapper that will be used for generic cmis page read
        """
        status,data_buffer = self.read(msa_page=msa_page, offset=offset, size=size, bank=bank, delay_ms=delay_ms, verbose=verbose)
        return status,data_buffer

    def write(self, msa_page, offset, data, bank=0, delay_ms=None, verbose=False):
        msa_page = int(msa_page)
        offset = int(offset)
        bank = int(bank)

        if msa_page > 0 and offset < 128:
            return False
        self.set_bank(bank)
        if verbose:
            # do nothing as this information is not needed at cli level for ssh query
            # logger.info('Bank: %d' % bank)
            pass

        if offset < 128:
            remain_byte = 128 - offset
        else:
            remain_byte = 256 - offset
        if remain_byte <= len(data):
            write_byte = remain_byte
        else:
            write_byte = len(data)
        data_out = array('B', write_byte * [0])
        for i in range(write_byte):
            data_out[i] = data[i]
        self.set_page(msa_page)
        if delay_ms is not None:
            delay(delay_ms)
        self.reg_write(offset, data_out)

    def i2c_write(self, msa_page, offset, data, bank=0, delay_ms=None, verbose=False):
        """
        This is a high level i2c_write wrapper that will be used for generic cmis page write
        """
        return self.write(msa_page=msa_page, offset=offset, data=data, bank=bank, delay_ms=delay_ms, verbose=verbose)

    def poll_status(self, status_reg, status_val, delay_ms=20, retry=100, ack_poll=False, verbose=False):
        if ack_poll:
            if not self.acknowledge_poll(timeout_ms=1000):
                raise cmis_mcu_mezz_smb_Exception("Device I2C Nacked")
        status = array('B', [0])
        status[0] = status_val ^ 0xFF
        reg_data = status_reg
        if type(status_reg) != int:
            reg_data = status_reg.value
        for i in range(retry):
            self.reg_read(reg_data, status)
            if verbose:
                print("Status: %02X Retry Counts: %d" % (status[0], i))
            if status[0] == status_val:
                return True, status[0]
            if delay_ms is not None:
                delay(delay_ms)

        return (status[0] == status_val), status[0]

    def acknowledge_poll(self, timeout_ms=100, delay_ms=10):
        return self.dev.ack_poll(self.dev_addr, timeout_ms=timeout_ms, delay_ms=delay_ms)

    def exit_service_mode(self, retry=3, delay_ms=100,verbose=False):
        data = array('B', [0])
        self.reg_read(0x54, data)
        if verbose:
            print("System Mode: %02X" % data[0])
        if data[0] == 0x00:
            return True
        for i in range(retry):
            pwd = [0x01, 0x02, 0x03, 0x04]
            self.reg_write(0x7A, pwd)
            delay(delay_ms)
            (ret, status) = self.poll_status(0x54, 0x00, delay_ms=10, retry=5)
            if verbose:
                print('Retry: %d, Status: %02X' % (i, status))
            if ret:
                return True
        (ret, status) = self.poll_status(0x54, 0x00, delay_ms=0, retry=1)
        if not ret:
            raise cmis_mcu_mezz_smb_Exception('Unable to exit service mode.')
        return ret

    def msa_pattern_setup(self, channel=9, interface=1, pattern=0, delay_ms=1000,enable=True):
        if not self.exit_service_mode():
            print('Unable to exit service mode.')
            return False
        channel = channel - 1
        if channel < 8:
            print("Channel(s): ", channel + 1)
        else:
            print("Channel(s): ALL")
        interface_str = {0: "HOST", 1: "MEDIA"}
        if interface < 0 or interface > 1:
            print('Only host: 0 and media: 1 interface can be selected.')
            return False
        print("Interface: ", interface_str[interface])
        gen_byte = 144 + interface * 8
        sel_byte = 148 + interface * 8

        pattern_str = {0: "PRBS31Q", 1: "PRBS31", 2: "PRBS23Q", 3: "PRBS23",
                       4: "PRBS15Q", 5: "PRBS15", 6: "PRBS13Q", 7: "PRBS13",
                       8: "PRBS9Q", 9: "PRBS9", 10: "PRBS7Q", 11: "PRBS7",
                       12: "SSPRQ", 13: "Reserved", 14: "Custom", 15: "User Pattern"}
        if pattern < 0 or pattern > 15:
            print('Only pattern ID between 0 - 15 can be selected.')
            return False
        print("Selected Pattern: ", pattern_str[pattern])
        self.set_page(0x13)
        if channel < 8:
            cur_pattern = array('B', [0])
            self.reg_read(sel_byte + int(channel / 2), cur_pattern)
            if channel % 2:
                cur_pattern[0] = cur_pattern[0] & 0x0F
                cur_pattern[0] = cur_pattern[0] | (pattern << 4)
            else:
                cur_pattern[0] = cur_pattern[0] & 0xF0
                cur_pattern[0] = cur_pattern[0] | (pattern & 0xF)
            self.reg_write(sel_byte + int(channel / 2), cur_pattern)
        elif channel == 8:
            cur_pattern = array('B', 4 * [0])
            self.reg_read(sel_byte, cur_pattern)
            for i in range(4):
                cur_pattern[i] = (pattern << 4) | pattern
            self.reg_write(sel_byte, cur_pattern)
        delay(delay_ms)
        cur_ctrl = array('B', [0])
        self.reg_read(gen_byte, cur_ctrl)
        if channel < 8:
            if enable:
                cur_ctrl[0] = cur_ctrl[0] | (0x01 << channel)
            else:
                cur_ctrl[0] = cur_ctrl[0] & ~(0x01 << channel)
        elif channel == 8:
            if enable:
                cur_ctrl[0] = 0xFF
            else:
                cur_ctrl[0] = 0x00
        self.reg_write(gen_byte, cur_ctrl)
        delay(delay_ms)

        return True

    def tx_disable_handler(self, channel, set_dis):
        self.set_page(0x10)
        tx_dis_reg = array('B', [0])
        self.reg_read(130, tx_dis_reg)
        print('Current Setting: ' + "{0:08b}".format(tx_dis_reg[0]))
        if channel == 8:
            if set_dis:
                tx_dis_reg[0] = 0xFF
            else:
                tx_dis_reg[0] = 0x00
        else:
            if set_dis:
                tx_dis_reg[0] |= (0x01 << channel)
            else:
                tx_dis_reg[0] &= ~(0x01 << channel)
        self.reg_write(130, tx_dis_reg)
        delay(self.delay_ms)
        self.reg_read(130, tx_dis_reg)
        print('New Setting: ' + "{0:08b}".format(tx_dis_reg[0]))
        return True, tx_dis_reg[0]

    @staticmethod
    def calculate_ber(data):
        s1 = (data[0] & 0xF8) >> 3
        m1 = ((data[0] & 0x7) << 8) | data[1]
        return m1 * (10 ** (s1 - 24))

    @staticmethod
    def float_to_u16_sff8636(val):
        shift_counter = 0
        s = 0
        m = 0
        if val < 0:
            val = -1 * val
        if val < 2047:
            while val < 2047 and val != 0:
                val *= 10
                shift_counter -= 1
            m = math.floor(val / 10)
            s = 24 + shift_counter + 1
        else:
            while val >= 2047 and val != 0:
                val /= 10
                shift_counter += 1
            m = math.floor(val)
            s = 24 + shift_counter

        ret = (s << 11) + m
        return ret

    def msa_read_simple_ber(self, channel=9, pattern=0, device=0, interface=1, option=0, spacing=10):
        if not self.exit_service_mode():
            print('Unable to exit service mode.')
            return False
        pattern_str = {0: "PRBS31Q", 1: "PRBS31", 2: "PRBS23Q", 3: "PRBS23",
                       4: "PRBS15Q", 5: "PRBS15", 6: "PRBS13Q", 7: "PRBS13",
                       8: "PRBS9Q", 9: "PRBS9", 10: "PRBS7Q", 11: "PRBS7",
                       12: "SSPRQ", 13: "Reserved", 14: "Custom", 15: "User Pattern"}
        if pattern < 0 or pattern > 15:
            print('Only pattern ID between 0 - 15 can be selected.')
            return False
        print("Selected Pattern: ", pattern_str[pattern])
        ber_data_offset = 192
        ber_ctrl_offset = 160
        complete_flag_offset = 134
        mask = 0xFF
        if interface == 1:
            ber_data_offset = 208
            ber_ctrl_offset = 168
            complete_flag_offset = 135
            if device == 0:
                mask = 0x0F

        channel = channel - 1
        data_in = array('B', 16 * [0])
        channel_size = 4
        if device == 1 or interface == 0:
            channel_size = 8

        if channel > channel_size:
            print('Invalid channel selection')
            return False

        if option == 1:
            header = ['MSB'] * channel_size
        elif option == 2:
            header = ['LSB'] * channel_size
        else:
            header = ['COMB'] * channel_size
        row_format = "{:^{spacing}}" * (len(header) + 1)
        print(row_format.format("", *header, spacing=spacing))
        pattern_byte = ((pattern & 0x0F) << 4) | (pattern & 0x0F)
        self.set_page(0x13)
        self.reg_write(ber_ctrl_offset, [0xFF])  # enable
        self.reg_write(ber_ctrl_offset + 1, [0x00])
        self.reg_write(ber_ctrl_offset + 2, [0x00])
        self.reg_write(ber_ctrl_offset + 4, [pattern_byte, pattern_byte, pattern_byte, pattern_byte])  # pattern select
        delay(5000)
        self.set_page(0x14)
        try:
            while True:
                data_line = ['-'] * channel_size
                status, ret = self.poll_status(complete_flag_offset, mask, 50, 150)
                if not status:
                    print('Timed out waiting for complete flag set')
                    return False
                self.reg_write(130, [0x00])
                self.reg_write(128, [0x11])
                delay(120)
                self.reg_read(ber_data_offset, data_in)
                if channel_size == channel:
                    for i in range(channel_size):
                        data_line[i] = "{:.2E}".format(self.calculate_ber(data_in[2 * i:2 * i + 2]))
                else:
                    data_line[channel] = "{:.2E}".format(self.calculate_ber(data_in[2 * channel:2 * channel + 2]))
                print(row_format.format("", *data_line, spacing=spacing))
        except KeyboardInterrupt:
            self.set_page(0x13)
            self.reg_write(ber_ctrl_offset, [0x00])
            print('Exiting BER Checker..')


class cmis_mcu_mezz_smb_Exception(Exception):
    def __init__(self, detailed_error_message):
        super(cmis_mcu_mezz_smb_Exception, self).__init__(detailed_error_message)

if __name__ == "__main__":
    logger.debug('cmis_mcu_i2c.py')
