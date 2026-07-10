#!/usr/bin/python3
# -*- coding: UTF-8 -*-
"""
* onboard temperature sensors
* FAN trays
* PSU
"""
import os
import glob
from lxml import etree as ET

MAILBOX_DIR = "/sys/bus/i2c/devices/"
PORTS_DIR = "/sys/class/net/"
CONFIG_NAME = "dev.xml"


def getPMCreg(location):
    retval = "ERR"
    if not os.path.isfile(location):
        return "%s %s  notfound" % (retval, location)
    try:
        with open(location, "r") as fd:
            retval = fd.read()
    except Exception:
        pass
        # logging.error("Unable to open ", location, "file !")

    retval = retval.rstrip("\r\n")
    retval = retval.lstrip(" ")
    return retval


# Get a mailbox register
def get_pmc_register(reg_name):
    retval = "ERR"
    if reg_name[0:4] == "/rif" or reg_name[0:4] == "/ma1" or reg_name[0:4] == "/eth":
        mb_reg_file = PORTS_DIR + reg_name
    else:
        mb_reg_file = MAILBOX_DIR + reg_name
    filepath = glob.glob(mb_reg_file)
    if len(filepath) == 0:
        return "%s %s  notfound" % (retval, mb_reg_file)
    mb_reg_file = filepath[0]  # use first found patch
    if not os.path.isfile(mb_reg_file):
        return "%s %s  notfound" % (retval, mb_reg_file)
    try:
        with open(mb_reg_file, "r") as fd:
            retval = fd.read()
    except Exception:
        pass
        # logging.error("Unable to open ", mb_reg_file, "file !")

    retval = retval.rstrip("\r\n")
    retval = retval.lstrip(" ")
    return retval


class checktype:
    def __init__(self, test1):
        self.test1 = test1

    @staticmethod
    def check(name, location, bit, value, tips, err1):
        psu_status = int(get_pmc_register(location), 16)
        val = (psu_status & (1 << bit)) >> bit
        if val != value:
            err1["errmsg"] = tips
            err1["code"] = -1
            return -1
        else:
            err1["errmsg"] = "none"
            err1["code"] = 0
            return 0

    @staticmethod
    def getValue(location, bit, type):
        value_t = get_pmc_register(location)
        if value_t.startswith("ERR"):
            return value_t
        if type == 1:
            return float(value_t) / 1000
        elif type == 2:
            return float(value_t) / 100
        elif type == 3:
            psu_status = int(value_t, 16)
            return (psu_status & (1 << bit)) >> bit
        elif type == 4:
            return int(value_t, 10)
        else:
            return value_t

    # temperature
    @staticmethod
    def getTemp(self, name, location, ret_t):
        ret2 = self.getValue(location + "temp1_input", " ", 1)
        ret3 = self.getValue(location + "temp1_max", " ", 1)
        ret4 = self.getValue(location + "temp1_max_hyst", " ", 1)
        ret_t["temp1_input"] = ret2
        ret_t["temp1_max"] = ret3
        ret_t["temp1_max_hyst"] = ret4

    @staticmethod
    def getLM75(name, location, result):
        c1 = checktype
        r1 = {}
        c1.getTemp(c1, name, location, r1)
        result[name] = r1


class status:
    def __init__(self, productname):
        self.productname = productname

    @staticmethod
    def getETroot(filename):
        tree = ET.parse(filename)
        root = tree.getroot()
        return root

    @staticmethod
    def getDecodValue(collection, decode):
        decodes = collection.find("decode")
        testdecode = decodes.find(decode)
        test = {}
        for neighbor in testdecode.iter("code"):
            test[neighbor.attrib["key"]] = neighbor.attrib["value"]
        return test

    @staticmethod
    def getfileValue(location):
        return checktype.getValue(location, " ", " ")

    @staticmethod
    def getETValue(a, filename, tagname):
        root = status.getETroot(filename)
        for neighbor in root.iter(tagname):
            prob_t = {}
            prob_t = neighbor.attrib
            prob_t["errcode"] = 0
            prob_t["errmsg"] = ""
            for pros in neighbor.iter("property"):
                ret = dict(neighbor.attrib.items() + pros.attrib.items())
                if "type" not in ret.keys():
                    val = "0"
                else:
                    val = ret["type"]
                if "bit" not in ret.keys():
                    bit = "0"
                else:
                    bit = ret["bit"]
                s = checktype.getValue(ret["location"], int(bit), int(val))
                if isinstance(s, str) and s.startswith("ERR"):
                    prob_t["errcode"] = -1
                    prob_t["errmsg"] = s
                if "default" in ret.keys():
                    rt = status.getDecodValue(root, ret["decode"])
                    prob_t["errmsg"] = rt[str(s)]
                    if str(s) != ret["default"]:
                        prob_t["errcode"] = -1
                        break
                else:
                    if "decode" in ret.keys():
                        rt = status.getDecodValue(root, ret["decode"])
                        if (
                            ret["decode"] == "psutype"
                            and s.replace("\x00", "").rstrip() not in rt.keys()
                        ):  # PSU type detect
                            prob_t["errcode"] = -1
                            prob_t["errmsg"] = "%s" % ("The power type does not match, please check whether the power is correct!")
                        else:
                            s = rt[str(s).replace("\x00", "").rstrip()]
                name = ret["name"]
                prob_t[name] = str(s)
            a.append(prob_t)

    @staticmethod
    def getCPUValue(a, filename, tagname):
        root = status.getETroot(filename)
        for neighbor in root.iter(tagname):
            location = neighbor.attrib["location"]
        L = []
        for dirpath, dirnames, filenames in os.walk(location):
            for file in filenames:
                if file.endswith("input"):
                    L.append(os.path.join(dirpath, file))
            L = sorted(L, reverse=False)
        for i in range(len(L)):
            prob_t = {}
            prob_t["name"] = getPMCreg("%s/temp%d_label" % (location, i + 1))
            prob_t["temp"] = (
                float(getPMCreg("%s/temp%d_input" % (location, i + 1))) / 1000
            )
            prob_t["alarm"] = (
                float(getPMCreg("%s/temp%d_crit_alarm" % (location, i + 1))) / 1000
            )
            prob_t["crit"] = (
                float(getPMCreg("%s/temp%d_crit" % (location, i + 1))) / 1000
            )
            prob_t["max"] = float(getPMCreg("%s/temp%d_max" % (location, i + 1))) / 1000
            a.append(prob_t)

    @staticmethod
    def getFileName():
        return os.path.dirname(os.path.realpath(__file__)) + "/" + CONFIG_NAME

    @staticmethod
    def getFan(ret):
        _filename = status.getFileName()
        _tagname = "fan"
        status.getvalue(ret, _filename, _tagname)

    @staticmethod
    def checkFan(ret):
        _filename = status.getFileName()
        # _filename = "/usr/local/bin/" + status.getFileName()
        _tagname = "fan"
        status.getETValue(ret, _filename, _tagname)

    @staticmethod
    def getTemp(ret):
        _filename = status.getFileName()
        # _filename = "/usr/local/bin/" + status.getFileName()
        _tagname = "temp"
        status.getETValue(ret, _filename, _tagname)

    @staticmethod
    def getPsu(ret):
        _filename = status.getFileName()
        # _filename = "/usr/local/bin/" + status.getFileName()
        _tagname = "psu"
        status.getETValue(ret, _filename, _tagname)

    @staticmethod
    def getcputemp(ret):
        _filename = status.getFileName()
        _tagname = "cpus"
        status.getCPUValue(ret, _filename, _tagname)

    @staticmethod
    def getMgmtRx(ret):
        _filename = status.getFileName()
        # _filename = "/usr/local/bin/" + status.getFileName()
        _tagname = "mgmt_rx"
        status.getETValue(ret, _filename, _tagname)
