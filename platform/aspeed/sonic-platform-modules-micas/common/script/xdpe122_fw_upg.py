#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import os
import sys
import click
import logging
from platform_util import get_value, set_value
from platform_util import setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS


FW_UPG_UPLOAD_RETRY = 3
FW_UPG_UPLOAD_STATUS_OK = 0
FW_UPG_UPLOAD_STATUS_READ_FAIL = -1
FW_UPG_UPLOAD_STATUS_EAGAIN = -2
FW_UPG_UPLOAD_STATUS_MEMORY_ERROR = -3
FW_UPG_UPLOAD_STATUS_MEMORY_FULL = -4


DEBUG_FILE = "/etc/.xdpe122_fw_upg_debug_flag"

# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "xdpe122_fw_upg.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)
def log_message(message):
    logger.info(message)


def log_debug(message):
    logger.debug(message)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


class IntOrHexParamType(click.ParamType):
    name = "int_or_hex"
    def convert(self, value, param, ctx):
        try:
            # Automatically detect base: decimal, hex (0x..), octal (0o..), binary (0b..)
            return int(value, 0)
        except ValueError:
            self.fail(f"{value!r} is not a valid integer (supports decimal or 0x hex)", param, ctx)

INT_OR_HEX = IntOrHexParamType()


class Xdpe122Firmware:
    def __init__(self, firmware_file, bus, addr):
        self.firmware = firmware_file
        self.i2c_bus = bus
        self.i2c_addr = addr
        loc = "%d-%04x" % (bus, addr)
        self.fw_upg_dev_file = "/dev/xdpe122_%d_0x%02x" % (bus, addr)
        self.fw_upg_dev_available = "/sys/bus/i2c/devices/%s/dev_available" % loc
        self.chip_crc_path = "/sys/bus/i2c/devices/%s/crc" % loc
        self.chip_nvm_time_path = "/sys/bus/i2c/devices/%s/chip_nvm_time" % loc
        self.upload_data_to_emtp = "/sys/bus/i2c/devices/%s/upload_data_to_emtp" % loc
        self.download_emtp_to_data = "/sys/bus/i2c/devices/%s/download_emtp_to_data" % loc
        self.upload_fault1 = "/sys/bus/i2c/devices/%s/fault1" % loc
        self.upload_fault2 = "/sys/bus/i2c/devices/%s/fault2" % loc
        self.parsed_data = []
        self.file_crc = 0

    def do_upload_data_to_emtp(self):
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.upload_data_to_emtp,
            "value": 1,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "upload_data_to_emtp failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "upload_data_to_emtp success"
        log_message(msg)
        return True, msg

    def do_download_emtp_to_data(self):
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.download_emtp_to_data,
            "value": 1,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "download_emtp_to_data failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "download_emtp_to_data success"
        log_message(msg)
        return True, msg

    def program_data(self, fd, data_list):
        offset_start = int(data_list[0], 16)
        # Program
        for i in range(1, 17):
            if data_list[i] == "----":
                continue
            offset = offset_start + i - 1
            val = int(data_list[i], 16)
            wr_buf = [val & 0xff, (val >> 8) & 0xff]
            log_debug("Start programming offset: 0x%04x, wr_buf: %s" % (offset, wr_buf))
            os.lseek(fd, offset, os.SEEK_SET)
            ret = os.write(fd, bytes(wr_buf))
            if ret != len(wr_buf):
                msg = "os.write failed, write data: %s, ret: %s" % (wr_buf, ret)
                log_message(msg)
                return False, msg
            log_debug("programming offset: 0x%04x, wr_buf: %s success" % (offset, wr_buf))
        msg = "program_data success"
        log_debug(msg)
        return True, msg

    def program(self):
        fd = -1
        if not os.path.exists(self.fw_upg_dev_file):
            msg = "%s not found, program failed" % self.fw_upg_dev_file
            log_message(msg)
            return False, msg
        try:
            fd = os.open(self.fw_upg_dev_file, os.O_WRONLY)
            if fd < 0:
                msg = "os.open %s failed, fd: %d" % (self.fw_upg_dev_file, fd)
                log_message(msg)
                return False, msg
            for entry in self.parsed_data:
                ret, msg = self.program_data(fd, entry)
                if ret is False:
                    return False, msg
            msg = "program success"
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "program raise exception, errmsg: %s" % str(e)
            log_message(msg)
            return False, msg
        finally:
            if fd > 0:
                os.close(fd)

    def check_chip_nvm_time(self):
        log_message("Starting to check chip_nvm_time")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.chip_nvm_time_path,
            "int_decode": 10,
        }
        ret, chip_nvm_time = get_value(get_val_conf)
        if ret is False:
            msg = "get chip_nvm_time failed, msg: %s" % chip_nvm_time
            log_message(msg)
            return False, msg
        if chip_nvm_time <= 0:
            msg = "Invalid chip_nvm_time: %d" % chip_nvm_time
            log_message(msg)
            return False, msg
        msg = "check chip_nvm_time success, chip_nvm_time: %d" % chip_nvm_time
        log_message(msg)
        return True, msg

    def check_upload_status(self):
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.upload_fault1,
        }
        ret, upload_fault1 = get_value(get_val_conf)
        if ret is False:
            msg = "get upload_fault1 status failed, msg: %s" % upload_fault1
            log_message(msg)
            return FW_UPG_UPLOAD_STATUS_READ_FAIL, msg

        msg = "get upload_fault1 status success, value: 0x%04x" % upload_fault1
        log_message(msg)

        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.upload_fault2,
        }
        ret, upload_fault2 = get_value(get_val_conf)
        if ret is False:
            msg = "get upload_fault2 status failed, msg: %s" % upload_fault2
            log_message(msg)
            return FW_UPG_UPLOAD_STATUS_READ_FAIL, msg

        msg = "get upload_fault2 status success, value: 0x%04x" % upload_fault2
        log_message(msg)

        # If upload_fault1 bit 0 and upload_fault2 bit1 and bit3 are set to 0, upload is successful.
        if (upload_fault1 & 0x01) == 0 and (upload_fault2 & 0x0a) == 0:
            msg = "upload status check ok"
            log_message(msg)
            return FW_UPG_UPLOAD_STATUS_OK, msg

        # If upload_fault1 bit 0 is set to 1
        # upload has failed because of an invalid password.
        # try upload_data_to_emtp again
        if upload_fault1 & (1 << 0) == 1:
            msg = "upload has failed because of an invalid password, try again"
            log_message(msg)
            return FW_UPG_UPLOAD_STATUS_EAGAIN, msg

        # If upload_fault2 bit 1 is set to 1
        # upload has failed because of a defective bit in the memory.
        if upload_fault2 & (1 << 1) == 1:
            msg = "upload has failed because of a defective bit in the memory."
            log_message(msg)
            return FW_UPG_UPLOAD_STATUS_MEMORY_ERROR, msg

        # upload_fault2 bit 3 is set to 1
        # the memory space is full and cannot be reprogrammed. The device must be replaced.
        msg = "the memory space is full and cannot be reprogrammed. The device must be replaced."
        log_message(msg)
        return FW_UPG_UPLOAD_STATUS_MEMORY_FULL, msg

    def check_crc(self):
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.chip_crc_path,
        }
        ret, chip_crc = get_value(get_val_conf)
        if ret is False:
            msg = "get chip crc failed, msg: %s" % chip_crc
            log_message(msg)
            return False, msg
        if chip_crc != self.file_crc:
            msg = "CRC not match, chip_crc: 0x%08x, file_crc: 0x%08x" % (chip_crc, self.file_crc)
            log_message(msg)
            return False, msg
        msg = "CRC check ok, chip_crc: 0x%08x, file_crc: 0x%08x" % (chip_crc, self.file_crc)
        log_message(msg)
        return True, msg

    def set_dev_available(self, enable):
        if enable is True:
            value = 1
        else:
            value = 0
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.fw_upg_dev_available,
            "value": value,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "set dev_available failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "set dev_available success, value: %d" % value
        log_message(msg)
        return True, msg

    def parse_upg_file(self):
        try:
            if not os.path.isfile(self.firmware):
                msg = "%s not found" % self.firmware
                log_message(msg)
                return False, msg
            line_num = 0
            find_config_data = 0
            with open(self.firmware, 'r') as fd:
                for line in fd:
                    line_num += 1
                    if "Checksum" in line:
                        line_tmp = line.split()
                        self.file_crc = int(line_tmp[len(line_tmp)-1], 16)
                        msg = "Line number: %d, file_crc: 0x%08X" % (line_num, self.file_crc)
                        log_message(msg)
                        continue

                    if "[Config Data]" in line and find_config_data == 0:
                        find_config_data = 1
                        continue

                    if find_config_data == 1:
                        if "End" in line:
                            break
                        line_tmp = line.split()
                        log_debug("Line number: %d, data: %s" % (line_num, line_tmp))
                        self.parsed_data.append(line_tmp)
            msg = "parse upgrade file success, line number: %d" % line_num
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "parse %s failed, Line number: %d, errmsg: %s" % (self.firmware, line_num, str(e))
            log_message(msg)
            return False, msg

    def do_fw_upg_init_check(self):
        # check chip NVM time
        ret, log = self.check_chip_nvm_time()
        if ret is False:
            return ret, log
        return True, "fw_upg_init_check success"

    def do_fw_upg_upload(self):
        for i in range(FW_UPG_UPLOAD_RETRY):
            # upload_data_to_emtp
            ret, log = self.do_upload_data_to_emtp()
            if ret is False:
                return ret, log

            # check_upload_status
            ret, log = self.check_upload_status()
            if ret == FW_UPG_UPLOAD_STATUS_OK:
                msg = "do_fw_upg_upload success, retry: %d" % i
                log_message(msg)
                return True, msg
            if ret != FW_UPG_UPLOAD_STATUS_EAGAIN:
                return False, log
        msg = "do_fw_upg_upload retry failed"
        log_message(msg)
        return False, msg

    def do_fw_upg(self):
        try:
            debug_init()

            ret, log = self.parse_upg_file()
            if ret is False:
                return ret, log

            # check crc
            ret, log = self.check_crc()
            if ret is True:
                msg = "device crc is equal to file crc, skip upgrade"
                log_message(msg)
                return True, msg

            # set dev_available not available
            log_message("Starting to set dev_available not available")
            ret, log = self.set_dev_available(False)
            if ret is False:
                return ret, log

            log_message("Starting to do_fw_upg_init_check")
            ret, log = self.do_fw_upg_init_check()
            if ret is False:
                return ret, log

            log_message("Starting to program.")
            ret, log = self.program()
            if ret is False:
                return ret, log

            log_message("Starting to do_fw_upg_upload")
            ret, log = self.do_fw_upg_upload()
            if ret is False:
                return ret, log

            log_message("Starting to download emtp to data")
            ret, log = self.do_download_emtp_to_data()
            if ret is False:
                return ret, log

            log_message("Starting to check crc")
            ret, log = self.check_crc()
            if ret is False:
                return ret, log

            msg = "Upgrade success"
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "do_fw_upg raise exception, errmsg: %s" % (str(e))
            log_message(msg)
            return False, msg
        finally:
            # set dev_available available
            log_message("Starting to set dev_available available")
            self.set_dev_available(True)

    def upgrade(self):
        print("+================================+")
        print("|  Doing upgrade, please wait... |")
        ret, log = self.do_fw_upg()
        if ret is True:
            print("|       upgrade succeeded!       |")
            print("+================================+")
            sys.exit(0)
        else:
            print("|        upgrade failed!         |")
            print("+================================+")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)


def do_xdpe122_fw_upg(file, bus, addr):
    firmware = Xdpe122Firmware(file, bus, addr)
    ret, log = firmware.do_fw_upg()
    return ret, log


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''Upgrade xdpe122 firmware script'''

# Cold upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def upgrade(firmware_file, bus, addr):
    '''xdpe122 firmware upgrade'''
    firmware = Xdpe122Firmware(firmware_file, bus, addr)
    firmware.upgrade()

if __name__ == "__main__":
    main()