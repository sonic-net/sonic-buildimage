"""
file        cdb.py
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

brief   This file includes all the cdb utilities

section
"""
import os
import sys
import subprocess
import re
from util import UtilObj as u
import cmis_mcu_i2c as cmis_smb
from GS_timing import *
from array import array
from os import path
import math
import logging
import time
from oe_cmis_rw import oe_cmis_i2c_rw as oe_cmis_util

logger = logging.getLogger()

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler(sys.stdout))

class cdb_cmd_fields():
    cmd_code_msb = 0x00
    cmd_code_lsb = 0x01
    epl_len_msb = 0x02
    epl_len_lsb = 0x03
    lpl_len = 0x04
    check_code = 0x05
    pwd_0 = 0x08
    addr_0 = 0x08
    pwd_1 = 0x09
    addr_1 = 0x09
    pwd_2 = 0x0A
    addr_2 = 0x0A
    pwd_3 = 0x0B
    addr_3 = 0x0B


class cdbObj(object):
    driver_obj = None
    cmd_data = [0x00] * 12

    def __init__(self, oe_num=0):
        self.cmis_i2c = cmis_smb.cmis_mcu_mezz_smb(oe_mcu=oe_num)
        self.oe_mcu = oe_num
        self.cmis_util = oe_cmis_util()

    def __enter__(self):
        self.cmis_i2c.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.cmis_i2c.close()
        return

    def open(self):
        self.cmis_i2c.open()
    
    def close(self):
        self.cmis_i2c.close()

    def exec_rd_cmd(self, offset, rd_len, bank_num=0, page_num=0, verbose=False):
        i = 0
        timeout = 25
        while i <= timeout:
            try:
                status, output = self.cmis_util.cmis_read(self.oe_mcu, bank_num, page_num, offset, rd_len, lock=False)
                if status != True:
                    raise CdbException("oe_cmis_sysfs_rw READ ERROR {}".format(output))
                else:
                    break
            except CdbException as e:
                if (verbose == True):
                    print("An error occurred: {}".format(str(e)))
                raise cmis_smb.cmis_mcu_mezz_smb_Exception('Exception on cmis_read')
        if isinstance(output, list):
            result = array('B', output)
        else:
            result = array('B', [])
        return result

    def exec_wr_cmd(self, offset, wr_data_arr, bank_num=0x0, page_num=0x0, verbose=False):
        try:
            status, output = self.cmis_util.cmis_write(self.oe_mcu, bank_num, page_num, offset, wr_data_arr, lock=False)
            if status != True:
                raise CdbException("oe_cmis_sysfs_rw WRITE ERROR {}".format(output))
        except CdbException as e:
            if (verbose == True):
                print("An error occurred: {}".format(str(e)))
            raise cmis_smb.cmis_mcu_mezz_smb_Exception('Exception on cmis_write')
        offset = offset + 1

    def set_cmd_field(self, field, val):
        self.cmd_data[field] = val

    def set_word_field(self, field_msb, val):
        data = [(val >> 8) & 0xFF, val & 0xFF]
        self.cmd_data[field_msb] = data[0]
        self.cmd_data[field_msb + 1] = data[1]

    def set_dword_field(self, field_msb, val):
        data = [(val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF]
        self.cmd_data[field_msb] = data[0]
        self.cmd_data[field_msb + 1] = data[1]
        self.cmd_data[field_msb + 2] = data[2]
        self.cmd_data[field_msb + 3] = data[3]

    def set_pwd_field(self, pwd):
        self.set_cmd_field(cdb_cmd_fields.pwd_0, pwd[0])
        self.set_cmd_field(cdb_cmd_fields.pwd_1, pwd[1])
        self.set_cmd_field(cdb_cmd_fields.pwd_2, pwd[2])
        self.set_cmd_field(cdb_cmd_fields.pwd_3, pwd[3])

    def clear_cmd_fields(self):
        for i in range(len(self.cmd_data)):
            self.cmd_data[i] = 0x00

    def write_cmd_fields(self):
        # self.cmis_i2c.set_page(0x9F)                            # set page
        # self.cmis_i2c.reg_write(0x82, self.cmd_data[2:])  # write cmd fields
        # self.cmis_i2c.reg_write(0x80, self.cmd_data[:2])  # write cmd
        self.exec_wr_cmd(0x82, self.cmd_data[2:], 0, 0x9F)
        self.exec_wr_cmd(0x80, self.cmd_data[:2], 0, 0x9F)

    def set_check_code(self, data_len=None, more_data=None):
        check_sum = 0
        check_len = len(self.cmd_data)
        if data_len is not None:
            check_len = data_len
        for i in range(check_len):
            check_sum += self.cmd_data[i]
        if more_data:
            for data in more_data:
                check_sum += data
        check_sum ^= 0xFF
        check_sum &= 0xFF
        self.set_cmd_field(cdb_cmd_fields.check_code, check_sum)

    def poll_reg_status(self, status_reg, status_val, delay_ms=20, retry=100, ack_poll=False, verbose=False):
        if ack_poll:
            if not self.cmis_i2c.acknowledge_poll(timeout_ms=1000):
                raise CdbException("Device I2C Nacked")
        status = array('B', [status_val ^ 0xFF])
        reg_data = status_reg
        if type(status_reg) != int:
            reg_data = status_reg.value
        for i in range(retry):
            status = self.exec_rd_cmd(reg_data, 1)
            # self.cmis_i2c.reg_read(reg_data, status)
            if verbose:
                print("Status: %02X Retry Counts: %d" % (status[0], i))
            if status[0] == status_val:
                return True, status[0]
            delay(delay_ms)

        return (status[0] == status_val), status[0]

    def poll_status(self, delay_ms=1, timeout=10000, force=False, get_ret=False, verbose=True):
        i = 0
        status = array('B', [0])
        fw_mode = array('B', [0x55])
        while i <= timeout:
            try:
                status = self.exec_rd_cmd(0x25, 1)
                fw_mode = self.exec_rd_cmd(0x53, 1)
                # self.cmis_i2c.reg_read(0x25, status, verbose=False)
                # self.cmis_i2c.reg_read(0x53, fw_mode, verbose=False)
            except cmis_smb.cmis_mcu_mezz_smb_Exception:  # module i2c not ready, continue to ack poll
                delay(delay_ms)
                i += 1
                continue
            # if verbose:
            #     logger.debug("Status %02X Mode %02X" % (status[0], fw_mode[0]))
            if not force and fw_mode[0] != 0xAA and status[0] == 0 and i > 500:  # check if system is still in cdb mode, if not then probbaly reset happened
                logger.error('CMD Readback Fw Mode: %02X Status Code: %02X' % (fw_mode[0], status[0]))
                logger.error('CMD Failed Polling Status, Reset Occurred')
                raise CdbException('CDB Mode Is No Longer Valid, A System Reset May Have Occurred')
            if status[0] == 0x01:  # if status is success
                return True, status[0]
            if (status[0] & 0x80 == 0x00) and (status[0] & 0xF0 == 0x40):  # if currently not busy and failed bit is set
                try:
                    status = self.exec_rd_cmd(0x25, 1)
                    # self.cmis_i2c.reg_read(0x25, status)  # read the status one more time to make sure that it wasn't just the previous latched status
                except cmis_smb.cmis_mcu_mezz_smb_Exception:  # module i2c not ready, continue to ack poll
                    raise CdbException('I2C Nacked on Status Read')
                if status[0] == 0x01:
                    return True, status[0]
                if (status[0] & 0xF0 == 0x40):  # check for failure again
                    if get_ret:
                        return False, status[0]
                    else:
                        logger.error('CMD Failed Status Code: %02X' % status[0])
                        raise CdbException('CMD Failed Status Code: %02X' % status[0])
            delay(delay_ms)
            i += 1
        if verbose:
            logger.debug('CMD Poll Time Out Status Code: %02X' % status[0])
        return False, status[0]

    def cdb_unlock(self, poll_count=2, delay_s=1):
        pwd = [0x00, 0x00, 0x10, 0x11]
        for i in range(poll_count):
            logger.debug('CDB Unlock Poll Count: %d' % i)
            self.clear_cmd_fields()
            self.set_pwd_field(pwd)
            self.set_cmd_field(cdb_cmd_fields.lpl_len, 0x04)
            self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0001)
            self.set_check_code()
            self.write_cmd_fields()
            ret, status = self.poll_status(delay_ms=100, timeout=100, force=True, verbose=True)
            if ret:
                return True
            elif status == 0x46:
                pwd[1] ^= 0x01
            delay(delay_s * 1000)
        ret, status = self.poll_status(delay_ms=100, timeout=100, force=True, verbose=True)
        if ret:
            return True
        else:
            logger.error('CDB Failed to Unlock.')
            raise CdbException('CDB Failed to Unlock.')
            return False
    
    def cdb_lock(self, poll_count=2, delay_s=1):
        pwd = [0x12, 0x34, 0x56, 0x78]
        for i in range(poll_count):
            logger.debug('CDB Unlock Poll Count: %d' % i)
            self.clear_cmd_fields()
            self.set_pwd_field(pwd)
            self.set_cmd_field(cdb_cmd_fields.lpl_len, 0x04)
            self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0001)
            self.set_check_code()
            self.write_cmd_fields()
            ret, status = self.poll_status(delay_ms=100, timeout=100, force=True, get_ret=True, verbose=True)
            if status == 0x46:
                return True
            delay(delay_s * 1000)
        ret, status = self.poll_status(delay_ms=100, timeout=100, force=True, get_ret=True, verbose=True)
        if status == 0x46:
            return True
        else:
            logger.error('CDB Failed to Lock.')
            raise CdbException('CDB Failed to Lock.')
            return False

    @staticmethod
    def process_fw_status(fw_info, fw_info_extra=None, more=False):
        img_1 = 'A'
        img_2 = 'B'
        fw_ver_1 = "%02X.%02X" % (fw_info[2], fw_info[3])
        fw_ver_2 = "%02X.%02X" % (fw_info[38], fw_info[39])
        if more:
            fw_ver_1 = '%02X.%02X.%02X' % (fw_info[2], fw_info[3], fw_info[4])
            fw_ver_2 = '%02X.%02X.%02X' % (fw_info[38], fw_info[39], fw_info[40])
        fw_crc_1 = '%02X%02X%02X%02X' % (fw_info[6], fw_info[7], fw_info[8], fw_info[9])
        fw_crc_2 = '%02X%02X%02X%02X' % (fw_info[42], fw_info[43], fw_info[44], fw_info[45])
        ee_ver_1 = '%02X.%02X.%02X' % (fw_info[10], fw_info[11], fw_info[12])
        ee_ver_2 = '%02X.%02X.%02X' % (fw_info[46], fw_info[47], fw_info[48])
        ee_crc_1 = '%02X%02X%02X%02X' % (fw_info[13], fw_info[14], fw_info[15], fw_info[16])
        ee_crc_2 = '%02X%02X%02X%02X' % (fw_info[49], fw_info[50], fw_info[51], fw_info[52])
        if fw_info_extra is not None:
            oe_fw_ver_1 = '%08X' % (u.uint8_to_uint32(fw_info_extra[0:4]))
            oe_fw_ver_2 = '%08X' % (u.uint8_to_uint32(fw_info_extra[64:68]))
            oe_fw_crc_1 = '%08X' % (u.uint8_to_uint32(fw_info_extra[4:8]))
            oe_fw_crc_2 = '%08X' % (u.uint8_to_uint32(fw_info_extra[68:72]))

        bl_ver = '%02X.%02X.%02X' % (fw_info[74], fw_info[75], fw_info[76])

        if fw_info[0] & 0x10:
            img_1 = 'B'
            img_2 = 'A'
            fw_ver_2 = "%02X.%02X" % (fw_info[2], fw_info[3])
            fw_ver_1 = "%02X.%02X" % (fw_info[38], fw_info[39])
            if more:
                fw_ver_2 = '%02X.%02X.%02X' % (fw_info[2], fw_info[3], fw_info[4])
                fw_ver_1 = '%02X.%02X.%02X' % (fw_info[38], fw_info[39], fw_info[40])
            fw_crc_2 = '%02X%02X%02X%02X' % (fw_info[6], fw_info[7], fw_info[8], fw_info[9])
            fw_crc_1 = '%02X%02X%02X%02X' % (fw_info[42], fw_info[43], fw_info[44], fw_info[45])
            ee_ver_2 = '%02X.%02X.%02X' % (fw_info[10], fw_info[11], fw_info[12])
            ee_ver_1 = '%02X.%02X.%02X' % (fw_info[46], fw_info[47], fw_info[48])
            ee_crc_2 = '%02X%02X%02X%02X' % (fw_info[13], fw_info[14], fw_info[15], fw_info[16])
            ee_crc_1 = '%02X%02X%02X%02X' % (fw_info[49], fw_info[50], fw_info[51], fw_info[52])
        logger.info('CDB Status: %02X' % fw_info[0])
        logger.info('Running Image: ' + img_1)
        logger.info('FW Revision: ' + fw_ver_1)
        logger.info('FW CRC: ' + fw_crc_1)
        logger.info('EE Revision: ' + ee_ver_1)
        logger.info('EE CRC: ' + ee_crc_1)
        logger.info('')
        logger.info('Not running Image: ' + img_2)
        logger.info('FW Revision: ' + fw_ver_2)
        logger.info('FW CRC: ' + fw_crc_2)
        logger.info('EE Revision: ' + ee_ver_2)
        logger.info('EE CRC: ' + ee_crc_2)
        logger.info('')
        if more:
            logger.info('Bootloader Revision: ' + bl_ver)
            logger.info('')

    @staticmethod
    def get_fw_version_info(fw_info, fw_info_extra=None):
        fw_ver = [{}, {}]
        fw_ver[0]["CMIS_FW"] = {"version": bytearray(fw_info[2:4]), "crc": bytearray(fw_info[6:10])}
        fw_ver[0]["CMIS_EE"] = {"version": bytearray(fw_info[10:13]), "crc": bytearray(fw_info[13:17])}
        if fw_info_extra is not None:
            fw_ver[0]["OE_FW"] = {"version": bytearray(fw_info_extra[2:4]), "crc": bytearray(fw_info_extra[4:8])}
            fw_ver[0]["OE_EE"] = {"version": bytearray(fw_info_extra[10:12]), "crc": bytearray(fw_info_extra[12:16])}
            fw_ver[0]["RLM_FW"] = {"version": bytearray(fw_info_extra[18:20]), "crc": bytearray(fw_info_extra[20:24])}
            fw_ver[0]["RLM_EE"] = {"version": bytearray(fw_info_extra[26:28]), "crc": bytearray(fw_info_extra[28:32])}

        fw_ver[1]["CMIS_FW"] = {"version": bytearray(fw_info[38:40]), "crc": bytearray(fw_info[42:46])}
        fw_ver[1]["CMIS_EE"] = {"version": bytearray(fw_info[46:49]), "crc": bytearray(fw_info[49:43])}
        if fw_info_extra is not None:
            fw_ver[1]["OE_FW"] = {"version": bytearray(fw_info_extra[66:68]), "crc": bytearray(fw_info_extra[68:72])}
            fw_ver[1]["OE_EE"] = {"version": bytearray(fw_info_extra[74:76]), "crc": bytearray(fw_info_extra[76:80])}
            fw_ver[1]["RLM_FW"] = {"version": bytearray(fw_info_extra[82:84]), "crc": bytearray(fw_info_extra[84:88])}
            fw_ver[1]["RLM_EE"] = {"version": bytearray(fw_info_extra[90:92]), "crc": bytearray(fw_info_extra[92:96])}

        return fw_ver

    def fw_status(self, poll_count=2, delay_ms=1, verbose=True, more=True):
        self.cdb_unlock(poll_count)
        self.clear_cmd_fields()
        self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0100)
        if more:
            self.set_cmd_field(cdb_cmd_fields.lpl_len, 1)
        self.set_check_code()
        self.write_cmd_fields()
        ret, status = self.poll_status(delay_ms=100)
        if not ret:
            raise CdbException('CDB FW Status Timed out from status polling')
        fw_info = array('B', 120 * [0])
        # self.cmis_i2c.set_page(0x9F)
        # self.cmis_i2c.reg_read(0x88, fw_info)               # 120 bytes starting from byte 136
        fw_info = self.exec_rd_cmd(0x88, 120, 0, 0x9F)
        fw_info_extra = array('B', 128 * [0])
        # self.cmis_i2c.set_page(0xA0)
        # self.cmis_i2c.reg_read(0x80, fw_info_extra)
        fw_info_extra = self.exec_rd_cmd(0x80, 128, 0, 0xA0)
        if verbose:
            self.process_fw_status(fw_info, fw_info_extra=fw_info_extra, more=more)
        return True, fw_info, fw_info_extra

    def fw_run(self, bank=None, verbose=True):
        if not self.cdb_unlock():
            return False
        self.clear_cmd_fields()
        self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0109)
        self.set_cmd_field(cdb_cmd_fields.lpl_len, 0x04)
        if bank is not None:
            self.set_cmd_field(cdb_cmd_fields.addr_1, bank)
        self.set_check_code()
        try:
            self.write_cmd_fields()
        except:
            delay(5000)
            if not self.cmis_i2c.acknowledge_poll(10000):
                logger.info('Timed out from acknowledge polling')
                raise CdbException('Timed out from acknowledge polling')
            (status, info, extra_info) = self.fw_status(poll_count=100, delay_ms=5, verbose=verbose)
            if not status:
                logger.info('Failed running to the new loaded image')
                raise CdbException('Failed running to the new loaded image')

            return True
        delay(5000)
        if not self.cmis_i2c.acknowledge_poll(10000):
            logger.info('Timed out from acknowledge polling')
            raise CdbException('Timed out from acknowledge polling')
        (status, info, extra_info) = self.fw_status(poll_count=100, delay_ms=5, verbose=verbose)
        if not status:
            logger.info('Failed running to the new loaded image')
            raise CdbException('Failed running to the new loaded image')

        return True

    def fw_commit(self):
        for x in range(0, 1000):
            try:
                if not self.cdb_unlock():
                    return False
                self.clear_cmd_fields()
                self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x010A)
                self.set_check_code()
                self.write_cmd_fields()
                delay(100)
                ret, status = self.poll_status(delay_ms=10, timeout=100)
                if not ret:
                    raise CdbException('Failed committing to the new loaded image')
                return True
            except:
                self.cmis_i2c.acknowledge_poll(timeout_ms=2000, delay_ms=1)
                logger.error("CDB Commit Retry %d" % x)
                continue
        return True

    def fw_complete_load(self, poll_count=1000, delay_ms=10):
        ret = True
        for x in range(0, poll_count):
            try:
                self.clear_cmd_fields()
                self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0107)
                self.set_check_code()
                self.write_cmd_fields()
                delay(100)
                ret, status = self.poll_status(delay_ms=delay_ms, get_ret=True)
                if not ret and status != 0x45:  # return code 0x45 means cmd payload checksum error, i2c glitch may have occured, try to handle it here
                    logger.error('CDB Failed to complete loading.')
                    raise CdbException('CDB Failed complete fw load')
                if ret and status == 0x01:
                    return True
            except:
                self.cmis_i2c.acknowledge_poll(timeout_ms=2000, delay_ms=1)
                logger.error("CDB Complete Retry %d" % x)
                continue
        if not ret:
            logger.error('CDB Failed to complete loading.')
            raise CdbException('CDB Failed complete fw load')
        return True

    def fw_start_download(self, header, image_size, start_addr=None, bootloader=False, rlm_num=None):
        self.clear_cmd_fields()
        self.set_dword_field(cdb_cmd_fields.addr_0, image_size)
        # self.cmis_i2c.set_page(0x9F)
        # self.cmis_i2c.reg_write(140, 116 * [0x00])
        # self.cmis_i2c.reg_write(144, header)
        self.exec_wr_cmd(140, 116 * [0x00], 0, 0x9F)
        self.exec_wr_cmd(144, header, 0, 0x9F)
        more_data = header
        if start_addr is not None:
            start_addr_data = [(start_addr >> 24) & 0xFF, (start_addr >> 16) & 0xFF, (start_addr >> 8) & 0xFF, start_addr & 0xFF]
            self.exec_wr_cmd(200, start_addr_data, 0, 0x9F)
            # self.cmis_i2c.reg_write(200, start_addr_data)
            more_data = header + start_addr_data
        if rlm_num is not None:
            self.exec_wr_cmd(204, [rlm_num], 0, 0x9F)
            # self.cmis_i2c.reg_write(204, [rlm_num])
            more_data = more_data + [rlm_num]
        self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0101)
        if rlm_num is not None:
            self.set_cmd_field(cdb_cmd_fields.lpl_len, 121)
        else:
            self.set_cmd_field(cdb_cmd_fields.lpl_len, 120)
        self.set_check_code(more_data=more_data)
        self.write_cmd_fields()
        if bootloader:
            delay(500)
        else:
            delay(100)
        ret, status = self.poll_status(delay_ms=10, force=True)
        if not ret:
            logger.error('CDB Failed to start loading.')
            raise CdbException('CDB Failed start fw load')
        return True

    def fw_abort_load(self):
        self.clear_cmd_fields()
        self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0102)
        self.set_check_code(data_len=8)
        self.write_cmd_fields()
        delay(100)
        ret, status = self.poll_status(delay_ms=10)
        if not ret:
            logger.error('CDB Failed to abort loading.')
            raise CdbException('CDB Failed abort fw load')
        return True

    def fw_load_epl(self, data_buffer, data_length, addr=None):
        page_num = int(data_length / 128)
        remainder = data_length % 128
        i = 0
        while i < page_num:
            # self.cmis_i2c.set_page(0xA0 + i)  # Select page
            # self.cmis_i2c.reg_write(0x80, data_buffer[0x80 * i: 0x80 * i + 0x80])  # Write to SRAM
            self.exec_wr_cmd(0x80, data_buffer[0x80 * i: 0x80 * i + 0x80], 0, (0xA0 + i))
            i = i + 1
        if remainder > 0:
            # self.cmis_i2c.set_page(0xA0 + i)
            # self.cmis_i2c.reg_write(0x80, data_buffer[0x80 * i: 0x80 * i + remainder])
            self.exec_wr_cmd(0x80, data_buffer[0x80 * i: 0x80 * i + remainder], 0, (0xA0 + i))
        self.clear_cmd_fields()
        self.set_word_field(cdb_cmd_fields.epl_len_msb, page_num * 0x80 + remainder)
        self.set_cmd_field(cdb_cmd_fields.lpl_len, 0x04)
        if addr is not None:
            self.set_dword_field(cdb_cmd_fields.addr_0, addr)
        self.set_word_field(cdb_cmd_fields.cmd_code_msb, 0x0104)
        self.set_check_code()
        self.write_cmd_fields()
        delay(50)
        ret, status = self.poll_status(delay_ms=20)
        if not ret:
            logger.error('Loading EPL Failed.')
            raise CdbException('Failed loading EPL')
        return True

    def cdb_read_serial_number(self):
        sn_data = array('B', 16 * [0])
        # self.cmis_i2c.set_page(0x00)
        # self.cmis_i2c.reg_read(166, sn_data)               # 16 bytes starting from byte 166
        sn_data = self.exec_rd_cmd(166, 16)
        sn_str = bytes(sn_data).decode('ascii').strip()
        return sn_data, sn_str
    
    @staticmethod
    def cdb_parse_serial_number_from_filename(filename):
        fn_items = filename.split('_')
        # product str, sn str, oe_num, ee type
        print(fn_items)
        return fn_items[0], fn_items[1], fn_items[2], fn_items[3]

    def cdb_check_cmis_ee_match(self, filename):
        supported_products = ['Bailly']
        product_str, sn_str, oe_num_str, ee_type = self.cdb_parse_serial_number_from_filename(filename)
        if product_str not in supported_products:
            raise CdbException("Unsupported product or Invalid Image File. Please use the original provided image file without renaming")
        try:
            oe_num = int(oe_num_str[2])
        except:
            raise CdbException("Invalid OE Number Conversion or Invalid Image File. Please use the original provided image file without renaming")
        if oe_num > 7:
            raise CdbException("Invalid OE Number or Invalid Image File. Please use the original provided image file without renaming")
        sn_str += "%02X" % oe_num
        if ee_type != 'CMIS':
            raise CdbException("Unsupported image type or Invalid Image File. Please use the original provided image file without renaming")
        try:
            sn_data, rd_sn_str = self.cdb_read_serial_number()
        except:
            raise CdbException("Unable to read out serial number. Please check i2c communication to the CMIS interface")
        if rd_sn_str != sn_str:
            print(rd_sn_str, end='')
            print(' vs ', end='')
            print(sn_str)
            raise CdbException("Mismatch Serial Number. Please use the image file with the matching serial number")
        return True, product_str, sn_str, oe_num, ee_type
    
    def cdb_check_oe_ee_match(self, filename):
        supported_products = ['Bailly']
        product_str, sn_str, oe_num_str, ee_type = self.cdb_parse_serial_number_from_filename(filename)
        if product_str not in supported_products:
            raise CdbException("Unsupported product or Invalid Image File. Please use the original provided image file without renaming")
        try:
            oe_num = int(oe_num_str[2])
        except:
            raise CdbException("Invalid OE Number Conversion or Invalid Image File. Please use the original provided image file without renaming")
        if oe_num > 7:
            raise CdbException("Invalid OE Number or Invalid Image File. Please use the original provided image file without renaming")
        sn_str += "%02X" % oe_num
        if ee_type != 'OE':
            raise CdbException("Unsupported image type or Invalid Image File. Please use the original provided image file without renaming")
        try:
            sn_data, rd_sn_str = self.cdb_read_serial_number()
        except:
            raise CdbException("Unable to read out serial number. Please check i2c communication to the CMIS interface")
        if rd_sn_str != sn_str:
            print(rd_sn_str, end='')
            print(' vs ', end='')
            print(sn_str)
            raise CdbException("Mismatch Serial Number. Please use the image file with the matching serial number")
        return True, product_str, sn_str, oe_num, ee_type

    @staticmethod
    def parse_cdb_image_info(file_lines):
        image_info = []
        img_num = int((len(file_lines) - 1) / 2)

        for i in range(img_num):
            image = {}
            img_header = file_lines[2 * i + 1].rstrip()
            header_data = [int(img_header[n:n + 2], 16) for n in range(0, len(img_header), 2)]
            img_line = file_lines[2 * i + 2].rstrip()
            img_size = int(len(img_line) / 2)
            image['HEADER'] = header_data
            image['DATA_STR'] = img_line
            image['SIZE'] = img_size
            image['TYPE'] = header_data[0]
            if (header_data[1] & 0x80):
                image['IMG_FULL'] = 1
            else:
                image['IMG_FULL'] = 0
            image['LOC'] = (header_data[1] & 0x03)
            image['LOAD_OPT'] = header_data[2]
            unique_id_str = bytes(header_data[3:]).hex().upper()
            image['UNIQUE_ID'] = unique_id_str
            image_info.append(image)

        return image_info

    @staticmethod
    def cdb_parse_file_header(file_header):
        file_info = {}
        file_info['id'] = file_header[:16]
        file_info['img_num'] = int(file_header[16:18], 16)
        file_info['img_layout'] = []
        file_info['load_action'] = []
        for i in range(file_info['img_num']):
            file_info['img_layout'].append(file_header[18+4*i:18+4*i+4])
        for i in range(file_info['img_num']):
            file_info['load_action'].append(file_header[18+file_info['img_num']*4+4*i:18+file_info['img_num']*4+4*i+4])
            if file_info['load_action'][-1] == 'FFFF':
                break
        # print(file_info['id'])
        # print(file_info['img_num'])
        print(file_info['img_layout'])
        print(file_info['load_action'])
        return file_info

    def cdb_load_cmis_fw(self, image_info, size, verify=False):
        ret, fw_info, extra_fw_info = self.fw_status()
        image_dict = image_info

        check_image = image_dict['LOC'] - 1            # 0 -> BANK1, 1 -> BANK2 
        via_bootloader = False
        if image_dict['LOAD_OPT'] & 0x03 == 0x03:
            via_bootloader = True

        logger.info('Upgrading Bank %d\n' % check_image)
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE']
        img_data = image_dict['DATA_STR']
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, bootloader=via_bootloader)
        logger.debug('FW Download Start CMD Done')
        if via_bootloader:
            self.poll_reg_status(status_reg=0x53, status_val=0xAA, ack_poll=True, verbose=True)
        data_buffer = array('B', size * [0])
        img_line_idx = 0
        load_addr = 0
        logger.info('Start Updating MCU Bank%d...' % check_image)
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(100)
        self.fw_complete_load()  # cdb fw load complete cmd
        ret, fw_info, extra_fw_info = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info)  # extract only mcu info
        self.cmis_i2c.acknowledge_poll(timeout_ms=2000, delay_ms=1)
        logger.info('Jumpping to Bank %d' % image_dict['LOC'])
        self.fw_run(bank=check_image, verbose=False)  # jump to the bank with bew rev
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except cmis_smb.cmis_mcu_mezz_smb_Exception as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Jumpping to Bank %d Completed' % image_dict['LOC'])
        ret, fw_info, extra_fw_info = self.fw_status()  # get version & crc info
        actual_fw_ver = self.get_fw_version_info(fw_info)  # extract only mcu info
        logger.info('Commiting Image')
        self.fw_commit()  # commit new image, on next powerup/reset, fw will load new rev img
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except cmis_smb.cmis_mcu_mezz_smb_Exception as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished MCU FW Upgrade')
        return actual_fw_ver[check_image]["CMIS_FW"]  # return version & crc for host to validate

    def cdb_load_oe_fw(self, image_info, size, verify=False):
        IMG_TYPE_STR = ['Partial', 'Full']
        IMG_DATA_STR = ['Invalid', 'Bank 1', 'Bank 2', 'Both Banks']
        LOAD_OPT_STR = ['Invalid', 'Other Bank', 'Current Bank', 'Both Banks']
        image_dict = image_info
        image_start_addr = int(image_dict['DATA_STR'][:8], 16)
        ret, fw_info, fw_info_extra = self.fw_status()


        logger.info('Image Type: %s Image, %s\n' % (IMG_TYPE_STR[image_dict['IMG_FULL']], IMG_DATA_STR[image_dict['LOC']]))

        logger.info('Upgrading OE %s\n' % LOAD_OPT_STR[image_dict['LOAD_OPT']])
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE'] - 4                     # first 4 bytes is address
        img_data = image_dict['DATA_STR'][8:]                  # first 4 bytes is address                             
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, bootloader=False)
        logger.debug('FW Download Start CMD Done')
        data_buffer = array('B', size * [0])                # step size
        img_line_idx = 0
        load_addr = image_start_addr
        logger.info('Start Updating OE...')
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(10)
        self.fw_complete_load()  # cdb fw load complete cmd
        ret, fw_info, fw_info_extra = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info=fw_info, fw_info_extra=fw_info_extra)  # extract only mcu info
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except cmis_smb.cmis_mcu_mezz_smb_Exception as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished OE FW Upgrade')
        return loaded_fw_ver[0]["OE_FW"]  # return version & crc for host to validate
    
    def cdb_load_oe_ee_full(self, image_info, size, verify=True):
        IMG_TYPE_STR = ['Partial', 'Full']
        IMG_DATA_STR = ['Invalid', 'Bank 1', 'Bank 2', 'Both Banks']
        LOAD_OPT_STR = ['Invalid', 'Other Bank', 'Current Bank', 'Both Banks']
        image_dict = image_info
        
        if verify:
            self.cdb_check_oe_ee_match(filename=image_dict['FILENAME'])
        
        image_start_addr = int(image_dict['DATA_STR'][:8], 16)
        ret, fw_info, fw_info_extra = self.fw_status()

        logger.info('Image Type: %s Image, %s\n' % (IMG_TYPE_STR[image_dict['IMG_FULL']], IMG_DATA_STR[image_dict['LOC']]))

        logger.info('Upgrading OE %s\n' % LOAD_OPT_STR[image_dict['LOAD_OPT']])
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE'] - 4                     # first 4 bytes is address
        img_data = image_dict['DATA_STR'][8:]                  # first 4 bytes is address                             
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, start_addr=image_start_addr, bootloader=False)
        logger.debug('FW Download Start CMD Done')
        data_buffer = array('B', size * [0])                # step size
        img_line_idx = 0
        load_addr = image_start_addr
        logger.info('Start Updating OE EE...')
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(10)
        self.fw_complete_load()  # cdb fw load complete cmd
        ret, fw_info, fw_info_extra = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info=fw_info, fw_info_extra=fw_info_extra)  # extract only mcu info
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except cmis_smb.cmis_mcu_mezz_smb_Exception as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished OE EE Upgrade')
        return loaded_fw_ver[0]["OE_EE"]  # return version & crc for host to validate

    def cdb_load_cmis_ee_full(self, image_info, size, verify=True):
        IMG_TYPE_STR = ['Partial', 'Full']
        IMG_DATA_STR = ['Invalid', 'Bank 1', 'Bank 2', 'Both Banks']
        LOAD_OPT_STR = ['Invalid', 'Other Bank', 'Current Bank', 'Both Banks']
        
        image_dict = image_info

        if verify:
            self.cdb_check_cmis_ee_match(filename=image_dict['FILENAME'])

        ret, fw_info, fw_info_extra = self.fw_status()

        logger.info('Input file contain image type: %s Image: %s Size: %d\n' % (IMG_TYPE_STR[image_dict['IMG_FULL']], IMG_DATA_STR[image_dict['LOC']], image_dict['SIZE']))

        logger.info('Upgrading CMIS_EE %s\n' % LOAD_OPT_STR[image_dict['LOAD_OPT']])
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE']                          # total byte size
        img_data = image_dict['DATA_STR']                       # byte buffer                            
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, bootloader=False)
        logger.debug('FW Download Start CMD Done')
        data_buffer = array('B', size * [0])                # step size
        img_line_idx = 0
        load_addr = 0
        logger.info('Start Updating CMIS_EE...')
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(10)
        self.fw_complete_load()  # cdb fw load complete cmd
        ret, fw_info, fw_info_extra = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info=fw_info, fw_info_extra=fw_info_extra)  # extract only mcu info
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except cmis_smb.cmis_mcu_mezz_smb_Exception as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished CMIS EE Upgrade')
        return loaded_fw_ver[0]["CMIS_EE"]  # return version & crc for host to validate

    def cdb_load_rlm_fw(self, image_info, size, rlm_num=0, restart=True, verify=False):
        ret, fw_info, extra_fw_info = self.fw_status()
        image_dict = image_info

        check_image = image_dict['LOC'] - 1            # 0 -> BANK1, 1 -> BANK2 
        via_bootloader = False
        if image_dict['LOAD_OPT'] & 0x03 == 0x03:
            via_bootloader = True

        logger.info('Upgrading Bank %d\n' % check_image)
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE']
        img_data = image_dict['DATA_STR']
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, bootloader=via_bootloader, rlm_num=rlm_num)
        logger.debug('FW Download Start CMD Done')
        if via_bootloader:
            self.poll_reg_status(status_reg=0x53, status_val=0xAA, ack_poll=True, verbose=True)
        data_buffer = array('B', size * [0])
        img_line_idx = 0
        load_addr = 0
        logger.info('Start Updating RLM Bank%d...' % check_image)
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(10)
        self.fw_complete_load(poll_count=10)  # cdb fw load complete cmd
        ret, fw_info, extra_fw_info = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info)  # extract only mcu info
        logger.info('Jumpping to Bank %d' % image_dict['LOC'])
        self.fw_run(bank=check_image, verbose=False)  # jump to the bank with bew rev
        logger.info('Jumpping to Bank %d Completed' % image_dict['LOC'])
        ret, fw_info, extra_fw_info = self.fw_status()  # get version & crc info
        actual_fw_ver = self.get_fw_version_info(fw_info)  # extract only mcu info
        logger.info('Commiting Image')
        self.fw_commit()  # commit new image, on next powerup/reset, fw will load new rev img
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except dd.QsfpDdException as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished RLM FW Upgrade')
        # return actual_fw_ver[check_image]["CMIS_FW"]  # return version & crc for host to validate

    def cdb_load_rlm_ee_full(self, image_info, size, rlm_num=0, restart=True, verify=False):
        IMG_TYPE_STR = ['Partial', 'Full']
        IMG_DATA_STR = ['Invalid', 'Bank 1', 'Bank 2', 'Both Banks']
        LOAD_OPT_STR = ['Invalid', 'Other Bank', 'Current Bank', 'Both Banks']
        
        image_dict = image_info

        ret, fw_info, fw_info_extra = self.fw_status()

        logger.info('Input file contain image type: %s Image: %s Size: %d\n' % (IMG_TYPE_STR[image_dict['IMG_FULL']], IMG_DATA_STR[image_dict['LOC']], image_dict['SIZE']))

        logger.info('Upgrading CMIS_EE %s\n' % LOAD_OPT_STR[image_dict['LOAD_OPT']])
        logger.debug('FW Load Abort CMD')
        self.fw_abort_load()
        logger.debug('FW Load Abort CMD Done')
        header = image_dict['HEADER']
        data_size = image_dict['SIZE']                          # total byte size
        img_data = image_dict['DATA_STR']                       # byte buffer                            
        logger.debug('FW Download Start CMD')
        self.fw_start_download(header=header, image_size=data_size, bootloader=False, rlm_num=rlm_num)
        logger.debug('FW Download Start CMD Done')
        data_buffer = array('B', size * [0])                # step size
        img_line_idx = 0
        load_addr = 0
        logger.info('Start Updating RLM_EE...')
        total_iteration = math.ceil(data_size / size)
        while data_size > 0:
            if data_size >= size:
                read_size = size
            else:
                read_size = data_size
            for x in range(len(data_buffer)):  # Initialize data_buffer to all zeroes
                data_buffer[x] = 0
            raw_data = img_data[img_line_idx:img_line_idx + read_size * 2]
            img_line_idx += read_size * 2
            for x in range(read_size):
                data_buffer[x] = int(raw_data[2 * x:2 * x + 2], 16)
            data_size -= read_size
            if not self.fw_load_epl(data_buffer, read_size, addr=load_addr):
                return False
            load_addr += read_size
            print('.', end='', flush=True)
        logger.info('\n')
        logger.info('Completed')
        delay(10)
        self.fw_complete_load()  # cdb fw load complete cmd
        ret, fw_info, fw_info_extra = self.fw_status()  # get version & crc information
        loaded_fw_ver = self.get_fw_version_info(fw_info=fw_info, fw_info_extra=fw_info_extra)  # extract only mcu info
        self.exec_wr_cmd(26, [0x08])
        # self.cmis_i2c.reg_write(offset=26, data=[0x08])  # msa soft reset
        try:
            self.cmis_i2c.acknowledge_poll(timeout_ms=15000, delay_ms=500)
        except dd.QsfpDdException as e:
            raise CdbException(e.detailed_error_message)
        logger.info('Finished CMIS EE Upgrade')
        return loaded_fw_ver[0]["CMIS_EE"]  # return version & crc for host to validate

    def cdb_update_general(self, filename, size, rlm_num, verify=True):        
        logger.info('input file: %s' % filename)
        try:
            hex_file = open(filename, 'r')  # Open input hex file
        except FileNotFoundError:
            raise CdbException("file not found error")
        
        file_lines = hex_file.readlines()
        hex_file.close()

        logger.info("File Header: %s" % file_lines[0])
        
        file_info = self.cdb_parse_file_header(file_lines[0])

        if file_info['id'] != '4252434D204F5344':
            raise CdbException('Invalid File, Please use the Broadcom Official Released Upgrading Image')

        image_info = self.parse_cdb_image_info(file_lines)

        if file_info['img_num'] != len(image_info):
            raise CdbException('Mismatch File Desciption, Please use Broadcom Official Released Upgrading Image')
        
        IMG_TYPE = [0xCF, 0xCE, 0xEF, 0xEE, 0xAF, 0xAE]
        
        for img in image_info:
            if img.get('TYPE', 0x00) not in IMG_TYPE:
                raise CdbException('Unsupported Image Type, Please use Official Broadcom Released Upgrading Image')
            if img.get('UNIQUE_ID', '') != '4F534443504F4241494C4C594649524D57415245':
                raise CdbException('Invalid Image Header, Please use the Broadcom Official Released Upgrading Image')
        
        for ld_ac in file_info['load_action']:
            if ld_ac == 'FFFF':
                logger.info('End of Loading Action, Upgrading Complete')
                return
            try:
                img_idx = file_info['img_layout'].index(ld_ac)
            except ValueError:
                raise CdbException('Mismatch Image Loading Desciption, Please use Official Broadcom Released Upgrading Image')
            logger.info('msa soft reset')
            self.exec_wr_cmd(26, [0x08]) # msa soft reset
            delay(5000)
            logger.info('Loading Image %d' % img_idx)
            image_dict = image_info[img_idx]
            image_dict['FILENAME'] = os.path.basename(filename)
            if image_dict['TYPE'] == 0xCF:
                logger.info('Loading CMIS FW')
                self.cdb_load_cmis_fw(image_info=image_dict, size=size, verify=verify)
                delay(3000)
            elif image_dict['TYPE'] == 0xCE:
                logger.info('Loading CMIS EE')
                self.cdb_load_cmis_ee_full(image_info=image_dict, size=size, verify=verify)
                delay(3000)
            elif image_dict['TYPE'] == 0xEF:
                logger.info('Loading PRISM FW')
                self.cdb_load_oe_fw(image_info=image_dict, size=size, verify=verify)
                delay(3000)
            elif image_dict['TYPE'] == 0xEE:
                logger.info('Loading PRISM EE')
                self.cdb_load_oe_ee_full(image_info=image_dict, size=size, verify=verify)
                delay(3000)
            elif image_dict['TYPE'] == 0xAF:
                if rlm_num is None:
                    rlm_nums = [0, 1]
                else:
                    rlm_nums = [rlm_num]
                logger.info('Loading RLM FW')
                for _rlm in rlm_nums:
                    self.cdb_load_rlm_fw(image_info=image_dict, size=size, rlm_num=_rlm, verify=verify)
                    delay(3000)
            elif image_dict['TYPE'] == 0xAE:
                if rlm_num is None:
                    rlm_nums = [0, 1]
                else:
                    rlm_nums = [rlm_num]
                logger.info('Loading RLM EE')
                for _rlm in rlm_nums:
                    self.cdb_load_rlm_ee_full(image_info=image_dict, size=size, rlm_num=_rlm, verify=verify)
                    delay(3000)


class CdbException(Exception):
    def __init__(self, detailed_error_message):
        super(CdbException, self).__init__(detailed_error_message)


if __name__ == '__main__':
    pass