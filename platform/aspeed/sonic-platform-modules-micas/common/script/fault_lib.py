#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import sys
import os
import time
import datetime
import syslog
import traceback
import glob
import subprocess
import logging
from time import monotonic as _time
from platform_config import get_config_param
from platform_util import get_value, set_value, read_sysfs, setup_logger, BSP_COMMON_LOG_DIR
from leak_monitor import LeakMonitor, STATUS_FAIL, STATUS_BREAK, STATUS_LEAK, STATUS_NORMAL


LEAK_STATUS_FAIL_VALUE = -1
LEAK_STATUS_BREAK_VALUE = 0
LEAK_STATUS_OK_VALUE = 1
LEAK_STATUS_NOK_VALUE = 2


error_sensor_value = -9999000  # get sensor value error
invalid_sensor_value = -10000000  # get sensor value invalid


DEBUG_FILE = "/etc/.fault_record_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "fault_record_debug.log"
logger = setup_logger(LOG_FILE)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def fault_record_err(s):
    # s = s.decode('utf-8').encode('gb2312')
    logger.error(s)


def fault_record_warn(s):
    # s = s.decode('utf-8').encode('gb2312')
    logger.warning(s)


def fault_record_debug(s):
    # s = s.decode('utf-8').encode('gb2312')
    logger.debug(s)


VOUT_MODE_PARAM = {
    0x18: 256,         # 2^8
    0x17: 512,         # 2^9
    0x16: 1024,        # 2^10
    0x15: 2048,        # 2^11
    0x14: 4096,        # 2^12
    0x13: 8192,        # 2^13
    0x12: 16384,       # 2^14
    0x11: 32768,       # 2^15
    0x10: 65536,       # 2^16
}

UCD90160_PAGE_FAULT = 0x80000000  # bit31

UCD90160_FAULT_TYPE_OFFSET = 27  # bit27
UCD90160_FAULT_TYPE_MASK = 0xF

UCD90160_FAULT_PAGE_OFFSET = 23  # bit23
UCD90160_FAULT_PAGE_MASK = 0xF

STATUS_UNKNOWN = -1
STATUS_NORNAL = 0

VOLTAGE_STATUS_OVER_VOLTAGE = 1
VOLTAGE_STATUS_UNDER_VOLTAGE = 2

CURRENT_STATUS_OVER_CURRENT_WARN = 1
CURRENT_STATUS_OVER_CURRENT_FAULT = 2

DEVICE_SYSFS_PRE = "/sys/bus/i2c/devices/%d-00%02x/"
DEVICE_SYSFS_HWMON = "hwmon/hwmon*/"
UCD90160_FAULT_RECORD = "fault_record"

'''
"0": {
    "ch": "0",
    "name": "ucd90160_1",
    "raw": "0x00 0x01 0x21 0xe4 0x87 0x80 0x00 0x00 0x00 0x00",
    "vout_status": "1",
    "iout_status": "0",
    "value": "3.3"
},
'''


class FaultRecord():
    def __init__(self):
        self.chips = get_config_param("FAULT_RECORD", [])
        self.pmbus_decoder = {
            "vout": [0x80, 0x10],
            "iout": [0x20, 0x80],
        }
        self.pmbus_status_decode = {
            "vout": {
                0x80: VOLTAGE_STATUS_OVER_VOLTAGE,
                0x10: VOLTAGE_STATUS_UNDER_VOLTAGE,
                0x00: STATUS_NORNAL
            },
            "iout": {
                0x20: CURRENT_STATUS_OVER_CURRENT_WARN,
                0x80: CURRENT_STATUS_OVER_CURRENT_FAULT,
                0x00: STATUS_NORNAL
            }
        }
        self.vout_status_map = {
            VOLTAGE_STATUS_OVER_VOLTAGE: "Over-Voltage",
            VOLTAGE_STATUS_UNDER_VOLTAGE: "Under-Voltage"
        }

        self.iout_status_map = {
            CURRENT_STATUS_OVER_CURRENT_WARN: "Over-Current-Warning",
            CURRENT_STATUS_OVER_CURRENT_FAULT: "Over-Current-Fault"
        }

    def debug_init(self):
        global debuglevel
        try:
            with open(LIB_DEBUG_FILE, "r") as fd:
                value = fd.read()
            debuglevel = int(value)
        except Exception as e:
            debuglevel = 0
        return

    def parse_ucd90160(self, chip):
        try:
            chip_name = chip.get("chip", None)
            bus = chip.get("bus", None)
            addr = chip.get("addr", None)
            rails = chip.get("rails", None)
            display_alias = chip.get("display_alias", rails)
            clear_cfg = chip.get("clear", None)
            path = DEVICE_SYSFS_PRE % (bus, addr) + UCD90160_FAULT_RECORD
            ret, raw_data = read_sysfs(path)
            if ret is False:
                fault_record_err("UCD90160 read %s failed, msg %s" % (path, raw_data))
                return []
            record_list = list()
            for index in range(32):
                fault_record_dict = {}
                offset = index * 16
                # index is valid fault record
                if (ord(raw_data[offset])) == 0x0a:
                    tmp_data = (ord(raw_data[offset + 5]) << 24 | ord(raw_data[offset + 6]) << 16
                                | ord(raw_data[offset + 7]) << 8 | ord(raw_data[offset + 8]))
                    if tmp_data & UCD90160_PAGE_FAULT:  # is page fault
                        fault_type = tmp_data >> UCD90160_FAULT_TYPE_OFFSET & UCD90160_FAULT_TYPE_MASK
                        fault_page = tmp_data >> UCD90160_FAULT_PAGE_OFFSET & UCD90160_FAULT_PAGE_MASK
                        if fault_type == 0:
                            status_vout_mode = VOLTAGE_STATUS_OVER_VOLTAGE
                        elif fault_type == 1:
                            status_vout_mode = VOLTAGE_STATUS_UNDER_VOLTAGE
                        else:
                            status_vout_mode = STATUS_UNKNOWN
                        vout_mode = ord(raw_data[offset + 11])
                        exponent = VOUT_MODE_PARAM[vout_mode]
                        tmp_value = ord(raw_data[offset + 10]) << 8 | ord(raw_data[offset + 9])
                        value = float(float(tmp_value) / exponent)
                        raw_list = ""
                        for y in range(0x0a):
                            raw_list = raw_list + "0x%02x" % ord(raw_data[offset + y + 1]) + " "
                            # raw_list.append("0x%02x" % ord(raw_data[offset + y + 1]))
                        fault_record_dict["ch"] = str(fault_page)
                        sub_name = rails.get(fault_page, None)
                        sub_alias = display_alias.get(fault_page, sub_name)
                        fault_record_dict["name"] = chip_name + "_" + sub_name
                        fault_record_dict["display_name"] = sub_alias
                        fault_record_dict["raw"] = raw_list
                        fault_record_dict["raw"] = fault_record_dict["raw"].strip()
                        fault_record_dict["vout_status"] = str(status_vout_mode)
                        fault_record_dict["iout_status"] = str(STATUS_NORNAL)
                        fault_record_dict["value"] = ('%.3f' % value)
                        status_msg_map = self.vout_status_map
                        msg_status = status_msg_map.get(status_vout_mode, None)
                        if msg_status is not None:
                            fault_record_warn("%s %s ch%d %s detected" % (chip_name, sub_name, fault_page, msg_status))
                        if fault_record_dict["vout_status"] != str(STATUS_NORNAL):
                            record_list.append(fault_record_dict)
            # Clear current chip fault record
            if clear_cfg is not None:
                ret, msg = set_value(clear_cfg)
                if ret is False:
                    fault_record_err(msg)
                else:
                    fault_record_debug(msg)
            return record_list
        except Exception as e:
            fault_record_err(str(e))
            return []

    def parse_pmbus_status(self, chip):
        try:
            chip_name = chip.get("chip", None)
            bus = chip.get("bus", None)
            addr = chip.get("addr", None)
            rails = chip.get("rails", None)
            path = DEVICE_SYSFS_PRE % (bus, addr) + DEVICE_SYSFS_HWMON
            records = chip.get("fault_record", None)
            ch_num = chip.get("ch_num", None)
            okval = chip.get("okval", None)
            decode_v = chip.get("decode_v", None)
            mask = chip.get("mask", None)
            clear_cfg = chip.get("clear", None)
            display_alias = chip.get("display_alias", rails)
            hw_index = glob.glob(path)[0].split("hwmon")[-1].replace("/", "")
            record_list = list()
            for ch in range(ch_num):
                fault_record_dict = {}
                fault_record_dict["ch"] = str(ch)
                fault_record_dict["value"] = "NA"
                fault_record_dict["raw"] = ""
                sub_name = rails.get(ch, None)
                sub_alias = display_alias.get(ch, sub_name)
                for index in range(len(records)):
                    ftype = records[index]
                    decode = decode_v[index]
                    ok_val = okval[index]
                    data_mask = mask[index]
                    node = ("status%d_" % ch) + ftype
                    real_path = "/sys/kernel/debug/pmbus/hwmon%s/%s" % (hw_index, node)
                    ret, retval = read_sysfs(real_path)
                    if ret is False:
                        fault_record_err("PMBUS debugfs %s read failed, msg: %s" % (real_path, retval))
                        continue
                    raw_data = int(retval, decode) & data_mask
                    fault_record_dict["raw"] = fault_record_dict["raw"] + "0x%02x" % raw_data + " "
                    decoder = self.pmbus_decoder.get(ftype, None)
                    status_decode = self.pmbus_status_decode.get(ftype, None)
                    tmp_status = STATUS_NORNAL
                    # Only one fault type would be recorded
                    for val in decoder:
                        if raw_data & val != ok_val:
                            tmp_status = val
                    status_mode = status_decode.get(tmp_status)
                    if ftype == "vout":
                        status_msg_map = self.vout_status_map
                    elif ftype == "iout":
                        status_msg_map = self.iout_status_map
                    else:
                        fault_record_err("Unknown Fault record type %s" % ftype)
                        continue
                    fault_record_dict[ftype + "_status"] = str(status_mode)
                    msg_status = status_msg_map.get(status_mode, None)
                    if msg_status is not None:
                        fault_record_warn("%s-%s %s ch%d %s detected" % (chip_name, sub_name, ftype, ch, msg_status))
                fault_record_dict["raw"] = fault_record_dict["raw"].strip()
                fault_record_dict["name"] = chip_name + "_" + sub_name
                fault_record_dict["display_name"] = sub_alias
                if fault_record_dict["vout_status"] != str(
                        STATUS_NORNAL) or fault_record_dict["iout_status"] != str(STATUS_NORNAL):
                    record_list.append(fault_record_dict)
            # Clear current chip fault record
            if clear_cfg is not None:
                ret, msg = set_value(clear_cfg)
                if ret is False:
                    fault_record_err(msg)
                else:
                    fault_record_debug(msg)
            return record_list
        except Exception as e:
            fault_record_err(str(e))
            return []

    def get_leak_detect_status(self, chip):
        try:
            chip_name = chip.get("chip", None)
            if chip_name is None:
                return False, None, "chip is None."

            leak_status = LEAK_STATUS_OK_VALUE
            leak_monitor = LeakMonitor()

            # set default vol
            leak_detect_vol = invalid_sensor_value
            for point in leak_monitor.monitor_points:
                name = point['name']
                if name == chip_name:
                    status, value = leak_monitor.get_leakage_filter(point)
                    if status == STATUS_FAIL:
                        leak_detect_vol = error_sensor_value
                        leak_status = LEAK_STATUS_FAIL_VALUE
                        fault_record_warn("%s detected %s." % (chip_name, status))
                    elif status == STATUS_BREAK:
                        leak_detect_vol = value
                        leak_status = LEAK_STATUS_BREAK_VALUE
                        fault_record_warn("%s detected rope %s." % (chip_name, status))
                    elif status == STATUS_LEAK:
                        leak_detect_vol = value
                        leak_status = LEAK_STATUS_NOK_VALUE
                        fault_record_warn("%s detected rope %s." % (chip_name, status))
                    elif status == STATUS_NORMAL:
                        leak_detect_vol = value
                        leak_status = LEAK_STATUS_OK_VALUE
                    else:
                        fault_record_err("%s get_leakage_filter failed." % chip_name)
                        return False, None, "%s get_leakage_filter failed." % chip_name

                    return True, leak_status, leak_detect_vol
            return False, None, "not find chip_name %s." % chip_name
        except Exception as e:
            fault_record_err(str(e))
            return False, None, str(e)

    def parse_leak_detect(self, chip):
        try:
            chip_name = chip.get("chip", None)
            if chip_name is None:
                return []

            ch = chip.get("ch", None)
            parse = chip.get("parse", None)
            record_list = list()

            leak_status = LEAK_STATUS_OK_VALUE
            leak_monitor = LeakMonitor()
            max = float(leak_monitor.break_vol_threshold) / 1000
            min = float(leak_monitor.leak_vol_threshold) / 1000

            ret, leak_status, value = self.get_leak_detect_status(chip)
            if ret is False:
                fault_record_err("get %s leak detect status failed, log: %s." % (chip_name, value))
                return []

            if leak_status != LEAK_STATUS_OK_VALUE:
                fault_record_dict = {}
                fault_record_dict["ch"] = str(ch)
                fault_record_dict["name"] = chip_name
                fault_record_dict["vout_status"] = "NA"
                fault_record_dict["iout_status"] = "NA"
                fault_record_dict["leak_status"] = str(leak_status)
                fault_record_dict["value"] = str(float(value) / 1000)
                fault_record_dict["parse"] = parse
                fault_record_dict["raw"] = str(float(value) / 1000) + "v " + str(min) + "v " + str(max) + "v"
                record_list.append(fault_record_dict)
            return record_list
        except Exception as e:
            fault_record_err(str(e))
            return []

    def get_fault_record(self):
        self.debug_init()
        fault_record_list = list()
        record_dict = {}
        fault_cnt = 0
        for chip in self.chips:
            parse = chip.get("parse", None)
            if parse == "pmbus_status":
                record_list = self.parse_pmbus_status(chip)
            elif parse == "ucd90160":
                record_list = self.parse_ucd90160(chip)
            elif parse == "leak_detect":
                record_list = self.parse_leak_detect(chip)
            else:
                continue
            fault_record_list = fault_record_list + record_list
            fault_cnt = fault_cnt + len(record_list)
        return fault_cnt, fault_record_list


def get_chip_fault_record():
    debug_init()
    dev = FaultRecord()
    dev_num, dev_list = dev.get_fault_record()
    dev_dict_tmp = {}
    dev_dict = {}
    dev_dict["fault_record"] = {}
    dev_dict_tmp["number"] = dev_num
    for index in range(len(dev_list)):
        dev_dict_tmp["%d" % index] = dev_list[index]
    dev_dict["fault_record"].update(dev_dict_tmp)
    return dev_dict
