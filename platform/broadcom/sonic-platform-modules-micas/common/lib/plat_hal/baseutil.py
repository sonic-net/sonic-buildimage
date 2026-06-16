#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import importlib.machinery
import os
import syslog
import json
import subprocess
import glob
from plat_hal.osutil import osutil

SYSLOG_IDENTIFIER = "HAL"

CONFIG_DB_PATH = "/etc/sonic/config_db.json"
BOARD_ID_PATH = "/sys/module/platform_common/parameters/dfd_my_type"
BOARD_AIRFLOW_PATH = "/etc/sonic/.airflow"


def getonieplatform(path):
    if not os.path.isfile(path):
        return ""
    machine_vars = {}
    with open(path) as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars.get("onie_platform")


def getboardid():
    if not os.path.exists(BOARD_ID_PATH):
        return "NA"
    with open(BOARD_ID_PATH) as fd:
        id_str = fd.read().strip()
    return "0x%x" % (int(id_str, 10))


def getboardairflow():
    if not os.path.exists(BOARD_AIRFLOW_PATH):
        return "NA"
    with open(BOARD_AIRFLOW_PATH) as fd:
        airflow_str = fd.read().strip()
    data = json.loads(airflow_str)
    airflow = data.get("board", "NA")
    return airflow


def getplatform_config_db():
    if not os.path.isfile(CONFIG_DB_PATH):
        return ""
    val = subprocess.check_output(["sonic-cfggen", "-j", CONFIG_DB_PATH, "-v", "DEVICE_METADATA.localhost.platform"]).decode().strip()
    if len(val) <= 0:
        return ""
    return val


def getplatform_name():
    if os.path.isfile('/host/machine.conf'):
        return getonieplatform('/host/machine.conf')
    if os.path.isfile('/etc/sonic/machine.conf'):
        return getonieplatform('/etc/sonic/machine.conf')
    return getplatform_config_db()


platform = (getplatform_name()).replace("-", "_")
boardid = getboardid()
boardairflow = getboardairflow()


CONFIG_FILE_PATH_LIST = [
    "/usr/lib/python3/dist-packages/",
    "/usr/local/lib/*/dist-packages/hal-config/"
]


DEVICE_CONFIG_FILE_LIST = [
    platform + "_" + boardid + "_" + boardairflow + "_device.py",
    platform + "_" + boardid + "_device.py",
    platform + "_" + boardairflow + "_device.py",
    platform + "_device.py"
]


MONITOR_CONFIG_FILE_LIST = [
    platform + "_" + boardid + "_" + boardairflow + "_monitor.py",
    platform + "_" + boardid + "_monitor.py",
    platform + "_" + boardairflow + "_monitor.py",
    platform + "_monitor.py"
]


class baseutil:

    CONFIG_NAME = 'devices'
    MONITOR_CONFIG_NAME = 'monitor'
    UBOOT_ENV_URL = '/etc/device/uboot_env'

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
    def get_productname():
        ret, val = osutil.command("cat %s |grep productname | awk -F\"=\" '{print $2;}'" % baseutil.UBOOT_ENV_URL)
        tmp = val.lower().replace('-', '_')
        if ret != 0 or len(val) <= 0:
            raise Exception("get productname error")
        return tmp

    @staticmethod
    def get_platform():
        ret, val = osutil.command("cat %s |grep conffitname | awk -F\"=\" '{print $2;}'" % baseutil.UBOOT_ENV_URL)
        if ret != 0 or len(val) <= 0:
            raise Exception("get platform error")
        return val

    @staticmethod
    def get_product_fullname():
        ret, val = osutil.command("cat %s |grep productname | awk -F\"=\" '{print $2;}'" % baseutil.UBOOT_ENV_URL)
        if ret != 0 or len(val) <= 0:
            raise Exception("get productname error")
        return val

    @staticmethod
    def logger_debug(msg):
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_DEBUG, msg)
        syslog.closelog()
