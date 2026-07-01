#!/usr/bin/env python_nos

import sys
import os
import fcntl
import re
import subprocess
import shlex
import time
import mmap
import glob
import logging.handlers
import shutil
import gzip
import ast
import ctypes
import json
import syslog
import time
import copy
import zlib
import binascii
from typing import Tuple
from public.platform_common_config import (
    MGMT_VERSION_PATH, 
    SYSLOG_PREFIX, 
    EXECUTABLE_FILE_PATH, 
    S3IP_SYSFS_NAME, 
    S3IP_INVALID_VALUE, 
    REBOOT_CAUSE_PATH_DIR, 
    REBOOT_CAUSE_POWER_LOSS, 
    REBOOT_CAUSE_STR2INT, 
    REBOOT_TYPE_PATH, 
    PLATFORM_INFO_DIR, 
    HOST_MACHINE, 
    ONIE_HOST_MACHINE,
    PLATFORM_REBOOT_HISTORY_FILE,
    SONIC_DB_DATABASE_CONFIG_PATH,
    SONIC_STATE_DB_NAME,
    BMC_PATCH_KEY,
    BMC_PATCH_COUNTDOWN_FIELD,
    BMC_CHECK_UPDATING_ENABLE
)

G_RESTFUL_CLASS = None
try:
    from restful_util.restful_interface import *
    G_RESTFUL_CLASS = RestfulApi()
except Exception as e:
    pass

def is_docker():
    if os.path.exists('/.dockerenv'):
        return True
    try:
        with open('/proc/self/cgroup', 'rt') as f:
            if 'docker' in f.read():
                return True
    except Exception:
        pass
    return False

if not is_docker():
    import click
    class AliasedGroup(click.Group):
        def get_command(self, ctx, cmd_name):
            rv = click.Group.get_command(self, ctx, cmd_name)
            if rv is not None:
                return rv
            matches = [x for x in self.list_commands(ctx)
                       if x.startswith(cmd_name)]
            if not matches:
                return None
            if len(matches) == 1:
                return click.Group.get_command(self, ctx, matches[0])
            ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
            return None
else:
    class AliasedGroup:
        """Placeholder for AliasedGroup when click is not available (e.g., in Docker)."""
        pass

INVALID_DECODE_FUNC = -1
FIX_DECODE_FUNC = 1
REVERSE_DECODE_FUNC = 2

CHECK_VALUE_NOT_OK = 0
CHECK_VALUE_OK = 1

GET_BY_COMMON   = "common"
GET_BY_RESTFUL = "restful"
GET_BY_S3IP     = "s3ip"

CONFIG_DB_PATH = PLATFORM_INFO_DIR + "config_db.json"
MAILBOX_DIR = "/sys/bus/i2c/devices/"

DEV_PRESENT     = 1
DEV_ABSENT      = 0

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}

MAX_HEADER_SIZE = 1000

# bsp common log dir
BSP_COMMON_LOG_DIR = "/var/log/bsp_tech/"

# bsp common debug file dir
BSP_COMMON_DEBUG_FILE_DIR = "/var/log/bsp_tech/debug_file/"

PLATFORM_REBOOT_REASON_FILE = "/etc/.platform_reboot_reason"
PLATFORM_UTIL_GPIO_DEBUG_FILE = "/etc/.platform_util_gpio_debug"

S3IP_FAIL_RET_LIST = ["NA", "ACCESS FAILED", "UNKNOWN", "NO_CFG", S3IP_INVALID_VALUE]

SAME_SYSLOG_INTERVAL = 180

VR_CHIP_ID_PATH = "/sys/bus/i2c/devices/%d-%04x/device_id"

PLATFORM_I2C_RETRY_TIME = 3

__all__ = [
    "AliasedGroup",
    "strtoint",
    "byteTostr",
    "PLATFORM_I2C_RETRY_TIME",
    "getplatform_name",
    "wbi2cget",
    "wbi2cset",
    "wbpcird",
    "wbpciwr",
    "wbi2cgetWord",
    "wbi2csetWord",
    "wbi2cset_pec",
    "wbi2cset_wordpec",
    "wbsysset",
    "dev_file_read",
    "dev_file_write",
    "io_rd",
    "io_wr",
    "exec_os_cmd",
    "exec_os_cmd_log",
    "write_sysfs",
    "read_sysfs",
    "read_file_with_lock",
    "write_file_with_lock",
    "get_sysfs_value",
    "write_sysfs_value",
    "get_value",
    "set_value",
    "getSdkReg",
    "getMacTemp",
    "getMacTemp_sysfs",
    "get_format_value",
    "log_to_file",
    "append_reboot_history",
    "get_reboot_history",
    "record_reboot_cause",
    "GET_BY_COMMON",
    "GET_BY_RESTFUL",
    "GET_BY_S3IP",
    "DEV_PRESENT",
    "DEV_ABSENT",
    "get_multiple_info_from_restful",
    "get_onie_info",
    "get_single_info_from_restful",
    "G_RESTFUL_CLASS",
    "get_monotonic_time",
    "setup_logger",
    "BSP_COMMON_LOG_DIR",
    "get_multiple_info_from_s3ip",
    "S3IP_PREFIX_DIR_NAME",
    "DEV_TYPE_FAN",
    "DEV_TYPE_FAN_MOTOR",
    "DEV_TYPE_PSU",
    "DEV_TYPE_CPLD",
    "DEV_TYPE_FPGA",
    "DEV_TYPE_BMC",
    "DEV_TYPE_VOL",
    "DEV_TYPE_CURR",
    "DEV_TYPE_TEMP",
    "DEV_TYPE_SLOT",
    "DEV_TYPE_POWER",
    "DEV_TYPE_MISC",
    "CONTEXT_SETTINGS",
    "waitForDocker",
    "unload_process_byPid",
    "common_syslog_emerg",
    "common_syslog_alert",
    "common_syslog_crit",
    "common_syslog_error",
    "common_syslog_warn",
    "common_syslog_notice",
    "common_syslog_info",
    "common_syslog_debug",
    "check_bmc_updating_func",
    "calculate_file_crc32",
    "read_s3ip_sysfs",
    "allow_syslog",
    "CHECK_VALUE_OK",
    "CHECK_VALUE_NOT_OK",
    "check_value_and_get_value",
    "get_vr_chip_id",
    "BSP_COMMON_DEBUG_FILE_DIR",
]

class CodeVisitor(ast.NodeVisitor):

    def __init__(self):
        self.value = None

    def get_value(self):
        return self.value

    def get_op_value(self, node):
        if isinstance(node, ast.Call):       # node is func call
            value = self.visit_Call(node)
        elif isinstance(node, ast.BinOp):    # node is BinOp
            value = self.visit_BinOp(node)
        elif isinstance(node, ast.UnaryOp):  # node is UnaryOp
            value = self.visit_UnaryOp(node)
        elif isinstance(node, ast.Num):      # node is Num Constant
            value = node.n
        elif isinstance(node, ast.Str):      # node is Str Constant
            value = node.s
        elif isinstance(node, ast.List):     # node is List Constant
            value = [element.value for element in node.elts]
        else:
            raise NotImplementedError("Unsupport operand type: %s" % type(node))
        return value

    def visit_UnaryOp(self, node):
        '''
        node.op: operand type, only support ast.UAdd/ast.USub
        node.operand: only support ast.Call/ast.Constant(ast.Num/ast.Str)/ast.BinOp/ast.UnaryOp
        '''

        operand_value = self.get_op_value(node.operand)
        if isinstance(node.op, ast.UAdd):
            self.value = operand_value
        elif isinstance(node.op, ast.USub):
            self.value = 0 - operand_value
        else:
            raise NotImplementedError("Unsupport arithmetic methods %s" % type(node.op))
        return self.value

    def visit_BinOp(self, node):
        '''
        node.left: left operand,  only support ast.Call/ast.Constant(ast.Num)/ast.BinOp
        node.op: operand type, only support ast.Add/ast.Sub/ast.Mult/ast.Div
        node.right: right operan, only support ast.Call/ast.Constant(ast.Num/ast.Str)/ast.BinOp
        '''
        left_value = self.get_op_value(node.left)
        right_value = self.get_op_value(node.right)

        if isinstance(node.op, ast.Add):
            self.value = left_value + right_value
        elif isinstance(node.op, ast.Sub):
            self.value = left_value - right_value
        elif isinstance(node.op, ast.Mult):
            self.value = left_value * right_value
        elif isinstance(node.op, ast.Div):
            self.value = left_value / right_value
        else:
            raise NotImplementedError("Unsupport arithmetic methods %s" % type(node.op))
        return self.value

    def visit_Call(self, node):
        '''
        node.func.id: func name, only support 'float', 'int', 'str'
        node.args: func args list,only support ast.Constant(ast.Num/ast.Str)/ast.BinOp/ast.Call
        str/float only support one parameter, eg: float(XXX), str(xxx)
        int support one or two parameters, eg: int(xxx) or int(xxx, 16)
        xxx can be ast.Call/ast.Constant(ast.Num/ast.Str)/ast.BinOp
        '''
        calc_tuple = ("float", "int", "str", "max", "min")

        if node.func.id not in calc_tuple:
            raise NotImplementedError("Unsupport function call type: %s" % node.func.id)

        args_val_list = []
        for item in node.args:
            ret = self.get_op_value(item)
            if isinstance(ret, list):
                args_val_list.extend(ret)
            else:
                args_val_list.append(ret)

        if node.func.id == "str":
            if len(args_val_list) != 1:
                raise TypeError("str() takes 1 positional argument but %s were given" % len(args_val_list))
            value = str(args_val_list[0])
            self.value = value
            return value

        if node.func.id == "float":
            if len(args_val_list) != 1:
                raise TypeError("float() takes 1 positional argument but %s were given" % len(args_val_list))
            value = float(args_val_list[0])
            self.value = value
            return value

        if node.func.id == "max":
            value = max(args_val_list)
            self.value = value
            return value

        if node.func.id == "min":
            value = min(args_val_list)
            self.value = value
            return value
        # int
        if len(args_val_list) == 1:
            value = int(args_val_list[0])
            self.value = value
            return value
        if len(args_val_list) == 2:
            value = int(args_val_list[0], args_val_list[1])
            self.value = value
            return value
        raise TypeError("int() takes 1 or 2 arguments (%s given)" % len(args_val_list))

def allow_syslog(log_last_time, additional_key = "", min_interval = SAME_SYSLOG_INTERVAL):
    """
    Whether to allow syslog printing (based on log suppression interval).
    Uses monotonic time for robustness; exceptions are safely handled.
    """
    try:
        # find who is trying to print syslog
        frame = sys._getframe(2)

        now = time.monotonic()
        # key: uniquely identifies a specific syslog message for a device.
        key = "file:%s line:%s addition: %s" % (frame.f_code.co_filename, str(frame.f_lineno), additional_key)
        last = log_last_time.get(key)
        if last is None:
            log_last_time[key] = now
            return True, log_last_time
        else:
            if (now - last >= min_interval):
                log_last_time[key] = now
                return True, log_last_time
            else:
                return False, log_last_time
    except Exception as e:
        # On error, print log once and return True for safety
        # print(f"allow_syslog exception: {e}")
        return True, log_last_time


def common_syslog_emerg(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-EMERG-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_EMERG, info)

def common_syslog_alert(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-ALERT-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_ALERT, info)

def common_syslog_crit(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-CRIT-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_CRIT, info)

def common_syslog_error(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-ERR-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_ERR, info)

def common_syslog_warn(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-WARNING-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_WARNING, info)

def common_syslog_notice(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-NOTICE-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_NOTICE, info)

def common_syslog_info(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-INFO-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_INFO, info)

def common_syslog_debug(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-DEBUG-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_DEBUG, info)

def record_reboot_cause(reason, info):
    try:
        with open(PLATFORM_REBOOT_REASON_FILE, 'w') as file:
            file.write(reason)
    except Exception as e:
        pass
    append_reboot_history(reason)
    common_syslog_warn("REBOOT", ("The system will reboot due to %s." % info))

def platform_reboot(reason, info):
    try:
        record_reboot_cause(reason, info)
        time.sleep(0.1)
        os.system("/sbin/reboot")
        return True, "success"
    except Exception as e:
        return False, str(e)

def secret_key_decode(decode_func, ori_value, addr, is_set):
    """
    Function to process the value based on the decode_func parameter.

    Parameters:
        decode_func (int): Determines the type of processing to apply. Possible values:
            - FIX_DECODE_FUNC = 1: fix mode.
            - REVERSE_DECODE_FUNC = 2: Reverse mode.
        ori_value (int): The original value to process.
        is_set (bool): Whether the value is being set (True) or read (False).

    Returns:
        int: The processed value based on the decode_func parameter, or False if the mode is invalid.
    """
    if decode_func not in [FIX_DECODE_FUNC, REVERSE_DECODE_FUNC]:
        return False, ("secret_key_decode failed. decode_func: %s, ori_value: %d, addr: 0x%x" % (decode_func, ori_value, addr))

    if is_set:
        if decode_func == FIX_DECODE_FUNC:
            # FIX_DECODE_FUNC: Simply return the original value
            return True, ori_value
        elif decode_func == REVERSE_DECODE_FUNC:
            # Reverse mode: Take the high 7 bits, reverse them, and keep the lowest bit unchanged
            reversed_value = (~addr & 0xFE) | (ori_value & 0x01)
            return True, reversed_value
    else:
        if decode_func == REVERSE_DECODE_FUNC or decode_func == FIX_DECODE_FUNC:
            # Return only the lowest bit
            return True, ori_value & 0x01
    # If no valid operation is matched, return False as a fallback
    return False, ("secret_key_decode failed. decode_func: %d, ori_value: %d, addr: 0x%x" % (decode_func, ori_value, addr))

def inttostr(vl, length):
    if not isinstance(vl, int):
        raise Exception(" type error")
    index = 0
    ret_t = ""
    while index < length:
        ret = 0xff & (vl >> index * 8)
        ret_t += chr(ret)
        index += 1
    return ret_t


def get_monotonic_time():
    return time.monotonic()


def strtoint(str_tmp):
    value = 0
    rest_v = str_tmp.replace("0X", "").replace("0x", "")
    str_len = len(rest_v)
    for index, val in enumerate(rest_v):
        value |= int(val, 16) << ((str_len - index - 1) * 4)
    return value


def inttobytes(val, length):
    if not isinstance(val, int):
        raise Exception("type error")
    data_array = bytearray()
    index = 0
    while index < length:
        ret = 0xff & (val >> index * 8)
        data_array.append(ret)
        index += 1
    return data_array


def byteTostr(val):
    strtmp = ''
    for value in val:
        strtmp += chr(value)
    return strtmp


def typeTostr(val):
    strtmp = ''
    if isinstance(val, bytes):
        strtmp = byteTostr(val)
    return strtmp

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

def getplatform_config_db():
    if not os.path.isfile(CONFIG_DB_PATH):
        return ""
    result = subprocess.run(
        ["sonic-cfggen", "-j", CONFIG_DB_PATH, "-v", "DEVICE_METADATA.localhost.platform"],
        check=False,
        capture_output=True,
        text=True,
    )
    val = result.stdout.strip()
    if len(val) <= 0:
        return ""
    return val

def getplatform_name():
    if os.path.isfile(HOST_MACHINE):
        return getonieplatform(HOST_MACHINE)
    if os.path.isfile(PLATFORM_INFO_DIR + 'machine.conf'):
        return getonieplatform(PLATFORM_INFO_DIR + 'machine.conf')
    return getplatform_config_db()

# i2cget byte data with offset
def wbi2cget(bus, devno, address, word=None):
    if word is None:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x " % (bus, devno, address)
    else:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x %s" % (bus, devno, address, word)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2cset byte data with offset
def wbi2cset(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x" % (
        bus, devno, address, byte)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


def wbpcird(pcibus, slot, fn, resource, offset):
    '''read pci register'''
    if offset % 4 != 0:
        return "ERR offset: %d not 4 bytes align"
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(resource))
    with open(filename, "r+") as file:
        size = os.path.getsize(filename)
        data = mmap.mmap(file.fileno(), size)
        result = data[offset: offset + 4]
        s = result[::-1]
        val = 0
        for value in s:
            val = val << 8 | value
        data.close()
    return "0x%08x" % val


def wbpciwr(pcibus, slot, fn, resource, offset, data):
    '''write pci register'''
    ret = inttobytes(data, 4)
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(resource))
    with open(filename, "r+") as file:
        size = os.path.getsize(filename)
        data = mmap.mmap(file.fileno(), size)
        data[offset: offset + 4] = ret
        result = data[offset: offset + 4]
        s = result[::-1]
        val = 0
        for value in s:
            val = val << 8 | value
        data.close()


def wbi2cgetWord(bus, devno, address):
    command_line = "i2cget -f -y %d 0x%02x 0x%02x w" % (bus, devno, address)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2sget word data with offset
def wbi2csetWord(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%x w" % (
        bus, devno, address, byte)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2cget byte data without offset
def wbi2cgetByte(bus, devno):
    command_line = "i2cget -f -y %d 0x%02x" % (bus, devno)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2cset byte data without offset
def wbi2csetByte(bus, devno, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x" % (
        bus, devno, byte)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2cset byte data with offset and PEC
def wbi2cset_pec(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x bp" % (
        bus, devno, address, byte)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


# i2cset word data with offset and PEC
def wbi2cset_wordpec(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x wp" % (
        bus, devno, address, byte)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


def wbsysset(location, value):
    command_line = "echo 0x%02x > %s" % (value, location)
    ret, ret_t = exec_os_cmd(command_line)
    if ret == 0:
        return True, ret_t
    return False, ret_t


def dev_file_read(path, offset, read_len):
    val_list = []
    msg = ""
    ret = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    try:
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.read(fd, read_len)
        for item in ret:
            val_list.append(item)
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)
    return True, val_list


def dev_file_write(path, offset, buf_list):
    msg = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    if isinstance(buf_list, list):
        if len(buf_list) == 0:
            msg = "buf_list:%s is NONE !" % buf_list
            return False, msg
    elif isinstance(buf_list, int):
        buf_list = [buf_list]
    else:
        msg = "buf_list:%s is not list type or not int type !" % buf_list
        return False, msg

    try:
        fd = os.open(path, os.O_WRONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.write(fd, bytes(buf_list))
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    return True, ret


def exec_os_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    return status, output

def exec_os_cmd_log(cmd):
    proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE, shell=False, stderr=sys.stderr, close_fds=True,
                            stdout=sys.stdout, universal_newlines=True, bufsize=1)
    proc.wait()
    stdout = proc.communicate()[0]
    stdout = typeTostr(stdout)
    return proc.returncode, stdout

def io_rd(reg_addr, read_len=1):
    try:
        regaddr = 0
        if isinstance(reg_addr, int):
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        devfile = "/dev/port"
        fd = os.open(devfile, os.O_RDWR | os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        val = os.read(fd, read_len)
        return "".join(["%02x" % item for item in val])
    except ValueError:
        return None
    except Exception as e:
        print(e)
        return None
    finally:
        os.close(fd)


def io_wr(reg_addr, reg_data):
    try:
        regdata = 0
        regaddr = 0
        if isinstance(reg_addr, int):
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        if isinstance(reg_data, int):
            regdata = reg_data
        else:
            regdata = int(reg_data, 16)
        devfile = "/dev/port"
        fd = os.open(devfile, os.O_RDWR | os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        os.write(fd, regdata.to_bytes(1, 'little'))
        return True
    except ValueError as e:
        print(e)
        return False
    except Exception as e:
        print(e)
        return False
    finally:
        os.close(fd)


def lock_file(file):
    fcntl.flock(file, fcntl.LOCK_EX)


def unlock_file(file):
    fcntl.flock(file, fcntl.LOCK_UN)


def read_file_with_lock(file_path):
    try:
        with open(file_path, 'r') as f:
            lock_file(f)
            try:
                data = f.read()
                return True, data
            except Exception as e:
                return False, (f"Error reading file: {e}")
            finally:
                unlock_file(f)

    except FileNotFoundError:
        return False, (f"File not found: {file_path}")
    except IOError as e:
        return False, (f"I/O error occurred: {e}")


def write_file_with_lock(file_path, data):
    try:
        with open(file_path, 'w') as f:
            lock_file(f)
            try:
                f.write(data + '\n')
                return True, None
            except Exception as e:
                print(f"Error writing to file: {e}")
                return False, (f"Error writing to file: {e}")
            finally:
                unlock_file(f)
    except IOError as e:
        return False, (f"I/O error occurred: {e}")


def write_sysfs(location, value):
    try:
        locations = glob.glob(location)
        if len(locations) == 0:
            return False, ("%s not found" % location)
        with open(locations[0], 'w') as fd1:
            fd1.write(value)
    except Exception as e:
        return False, (str(e) + " location[%s]" % location)
    return True, ("set location[%s] %s success !" % (location, value))

def read_sysfs(location):
    try:
        locations = glob.glob(location)
        with open(locations[0], 'rb') as fd1:
            retval = fd1.read()
        retval = typeTostr(retval)
        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
    except Exception as e:
        return False, (str(e) + "location[%s]" % location)
    return True, retval

def calculate_md5(file_path):
    with open(file_path, 'rb') as f:
        md5 = hashlib.md5()
        while True:
            data = f.read(4096)
            if not data:
                break
            md5.update(data)
    return md5.hexdigest()

def getPid(name):
    ret = []
    for dirname in os.listdir('/proc'):
        if dirname == 'curproc':
            continue
        try:
            with open('/proc/{}/cmdline'.format(dirname), mode='r') as fd:
                content = fd.read()
        except Exception:
            continue
        if name in content:
            ret.append(dirname)
    return ret

def unload_process_byPid(script_name):
    rets = getPid(script_name)
    for ret in rets:
        cmd = "kill " + ret
        exec_os_cmd(cmd)
    return True, ("unload_process_byPid kill %s success" % script_name)

def set_file_and_dir_permissions_if_root(
    file_path: str,
    dir_mode: int = 0o777,
    file_mode: int = 0o666
) -> Tuple[bool, str]:
    """
    (Updated docstring) If the current user is root, atomically set permissions for the parent directory of the specified file (to `dir_mode`)
    and the file itself (to `file_mode`). The operation is atomic: either both permissions are set successfully, or no changes are made.

    Parameters:
        file_path (str): Absolute path to the file (e.g., /a/b/c.txt).
        dir_mode (int): Permissions for the parent directory (default: 0o777, recommended for most directories).
        file_mode (int): Permissions for the file (default: 0o666, recommended for most files).

    Returns:
        Tuple[bool, str]: (True if successful, "Success") or (False if failed, detailed error message).
    """
    # Check if the current user is root (only root can modify permissions of other users' files)
    if os.geteuid() != 0:
        return False, "Not running as root (requires root privileges)"

    # Check if the file path is absolute (relative paths may cause unexpected parent directory resolution)
    if not os.path.isabs(file_path):
        return False, "File path must be an absolute path (e.g., /a/b/c.txt)"

    # Check if the file exists (cannot set permissions for a non-existent file)
    if not os.path.exists(file_path):
        return False, f"File not found: {file_path} (verify the path is correct)"

    parent_dir = os.path.dirname(file_path)

    # Get the original permissions of the parent directory (to rollback if file permission setting fails)
    try:
        original_dir_stat = os.stat(parent_dir)
        original_dir_mode = original_dir_stat.st_mode & 0o777  # Extract only the permission bits (ignore other flags)
    except Exception as e:
        return False, f"Failed to retrieve parent directory permissions: {str(e)}"

    # Attempt to set permissions for the parent directory
    try:
        os.chmod(parent_dir, dir_mode)
    except Exception as e:
        return False, f"Failed to set parent directory permissions: {str(e)}"

    # Attempt to set permissions for the file (rollback parent directory permissions if this fails)
    try:
        os.chmod(file_path, file_mode)
    except Exception as e:
        # Rollback: Restore the parent directory to its original permissions
        try:
            os.chmod(parent_dir, original_dir_mode)
        except Exception as rollback_e:
            return False, f"Failed to set file permissions (and rollback of parent directory permissions failed): {str(e)}; Rollback error: {str(rollback_e)}"
        return False, f"Failed to set file permissions (parent directory permissions rolled back successfully): {str(e)}"

    return True, "Success"

def setup_logger(log_file, max_bytes=1024*1024*5, backup_count=3, formatter_type="default"):
    dir_path = os.path.dirname(log_file)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
    # creat logger
    logger = logging.getLogger(log_file)
    if not logger.hasHandlers():
        logger.setLevel(logging.INFO)

        # creat RotatingFileHandler set log file size and backupCount
        handler = logging.handlers.RotatingFileHandler(log_file, maxBytes=max_bytes, backupCount=backup_count)
        handler.setLevel(logging.DEBUG)

        # creat formatter and addd to handler
        if formatter_type == "default":
            formatter = logging.Formatter("%(asctime)s %(levelname)s %(filename)s[%(funcName)s][%(lineno)s]: %(message)s")
        elif formatter_type == "simple":
            formatter = logging.Formatter("%(message)s:%(asctime)s")
        else:
            formatter = logging.Formatter("%(asctime)s %(levelname)s %(filename)s[%(funcName)s][%(lineno)s]: %(message)s")
        handler.setFormatter(formatter)

        logger.addHandler(handler)

        set_file_and_dir_permissions_if_root(log_file)

    return logger

def log_to_file(content, log_file_path, max_size=5*1024*1024):
    try:
        logger = setup_logger(log_file_path, max_size)
        logger.info(content)

    except Exception as e:
        return False, (str(e) + "log_file_path[%s]" % log_file_path)
    return True, "log_to_file success"

def append_reboot_history(content, max_size=5*1024*1024, backupCount=3):
    try:
        reboot_logger = setup_logger(PLATFORM_REBOOT_HISTORY_FILE, max_size, backupCount, formatter_type="simple")
        reboot_logger.info(content)

    except Exception as e:
        return False, (str(e) + "reboot_path[%s]" % PLATFORM_REBOOT_HISTORY_FILE)
    return True, "Write success"

def get_reboot_history(file_path=PLATFORM_REBOOT_HISTORY_FILE):
    if not os.path.isfile(file_path):
        return False, (file_path + " not exist")
    history = []
    try:
        with open(file_path, 'r') as f:
            for line in f:
                if line.strip():
                    cause, timestamp = line.strip().split(":", 1)
                    history.append((cause, timestamp))
    except Exception as e:
        return False, (str(e) + " file_path[%s]" % file_path)
    return True, history

def get_pmc_register(reg_name):
    retval = 'ERR'
    mb_reg_file = MAILBOX_DIR + reg_name
    filepath = glob.glob(mb_reg_file)
    if len(filepath) == 0:
        return "%s %s  notfound" % (retval, mb_reg_file)
    mb_reg_file = filepath[0]
    if not os.path.isfile(mb_reg_file):
        return "%s %s  notfound" % (retval, mb_reg_file)
    try:
        with open(mb_reg_file, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        retval = retval + str(error)
    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    return retval


def get_sysfs_value(location):
    pos_t = str(location)
    name = get_pmc_register(pos_t)
    return name


def write_sysfs_value(reg_name, value):
    fileLoc = MAILBOX_DIR + reg_name
    try:
        if not os.path.isfile(fileLoc):
            print(fileLoc, 'not found !')
            return False
        with open(fileLoc, 'w') as fd:
            fd.write(value)
    except Exception:
        print("Unable to open " + fileLoc + "file !")
        return False
    return True


def wb_peci_pcicfglocal_rd(bus, device, func, reg, rd_len):
    valid_rd_len = (1, 2, 4)
    if rd_len not in valid_rd_len:
        return False, "Invalid read length: %d" % rd_len

    command_line = "dfd_debug peci_pcicfglocal_rd 0x%02x 0x%02x 0x%02x 0x%04x %d" % (bus, device, func, reg, rd_len)
    status, val_t = exec_os_cmd(command_line)
    if status != 0:
        return False, val_t

    val_list = []
    lines = val_t.strip("").split("\n")
    for line in lines:
        if line.startswith("0x"):
            val_list_tmp = re.sub(r'\s+', ' ', line.strip()).split(" ")
            val_list.extend(val_list_tmp[1:])

    if len(val_list) == 0:
        return False, "peci_pcicfglocal_rd fail, msg: %s " % val_t

    value = 0
    for index, val in enumerate(val_list):
        value |= (int(val, 16) << (8 * index))
    return True, value

def get_onie_info(info_type):
    if not os.path.isfile(ONIE_HOST_MACHINE):
        return ONIE_HOST_MACHINE + " not exist"
    machine_vars = {}
    with open(ONIE_HOST_MACHINE) as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars.get(info_type)

def decode_value(config, value=None):
    try:
        decode_type = config.get("decode_type")
        decode_info = config.get("decode_info", {})

        if decode_type == "direct_config":
            return True, config.get("value")
        elif decode_type == "format":
            format_value = ""
            attr_unit = decode_info.get("unit", None)
            attr_decimal_precision = decode_info.get("decimal_precision", None)
            formula = decode_info.get("formula", None)
            if formula is not None:
                value = str(get_format_value(formula % (float(value))))
                format_value = value
            if attr_decimal_precision is not None:
                format = "%%.%df" % attr_decimal_precision
                value = str(format % (float(value)))
                format_value = value
            if attr_unit is not None:
                format_value += " %s" % attr_unit
            return True, format_value
        elif decode_type == "decode":
            return True, decode_info.get(value, value)
        else:
            return False, "unsupport decode_type %s" % decode_type

    except Exception as e:
        return False, "decode_value error, config: %s, value: %s, reason: %s" % (config, value, e)

DEV_TYPE_FAN = "fan"
DEV_TYPE_FAN_MOTOR = "fan_motor"
DEV_TYPE_PSU = "psu"
DEV_TYPE_PSU_TEMP = "psu_temp"
DEV_TYPE_CPLD = "cpld"
DEV_TYPE_FPGA = "fpga"
DEV_TYPE_VOL = "vol"
DEV_TYPE_CURR = "curr"
DEV_TYPE_TEMP = "temp"
DEV_TYPE_SLOT = "slot"
DEV_TYPE_POWER = "power"
DEV_TYPE_MISC = "misc"
DEV_TYPE_BMC = "bmc"

def get_single_info_from_restful(config):
    """
    Retrieve information of a single field of a specific type

    Parameters:
        config: such as {"type" : "fan", "index" : 1, "field_name" : "present"}
    """
    if not G_RESTFUL_CLASS:
        return False, "restful interface unsupport"

    info_type = config.get("type")
    if info_type == DEV_TYPE_FAN:
        field_name = config.get("field_name")
        fan_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_fan_num()
        ret, value = G_RESTFUL_CLASS.get_fan_field_value(fan_index, field_name)
    elif info_type == DEV_TYPE_FAN_MOTOR:
        field_name = config.get("field_name")
        fan_index = config.get("index")
        motor_index = config.get("motor_index")
        ret, value = G_RESTFUL_CLASS.get_fan_motor_field_value(fan_index, motor_index, field_name)
    elif info_type == DEV_TYPE_PSU:
        field_name = config.get("field_name")
        psu_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_psu_num()
        ret, value = G_RESTFUL_CLASS.get_psu_field_value(psu_index, field_name)
    elif info_type == DEV_TYPE_PSU_TEMP:
        field_name = config.get("field_name")
        psu_index = config.get("index")
        temp_index = config.get("temp_index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_psu_temp_num(psu_index)
        ret, value = G_RESTFUL_CLASS.get_psu_temp_field_value(psu_index, temp_index, field_name)
    elif info_type == DEV_TYPE_CPLD:
        field_name = config.get("field_name")
        cpld_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_cpld_num()
        ret, value = G_RESTFUL_CLASS.get_cpld_field_value(cpld_index, field_name)
    elif info_type == DEV_TYPE_FPGA:
        field_name = config.get("field_name")
        fpga_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_fpga_num()
        ret, value = G_RESTFUL_CLASS.get_fpga_field_value(fpga_index, field_name)
    elif info_type == DEV_TYPE_VOL:
        field_name = config.get("field_name")
        vol_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_vol_num()
        ret, value = G_RESTFUL_CLASS.get_vol_field_value(vol_index, field_name)
    elif info_type == DEV_TYPE_CURR:
        field_name = config.get("field_name")
        curr_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_curr_num()
        ret, value = G_RESTFUL_CLASS.get_curr_field_value(curr_index, field_name)
    elif info_type == DEV_TYPE_TEMP:
        field_name = config.get("field_name")
        temp_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_temp_num()
        ret, value = G_RESTFUL_CLASS.get_temp_field_value(temp_index, field_name)
    elif info_type == DEV_TYPE_SLOT:
        field_name = config.get("field_name")
        slot_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_slot_num()
        ret, value = G_RESTFUL_CLASS.get_slot_field_value(slot_index, field_name)
    elif info_type == DEV_TYPE_POWER:
        field_name = config.get("field_name")
        power_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_power_num()
        ret, value = G_RESTFUL_CLASS.get_power_field_value(power_index, field_name)
    elif info_type == DEV_TYPE_MISC:
        field_name = config.get("field_name")
        misc_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_misc_num()
        ret, value = G_RESTFUL_CLASS.get_misc_field_value(misc_index, field_name)
    elif info_type == DEV_TYPE_BMC:
        field_name = config.get("field_name")
        bmc_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_bmc_num()
        ret, value = G_RESTFUL_CLASS.get_bmc_field_value(bmc_index,field_name)
    else:
        return False, "unsupport restful_info_type %s" % info_type
    return ret, str(value)

def get_multiple_info_from_restful(config):
    """
    Retrieve information of multiple field of a specific type

    Parameters:
        config: such as {"type" : "fan", "index" : 1, "field" :
                            {
                                "name":"name",
                                "presence": "present",
                            }
                        }
    """

    result_dict = {}
    field_info = config.get("field", {})
    if not isinstance(field_info, dict) or len(field_info) == 0:
        msg = "get restful info err, field config : %s err" % field_info
        return False, msg

    for field_key, field_value in field_info.items():
        single_get_config = {}
        single_get_config["type"] = config.get("type")
        single_get_config["index"] = config.get("index")
        unit = None
        value = None

        if isinstance(field_value, dict):
            unit = field_value.get("unit")
            if field_value.get("gettype"):
                field_value["index"] = config.get("index")
                ret, value = get_single_info_from_restful(field_value)
            elif field_value.get("key"):
                single_get_config["field_name"] = field_value.get("key")
                ret, value = get_single_info_from_restful(single_get_config)
            if field_value.get("decode_type"):
                ret, value = decode_value(field_value, value)
            if unit:
                value += " %s" % unit
        else:
            single_get_config["field_name"] = field_value
            ret, value = get_single_info_from_restful(single_get_config)
        result_dict[field_key] = value

    if len(result_dict) == 0:
        return False, "get restful info null, config: %s" % config
    return True, result_dict

S3IP_PREFIX_DIR_NAME = {
    DEV_TYPE_CURR: "curr_sensor",
    DEV_TYPE_TEMP: "temp_sensor",
    DEV_TYPE_VOL: "vol_sensor",
    DEV_TYPE_POWER: "power_sensor",
}

def get_multiple_info_from_s3ip(config):
    dev_type = config.get("type")
    index = config.get("index")
    prefix = f"/sys/{S3IP_SYSFS_NAME}/{S3IP_PREFIX_DIR_NAME.get(dev_type, dev_type)}/{dev_type}{index}/"
    field_info = config.get("field", {})
    if not isinstance(field_info, dict) or len(field_info) == 0:
        return False, "get s3ip info err, field config err"

    result_dict = {}
    dev_type = config.get("type")
    for field_key, field_value in field_info.items():
        info = None
        if isinstance(field_value, dict):
            if field_value.get("key"):
                ret, info = read_sysfs(prefix + field_value.get("key"))
            if field_value.get("decode_type"):
                ret, info = decode_value(field_value, info)
        else:
            if field_value == "auto_name_by_type_index":
                info = dev_type + str(config.get("index"))
            else:
                ret, info = read_sysfs(prefix + field_value)
        result_dict[field_key] = info

    if len(result_dict) == 0:
        return False, "get s3ip info null, config: %s" % config
    return True, result_dict

def get_secret_key_register_addr(config):
    way = config.get("gettype")
    if way == "devfile":
        return config.get("offset")
    else:
        return None

def supervisor_update():
    subprocess.run(["supervisorctl", "update"], check=False)
    time.sleep(1)

def check_supervisor_sock_exists():
    path = "/var/run/supervisor/supervisor.sock"
    if os.path.exists(path):
        return True
    return False

def check_supervisor_ready(timeout = 30):
    time_cnt = 0
    while True:
        ret = check_supervisor_sock_exists()
        if ret is True:
            return True
        time_cnt += 1
        if time_cnt > timeout:
            break
        time.sleep(1)
    return False

def check_supervisor_process_run_by_name(process_name):
    try:
        ret, log = exec_os_cmd("supervisorctl status %s" % process_name)
        if "RUNNING" in log:
            return True, True
        else:
            return True, False
    except Exception as e:
        pass
    return False, "supervisorctl get %s status fail" % process_name

def get_value_once(config):
    try:
        delay_time = config.get("delay", None)
        if delay_time is not None:
            time.sleep(delay_time)

        way = config.get("gettype")
        int_decode = config.get("int_decode", 16)
        if way == 'sysfs':
            loc = config.get("loc")
            ret, val = read_sysfs(loc)
            if ret is True:
                return True, int(val, int_decode)
            return False, ("sysfs read %s failed. log:%s" % (loc, val))
        if way == "i2c":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset", 0)
            ret, val = wbi2cget(bus, addr, offset)
            if ret is True:
                return True, int(val, int_decode)
            return False, ("i2c read failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
        if way == "io":
            io_addr = config.get('io_addr')
            val = io_rd(io_addr)
            if len(val) != 0:
                return True, int(val, int_decode)
            return False, ("io_addr read 0x%x failed" % io_addr)
        if way == "i2cword":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            ret, val = wbi2cgetWord(bus, addr, offset)
            if ret is True:
                return True, int(val, int_decode)
            return False, ("i2cword read failed. bus:%d, addr:0x%x, offset:0x%x" % (bus, addr, offset))
        if way == "i2cbyte":
            bus = config.get("bus")
            addr = config.get("loc")
            ret, val = wbi2cgetByte(bus, addr)
            if ret is True:
                return True, int(val, int_decode)
            return False, ("i2cbyte read failed. bus:%d, addr:0x%x" % (bus, addr))
        if way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            read_len = config.get("read_len", 1)
            ret, val_list = dev_file_read(path, offset, read_len)
            if ret is True:
                if read_len == 1:
                    val = val_list[0]
                    return True, val
                return True, val_list
            return False, ("devfile read failed. path:%s, offset:0x%x, read_len:%d" % (path, offset, read_len))
        if way == 'cmd':
            cmd = config.get("cmd")
            ret, val = exec_os_cmd(cmd)
            if ret:
                return False, ("cmd read exec %s failed, log: %s" % (cmd, val))
            return True, int(val, int_decode)
        if way == 'file_exist':
            judge_file = config.get('judge_file', None)
            if os.path.exists(judge_file):
                return True, True
            return True, False
        if way == 'pci':
            pcibus = config.get("pcibus")
            slot = config.get("slot")
            fn = config.get("fn")
            bar = config.get("bar")
            offset = config.get("offset")
            data = config.get("data")
            return wbpcird(pcibus, slot, fn, bar, offset, data)
        if way == 'peci_pcicfglocal_rd':
            bus = config.get("bus")
            dev = config.get("dev")
            func = config.get("func")
            reg = config.get("reg")
            rd_len = config.get("rd_len")
            ret, val = wb_peci_pcicfglocal_rd(bus, dev, func, reg, rd_len)
            return ret, val
        if way == 'bit_rd':
            rd_config = config.get("rd_config")
            rd_bit = config.get("rd_bit")
            ret, rd_value = get_value_once(rd_config)
            if ret is False:
                return False, ("bit_rd read failed, log: %s" % rd_value)
            val = (rd_value & (1 << rd_bit)) >> rd_bit
            return True, val
        if way == 'secret_key':
            secret_key_config = config.get("secret_key_config")
            if secret_key_config.get("gettype") != "devfile":
                return False, "secret_key read only support gettype=devfile"
            ret, ori_value = get_value_once(secret_key_config)
            if ret is False:
                return False, ("secret_key read failed, log: %s" % ori_value)
            decode_func = config.get("decode_func", INVALID_DECODE_FUNC)
            addr = get_secret_key_register_addr(secret_key_config)
            if addr is None:
                return False, ("get_secret_key_register_addr failed")
            ret, val = secret_key_decode(decode_func, ori_value, addr, 0)
            if ret == False:
                return False, val
            return True, val
        if way == 'cmd_str':
            cmd = config.get("cmd")
            ret, val = exec_os_cmd(cmd)
            if ret:
                return False, ("cmd read exec %s failed, log: %s" % (cmd, val))
            else:
                return True, val
        if way == 'gpio':
            return _get_gpio_value(config)
        return False, ("%s is not support read type" % way)
    except Exception as e:
        return False, ("get_value_once exception:%s happen" % str(e))


GPIO_MAX_NAME_SIZE = 32
GPIOHANDLES_MAX = 64
GPIO_V2_LINES_MAX = 64

GPIOHANDLE_REQUEST_INPUT = (1 << 0)
GPIOHANDLE_REQUEST_OUTPUT = (1 << 1)
GPIOHANDLE_REQUEST_ACTIVE_LOW = (1 << 2)

GPIO_DIRECTION_INPUT = "input"
GPIO_DIRECTION_OUTPUT = "output"

GPIO_V2_LINE_FLAG_USED = (1 << 0)
GPIO_V2_LINE_FLAG_ACTIVE_LOW = (1 << 1)
GPIO_V2_LINE_FLAG_INPUT = (1 << 2)
GPIO_V2_LINE_FLAG_OUTPUT = (1 << 3)

GPIO_V2_LINE_ATTR_ID_FLAGS = 1
GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES = 2

_GPIO_IOC_NRBITS = 8
_GPIO_IOC_TYPEBITS = 8
_GPIO_IOC_SIZEBITS = 14
_GPIO_IOC_NRSHIFT = 0
_GPIO_IOC_TYPESHIFT = _GPIO_IOC_NRSHIFT + _GPIO_IOC_NRBITS
_GPIO_IOC_SIZESHIFT = _GPIO_IOC_TYPESHIFT + _GPIO_IOC_TYPEBITS
_GPIO_IOC_DIRSHIFT = _GPIO_IOC_SIZESHIFT + _GPIO_IOC_SIZEBITS
_GPIO_IOC_READ = 2
_GPIO_IOC_WRITE = 1


class _GPIOChipInfo(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char * GPIO_MAX_NAME_SIZE),
        ("label", ctypes.c_char * GPIO_MAX_NAME_SIZE),
        ("lines", ctypes.c_uint32),
    ]


class _GPIOLineInfo(ctypes.Structure):
    _fields_ = [
        ("line_offset", ctypes.c_uint32),
        ("flags", ctypes.c_uint32),
        ("name", ctypes.c_char * GPIO_MAX_NAME_SIZE),
        ("consumer", ctypes.c_char * GPIO_MAX_NAME_SIZE),
    ]


class _GPIOHandleRequest(ctypes.Structure):
    _fields_ = [
        ("lineoffsets", ctypes.c_uint32 * GPIOHANDLES_MAX),
        ("flags", ctypes.c_uint32),
        ("default_values", ctypes.c_uint8 * GPIOHANDLES_MAX),
        ("consumer_label", ctypes.c_char * GPIO_MAX_NAME_SIZE),
        ("lines", ctypes.c_uint32),
        ("fd", ctypes.c_int),
    ]


class _GPIOHandleData(ctypes.Structure):
    _fields_ = [
        ("values", ctypes.c_uint8 * GPIOHANDLES_MAX),
    ]


class _GPIOV2LineValues(ctypes.Structure):
    _fields_ = [
        ("bits", ctypes.c_uint64),
        ("mask", ctypes.c_uint64),
    ]


class _GPIOV2LineAttribute(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32),
        ("padding", ctypes.c_uint32),
        ("value", ctypes.c_uint64),
    ]


class _GPIOV2LineConfigAttribute(ctypes.Structure):
    _fields_ = [
        ("attr", _GPIOV2LineAttribute),
        ("mask", ctypes.c_uint64),
    ]


class _GPIOV2LineConfig(ctypes.Structure):
    _fields_ = [
        ("flags", ctypes.c_uint64),
        ("num_attrs", ctypes.c_uint32),
        ("padding", ctypes.c_uint32 * 5),
        ("attrs", _GPIOV2LineConfigAttribute * 10),
    ]


class _GPIOV2LineRequest(ctypes.Structure):
    _fields_ = [
        ("offsets", ctypes.c_uint32 * GPIO_V2_LINES_MAX),
        ("consumer", ctypes.c_char * GPIO_MAX_NAME_SIZE),
        ("config", _GPIOV2LineConfig),
        ("num_lines", ctypes.c_uint32),
        ("event_buffer_size", ctypes.c_uint32),
        ("padding", ctypes.c_uint32 * 5),
        ("fd", ctypes.c_int32),
    ]


def _gpio_ioc(direction, type_num, nr, size):
    """Build a raw ioctl number for the GPIO character device ABI."""
    return ((direction << _GPIO_IOC_DIRSHIFT) |
            (type_num << _GPIO_IOC_TYPESHIFT) |
            (nr << _GPIO_IOC_NRSHIFT) |
            (size << _GPIO_IOC_SIZESHIFT))


def _gpio_ior(type_num, nr, data_type):
    """Build a read-only GPIO ioctl number."""
    return _gpio_ioc(_GPIO_IOC_READ, type_num, nr, ctypes.sizeof(data_type))


def _gpio_iowr(type_num, nr, data_type):
    """Build a read-write GPIO ioctl number."""
    return _gpio_ioc(_GPIO_IOC_READ | _GPIO_IOC_WRITE, type_num, nr, ctypes.sizeof(data_type))


GPIO_GET_CHIPINFO_IOCTL = _gpio_ior(0xB4, 0x01, _GPIOChipInfo)
GPIO_GET_LINEINFO_IOCTL = _gpio_iowr(0xB4, 0x02, _GPIOLineInfo)
GPIO_GET_LINEHANDLE_IOCTL = _gpio_iowr(0xB4, 0x03, _GPIOHandleRequest)
GPIO_V2_GET_LINE_IOCTL = _gpio_iowr(0xB4, 0x07, _GPIOV2LineRequest)
GPIOHANDLE_GET_LINE_VALUES_IOCTL = _gpio_iowr(0xB4, 0x08, _GPIOHandleData)
GPIO_V2_LINE_GET_VALUES_IOCTL = _gpio_iowr(0xB4, 0x0E, _GPIOV2LineValues)


def _gpio_has_cli_tools(config, is_write):
    """Check whether the legacy libgpiod CLI tools are available."""
    required_tools = ["gpioset"] if is_write else ["gpioget"]
    if config.get("line_name") is not None:
        required_tools.append("gpiofind")
    return all(shutil.which(tool_name) for tool_name in required_tools)


def _gpio_get_config_value(config):
    """Validate and return the GPIO output value from config."""
    gpio_value = config.get("value")

    if gpio_value is None:
        return False, "gpio config missing value"
    if gpio_value not in (0, 1):
        return False, ("unsupported gpio value: %s" % gpio_value)

    return True, gpio_value


def _gpio_get_target_by_chip_offset(gpio_chip, gpio_offset):
    """Validate the chip+offset addressing form used by the GPIO helpers."""
    if not isinstance(gpio_chip, str) or len(gpio_chip.strip()) == 0:
        return False, "gpio config chip must be a non-empty string"
    if not isinstance(gpio_offset, int):
        return False, ("unsupported gpio offset: %s" % gpio_offset)

    return True, {
        "chip": gpio_chip.strip(),
        "offset": gpio_offset,
    }


def _gpio_resolve_line_name_by_cli(gpio_line_name):
    """Resolve a GPIO line name through gpiofind."""
    cmd = "gpiofind %s" % shlex.quote(str(gpio_line_name))
    ret, log = exec_os_cmd(cmd)
    if ret:
        return False, ("gpio resolve line %s failed, log: %s" % (gpio_line_name, log))

    gpiofind_output = log.strip().splitlines()
    if len(gpiofind_output) == 0:
        return False, ("gpio resolve line %s failed: empty gpiofind output" % gpio_line_name)

    gpiofind_tokens = gpiofind_output[-1].replace(":", " ").replace(",", " ").split()
    if len(gpiofind_tokens) < 2:
        return False, ("gpio resolve line %s failed: unexpected gpiofind output %s" %
                       (gpio_line_name, log.strip()))

    resolved_chip = gpiofind_tokens[0]
    try:
        resolved_offset = int(gpiofind_tokens[-1], 10)
    except (TypeError, ValueError):
        return False, ("gpio resolve line %s failed: unexpected gpiofind output %s" %
                       (gpio_line_name, log.strip()))

    return True, {
        "chip": resolved_chip,
        "offset": resolved_offset,
    }


def _gpio_get_target_by_cli(config):
    """Resolve the legacy CLI GPIO target from config."""
    gpio_chip = config.get("chip")
    gpio_offset = config.get("offset")
    gpio_line_name = config.get("line_name")

    has_line_name = gpio_line_name is not None
    has_chip = gpio_chip is not None
    has_offset = gpio_offset is not None

    # Temporarily disable the line_name path and only allow chip+offset.
    # if has_line_name:
    #     if not isinstance(gpio_line_name, str) or len(gpio_line_name.strip()) == 0:
    #         return False, "gpio config line_name must be a non-empty string"
    #     return _gpio_resolve_line_name_by_cli(gpio_line_name.strip())
    if not has_chip or not has_offset:
        return False, "gpio config must use chip+offset"
    return _gpio_get_target_by_chip_offset(gpio_chip, gpio_offset)


def _gpio_build_set_cmd(gpio_target, gpio_value, mode, active_low):
    """Build the gpioset command line for the target line."""
    cmd_args = [
        "gpioset",
        "--mode=%s" % mode,
    ]
    if active_low:
        cmd_args.append("--active-low")
    cmd_args.extend([
        gpio_target["chip"],
        "%s=%d" % (gpio_target["offset"], gpio_value),
    ])
    return " ".join(shlex.quote(str(arg)) for arg in cmd_args)


def _gpio_build_get_cmd(gpio_target):
    """Build the gpioget command line for the target line."""
    cmd_args = [
        "gpioget",
        gpio_target["chip"],
        str(gpio_target["offset"]),
    ]
    return " ".join(shlex.quote(str(arg)) for arg in cmd_args)


def _get_gpio_value_by_cli(config):
    """Read a GPIO value through the legacy libgpiod CLI tools."""
    ret, gpio_target = _gpio_get_target_by_cli(config)
    if ret is not True:
        return False, gpio_target

    cmd = _gpio_build_get_cmd(gpio_target)
    ret, log = exec_os_cmd(cmd)
    if ret:
        return False, ("gpio read exec %s failed, log: %s" % (cmd, log))

    gpio_value = log.strip()
    if gpio_value not in ("0", "1"):
        return False, ("gpio read exec %s failed: unexpected output %s" % (cmd, log.strip()))

    return True, int(gpio_value)


def _set_gpio_value_by_cli(config):
    """Write a GPIO value through the legacy libgpiod CLI tools."""
    mode = config.get("mode", "exit")
    active_low = config.get("active_low", False)

    ret, gpio_target = _gpio_get_target_by_cli(config)
    if ret is not True:
        return False, gpio_target

    ret, gpio_value = _gpio_get_config_value(config)
    if ret is not True:
        return False, gpio_value

    cmd = _gpio_build_set_cmd(gpio_target, gpio_value, mode, active_low)
    ret, log = exec_os_cmd(cmd)
    if ret:
        return False, ("gpio write exec %s failed, log: %s" % (cmd, log))
    return True, ("gpio write exec %s success" % cmd)


def _gpio_decode_string(raw_value):
    """Decode a fixed-size kernel C string field into Python text."""
    return bytes(raw_value).split(b"\0", 1)[0].decode("utf-8", "ignore")


def _gpio_debug_log(message):
    """Emit GPIO debug logs only when the GPIO debug flag file exists."""
    if not os.path.exists(PLATFORM_UTIL_GPIO_DEBUG_FILE):
        return
    print("[platform_util][gpio] %s" % message)


def _gpio_ioctl(fd, ioctl_num, data):
    """Execute a GPIO ioctl and copy the updated structure back into ctypes."""
    if fd is None or not isinstance(fd, int) or fd < 0:
        raise ValueError("gpio ioctl invalid fd: %s" % fd)
    if data is None:
        raise ValueError("gpio ioctl data must not be None")

    _gpio_debug_log("ioctl begin fd=%s ioctl=0x%x type=%s" % (fd, ioctl_num, type(data).__name__))
    buffer = bytearray(ctypes.string_at(ctypes.addressof(data), ctypes.sizeof(data)))
    try:
        fcntl.ioctl(fd, ioctl_num, buffer, True)
    except OSError as e:
        error_message = ("gpio ioctl failed fd=%s ioctl=0x%x type=%s errno=%s error=%s" %
                         (fd, ioctl_num, type(data).__name__, getattr(e, "errno", None), str(e)))
        _gpio_debug_log(error_message)
        raise OSError(error_message)
    except (TypeError, ValueError) as e:
        error_message = ("gpio ioctl invalid arguments fd=%s ioctl=0x%x type=%s error=%s" %
                         (fd, ioctl_num, type(data).__name__, str(e)))
        _gpio_debug_log(error_message)
        raise ValueError(error_message)

    ctypes.memmove(ctypes.addressof(data), bytes(buffer), ctypes.sizeof(data))
    _gpio_debug_log("ioctl end fd=%s ioctl=0x%x type=%s" % (fd, ioctl_num, type(data).__name__))
    return data


def _gpio_get_chip_info(chip_fd):
    """Read chip metadata from a gpiochip fd."""
    try:
        chip_info = _GPIOChipInfo()
        _gpio_ioctl(chip_fd, GPIO_GET_CHIPINFO_IOCTL, chip_info)
        decoded_chip_info = {
            "name": _gpio_decode_string(chip_info.name),
            "label": _gpio_decode_string(chip_info.label),
            "lines": chip_info.lines,
        }
        _gpio_debug_log("chip info fd=%s info=%s" % (chip_fd, decoded_chip_info))
        return True, decoded_chip_info
    except (OSError, ValueError) as e:
        _gpio_debug_log("chip info failed fd=%s error=%s" % (chip_fd, str(e)))
        return False, ("gpio get chip info failed: %s" % str(e))


def _gpio_get_line_info(chip_fd, line_offset):
    """Read metadata for one GPIO line on a chip."""
    try:
        line_info = _GPIOLineInfo()
        line_info.line_offset = line_offset
        _gpio_ioctl(chip_fd, GPIO_GET_LINEINFO_IOCTL, line_info)
        decoded_line_info = {
            "offset": line_info.line_offset,
            "name": _gpio_decode_string(line_info.name),
            "consumer": _gpio_decode_string(line_info.consumer),
            "flags": line_info.flags,
        }
        _gpio_debug_log("line info fd=%s offset=%s info=%s" % (chip_fd, line_offset, decoded_line_info))
        return True, decoded_line_info
    except (OSError, ValueError) as e:
        _gpio_debug_log("line info failed fd=%s offset=%s error=%s" % (chip_fd, line_offset, str(e)))
        return False, ("gpio get line info failed for offset %s: %s" % (line_offset, str(e)))


def _gpio_iter_chip_paths():
    """List all gpiochip character devices available on the system."""
    chip_paths = sorted(glob.glob("/dev/gpiochip*"))
    if len(chip_paths) == 0:
        _gpio_debug_log("iter chip paths found none")
        return False, "gpio chardev not found under /dev/gpiochip*"
    _gpio_debug_log("iter chip paths found=%s" % chip_paths)
    return True, chip_paths


def _gpio_open_chip_by_path(chip_path):
    """Open one gpiochip path and attach decoded chip information."""
    try:
        chip_fd = os.open(chip_path, os.O_RDONLY)
        _gpio_debug_log("open chip path=%s fd=%s" % (chip_path, chip_fd))
    except OSError as e:
        _gpio_debug_log("open chip failed path=%s error=%s" % (chip_path, str(e)))
        return False, ("open gpio chip %s failed: %s" % (chip_path, str(e)))

    ret, chip_info = _gpio_get_chip_info(chip_fd)
    if ret is not True:
        os.close(chip_fd)
        _gpio_debug_log("open chip path=%s failed during chip info" % chip_path)
        return False, chip_info

    _gpio_debug_log("open chip success path=%s fd=%s chip=%s" % (chip_path, chip_fd, chip_info))
    return True, {
        "fd": chip_fd,
        "path": chip_path,
        "info": chip_info,
    }


def _gpio_chip_identifier_matches(chip_identifier, chip_path, chip_info):
    """Check whether a chip identifier matches path, basename, name or label."""
    chip_identifier = chip_identifier.strip()
    return chip_identifier in (
        chip_path,
        os.path.basename(chip_path),
        chip_info.get("name"),
        chip_info.get("label"),
    )


def _gpio_open_chip_by_identifier(gpio_chip):
    """Resolve and open a gpiochip by identifier."""
    if not isinstance(gpio_chip, str) or len(gpio_chip.strip()) == 0:
        return False, "gpio config chip must be a non-empty string"

    gpio_chip = gpio_chip.strip()
    _gpio_debug_log("open chip by identifier identifier=%s" % gpio_chip)
    ret, chip_paths = _gpio_iter_chip_paths()
    if ret is not True:
        return False, chip_paths

    for chip_path in chip_paths:
        ret, chip_ref = _gpio_open_chip_by_path(chip_path)
        if ret is not True:
            _gpio_debug_log("skip chip path=%s because open failed" % chip_path)
            continue
        if _gpio_chip_identifier_matches(gpio_chip, chip_path, chip_ref["info"]):
            _gpio_debug_log("matched chip identifier=%s path=%s info=%s" % (gpio_chip, chip_path, chip_ref["info"]))
            return True, chip_ref
        os.close(chip_ref["fd"])
        _gpio_debug_log("chip identifier=%s not matched path=%s" % (gpio_chip, chip_path))

    _gpio_debug_log("chip identifier=%s not found" % gpio_chip)
    return False, ("gpio chip %s not found" % gpio_chip)


def _gpio_find_line_offset_on_chip(chip_fd, chip_info, gpio_line_name):
    """Search one gpiochip for a named line and return its offset."""
    _gpio_debug_log("find line offset begin chip=%s line_name=%s lines=%s" %
                    (chip_info.get("name"), gpio_line_name, chip_info.get("lines")))
    for offset in range(chip_info["lines"]):
        ret, line_info = _gpio_get_line_info(chip_fd, offset)
        if ret is not True:
            _gpio_debug_log("find line offset failed chip=%s offset=%s error=%s" %
                            (chip_info.get("name"), offset, line_info))
            return False, line_info
        if line_info["name"] == gpio_line_name:
            _gpio_debug_log("find line offset success chip=%s line_name=%s offset=%s" %
                            (chip_info.get("name"), gpio_line_name, offset))
            return True, offset

    _gpio_debug_log("find line offset miss chip=%s line_name=%s" % (chip_info.get("name"), gpio_line_name))
    return False, ("gpio line %s not found on chip %s" % (gpio_line_name, chip_info.get("name")))


def _gpio_get_target_by_line_name_by_ioctl(gpio_line_name):
    """Resolve a line name across gpiochips through the ioctl backend."""
    if not isinstance(gpio_line_name, str) or len(gpio_line_name.strip()) == 0:
        return False, "gpio config line_name must be a non-empty string"

    gpio_line_name = gpio_line_name.strip()
    _gpio_debug_log("resolve line name by ioctl line_name=%s" % gpio_line_name)
    ret, chip_paths = _gpio_iter_chip_paths()
    if ret is not True:
        return False, chip_paths

    matched_target = None
    for chip_path in chip_paths:
        ret, chip_ref = _gpio_open_chip_by_path(chip_path)
        if ret is not True:
            continue
        try:
            ret, resolved_offset = _gpio_find_line_offset_on_chip(chip_ref["fd"], chip_ref["info"], gpio_line_name)
            if ret is not True:
                continue
            current_target = {
                "chip_path": chip_ref["path"],
                "chip_name": chip_ref["info"]["name"],
                "offset": resolved_offset,
            }
            _gpio_debug_log("resolve line candidate=%s" % current_target)
            if matched_target is not None:
                _gpio_debug_log("resolve line ambiguous previous=%s current=%s" % (matched_target, current_target))
                return False, ("gpio line %s is ambiguous across chips %s and %s" %
                               (gpio_line_name, matched_target["chip_name"], current_target["chip_name"]))
            matched_target = current_target
        finally:
            os.close(chip_ref["fd"])

    if matched_target is None:
        _gpio_debug_log("resolve line failed line_name=%s" % gpio_line_name)
        return False, ("gpio line %s not found" % gpio_line_name)
    _gpio_debug_log("resolve line success line_name=%s target=%s" % (gpio_line_name, matched_target))
    return True, matched_target


def _gpio_get_target_by_chip_offset_ioctl(gpio_chip, gpio_offset):
    """Resolve chip+offset addressing into the concrete ioctl target."""
    if not isinstance(gpio_offset, int):
        return False, ("unsupported gpio offset: %s" % gpio_offset)

    _gpio_debug_log("resolve chip+offset by ioctl chip=%s offset=%s" % (gpio_chip, gpio_offset))
    ret, chip_ref = _gpio_open_chip_by_identifier(gpio_chip)
    if ret is not True:
        return False, chip_ref

    try:
        if gpio_offset < 0 or gpio_offset >= chip_ref["info"]["lines"]:
            _gpio_debug_log("resolve chip+offset out of range chip=%s offset=%s lines=%s" %
                            (chip_ref["info"]["name"], gpio_offset, chip_ref["info"]["lines"]))
            return False, ("gpio offset %s out of range on chip %s" %
                           (gpio_offset, chip_ref["info"]["name"]))
        gpio_target = {
            "chip_path": chip_ref["path"],
            "chip_name": chip_ref["info"]["name"],
            "offset": gpio_offset,
        }
        _gpio_debug_log("resolve chip+offset success target=%s" % gpio_target)
        return True, gpio_target
    finally:
        os.close(chip_ref["fd"])


def _gpio_get_target_by_ioctl(config):
    """Resolve the ioctl GPIO target from config."""
    gpio_chip = config.get("chip")
    gpio_offset = config.get("offset")
    gpio_line_name = config.get("line_name")

    has_line_name = gpio_line_name is not None
    has_chip = gpio_chip is not None
    has_offset = gpio_offset is not None

    _gpio_debug_log("get target by ioctl config=%s" % config)
    # The current GPIO policy is intentionally narrowed to chip+offset.
    # Keep the line_name path commented here so it can be restored later
    # without needing to rediscover the older implementation.
    # if has_line_name:
    #     _gpio_debug_log("get target by ioctl use line_name path")
    #     return _gpio_get_target_by_line_name_by_ioctl(gpio_line_name)
    if not has_chip or not has_offset:
        return False, "gpio config must use chip+offset"
    _gpio_debug_log("get target by ioctl use chip+offset path")
    return _gpio_get_target_by_chip_offset_ioctl(gpio_chip, gpio_offset)


def _gpio_get_consumer(config):
    """Return a safe consumer label for GPIO requests."""
    consumer = config.get("consumer", "platform_util")
    if not isinstance(consumer, str) or len(consumer.strip()) == 0:
        return "platform_util"
    return consumer.strip()


def _gpio_get_config_ioctl_mode(config):
    """Validate the supported ioctl write mode."""
    mode = config.get("mode", "exit")
    _gpio_debug_log("get ioctl mode mode=%s" % mode)
    if mode != "exit":
        return False, "gpio ioctl backend only supports mode=exit"
    return True, mode


def _gpio_get_config_read_direction(config):
    """Validate the requested GPIO read direction policy."""
    read_direction = config.get("read_direction")
    if read_direction not in (GPIO_DIRECTION_INPUT, GPIO_DIRECTION_OUTPUT):
        return False, "gpio config must set read_direction to input or output"
    return True, read_direction


'''requested direction'''
def _gpio_get_request_flags(direction, active_low):
    """Translate the v1 GPIO direction and polarity into request flags."""
    if direction == GPIO_DIRECTION_INPUT:
        flags = GPIOHANDLE_REQUEST_INPUT
    elif direction == GPIO_DIRECTION_OUTPUT:
        flags = GPIOHANDLE_REQUEST_OUTPUT
    else:
        raise ValueError("unsupported gpio direction: %s" % direction)

    if active_low:
        flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW
    _gpio_debug_log("get request flags direction=%s active_low=%s flags=0x%x" % (direction, active_low, flags))
    return flags


'''keep direction'''
def _gpio_get_request_flags_v2(direction, active_low):
    """Translate the GPIO read policy into v2 line request flags."""
    flags = 0
    if direction == GPIO_DIRECTION_INPUT:
        # For input reads we must request the line explicitly as input.
        flags |= GPIO_V2_LINE_FLAG_INPUT
    elif direction == GPIO_DIRECTION_OUTPUT:
        # For output reads, do not force the line back through output request
        # configuration. Leaving direction unset asks the v2 ABI to request the
        # line "as is", so we can read the current driven level without changing
        # the existing output state.
        pass
    else:
        raise ValueError("unsupported gpio direction: %s" % direction)

    if active_low:
        flags |= GPIO_V2_LINE_FLAG_ACTIVE_LOW

    _gpio_debug_log("get v2 line flags direction=%s active_low=%s flags=0x%x" %
                    (direction, active_low, flags))
    return flags


def _gpio_encode_consumer_label(consumer):
    """Encode and truncate the consumer label for kernel request structs."""
    return consumer.encode("utf-8")[:GPIO_MAX_NAME_SIZE - 1]


def _gpio_open_chip_fd(gpio_target, operation_name):
    """Open the gpiochip file for one GPIO operation."""
    chip_fd = os.open(gpio_target["chip_path"], os.O_RDONLY)
    _gpio_debug_log("%s opened chip path=%s fd=%s" % (operation_name, gpio_target["chip_path"], chip_fd))
    return chip_fd


def _gpio_close_fd(fd, fd_name, operation_name):
    """Close one GPIO-related fd with matching debug output."""
    if fd is None:
        return
    os.close(fd)
    _gpio_debug_log("%s closed %s=%s" % (operation_name, fd_name, fd))


def _gpio_request_line_handle_for_requested_direction(chip_fd, gpio_target, direction, active_low, consumer,
                                                      default_value=None):
    """Request a line handle using the explicitly requested direction."""
    request = _GPIOHandleRequest()
    request.lineoffsets[0] = gpio_target["offset"]
    request.flags = _gpio_get_request_flags(direction, active_low)
    request.lines = 1
    request.consumer_label = _gpio_encode_consumer_label(consumer)
    if default_value is not None:
        # In the v1 ABI the output value is applied during the request itself.
        # There is no separate "request as output but preserve current value"
        # mode, which is why v1 cannot safely read back an existing output level
        # without potentially changing it.
        request.default_values[0] = default_value
    _gpio_ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, request)
    return request.fd


def _gpio_request_line_handle_keep_state(chip_fd, gpio_target, direction, active_low, consumer):
    """Request a line handle without re-driving an already configured output."""
    request = _GPIOV2LineRequest()
    request.offsets[0] = gpio_target["offset"]
    request.consumer = _gpio_encode_consumer_label(consumer)
    # v2 lets us leave direction unset for output readback so the kernel keeps
    # the existing line state instead of re-driving the pin with a default value.
    request.config.flags = _gpio_get_request_flags_v2(direction, active_low)
    request.num_lines = 1
    _gpio_ioctl(chip_fd, GPIO_V2_GET_LINE_IOCTL, request)
    return request.fd


def _gpio_get_line_value_keep_state(gpio_target, direction, active_low, consumer):
    """Read one GPIO value while keeping the current output state unchanged."""
    chip_fd = None
    line_fd = None
    operation_name = "get line value via ioctl v2"
    try:
        _gpio_debug_log("%s begin target=%s direction=%s active_low=%s consumer=%s" %
                        (operation_name, gpio_target, direction, active_low, consumer))
        chip_fd = _gpio_open_chip_fd(gpio_target, operation_name)
        line_fd = _gpio_request_line_handle_keep_state(chip_fd, gpio_target, direction, active_low, consumer)
        _gpio_debug_log("%s request success target=%s line_fd=%s" % (operation_name, gpio_target, line_fd))

        line_values = _GPIOV2LineValues()
        # Only one line is requested here, so bit0/mask0 always corresponds to
        # the requested GPIO offset.
        line_values.mask = 0x1
        _gpio_ioctl(line_fd, GPIO_V2_LINE_GET_VALUES_IOCTL, line_values)
        gpio_value = 1 if (line_values.bits & 0x1) else 0
        _gpio_debug_log("%s success target=%s direction=%s value=%s" %
                        (operation_name, gpio_target, direction, gpio_value))
        return True, gpio_value
    except (OSError, ValueError) as e:
        _gpio_debug_log("%s failed target=%s direction=%s error=%s" %
                        (operation_name, gpio_target, direction, str(e)))
        return False, ("gpio get line value via ioctl v2 failed for %s offset %s: %s" %
                       (gpio_target["chip_path"], gpio_target["offset"], str(e)))
    finally:
        _gpio_close_fd(line_fd, "line fd", operation_name)
        _gpio_close_fd(chip_fd, "chip fd", operation_name)


def _gpio_get_line_value_by_requested_direction(gpio_target, direction, active_low, consumer):
    """Read one GPIO value after requesting the line with an explicit direction."""
    chip_fd = None
    line_fd = None
    operation_name = "get line value"
    try:
        _gpio_debug_log("%s begin target=%s direction=%s active_low=%s consumer=%s" %
                        (operation_name, gpio_target, direction, active_low, consumer))
        chip_fd = _gpio_open_chip_fd(gpio_target, operation_name)
        line_fd = _gpio_request_line_handle_for_requested_direction(
            chip_fd, gpio_target, direction, active_low, consumer)
        _gpio_debug_log("%s request success target=%s line_fd=%s" % (operation_name, gpio_target, line_fd))

        line_data = _GPIOHandleData()
        # The v1 handle fd is the object that actually carries the requested
        # line, so value reads must go through line_fd rather than chip_fd.
        _gpio_ioctl(line_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, line_data)
        gpio_value = int(line_data.values[0])
        _gpio_debug_log("%s success target=%s direction=%s value=%s" %
                        (operation_name, gpio_target, direction, gpio_value))
        return True, gpio_value
    except (OSError, ValueError) as e:
        _gpio_debug_log("%s failed target=%s direction=%s error=%s" %
                        (operation_name, gpio_target, direction, str(e)))
        return False, ("gpio get line value failed for %s offset %s: %s" %
                       (gpio_target["chip_path"], gpio_target["offset"], str(e)))
    finally:
        _gpio_close_fd(line_fd, "line fd", operation_name)
        _gpio_close_fd(chip_fd, "chip fd", operation_name)


def _gpio_set_line_value_via_ioctl(gpio_target, gpio_value, active_low, consumer):
    """Drive one GPIO line through the legacy v1 ioctl ABI."""
    chip_fd = None
    line_fd = None
    operation_name = "set line value"
    try:
        _gpio_debug_log("%s begin target=%s active_low=%s consumer=%s value=%s" %
                        (operation_name, gpio_target, active_low, consumer, gpio_value))
        chip_fd = _gpio_open_chip_fd(gpio_target, operation_name)
        # For v1 writes the request call itself drives the line to gpio_value.
        # We keep the returned line fd only so the handle remains valid until the
        # function completes, then close it immediately in finally.
        line_fd = _gpio_request_line_handle_for_requested_direction(
            chip_fd, gpio_target, GPIO_DIRECTION_OUTPUT, active_low, consumer, gpio_value)
        _gpio_debug_log("%s success target=%s value=%s line_fd=%s" %
                        (operation_name, gpio_target, gpio_value, line_fd))
        return True, ("gpio write %s offset %s value %s success" %
                      (gpio_target["chip_path"], gpio_target["offset"], gpio_value))
    except (OSError, ValueError) as e:
        _gpio_debug_log("%s failed target=%s value=%s error=%s" %
                        (operation_name, gpio_target, gpio_value, str(e)))
        return False, ("gpio set line value failed for %s offset %s: %s" %
                       (gpio_target["chip_path"], gpio_target["offset"], str(e)))
    finally:
        _gpio_close_fd(line_fd, "line fd", operation_name)
        _gpio_close_fd(chip_fd, "chip fd", operation_name)


def _get_gpio_value_by_ioctl(config):
    """Read one GPIO value through the ioctl backend."""
    active_low = config.get("active_low", False)
    consumer = _gpio_get_consumer(config)
    _gpio_debug_log("get value by ioctl begin config=%s" % config)
    ret, mode = _gpio_get_config_ioctl_mode(config)
    if ret is not True:
        _gpio_debug_log("get value by ioctl mode check failed error=%s" % mode)
        return False, mode

    ret, gpio_target = _gpio_get_target_by_ioctl(config)
    if ret is not True:
        _gpio_debug_log("get value by ioctl target resolve failed error=%s" % gpio_target)
        return False, gpio_target

    ret, read_direction = _gpio_get_config_read_direction(config)
    if ret is not True:
        _gpio_debug_log("get value by ioctl read_direction check failed error=%s" % read_direction)
        return False, read_direction

    # Prefer v2 first because it can read both input levels and current output
    # levels without disturbing an already-driven output line.
    ret, gpio_value = _gpio_get_line_value_keep_state(gpio_target, read_direction, active_low, consumer)
    if ret is True:
        return True, gpio_value

    if read_direction == GPIO_DIRECTION_INPUT:
        # v1 fallback is only safe for input reads. For output reads, v1 request
        # semantics can change the current output level during the request.
        _gpio_debug_log("get value by ioctl fallback to v1 input path because v2 read failed error=%s" % gpio_value)
        return _gpio_get_line_value_by_requested_direction(gpio_target, read_direction, active_low, consumer)

    return False, gpio_value


def _set_gpio_value_by_ioctl(config):
    """Write one GPIO value through the ioctl backend."""
    active_low = config.get("active_low", False)
    consumer = _gpio_get_consumer(config)
    _gpio_debug_log("set value by ioctl begin config=%s" % config)

    ret, mode = _gpio_get_config_ioctl_mode(config)
    if ret is not True:
        _gpio_debug_log("set value by ioctl mode check failed error=%s" % mode)
        return False, mode

    ret, gpio_target = _gpio_get_target_by_ioctl(config)
    if ret is not True:
        _gpio_debug_log("set value by ioctl target resolve failed error=%s" % gpio_target)
        return False, gpio_target

    ret, gpio_value = _gpio_get_config_value(config)
    if ret is not True:
        _gpio_debug_log("set value by ioctl value check failed error=%s" % gpio_value)
        return False, gpio_value

    # Writes still use the simple v1 handle request path because request-time
    # default_values already matches the required "set and release" behavior.
    return _gpio_set_line_value_via_ioctl(gpio_target, gpio_value, active_low, consumer)


def _get_gpio_value(config):
    """Entry point for GPIO reads using the currently enabled backend policy."""
    ret, read_direction = _gpio_get_config_read_direction(config)
    if ret is not True:
        return False, read_direction

    # Temporarily disable the gpio CLI path and force all reads through ioctl.
    # if _gpio_has_cli_tools(config, False):
    #     return _get_gpio_value_by_cli(config)
    # else:
    #     return _get_gpio_value_by_ioctl(config)
    return _get_gpio_value_by_ioctl(config)


def _set_gpio_value(config):
    """Entry point for GPIO writes using the currently enabled backend policy."""
    # Temporarily disable the gpio CLI path and force all writes through ioctl.
    # if _gpio_has_cli_tools(config, True):
    #     return _set_gpio_value_by_cli(config)
    # else:
    #     return _set_gpio_value_by_ioctl(config)
    return _set_gpio_value_by_ioctl(config)


def set_value_once(config):
    try:
        delay_time = config.get("delay", None)
        if delay_time is not None:
            time.sleep(delay_time)

        way = config.get("gettype")
        if way == 'sysfs':
            loc = config.get("loc")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            mask_tuple = (0xff, 0)
            if mask not in mask_tuple:
                ret, read_value = read_sysfs(loc)
                if ret is True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("sysfs read %s failed. log:%s" % (loc, read_value))
            ret, log = write_sysfs(loc, "0x%02x" % value)
            if ret is not True:
                return False, ("sysfs %s write 0x%x failed" % (loc, value))
            return True, ("sysfs write 0x%x success" % value)
        if way == "i2c":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            mask_tuple = (0xff, 0)
            if mask not in mask_tuple:
                ret, read_value = wbi2cget(bus, addr, offset)
                if ret is True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("i2c read failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
            ret, log = wbi2cset(bus, addr, offset, value)
            if ret is not True:
                return False, ("i2c write bus:%d, addr:0x%x, offset:0x%x, value:0x%x failed" %
                               (bus, addr, offset, value))
            return True, ("i2c write bus:%d, addr:0x%x, offset:0x%x, value:0x%x success" %
                          (bus, addr, offset, value))
        if way == "io":
            io_addr = config.get('io_addr')
            value = config.get('value')
            mask = config.get("mask", 0xff)
            mask_tuple = (0xff, 0)
            if mask not in mask_tuple:
                read_value = io_rd(io_addr)
                if read_value is None:
                    return False, ("io_addr 0x%x read failed" % (io_addr))
                read_value = int(read_value, base=16)
                value = (read_value & mask) | value
            ret = io_wr(io_addr, value)
            if ret is not True:
                return False, ("io_addr 0x%x write 0x%x failed" % (io_addr, value))
            return True, ("io_addr 0x%x write 0x%x success" % (io_addr, value))
        if way == 'i2cword':
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            mask_tuple = (0xff, 0)
            if mask not in mask_tuple:
                ret, read_value = wbi2cgetWord(bus, addr, offset)
                if ret is True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("i2c read word failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
            ret, log = wbi2csetWord(bus, addr, offset, value)
            if ret is not True:
                return False, ("i2cword write bus:%d, addr:0x%x, offset:0x%x, value:0x%x failed" %
                               (bus, addr, offset, value))
            return True, ("i2cword write bus:%d, addr:0x%x, offset:0x%x, value:0x%x success" %
                          (bus, addr, offset, value))
        if way == 'i2cbyte':
            bus = config.get("bus")
            addr = config.get("loc")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            mask_tuple = (0xff, 0)
            if mask not in mask_tuple:
                ret, read_value = wbi2cgetByte(bus, addr)
                if ret is True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("i2c read byte failed. bus:%d, addr:0x%x" % (bus, addr))
            ret, log = wbi2csetByte(bus, addr, value)
            if ret is not True:
                return False, ("i2cbyte write bus:%d, addr:0x%x, value:0x%x failed" %
                               (bus, addr, value))
            return True, ("i2cbyte write bus:%d, addr:0x%x, value:0x%x success" %
                          (bus, addr, value))
        if way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            val = config.get("value")
            mask = config.get("mask")

            if isinstance(val, list):
                length = len(val)
                if length == 0:
                    msg = "val:%s is NONE !" % val
                    return False, msg
                write_value = val

                if mask:
                    if (not isinstance(mask, list)) or (len(mask) != length):
                        msg = "mask:%s is unexpect list!" % mask
                        return False, msg
            elif isinstance(val, int):
                length = 1
                write_value = [val]

                if mask:
                    if not isinstance(mask, int):
                        msg = "mask:%s is unexpect int!" % mask
                        return False, msg
                    mask = [mask]
            else:
                msg = "val:%s is not list type or not int type !" % val
                return False, msg

            if mask:
                ret, val_list = dev_file_read(path, offset, length)
                if ret is True:
                    mask_val_list = [v & ~m for v, m in zip(val_list, mask)]
                    mask_write_value = [v & m for v, m in zip(write_value, mask)]
                    write_value = [mv | v for mv, v in zip(mask_val_list, mask_write_value)]
                else:
                    return False, ("devfile read path %s failed. log:%s" % (path, val_list))

            ret, log = dev_file_write(path, offset, write_value)
            if ret is True:
                return True, ("devfile write path:%s, offset:0x%x, write_value:%s success." % (path, offset, write_value))
            return False, ("devfile write  path:%s, offset:0x%x, write_value:%s failed.log:%s" %
                           (path, offset, write_value, log))

        if way == 'gpio':
            # Use gpioset to drive one GPIO line by gpiochip name and offset.
            # Command format:
            #   gpioset --mode=<mode> [--active-low] <chip> <offset>=<value>
            # Example:
            #   gpioset --mode=exit 1e780000.gpio 64=1
            # mode is passed through to gpioset --mode. Common modes are:
            #   exit: set line value and exit immediately (default)
            #   wait: keep the line requested until gpioset exits
            #   time: keep the line requested for a timed interval
            #   signal: keep the line requested until a signal is received
            # Actual supported modes still depend on the libgpio/gpioset version on target.
            return _set_gpio_value(config)

        if way == 'cmd':
            cmd = config.get("cmd")
            ret, log = exec_os_cmd(cmd)
            if ret:
                return False, ("cmd write exec %s failed, log: %s" % (cmd, log))
            return True, ("cmd write exec %s success" % cmd)
        if way == 'bit_wr':
            mask = config.get("mask")
            bit_val = config.get("value")
            val_config = config.get("val_config")
            ret, rd_value = get_value_once(val_config)
            if ret is False:
                return False, ("bit_wr read failed, log: %s" % rd_value)
            wr_val = (rd_value & mask) | bit_val
            val_config["value"] = wr_val
            ret, log = set_value_once(val_config)
            if ret is False:
                return False, ("bit_wr failed, log: %s" % log)
            return True, ("bit_wr success, log: %s" % log)
        if way == 'secret_key':
            secret_key_config = config.get("secret_key_config")
            if secret_key_config.get("gettype") != "devfile":
                return False, "secret_key write only support gettype=devfile"
            addr = get_secret_key_register_addr(secret_key_config)
            if addr is None:
                return False, ("get_secret_key_register_addr failed")
            decode_func = config.get("decode_func", INVALID_DECODE_FUNC)
            count = config.get("count", 1)
            ori_value = secret_key_config['value']
            tmp_secret_key_config = copy.deepcopy(secret_key_config)
            tmp_value_list = []
            for count_i in range (count):
                ret, value = secret_key_decode(decode_func, ori_value, addr + count_i, 1)
                if ret is False:
                    return False, ("secret_key_decode failed, log: %s" % value)
                tmp_value_list.append(value)
            tmp_secret_key_config['value'] = tmp_value_list
            ret, log = set_value_once(tmp_secret_key_config)
            if ret is False:
                return False, ("secret_key_wr failed, log: %s" % log)
            return True, ("secret_key_wr success, log: %s" % log)
        if way == 'creat_file':
            file_name = config.get("file")
            ret, log = exec_os_cmd("touch %s" % file_name)
            if ret:
                return False, ("creat file %s failed, log: %s" % (file_name, log))
            exec_os_cmd("sync")
            return True, ("creat file %s success" % file_name)
        if way == 'remove_file':
            file_name = config.get("file")
            ret, log = exec_os_cmd("rm -rf %s" % file_name)
            if ret:
                return False, ("remove file %s failed, log: %s" % (file_name, log))
            exec_os_cmd("sync")
            return True, ("remove file %s success" % file_name)
        if way == 'pci':
            pcibus = config.get("pcibus")
            slot = config.get("slot")
            fn = config.get("fn")
            bar = config.get("bar")
            offset = config.get("offset")
            data = config.get("data")
            return wbpciwr(pcibus, slot, fn, bar, offset, data)
        if way == 'load_process':
            script_name = config.get("script_name")
            support_multi_process = config.get("support_multi_process", 0)
            rets = getPid(script_name)
            if len(rets) == 0 or support_multi_process:
                cmd = "nohup %s%s start > /dev/null 2>&1 &" % (EXECUTABLE_FILE_PATH, script_name)
                ret, log = exec_os_cmd(cmd)
                if ret:
                    return False, ("load_process exec %s failed, log: %s" % (cmd, log))
                return True, ("load_process exec %s success" % cmd)
            return True, ("load_process script_name:%s already running" % (script_name))
        if way == 'unload_process':
            script_name = config.get("script_name")
            rets = getPid(script_name)
            for ret in rets:
                cmd = "kill " + ret
                exec_os_cmd(cmd)
            return True, ("unload_process kill %s success" % script_name)
        if way == 'supervisor_load_process':
            script_name = config.get("script_name")
            ret, runnig = check_supervisor_process_run_by_name(script_name)
            if ret is False:
                return ret, runnig
            if runnig is False:
                cmd = "supervisorctl start %s" % script_name
                ret, log = exec_os_cmd(cmd)
                if ret:
                    return False, ("supervisor_load_process exec %s failed, log: %s" % (cmd, log))
                return True, ("supervisor_load_process exec %s success" % cmd)
            return True, ("supervisor_load_process script_name:%s already running" % (script_name))
        if way == 'supervisor_unload_process':
            script_name = config.get("script_name")
            ret, runnig = check_supervisor_process_run_by_name(script_name)
            if ret is False:
                return ret, runnig
            if runnig is True:
                cmd = "supervisorctl stop %s" % script_name
                ret, log = exec_os_cmd(cmd)
                if ret:
                    return False, ("supervisor_unload_process exec %s failed, log: %s" % (cmd, log))
                return True, ("supervisor_unload_process exec %s success" % cmd)
            else:
                return True, ("supervisor_unload_process script_name:%s already stop" % (script_name))
        if way == "log_to_file":
            log_file_path = config.get("log_file_path")
            log_config = config.get("log_config")
            file_max_size = config.get("file_max_size", 5*1024*1024)
            if log_file_path is None or log_config is None:
                return False, ("log_to_file config err, please check, config: %s", config)
            ret, log_value = get_value_once(log_config)
            if ret is False:
                return False, ("log_to_file read failed, log: %s" % log_value)

            return log_to_file(log_value, log_file_path, file_max_size)
        return False, ("%s not support write type" % way)
    except Exception as e:
        return False, ("set_value_once exception:%s happen" % str(e))


def get_value(config):
    retrytime = PLATFORM_I2C_RETRY_TIME
    for i in range(retrytime):
        ret, val = get_value_once(config)
        if ret is True:
            return True, val
        time.sleep(0.1)
    return False, val


def set_value(config):
    if config.get("pre_check") is not None:
        ret, rd_value = get_value(config["pre_check"])
        if ret is False:
            log = "do pre check get_value failed, msg: %s" % rd_value
            return False, log
        mask = config["pre_check"].get("mask")
        if mask is not None:
            value = rd_value & mask
        else:
            value = rd_value
        okval = config["pre_check"].get("okval")
        if value != okval:
            log = ("pre_check not ok, rd_value: %s, mask: %s, okval: %s, don't need to set_value" %
                (rd_value, mask, okval))
            return True, log

    retrytime = PLATFORM_I2C_RETRY_TIME
    ignore_result_flag = config.get("ignore_result", 0)
    for i in range(retrytime):
        ret, log = set_value_once(config)
        if ret is True:
            return True, log
        if ignore_result_flag == 1:
            return True, log
        time.sleep(0.1)
    return False, log


class CompressedRotatingFileHandler(logging.handlers.RotatingFileHandler):
    def doRollover(self):
        """
        Do a rollover, as described in __init__().
        """
        if self.stream:
            self.stream.close()
            self.stream = None
        if self.backupCount > 0:
            for i in range(self.backupCount - 1, 0, -1):
                sfn = "%s.%d.gz" % (self.baseFilename, i)
                dfn = "%s.%d.gz" % (self.baseFilename, i + 1)
                if os.path.exists(sfn):
                    if os.path.exists(dfn):
                        os.remove(dfn)
                    os.rename(sfn, dfn)
            dfn = self.baseFilename + ".1.gz"
            if os.path.exists(dfn):
                os.remove(dfn)
            # These two lines below are the only new lines. I commented out the os.rename(self.baseFilename, dfn) and
            #  replaced it with these two lines.
            with open(self.baseFilename, 'rb') as f_in, gzip.open(dfn, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
        self.mode = 'w'
        self.stream = self._open()


def getSdkReg(reg):
    try:
        cmd = "bcmcmd -t 1 'getr %s ' < /dev/null" % reg
        ret, result = exec_os_cmd(cmd)
        result_t = result.strip().replace("\r", "").replace("\n", "")
        if ret != 0 or "Error:" in result_t:
            return False, result
        patt = r"%s.(.*):(.*)>drivshell" % reg
        rt = re.findall(patt, result_t, re.S)
        test = re.findall("=(.*)", rt[0][0])[0]
    except Exception:
        return False, 'getsdk register error'
    return True, test


def getMacTemp():
    result = {}
    exec_os_cmd("bcmcmd -t 1 \"show temp\" < /dev/null")
    ret, log = exec_os_cmd("bcmcmd -t 1 \"show temp\" < /dev/null")
    if ret:
        return False, result
    logs = log.splitlines()
    for line in logs:
        if "average" in line:
            b = re.findall(r'\d+.\d+', line)
            result["average"] = b[0]
        elif "maximum" in line:
            b = re.findall(r'\d+.\d+', line)
            result["maximum"] = b[0]
    return True, result


def getMacTemp_sysfs(mactempconf):
    temp = -1000000
    try:
        temp_list = []
        mac_temp_loc = mactempconf.get("loc", [])
        mac_temp_flag = mactempconf.get("flag", None)
        if mac_temp_flag is not None:
            gettype = mac_temp_flag.get('gettype')
            okbit = mac_temp_flag.get('okbit')
            okval = mac_temp_flag.get('okval')
            if gettype == "io":
                io_addr = mac_temp_flag.get('io_addr')
                val = io_rd(io_addr)
                if val is None:
                    raise Exception("get mac_flag by io failed.")
            else:
                bus = mac_temp_flag.get('bus')
                loc = mac_temp_flag.get('loc')
                offset = mac_temp_flag.get('offset')
                ind, val = wbi2cget(bus, loc, offset)
                if ind is not True:
                    raise Exception("get mac_flag by i2c failed.")
            val_t = (int(val, 16) & (1 << okbit)) >> okbit
            if val_t != okval:
                raise Exception("mac_flag invalid, val_t:%d." % val_t)
        for loc in mac_temp_loc:
            temp_s = get_sysfs_value(loc)
            if isinstance(temp_s, str) and temp_s.startswith("ERR"):
                raise Exception("get mac temp error. loc:%s" % loc)
            temp_t = int(temp_s)
            if temp_t == -1000000:
                raise Exception("mac temp invalid.loc:%s" % loc)
            temp_list.append(temp_t)
        temp_list.sort(reverse=True)
        temp = temp_list[0]
    except Exception:
        return False, temp
    return True, temp

def get_format_value(format_str):
    ast_obj = ast.parse(format_str, mode='eval')
    visitor = CodeVisitor()
    visitor.visit(ast_obj)
    ret = visitor.get_value()
    return ret

def check_value(config):
    # check value
    retrytime = config.get("retry", 1)
    for i in range(retrytime):
        ret, rd_value = get_value(config)
        if ret is False:
            log = "get_value failed, msg: %s" % rd_value
            return False, log

        mask = config.get("mask")
        if mask is not None:
            value = rd_value & mask
        else:
            value = rd_value
        okval = config.get("okval")
        if value == okval:
            log = ("check ok, rd_value: %s, mask: %s, okval: %s, retry: %s" %
                (rd_value, mask, okval, i))
            return True, log
        # check failed, sleep to retry.
        sleep_time = config.get("sleep_time")
        if sleep_time is not None:
            time.sleep(sleep_time)
    log = ("check failed, rd_value: %s, mask: %s, okval: %s, retry: %s" %
            (rd_value, mask, okval, retrytime))
    return False, log

def check_value_and_get_value(config):
    okval = config.get("okval", None)
    mask = config.get("mask", 0xff)
    retrytime = config.get("retry", 1)
    if okval is None:
        log = 'Failed: okval is None. config: %s' % config
        return False, log, log
    for i in range(retrytime):
        ret, val = get_value(config)
        if ret is True:
            val &= mask
            # Regardless of whether the register check is okval, return the original value of the register val
            if isinstance(okval, list):
                if val in okval:
                    return True, CHECK_VALUE_OK, val
                else:
                    return True, CHECK_VALUE_NOT_OK, val
            else:
                if okval == val:
                    return True, CHECK_VALUE_OK, val
                return True, CHECK_VALUE_NOT_OK, val
        time.sleep(0.1)
    return False, val, val

def generate_mgmt_version_file():
    cmd = "nohup platform_manufacturer.py -u > /dev/null 2>&1 &"
    rets = getPid("platform_manufacturer.py")
    if len(rets) == 0:
        exec_os_cmd(cmd)

def update_mgmt_version(generate_mgmt_version = 0):
    if generate_mgmt_version == 1:
        for i in range(10):
            generate_mgmt_version_file()
            if os.path.exists(MGMT_VERSION_PATH):
                return True, "generate mgmt_version success"
            time.sleep(1)
        return False, "generate mgmt_version,failed, %s not exits" % MGMT_VERSION_PATH
    return True, "skip generate mgmt_version"

def get_mgmt_version():
    ret, val = update_mgmt_version(1)
    if ret is False:
        return ret, val
    return read_sysfs(MGMT_VERSION_PATH)

def waitForSdk_byfile(sdk_fpath, timeout):
    time_cnt = 0
    while True:
        try:
            if os.path.exists(sdk_fpath):
                break
            else:
                sys.stdout.write(".")
                sys.stdout.flush()
                time_cnt = time_cnt + 1
                if time_cnt > timeout:
                    raise Exception("waitForSdk timeout")
                time.sleep(1)
        except Exception as e:
            return False
    return True

def waitForSdk_byredis(redis_cmd, timeout):
    time_cnt = 0
    while True:
        try:
            ret, val = get_value(redis_cmd)
            if ret is True and "READY" in val:
                break
            else:
                sys.stdout.write(".")
                sys.stdout.flush()
                time_cnt = time_cnt + 1
                if time_cnt > timeout:
                    raise Exception("waitForSdk timeout")
                time.sleep(1)
        except Exception as e:
            return False
    return True

def waitForDocker(sdkcheck_params, timeout = 180):
    if sdkcheck_params is None:
        return True
    if sdkcheck_params.get("checktype") == "file":  # Judge by file
        sdk_fpath = sdkcheck_params.get("sdk_fpath")
        return waitForSdk_byfile(sdk_fpath, timeout)
    elif sdkcheck_params.get("checktype") == "redis":  # Judge by redis
        redis_cmd = sdkcheck_params.get("redis_cmd")
        return waitForSdk_byredis(redis_cmd, timeout)
    else:
        # unsupport
        return False

def calculate_file_crc32(file_path):
    file_max_read_len = 4096
    try:
        with open(file_path, 'rb') as f:
            crc = 0
            while True:
                data = f.read(file_max_read_len)
                if not data:
                    break
                crc = zlib.crc32(data, crc)
        return True, crc & 0xffffffff
    except Exception as e:
        msg = "Calculate file CRC32 fail, file: %s, reason: %s"  % (file_path, str(e))
        return False, msg

def read_s3ip_sysfs(sysfs_path):
    ret, val = read_sysfs(sysfs_path)
    if ret is True:
        if val in S3IP_FAIL_RET_LIST:
            return False, "read s3ip sysfs %s fail, ret: %s" % (sysfs_path, val)
        else:
            return True, val
    else:
        return False, val
def calculate_data_crc32(data):
    # data is bytearray
    return (binascii.crc32(data) & 0xFFFFFFFF)

def parse_file_head(file, head_info_config):
    try:
        with open(file, 'r', errors='ignore') as fd:
            rdbuf = fd.read(MAX_HEADER_SIZE)
        file_head_start = rdbuf.index('FILEHEADER(\n')  # ponit to F
        file_head_start += rdbuf[file_head_start:].index('\n')  # ponit to \n
        file_head_end = rdbuf.index(')\n')
        header_buf = rdbuf[file_head_start + 1: file_head_end - 1]
        for line in header_buf.split('\n'):
            head_list = line.split('=', 1)
            head_key = head_list[0]
            head_val = head_list[1]
            head_info_config[head_key] = head_val
        msg = "file: %s head_info_config: %s" % (file, head_info_config)
        return True, msg
    except Exception as e:
        msg = "parse %s head failed, msg: %s" % (file, str(e))
        return False, msg

def do_fw_upg_raw_file_generate(head_file, head_info_config, raw_file = None):
    try:
        if raw_file is None:
            dir_name = os.path.dirname(head_file)
            file_name = os.path.basename(head_file)
            raw_file = os.path.join(dir_name, f"raw_{file_name}")

        with open(head_file, 'rb') as fd:
            rdbuf = fd.read()

        # Locate header position (same logic as original code; note newline in binary data is b'\n')
        # Note: If "FILEHEADER(\n" in original file is a text string, need to convert to byte string for matching
        file_head_end = rdbuf.index(b')\n') + 2

        # Strip header and retain remaining content (binary data)
        remaining_content = rdbuf[file_head_end:].lstrip()

        with open(raw_file, 'wb') as fd:
            fd.write(remaining_content)

        ret, val = calculate_file_crc32(raw_file)
        if ret is False:
            return ret, val

        ret, msg = parse_file_head(head_file, head_info_config)
        if ret is False:
            return False, msg

        head_crc = head_info_config.get('CRC', None)
        if head_crc is None:
            msg = "head crc not found in %s" % (head_file)
            return False, msg

        if val != int(head_crc, 16):
            msg = "raw_file crc check fail, crc 0x%x, head_crc: %s" % (val, head_crc)
            return False, msg

        return True, raw_file

    except Exception as e:
        msg = f"Failed to backup and strip head: {str(e)}"
        return False, msg

def get_vr_chip_id(bus, addr):
    path = VR_CHIP_ID_PATH % (bus, addr)
    get_val_conf = {
        "gettype": "sysfs",
        "loc": path,
    }
    ret, ic_device_id = get_value(get_val_conf)
    if ret is False:
        msg = "get ic_device_id failed, msg: %s" % ic_device_id
        return False, msg
    return True, ic_device_id

def atomic_write(target_file, content):
    temp_file = target_file + "_tmp"
    try:
        # Write to the temporary file
        with open(temp_file, "w", encoding="utf-8") as f:
            f.write(content)
            f.flush()
            os.fsync(f.fileno())
        # Atomically rename temp file to target file
        os.rename(temp_file, target_file)
        return True, ""
    except Exception as e:
        # Clean up temp file if present
        try:
            if os.path.exists(temp_file):
                os.remove(temp_file)
        except Exception:
            pass
        return False, f"Failed to write: {e}"

def check_cold_reboot(self):
    if os.path.exists(REBOOT_TYPE_PATH):
        try:
            with open(REBOOT_TYPE_PATH, "r") as f:
                reboot_type = f.read().strip()
                # Check if the file is empty or contains invalid data
                if not reboot_type:
                    self.log_error("Reboot type file is empty or invalid.")
                    msg = "Reboot type file is empty or invalid."
                    return False, msg
                # Validate reboot_type against expected values
                if reboot_type not in [str(val) for val in REBOOT_CAUSE_STR2INT.values()]:
                    self.log_error(f"Invalid reboot type: {reboot_type}")
                    msg = "Invalid reboot type: %s" % reboot_type
                    return False, msg
                if reboot_type == str(REBOOT_CAUSE_STR2INT[REBOOT_CAUSE_POWER_LOSS]):
                    self.log_debug("Check reboot type success, cold reboot")
                    return True, True
                else:
                    self.log_debug("Check reboot type success, not cold reboot")
                    return True, False
        except Exception as e:
            self.log_error("Check cold reboot type failed: %s" % str(e))
            msg = "Check cold reboot type failed: %s" % str(e)
            return False, msg
    else:
        self.log_debug("Reboot type file does not exist.")
        msg = "Reboot type file does not exist."
        return False, msg

def check_bmc_updating_func():
    # Check if BMC updating check is enabled via config

    if not BMC_CHECK_UPDATING_ENABLE:
        return False, "BMC updating check is disabled"

    try:
        try:
            with open(SONIC_DB_DATABASE_CONFIG_PATH, "r") as fd:
                db_cfg = json.load(fd)
        except Exception as e:
            return False, "Read db config file failed: %s" % str(e)

        state_db_cfg = db_cfg.get("DATABASES", {}).get(SONIC_STATE_DB_NAME, {})
        state_db_id = str(state_db_cfg.get("id", "6"))
        instance_name = state_db_cfg.get("instance", "redis")
        sock_path = db_cfg.get("INSTANCES", {}).get(instance_name, {}).get("unix_socket_path", "")
        if not sock_path:
            return False, "STATE_DB unix socket path not found"
        if not state_db_id.isdigit():
            return False, "Invalid STATE_DB id: %s" % state_db_id

        countdown_cmd = [
            "docker", "exec", "database", "redis-cli",
            "-s", sock_path,
            "--raw",
            "-n", state_db_id,
            "HGET", BMC_PATCH_KEY, BMC_PATCH_COUNTDOWN_FIELD
        ]
        proc = subprocess.Popen(
            countdown_cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        count_down_str, _ = proc.communicate()
        ret = proc.returncode
        if ret != 0:
            return False, "Failed to check BMC updating: %s" % count_down_str
        count_down_text = count_down_str.strip().strip('"')
        if not count_down_text or count_down_text == "(nil)":
            return False, "BMC is not updating"
        if not count_down_text.isdigit():
            return False, "Invalid BMC countdown value: '%s'" % count_down_text
        count_down = int(count_down_text)
        if count_down > 0:
            return True, "BMC is updating, countdown: %d seconds" % count_down
        return False, "BMC updating time out"
    except Exception as e:
        return False, "Failed to check BMC updating status: %s" % str(e)
