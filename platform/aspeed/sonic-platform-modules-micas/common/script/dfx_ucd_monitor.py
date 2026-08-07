#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import sys
import os
import time
import logging
import traceback
import syslog
from logging.handlers import RotatingFileHandler
from platform_util import read_sysfs, write_sysfs, allow_syslog, BSP_COMMON_LOG_DIR, get_value, setup_logger
from platform_config import get_config_param

DFX_UCD_MONITOR_INFO = get_config_param("DFX_UCD_MONITOR_INFO", {})
DEBUG_FILE = "/etc/.dfx_ucd_monitor_debug"
LOG_FILE = BSP_COMMON_LOG_DIR + "dfx_ucd_monitor_debug.log"
logger = setup_logger(LOG_FILE)

debuglevel = 0
STATUS_OK = 0
FAULT_FLAG = "/sys/bus/i2c/devices/%s/fault_flag"
FAULT_RECORD_ANALY = "/sys/bus/i2c/devices/%s/fault_record_analy"
LOG_LAST_TIME = {}
STATUS_NOT_OK = "NOT OK"
STATUS_NEED_CHECK = "NEED"
STATUS_DONT_CHECK = "DONT"

'''
DFX_UCD_MONITOR_INFO = {
    "interval": 60,
    "device_list": [
        {
            "name": "UCD90160_48_5b",
            "loc": "48-005b",
        },
        {
            "name": "UCD90160_54_5b",
            "loc": "54-005b",
        },
        {
            "name": "UCD90160_33_5b",
            "loc": "33-005b",
            "pre_check": {"gettype": "sysfs", "loc": "/sys/s3ip/system/cpu_board_status", "okval": 1, "presentbit": 1},
        },
        {
            "name": "UCD90160_41_5b",
            "loc": "41-005b",
            "pre_check": {"gettype": "sysfs", "loc": "/sys/s3ip/system/cpu_board_status", "okval": 1, "presentbit": 1},
        },
    ]
}
'''

def dfx_ucd_monitor_debug(s):
    logger.debug(s)


def dfx_ucd_monitor_error(s):
    logger.error(s)


def dfx_ucd_monitor_info(s):
    logger.info(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
        debuglevel = 1
    else:
        logger.setLevel(logging.INFO)
        debuglevel = 0


def is_need_check(param):
    try:
        ret = {}
        ret["status"] = ''

        ret_t, val = get_value(param)
        if ret_t is False:
            ret["status"] = STATUS_NOT_OK
            dfx_ucd_monitor_error("is_need_check:get value failed, param: %s, log: %s" % (param, val))
            return ret

        presentbit = param.get('presentbit', 0)
        okval = param.get('okval')
        val_t = (val & (1 << presentbit)) >> presentbit
        if val_t != okval:
            ret["status"] = STATUS_DONT_CHECK
        else:
            ret["status"] = STATUS_NEED_CHECK
    except Exception as e:
        ret["status"] = STATUS_NOT_OK
        dfx_ucd_monitor_error("is_need_check error, msg: %s" % (traceback.format_exc()))
    return ret


def monitor_syslog(s):
    dfx_ucd_monitor_info("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))

def monitor_syslog_debug(s):
    if debuglevel == 1:
        syslog.openlog("DFX_UCD_MONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)
    dfx_ucd_monitor_debug(s)

def monitor_syslog_error(s):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        syslog.openlog("DFX_UCD_MONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)
    dfx_ucd_monitor_error(s)

class Device():
    def __init__(self, dev_conf):
        self.dev_conf = dev_conf
        self.dev_name = dev_conf.get('name', '')
        self.bus_addr = dev_conf.get("loc", None)
        self.last_status = None

    def check_fault(self):
        monitor_syslog_debug("dev: %s check start." % self.dev_name)
        try:
            if not self.bus_addr:
                monitor_syslog_error("dev: %s configuration is invalid." % self.dev_name)
                return False
            pre_check = self.dev_conf.get("pre_check", None)
            if pre_check is not None:
                ret = is_need_check(pre_check)
                monitor_syslog_debug("%s is_need_check status:%s" % (self.dev_name, ret.get('status')))
                if ret.get('status') != STATUS_NEED_CHECK:
                    return

            # get fault_flag 
            fault_flag_path = FAULT_FLAG % self.bus_addr
            ret, fault_flag = read_sysfs(fault_flag_path)
            if not ret:
                monitor_syslog_error("dev: %s, fault_flag read failed at %s." % (self.dev_name, fault_flag_path))
                return False

            fault_flag = int(fault_flag)
            monitor_syslog_debug("dev: %s, fault_flag=%d" % (self.dev_name, fault_flag))

            # first or status not ok
            if self.last_status is None or fault_flag != STATUS_OK:
                #not ok
                if fault_flag != STATUS_OK:
                    fault_record_path = FAULT_RECORD_ANALY % self.bus_addr
                    ret, fault_record = read_sysfs(fault_record_path)
                    if not ret:
                        fault_record = "Failed to read fault_record_analy at %s" % fault_record_path
                        monitor_syslog_error(fault_record)

                    # record 
                    monitor_syslog("dev: %s, fault detected. \nFault Flag: %d\nFault Record:\n%s" % (self.dev_name, fault_flag, fault_record))

                    # clear status
                    ret = write_sysfs(fault_record_path, "0")
                    if ret:
                        monitor_syslog("dev: %s, fault cleared successfully." % self.dev_name)
                    else:
                        monitor_syslog_error("dev: %s, failed to clear fault at %s." % (self.dev_name, fault_record_path))
                else:
                    # ok
                    if self.last_status is None:
                        monitor_syslog("dev: %s, Status OK. Fault Flag: %d" % (self.dev_name, fault_flag))
                    else:
                        monitor_syslog_debug("dev: %s, Status OK. Fault Flag: %d" % (self.dev_name, fault_flag))

                self.last_status = fault_flag
            else:
                monitor_syslog_debug("dev: %s, fault flag: %d" % (self.dev_name, fault_flag))

        except Exception:
            monitor_syslog_error('dev: %s check error. msg: %s\n' % (self.dev_name, str(traceback.format_exc())))

class Monitor():
    def __init__(self):
        self.interval = DFX_UCD_MONITOR_INFO.get('interval', 60)
        devices_conf = DFX_UCD_MONITOR_INFO.get('device_list', [])

        self.dev_list_obj = []
        for dev_conf in devices_conf:
            tmp_dev_obj = Device(dev_conf)
            self.dev_list_obj.append(tmp_dev_obj)

    def doWork(self):
        for dev in self.dev_list_obj:
            dev.check_fault()

    def run(self):
        while True:
            try:
                debug_init()
                self.doWork()
            except Exception as e:
                monitor_syslog_error('%%DFX_UCD_MONITOR-3-EXCEPTION: %s.' % (str(e)))
            time.sleep(self.interval)

if __name__ == '__main__':
    debug_init()
    monitor = Monitor()
    monitor.run()
