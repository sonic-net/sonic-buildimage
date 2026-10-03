#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import sys
import os
import click
import signal
import logging
import time
from platform_util import set_value, get_value, setup_logger, parse_file_head, do_fw_upg_raw_file_generate, BSP_COMMON_LOG_DIR
from platform_config import UPGRADE_SUMMARY
from public.platform_common_config import S3IP_SYSFS_NAME

ERR_PSU_FW_HEAD_PARSE = -801
ERR_PSU_FW_HEAD_CHECK = -802

I2C_SMBUS_BLOCK_MAX = 32
PSU_STATUS = 0 
PSU_INPUT_STATUS = 1
PSU_PRESENT = 1
PSU_FW_RETRY_COUNT_MAX = 3
FORCE_UPGRADE = 1

# common psu upgrade PMBus command
MFR_HW_COMPATIBILITY = 0xD4
MFR_FW_UPLOAD_CAPABILITY = 0xD5
MFR_FWUPLOAD_MODE = 0xD6
MFR_FWUPLOAD = 0xD7
MFR_FWUPLOAD_STATUS = 0xD8

# MFR_FWUPLOAD_MODE write value
MFR_FWUPLOAD_MODE_EN = 1
MFR_FWUPLOAD_MODE_EXIT = 0

# MFR_FWUPLOAD read value mask
MFR_FWUPLOAD_RCV_IMG_SUCCESS = 0x01
MFR_FWUPLOAD_RCV_IMG_NOT_YET = 0x02
MFR_FWUPLOAD_RCV_IMG_BAD = 0x04
MFR_FWUPLOAD_RCV_IMG_NOT_SUPPORT = 0x10

# psu upgrade delay time
PSU_FW_ENTER_UPLOAD_MODE_DELAY_TIME_S = 1
PSU_FW_EXIT_UPLOAD_MODE_DELAY_TIME_S = 5
PSU_FW_FIRST_BLOCK_DELAY_TIME_S = 15
PSU_FW_CHECKSUM_DELAY_TIME_S = 3

DEBUG_FILE = "/etc/.psu_upgrade_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "psu_upgrade_debug.log"
logger = setup_logger(LOG_FILE)

# PSU models that support firmware upgrade
# Key: (vendor, model)
# Value: real model name
PSU_MODEL_MAP = {
    ('Great Wall', 'CRPS2700T2W'): 'CRPS2700T2W',
    ('Great Wall', 'CRPS3200T2W'): 'CRPS3200T2W',
    ('APLUSPOWER', 'AP-CA2700F12DT'): 'AP-CA2700F12DT',
    ('APLUSPOWER', 'AP-CA3200F12DT'): 'AP-CA3200F12DT',
    ('Great Wall', 'CRPS2700D2'): 'CRPS2700D2',
    ('APLUSPOWER', 'AP-CA2700F12SA'): 'AP-CA2700F12SA',
    ('Great Wall', 'CRPS3000CL'): 'CRPS3000CL',
}

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def psu_upg_error(s):
    logger.error(s)

def psu_upg_debug(s):
    logger.debug(s)

def signal_init():
    signal.signal(signal.SIGINT, signal.SIG_IGN)  # ignore ctrl+c signal
    signal.signal(signal.SIGTERM, signal.SIG_IGN)  # ignore kill signal
    signal.signal(signal.SIGTSTP, signal.SIG_IGN)  # ignore ctrl+z signal

class PsuUpgrade():
    def __init__(self):
        signal_init()
        debug_init()
        self.upgrade_param = UPGRADE_SUMMARY.copy()
        self.psu_upgrade_param = self.upgrade_param.get('psu', {})
        self.head_info_config = {}
        self.force = None
        self.psu_num = None
        self.pmbus_path = None
        self.block_size = None
        self.write_time = None

    def get_psu_upgrade_param(self):
        return self.psu_upgrade_param

    def get_psu_model_map(self, mfr_id, mfr_model):
        return PSU_MODEL_MAP.get((mfr_id, mfr_model), None)
    
    def psu_firmware_head_check(self, psu_model):
        try:
            # Upgrade file header parameter extraction
            fw_type = self.head_info_config.get('TYPE')
            fw_chipname = self.head_info_config.get('CHIPNAME')
            fw_filetype = self.head_info_config.get('FILETYPE')

            # type check
            if fw_type != 'psu':
                msg = "firmware TYPE:%s not match." % fw_type
                psu_upg_debug(msg)
                return False, ERR_PSU_FW_HEAD_CHECK

            # chipname check 
            if psu_model != fw_chipname:
                msg = "psu_model:%s not match CHIPNAME:%s" % (psu_model, fw_chipname)
                psu_upg_debug(msg)
                return False, ERR_PSU_FW_HEAD_CHECK

            # filetype check
            if fw_filetype != 'PSU_PMBUS':
                msg = "FILETYPE:%s incorrect." % fw_filetype
                psu_upg_debug(msg)
                return False, ERR_PSU_FW_HEAD_CHECK

            msg = "psu firmware head check success"
            psu_upg_debug(msg)
            return True, msg

        except Exception as e:
            msg = "do psu firmware head check exception happend. reason:%s" % str(e)
            psu_upg_error(msg)
            return False, ERR_PSU_FW_HEAD_CHECK

    def psu_present_check(self, slot):
        try:
            config = {
                "loc": f"/sys/{S3IP_SYSFS_NAME}/psu/psu{slot}/present",
                "gettype": "sysfs", 
                "int_decode": 10
            }
            ret, val = get_value(config)
            if ret is False:
                msg = "get PSU%d present status failed." % slot
                psu_upg_error(msg)
                return False, msg

            if val != PSU_PRESENT:
                msg = "PSU%d absent." % slot
                psu_upg_debug(msg)
                return False, msg

            msg = "PSU%d present check success." % slot
            psu_upg_debug(msg)
            return True, msg  
        except Exception as e:
            msg = f"psu absent check exception happened. reason: {e}"
            psu_upg_error(msg)
            return False, msg

    def get_psu_pmbus_path(self, slot):
        try:
            config = {
                "loc": f"/sys/{S3IP_SYSFS_NAME}/psu/psu{slot}/pmbus_bus",
                "gettype": "sysfs", 
                "int_decode": 10
            }
            ret, bus = get_value(config)
            if ret is False:
                msg = "get PSU%d bus failed." % slot
                psu_upg_error(msg)
                return False, msg
            
            config = {
                "loc": f"/sys/{S3IP_SYSFS_NAME}/psu/psu{slot}/pmbus_addr",
                "gettype": "sysfs", 
                "int_decode": 16
            }
            ret, psmbus_addr = get_value(config)
            if ret is False:
                msg = "get PSU%d pmbus_addr failed." % slot
                psu_upg_error(msg)
                return False, msg
            
            pmbus_path = "/sys/bus/i2c/devices/%d-%04x" % (bus, psmbus_addr)
            return True, pmbus_path
        except Exception as e:
            msg = "get psu pmbus path exception happened. reason:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def psu_status_check(self, slot, status_type):
        try: 
            ret, pmbus_path = self.get_psu_pmbus_path(slot)
            if ret is False:
                msg = "get PSU%d pmbus path failed." % slot
                psu_upg_error(msg)
                return False, msg

            # Read status_word from PMBus
            status_word_path = os.path.join(pmbus_path, 'status_word')
            with open(status_word_path, 'r') as file:
                psu_status_str = file.read().strip()

            psu_status = int(psu_status_str, 16)
            if status_type == PSU_STATUS:
                # Check if bit11(POWER_GOOD) is set for status_word
                if (psu_status & 0x0800) != 0:
                    msg = "PSU%d status fail, status_word: 0x%x." % (slot, psu_status)
                    psu_upg_error(msg)
                    return False, msg
            elif status_type == PSU_INPUT_STATUS:
                # Check if bit3(VIN-UV_FAULT) and bit13(INPUT) are set for status_word
                if (psu_status & 0x2008) != 0:
                    msg = "PSU%d input status fail, status_word: 0x%x." % (slot, psu_status)
                    psu_upg_error(msg)
                    return False, msg
            else:
                msg = "unsupported psu status_type."
                psu_upg_error(msg)
                return False, msg

            msg = "PSU%d status check success." % slot
            return True, msg
        except Exception as e:
            msg = "do psu status check exception happened. reason:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def psu_redundancy_check(self, slot):
        try:      
            dev_safe_psu_num = self.psu_upgrade_param.get("dev_safe_psu_num", None)
            if dev_safe_psu_num is None:
                config = {
                    "loc": f"/sys/{S3IP_SYSFS_NAME}/psu/redundancy_num",
                    "gettype": "sysfs", 
                    "int_decode": 10
                }
                ret, redundancy_num = get_value(config)
                if ret is False:
                    msg = "get PSU%d redundancy_num failed." % slot
                    psu_upg_error(msg)
                    return False, msg
                dev_safe_psu_num = self.psu_num - redundancy_num

            ok_psu_num = 0
            for i in range(1, self.psu_num + 1):
                if i == slot:
                    # Skip upgrading PSU
                    continue

                # Power status check
                ret, msg = self.psu_status_check(i, PSU_STATUS)
                if ret is True:
                    ok_psu_num += 1
                    msg = "PSU%d status is ok." % i
                    psu_upg_debug(msg)
                else:
                    msg = "PSU%d status is fail." % i
                    psu_upg_debug(msg)
                    
            if ok_psu_num >= dev_safe_psu_num:
                msg  = "The power supply meets the upgrade conditions."
                psu_upg_debug(msg)
                return True, msg
            else:
                msg = "The power supply does not meet the upgrade condition."
                psu_upg_error(msg)
                return False, msg

        except Exception as e:
            msg = "do psu redundancy check exception happened. reason:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def psu_upgrade_check(self, file, slot, psu_model):
        try:
            ret, log = parse_file_head(file, self.head_info_config)
            psu_upg_debug(log)
            if ret is False:
                return False, ERR_PSU_FW_HEAD_PARSE

            # firmware head check
            ret, msg = self.psu_firmware_head_check(psu_model)
            if ret is False:
                psu_upg_debug("firmware head check fail.")
                return ret, msg
                
            # Power input status check
            ret, msg = self.psu_status_check(slot, PSU_INPUT_STATUS)
            if ret is False:
                msg = "psu%d input status fail." % slot
                psu_upg_error(msg)
                return False, msg
            psu_upg_debug("PSU%d input status check success" % slot)

            # Power redundancy check
            ret, msg = self.psu_redundancy_check(slot)
            if ret is False:
                msg = "psu redundancy check fail."
                psu_upg_error(msg)
                return False, msg

            msg = "PSU%d upgrade check success." % slot
            psu_upg_debug(msg)
            return True, msg

        except Exception as e:
            msg = "psu upgrade check happened exception. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def set_dev_available(self, enable):
        if enable is True:
            value = 1
        else:
            value = 0
        set_val_conf = {
            "gettype": "sysfs",
            "loc": "%s/dev_available" % self.pmbus_path,
            "value": value,
        }
        ret, log = set_value(set_val_conf)
        if ret is False:
            msg = "set dev_available failed, msg: %s" % log
            psu_upg_error(msg)
            return False, msg
        msg = "set dev_available success, value: %d" % value
        psu_upg_debug(msg)
        return True, msg

    def psu_fw_header_parse(self, header_bytes):
        if len(header_bytes) != 32:
            msg = "The length of header_bytes must be 32 bytes"
            psu_upg_error(msg)
            return False, msg

        # Slice by structure field
        crc = header_bytes[0:2]
        image_information = header_bytes[2:10]
        reserved_1 = header_bytes[10:21]
        reserved_2 = header_bytes[21:23]
        fw_major = header_bytes[23]
        fw_minor_primary = header_bytes[24]
        fw_minor_secondary = header_bytes[25]
        hw_compatibility = header_bytes[26:28]
        block_size_bytes = header_bytes[28:30]
        write_time_bytes = header_bytes[30:32]
        block_size = block_size_bytes[0] | (block_size_bytes[1] << 8)
        write_time = write_time_bytes[0] | (write_time_bytes[1] << 8)

        psu_upg_debug("Firmware Image Header:")
        psu_upg_debug(f"    CRC: {crc.hex()}")
        psu_upg_debug(f"    image_information: {image_information.decode('ascii', errors='ignore')}")
        psu_upg_debug(f"    reserved_1: {reserved_1.decode('ascii', errors='ignore')}")
        psu_upg_debug(f"    reserved_2: {reserved_2.decode('ascii', errors='ignore')}")
        psu_upg_debug(f"    fw_version: {fw_major:x}.{fw_minor_primary:x}.{fw_minor_secondary:x}")
        psu_upg_debug(f"    hw_compatibility: {hw_compatibility.hex()}")
        psu_upg_debug(f"    block_size: {block_size}")
        psu_upg_debug(f"    write_time: {write_time}ms")

        if block_size > I2C_SMBUS_BLOCK_MAX:
            msg = "block size exceed I2C_SMBUS_BLOCK_MAX(%d)." % I2C_SMBUS_BLOCK_MAX
            psu_upg_error(msg)
            return False, msg

        self.block_size = block_size
        self.write_time = write_time
        return True, "psu firmware header parse success"
    
    def psu_set_fwupload_mode(self, slot, value):
        try:
            config = {
                "loc": "%s/mfr_fwupload_mode" % self.pmbus_path,
                "gettype": "sysfs", 
                "value": value
            }
            ret, msg = set_value(config)
            if ret is False:
                err_msg = "set psu fwupload mode failed"
                psu_upg_error(err_msg)
                return False, err_msg

            return True, "psu set fwupload mode success"
        except Exception as e:
            err_msg = "set psu fwupload mode happened exception. log:%s" % str(e)
            psu_upg_error(err_msg)
            return False, err_msg

    def psu_get_fwupload_mode(self, slot):
        try:
            config = {
                "loc": "%s/mfr_fwupload_mode" % self.pmbus_path,
                "gettype": "sysfs", 
                "int_decode": 16
            }
            ret, val = get_value(config)
            if ret is False:
                msg = "get fwupload mode failed"
                psu_upg_error(msg)
                return False, msg

            return True, val
        except Exception as e:
            msg = "get psu fwupload mode happened exception. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg
        
    def psu_get_fwupload_status(self, slot):
        try:
            config = {
                "loc": "%s/mfr_fwupload_status" % self.pmbus_path,
                "gettype": "sysfs", 
                "int_decode": 16
            }
            ret, val = get_value(config)
            if ret is False:
                msg = "get fwupload status failed"
                psu_upg_error(msg)
                return False, msg
            
            return True, val
        except Exception as e:
            msg = "get psu fwupload status happened exception. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg
        
    def psu_upg_send_img_block(self, slot, block_bytes):
        try:
            fwupload_path =  "%s/mfr_fwupload" % self.pmbus_path
            hex_str = block_bytes.hex()
            with open(fwupload_path, 'w') as f:
                f.write(hex_str)

            return True, "write block successfully"
        except Exception as e:
            msg = "write block to %s failed: %s" % (fwupload_path, str(e))
            psu_upg_error(msg)
            return False, msg

    def psu_upg_write_img(self, raw_file, slot):
        try:
            with open(raw_file, 'rb') as f:
                raw_data = f.read()

            size = len(raw_data)
            if size % self.block_size != 0:
                msg = "Image size is not multiple of block size %d." % self.block_size
                psu_upg_error(msg)
                return False, msg

            block_num = size // self.block_size
            psu_upg_debug("image block num: %d, block size: %d" % (block_num, self.block_size))

            psu_upg_debug("Writing image...")
            # send first block of image
            first_block = raw_data[0:self.block_size]
            ret, msg = self.psu_upg_send_img_block(slot, first_block)
            if ret is False:
                msg = "send first image block failed."
                psu_upg_error(msg)
                return False, msg

            time.sleep(PSU_FW_FIRST_BLOCK_DELAY_TIME_S)

            # send remain block of image
            for send_block_cnt in range(1, block_num):
                ret, status = self.psu_get_fwupload_status(slot)
                if ret is True:
                    if status != MFR_FWUPLOAD_RCV_IMG_NOT_YET:
                        msg = "Send image fail, status: 0x%04x" % status
                        psu_upg_error(msg)
                        return False, msg
                else:
                    return False, msg
                    

                start = send_block_cnt * self.block_size
                end = start + self.block_size
                block_data = raw_data[start:end]
                ret, msg = self.psu_upg_send_img_block(slot, block_data)
                if ret is False:
                    msg = "send image block failed."
                    psu_upg_error(msg)
                    return False, msg 

                # ms covert to Seconds
                time.sleep(self.write_time / 1000.0)

            time.sleep(PSU_FW_CHECKSUM_DELAY_TIME_S)

            ret, status = self.psu_get_fwupload_status(slot)
            if ret is True:
                if status != MFR_FWUPLOAD_RCV_IMG_SUCCESS:
                    msg = "Image received fail, status: 0x%04x" % status
                    psu_upg_error(msg)
                    return False, msg
            else:
                return False, msg

            msg = "write image successfully."
            psu_upg_debug(msg)
            return True, msg
        except Exception as e:
            msg = "psu upgrade write image exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def psu_upg_handle(self, raw_file, slot):
        try:
            psu_upg_debug("Upgrading...")
            for retry in range(PSU_FW_RETRY_COUNT_MAX):
                if retry > 0:
                    psu_upg_debug("Retrying to upgrade... attempt %d/%d" % (retry + 1, PSU_FW_RETRY_COUNT_MAX))
                rcv_img_success = False

                # Enter firmware upload mode
                ret, msg = self.psu_set_fwupload_mode(slot, MFR_FWUPLOAD_MODE_EN)
                if ret is False:
                    psu_upg_error("Send MFR_FWUPLOAD_MODE_EN fail.")
                    continue

                # Waiting to enter firmware upload mode
                time.sleep(PSU_FW_ENTER_UPLOAD_MODE_DELAY_TIME_S)

                ret, val = self.psu_get_fwupload_mode(slot)
                if val != MFR_FWUPLOAD_MODE_EN:
                    psu_upg_error("Enter firmware upload mode fail.")
                    continue
                else:
                   psu_upg_debug("set psu fwupload mode success")

                ret, msg = self.psu_upg_write_img(raw_file, slot)
                if ret is False:
                    psu_upg_error("Write firmware image fail.")
                    rcv_img_success = False
                    # Continue to execute psu_fwupload_mode_exit
                else:
                    rcv_img_success = True

                # Exit firmware upload mode
                ret, msg = self.psu_set_fwupload_mode(slot, MFR_FWUPLOAD_MODE_EXIT)
                if ret is False:
                    psu_upg_error("Send MFR_FWUPLOAD_MODE_EXIT fail.")
                    continue

                # Waiting to exit firmware upload mode
                time.sleep(PSU_FW_EXIT_UPLOAD_MODE_DELAY_TIME_S)

                if rcv_img_success:
                    ret, val = self.psu_get_fwupload_mode(slot)
                    if val == MFR_FWUPLOAD_MODE_EXIT:
                        msg = "psu exit upgrade mode success."
                        psu_upg_debug(msg)
                        return True, msg
                    else:
                        psu_upg_error("psu exit upgrade mode fail.")

            return False, "psu upgrade handle fail."
        except Exception as e:
            msg = "do psu upgrade handle exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def common_psu_upgrade(self, raw_file, slot):
        try:
            with open(raw_file, 'rb') as f:
                raw_data = f.read()

            # parse psu firmware header
            ret, msg = self.psu_fw_header_parse(raw_data[0:32])
            if ret is False:
                return False, msg

            # set dev_available not available
            psu_upg_debug("Starting to set dev_available not available")
            ret, msg = self.set_dev_available(False)
            if ret is False:
                return False, msg
            
            ret, msg = self.psu_upg_handle(raw_file, slot)
            if ret is False:
                 return False, msg
            
            msg = "common psu upgrade success."
            return True, msg
        except Exception as e:
            msg = "do common psu upgrade exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg
        finally:
            # set dev_available available
            psu_upg_debug("Starting to set dev_available available")
            self.set_dev_available(True)

    def psu_upgrading(self, file, slot):
        raw_file = None
        try:
            ret, raw_file = do_fw_upg_raw_file_generate(file, self.head_info_config)
            if ret is False:
                msg = "generate raw file failed, reason: %s" % raw_file
                psu_upg_error(msg)
                return False, msg

            ret, msg = self.common_psu_upgrade(raw_file, slot)
            if ret is False:
                msg = "common psu upgrade failed"
                psu_upg_error(msg)
                return False, msg

            return True, "psu upgrading success."
        except Exception as e:
            msg = "do psu upgrading exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg
        finally:
            if raw_file and os.path.exists(raw_file):
                os.remove(raw_file)

    def get_psu_model(self, slot, psu_model):
        try:
            ret, self.pmbus_path = self.get_psu_pmbus_path(slot)
            if ret is False:
                msg = "get PSU%d pmbus path failed." % slot
                psu_upg_error(msg)
                return False, msg
            
            mfr_id_path = os.path.join(self.pmbus_path, 'mfr_id')
            mfr_model_path = os.path.join(self.pmbus_path, 'mfr_model')

            try:
                with open(mfr_id_path, 'r') as file:
                    mfr_id = file.read().strip()
                with open(mfr_model_path, 'r') as file:
                    mfr_model = file.read().strip()
            except Exception as e:
                msg = "Failed to read mfr_id or mfr_model file: %s" % str(e)
                psu_upg_error(msg)
                return False, msg

            psu_upg_debug("PSU%d mfr_id: %s" % (slot, mfr_id))
            psu_upg_debug("PSU%d mfr_model: %s" % (slot, mfr_model))

            real_psu_model = self.get_psu_model_map(mfr_id, mfr_model)
            if real_psu_model is None:
                if psu_model is not None:
                    if self.force == FORCE_UPGRADE:
                        real_psu_model = psu_model
                        msg = "unknown psu, specify psu model[%s]" % psu_model
                        psu_upg_debug(msg)
                    else:
                        msg = "unknown psu, manual specification of model requires force"
                        psu_upg_error(msg)
                        return False, msg
                else:
                    msg = "get psu model failed."
                    psu_upg_error(msg)
                    return False, msg
            else:
                if psu_model is not None and real_psu_model != psu_model:
                    if self.force == FORCE_UPGRADE:
                        psu_upg_debug("force upgrade psu by config psu model[%s]" % psu_model)
                        real_psu_model = psu_model
                    else:
                        msg = "psu model[%s] not match config psu model[%s]" % (real_psu_model, psu_model)
                        psu_upg_error(msg)
                        return False, msg

            psu_upg_debug("PSU%d psu_model: %s" % (slot, real_psu_model))
            return True, real_psu_model
        except Exception as e:
            msg = "do get psu model exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def do_psu_file_upgrade(self, file, slot, psu_model):
        # PSU upgrade check
        ret, msg = self.psu_upgrade_check(file, slot, psu_model)
        if ret is False:
            return False, msg

        ret, msg = self.psu_upgrading(file, slot)
        if ret is False:
            return False, msg
        
        msg  = "upgrade success"
        psu_upg_debug(msg)
        return True, msg

    def do_psu_dir_upgrade(self, path, slot, psu_model):
        """
        Walk through all files under the directory 'path' and try to upgrade PSU firmware on each file.
        Skip files that return ERR_PSU_FW_HEAD_PARSE or ERR_PSU_FW_HEAD_CHECK errors.
        Return immediately with (True, file_path) if upgrade succeeds on a file.
        Return immediately with (False, error message) if upgrade fails on a file with other errors.
        If no upgradable firmware is found after walking all files, return (False, "Not found upgradable firmware.").
        """
        for root, dirs, names in os.walk(path):
            #root: directory absolute path
            #dirs: folder path collection under directory
            #names: file path collection under directory
            for filename in names:
                # file_path is file absolute path
                file_path = os.path.join(root, filename)
                ret, msg = self.do_psu_file_upgrade(file_path, slot, psu_model)
                if ret is True:
                    return True, file_path
                else:
                    if msg == ERR_PSU_FW_HEAD_PARSE or msg == ERR_PSU_FW_HEAD_CHECK:
                        continue
                    else:
                        return False, msg

        msg = "Not found upgradable firmware."
        return False, msg
    
    def upgrade_single(self, upgrade_method, path, slot, psu_model):
        ret, msg = self.psu_present_check(slot)
        if ret is False:
            msg = "PSU%d present check fail." % slot
            psu_upg_error(msg)
            return False, msg

        ret, msg = self.get_psu_model(slot, psu_model)
        if ret is False:
            return False, msg
        else:
            psu_model = msg

        ret, log = upgrade_method(path, slot, psu_model)
        if ret:
            msg = "PSU%d: upgrade %s success.\n" % (slot, log if os.path.isdir(path) else path)
            psu_upg_debug(msg)
            return True, msg
        else:
            msg = "PSU%d: upgrade fail, log: %s" % (slot, log)
            psu_upg_error(msg)
            return False, msg

    def perform_upgrade(self, upgrade_method, path, slot, psu_model):
        try:
            UPGRADE_SUMMARY = "UPGRADE SUMMARY: \n"
            if slot is None:
                has_fail = False
                for i in range(1, self.psu_num + 1):
                    ret, msg = self.upgrade_single(upgrade_method, path, i, psu_model)
                    if ret is False:
                        has_fail = True
                    UPGRADE_SUMMARY += msg
                            
                if has_fail is True:
                    ret = False
                else:
                    ret = True
            else:
                ret, msg = self.upgrade_single(upgrade_method, path, slot, psu_model)
                UPGRADE_SUMMARY += msg

            return ret, UPGRADE_SUMMARY
        except Exception as e:
            msg = "do perform upgrade exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def get_psu_num(self):
        try:
            config = {
                "loc": f"/sys/{S3IP_SYSFS_NAME}/psu/number",
                "gettype": "sysfs",
                "int_decode": 10
            }
            ret, self.psu_num = get_value(config)
            if ret is False:
                msg = "get psu number failed"
                psu_upg_error(msg)
                return False, msg

            msg = "total psu num: %d" % self.psu_num
            psu_upg_debug(msg)
            return True, msg
        except Exception as e:
            msg = "get psu_num exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def do_psu_upgrade(self, path, slot, psu_model, force):
        try:
            self.force = force
            ret, msg = self.get_psu_num()
            if ret is False:
                return False, msg

            if slot is not None:
                if not (1 <= slot <= self.psu_num):
                    msg = f"PSU slot {slot} does not exist, PSU slot num is {self.psu_num}"
                    psu_upg_error(msg)
                    return False, msg

            if os.path.isdir(path):
                return self.perform_upgrade(self.do_psu_dir_upgrade, path, slot, psu_model)
            elif os.path.isfile(path):
                return self.perform_upgrade(self.do_psu_file_upgrade, path, slot, psu_model)
            else:
                msg = "path:%s not found." % path
                psu_upg_error(msg)
                return False, msg

        except Exception as e:
            msg = "do psu upggrade exception happened. log:%s" % str(e)
            psu_upg_error(msg)
            return False, msg

    def do_psu_upgrade_main(self, file, slot, psu_model, force):
        print("+================================+")
        print("|  Doing upgrade, please wait... |")
        ret, log = self.do_psu_upgrade(file, slot, psu_model, force)
        if ret == True:
            print("|       upgrade succeeded!       |")
            print("+================================+")
            print("%s" % log)
            exit(0)
        else:
            print("|        upgrade failed!         |")
            print("+================================+")
            print("%s" % log)
            exit(1)


@click.command()
@click.argument('file', required=True)
@click.argument('slot', required=False, type=int, default=None)
@click.argument('psu_model', required=False, default=None)
@click.argument('force', required=False, type=int, default=None)
def main(file, slot, psu_model, force):
    '''psu upgrade'''
    psu_upg = PsuUpgrade()
    psu_upg.do_psu_upgrade_main(file, slot, psu_model, force)


if __name__ == '__main__':
    main()