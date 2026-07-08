# -*- coding: UTF-8 -*-
#-------------------------------------------------------------------------
# Name:        ruijieutil
# Purpose:     common configuration and api
#
# Author:      rd
#
# Created:     02/07/2018
# Copyright:   (c) rd 2018
#-------------------------------------------------------------------------
import sys
import os
import re
import syslog
import time
import binascii
import tty
import termios
import threading
import click
import mmap
from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe
from ruijieconfig import rg_eeprom, FRULISTS, MAC_DEFAULT_PARAM, MAC_AVS_PARAM, FANS_DEF, \
        FAN_PROTECT, E2_LOC, E2_PROTECT, RUIJIE_SERVICE_TAG, RUIJIE_DIAG_VERSION, \
        STARTMODULE, RUIJIE_CARDID, RUIJIE_PRODUCTNAME, RUIJIE_PART_NUMBER, \
        RUIJIE_LABEL_REVISION, RUIJIE_MAC_SIZE, RUIJIE_MANUF_NAME, RUIJIE_MANUF_COUNTRY, \
        RUIJIE_VENDOR_NAME, MAILBOX_DIR

try:
    from eepromutil.fru import ipmifru

except Exception or SystemExit:
    pass

import logging.handlers
import shutil
import gzip
import glob

__all__ = ["MENUID", "MENUPARENT", "MENUVALUE", "CHILDID", "MENUITEMNAME", "MENUITEMDEAL",
           "GOBACK", "GOQUIT", "file_name", "CRITICAL", "FATAL", "ERROR", "WARNING", "WARN",
           "INFO", "DEBUG", "NOTSET", "levelNames", "TLV_INFO_ID_STRING", "TLV_INFO_VERSION",
           "TLV_INFO_LENGTH", "TLV_INFO_LENGTH_VALUE", "TLV_CODE_PRODUCT_NAME",
           "TLV_CODE_PART_NUMBER", "TLV_CODE_SERIAL_NUMBER", "TLV_CODE_MAC_BASE",
           "TLV_CODE_MANUF_DATE", "TLV_CODE_DEVICE_VERSION", "TLV_CODE_LABEL_REVISION", 
           "TLV_CODE_PLATFORM_NAME", "TLV_CODE_ONIE_VERSION", "TLV_CODE_MAC_SIZE",
           "TLV_CODE_MANUF_NAME", "TLV_CODE_MANUF_COUNTRY", "TLV_CODE_VENDOR_NAME",
           "TLV_CODE_DIAG_VERSION", "TLV_CODE_SERVICE_TAG", "TLV_CODE_VENDOR_EXT",
           "TLV_CODE_CRC_32", "_TLV_DISPLAY_VENDOR_EXT", "TLV_CODE_RJ_CARID",
           "_TLV_INFO_HDR_LEN", "SYSLOG_IDENTIFIER", "log_info", "log_debug", "log_warning",
           "log_error", "CompressedRotatingFileHandler", "SETMACException", "checkinput",
           "checkinputproduct", "getInputSetmac", "fan_tlv", "AVSUTIL", "I2CUTIL", "BMC",
           "getSdkReg", "getfilevalue", "get_sysfs_value", "write_sysfs_value",
           "RJPRINTERR", "strtoint", "inttostr", "str_to_hex", "hex_to_str", "str_to_bin",
           "bin_to_str", "getMacTemp", "getMacTemp_sysfs", "restartDockerService",
           "waitForDhcp", "waitForSdk", "waitForDocker", "getTLV_BODY", "_crc32",
           "printvalue", "generate_value", "getsyseeprombyId", "fac_init_cardidcheck",
           "isValidMac", "util_setmac", "getInputCheck", "getrawch", "upper_input",
           "changeTypeValue", "astrcmp", "generate_ext", "rji2cget", "rji2cset", "rjpcird",
           "rjpciwr", "rjsysset", "rji2cgetWord", "rji2csetWord", "fan_setmac", "checkfansninput",
           "checkfanhwinput", "util_show_fanse2", "get_fane2_sysfs", "util_show_fane2", "getPid",
           "fac_fans_setmac_tlv", "fac_fan_setmac_fru", "fac_fans_setmac", "fac_fan_setmac",
           "writeToEEprom", "get_local_eth0_mac", "getonieversion", "createbmcMac",
           "fac_board_setmac", "ipmi_set_mac", "getInputValue", "bmc_setmac", "closeProtocol",
           "checkSdkMem", "getch", "get_raw_input", "getsysvalue", "get_pmc_register", "decoder",
           "decode_eeprom", "get_sys_eeprom", "getCardId", "getsysmeminfo", "getsysmeminfo_detail",
           "getDmiSysByType", "gethwsys", "getsysbios", "searchDirByName", "getUsbLocation",
           "getusbinfo", "get_cpu_info", "get_version_config_info", "io_rd", "io_wr"]

MENUID = "menuid"
MENUPARENT = "parentid"
MENUVALUE = "value"
CHILDID = "childid"
MENUITEMNAME = "name"
MENUITEMDEAL = "deal"
GOBACK = "goBack"
GOQUIT = "quit"

file_name = "/etc/init.d/opennsl-modules-3.16.0-5-amd64"
##########################################################################
# ERROR LOG LEVEL
##########################################################################
CRITICAL = 50
FATAL = CRITICAL
ERROR = 40
WARNING = 30
WARN = WARNING
INFO = 20
DEBUG = 10
NOTSET = 0

levelNames = {
    CRITICAL: 'CRITICAL',
    ERROR: 'ERROR',
    WARNING: 'WARNING',
    INFO: 'INFO',
    DEBUG: 'DEBUG',
    NOTSET: 'NOTSET',
    'CRITICAL': CRITICAL,
    'ERROR': ERROR,
    'WARN': WARNING,
    'WARNING': WARNING,
    'INFO': INFO,
    'DEBUG': DEBUG,
    'NOTSET': NOTSET,
}

TLV_INFO_ID_STRING = "TlvInfo\x00"
TLV_INFO_VERSION = 0x01
TLV_INFO_LENGTH = 0x00
TLV_INFO_LENGTH_VALUE = 0xba

##########################################################################
# eeprom info
##########################################################################
TLV_CODE_PRODUCT_NAME = 0x21
TLV_CODE_PART_NUMBER = 0x22
TLV_CODE_SERIAL_NUMBER = 0x23
TLV_CODE_MAC_BASE = 0x24
TLV_CODE_MANUF_DATE = 0x25
TLV_CODE_DEVICE_VERSION = 0x26
TLV_CODE_LABEL_REVISION = 0x27
TLV_CODE_PLATFORM_NAME = 0x28
TLV_CODE_ONIE_VERSION = 0x29
TLV_CODE_MAC_SIZE = 0x2A
TLV_CODE_MANUF_NAME = 0x2B
TLV_CODE_MANUF_COUNTRY = 0x2C
TLV_CODE_VENDOR_NAME = 0x2D
TLV_CODE_DIAG_VERSION = 0x2E
TLV_CODE_SERVICE_TAG = 0x2F
TLV_CODE_VENDOR_EXT = 0xFD
TLV_CODE_CRC_32 = 0xFE
_TLV_DISPLAY_VENDOR_EXT = 1
TLV_CODE_RJ_CARID = 0x01
_TLV_INFO_HDR_LEN = 11


SYSLOG_IDENTIFIER = "UTILTOOL"

# ========================== Syslog wrappers ==========================

def log_info(msg, also_print_to_console=False):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)


def log_debug(msg, also_print_to_console=False):
    try:
        syslog.openlog(SYSLOG_IDENTIFIER)
        syslog.syslog(syslog.LOG_DEBUG, msg)
        syslog.closelog()

        if also_print_to_console:
            click.echo(msg)
    except Exception as e:
        pass


def log_warning(msg, also_print_to_console=False):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_WARNING, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)


def log_error(msg, also_print_to_console=False):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)

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


class SETMACException(Exception):
    def __init__(self, param='ERROR', errno="-1"):
        err = "Setmac fail[%s]: %s" % (errno, param)
        Exception.__init__(self, err)
        self.param = param
        self.errno = errno

def checkinput(b):
    if b.isdigit() == False:
        raise Exception("Ivalid Number")
    if int(b) > 0xff or int(b) < 0:
        raise Exception("Out of area")

def checkinputproduct(b):
    if b.isalnum() ==False:
        raise Exception("Invalid string")

def getInputSetmac(val):
    bia = val.boardInfoArea
    pia = val.productInfoArea
    if bia != None:
        a = raw_input("[Board Card]Product Serial Number:")
        if len(a) != 13:
            raise Exception("Invalid Serial Number length")
        checkinputproduct(a)
        bia.boardSerialNumber = a
        b = raw_input("[Board Card]Product Version:(from 1-255)")
        checkinput(b)
        b = "%0x" % int(b)
        bia.boardextra1 = b.upper()
    if pia != None:
        a = raw_input("[Product Area]Product Serial Number:")
        if len(a) != 13:
            raise Exception("Invalid Serial Number")
        checkinputproduct(a)
        pia.productSerialNumber = a
        b = raw_input("[Product Area]Product Version:(from 1-255)")
        checkinput(b)
        b = "%0x" % int(b)
        pia.productVersion = b.upper()
    return val

class fan_tlv(object):
    VERSION = 0x01                    # E2PROM Version，start from 0x01
    FLAG = 0x7E                       # New E2PROM version flag is 0x7E
    HW_VER = 0X01                     # compose by master version and fixed version
    TYPE = 0xf1                       # hw type defination 
    TLV_LEN = 00                      # data length (16bit)
    _FAN_TLV_HDR_LEN = 6
    _FAN_TLV_CRC_LEN = 2

    _FAN_TLV_TYPE_NAME = 0x02
    _FAN_TLV_TYPE_SN = 0x03
    _FAN_TLV_TYPE_HW_INFO = 0x05
    _FAN_TLV_TYPE_DEV_TYPE = 0x06

    _fandecodetime = 0

    @property
    def dstatus(self):
        return self._dstatus

    @property
    def typename(self):
        return self._typename

    @property
    def typesn(self):
        return self._typesn

    @property
    def typehwinfo(self):
        return self._typehwinfo

    @property
    def typedevtype(self):
        return self._typedevtype

    @property
    def fanbus(self):
        return self._fanbus

    @property
    def fanloc(self):
        return self._fanloc

    @property
    def fandecodetime(self):
        return self._fandecodetime

    def __init__(self):
        self._typename = ""
        self._typesn = ""
        self._typehwinfo = ""
        self._typedevtype = ""
        self._dstatus = 0

    def strtoarr(self, str):
        s = []
        if str is not None:
            for index in range(len(str)):
                s.append(str[index])
        return s

    def generate_fan_value(self):
        bin_buffer = [chr(0xff)] * 256
        bin_buffer[0] = chr(self.VERSION)
        bin_buffer[1] = chr(self.FLAG)
        bin_buffer[2] = chr(self.HW_VER)
        bin_buffer[3] = chr(self.TYPE)

        temp_t = "%08x" % self.typedevtype  # handle devtype first
        typedevtype_t = hex_to_str(temp_t)
        total_len = len(self.typename) + len(self.typesn) + \
            len(self.typehwinfo) + len(typedevtype_t) + 8

        bin_buffer[4] = chr(total_len >> 8)
        bin_buffer[5] = chr(total_len & 0x00FF)

        index_start = 6
        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_NAME)
        bin_buffer[index_start + 1] = chr(len(self.typename))
        bin_buffer[index_start + 2: index_start + 2 +
                   len(self.typename)] = self.strtoarr(self.typename)
        index_start = index_start + 2 + len(self.typename)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_SN)
        bin_buffer[index_start + 1] = chr(len(self.typesn))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(self.typesn)] = self.strtoarr(self.typesn)
        index_start = index_start + 2 + len(self.typesn)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_HW_INFO)
        bin_buffer[index_start + 1] = chr(len(self.typehwinfo))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(self.typehwinfo)] = self.strtoarr(self.typehwinfo)
        index_start = index_start + 2 + len(self.typehwinfo)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_DEV_TYPE)
        bin_buffer[index_start + 1] = chr(len(typedevtype_t))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(typedevtype_t)] = self.strtoarr(typedevtype_t)
        index_start = index_start + 2 + len(typedevtype_t)

        crcs = fan_tlv.fancrc(''.join(bin_buffer[0:index_start]))  # check 2bytes
        bin_buffer[index_start] = chr(crcs >> 8)
        bin_buffer[index_start + 1] = chr(crcs & 0x00ff)
        return bin_buffer

    def decode(self, e2):
        ret = []
        self.VERSION = ord(e2[0])
        self.FLAG = ord(e2[1])
        self.HW_VER = ord(e2[2])
        self.TYPE = ord(e2[3])
        self.TLV_LEN = (ord(e2[4]) << 8) | ord(e2[5])

        tlv_index = self._FAN_TLV_HDR_LEN
        tlv_end = self._FAN_TLV_HDR_LEN + self.TLV_LEN

        # check checksum
        if len(e2) < self._FAN_TLV_HDR_LEN + self.TLV_LEN + 2:
            self._dstatus = -2
            return ret
        sumcrc = fan_tlv.fancrc(e2[0:self._FAN_TLV_HDR_LEN + self.TLV_LEN])
        readcrc = ord(e2[self._FAN_TLV_HDR_LEN + self.TLV_LEN]
                      ) << 8 | ord(e2[self._FAN_TLV_HDR_LEN + self.TLV_LEN + 1])
        if sumcrc != readcrc:
            self._dstatus = -1
            return ret
        else:
            self._dstatus = 0
        while (tlv_index + 2) < len(e2) and tlv_index < tlv_end:
            s = self.decoder(
                e2[tlv_index:tlv_index + 2 + ord(e2[tlv_index + 1])])
            tlv_index += ord(e2[tlv_index + 1]) + 2
            ret.append(s)

        return ret

    @staticmethod
    def fancrc(t):
        sum = 0
        for index in range(len(t)):
            sum += ord(t[index])
        return sum

    def decoder(self, t):
        try:
            name = ""
            value = ""
            if ord(t[0]) == self._FAN_TLV_TYPE_NAME:
                name = "Product Name"
                value = str(t[2:2 + ord(t[1])])
                self._typename = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_SN:
                name = "serial Number"
                value = str(t[2:2 + ord(t[1])])
                self._typesn = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_HW_INFO:
                name = "hardware info"
                value = str(t[2:2 + ord(t[1])])
                self._typehwinfo = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_DEV_TYPE:
                name = "dev type"
                value = str(t[2:2 + ord(t[1])])
                value = str_to_hex(value)
                self._typedevtype = value
                value = "0x08%x" % value
        except Exception as e:
            print(e)
        return {"name": name, "code": ord(t[0]), "value": value}

    def __str__(self):
        formatstr = "VERSION     : 0x%02x  \n" \
                    "   FLAG     : 0x%02x  \n" \
                    " HW_VER     : 0x%02x  \n" \
                    "   TYPE     : 0x%02x  \n" \
                    "typename    : %s      \n" \
                    "typesn      : %s      \n" \
                    "typehwinfo  : %s      \n"
        return formatstr % (self.VERSION, self.FLAG, self.HW_VER, self.TYPE, self.typename, self.typesn, self.typehwinfo)


class AVSUTIL():
    @staticmethod
    def mac_avs_chip(bus, devno, loc, open, close, loop, protectaddr, level, loopaddr):
        # disable protection
        rji2cset(bus, devno, protectaddr, open)
        rji2cset(bus, devno, loopaddr, loop)
        rji2csetWord(bus, devno, loc, level)
        ret, value = rji2cgetWord(bus, devno, loc)
        if strtoint(value) == level:
            ret = 0
        # enable protection
        rji2cset(bus, devno, protectaddr, close)
        if ret == 0:
            return True
        return False

    @staticmethod
    def macPressure_adj(macavs, avs_param, mac_def_param):
        # check whether it within range
        max_adj = max(avs_param.keys())
        min_adj = min(avs_param.keys())
        type = mac_def_param["type"]
        level = 0
        if type == 0:
            if macavs not in range(min_adj, max_adj + 1):
                return False
            else:
                level = macavs
        else:
            if macavs not in range(min_adj, max_adj + 1):
                level = mac_def_param["default"]
            else:
                level = macavs
        ret = AVSUTIL.mac_avs_chip(mac_def_param["bus"], mac_def_param["devno"], mac_def_param["addr"], mac_def_param["open"],
                                   mac_def_param["close"], mac_def_param["loop"], mac_def_param["protectaddr"], avs_param[level], mac_def_param["loopaddr"])
        return ret

    @staticmethod
    def mac_adj():
        macavs = 0
        name = MAC_DEFAULT_PARAM["sdkreg"]
        ret, status = getSdkReg(name)
        if ret == False:
            return False
        status = strtoint(status)
        # shift operation
        if MAC_DEFAULT_PARAM["sdktype"] != 0:
            status = (
                status >> MAC_DEFAULT_PARAM["macregloc"]) & MAC_DEFAULT_PARAM["mask"]
        macavs = status
        ret = AVSUTIL.macPressure_adj(macavs, MAC_AVS_PARAM, MAC_DEFAULT_PARAM)
        return ret


class I2CUTIL():
    @staticmethod
    def getvaluefromdevice(name):
        ret = []
        for item in DEVICE:
            if item["name"] == name:
                ret.append(item)
        return ret

    @staticmethod
    def openFanE2Protect():
        rji2cset(FAN_PROTECT["bus"], FAN_PROTECT["devno"],
                 FAN_PROTECT["addr"], FAN_PROTECT["open"])

    @staticmethod
    def closeFanE2Protect():
        rji2cset(FAN_PROTECT["bus"], FAN_PROTECT["devno"],
                 FAN_PROTECT["addr"], FAN_PROTECT["close"])

    @staticmethod
    def writeToFanE2(bus, loc, rst_arr):
        index = 0
        for item in rst_arr:
            rji2cset(bus, loc, index, ord(item))
            index += 1
            
    @staticmethod
    def writeToE2(bus, loc, rst_arr):
        index = 0
        for item in rst_arr:
            rji2cset(bus, loc, index, ord(item))
            index += 1
    
    @staticmethod
    def getE2File(bus, loc):
        return "/sys/bus/i2c/devices/%d-00%02x/eeprom" % (bus, loc)


class BMC():
    _instance_lock = threading.Lock()

    def __init__(self):
        pass

    def __new__(cls, *args, **kwargs):
        if not hasattr(Singleton, "_instance"):
            with Singleton._instance_lock:
                if not hasattr(Singleton, "_instance"):
                    Singleton._instance = object.__new__(cls)
        return Singleton._instance

#  Internal interface 


def getSdkReg(reg):
    try:
        cmd = ["bcmcmd", "-t", "1", "getr"+str(reg)]
        ret, result = getstatusoutput_noshell(cmd)
        result_t = result.strip().replace("\r", "").replace("\n", "")
        if ret != 0 or "Error:" in result_t:
            return False, result
        patt = r"%s.(.*):(.*)>drivshell" % reg
        rt = re.findall(patt, result_t, re.S)
        test = re.findall("=(.*)", rt[0][0])[0]
    except Exception as e:
        return False, 'getsdk register error'
    return True, test


def getfilevalue(location):
    try:
        with open(location, 'r') as fd:
            value = fd.read()
        return True, value.strip()
    except Exception as e:
        return False, "error"


def get_sysfs_value(location):
    pos_t = str(location)
    name = get_pmc_register(pos_t)
    return name


def write_sysfs_value(reg_name, value):
    fileLoc = MAILBOX_DIR + reg_name
    try:
        if not os.path.isfile(fileLoc):
            print(fileLoc,  'not found !')
            return False
        with open(fileLoc, 'w') as fd:
            fd.write(value)
    except Exception as error:
        log_error("Unable to open " + fileLoc + "file !")
        return False
    return True


def RJPRINTERR(str):
    print("\033[0;31m%s\033[0m" % str)


def strtoint(str):  # convert Hex string to int such as "4040"/"0x4040"/"0X4040" = 16448
    value = 0
    rest_v = str.replace("0X", "").replace("0x", "")
    for index in range(len(rest_v)):
        print(rest_v[index])
        value |= int(rest_v[index], 16) << ((len(rest_v) - index - 1) * 4)
    return value

def inttostr(vl,len): # convert int to string such as 0x3030 = 00
    if type(vl) != int:
        raise Exception(" type error")
    index = 0
    ret_t = ""
    while index < len:
        ret = 0xff & (vl >> index * 8)
        ret_t += chr(ret)
        index += 1;
    return ret_t

def str_to_hex(rest_v):
    value = 0
    for index in range(len(rest_v)):
        value |= ord(rest_v[index]) << ((len(rest_v) - index - 1) * 8)
    return value


def hex_to_str(s):
    len_t = len(s)
    if len_t % 2 != 0:
        return 0
    ret = ""
    for t in range(0, int(len_t / 2)):
        ret += chr(int(s[2 * t:2 * t + 2], 16))
    return ret


def str_to_bin(s):
    return ' '.join([bin(ord(c)).replace('0b', '') for c in s])


def bin_to_str(s):
    return ''.join([chr(i) for i in [int(b, 2) for b in s.split(' ')]])


def getMacTemp():
    result = {}
    #waitForDocker()
    # exec twice, get the second result
    getstatusoutput_noshell(["bcmcmd", "-t", "1", "show temp"])
    ret, log = getstatusoutput_noshell(["bcmcmd", "-t", "1", "show temp"])
    if ret:
        return False, result
    else:
        # decode obtained info
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
    try:
        temp = -1000000
        temp_list = []
        mac_temp_loc = mactempconf.get("loc", [])
        mac_temp_flag = mactempconf.get("flag",None)
        if mac_temp_flag is not None: # check mac temperature vaild flag
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
                ind, val = rji2cget(bus, loc, offset)
                if ind is not True:
                    raise Exception("get mac_flag by i2c failed.")
            val_t = (int(val,16) & (1<< okbit)) >> okbit
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
    except Exception as e:
        return False, temp
    return True, temp

def restartDockerService(force=False):
    container_name = ["database","snmp","syncd","swss","dhcp_relay","radv","teamd","pmon"]
    ret, status = getstatusoutput_noshell(["docker", "ps"])
    if ret == 0 :
        for tmpname in container_name:
            if (tmpname not in status):
                if (force == True):
                    getstatusoutput_noshell(["docker", "restart", tmpname])
                else:
                    getstatusoutput_noshell(["systemctl", "restart", tmpname])


def waitForDhcp(timeout):
    time_cnt = 0
    while True:
        try:
            ret, status = getstatusoutput_noshell(["systemctl", "status", "dhcp_relay.service"])
            if (ret == 0 and "running" in status)  or "SUCCESS" in status:
                break
            else:
                sys.stdout.write(".")
                sys.stdout.flush()
                time_cnt = time_cnt + 1
                if time_cnt > timeout:
                    raise Exception("waitForDhcp timeout")
                time.sleep(1)
        except Exception as e:
            return False
    return True

def waitForSdk(sdk_fpath ,timeout):
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

def waitForDocker(need_restart=False,timeout=180):
    sdkcheck_params = STARTMODULE.get("sdkcheck",{})
    if sdkcheck_params.get("checktype") == "file": # pass file check 
        sdk_fpath = sdkcheck_params.get("sdk_fpath")
        return waitForSdk(sdk_fpath,timeout)
    return waitForDhcp(timeout)


def getTLV_BODY(type, productname):
    x = []
    temp_t = ""
    if type == TLV_CODE_MAC_BASE:
        arr = productname.split(':')
        for tt in arr:
            temp_t += chr(int(tt, 16))
    elif type == TLV_CODE_DEVICE_VERSION:
        temp_t = chr(productname)
    elif type == TLV_CODE_MAC_SIZE:
        temp_t = chr(productname >> 8) + chr(productname & 0x00ff)
    else:
        temp_t = productname
    x.append(chr(type))
    x.append(chr(len(temp_t)))
    for i in temp_t:
        x.append(i)
    return x


def _crc32(v):
    return '0x%08x' % (binascii.crc32(v) & 0xffffffff)  # get 8 bytes of crc32 %x return hex


def printvalue(b):
    index = 0
    for i in range(0, len(b)):
        if index % 16 == 0:
            print(" ")
        print("%02x " % ord(b[i]))
        index += 1
    print("\n")


def generate_value(_t):
    ret = []
    for i in TLV_INFO_ID_STRING:
        ret.append(i)
    ret.append(chr(TLV_INFO_VERSION))
    ret.append(chr(TLV_INFO_LENGTH))
    ret.append(chr(TLV_INFO_LENGTH_VALUE))

    total_len = 0
    for key in _t:
        x = getTLV_BODY(key, _t[key])
        ret += x
        total_len += len(x)
    ret[10] = chr(total_len + 6)

    ret.append(chr(0xFE))
    ret.append(chr(0x04))
    s = _crc32(''.join(ret))
    for t in range(0, 4):
        ret.append(chr(int(s[2 * t + 2:2 * t + 4], 16)))
    totallen = len(ret)
    if (totallen < 256):
        for left_t in range(0, 256 - totallen):
            ret.append(chr(0x00))
    return (ret, True)


def getsyseeprombyId(id):  
    ret = get_sys_eeprom()
    for item in ret:
        if item["code"] == id:
            return item
    return None


def fac_init_cardidcheck():
    rest = getsyseeprombyId(TLV_CODE_RJ_CARID)  # check cardId same or not
    if rest is None:
        print("need to program write bin file")
        return False
    else:
        rest_v = rest['value']
        value = strtoint(rest_v)
        if value == RUIJIE_CARDID:
            log_debug("check card ID pass")
        else:
            log_debug("check card ID error")
            return False
    return True


def isValidMac(mac):
    if re.match(r"^\s*([0-9a-fA-F]{2,2}:){5,5}[0-9a-fA-F]{2,2}\s*$", mac):
        return True
    return False

#  Internet cardsetmac


def util_setmac(eth, mac):
    rulefile = "/etc/udev/rules.d/70-persistent-net.rules"
    if isValidMac(mac) == False:
        return False, "MAC invaild"
    cmd1 = ["ethtool", "-e", eth] 
    cmd2 = ["grep", "0x0010"]
    cmd3 = ["awk", '{print \"0x\"$13$12$15$14}']
    ret, log = getstatusoutput_noshell_pipe(cmd1, cmd2, cmd3)
    log_debug(log)
    magic = ""
    if ret == 0 and len(log):
        magic = log
    macs = mac.upper().split(":")

    # chage ETH0 to value after setmac
    ifconfigcmd = ["ifconfig", "eth0", "hw", "ether", mac]
    log_debug(' '.join(ifconfigcmd))
    ret, status = getstatusoutput_noshell(ifconfigcmd)
    if ret:
        raise SETMACException("software set  Internet cardMAC error")
    index = 0
    for item in macs:
        cmd = ["ethtool", "-E", eht, "magic", magic, "offset", str(index), "value", "0x"+item]
        log_debug(cmd)
        index += 1
        ret, log = getstatusoutput_noshell(cmd)
        if ret != 0:
            raise SETMACException(" set hardware Internet card MAC error")
    # get value after setting 
    cmd_t = ["ethtool", "-e", "eth0", "offset", "0", "length", "6"]
    ret, log = getstatusoutput_noshell(cmd_t)
    m = re.split(':', log)[-1].strip().upper()
    mac_result = m.upper().split(" ")

    for ind, s in enumerate(macs):
        if s != mac_result[ind]:
            RJPRINTERR("MAC comparison error")
    if os.path.exists(rulefile):
        os.remove(rulefile)  
    print("MGMT MAC[%s]" % mac)
    return True


def getInputCheck(tips):
    str = raw_input(tips)
    if astrcmp(str, "y") or astrcmp(str, "ye") or astrcmp(str, "yes") or astrcmp(str, ""):
        return True
    else:
        return False

def getrawch():
  fd = sys.stdin.fileno()
  old_settings = termios.tcgetattr(fd)
  try:
    tty.setraw(sys.stdin.fileno())
    ch = sys.stdin.read(1)
  finally:
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
  return ch

def upper_input(tips):
    sys.stdout.write(tips)
    sys.stdout.flush()
    passwd = []
    while True:
        ch = getrawch().upper()
        if ch == "\r" or ch == "\n":
          return "".join(passwd)
        elif ch == '\b' or ord(ch) == 127:
            if passwd:
                del passwd[-1]
                sys.stdout.write('\b \b')
        else:
          sys.stdout.write(ch)
          passwd.append(ch)

def changeTypeValue(_value, type1, tips, example):
    if type1 == TLV_CODE_PRODUCT_NAME:
        while True:
            print("please check  (1)air from forward to backward/(2)air from backward to forward:")
            option = raw_input()
            if option == "1":
                _value[type1] = example + "-F-RJ"
                print("check Product is air from forward to backward device,Product Name:%s"%_value[type1])
                break
            elif option == "2":
                _value[type1] = example + "-R-RJ"
                print("check Product is air from backward to forward device,Product Name:%s"%_value[type1])
                break
            else:
                print("input incorrect, check please")
        return True
    print("Please input[%s]such as(%s):" % (tips, example))
    name = upper_input("")
    if type1 == TLV_CODE_MAC_BASE:
        if len(name) != 12:
            raise SETMACException("MAC address length incorrect, check please")
        release_mac = ""
        for i in range(int(len(name) / 2)):
            if i == 0:
                release_mac += name[i * 2:i * 2 + 2]
            else:
                release_mac += ":" + name[i * 2:i * 2 + 2]
        if isValidMac(release_mac) == True:
            _value[type1] = release_mac
        else:
            raise SETMACException("MAC address invaild, check please")
    elif type1 == TLV_CODE_DEVICE_VERSION:
        if name.isdigit():
            _value[type1] = int(name)
        else:
            raise SETMACException("Version is not number, check please")
    elif type1 == TLV_CODE_MAC_SIZE:
        if name.isdigit():
            _value[type1] = int(name, 16)
        else:
            raise SETMACException("Version is not number, check please")
    elif type1 == TLV_CODE_SERIAL_NUMBER:
        if name.isalnum() == False:
            raise SETMACException("Serial Number invaild string, check please")
        elif len(name) != 13:
            raise SETMACException("Serial Number length incorrect, check please")
        else:
            _value[type1] = name
    elif type1 == TLV_CODE_VENDOR_EXT:
        _value[type1] = name
    else:
        _value[type1] = name
    return True


def astrcmp(str1, str2):
    return str1.lower() == str2.lower()


def generate_ext(cardid):
    s = "%08x" % cardid
    ret = ""
    for t in range(0, 4):
        ret += chr(int(s[2 * t:2 * t + 2], 16))
    ret = chr(0x01) + chr(len(ret)) + ret
    return ret


def rji2cget(bus, devno, address):
    command_line = ["i2cget", "-f", "-y", str(bus), "0x%02x"%str(devno), "0x%02x"%str(address)]
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = getstatusoutput_noshell(command_line)
        if ret == 0:
            return True, ret_t
        time.sleep(0.1)
    return False, ret_t


def rji2cset(bus, devno, address, byte):
    command_line = ["i2cset", "-f", "-y", str(bus), "0x%02x"%str(devno), "0x%02x"%str(address), "0x%02x"%str(byte)]
    retrytime = 6
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = getstatusoutput_noshell(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t

def rjpcird(pcibus , slot , fn, bar,  offset):
    '''read pci register'''
    if offset % 4 != 0:
        return
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
    file = open(filename, "r+")
    size = os.path.getsize(filename)
    data = mmap.mmap(file.fileno(), size)
    result = data[offset: offset + 4]
    s = result[::-1]
    val = 0
    for i in range(0, len(s)):
        val = val << 8  | ord(s[i])
    return "0x%08x" % val


def rjpciwr(pcibus , slot ,fn, bar, offset, data):
    '''write pci register'''
    ret = inttostr(data, 4)
    filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
    file = open(filename, "r+")
    size = os.path.getsize(filename)
    data = mmap.mmap(file.fileno(), size)
    data[offset: offset + 4] = ret
    result = data[offset: offset + 4]
    s = result[::-1]
    val = 0
    for i in range(0, len(s)):
        val = val << 8  | ord(s[i])
    data.close()

def rjsysset(location, value):
    retrytime = 6
    for i in range(retrytime):
        try:
            with open(location, 'w') as f:
                f.write('0x%02x\n'%value)
        except (IOError, FileNotFoundError) as e:
            return False, str(e)
    return True, ''


def rji2cgetWord(bus, devno, address):
    command_line = ["i2cget", "-f", "-y", str(bus), "0x%02x"%str(devno), "0x%02x"%str(address), "w"]
    retrytime = 3
    ret_t = ""
    for i in range(retrytime):
        ret, ret_t = getstatusoutput_noshell(command_line)
        if ret == 0:
            return True, ret_t
    return False, ret_t


def rji2csetWord(bus, devno, address, byte):
    command_line = ["i2cset", "-f", "-y", str(bus), "0x%02x"%str(devno), "0x%02x"%str(address), "0x%x"%str(byte), "w"]
    getstatusoutput_noshell(command_line)


def fan_setmac():
    rji2cset(FAN_PROTECT["bus"], FAN_PROTECT["devno"],
             FAN_PROTECT["addr"], FAN_PROTECT["open"])
    rji2cset(FAN_PROTECT["bus"], FAN_PROTECT["devno"],
             FAN_PROTECT["addr"], FAN_PROTECT["close"])


def checkfansninput(fan_sn, fansntemp):
    if fan_sn in fansntemp:
        RJPRINTERR("exist same Serial Number，please input again")
        return False
    if(len(fan_sn) != 13):
        RJPRINTERR("Serial Number length incorrect，please input again")
        return False
    return True

# check 输入的hardware version


def checkfanhwinput(hw):
    if len(hw) != 4:
        RJPRINTERR("hardware version length incorrect, please input again")
        return False
    if hw.find(".") != 1:
        RJPRINTERR("hardware version incorrect, please input again")
        return False
    return True


def util_show_fanse2(fans):
    formatstr = "%-8s  %-20s  %-20s  %-20s %-20s"
    print(formatstr % ("id", "Name", "hardware version", "Serial Number", "Time"))
    print(formatstr % ("------", "---------------", "---------------", "---------------", "----"))
    for fan in fans:
        # print fan.dstatus
        if fan.dstatus < 0:
            print("%-8s" % ("FAN%d" % (fans.index(fan) + 1)))
            RJPRINTERR("  decode e2 error")
        else:
            print(formatstr % ("FAN%d" % (fans.index(fan) + 1), fan.typename.replace(chr(0x00), ""),
                               fan.typehwinfo.replace(chr(0x00), ""), fan.typesn.replace(chr(0x00), ""), fan.fandecodetime))


def get_fane2_sysfs(bus, loc):
    rg_fan_e2 = "%d-%04x/fan" % (bus, loc)
    eeprom = get_sysfs_value(rg_fan_e2)
    return eeprom


def util_show_fane2():
    ret = sorted(I2CUTIL.getvaluefromdevice("rg_fan"))    
    if len(ret) <=0:
        return None
    fans = []
    for index in range(len(ret)):
        t1 = (int(round(time.time() * 1000)))
        eeprom = get_fane2_sysfs(ret[index]["bus"], ret[index]["loc"])
        t2 = (int(round(time.time() * 1000)))
        fane2 = fan_tlv()
        fane2.fandecodetime = t2 - t1
        fane2.decode(eeprom)
        fans.append(fane2)
    util_show_fanse2(fans)


def getPid(name):
    ret = []
    for dirname in os.listdir('/proc'):
        if dirname == 'curproc':
            continue
        try:
            with open('/proc/{}/cmdline'.format(dirname), mode='rb') as fd:
                content = fd.read()
        except Exception:
            continue
        if name in content:
            ret.append(dirname)
    return ret

def fac_fans_setmac_tlv(ret):
    if len(ret) <=0:
        return None
    fans = []
    fansntemp = []
    for index in range(len(ret)):
        item = ret[index]
        log_debug(item)
        eeprom = get_fane2_sysfs(item["bus"], item["loc"])
        fane2 = fan_tlv()
        fane2.decode(eeprom)
        fane2.fanbus = item["bus"]
        fane2.fanloc = item["loc"]
        log_debug("decode eeprom success")

        print("Fan[%d]-[%s]setmac" % ((index + 1), FANS_DEF[fane2.typedevtype]))
        while True:
            print("Please input[%s]:" % "Serial Number")
            fan_sn = raw_input()
            if checkfansninput(fan_sn, fansntemp) == False:
                continue
            fansntemp.append(fan_sn)
            fan_sn = fan_sn + chr(0x00)
            fane2.typesn = fan_sn + chr(0x00)
            break
        while True:
            print("Please input[%s]:" % "hardware version")
            hwinfo = raw_input()
            if checkfanhwinput(hwinfo) == False:
                continue
            fan_hwinfo = hwinfo + chr(0x00)
            fane2.typehwinfo = fan_hwinfo + chr(0x00)
            break
        log_debug(fane2.typedevtype)
        fane2.typename = FANS_DEF[fane2.typedevtype] + chr(0x00)
        fans.append(fane2)
        print("\n")
    print("\n*******************************\n")

    util_show_fanse2(fans)
    if getInputCheck("check input correctly or not（Yes/No):") == True:
        for fan in fans:
            log_debug("ouput fan")
            fac_fan_setmac(fan)
    else:
        print("setmac quit")
        return False


def fac_fan_setmac_fru(ret):
    fans =  FRULISTS.get('fans')
    
    fanfrus = {}
    newfrus = {}
    
    #getmsg
    try:
        for fan in fans:
            print("===============%s ================getmessage" % fan.get('name'))
            eeprom = getsysvalue(I2CUTIL.getE2File(fan.get('bus'), fan.get('loc')))
            fru = ipmifru()
            fru.decodeBin(eeprom)
            fanfrus[fan.get('name')] = fru
    except Exception as e:
        print(str(e))
        return False
    
    #setmsg
    for fan in fans:
        print("===============%s ================setmac" % fan.get('name'))
        fruold = fanfrus.get(fan.get('name'))
        newfru = getInputSetmac(fruold)
        newfru.recalcute()
        newfrus[fan.get('name')] = newfru
    #writemsg
    for fan in fans:
        print("===============%s ================writeToE2" % fan.get('name'))
        ret_t = newfrus.get(fan.get('name'))
        I2CUTIL.openFanE2Protect()
        I2CUTIL.writeToFanE2(fan.get('bus'), fan.get('loc'), ret_t.bindata)
        I2CUTIL.closeFanE2Protect()
    #check
    try:
        for fan in fans:
            print("===============%s ================getmessage" % fan.get('name'))
            eeprom = getsysvalue(I2CUTIL.getE2File(fan.get('bus'), fan.get('loc')))
            fru = ipmifru()
            fru.decodeBin(eeprom)
    except Exception as e:
        print(str(e))
        return False
    return True

def fac_fans_setmac():
    ret =  I2CUTIL.getvaluefromdevice("rg_fan")
    if ret is not None and len(ret) > 0:
        return fac_fans_setmac_tlv(ret)
    fans =  FRULISTS.get('fans', None)
    if fans is not None and len(fans)>0:
        return fac_fan_setmac_fru(ret)
    return False

def fac_fan_setmac(item):
    I2CUTIL.openFanE2Protect()
    I2CUTIL.writeToFanE2(item.fanbus, item.fanloc, item.generate_fan_value())
    I2CUTIL.closeFanE2Protect()

def writeToEEprom(rst_arr):
    dealtype = E2_PROTECT.get('gettype',None)
    if dealtype is None:
        rji2cset(E2_PROTECT["bus"], E2_PROTECT["devno"],
                 E2_PROTECT["addr"], E2_PROTECT["open"])
    elif dealtype == "io":
        io_wr(E2_PROTECT["io_addr"], E2_PROTECT["open"])
    index = 0
    for item in rst_arr:
        rji2cset(E2_LOC["bus"], E2_LOC["devno"], index, ord(item))
        index += 1
    
    if dealtype is None:
        rji2cset(E2_PROTECT["bus"], E2_PROTECT["devno"],
                 E2_PROTECT["addr"], E2_PROTECT["close"])
    elif dealtype == "io":
        io_wr(E2_PROTECT["io_addr"], E2_PROTECT["close"])
    # deal last drivers
    getstatusoutput_noshell(["rmmod", "at24"])
    getstatusoutput_noshell(["modprobe", "at24"])
    getstatusoutput_noshell(["rm", "-f", "/var/cache/sonic/decode-syseeprom/syseeprom_cache"])


def get_local_eth0_mac():
    cmd1 = ["ifconfig", "eth0"]
    cmd2 = ["grep", "HWaddr"]
    print(getstatusoutput_noshell_pipe(cmd1, cmd2))

def getonieversion():
    if not os.path.isfile('/host/machine.conf'):
        return ""
    machine_vars = {}
    with open('/host/machine.conf') as machine_file:
        for line in machine_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            machine_vars[tokens[0]] = tokens[1].strip()
    return machine_vars.get("onie_version")

def createbmcMac(cpumac , num = 2):
    bcmvalue =  strtoint(cpumac[cpumac.rindex(":")+ 1:len(cpumac)]) + num
    # bmcmac = 
    t = cpumac.split(":")
    t[5] = "%02x" %  bcmvalue
    bmcmac = ":".join(t)
    return bmcmac.upper()


def fac_board_setmac():
    _value = {}
    # default value
    _value[TLV_CODE_VENDOR_EXT] = generate_ext(RUIJIE_CARDID)  # generate id
    _value[TLV_CODE_PRODUCT_NAME] = RUIJIE_PRODUCTNAME
    _value[TLV_CODE_PART_NUMBER]    = RUIJIE_PART_NUMBER
    _value[TLV_CODE_LABEL_REVISION] = RUIJIE_LABEL_REVISION
    _value[TLV_CODE_PLATFORM_NAME]  = platform
    _value[TLV_CODE_ONIE_VERSION]   = getonieversion()
    _value[TLV_CODE_MAC_SIZE]       = RUIJIE_MAC_SIZE
    _value[TLV_CODE_MANUF_NAME]     = RUIJIE_MANUF_NAME
    _value[TLV_CODE_MANUF_COUNTRY]  = RUIJIE_MANUF_COUNTRY
    _value[TLV_CODE_VENDOR_NAME]    = RUIJIE_VENDOR_NAME
    _value[TLV_CODE_DIAG_VERSION]   = RUIJIE_DIAG_VERSION
    _value[TLV_CODE_SERVICE_TAG]    = RUIJIE_SERVICE_TAG
    try:
        if 0x00004052 == RUIJIE_CARDID:
            _value[TLV_CODE_PRODUCT_NAME] = RUIJIE_PRODUCTNAME + "-RJ"
        elif 0x00004051 == RUIJIE_CARDID or 0x00004050 == RUIJIE_CARDID:
            changeTypeValue(_value, TLV_CODE_PRODUCT_NAME,
                        "Product name",RUIJIE_PRODUCTNAME)

        changeTypeValue(_value, TLV_CODE_SERIAL_NUMBER,
                        "SN", "0000000000000")  # add serial number
        changeTypeValue(_value, TLV_CODE_DEVICE_VERSION,
                        "hardware version", "101")  # hardware version
        changeTypeValue(_value, TLV_CODE_MAC_BASE,
                        "MAC address", "58696cfb2108")  # MAC address
        _value[TLV_CODE_MANUF_DATE] = time.strftime(
            '%m/%d/%Y %H:%M:%S', time.localtime())  # add setmac time
        rst, ret = generate_value(_value)
        if util_setmac("eth0", _value[TLV_CODE_MAC_BASE]) == True:  #  set  Internet cardIP
            writeToEEprom(rst)  # write to e2
            # set BMC MAC
            if "bmcsetmac" in FACTESTMODULE and FACTESTMODULE['bmcsetmac'] == 1:
                bmcmac = createbmcMac(_value[TLV_CODE_MAC_BASE])
                if ipmi_set_mac(bmcmac) == True:
                    print("BMC  MAC[%s]"%bmcmac)
                else:
                    print("SET BMC MAC FAILED")
                    return False
        else:
            return False
    except SETMACException as e:
        #print(e)
        RJPRINTERR("\n\n%s\n\n" % e)
        return False
    except ValueError as e:
        return False
    return True


def ipmi_set_mac(mac):
    macs = mac.split(":")
    cmdinit = ["ipmitool", "raw", "0x0c", "0x01", "0x01", "0xc2", "0x00"]
    cmdset = ["ipmitool", "raw", "0x0c", "0x01", "0x01", "0x05"]
    for ind in range(len(macs)):
        cmdset.append("0x%02x" % int(macs[ind], 16))
    getstatusoutput_noshell(cmdinit)
    ret, status = getstatusoutput_noshell(cmdset)
    if ret:
        RJPRINTERR("\n\n%s\n\n" % status)
        return False
    return True


def getInputValue(title, tips):
    print("Please input[%s]such as(%s):" % (title, tips))
    name = raw_input()

    return name


def bmc_setmac():
    tips = "BMC MAC"
    print("Please input value you want to change[%s]:" % tips)
    name = raw_input()
    if len(name) != 12:
        RJPRINTERR("\nMAC address invaild, try again\n")
        return False
    release_mac = ""
    for i in range(int(len(name) / 2)):
        if i == 0:
            release_mac += name[i * 2:i * 2 + 2]
        else:
            release_mac += ":" + name[i * 2:i * 2 + 2]
    if isValidMac(release_mac) == True:
        if ipmi_set_mac(release_mac) == True:
            return True
    else:
        RJPRINTERR("\nMAC address invaild, try again\n")
    return False


def closeProtocol():
    # disable LLDP
    log_info("disable LLDP")
    sys.stdout.write(".")
    sys.stdout.flush()
    getstatusoutput_noshell(["systemctl", "stop", "lldp.service"])
    log_info("disable lldp service")
    sys.stdout.write(".")
    sys.stdout.flush()
    getstatusoutput_noshell(["systemctl", "stop", "bgp.service"])
    log_info("disable bgp service")
    sys.stdout.write(".")
    sys.stdout.flush()
    #ret, status = rj_os_system('bcmcmd "port ce,xe stp=disable"')

# check SDK memory must be 256M


def checkSdkMem():
    ind = 0
    file_data = ""
    with open(file_name, "r") as f:
        for line in f:
            if "dmasize=16M" in line:
                line = line.replace("dmasize=16M", "dmasize=256M")
                ind = -1
            file_data += line
    if ind == 0:
        return
    with open(file_name, "w") as f:
        f.write(file_data)
    print("change SDK memory to 256, reboot required")
    getstatusoutput_noshell(["sync"])
    getstatusoutput_noshell(["reboot"])

##########################################################################
# receives a character setting
##########################################################################


def getch(msg):
    ret = ""
    fd = sys.stdin.fileno()
    old_ttyinfo = termios.tcgetattr(fd)
    new_ttyinfo = old_ttyinfo[:]
    new_ttyinfo[3] &= ~termios.ICANON
    new_ttyinfo[3] &= ~termios.ECHO
    sys.stdout.write(msg)
    sys.stdout.flush()
    try:
        termios.tcsetattr(fd, termios.TCSANOW, new_ttyinfo)
        ret = os.read(fd, 1)
    finally:
        # print "try to setting"
        termios.tcsetattr(fd, termios.TCSANOW, old_ttyinfo)
    return ret

def get_raw_input():
    ret=""
    fd=sys.stdin.fileno()
    old_ttyinfo=termios.tcgetattr(fd)
    new_ttyinfo=old_ttyinfo[:]
    new_ttyinfo[3] &= ~termios.ICANON
    new_ttyinfo[3] &= ~termios.ECHO
    try:
        termios.tcsetattr(fd,termios.TCSANOW,new_ttyinfo)
        ret=raw_input("")
    except Exception as e:
        print(e)
    finally:
        termios.tcsetattr(fd,termios.TCSANOW,old_ttyinfo)
    return ret


def getsysvalue(location):
    retval = None
    mb_reg_file = location
    if (not os.path.isfile(mb_reg_file)):
        print(mb_reg_file,  'not found !')
        return retval
    try:
        if (not os.path.isfile(mb_reg_file)):
            print(mb_reg_file,  'not found !')
            return retval
        with open(mb_reg_file, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        log_error("Unable to open " + mb_reg_file + "file !")
    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    #log_debug(retval)
    return retval

# get file value


def get_pmc_register(reg_name):
    retval = 'ERR'
    mb_reg_file = MAILBOX_DIR + reg_name
    filepath = glob.glob(mb_reg_file)
    if(len(filepath) == 0):
        return "%s %s  notfound"% (retval , mb_reg_file)
    mb_reg_file = filepath[0]       
    if (not os.path.isfile(mb_reg_file)):
        return "%s %s  notfound"% (retval , mb_reg_file)
    try:
        with open(mb_reg_file, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        pass
    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    return retval

# decode EEPROM


def decoder(s, t):
    if ord(t[0]) == TLV_CODE_PRODUCT_NAME:
        name = "Product Name"
        value = str(t[2:2 + ord(t[1])])
    elif ord(t[0]) == TLV_CODE_PART_NUMBER:
        name = "Part Number"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_SERIAL_NUMBER:
        name = "Serial Number"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_MAC_BASE:
        name = "Base MAC Address"
        value = ":".join([binascii.b2a_hex(T) for T in t[2:8]]).upper()
    elif ord(t[0]) == TLV_CODE_MANUF_DATE:
        name = "Manufacture Date"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_DEVICE_VERSION:
        name = "Device Version"
        value = str(ord(t[2]))
    elif ord(t[0]) == TLV_CODE_LABEL_REVISION:
        name = "Label Revision"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_PLATFORM_NAME:
        name = "Platform Name"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_ONIE_VERSION:
        name = "ONIE Version"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_MAC_SIZE:
        name = "MAC Addresses"
        value = str((ord(t[2]) << 8) | ord(t[3]))
    elif ord(t[0]) == TLV_CODE_MANUF_NAME:
        name = "Manufacturer"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_MANUF_COUNTRY:
        name = "Manufacture Country"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_VENDOR_NAME:
        name = "Vendor Name"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_DIAG_VERSION:
        name = "Diag Version"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_SERVICE_TAG:
        name = "Service Tag"
        value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_VENDOR_EXT:
        name = "Vendor Extension"
        value = ""
        if _TLV_DISPLAY_VENDOR_EXT:
            value = t[2:2 + ord(t[1])]
    elif ord(t[0]) == TLV_CODE_CRC_32 and len(t) == 6:
        name = "CRC-32"
        value = "0x%08X" % (((ord(t[2]) << 24) | (
            ord(t[3]) << 16) | (ord(t[4]) << 8) | ord(t[5])),)
    elif ord(t[0]) == TLV_CODE_RJ_CARID:
        name = "rj_cardid"
        value = ""
        for c in t[2:2 + ord(t[1])]:
            value += "%02X" % (ord(c),)
    else:
        name = "Unknown"
        value = ""
        for c in t[2:2 + ord(t[1])]:
            value += "0x%02X " % (ord(c),)
    return {"name": name, "code": ord(t[0]), "value": value}


def decode_eeprom(e):
    total_len = (ord(e[9]) << 8) | ord(e[10])
    tlv_index = _TLV_INFO_HDR_LEN
    tlv_end = _TLV_INFO_HDR_LEN + total_len
    ret = []
    while (tlv_index + 2) < len(e) and tlv_index < tlv_end:
        rt = decoder(None, e[tlv_index:tlv_index + 2 + ord(e[tlv_index + 1])])
        ret.append(rt)
        if ord(e[tlv_index]) == TLV_CODE_CRC_32:
            break
        tlv_index += ord(e[tlv_index + 1]) + 2
    for item in ret:
        if item['code'] == TLV_CODE_VENDOR_EXT:
            rt = decoder(None, item["value"]
                         [0: 0 + 2 + ord(item["value"][0 + 1])])
            ret.append(rt)
    return ret


def get_sys_eeprom():
    eeprom = get_sysfs_value(rg_eeprom)
    return decode_eeprom(eeprom)

# get card ID
def getCardId():
    ret = get_sys_eeprom()
    for item in ret:
        if item['code'] == TLV_CODE_RJ_CARID:
            return item.get('value',None)
    return None


###########################################
# get memory slot and number via DMI command
###########################################
def getsysmeminfo():
    ret, log = getstatusoutput_noshell(["which", "dmidecode"])
    if ret != 0 or len(log) <= 0:
        error = "cmd find dmidecode"
        return False, error
    cmd1 = [log[0].rstrip('\n')]
    cmd2 = ["grep", "-P", "-A5", "Memory\s+Device"]
    cmd3 = ["grep", "Size"]
    cmd4 = ["grep", "-v", "Range"]
    # get total number first
    result = []
    ret1, log1 = getstatusoutput_noshell_pipe(cmd1, cmd2, cmd3, cmd4)
    if ret1 == 0 and len(log1):
        log1 = log1.lstrip()
        arr = log1.split("\n")
        #total = len(arr)  # total slot number
        for i in range(len(arr)):
            val = re.sub("\D", "", arr[i])
            if val == "":
                val = arr[i].lstrip()
                val = re.sub('Size:', '', val).lstrip()
            # print val
            result.append({"slot": i + 1, "size": val})
        return True, result
    return False, "error"

###########################################
# get memory slot and number via DMI command
# return various arrays
###########################################
def getsysmeminfo_detail():
    ret, log = getstatusoutput_noshell(["which", "dmidecode"])
    if ret != 0 or len(log) <= 0:
        error = "cmd find dmidecode"
        return False, error
    cmd1 = [log[0].rstrip('\n')] + ["-t", "17"]
    cmd2 = ["grep", "-A21", "Memory Device"]  # 17
    # get total number
    ret1, log1 = getstatusoutput_noshell_pipe(cmd1, cmd2)
    if ret1 != 0 or len(log1) <= 0:
        return False, "command execution error[%s][%s]" % (cmd1, cmd2)
    result_t = log1.split("--")
    mem_rets = []
    for item in result_t:
        its = item.replace("\t", "").strip().split("\n")
        ret = {}
        for it in its:
            if ":" in it:
                key = it.split(":")[0].lstrip()
                value = it.split(":")[1].lstrip()
                ret[key] = value
        mem_rets.append(ret)
    return True, mem_rets


###########################################
# get BIOS info via DMI command
###########################################
def getDmiSysByType(type_t):
    ret, log = getstatusoutput_noshell(["which", "dmidecode"])
    if ret != 0 or len(log) <= 0:
        error = "cmd find dmidecode"
        return False, error
    cmd = [log[0].rstrip('\n')] + ["-t", str(type_t)]
    # get total number
    ret1, log1 = getstatusoutput_noshell(cmd)
    if ret1 != 0 or len(log1) <= 0:
        return False, "command execution error[%s]" % cmd
    its = log1.replace("\t", "").strip().split("\n")
    ret = {}
    for it in its:
        if ":" in it:
            key = it.split(":")[0].lstrip()
            value = it.split(":")[1].lstrip()
            ret[key] = value
    return True, ret


def gethwsys():
    return getDmiSysByType(1)

###########################################
# get BIOS info via DMI command


def getsysbios():
    return getDmiSysByType(0)


def searchDirByName(name, dir):
    result = []
    try:
        files = os.listdir(dir)
        for file in files:
            if name in file:
                result.append(os.path.join(dir, file))
    except Exception as e:
        pass
    return result


def getUsbLocation():
    dir = "/sys/block/"
    spect = "sd"
    usbpath = ""
    result = searchDirByName(spect, dir)
    if len(result) <= 0:
        return False
    for item in result:
        with open(os.path.join(item, "removable"), 'r') as fd:
            value = fd.read()
            if value.strip() == "1":  # U-Disk found
                usbpath = item
                break
    if usbpath == "":  # no U-Disk found
        log_debug("no usb found")
        return False, usbpath
    return True, usbpath

# judge USB file
def getusbinfo():
    ret, path = getUsbLocation()
    if ret == False:
        return False, "not usb exists"
    str = os.path.join(path, "size")
    ret, value = getfilevalue(str)
    if ret == True:
        return True, {"id": os.path.basename(path), "size": float(value) * 512 / 1024 / 1024 / 1024}
    else:
        return False, "Err"

def get_cpu_info():
    cmd1 = ["cat", "/proc/cpuinfo"]
    cmd2 = ["grep", "processor", "-A18"]  # 17

    ret, log1 = getstatusoutput_noshell_pipe(cmd1, cmd2)
    if ret != 0 or len(log1) <= 0:
        return False, "command execution error[%s][%s]" % (cmd1, cmd2)
    result_t = log1.split("--")
    mem_rets = []
    for item in result_t:
        its = item.replace("\t", "").strip().split("\n")
        ret = {}
        for it in its:
            if ":" in it:
                key = it.split(":")[0].lstrip()
                value = it.split(":")[1].lstrip()
                ret[key] = value
        mem_rets.append(ret)
    return True, mem_rets
#  read file
def get_version_config_info(attr_key, file_name=None):
    if file_name is None:
        version_conf_filename = "/root/version.json"
    else:
        version_conf_filename = file_name
    if not os.path.isfile(version_conf_filename):
        return None
    with open(version_conf_filename) as rjconf_file:
        for line in rjconf_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            if tokens[0] == attr_key:
                return tokens[1].strip()
    return None


def io_rd(reg_addr, len =1):
    u'''io read'''
    try: 
        regaddr = 0
        if type(reg_addr) == int:
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        devfile = "/dev/port"
        fd = os.open(devfile, os.O_RDWR|os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        str = os.read(fd, len)
        return "".join(["%02x"% ord(item) for item in str])
    except ValueError: 
        return None
    except Exception as e:
        print(e)
        return None
    finally:
        os.close(fd)
    

def io_wr(reg_addr, reg_data):
    u'''io write'''
    try:
        regdata  = 0
        regaddr  = 0
        if type(reg_addr) == int:
            regaddr = reg_addr
        else:
            regaddr = int(reg_addr, 16)
        if type(reg_data) == int:
            regdata = reg_data
        else:
            regdata = int(reg_data, 16)
        devfile = "/dev/port"
        fd = os.open(devfile, os.O_RDWR|os.O_CREAT)
        os.lseek(fd, regaddr, os.SEEK_SET)
        os.write(fd, chr(regdata))
        return True
    except ValueError as e:
        print(e)
        return False
    except Exception as e:
        print(e)
        return False
    finally:
        os.close(fd)
    
