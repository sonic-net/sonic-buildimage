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


CONFIG_DB_PATH = "/etc/sonic/config_db.json"
MAILBOX_DIR = "/sys/bus/i2c/devices/"


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
    "wb_os_system",
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
    "get_format_value"
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
        calc_tuple = ("float", "int", "str")

        if node.func.id not in calc_tuple:
            raise NotImplementedError("Unsupport function call type: %s" % node.func.id)

        args_val_list = []
        for item in node.args:
            ret = self.get_op_value(item)
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
    if os.path.isfile('/usr/share/sonic/hwsku/machine.conf'):
        return getonieplatform('/usr/share/sonic/hwsku/machine.conf')
    return getplatform_config_db()


def wbi2cget(bus, devno, address, word=None):
    if word is None:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x " % (bus, devno, address)
    else:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x %s" % (bus, devno, address, word)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
        if ret == 0:
            return True, ret_t
        time.sleep(0.1)
    return False, ret_t


def wbi2cset(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
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
    retrytime = 3
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def wbi2csetWord(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%x w" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def wbi2cset_pec(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x bp" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def wbi2cset_wordpec(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x wp" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def wbsysset(location, value):
    command_line = "echo 0x%02x > %s" % (value, location)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = wb_os_system(command_line)
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

    if not isinstance(buf_list, list) or len(buf_list) == 0:
        msg = "buf:%s is not list type or is NONE !" % buf_list
        return False, msg

    if not os.path.exists(path):
        msg = path + " not found !"
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


def wb_os_system(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    return status, output


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


def exec_os_cmd(cmd):
    cmds = cmd.split('|')
    procs = []
    for i, c in enumerate(cmds):
        stdin = None if i == 0 else procs[i-1].stdout
        p = subprocess.Popen(shlex.split(c), stdin=stdin, stdout=subprocess.PIPE, shell=False, stderr=subprocess.STDOUT)
        procs.append(p)
    for proc in procs:
        proc.wait()
    return procs[-1].returncode, typeTostr(procs[-1].communicate()[0])


def exec_os_cmd_log(cmd):
    proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE, shell=False, stderr=sys.stderr, close_fds=True,
                            stdout=sys.stdout, universal_newlines=True, bufsize=1)
    proc.wait()
    stdout = proc.communicate()[0]
    stdout = typeTostr(stdout)
    return proc.returncode, stdout


def write_sysfs(location, value):
    try:
        if not os.path.isfile(location):
            return False, ("location[%s] not found !" % location)
        with open(location, 'w') as fd1:
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


def get_value_once(config):
    try:
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
        if way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            read_len = config.get("read_len")
            ret, val_list = dev_file_read(path, offset, read_len)
            if ret is True:
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
        return False, "not support read type"
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
        if way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            buf_list = config.get("value")
            ret, log = dev_file_write(path, offset, buf_list)
            if ret is True:
                return True, ("devfile write path:%s, offset:0x%x, buf_list:%s success." % (path, offset, buf_list))
            return False, ("devfile read  path:%s, offset:0x%x, buf_list:%s failed.log:%s" %
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
        return False, "not support write type"
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
        ret, result = wb_os_system(cmd)
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
    wb_os_system("bcmcmd -t 1 \"show temp\" < /dev/null")
    ret, log = wb_os_system("bcmcmd -t 1 \"show temp\" < /dev/null")
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

