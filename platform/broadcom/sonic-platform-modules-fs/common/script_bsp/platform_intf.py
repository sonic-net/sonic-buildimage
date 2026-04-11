#!/usr/bin/env python3
import os
import syslog
import glob
import importlib.machinery
import logging
import functools
from platform_util import dev_file_read, dev_file_write, write_sysfs, read_sysfs, setup_logger, BSP_COMMON_LOG_DIR
from platform_util import get_mgmt_version
from wbutil.baseutil import get_platform_info

__all__ = [
    "platform_reg_read",
    "platform_reg_write",
    "platform_set_optoe_type",
    "platform_get_optoe_type",
    "platform_sfp_read",
    "platform_sfp_write",
    "platform_bmc_read",
    "platform_bmc_write",
]

CPLD = 0
FPGA = 1
CPLD_PATH = "/dev/cpld%d"
FPGA_PATH = "/dev/fpga%d"
LPC_BMC_PATH = "/dev/lpc_bmc"

WIDTH_4 = 4


OPTOE_PATH = "/sys/bus/i2c/devices/%d-0050/"
OPTOE_DEV_CLASS = "dev_class"
OPTOE_EEPROM = "eeprom"

DEBUG_FILE = "/etc/.platform_intf_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_intf_debug.log"
logger = setup_logger(LOG_FILE)

CONFIG_FILE_LIST = [
    "/usr/local/bin/",
    "/usr/local/lib/*/dist-packages/config/"
]

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def platform_intf_debug(s):
    logger.debug(s)

def platform_intf_error(s):
    logger.error(s)

def auto_debug_init(cls):
    for attr_name, attr in cls.__dict__.items():
        if callable(attr):
            @functools.wraps(attr)
            def wrapper(self, *args, __attr=attr, **kwargs):
                debug_init()
                return __attr(self, *args, **kwargs)
            setattr(cls, attr_name, wrapper)
    return cls

@auto_debug_init
class IntfPlatform:
    CONFIG_NAME = 'PLATFORM_INTF_OPTOE'
    __port_optoe_dict = {}

    def __init__(self):
        real_path = None
        status, platform_name = get_platform_info()
        if status is False:
            raise Exception("get_platform_info error, msg: %s" % platform_name)
        for configfile_path in CONFIG_FILE_LIST:
            if "/*/" in configfile_path:
                filepath = glob.glob(configfile_path)
                if len(filepath) == 0:
                    continue
                configfile_path = filepath[0]
            configfile = configfile_path + platform_name + "_port_config.py"
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
        if port_num <= 0:
            msg = "PLATFORM_INTF_OPTOE port_num config error, port_num: %d!" % port_num
            return False, msg

        if port <= 0 or port > port_num:
            msg = "port out of range !"
            return False, msg

        port_bus_map = self.__port_optoe_dict.get("port_bus_map")
        optoe_start_bus = self.__port_optoe_dict.get("optoe_start_bus", 0)
        if port_bus_map is None: # get port bus by optoe_start_bus
            if optoe_start_bus <= 0:
                msg = "PLATFORM_INTF_OPTOE optoe_start_bus config error, optoe_start_bus: %d" % optoe_start_bus
                return False, msg
            port_bus = port + optoe_start_bus - 1
        else: # get port bus by port_bus_map
            port_bus = port_bus_map.get(port)
            if port_bus is None:
                msg = "port %d don't has i2c bus" % port
                return False, msg
            if not isinstance(port_bus, int) or port_bus < 0:
                msg = "port %d i2c bus config error, port_bus: %s " % port_bus
                return False, msg

        path = OPTOE_PATH % (port_bus)
        platform_intf_debug("path:%s" % path)
        return True, path

    ###########################################
    # reg_read - read logic device register
    # @dev_type: 0: CPLD, 1: FPGA
    # @dev_id: device ID, start from 0
    # @offset: register offset
    # @size: read length
    # return:
    # @ret: True if read success, False if not
    # @info: The read value list if read success, otherwise the detail error message
    ###########################################
    def reg_read(self, dev_type, dev_id, offset, size):
        ret, path = self.get_dev_path(dev_type, dev_id)
        if ret is False:
            return False, path
        ret, info = dev_file_read(path, offset, size)
        return ret, info

    ###########################################
    # platform_reg_write - write logic device register
    # @dev_type: 0: CPLD, 1: FPGA
    # @dev_id: device ID, start from 0
    # @offset: register offset
    # @val_list: The write value list
    # return:
    # @ret: True if write success, False if not
    # @info: The write value length if write success, otherwise the detail error message
    ###########################################
    def reg_write(self, dev_type, dev_id, offset, val_list):
        ret, path = self.get_dev_path(dev_type, dev_id)
        if ret is False:
            return False, path
        ret, info = dev_file_write(path, offset, val_list)
        return ret, info

    ###########################################
    # set_optoe_type - set port optoe type
    # @port: port index start from 1
    # @optoe_type: optoe type, including the following values
    # 1: OPTOE1
    # 2: OPTOE2
    # 3: OPTOE3
    # return:
    # @ret: True if set optoe type success, False if not
    # @info: None if set optoe type success, otherwise the detail error message
    ###########################################
    def set_optoe_type(self, port, optoe_type):
        ret, path = self.get_port_path(port)
        if ret is False:
            return False, path
        optoe_type_path = path + OPTOE_DEV_CLASS
        ret, info = write_sysfs(optoe_type_path, "%d" % optoe_type)
        if ret is False:
            return False, info
        return True, None

    ###########################################
    # get_optoe_type - get port optoe type
    # @port: port index start from 1
    # return:
    # @ret: True if set optoe type success, False if not
    # @info: Optoe type value if get optoe type success, otherwise the detail error message
    # optoe type including the following values
    # 1: OPTOE1
    # 2: OPTOE2
    # 3: OPTOE3
    ###########################################
    def get_optoe_type(self, port):
        ret, path = self.get_port_path(port)
        if ret is False:
            return False, path
        optoe_type_path = path + OPTOE_DEV_CLASS
        ret, info = read_sysfs(optoe_type_path)
        if ret is False:
            return False, info
        return True, int(info)

    ###########################################
    # sfp_read -read sfp eeprom
    # @port_id: port index start from 1
    # @offset: sfp eeprom offset
    # @size: read sfp eeprom length
    # return:
    # @ret: True if read success, False if not
    # @info: The read value list if read success, otherwise the detail error message
    ###########################################
    def sfp_read(self, port_id, offset, size):
        ret, path = self.get_port_path(port_id)
        if ret is False:
            return False, path
        optoe_eeprom_path = path + OPTOE_EEPROM
        ret, info = dev_file_read(optoe_eeprom_path, offset, size)
        return ret, info

    ###########################################
    # sfp_write -write sfp eeprom
    # @port_id: port index start from 1
    # @offset: sfp eeprom offset
    # @val_list: The write value list
    # return:
    # @ret: True if read success, False if not
    # @info: The write value length if write success, otherwise the detail error message
    ###########################################
    def sfp_write(self, port_id, offset, val_list):
        ret, path = self.get_port_path(port_id)
        if ret is False:
            return False, path
        optoe_eeprom_path = path + OPTOE_EEPROM
        ret, info = dev_file_write(optoe_eeprom_path, offset, val_list)
        return ret, info


platform = IntfPlatform()


###########################################
# platform_reg_read - read logic device register
# @dev_type: 0: CPLD, 1: FPGA
# @dev_id: device ID, start from 0
# @offset: register offset
# @size: read length
# return:
# @ret: True if read success, False if not
# @info: The read value list if read success, otherwise the detail error message
###########################################
def platform_reg_read(dev_type, dev_id, offset, size):
    ret = False
    info = None

    # params check
    if (isinstance(dev_type, int) is False or isinstance(dev_id, int) is False or
            isinstance(offset, int) is False or isinstance(size, int) is False):
        info = "params type check fail in platform_reg_read"
        return ret, info
    if dev_id < 0 or offset < 0 or size <= 0:
        info = "params value check fail in platform_reg_read"
        return ret, info
    support_dev_type = (CPLD, FPGA)
    if dev_type not in support_dev_type:
        info = "dev_type match erro, fail in platform_reg_read"
        return ret, info

    # call the solve func
    return platform.reg_read(dev_type, dev_id, offset, size)


###########################################
# platform_reg_write - write logic device register
# @dev_type: 0: CPLD, 1: FPGA
# @dev_id: device ID, start from 0
# @offset: register offset
# @val_list: The write value list
# return:
# @ret: True if write success, False if not
# @info: The write value length if write success, otherwise the detail error message
###########################################
def platform_reg_write(dev_type, dev_id, offset, val_list):
    ret = False
    info = None

    # params check
    if (isinstance(dev_type, int) is False or isinstance(dev_id, int) is False or
            isinstance(offset, int) is False or isinstance(val_list, list) is False):
        info = "params type check fail in platform_reg_write"
        return ret, info
    if dev_id < 0 or offset < 0 or len(val_list) <= 0:
        info = "params value check fail in platform_reg_write"
        return ret, info
    support_dev_type = (CPLD, FPGA)
    if dev_type not in support_dev_type:
        info = "dev_type match erro, fail in platform_reg_write"
        return ret, info

    # call the solve func
    return platform.reg_write(dev_type, dev_id, offset, val_list)

###########################################
# platform_bmc_read - read bmc register
# @offset: register offset
# @size: read length
# return:
# @ret: True if read success, False if not
# @info: The read value list if read success, otherwise the detail error message
###########################################
def platform_bmc_read(offset, size):
    ret = False
    info = None

    # params check
    if (isinstance(offset, int) is False or isinstance(size, int) is False):
        info = "params type check fail in platform_bmc_read"
        return ret, info
    if (offset % WIDTH_4 !=0) or (size % WIDTH_4 !=0):
        info = "please input offset/size times of 4\n"
        return ret, info
    if offset < 0 or size <= 0:
        info = "params value check fail in platform_bmc_read"
        return ret, info

    # call the solve func
    return dev_file_read(LPC_BMC_PATH, offset, size)

###########################################
# platform_bmc_write - write bmc register
# @offset: register offset
# @val_list: The write value list
# return:
# @ret: True if write success, False if not
# @info: The write value length if write success, otherwise the detail error message
###########################################
def platform_bmc_write(offset, val_list):
    ret = False
    info = None

    # params check
    if (isinstance(offset, int) is False or isinstance(val_list, list) is False):
        info = "params type check fail in platform_bmc_write"
        return ret, info
    if (offset % WIDTH_4 !=0):
        info = "please input offset times of 4\n"
        return ret, info
    if offset < 0 or len(val_list) <= 0:
        info = "params value check fail in platform_bmc_write"
        return ret, info

    # call the solve func
    return dev_file_write(LPC_BMC_PATH, offset, val_list)

###########################################
# platform_set_optoe_type - set port optoe type
# @port: port index start from 1
# @optoe_type: optoe type, including the following values
# 1: OPTOE1
# 2: OPTOE2
# 3: OPTOE3
# return:
# @ret: True if set optoe type success, False if not
# @info: None if set optoe type success, otherwise the detail error message
###########################################
def platform_set_optoe_type(port, optoe_type):
    ret = False
    info = None

    # params check
    if isinstance(port, int) is False or isinstance(optoe_type, int) is False:
        info = "params type check fail in platform_set_optoe_type"
        return ret, info
    if port < 0 or optoe_type < 1 or optoe_type > 3:
        info = "params value check fail in platform_set_optoe_type"
        return ret, info

    # call the solve func
    return platform.set_optoe_type(port, optoe_type)


###########################################
# platform_get_optoe_type - get port optoe type
# @port: port index start from 1
# return:
# @ret: True if set optoe type success, False if not
# @info: Optoe type value if get optoe type success, otherwise the detail error message
# optoe type including the following values
# 1: OPTOE1
# 2: OPTOE2
# 3: OPTOE3
###########################################
def platform_get_optoe_type(port):
    ret = False
    info = None

    # params check
    if isinstance(port, int) is False:
        info = "params type check fail in platform_get_optoe_type"
        return ret, info
    if port < 0:
        info = "params value check fail in platform_get_optoe_type"
        return ret, info

    # call the solve func
    return platform.get_optoe_type(port)


###########################################
# platform_sfp_read -read sfp eeprom
# @port_id: port index start from 1
# @offset: sfp eeprom offset
# @size: read sfp eeprom length
# return:
# @ret: True if read success, False if not
# @info: The read value list if read success, otherwise the detail error message
###########################################
def platform_sfp_read(port_id, offset, size):
    ret = False
    info = None

    # params check
    if isinstance(port_id, int) is False or isinstance(offset, int) is False or isinstance(size, int) is False:
        info = "params type check fail in platform_sfp_read"
        return ret, info
    if port_id < 0 or offset < 0 or size <= 0:
        info = "params value check fail in platform_sfp_read"
        return ret, info

    # call the solve func
    return platform.sfp_read(port_id, offset, size)


###########################################
# platform_sfp_write -write sfp eeprom
# @port_id: port index start from 1
# @offset: sfp eeprom offset
# @val_list: The write value list
# return:
# @ret: True if read success, False if not
# @info: The write value length if write success, otherwise the detail error message
###########################################
def platform_sfp_write(port_id, offset, val_list):
    ret = False
    info = None

    # params check
    if isinstance(port_id, int) is False or isinstance(offset, int) is False or isinstance(val_list, list) is False:
        info = "params type check fail in platform_sfp_write"
        return ret, info
    if port_id < 0 or offset < 0 or len(val_list) <= 0:
        info = "params value check fail in platform_sfp_write"
        return ret, info

    # call the solve func
    return platform.sfp_write(port_id, offset, val_list)

###########################################
# platform_get_mgmt_version -get mgmt firmware version
# return:
# @ret: True if read success, False if not
# @info: mgmt firmware version if read success, otherwise the detail error message
###########################################
def platform_get_mgmt_version():
    return get_mgmt_version()
