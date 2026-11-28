#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import os
import sys
import click
import time
from platform_util import get_value, set_value
from platform_util import setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS


IC_DEVICE_ID_REG = 0xAD
IC_DEVICE_ID_SIZE = 4
IC_DEVICE_REV_REG = 0xAE

FW_UPG_OP_TYPE_WRITE = "write"
FW_UPG_OP_TYPE_CHECK = "check"
FW_UPG_IGNORE_HEX_CRC_LINE = 336
FW_UPG_LEGACY_HEX_CRC_LINE = 276
FW_UPG_PRODUCTION_HEX_CRC_LINE = 290

FW_UPG_MAX_CHIP_NVM_TIME = 28

# hexfile type define
FW_UPG_CHIP_ID_FAIL = 0        # Get ic_device_id failed */
FW_UPG_CHIP_ID_UNKNOWN = 1     # Get ic_device_id success, but unsupport to upgrade this chip id
FW_UPG_HEX_TYPE_IGNORE = 2     # Get ic_device_id success, and don't care about the hex file type/
FW_UPG_HEX_TYPE_FAIL = 3       # Get hex file type failed
FW_UPG_HEX_TYPE_UNKNOWN = 4    # Unknown hex file type
FW_UPG_HEX_TYPE_LEGACY = 5     # Legacy hex Files
FW_UPG_HEX_TYPE_PRODUCTION = 6 # Production hex Files


# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "renesas_fw_upg.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)
def log_message(message):
    logger.info(message)

def crc8_pec(data):
    """
    CRC-8 algorithm for SMBus PEC
    Polynomial: 0x07, Init: 0x00, XorOut: 0x00
    """
    poly = 0x07
    crc = 0x00

    for val in data:
        crc ^= val
        for i in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ poly) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def int_array_to_hex_string(int_array, reverse=False):
    """
    Concatenate an array of integers into a hexadecimal string prefixed with '0x'.

    Parameters:
    int_array (list): An array of integers, e.g., [0x12, 0x34, 0x56, 0x78]

    Returns:
    str: Concatenated hexadecimal string, e.g.
        normal: "0x12345678"
        reverse: "0x78563412"
    """
    log_message("int_array before: %s, reverse: %s" % (int_array, reverse))
    if reverse:
        int_array = int_array[::-1]  # reverse

    log_message("int_array after: %s, reverse: %s" % (int_array, reverse))

    hex_str = "0x" + "".join(f"{x:02X}" for x in int_array)
    return hex_str

class IntOrHexParamType(click.ParamType):
    name = "int_or_hex"
    def convert(self, value, param, ctx):
        try:
            # Automatically detect base: decimal, hex (0x..), octal (0o..), binary (0b..)
            return int(value, 0)
        except ValueError:
            self.fail(f"{value!r} is not a valid integer (supports decimal or 0x hex)", param, ctx)

INT_OR_HEX = IntOrHexParamType()


class RenesasFirmware:
    def __init__(self, firmware_file, bus, addr):
        self.firmware = firmware_file
        self.i2c_bus = bus
        self.i2c_addr = addr
        loc = "%d-%04x" % (bus, addr)
        self.fw_upg_dev_file = "/dev/renesas_%d_0x%02x" % (bus, addr)
        self.fw_upg_dev_available = "/sys/bus/i2c/devices/%s/dev_available" % loc
        self.chip_crc_path = "/sys/bus/i2c/devices/%s/chip_crc" % loc
        self.chip_nvm_time_path = "/sys/bus/i2c/devices/%s/chip_nvm_time" % loc
        self.hex_file_type_path = "/sys/bus/i2c/devices/%s/hex_file_type" % loc
        self.upg_status_path = "/sys/bus/i2c/devices/%s/upg_status" % loc
        self.bank_status_path = "/sys/bus/i2c/devices/%s/bank_status" % loc
        self.en_device_data_path = "/sys/bus/i2c/devices/%s/en_device_data" % loc
        self.mcu_fault_path = "/sys/bus/i2c/devices/%s/mcu_fault" % loc
        self.part_id_path = "/sys/bus/i2c/devices/%s/part_id" % loc
        self.cycling_vcc_path = "/sys/bus/i2c/devices/%s/cycling_vcc" % loc
        self.restore_cfg_path = "/sys/bus/i2c/devices/%s/restore_cfg" % loc
        self.ic_device_id_path = "/sys/bus/i2c/devices/%s/device_id" % loc
        self.ic_device_rev_path = "/sys/bus/i2c/devices/%s/ic_device_rev" % loc
        self.parsed_data = []
        self.hex_file_dev_id = 0
        self.ic_device_id = 0
        self.ic_device_rev = 0
        self.hex_file_version = 0
        self.hex_file_crc = 0

    def set_device_data(self, enable):
        if enable is True:
            value = 1
        else:
            value = 0
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.en_device_data_path,
            "value": value,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "set device data failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "set device data success, set value: %d" % value
        log_message(msg)
        return True, msg

    def cycling_vcc(self):
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.cycling_vcc_path,
            "value": 1,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "cycling vcc failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "cycling vcc success"
        log_message(msg)
        return True, msg

    def program(self):
        fd = -1
        if not os.path.exists(self.fw_upg_dev_file):
            msg = "%s not found, program failed" % self.fw_upg_dev_file
            log_message(msg)
            return False, msg
        try:
            fd = os.open(self.fw_upg_dev_file, os.O_WRONLY)
            for entry in self.parsed_data:
                if entry.get("operation") != "write":
                    continue
                reg_addr = entry.get("reg")
                os.lseek(fd, reg_addr, os.SEEK_SET)
                actual_data = entry.get("data")
                ret = os.write(fd, bytes(actual_data))
                if ret != len(actual_data):
                    msg = "os.write failed, write data: %s, ret: %s" % (actual_data, ret)
                    log_message(msg)
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
        }
        ret, chip_nvm_time = get_value(get_val_conf)
        if ret is False:
            msg = "get chip_nvm_time failed, msg: %s" % chip_nvm_time
            log_message(msg)
            return False, msg
        if chip_nvm_time <= 0 or chip_nvm_time > FW_UPG_MAX_CHIP_NVM_TIME:
            msg = "Invalid chip_nvm_time: %d" % chip_nvm_time
            log_message(msg)
            return False, msg
        msg = "check chip_nvm_time success, chip_nvm_time: %d" % chip_nvm_time
        log_message(msg)
        return True, msg

    def check_ic_device_id(self):
        """
        Read IC_DEVICE_ID from Device
        This value must match the data shown in the Device Table
        """
        log_message("Starting to check ic_device_id")
        if self.ic_device_id != self.hex_file_dev_id:
            msg = ("check ic_device_id failed, ic_device_id read: 0x%08x, hex_file_dev_id: 0x%08x" %
                (self.ic_device_id, self.hex_file_dev_id))
            log_message(msg)
            return False, msg
        msg = "check ic device id success, ic_device_id: 0x%08x" % self.ic_device_id
        log_message(msg)
        return True, msg

    def check_ic_device_rev(self):
        """
        Read IC_DEVICE_REV from Device.
        The device revision number must be 6.0.0.0 or greater.
        If not, contact Renesas for support
        """
        log_message("Starting to check ic_device_rev")
        ic_device_rev_msb = (self.ic_device_rev >> 24) & 0xFF
        if ic_device_rev_msb < 6:
            msg = ("ic_device_rev: 0x%08x, not support" % self.ic_device_rev)
            log_message(msg)
            return False, msg
        msg = "check ic_device_rev success, ic_device_rev: 0x%08x" % self.ic_device_rev
        log_message(msg)
        return True, msg

    def check_hex_file_compatibility(self):
        log_message("Starting to check hex file compatibility")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.hex_file_type_path,
            "int_decode": 10,
        }
        ret, hex_file_type = get_value(get_val_conf)
        if ret is False:
            msg = "get hex_file_type failed, msg: %s" % hex_file_type
            log_message(msg)
            return False, msg
        msg = "get hex_file_type success, value: %d" % hex_file_type
        log_message(msg)

        if hex_file_type == FW_UPG_CHIP_ID_FAIL:
            msg = "hex_file_type: %d, read chip id failed" % hex_file_type
            log_message(msg)
            return False, msg

        if hex_file_type == FW_UPG_CHIP_ID_UNKNOWN:
            msg = "hex_file_type: %d, unknown chip id: 0x%08x" % (hex_file_type, self.hex_file_dev_id)
            log_message(msg)
            return False, msg

        if hex_file_type == FW_UPG_HEX_TYPE_IGNORE:
            # get hex file crc value
            if len(self.parsed_data) < FW_UPG_IGNORE_HEX_CRC_LINE:
                msg = ("Can't get hex file crc value, hex_file_type: %d, parsed_data len: %d" %
                    (hex_file_type, len(self.parsed_data)))
                log_message(msg)
                return False, msg
            actual_data = self.parsed_data[FW_UPG_IGNORE_HEX_CRC_LINE-1].get("data")
            hex_file_crc_str = int_array_to_hex_string(actual_data, True)
            self.hex_file_crc = int(hex_file_crc_str, 16)
            msg = ("hex_file_type: %d, ignore hex file type, hex file crc: 0x%08x" %
                (hex_file_type, self.hex_file_crc))
            log_message(msg)
            return True, msg

        if hex_file_type == FW_UPG_HEX_TYPE_FAIL:
            msg = "hex_file_type: %d, get hex file type failed" % (hex_file_type)
            log_message(msg)
            return False, msg

        if hex_file_type == FW_UPG_HEX_TYPE_UNKNOWN:
            msg = "hex_file_type: %d, unknown hex file type" % (hex_file_type)
            log_message(msg)
            return False, msg

        if hex_file_type == FW_UPG_HEX_TYPE_LEGACY:
            # check_ic_device_rev
            ret, log = self.check_ic_device_rev()
            if ret is False:
                return ret, log

            # check hex file version,
            # Legacy HEX Files, IC_DEVICE_REV MSB from the HEX file must be 0x00 or 0x01.
            if self.hex_file_version != 0 and self.hex_file_version != 1:
                msg = ("Hex file not match, hex_file_type: %d, Legacy HEX File: %d" %
                    (hex_file_type, self.hex_file_version))
                log_message(msg)
                return False, msg
            msg = ("Hex file match, hex_file_type: %d, IC_DEVICE_REV MSB from the HEX file: %d" %
                    (hex_file_type, self.hex_file_version))
            log_message(msg)

            # get hex file crc value
            if len(self.parsed_data) < FW_UPG_LEGACY_HEX_CRC_LINE:
                msg = ("Can't get legacy hex file crc value, hex_file_type: %d, parsed_data len: %d" %
                    (hex_file_type, len(self.parsed_data)))
                log_message(msg)
                return False, msg
            actual_data = self.parsed_data[FW_UPG_LEGACY_HEX_CRC_LINE-1].get("data")
            hex_file_crc_str = int_array_to_hex_string(actual_data, True)
            self.hex_file_crc = int(hex_file_crc_str, 16)
            msg = ("hex_file_type: %d, legacy hex file type, hex file crc: 0x%08x" %
                (hex_file_type, self.hex_file_crc))
            log_message(msg)
            return True, msg

        if hex_file_type == FW_UPG_HEX_TYPE_PRODUCTION:
            # check_ic_device_rev
            ret, log = self.check_ic_device_rev()
            if ret is False:
                return ret, log

            # check hex file version
            # Production HEX File, IC_DEVICE_REV MSB from the HEX file must be 0x06 or greate
            if self.hex_file_version < 6:
                msg = ("Hex file not match, hex_file_type: %d, IC_DEVICE_REV MSB from the HEX file: %d" %
                    (hex_file_type, self.hex_file_version))
                log_message(msg)
                return False, msg
            msg = ("Hex file match, hex_file_type: %d, IC_DEVICE_REV MSB from the HEX file: %d" %
                    (hex_file_type, self.hex_file_version))
            log_message(msg)

            # get hex file crc value
            if len(self.parsed_data) < FW_UPG_PRODUCTION_HEX_CRC_LINE:
                msg = ("Can't get production hex file crc value, hex_file_type: %d, parsed_data len: %d" %
                    (hex_file_type, len(self.parsed_data)))
                log_message(msg)
                return False, msg
            actual_data = self.parsed_data[FW_UPG_PRODUCTION_HEX_CRC_LINE-1].get("data")
            hex_file_crc_str = int_array_to_hex_string(actual_data, True)
            self.hex_file_crc = int(hex_file_crc_str, 16)
            msg = ("hex_file_type: %d, production hex file type, hex file crc: 0x%08x" %
                (hex_file_type, self.hex_file_crc))
            log_message(msg)
            return True, msg
        # Unknown hex_file_type
        msg = "Unknown hex file type: %d" % hex_file_type
        log_message(msg)
        return False, msg

    def check_programmer_status(self):
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.upg_status_path,
        }
        time.sleep(2)
        ret, upg_status = get_value(get_val_conf)
        if ret is False:
            msg = "get programmer status failed, msg: %s" % upg_status
            log_message(msg)
            return False, msg

        msg = "get programmer status success, value: 0x%08x" % upg_status
        log_message(msg)

        err_cnt = 0
        # If this bit is 0, programming timeout.
        if upg_status & (1 << 0) == 0:
            log_message("Programming timeout")
            err_cnt +=1

        # If bit 1 is 1, programming has failed.
        if upg_status & (1 << 1) == 1:
            log_message("Programming has failed")
            err_cnt +=1

        # If bit 2 is 1, the HEX file contains more configurations than are available.
        if upg_status & (1 << 2) == 1:
            log_message("HEX file contains more configurations than are available")
            err_cnt +=1

        # If bit 3 is 1 a CRC mismatch exists within the configuration data.
        # Programming fails before OTP banks are consumed.
        if upg_status & (1 << 3) == 1:
            log_message("A CRC mismatch exists within the configuration data")
            log_message("Programming fails before OTP banks are consumed.")
            err_cnt +=1

        # If bit 4 is 1, the CRC check fails on the OTP memory.
        # Programming fails after OTP banks are consumed.
        if upg_status & (1 << 4) == 1:
            log_message("The CRC check fails on the OTP memory.")
            log_message("Programming fails after OTP banks are consumed.")
            err_cnt +=1

        # If bit 5 is 1, programming has failed.
        # Programming fails afterOTP banks are consumed.
        if upg_status & (1 << 5) == 1:
            log_message("Programming has failed")
            log_message("Programming fails after OTP banks are consumed.")
            err_cnt +=1

        if err_cnt > 0:
            return False, "Programming status check failed"
        msg = "programmer status check ok, value: 0x%08x" % upg_status
        log_message(msg)
        return True, msg

    def record_bank_status(self):
        """
        Read BANK_STATUS Register
        Read the BANK_STATUS through DMA from address 0x007F
        to retrieve status information for programming verification.
        Record this value for future reference as proof of successful programming.
        """
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.bank_status_path,
        }
        ret, bank_status = get_value(get_val_conf)
        if ret is False:
            msg = "get bank status failed, msg: %s" % bank_status
            log_message(msg)
            return False, msg
        msg = "get bank status success, value: 0x%08x" % bank_status
        log_message(msg)
        return True, msg

    def restore_cfg(self):
        """
        Use RESTORE_CFG Command to set Configuration ID
        """
        set_val_conf = {
            "gettype": "sysfs",
            "loc": self.restore_cfg_path,
            "value": 0, # only support Configuration ID 0
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "RESTORE_CFG set Configuration ID 0 failed, msg: %s" % log
            log_message(msg)
            return False, msg
        msg = "RESTORE_CFG set Configuration ID 0 success"
        log_message(msg)
        return True, msg

    def check_crc(self):
        """
        Retrieve CRC Value
        Read the CRC value through DMA from address 0x0094
        to verify the configuration saved at the selected Configuration ID.
        """
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.chip_crc_path,
        }
        ret, chip_crc = get_value(get_val_conf)
        if ret is False:
            msg = "get chip crc failed, msg: %s" % chip_crc
            log_message(msg)
            return False, msg
        if chip_crc != self.hex_file_crc:
            msg = "CRC not match, chip_crc: 0x%08x, hex_file_crc: 0x%08x" % (chip_crc, self.hex_file_crc)
            log_message(msg)
            return False, msg
        msg = "CRC check ok, chip_crc: 0x%08x, hex_file_crc: 0x%08x" % (chip_crc, self.hex_file_crc)
        log_message(msg)
        return True, msg

    def record_mcuflt(self):
        """
        Retrieve MCUFLT value
        Read the MCUFLT value through DMA from address 0xEC01 to check for further errors.
        Record this value for logging purposes.
        This value will be 0x00000000 if there are no errors.
        Record this value for logging purposes
        """
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.mcu_fault_path,
        }
        ret, mcu_status = get_value(get_val_conf)
        if ret is False:
            msg = "get MCU fault status failed, msg: %s" % mcu_status
            log_message(msg)
            return False, msg
        msg = "get MCU fault status success, value: 0x%08x" % mcu_status
        log_message(msg)
        return True, msg

    def record_part_id(self):
        """
        Retrieve Part ID
        Read the Part ID through DMA from address 0x00E8. Record this value for logging purposes.
        """
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.part_id_path,
        }
        ret, part_id = get_value(get_val_conf)
        if ret is False:
            msg = "get part id failed, msg: %s" % part_id
            log_message(msg)
            return False, msg
        msg = "get part id success, value: 0x%08x" % part_id
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

    def parse_register_data(self):
        try:
            if not os.path.isfile(self.firmware):
                msg = "%s not found" % self.firmware
                log_message(msg)
                return False, msg
            line_num = 0
            # Read data from file
            with open(self.firmware, 'r') as fd:
                for line in fd:
                    line_num += 1
                    line = line.strip()
                    # Create bytes data from every two characters,
                    # eg: hex file origin data:4907C4AD49D29B008B
                    # bytes_data = ['49', '07', 'C4', 'AD', '49', 'D2', '9B', '00', '8B']
                    bytes_data = [line[i:i+2] for i in range(0, len(line), 2)]

                    # Check if the first byte is '00' or '49'
                    if bytes_data[0] not in ['00', '49']:
                        msg = "Line number: %d, invalid header file record type: %s" % (line_num, bytes_data[0])
                        log_message(msg)
                        return False, msg

                    if bytes_data[0] == '00':
                        operation = FW_UPG_OP_TYPE_WRITE
                    else:
                        operation = FW_UPG_OP_TYPE_CHECK
                    data_count = int(bytes_data[1], 16)  # Data count
                    data_count_indeed = len(bytes_data) - 2 # Does not include the Header file record type and data_count
                    if data_count != data_count_indeed:
                        msg =("Line number: %d, invalid data_count. data_count read: %d, data_count_indeed: %d" %
                            (line_num, data_count, data_count_indeed))
                        log_message(msg)
                        return False, msg
                    address = int(bytes_data[2], 16)  # Address as integer
                    reg = int(bytes_data[3], 16)
                    actual_data = [int(data, 16) for data in bytes_data[4:-1]]
                    pec = int(bytes_data[-1], 16)  # PEC
                    crc_data = [address, reg] + actual_data
                    calc_pec = crc8_pec(crc_data)
                    if calc_pec != pec:
                        msg =("Line number: %d, PEC check error, pec calc: 0x%02x, pec read: 0x%02x" %
                            (line_num, calc_pec, pec))
                        log_message(msg)
                        return False, msg
                    if reg == IC_DEVICE_ID_REG:
                        if len(actual_data) != IC_DEVICE_ID_SIZE:
                            msg = "Line number: %d, Invalid device id len: %d" % (line_num, len(actual_data))
                            log_message(msg)
                            return False, msg
                        self.hex_file_dev_id = int(int_array_to_hex_string(actual_data), 16)
                        log_message("Line number: %d, hex_file_dev_id: 0x%08x" % (line_num, self.hex_file_dev_id))
                    elif reg == IC_DEVICE_REV_REG:
                        self.hex_file_version = actual_data[0]
                        log_message("Line number: %d, hex_file_version: %d" % (line_num, self.hex_file_version))

                    # Store parsed result in dictionary
                    self.parsed_data.append({
                        'operation': operation,
                        'data_count': data_count,
                        'address': address,
                        'reg': reg,
                        'data': actual_data,
                        'pec': pec
                    })
            msg = "parse hex file data success, line number: %d" % line_num
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

        # check id match
        ret, log = self.check_ic_device_id()
        if ret is False:
            return ret, log

        # check hex version match
        ret, log = self.check_hex_file_compatibility()
        if ret is False:
            return ret, log
        return True, "fw_upg_init_check success"

    def init_get_ic_device_id(self):
        # get ic_device_id
        log_message("Starting to get ic_device_id")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.ic_device_id_path,
        }
        ret, ic_device_id = get_value(get_val_conf)
        if ret is False:
            msg = "get ic_device_id failed, msg: %s" % ic_device_id
            log_message(msg)
            return False, msg
        msg = "get ic_device_id success, value: 0x%08x" % ic_device_id
        log_message(msg)
        self.ic_device_id = ic_device_id
        return True, msg

    def init_get_ic_device_rev(self):
        # get ic_device_rev
        log_message("Starting to get ic_device_rev")
        get_val_conf = {
            "gettype": "sysfs",
            "loc": self.ic_device_rev_path,
        }
        ret, ic_device_rev = get_value(get_val_conf)
        if ret is False:
            msg = "get ic_device_rev failed, msg: %s" % ic_device_rev
            log_message(msg)
            return False, msg
        msg = "get ic_device_rev success, value: 0x%08x" % ic_device_rev
        log_message(msg)
        self.ic_device_rev = ic_device_rev
        return True, msg

    def init_get_chip_info(self):
        ret, log = self.init_get_ic_device_id()
        if ret is False:
            return ret, log

        ret, log = self.init_get_ic_device_rev()
        if ret is False:
            return ret, log

        msg = "init_get_chip_info success"
        log_message(msg)
        return True, msg

    def do_fw_refresh(self):
        try:
            log_message("Starting to cycling vcc")
            ret, log = self.cycling_vcc()
            if ret is False:
                return ret, log

            # After cycling vcc, sleep 5s
            time.sleep(5)

            log_message("Starting to enable device data.")
            ret, log = self.set_device_data(True)
            if ret is False:
                return ret, log

            log_message("Starting to set Configuration ID")
            ret, log = self.restore_cfg()
            if ret is False:
                return ret, log

            log_message("Starting to check CRC Value.")
            ret, log = self.check_crc()
            if ret is False:
                return ret, log

            log_message("Starting to record MCU fault status.")
            ret, log = self.record_mcuflt()
            if ret is False:
                return ret, log

            log_message("Starting to restore device Data.")
            ret, log = self.set_device_data(False)
            if ret is False:
                return ret, log

            log_message("Starting to set Configuration ID again")
            ret, log = self.restore_cfg()
            if ret is False:
                return ret, log

            log_message("Starting to check CRC Value again")
            ret, log = self.check_crc()
            if ret is False:
                return ret, log

            log_message("Starting to record MCU fault status again")
            ret, log = self.record_mcuflt()
            if ret is False:
                return ret, log

            log_message("Starting to record part id")
            ret, log = self.record_part_id()
            if ret is False:
                return ret, log
            msg = "refresh success"
            log_message(msg)
            return True, msg
        except Exception as e:
            msg = "do_fw_refresh raise exception, errmsg: %s" % (str(e))
            log_message(msg)
            return False, msg


    def do_fw_upg(self):
        try:
            ret, log = self.parse_register_data()
            if ret is False:
                return ret, log

            # init get chip info
            log_message("Starting to get chip info")
            ret, log = self.init_get_chip_info()
            if ret is False:
                return ret, log

            # set dev_available not available
            log_message("Starting to set dev_available not available")
            ret, log = self.set_dev_available(False)
            if ret is False:
                return ret, log

            log_message("Starting to do_fw_upg_init_check")
            ret, log = self.do_fw_upg_init_check()
            if ret is False:
                return ret, log

            # check crc
            ret, log = self.check_crc()
            if ret is True:
                msg = "device crc is equal to file crc, skip upgrade"
                log_message(msg)
                return True, msg

            log_message("Starting to program.")
            ret, log = self.program()
            if ret is False:
                return ret, log

            log_message("Starting to check programmer status")
            ret, log = self.check_programmer_status()
            if ret is False:
                return ret, log

            log_message("Starting to record bank status")
            ret, log = self.record_bank_status()
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


def do_renesas_fw_upg(file, bus, addr):
    firmware = RenesasFirmware(file, bus, addr)
    ret, log = firmware.do_fw_upg()
    return ret, log


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''Upgrade renesas firmware script'''

# Cold upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def upgrade(firmware_file, bus, addr):
    '''renesas firmware upgrade'''
    firmware = RenesasFirmware(firmware_file, bus, addr)
    firmware.upgrade()

if __name__ == "__main__":
    main()