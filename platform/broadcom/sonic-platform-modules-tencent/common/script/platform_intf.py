#!/usr/bin/env python3
import os
import syslog
import importlib.machinery
from platform_util import getplatform_name, dev_file_read, dev_file_write, write_sysfs, read_sysfs

__all__ = [
    "platform_reg_read",
    "platform_reg_write",
    "platform_set_optoe_type",
    "platform_get_optoe_type",
    "platform_sfp_read",
    "platform_sfp_write",
]

CPLD = 0
FPGA = 1
CPLD_PATH = "/dev/cpld%d"
FPGA_PATH = "/dev/fpga%d"


OPTOE_PATH = "/sys/bus/i2c/devices/%d-0050/"
OPTOE_DEV_CLASS = "dev_class"
OPTOE_EEPROM = "eeprom"


PLATFORM_INTF_DEBUG_FILE = "/etc/.platform_intf_debug_flag"


CONFIG_FILE_LIST = ["/usr/local/bin/", "/usr/local/lib/python3/dist-packages/config/", "/usr/local/lib/python3.7/dist-packages/config/", "/usr/local/lib/python3.9/dist-packages/config/"]


def platform_intf_debug(s):
    if os.path.exists(PLATFORM_INTF_DEBUG_FILE):
        syslog.openlog("PLATFORM_INTF_DEBUG", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def platform_intf_error(s):
    if os.path.exists(PLATFORM_INTF_DEBUG_FILE):
        syslog.openlog("PLATFORM_INTF_ERROR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


class IntfPlatform:
    CONFIG_NAME = 'PLATFORM_INTF_OPTOE'
    __port_optoe_dict = {}
    def __init__(self):
        real_path = None
        platform = (getplatform_name()).replace("-", "_")
        for configfile_path in CONFIG_FILE_LIST:
            configfile = (configfile_path + platform + "_port_config.py")
            if os.path.exists(configfile):
                real_path = configfile
                break
        if real_path is None:
            raise Exception("get port config error")
        config = importlib.machinery.SourceFileLoader(self.CONFIG_NAME, real_path).load_module()
        self.__port_optoe_dict = config.PLATFORM_INTF_OPTOE

    def get_dev_path(self, dev_type, dev_id):
        if dev_type == CPLD:
            path = CPLD_PATH % dev_id
        elif dev_type == FPGA:
            path = FPGA_PATH % dev_id
        else:
            msg = "dev_type error!"
            return False, msg
        platform_intf_debug("path:%s" % path)
        return True, path

    def get_port_path(self, port):
        port_num = self.__port_optoe_dict.get("port_num", 0)
        optoe_start_bus = self.__port_optoe_dict.get("optoe_start_bus", 0)
        if port_num <= 0 or optoe_start_bus <= 0:
            msg = "PLATFORM_INTF_OPTOE config error!"
            return False, msg
        if port <= 0 or port > port_num:
            msg = "port out of range !"
            return False, msg
        path = OPTOE_PATH % (port + optoe_start_bus - 1)
        platform_intf_debug("path:%s" % path)
        return True, path

    def reg_read(self, dev_type, dev_id, offset, size):
        ret, path = self.get_dev_path(dev_type, dev_id)
        if ret == False:
            return False, path
        ret, info = dev_file_read(path, offset, size)
        return ret, info

    def reg_write(self, dev_type, dev_id, offset, val_list):
        ret, path = self.get_dev_path(dev_type, dev_id)
        if ret == False:
            return False, path
        ret, info = dev_file_write(path, offset, val_list)
        return ret, info

    def set_optoe_type(self, port, optoe_type):
        ret, path = self.get_port_path(port)
        if ret == False:
            return False, path
        optoe_type_path = path + OPTOE_DEV_CLASS
        ret, info = write_sysfs(optoe_type_path, "%d" % optoe_type)
        if ret == False:
            return False, info
        return True, None

    def get_optoe_type(self, port):
        ret, path = self.get_port_path(port)
        if ret == False:
            return False, path
        optoe_type_path = path + OPTOE_DEV_CLASS
        ret, info = read_sysfs(optoe_type_path)
        if ret == False:
            return False, info
        return True, int(info)

    def sfp_read(self, port_id, offset, size):
        ret, path = self.get_port_path(port_id)
        if ret == False:
            return False, path
        optoe_eeprom_path = path + OPTOE_EEPROM
        ret, info = dev_file_read(optoe_eeprom_path, offset, size)
        return ret, info

    def sfp_write(self, port_id, offset, val_list):
        ret, path = self.get_port_path(port_id)
        if ret == False:
            return False, path
        optoe_eeprom_path = path + OPTOE_EEPROM
        ret, info = dev_file_write(optoe_eeprom_path, offset, val_list)
        return ret, info


platform = IntfPlatform()


def platform_reg_read(dev_type, dev_id, offset, size):
    ret = False
    info = None

    '''1. params check
    '''
    if (isinstance(dev_type, int) is False or isinstance(dev_id, int) is False or
            isinstance(offset, int) is False or isinstance(size, int) is False):
        info = "params type check fail in platform_reg_read"
        return ret, info
    if dev_id < 0 or offset < 0 or size <= 0:
        info = "params value check fail in platform_reg_read"
        return ret, info
    if dev_type != CPLD and dev_type != FPGA:
        info = "dev_type match erro, fail in platform_reg_read"
        return ret, info

    '''2. call the solve func
    '''
    return platform.reg_read(dev_type, dev_id, offset, size)


def platform_reg_write(dev_type, dev_id, offset, val_list):
    ret = False
    info = None

    '''1. params check
    '''
    if (isinstance(dev_type, int) is False or isinstance(dev_id, int) is False or
            isinstance(offset, int) is False or isinstance(val_list, list) is False):
        info = "params type check fail in platform_reg_write"
        return ret, info
    if dev_id < 0 or offset < 0 or len(val_list) <= 0:
        info = "params value check fail in platform_reg_write"
        return ret, info
    if dev_type != CPLD and dev_type != FPGA:
        info = "dev_type match erro, fail in platform_reg_write"
        return ret, info

    '''2. call the solve func
    '''
    return platform.reg_write(dev_type, dev_id, offset, val_list)


def platform_set_optoe_type(port, optoe_type):
    ret = False
    info = None

    '''1. params check
    '''
    if isinstance(port, int) is False or isinstance(optoe_type, int) is False:
        info = "params type check fail in platform_set_optoe_type"
        return ret, info
    if port < 0 or optoe_type < 1 or optoe_type > 3:
        info = "params value check fail in platform_set_optoe_type"
        return ret, info

    '''2. call the solve func
    '''
    return platform.set_optoe_type(port, optoe_type)


def platform_get_optoe_type(port):
    ret = False
    info = None

    '''1. params check
    '''
    if isinstance(port, int) is False:
        info = "params type check fail in platform_get_optoe_type"
        return ret, info
    if port < 0:
        info = "params value check fail in platform_get_optoe_type"
        return ret, info

    '''2. call the solve func
    '''
    return platform.get_optoe_type(port)


def platform_sfp_read(port_id, offset, size):
    ret = False
    info = None

    '''1. params check
    '''
    if isinstance(port_id, int) is False or isinstance(offset, int) is False or isinstance(size, int) is False:
        info = "params type check fail in platform_sfp_read"
        return ret, info
    if port_id < 0 or offset < 0 or size <= 0:
        info = "params value check fail in platform_sfp_read"
        return ret, info

    '''2. call the solve func
    '''
    return platform.sfp_read(port_id, offset, size)


def platform_sfp_write(port_id, offset, val_list):
    ret = False
    info = None

    '''1. params check
    '''
    if isinstance(port_id, int) is False or isinstance(offset, int) is False or isinstance(val_list, list) is False:
        info = "params type check fail in platform_sfp_write"
        return ret, info
    if port_id < 0 or offset < 0 or len(val_list) <= 0:
        info = "params value check fail in platform_sfp_write"
        return ret, info

    '''2. call the solve func
    '''
    return platform.sfp_write(port_id, offset, val_list)