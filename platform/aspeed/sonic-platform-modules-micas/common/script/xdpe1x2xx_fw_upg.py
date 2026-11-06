#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import os
import sys
import click
import logging
import time
from platform_util import get_value, set_value, calculate_data_crc32
from platform_util import setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS

FW_UPG_UPLOAD_RETRY = 3
MAX_SOAK_TIME = 2 #2s
FW_DATA_PER_WRITE_LEN = 4

DEBUG_FILE = "/etc/.xdpe1x2xx_fw_upg_debug_flag"

# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "xdpe1x2xx_fw_upg.log"
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


def fw_upg_hex_to_bytes(hex_str):
    # Convert an hex string to 4 bytes in little-endian order.
    # e.g. "00000004" -> b'\x04\x00\x00\x00'
    # "02A402A4" -> b'\xa4\x02\xa4\x02'
    raw = bytes.fromhex(hex_str)
    val = raw[::-1]
    return val


class Xdpe1x2xxFirmware:
    def __init__(self, firmware_file, bus, addr):
        self.firmware = firmware_file
        self.i2c_bus = bus
        self.i2c_addr = addr
        loc = "%d-%04x" % (bus, addr)
        self.fw_upg_dev_file = "/dev/xdpe1x2xx_%d_0x%02x" % (bus, addr)
        self.fw_upg_dev_available = "/sys/bus/i2c/devices/%s/dev_available" % loc
        self.chip_crc_path = "/sys/bus/i2c/devices/%s/chip_crc" % loc
        self.otp_remain_space = "/sys/bus/i2c/devices/%s/remain_space" % loc
        self.otp_section_invalidate = "/sys/bus/i2c/devices/%s/otp_section_invalidate" % loc
        self.upload_to_otp = "/sys/bus/i2c/devices/%s/upload_to_otp" % loc
        self.clear_faults = "/sys/bus/i2c/devices/%s/clear_faults" % loc
        self.status0_cml = "/sys/bus/i2c/devices/%s/status0_cml" % loc
        self.status1_cml = "/sys/bus/i2c/devices/%s/status1_cml" % loc
        self.scap_addr = "/sys/bus/i2c/devices/%s/scap_addr" % loc
        self.parsed_data = []
        self.file_crc = 0
        self.scap_addr_val = 0
    '''
    def write_section_data_test(self, fd, data_byte):
        offset = 0
        length = len(data_byte)
        if (length % FW_DATA_PER_WRITE_LEN) != 0:
            msg = "Invalid date byte len: %d" % length
            log_message(msg)
            return False, msg

        while offset < length:
            chunk = data_byte[offset:offset + FW_DATA_PER_WRITE_LEN]
            bytes_written = os.write(fd, chunk)
            if bytes_written != FW_DATA_PER_WRITE_LEN:
                msg = "write failed, offset: 0x%x, ret: %s" % (offset, bytes_written)
                log_message(msg)
                return False, msg
            offset += bytes_written
        return True, "write section_data success"

    def write_data_test(self):
        fd = -1
        try:
            file = "/tmp/xdpe1x2xxx_fw.bin"
            fd = os.open(file, os.O_WRONLY|os.O_CREAT|os.O_TRUNC, 0o644)
            if fd < 0:
                msg = "os.open %s failed, fd: %d" % (file, fd)
                log_message(msg)
                return False, msg
            os.lseek(fd, 0, os.SEEK_SET)
            for section in self.parsed_data:
                section_name = section["name"]
                log_message("Starting to write %s section data..." % section_name)
                ret, msg = self.write_section_data_test(fd, section["data_byte"])
                if ret is False:
                    return False, msg
                log_message("Writing %s section data success" % section_name)
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
    '''

    def check_chip_crc(self):
        log_message("Starting to check chip crc")
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
            msg = "check chip crc failed, chip_crc: 0x%08x, file_crc: 0x%08x" % (chip_crc, self.file_crc)
            log_message(msg)
            return False, msg
        msg = "check chip crc success, chip_crc: 0x%08x, file_crc: 0x%08x" % (chip_crc, self.file_crc)
        log_message(msg)
        return True, msg

    def check_cml_fault(self, page):
        log_message("Starting to check status%d cml fault" % page)
        if page == 0:
            loc = self.status0_cml
        else:
            loc = self.status1_cml
        get_val_conf = {
            "gettype": "sysfs",
            "loc": loc,
        }
        ret, status = get_value(get_val_conf)
        if ret is False:
            msg = "get status%d cml failed, msg: %s" % (page, status)
            log_message(msg)
            return False, msg
        # bit0 means Other Memory Or Logic Fault
        if (status & 0x01) == 0x01:
            msg = "status%d cml check error, value: 0x%02x, Other Memory Or Logic Fault" % (page, status)
            log_message(msg)
            return False, msg
        msg = "status%d cml check ok, value: 0x%02x" % (page, status)
        log_message(msg)
        return True, msg

    def check_cml_faults(self):
        ret, msg = self.check_cml_fault(0)
        if ret is False:
            return False, msg

        ret, msg = self.check_cml_fault(1)
        if ret is False:
            return False, msg

        msg = "check_cml_faults ok"
        log_message(msg)
        return True, msg

    def do_upload_section_data_to_otp(self, section_name, size):
        log_message("Starting to upload %s section data to otp" % section_name)
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.upload_to_otp,
            "value": size,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "upload %s section data to otp failed, size: 0x%x, msg: %s" % (section_name, size, log)
            log_message(msg)
            return False, msg
        time.sleep(MAX_SOAK_TIME)
        msg = "upload %s section data to otp success, size: 0x%x" % (section_name, size)
        log_message(msg)
        return True, msg

    def do_clear_faults(self):
        log_message("Starting to clear faults")
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.clear_faults,
            "value": 1,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "clear faults failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "clear faults success"
        log_message(msg)
        return True, msg

    def program_data(self, fd, section_name,   data_byte):
        log_message("Starting to program %s data, seek_offset: 0x%x" % (section_name, self.scap_addr_val))

        length = len(data_byte)
        if (length % FW_DATA_PER_WRITE_LEN) != 0:
            msg = "Invalid date byte len: %d" % length
            log_message(msg)
            return False, msg

        try:
            os.lseek(fd, self.scap_addr_val, os.SEEK_SET)
            data_offset = 0
            while data_offset < length:
                chunk = data_byte[data_offset:data_offset + FW_DATA_PER_WRITE_LEN]
                bytes_written = os.write(fd, chunk)
                if bytes_written != FW_DATA_PER_WRITE_LEN:
                    msg = "write failed, data offset: 0x%x, ret: %s" % (data_offset, bytes_written)
                    log_message(msg)
                    return False, msg
                data_offset += bytes_written
            msg = "program %s data success" % section_name
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "program %s data raise exception, errmsg: %s" % (section_name, str(e))
            log_message(msg)
            return False, msg

    def set_otp_section_invalidate(self, section):
        section_name = section["name"]
        wr_value = section["header_data"] & 0xFFFF
        log_message("Starting to set %s otp section invalidate, write value: 0x%04x" % (section_name, wr_value))
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.otp_section_invalidate,
            "value": wr_value,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = ("set %s otp section invalidate failed, msg: %s" % (section_name, log))
            log_message(msg)
            return False, msg
        msg = ("set %s otp section invalidate success" % section_name)
        log_message(msg)
        return True, msg

    def program_section_data_once(self, fd, section):
        section_name = section["name"]
        data_byte = section["data_byte"]
        section_size = section["section_size"]

        # set otp section invalidate
        ret, msg = self.set_otp_section_invalidate(section)
        if ret is False:
            return False, msg

        # program section data
        ret, msg = self.program_data(fd, section_name, data_byte)
        if ret is False:
            return False, msg

        # clear faults
        ret, msg = self.do_clear_faults()
        if ret is False:
            return False, msg

        # upload section data to otp
        ret, msg = self.do_upload_section_data_to_otp(section_name, section_size)
        if ret is False:
            return False, msg

        # check cml fault
        ret, msg = self.check_cml_faults()
        if ret is False:
            return False, msg

        return True, "program section data success"

    def program_section_data(self, fd, section):
        for i in range(FW_UPG_UPLOAD_RETRY):
            ret, log = self.program_section_data_once(fd, section)
            if ret is True:
                msg = "program_section_data success, retry: %d" % i
                log_message(msg)
                return True, msg
        msg = "program_section_data retry failed"
        log_message(msg)
        return False, msg

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
            for section in self.parsed_data:
                ret, msg = self.program_section_data(fd, section)
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

    def get_scap_addr(self):
        log_message("Starting to get scratchpad address")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.scap_addr,
        }
        ret, scap_addr = get_value(get_val_conf)
        if ret is False:
            msg = "get scratchpad address failed, msg: %s" % scap_addr
            log_message(msg)
            return False, msg
        self.scap_addr_val = scap_addr
        msg = "get scratchpad address success, value: 0x%04x" % self.scap_addr_val
        log_message(msg)
        return True, msg

    def check_otp_remain_space(self):
        log_message("Starting to check otp remain space")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.otp_remain_space,
        }
        ret, remain_space = get_value(get_val_conf)
        if ret is False:
            msg = "get otp remain space failed, msg: %s" % remain_space
            log_message(msg)
            return False, msg
        msg = "get otp remain space success, value: 0x%04x" % remain_space
        log_message(msg)
        fw_size = 0
        for section in self.parsed_data:
            fw_size += section["section_size"]
        if remain_space < fw_size:
            msg = "Not enough remain space, remain_space: 0x%x, firmware file size: 0x%x" % (remain_space, fw_size)
            log_message(msg)
            return False, msg
        msg = "remain space check ok, remain_space: 0x%x, firmware file size: 0x%x" % (remain_space, fw_size)
        log_message(msg)
        return True, msg

    def do_fw_upg_init_check(self):
        # record otp remain space
        ret, log = self.check_otp_remain_space()
        if ret is False:
            return ret, log

        # get scratchpad address
        ret, log = self.get_scap_addr()
        if ret is False:
            return ret, log

        return True, "fw_upg_init_check success"

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

    def check_file_crc(self):
        total_crc_calc = 0
        try:
            for section in self.parsed_data:
                section_name = section["name"]
                # check header_crc
                header_data = bytearray()
                header_crc_file = section["header_crc"]
                header_data += fw_upg_hex_to_bytes(section["data"][0])
                header_data += fw_upg_hex_to_bytes(section["data"][1])
                header_crc_calc = calculate_data_crc32(header_data)
                if header_crc_calc != header_crc_file:
                    msg = ("Check %s section header crc failed, header_crc_file: 0x%x, header_crc_calc: 0x%x" %
                        (section_name, header_crc_file, header_crc_calc))
                    log_message(msg)
                    return False, msg
                msg = "Check %s section header crc success, header crc: 0x%x" % (section_name, header_crc_file)
                log_message(msg)
                # check data crc
                data_list = section["data"][3:-1]
                data_crc_file = section["data_crc"]
                data = bytearray()
                for value in data_list:
                    data += fw_upg_hex_to_bytes(value)
                data_crc_calc = calculate_data_crc32(data)
                if data_crc_calc != data_crc_file:
                    msg = ("Check %s section data crc failed, data_crc_file: 0x%x, data_crc_calc: 0x%x" %
                        (section_name, data_crc_file, data_crc_calc))
                    log_message(msg)
                    return False, msg
                msg = "Check %s section data crc success, data crc: 0x%x" % (section_name, data_crc_file)
                log_message(msg)
                total_crc_calc += header_crc_file
                total_crc_calc += data_crc_file
                header_crc_byte = fw_upg_hex_to_bytes(section["data"][2])
                data_crc_byte = fw_upg_hex_to_bytes(section["data"][-1])
                section["data_byte"] = header_data + header_crc_byte + data + data_crc_byte
            # check total crc
            total_crc = total_crc_calc & 0xFFFFFFFF
            if total_crc != self.file_crc:
                msg = "Check file crc failed, file crc read: 0x%x, file crc calc: 0x%x" % (self.file_crc, total_crc)
                log_message(msg)
                return False, msg
            msg = "check file crc success, crc value: 0x%x" % self.file_crc
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "check_file_crc raise exception, errmsg: %s" % (str(e))
            log_message(msg)
            return False, msg

    def parse_upg_file(self):
        try:
            if not os.path.isfile(self.firmware):
                msg = "%s not found" % self.firmware
                log_message(msg)
                return False, msg
            line_num = 0
            find_section_data = False
            section_first_line = False
            with open(self.firmware, 'r') as fd:
                for line in fd:
                    line_num += 1
                    line = line.strip()
                    if "End Configuration Data" in line:
                        log_message("Line number: %d, End Configuration Data" % line_num)
                        break
                    if "Configuration Checksum" in line:
                        self.file_crc = int(line.split(':', 1)[1].strip(), 16)
                        log_message("Line number: %d, file crc: 0x%08x" % (line_num, self.file_crc))
                        continue
                    if "//XV0" in line and "trim" not in line.lower():
                        section_conf = {}
                        section_name = line.split(" ", 1)[1].strip()
                        section_conf["name"] = section_name
                        section_conf["data"] = []
                        section_first_line = True
                        find_section_data = True
                        self.parsed_data.append(section_conf)
                        continue
                    if find_section_data is True:
                        section_datas =  line.split(" ")
                        datas = section_datas[1 : ]
                        log_debug("Line number: %d, data: %s" % (line_num, datas))
                        section_conf["data"] += datas
                        if section_first_line is True:
                            section_first_line = False
                            header_data = int(section_datas[1], 16)
                            section_conf["header_data"] = header_data
                            section_conf["loop"] = (header_data >> 24) & 0xFF
                            section_conf["cmd"] = (header_data >> 16) & 0xFF
                            section_conf["xv"] = (header_data >> 8) & 0xFF
                            section_conf["header_code"] = header_data & 0xFF
                            section_size = int(section_datas[2], 16) & 0xffff
                            section_conf["section_size"] = section_size
                            header_crc = int(section_datas[3], 16)
                            section_conf["header_crc"] = header_crc

            if len(self.parsed_data) == 0:
                msg = "parse %s failed, can't find section data" % self.firmware
                log_message(msg)
                return False, msg

            for section in self.parsed_data:
                section["data_crc"] = int(section["data"][-1], 16)
                log_message("section name  : %s" % section["name"])
                log_message("  header_data : 0x%08x" % section["header_data"])
                log_message("  loop        : 0x%02x" % section["loop"])
                log_message("  cmd         : 0x%02x" % section["cmd"])
                log_message("  xv          : 0x%02x" % section["xv"])
                log_message("  header_code : 0x%02x" % section["header_code"])
                log_message("  section_size: 0x%04x" % section["section_size"])
                log_message("  header_crc  : 0x%08x" % section["header_crc"])
                log_message("  data_crc    : 0x%08x" % section["data_crc"])
            msg = "parse upgrade file success, line number: %d" % line_num
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "parse %s failed, Line number: %d, errmsg: %s" % (self.firmware, line_num, str(e))
            log_message(msg)
            return False, msg

    def do_fw_upg(self):
        try:
            debug_init()

            ret, log = self.parse_upg_file()
            if ret is False:
                return ret, log

            ret, log = self.check_file_crc()
            if ret is False:
                return ret, log

            # set dev_available not available
            log_message("Starting to set dev_available not available")
            ret, log = self.set_dev_available(False)
            if ret is False:
                return ret, log

            log_message("Starting to check device crc")
            ret, log = self.check_chip_crc()
            if ret is True:
                msg = "device crc is equal to file crc, skip upgrade"
                log_message(msg)
                return True, msg

            log_message("Starting to do_fw_upg_init_check")
            ret, log = self.do_fw_upg_init_check()
            if ret is False:
                return ret, log

            log_message("Starting to program.")
            ret, log = self.program()
            if ret is False:
                return ret, log

            #log_message("Starting to check crc")
            #ret, log = self.check_chip_crc()
            #if ret is False:
            #    return ret, log

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


def do_xdpe1x2xx_fw_upg(file, bus, addr):
    firmware = Xdpe1x2xxFirmware(file, bus, addr)
    ret, log = firmware.do_fw_upg()
    return ret, log


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''Upgrade xdpe1x2xx firmware script'''

# Cold upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def upgrade(firmware_file, bus, addr):
    '''xdpe1x2xx firmware upgrade'''
    firmware = Xdpe1x2xxFirmware(firmware_file, bus, addr)
    firmware.upgrade()

if __name__ == "__main__":
    main()
