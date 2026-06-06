#!/usr/bin/python3

import sys
import os
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
import syslog
from typing import Tuple
from public.platform_common_config import MGMT_VERSION_PATH, SYSLOG_PREFIX

G_RESTFUL_CLASS = None
try:
    from restful_util.restful_interface import *
    G_RESTFUL_CLASS = RestfulApi()
except Exception as e:
    pass

INVALID_DECODE_FUNC = -1
FIX_DECODE_FUNC = 1
REVERSE_DECODE_FUNC = 2

CHECK_VALUE_NOT_OK = 0
CHECK_VALUE_OK = 1

GET_BY_COMMON   = "common"
GET_BY_RESTFUL = "restful"
GET_BY_S3IP     = "s3ip"

CONFIG_DB_PATH = "/etc/sonic/config_db.json"
MAILBOX_DIR = "/sys/bus/i2c/devices/"

DEV_PRESENT     = 1
DEV_ABSENT      = 0

# bsp common log dir
BSP_COMMON_LOG_DIR = "/var/log/bsp_tech/"

PLATFORM_REBOOT_REASON_FILE = "/etc/.platform_reboot_reason"

__all__ = [
    "strtoint",
    "byteTostr",
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
    "get_sysfs_value",
    "write_sysfs_value",
    "get_value",
    "set_value",
    "getSdkReg",
    "getMacTemp",
    "getMacTemp_sysfs",
    "get_format_value",
    "log_to_file",
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
    "DEV_TYPE_VOL",
    "DEV_TYPE_CURR",
    "DEV_TYPE_TEMP",
    "DEV_TYPE_SLOT",
    "DEV_TYPE_POWER",
    "waitForDocker",
    "common_syslog_emerg",
    "common_syslog_alert",
    "common_syslog_crit",
    "common_syslog_error",
    "common_syslog_warn",
    "common_syslog_notice",
    "common_syslog_info",
    "common_syslog_debug",
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



def common_syslog_emerg(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-EMERG-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_EMERG, info)

def common_syslog_alert(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-ALERT-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ALERT, info)

def common_syslog_crit(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-CRIT-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, info)

def common_syslog_error(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-ERR-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, info)

def common_syslog_warn(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-WARNING-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, info)

def common_syslog_notice(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-NOTICE-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_NOTICE, info)

def common_syslog_info(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-INFO-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, info)

def common_syslog_debug(title, info):
    syslog_prefix = SYSLOG_PREFIX
    syslog.openlog(f"{syslog_prefix}-DEBUG-{title}", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_DEBUG, info)

def platform_reboot(reason, info):
    try:
        with open(PLATFORM_REBOOT_REASON_FILE, 'w') as file:
            file.write(reason)

        common_syslog_warn("REBOOT", ("The system will reboot due to %s." % info))
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
    val = os.popen("sonic-cfggen -j %s -v DEVICE_METADATA.localhost.platform" % CONFIG_DB_PATH).read().strip()
    if len(val) <= 0:
        return ""
    return val

def getplatform_name():
    if os.path.isfile('/host/machine.conf'):
        return getonieplatform('/host/machine.conf')
    if os.path.isfile('/etc/sonic/machine.conf'):
        return getonieplatform('/etc/sonic/machine.conf')
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

def setup_logger(log_file, max_bytes=1024*1024*5, backup_count=3):
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
    if not os.path.isfile('/host/machine.conf'):
        return "/host/machine.conf not exist"
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
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
                value = str(eval(formula % (float(value))))
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
DEV_TYPE_CPLD = "cpld"
DEV_TYPE_FPGA = "fpga"
DEV_TYPE_VOL = "vol"
DEV_TYPE_CURR = "curr"
DEV_TYPE_TEMP = "temp"
DEV_TYPE_SLOT = "slot"
DEV_TYPE_POWER = "power"

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
        return G_RESTFUL_CLASS.get_fan_field_value(fan_index, field_name)
    elif info_type == DEV_TYPE_FAN_MOTOR:
        field_name = config.get("field_name")
        fan_index = config.get("index")
        motor_index = config.get("motor_index")
        return G_RESTFUL_CLASS.get_fan_motor_field_value(fan_index, motor_index, field_name)
    elif info_type == DEV_TYPE_PSU:
        field_name = config.get("field_name")
        psu_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_psu_num()
        return G_RESTFUL_CLASS.get_psu_field_value(psu_index, field_name)
    elif info_type == DEV_TYPE_CPLD:
        field_name = config.get("field_name")
        cpld_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_cpld_num()
        return G_RESTFUL_CLASS.get_cpld_field_value(cpld_index, field_name)
    elif info_type == DEV_TYPE_FPGA:
        field_name = config.get("field_name")
        fpga_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_fpga_num()
        return G_RESTFUL_CLASS.get_fpga_field_value(fpga_index, field_name)
    elif info_type == DEV_TYPE_VOL:
        field_name = config.get("field_name")
        vol_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_vol_num()
        return G_RESTFUL_CLASS.get_vol_field_value(vol_index, field_name)
    elif info_type == DEV_TYPE_CURR:
        field_name = config.get("field_name")
        curr_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_curr_num()
        return G_RESTFUL_CLASS.get_curr_field_value(curr_index, field_name)
    elif info_type == DEV_TYPE_TEMP:
        field_name = config.get("field_name")
        temp_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_temp_num()
        return G_RESTFUL_CLASS.get_temp_field_value(temp_index, field_name)
    elif info_type == DEV_TYPE_SLOT:
        field_name = config.get("field_name")
        slot_index = config.get("index")
        if field_name == "num":
            return G_RESTFUL_CLASS.get_slot_num()
        return G_RESTFUL_CLASS.get_slot_field_value(slot_index, field_name)
    else:
        return False, "unsupport restful_info_type %s" % info_type

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
    prefix = "/sys/s3ip/%s/%s%s/" % (S3IP_PREFIX_DIR_NAME.get(dev_type, dev_type), dev_type, index)
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
    cmd = "supervisorctl update"
    os.system(cmd)
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
        if way == 'secret_key_rd':
            secret_key_config = config.get("secret_key_config")
            if secret_key_config.get("gettype") != "devfile":
                return False, "secret_key_rd only support gettype=devfile"
            ret, ori_value = get_value_once(secret_key_config)
            if ret is False:
                return False, ("secret_key_rd read failed, log: %s" % ori_value)
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
        return False, ("%s is not support read type" % way)
    except Exception as e:
        return False, ("get_value_once exception:%s happen" % str(e))


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
            buf_list = config.get("value")
            ret, log = dev_file_write(path, offset, buf_list)
            if ret is True:
                return True, ("devfile write path:%s, offset:0x%x, buf_list:%s success." % (path, offset, buf_list))
            return False, ("devfile write  path:%s, offset:0x%x, buf_list:%s failed.log:%s" %
                           (path, offset, buf_list, log))
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
        if way == 'secret_key_wr':
            secret_key_config = config.get("secret_key_config")
            if secret_key_config.get("gettype") != "devfile":
                return False, "secret_key_wr only support gettype=devfile"
            addr = get_secret_key_register_addr(secret_key_config)
            if addr is None:
                return False, ("get_secret_key_register_addr failed")
            decode_func = config.get("decode_func", INVALID_DECODE_FUNC)
            ori_value = secret_key_config['value']
            ret, value = secret_key_decode(decode_func, ori_value, addr, 1)
            if ret is False:
                return False, ("secret_key_decode failed, log: %s" % value)
            secret_key_config['value'] = value
            ret, log = set_value_once(secret_key_config)
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
                cmd = "nohup %s start > /dev/null 2>&1 &" % script_name
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
    retrytime = 6
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

    retrytime = 6
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
    retrytime = config.get("retry", 6)
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

def waitForSdk(sdk_fpath, timeout):
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

def waitForDocker(sdkcheck_params, timeout = 180):
    if sdkcheck_params is None:
        return True
    if sdkcheck_params.get("checktype") == "file":  # Judge by file
        sdk_fpath = sdkcheck_params.get("sdk_fpath")
        return waitForSdk(sdk_fpath, timeout)
    else:
        # unsupport
        return False