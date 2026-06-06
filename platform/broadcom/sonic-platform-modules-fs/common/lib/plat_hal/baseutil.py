#!/usr/bin/env python3
#######################################################
#
# baseutil.py
# Python implementation of the Class baseutil
#
#######################################################
import importlib.machinery
import os
import syslog
import glob
from plat_hal.osutil import osutil
from wbutil.baseutil import get_platform_info
from wbutil.baseutil import get_board_id
from wbutil.baseutil import get_sub_version
from wbutil.baseutil import get_board_airflow


SYSLOG_IDENTIFIER = "HAL"

platform = "NA"
boardid = "NA"
boardairflow = "NA"
sub_ver = "NA"


def get_product_info():
    global platform
    global boardid
    global boardairflow
    global sub_ver

    status, val_tmp = get_platform_info()
    if status is True:
        platform = val_tmp

    status, val_tmp = get_board_id()
    if status is True:
        boardid = val_tmp

    status, val_tmp = get_board_airflow()
    if status is True:
        boardairflow = val_tmp

    status, val_tmp = get_sub_version()
    if status is True:
        sub_ver = val_tmp
    return

get_product_info()


CONFIG_FILE_PATH_LIST = [
    "/usr/local/bin/",
    "/usr/lib/python3/dist-packages/",
    "/usr/local/lib/*/dist-packages/hal-config/"
]


DEVICE_CONFIG_FILE_LIST = [
    platform + "_" + boardid + "_" + sub_ver + "_" + boardairflow + "_device.py",
    platform + "_" + boardid + "_" + sub_ver + "_device.py",
    platform + "_" + boardid + "_" + boardairflow + "_device.py",
    platform + "_" + boardid + "_device.py",
    platform + "_" + boardairflow + "_device.py",
    platform + "_device.py"
]


MONITOR_CONFIG_FILE_LIST = [
    platform + "_" + boardid + "_" + sub_ver + "_" + boardairflow + "_monitor.py",
    platform + "_" + boardid + "_" + sub_ver + "_monitor.py",
    platform + "_" + boardid + "_" + boardairflow + "_monitor.py",
    platform + "_" + boardid + "_monitor.py",
    platform + "_" + boardairflow + "_monitor.py",
    platform + "_monitor.py"
]


class baseutil:

    CONFIG_NAME = 'devices'
    MONITOR_CONFIG_NAME = 'monitor'

    @staticmethod
    def get_config():
        real_path = None
        for configfile_path in CONFIG_FILE_PATH_LIST:
            if "/*/" in configfile_path:
                filepath = glob.glob(configfile_path)
                if len(filepath) == 0:
                    continue
                configfile_path = filepath[0]
            for config_file in DEVICE_CONFIG_FILE_LIST:
                file = configfile_path + config_file
                if os.path.exists(file):
                    real_path = file
                    break
            if real_path is not None:
                break

        if real_path is None:
            raise Exception("get hal device config error")
        devices = importlib.machinery.SourceFileLoader(baseutil.CONFIG_NAME, real_path).load_module()
        return devices.devices

    @staticmethod
    def get_monitor_config():
        real_path = None
        for configfile_path in CONFIG_FILE_PATH_LIST:
            for config_file in MONITOR_CONFIG_FILE_LIST:
                file = configfile_path + config_file
                if os.path.exists(file):
                    real_path = file
                    break
            if real_path is not None:
                break

        if real_path is None:
            raise Exception("get hal monitor config error")
        monitor = importlib.machinery.SourceFileLoader(baseutil.MONITOR_CONFIG_NAME, real_path).load_module()
        return monitor.monitor

    @staticmethod
    def logger_debug(msg):
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_DEBUG, msg)
        syslog.closelog()
