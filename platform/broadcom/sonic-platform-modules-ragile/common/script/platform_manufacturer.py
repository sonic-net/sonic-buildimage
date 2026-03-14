#!/usr/bin/env python3

import re
import mmap
import fcntl
import subprocess
import shlex
import signal
import os
import time
import sys
from platform_config import MANUINFO_CONF
from monitor import status


INDENT = 4


def printerr(vchar):
    sys.stderr.write(vchar + '\n')


g_extra_cache = {}
g_meminfo_cache = {}
g_exphy_cache = {}


def exec_os_cmd(cmd, timeout = None):
    status, output = subprocess.getstatusoutput(cmd)
    return status, output


def exphyfwsplit():
    # improve performance
    global g_exphy_cache
    if g_exphy_cache:
        return
    cmd = "bcmcmd -t 1 \"phy control xe,ce fw_get\" |grep fw_version"
    ret, output = exec_os_cmd(cmd)
    if ret or len(output) == 0:
        raise Exception("run cmd: {} error, status: {}, msg: {}".format(cmd, ret, output))
    exphyfwstr = output.strip()
    portlist = exphyfwstr.split("\n")
    for port in portlist:
        phy_addr_str = get_regular_val(port, r"phy_addr\s*=\s*\w+", 0)
        if phy_addr_str.startswith("ERR"):
            continue
        phy_addr_key = phy_addr_str.replace(" ", "")
        if phy_addr_key in g_exphy_cache:
            continue

        g_exphy_cache[phy_addr_key] = {}

        fw_version_str = get_regular_val(port, r"fw_version\s*=\s*\w+", 0)
        if fw_version_str.startswith("ERR"):
            del g_exphy_cache[phy_addr_key]
            continue

        fw_version = fw_version_str.split("=")[1].strip()
        g_exphy_cache[phy_addr_key]["fw_version"] = fw_version

        if "success" in port:
            ret = "OK"
        else:
            ret = "Unexpected"
        g_exphy_cache[phy_addr_key]["status"] = ret
    return


def lshwmemorysplit():
    # improve performance
    global g_meminfo_cache
    if g_meminfo_cache:
        return
    cmd = "lshw -c memory"
    ret, output = exec_os_cmd(cmd)
    if ret or len(output) == 0:
        raise Exception("run cmd: {} error, status: {}, msg: {}".format(cmd, ret, output))
    memstr = output.strip()
    memlist = memstr.split("*-")
    for item in memlist:
        if item.strip().startswith("memory") and "System Memory" not in item:
            continue
        line_index = 0
        for line in item.splitlines():
            line_index += 1
            if line_index == 1:
                memdict_key = line
                g_meminfo_cache[memdict_key] = {}
            else:
                if ":" not in line:
                    continue
                key = line.split(":", 1)[0].strip()
                value = line.split(":", 1)[1].strip()
                g_meminfo_cache[memdict_key][key] = value
            if "empty" in item:
                break
    return


def run_extra_func(funcname):
    # improve performance
    if funcname in g_extra_cache:
        return g_extra_cache.get(funcname)
    func = getattr(status, funcname)
    ret = []
    func(ret)
    if ret:
        g_extra_cache[funcname] = ret
    return ret


def get_extra_value(funcname, itemid, key):
    for item in run_extra_func(funcname):
        if item.get("id") == itemid:
            return item.get(key, "NA")
    return "NA"


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


def checksignaldriver(name):
    modisexistcmd = "lsmod | grep -w %s | wc -l" % name
    ret, output = exec_os_cmd(modisexistcmd)
    if ret:
        return False
    if output.isdigit() and int(output) > 0:
        return True
    return False


def adddriver(name):
    cmd = "modprobe %s" % name
    if checksignaldriver(name) is not True:
        ret, log = exec_os_cmd(cmd)
        if ret != 0 or len(log) > 0:
            return False
        return True
    return True


def removedriver(name):
    cmd = "rmmod %s" % name
    if checksignaldriver(name):
        exec_os_cmd(cmd)


def add_5387_driver():
    errmsg = ""
    spi_gpio = "wb_spi_gpio"
    ret = adddriver(spi_gpio)
    if ret is False:
        errmsg = "modprobe wb_spi_gpio driver failed."
        return False, errmsg
    spi_5387_device = "wb_spi_93xx46 spi_bus_num=0"
    ret = adddriver(spi_5387_device)
    if ret is False:
        errmsg = "modprobe wb_spi_93xx46 driver failed."
        return ret, errmsg
    return True, ""


def remove_5387_driver():
    spi_5387_device = "wb_spi_93xx46"
    removedriver(spi_5387_device)
    spi_gpio = "wb_spi_gpio"
    removedriver(spi_gpio)


def deal_itmes(item_list):
    for item in item_list:
        dealtype = item.get("dealtype")
        if dealtype == "shell":
            cmd = item.get("cmd")
            timeout = item.get("timeout", 10)
            exec_os_cmd(cmd, timeout)
        elif dealtype == "io_wr":
            io_addr = item.get("io_addr")
            wr_value = item.get("value")
            io_wr(io_addr, wr_value)


def get_func_value(funcname, params):
    func = getattr(ExtraFunc, funcname)
    ret = func(params)
    return ret


def read_pci_reg(pcibus, slot, fn, resource, offset):
    '''read pci register'''
    if offset % 4 != 0:
        return "ERR offset: %d not 4 bytes align"
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(resource))
    size = os.path.getsize(filename)
    with open(filename, "r+") as file:
        data = mmap.mmap(file.fileno(), size)
        result = data[offset: offset + 4]
        s = result[::-1]
        val = 0
        for value in s:
            val = val << 8 | value
        data.close()
    return "%08x" % val


def devfileread(path, offset, length, bit_width):
    ret = ""
    val_str = ''
    val_list = []
    fd = -1
    if not os.path.exists(path):
        return "%s not found !" % path
    if length % bit_width != 0:
        return "only support read by bit_width"
    if length < bit_width:
        return "len needs to greater than or equal to bit_width"

    try:
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.read(fd, length)
        for item in ret:
            val_list.append(item)

        for i in range(0, length, bit_width):
            val_str += " 0x"
            for j in range(0, bit_width):
                val_str += "%02x" % val_list[i + bit_width - j - 1]
    except Exception as e:
        return str(e)
    finally:
        if fd > 0:
            os.close(fd)
    return val_str


def read_reg(loc, offset, size):
    with open(loc, 'rb') as file:
        file.seek(offset)
        return ' '.join(["%02x" % item for item in file.read(size)])


def std_match(stdout, pattern):
    if pattern is None:
        return stdout.strip()
    for line in stdout.splitlines():
        if re.match(pattern, line):
            return line.strip()
    raise EOFError("pattern: {} does not match anything in stdout {}".format(
        pattern, stdout))


def i2c_rd(bus, loc, offset):
    '''
    read i2c with i2cget command
    '''
    cmd = "i2cget -f -y {} {} {}".format(bus, loc, offset)
    retrytime = 6
    for i in range(retrytime):
        ret, stdout = subprocess.getstatusoutput(cmd)
        if ret == 0:
            return stdout
        time.sleep(0.1)
    raise RuntimeError("run cmd: {} error, status {}".format(cmd, ret))


def i2c_rd_bytes(bus, loc, offset, size):
    blist = []
    for i in range(size):
        ret = i2c_rd(bus, loc, offset + i)
        blist.append(ret)

    return blist


def get_pair_val(source, separator):
    try:
        value = source.split(separator, 1)[1]
    except (ValueError, IndexError):
        return "ERR separator: {} does not match in source: {}".format(separator, source)
    return value.strip()


def get_regular_val(source, pattern, group):
    try:
        value = re.findall(pattern, source)[group]
    except Exception:
        return "ERR pattern: {} does not match  in source: {} with group: {}".format(pattern, source, group)
    return value.strip()


def find_match(file2read, pattern):
    with open(file2read, 'r') as file:
        for line in file:
            if not re.match(pattern, line):
                continue
            return line.strip()
    return "ERR pattern %s not match in %s" % (pattern, file2read)


def readaline(file2read):
    with open(file2read, 'r') as file:
        return file.readline()


def sort_key(e):
    return e.arrt_index


class ExtraFunc(object):
    @staticmethod
    def get_bcm5387_version(params):
        version = ""
        try:
            ret, msg = add_5387_driver()
            if ret is False:
                raise Exception(msg)

            before_deal_list = params.get("before", [])
            deal_itmes(before_deal_list)

            ret, version = exec_os_cmd(params["get_version"])
            if ret != 0:
                version = "ERR " + version

            after_deal_list = params.get("after", [])
            deal_itmes(after_deal_list)

        except Exception as e:
            version = "ERR %s" % (str(e))
        finally:
            finally_deal_list = params.get("finally", [])
            deal_itmes(finally_deal_list)
            remove_5387_driver()
        return version

    @staticmethod
    def get_memory_value(params):
        root_key = params.get("root_key")
        sub_key = params.get("sub_key")
        lshwmemorysplit()
        return g_meminfo_cache.get(root_key, {}).get(sub_key, "NA")

    @staticmethod
    def get_memory_bank_value(params):
        lshwmemorysplit()
        bank = params.get("bankid")
        if g_meminfo_cache.get(bank, {}):
            return True
        return False

    @staticmethod
    def get_exphy_fw(phyid):
        exphyfwsplit()
        if phyid not in g_exphy_cache:
            return "ERR %s not found." % phyid
        fw_version = g_exphy_cache.get(phyid).get("fw_version")
        ret = g_exphy_cache.get(phyid).get("status")
        msg = "%s    %s" % (fw_version, ret)
        return msg

class CallbackSet:
    def cpld_format(self, blist):
        if isinstance(blist, str):
            blist = blist.split()
        elif not isinstance(blist, list) or len(blist) != 4:
            raise ValueError("cpld format: wrong parameter: {}".format(blist))

        return "{}{}{}{}".format(*blist).replace("0x", "")


class VersionHunter:
    call = CallbackSet()

    def __init__(self, entires):
        self.head = None
        self.next = None
        self.key = None
        self.cmd = None
        self.file = None
        self.reg = None
        self.i2c = None
        self.extra = None
        self.pattern = None
        self.separator = None
        self.parent = None
        self.ignore = False
        self.children = []
        self.level = 0
        self.callback = None
        self.delspace = None
        self.arrt_index = None
        self.config = None
        self.precheck = None
        self.func = None
        self.regular = None
        self.group = 0
        self.pci = None
        self.devfile = None
        self.decode = None
        self.timeout = 10
        self.__dict__.update(entires)

    def check_para(self):
        if self.pattern is None:
            return False
        if self.cmd is None or self.file is None:
            return False
        return True

    def get_version(self):
        ret = "NA"
        try:
            if self.cmd is not None:
                ret, output = exec_os_cmd(self.cmd, self.timeout)
                if ret or len(output) == 0:
                    raise RuntimeError("run cmd: {} error, status: {}, msg: {}".format(self.cmd, ret, output))
                ret = std_match(output, self.pattern)
            elif self.file is not None:
                ret = self.read_file()
            elif self.reg is not None:
                ret = read_reg(self.reg.get("loc"), self.reg.get("offset"),
                               self.reg.get("size"))
            elif self.extra:
                ret = get_extra_value(self.extra.get("funcname"),
                                      self.extra.get("id"),
                                      self.extra.get("key"))
            elif self.i2c:
                ret = i2c_rd_bytes(self.i2c.get("bus"), self.i2c.get("loc"),
                                   self.i2c.get("offset"),
                                   self.i2c.get("size"))
            elif self.config:
                ret = self.config
            elif self.func:
                ret = get_func_value(self.func.get("funcname"),
                                     self.func.get("params"))
            elif self.pci:
                ret = read_pci_reg(self.pci.get("bus"), self.pci.get("slot"),
                                   self.pci.get("fn"), self.pci.get("bar"), self.pci.get("offset"))
            elif self.devfile:
                ret = devfileread(self.devfile.get("loc"), self.devfile.get("offset"),
                                  self.devfile.get("len"), self.devfile.get("bit_width"))

        except Exception as e:
            # printerr(e.message)
            return "ERR %s" % str(e)
        return self.exe_callback(ret)

    def exe_callback(self, data):
        try:
            if self.callback:
                method = getattr(self.call, self.callback)
                return method(data)
        except Exception:
            return "ERR run callback method: {} error, data: {}".format(self.callback, data)
        return data

    def read_file(self):
        if self.pattern is not None:
            return find_match(self.file, self.pattern)
        return readaline(self.file)

    def hunt(self):
        if self.ignore:
            return
        indent = self.level * INDENT * " "

        if self.precheck:
            try:
                ret = get_func_value(self.precheck.get("funcname"), self.precheck.get("params"))
                if ret is not True:
                    return
            except Exception as e:
                err_msg = "ERR %s" % str(e)
                format_str = "{}{:<{}}{}".format(indent, self.key + ':',
                                                 (30 - len(indent)), err_msg)
                print(format_str)
                return
        # has children
        if self.children:
            self.children.sort(key=sort_key)
            format_str = "{}{}:".format(indent, self.key)
            print(format_str)
            for child in self.children:
                if not isinstance(child, VersionHunter):
                    continue
                child.level = self.level + 1
                child.hunt()
        else:
            version = self.get_version() or ""
            if not version.startswith("ERR"):
                version = version.replace("\x00", "").strip()
                if self.separator is not None:
                    version = get_pair_val(version, self.separator)
                if self.delspace is not None:
                    version = version.replace(" ", "")
                if self.regular is not None:
                    version = get_regular_val(version, self.regular, self.group)
                if self.decode is not None:
                    tmp_version = self.decode.get(version)
                    if tmp_version is None:
                        version = "ERR decode %s failed" % version
                    else:
                        version = tmp_version
            format_str = "{}{:<{}}{}".format(indent, self.key + ':',
                                             (30 - len(indent)), version)
            print(format_str)

        if self.next:
            print("")
            self.next.hunt()


pidfile = 0


def ApplicationInstance():
    global pidfile
    pidfile = open(os.path.realpath(__file__), "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return True
    except Exception:
        return False


def run():
    if os.geteuid() != 0:
        print("Root privileges are required for this operation")
        sys.exit(1)

    start_time = time.time()
    while True:
        ret = ApplicationInstance()
        if ret is True:
            break
        if time.time() - start_time > 10:
            printerr("manufacturer is running.")
            sys.exit(1)
        time.sleep(0.5)

    objmap = {}

    try:
        target = {}
        target.update(MANUINFO_CONF)
        for objname, value in target.items():
            objmap[objname] = VersionHunter(value)
    except Exception as e:
        printerr(str(e))
        sys.exit(1)

    head = None
    for objname, obj in objmap.items():
        if head is None and obj.head:
            head = obj
        if obj.parent:
            objmap.get(obj.parent).children.append(obj)
        if obj.next:
            obj.next = objmap.get(obj.next)

    head.hunt()


if __name__ == "__main__":
    run()
