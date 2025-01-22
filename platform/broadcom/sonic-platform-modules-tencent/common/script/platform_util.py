#!/usr/bin/python3
# -------------------------------------------------------------------------
#
# Author:      sonic_rd
#
# Created:     02/07/2018
# Copyright:   (c) sonic_rd 2018
# -------------------------------------------------------------------------
import sys
import os
import subprocess
import time
import mmap
import glob


CONFIG_DB_PATH = "/etc/sonic/config_db.json"


__all__ = [
    "byteTostr",
    "getplatform_name",
    "rji2cget",
    "rji2cset",
    "rjpcird",
    "rjpciwr",
    "rji2cgetWord",
    "rji2csetWord",
    "dev_file_read",
    "dev_file_write",
    "rj_os_system",
    "io_rd",
    "io_wr",
    "exec_os_cmd",
    "exec_os_cmd_log",
    "write_sysfs",
    "read_sysfs",
    "get_value",
    "set_value",
]

def inttostr(vl, len):
    if not isinstance(vl, int):
        raise Exception(" type error")
    index = 0
    ret_t = ""
    while index < len:
        ret = 0xff & (vl >> index * 8)
        ret_t += chr(ret)
        index += 1
    return ret_t


def byteTostr(val):
    strtmp = ''
    for i in range(len(val)):
        strtmp += chr(val[i])
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
    else:
        return val


def getplatform_name():
    if os.path.isfile('/host/machine.conf'):
        return getonieplatform('/host/machine.conf')
    elif os.path.isfile('/usr/share/sonic/hwsku/machine.conf'):
        return getonieplatform('/usr/share/sonic/hwsku/machine.conf')
    else:
        return getplatform_config_db()


def rji2cget(bus, devno, address, word=None):
    if word is None:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x " % (bus, devno, address)
    else:
        command_line = "i2cget -f -y %d 0x%02x 0x%02x %s" % (bus, devno, address, word)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = rj_os_system(command_line)
        if ret == 0:
            return True, ret_t
        time.sleep(0.1)
    return False, ret_t


def rji2cset(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%02x" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = rj_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def rjpcird(pcibus, slot, fn, bar, offset):
    '''read pci register'''
    if offset % 4 != 0:
        return "ERR offset: %d not 4 bytes align"
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
    with open(filename, "r+") as file:
        size = os.path.getsize(filename)
        data = mmap.mmap(file.fileno(), size)
        result = data[offset: offset + 4]
        s = result[::-1]
        val = 0
        for i in range(0, len(s)):
            val = val << 8 | s[i]
        data.close()
    return "0x%08x" % val


def rjpciwr(pcibus, slot, fn, bar, offset, data):
    '''write pci register'''
    ret = inttostr(data, 4)
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
    with open(filename, "r+") as file:
        size = os.path.getsize(filename)
        data = mmap.mmap(file.fileno(), size)
        data[offset: offset + 4] = ret
        result = data[offset: offset + 4]
        s = result[::-1]
        val = 0
        for i in range(0, len(s)):
            val = val << 8 | ord(s[i])
        data.close()


def rji2cgetWord(bus, devno, address):
    command_line = "i2cget -f -y %d 0x%02x 0x%02x w" % (bus, devno, address)
    retrytime = 3
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = rj_os_system(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def rji2csetWord(bus, devno, address, byte):
    command_line = "i2cset -f -y %d 0x%02x 0x%02x 0x%x w" % (
        bus, devno, address, byte)
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = rj_os_system(command_line)
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


def rj_os_system(cmd):
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
        str = os.read(fd, read_len)
        return "".join(["%02x" % item for item in str])
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
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True, stderr=subprocess.STDOUT)
    stdout = proc.communicate()[0]
    proc.wait()
    stdout = typeTostr(stdout)
    return proc.returncode, stdout


def exec_os_cmd_log(cmd):
    proc = subprocess.Popen((cmd), stdin=subprocess.PIPE, shell=True, stderr=sys.stderr, close_fds=True,
                            stdout=sys.stdout, universal_newlines=True, bufsize=1)

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


def get_value_once(config):
    try:
        way = config.get("gettype")
        int_decode = config.get("int_decode", 16)
        if way == 'sysfs':
            loc = config.get("loc")
            ret, val = read_sysfs(loc)
            if ret == True:
                return True, int(val, int_decode)
            else:
                return False, ("sysfs read %s failed. log:%s" % (loc, val))
        elif way == "i2c":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset", 0)
            ret, val = rji2cget(bus, addr, offset)
            if ret == True:
                return True, int(val, int_decode)
            else:
                return False, ("i2c read failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
        elif way == "io":
            io_addr = config.get('io_addr')
            val = io_rd(io_addr)
            if len(val) != 0:
                return True, int(val, int_decode)
            else:
                return False, ("io_addr read 0x%x failed" % io_addr)
        elif way == "i2cword":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            ret, val = rji2cgetWord(bus, addr, offset)
            if ret == True:
                return True, int(val, int_decode)
            else:
                return False, ("i2cword read failed. bus:%d, addr:0x%x, offset:0x%x" % (bus, addr, offset))
        elif way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            read_len = config.get("read_len")
            ret, val_list = dev_file_read(path, offset, read_len)
            if ret == True:
                return True, val_list
            else:
                return False, ("devfile read failed. path:%s, offset:0x%x, read_len:%d" % (path, offset, read_len))
        elif way == 'cmd':
            cmd = config.get("cmd")
            ret, val = exec_os_cmd(cmd)
            if ret:
                return False, ("cmd read exec %s failed, log: %s" % (cmd, val))
            else:
                return True, int(val, int_decode)
        elif way == 'file_exist':
            judge_file = config.get('judge_file', None)
            if os.path.exists(judge_file):
                return True, True
            return True, False
        else:
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
            if mask != 0xff and mask != 0:
                ret, read_value = read_sysfs(loc)
                if ret == True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("sysfs read %s failed. log:%s" % (loc, read_value))
            ret, log = write_sysfs(loc, "0x%02x" % value)
            if ret != True:
                return False, ("sysfs %s write 0x%x failed" % (loc, value))
            else:
                return True, ("sysfs write 0x%x success" % value)
        elif way == "i2c":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            if mask != 0xff and mask != 0:
                ret, read_value = rji2cget(bus, addr, offset)
                if ret == True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("i2c read failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
            ret, log = rji2cset(bus, addr, offset, value)
            if ret != True:
                return False, ("i2c write bus:%d, addr:0x%x, offset:0x%x, value:0x%x failed" %
                               (bus, addr, offset, value))
            else:
                return True, ("i2c write bus:%d, addr:0x%x, offset:0x%x, value:0x%x success" %
                              (bus, addr, offset, value))
        elif way == "io":
            io_addr = config.get('io_addr')
            value = config.get('value')
            mask = config.get("mask", 0xff)
            if mask != 0xff and mask != 0:
                read_value = io_rd(io_addr)
                if read_value is None:
                    return False, ("io_addr 0x%x read failed" % (io_addr))
                read_value = int(read_value, base=16)
                value = (read_value & mask) | value
            ret = io_wr(io_addr, value)
            if ret != True:
                return False, ("io_addr 0x%x write 0x%x failed" % (io_addr, value))
            else:
                return True, ("io_addr 0x%x write 0x%x success" % (io_addr, value))
        elif way == 'i2cword':
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get("value")
            mask = config.get("mask", 0xff)
            if mask != 0xff and mask != 0:
                ret, read_value = rji2cgetWord(bus, addr, offset)
                if ret == True:
                    read_value = int(read_value, base=16)
                    value = (read_value & mask) | value
                else:
                    return False, ("i2c read word failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
            ret, log = rji2csetWord(bus, addr, offset, value)
            if ret != True:
                return False, ("i2cword write bus:%d, addr:0x%x, offset:0x%x, value:0x%x failed" %
                               (bus, addr, offset, value))
            else:
                return True, ("i2cword write bus:%d, addr:0x%x, offset:0x%x, value:0x%x success" %
                              (bus, addr, offset, value))
        elif way == "devfile":
            path = config.get("path")
            offset = config.get("offset")
            buf_list = config.get("value")
            ret, log = dev_file_write(path, offset, buf_list)
            if ret == True:
                return True, ("devfile write path:%s, offset:0x%x, buf_list:%s success." % (path, offset, buf_list))
            else:
                return False, ("devfile read  path:%s, offset:0x%x, buf_list:%s failed.log:%s" %
                               (path, offset, buf_list, log))
        elif way == 'cmd':
            cmd = config.get("cmd")
            ret, log = exec_os_cmd(cmd)
            if ret:
                return False, ("cmd write exec %s failed, log: %s" % (cmd, log))
            else:
                return True, ("cmd write exec %s success" % cmd)
        elif way == 'bit_wr':
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
        elif way == 'creat_file':
            file_name = config.get("file")
            ret, log = exec_os_cmd("touch %s" % file_name)
            if ret:
                return False, ("creat file %s failed, log: %s" % (file_name, log))
            exec_os_cmd("sync")
            return True, ("creat file %s success" % file_name)
        elif way == 'remove_file':
            file_name = config.get("file")
            ret, log = exec_os_cmd("rm -rf %s" % file_name)
            if ret:
                return False, ("remove file %s failed, log: %s" % (file_name, log))
            exec_os_cmd("sync")
            return True, ("remove file %s success" % file_name)
        else:
            return False, "not support write type"
    except Exception as e:
        return False, ("set_value_once exception:%s happen" % str(e))


def get_value(config):
    retrytime = 6
    for i in range(retrytime):
        ret, val = get_value_once(config)
        if ret == True:
            return True, val
        time.sleep(0.1)
    return False, val


def set_value(config):
    retrytime = 6
    ignore_result_flag = config.get("ignore_result", 0)
    for i in range(retrytime):
        ret, log = set_value_once(config)
        if ret == True:
            return True, log
        if ignore_result_flag == 1:
            return True, log
        time.sleep(0.1)
    return False, log
