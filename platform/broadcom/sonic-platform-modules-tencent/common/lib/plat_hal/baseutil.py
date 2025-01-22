#!/usr/bin/env python3
#######################################################
#
# baseutil.py
# Python implementation of the Class baseutil
# Original author: rd@ruijie.com.cn
#
#######################################################
import importlib.machinery
import os
import syslog
from plat_hal.osutil import osutil

SYSLOG_IDENTIFIER = "HAL"

CONFIG_DB_PATH = "/etc/sonic/config_db.json"
BOARD_ID_PATH = "/sys/module/ruijie_common/parameters/dfd_my_type"


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


def getplatform_config_db():
    if not os.path.isfile(CONFIG_DB_PATH):
        return ""
    val = os.popen("sonic-cfggen -j %s -v DEVICE_METADATA.localhost.platform" % CONFIG_DB_PATH).read().strip()
    if len(val) <= 0:
        return ""
    else:
        return val


def getplatform_name():
    if os.path.isfile('/host/machine.conf'):
        return getonieplatform('/host/machine.conf')
    elif os.path.isfile('/usr/share/sonic/hwsku/machine.conf'):
        return getonieplatform('/usr/share/sonic/hwsku/machine.conf')
    else:
        return getplatform_config_db()


CONFIG_FILE_LIST = ["/usr/local/bin/", "/usr/lib/python3/dist-packages/", "/usr/local/lib/python3.7/dist-packages/hal-config/", "/usr/local/lib/python3.9/dist-packages/hal-config/"]

platform = (getplatform_name()).replace("-", "_")
boardid = getboardid()

boardid_devicefile = (platform + "_"+ boardid + "_device.py")
boardid_monitorfile = (platform + "_"+ boardid + "_monitor.py")

devicefile = (platform + "_device.py")
monitorfile = (platform + "_monitor.py")


class baseutil:

    CONFIG_NAME = 'devices'
    MONITOR_CONFIG_NAME = 'monitor'
    UBOOT_ENV_URL = '/etc/device/uboot_env'

    @staticmethod
    def get_config():
        real_path = None
        for configfile_path in CONFIG_FILE_LIST:
            boardid_config_file_path = (configfile_path + boardid_devicefile)
            config_file_path = (configfile_path + devicefile)
            if os.path.exists(boardid_config_file_path):
                real_path = boardid_config_file_path
                break
            elif os.path.exists(config_file_path):
                real_path = config_file_path
                break
        if real_path is None:
            raise Exception("get hal device config error")
        devices = importlib.machinery.SourceFileLoader(baseutil.CONFIG_NAME, real_path).load_module()
        return devices.devices

    @staticmethod
    def get_monitor_config():
        real_path = None
        for configfile_path in CONFIG_FILE_LIST:
            boardid_config_file_path = (configfile_path + boardid_monitorfile)
            config_file_path = (configfile_path + monitorfile)
            if os.path.exists(boardid_config_file_path):
                real_path = boardid_config_file_path
                break
            elif os.path.exists(config_file_path):
                real_path = config_file_path
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
        else:
            return tmp

    @staticmethod
    def get_platform():
        ret, val = osutil.command("cat %s |grep conffitname | awk -F\"=\" '{print $2;}'" % baseutil.UBOOT_ENV_URL)
        if ret != 0 or len(val) <= 0:
            raise Exception("get platform error")
        else:
            return val

    @staticmethod
    def get_product_fullname():
        ret, val = osutil.command("cat %s |grep productname | awk -F\"=\" '{print $2;}'" % baseutil.UBOOT_ENV_URL)
        if ret != 0 or len(val) <= 0:
            raise Exception("get productname error")
        else:
            return val

    @staticmethod
    def logger_debug(msg):
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_DEBUG, msg)
        syslog.closelog()
