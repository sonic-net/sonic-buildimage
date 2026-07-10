#!/usr/bin/python3
#   * onboard temperature sensors
#   * FAN trays
#   * PSU
#
import os
import xml.etree.ElementTree as ET
import glob
import json
from decimal import Decimal
from fru import ipmifru


MAILBOX_DIR = "/sys/bus/i2c/devices/"
BOARD_ID_PATH = "/sys/module/platform_common/parameters/dfd_my_type"
BOARD_AIRFLOW_PATH = "/etc/sonic/.airflow"


CONFIG_NAME = "dev.xml"


def byteTostr(val):
    strtmp = ''
    for value in val:
        strtmp += chr(value)
    return strtmp


def typeTostr(val):
    if isinstance(val, bytes):
        strtmp = byteTostr(val)
        return strtmp
    return val


def get_board_id():
    if not os.path.exists(BOARD_ID_PATH):
        return "NA"
    with open(BOARD_ID_PATH) as fd:
        id_str = fd.read().strip()
    return "0x%x" % (int(id_str, 10))


def getboardairflow():
    if not os.path.exists(BOARD_AIRFLOW_PATH):
        return "NA"
    with open(BOARD_AIRFLOW_PATH) as fd:
        airflow_str = fd.read().strip()
    data = json.loads(airflow_str)
    airflow = data.get("board", "NA")
    return airflow


boardid = get_board_id()
boardairflow = getboardairflow()


DEV_XML_FILE_LIST = [
    "dev_" + boardid + "_" + boardairflow + ".xml",
    "dev_" + boardid + ".xml",
    "dev_" + boardairflow + ".xml",
]


def dev_file_read(path, offset, read_len):
    retval = "ERR"
    val_list = []
    msg = ""
    ret = ""
    fd = -1

    if not os.path.exists(path):
        return False, "%s %s not found" % (retval, path)

    try:
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.read(fd, read_len)
        for item in ret:
            val_list.append(item)
    except Exception as e:
        msg = str(e)
        return False, "%s %s" % (retval, msg)
    finally:
        if fd > 0:
            os.close(fd)
    return True, val_list


def getPMCreg(location):
    retval = 'ERR'
    if not os.path.isfile(location):
        return "%s %s  notfound" % (retval, location)
    try:
        with open(location, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        return "ERR %s" % str(error)

    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    return retval


# Get a mailbox register
def get_pmc_register(reg_name):
    retval = 'ERR'
    mb_reg_file = reg_name
    filepath = glob.glob(mb_reg_file)
    if len(filepath) == 0:
        return "%s %s  notfound" % (retval, mb_reg_file)
    mb_reg_file = filepath[0]
    if not os.path.isfile(mb_reg_file):
        # print mb_reg_file,  'not found !'
        return "%s %s  notfound" % (retval, mb_reg_file)
    try:
        with open(mb_reg_file, 'rb') as fd:
            retval = fd.read()
        retval = typeTostr(retval)
    except Exception as error:
        retval = "%s %s read failed, msg: %s" % (retval, mb_reg_file, str(error))

    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    return retval


class checktype():
    def __init__(self, test1):
        self.test1 = test1

    @staticmethod
    def getValue(location, bit, data_type, coefficient=1, addend=0):
        try:
            value_t = get_pmc_register(location)
            if value_t.startswith("ERR") or value_t.startswith("NA"):
                return value_t
            if data_type == 1:
                return float('%.1f' % ((float(value_t) / 1000) + addend))
            if data_type == 2:
                return float('%.1f' % (float(value_t) / 100))
            if data_type == 3:
                psu_status = int(value_t, 16)
                return (psu_status & (1 << bit)) >> bit
            if data_type == 4:
                return int(value_t, 10)
            if data_type == 5:
                return float('%.1f' % (float(value_t) / 1000 / 1000))
            if data_type == 6:
                return Decimal(float(value_t) * coefficient / 1000).quantize(Decimal('0.000'))
            return value_t
        except Exception as e:
            value_t = "ERR %s" % str(e)
            return value_t

    # fanFRU
    @staticmethod
    def decodeBinByValue(retval):
        fru = ipmifru()
        fru.decodeBin(retval)
        return fru

    @staticmethod
    def getfruValue(prob_t, root, val):
        try:
            ret, binval_bytes = dev_file_read(val, 0, 256)
            if ret is False:
                return binval_bytes
            binval = byteTostr(binval_bytes)
            fanpro = {}
            ret = checktype.decodeBinByValue(binval)
            fanpro['fan_type'] = ret.productInfoArea.productName
            fanpro['hw_version'] = str(int(ret.productInfoArea.productVersion, 16))
            fanpro['sn'] = ret.productInfoArea.productSerialNumber
            fan_display_name_dict = status.getDecodValue(root, "fan_display_name")
            fan_name = fanpro['fan_type'].strip()
            if len(fan_display_name_dict) == 0:
                return fanpro
            if fan_name not in fan_display_name_dict:
                prob_t['errcode'] = -1
                prob_t['errmsg'] = '%s' % ("ERR fan name: %s not support" % fan_name)
            else:
                fanpro['fan_type'] = fan_display_name_dict[fan_name]
            return fanpro
        except Exception as error:
            return "ERR " + str(error)

    @staticmethod
    def getslotfruValue(val):
        try:
            binval = checktype.getValue(val, 0, 0)
            if binval.startswith("ERR"):
                return binval
            slotpro = {}
            ret = checktype.decodeBinByValue(binval)
            slotpro['slot_type'] = ret.boardInfoArea.boardProductName
            slotpro['hw_version'] = ret.boardInfoArea.boardextra1
            slotpro['sn'] = ret.boardInfoArea.boardSerialNumber
            return slotpro
        except Exception as error:
            return "ERR " + str(error)

    @staticmethod
    def getpsufruValue(prob_t, root, val):
        try:
            psu_match = False
            binval = checktype.getValue(val, 0, 0)
            if binval.startswith("ERR"):
                return binval
            psupro = {}
            ret = checktype.decodeBinByValue(binval)
            psupro['type1'] = ret.productInfoArea.productPartModelName
            psupro['sn'] = ret.productInfoArea.productSerialNumber
            psupro['hw_version'] = ret.productInfoArea.productVersion
            psu_dict = status.getDecodValue(root, "psutype")
            psupro['type1'] = psupro['type1'].strip()
            if len(psu_dict) == 0:
                return psupro
            for psu_name, display_name in psu_dict.items():
                if psu_name in psupro['type1']:
                    psupro['type1'] = display_name
                    psu_match = True
                    break
            if psu_match is not True:
                prob_t['errcode'] = -1
                prob_t['errmsg'] = '%s' % ("ERR psu name: %s not support" % psupro['type1'])
            return psupro
        except Exception as error:
            return "ERR " + str(error)


class status():
    def __init__(self, productname):
        self.productname = productname

    @staticmethod
    def getETroot(filename):
        tree = ET.parse(filename)
        root = tree.getroot()
        return root

    @staticmethod
    def getDecodValue(collection, decode):
        decodes = collection.find('decode')
        testdecode = decodes.find(decode)
        test = {}
        if testdecode is None:
            return test
        for neighbor in testdecode.iter('code'):
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
            prob_t['errcode'] = 0
            prob_t['errmsg'] = ''
            for pros in neighbor.iter("property"):
                ret = dict(list(neighbor.attrib.items()) + list(pros.attrib.items()))
                if ret.get('e2type') == 'fru' and ret.get("name") == "fru":
                    fruval = checktype.getfruValue(prob_t, root, ret["location"])
                    if isinstance(fruval, str) and fruval.startswith("ERR"):
                        prob_t['errcode'] = -1
                        prob_t['errmsg'] = fruval
                        break
                    prob_t.update(fruval)
                    continue

                if ret.get("name") == "psu" and ret.get('e2type') == 'fru':
                    psuval = checktype.getpsufruValue(prob_t, root, ret["location"])
                    if isinstance(psuval, str) and psuval.startswith("ERR"):
                        prob_t['errcode'] = -1
                        prob_t['errmsg'] = psuval
                        break
                    prob_t.update(psuval)
                    continue

                if ret.get("gettype") == "config":
                    prob_t[ret["name"]] = ret["value"]
                    continue

                if 'type' not in ret.keys():
                    val = "0"
                else:
                    val = ret["type"]
                if 'bit' not in ret.keys():
                    bit = "0"
                else:
                    bit = ret["bit"]
                if 'coefficient' not in ret.keys():
                    coefficient = 1
                else:
                    coefficient = float(ret["coefficient"])
                if 'addend' not in ret.keys():
                    addend = 0
                else:
                    addend = float(ret["addend"])

                s = checktype.getValue(ret["location"], int(bit), int(val), coefficient, addend)
                if isinstance(s, str) and s.startswith("ERR"):
                    prob_t['errcode'] = -1
                    prob_t['errmsg'] = s
                    break
                if 'default' in ret.keys():
                    rt = status.getDecodValue(root, ret['decode'])
                    prob_t['errmsg'] = rt[str(s)]
                    if str(s) != ret["default"]:
                        prob_t['errcode'] = -1
                        break
                else:
                    if 'decode' in ret.keys():
                        rt = status.getDecodValue(root, ret['decode'])
                        if (ret['decode'] == "psutype" and s.replace("\x00", "").rstrip() not in rt):
                            prob_t['errcode'] = -1
                            prob_t['errmsg'] = '%s' % ("ERR psu name: %s not support" %
                                                       (s.replace("\x00", "").rstrip()))
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
            prob_t["temp"] = float(getPMCreg("%s/temp%d_input" % (location, i + 1))) / 1000
            prob_t["alarm"] = float(getPMCreg("%s/temp%d_crit_alarm" % (location, i + 1))) / 1000
            prob_t["crit"] = float(getPMCreg("%s/temp%d_crit" % (location, i + 1))) / 1000
            prob_t["max"] = float(getPMCreg("%s/temp%d_max" % (location, i + 1))) / 1000
            a.append(prob_t)

    @staticmethod
    def getFileName():
        fpath = os.path.dirname(os.path.realpath(__file__))
        for file in DEV_XML_FILE_LIST:
            xml = fpath + "/" + file
            if os.path.exists(xml):
                return xml
        return fpath + "/" + CONFIG_NAME

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
    def getDcdc(ret):
        _filename = status.getFileName()
        _tagname = "dcdc"
        status.getETValue(ret, _filename, _tagname)

    @staticmethod
    def getmactemp(ret):
        _filename = status.getFileName()
        _tagname = "mactemp"
        status.getETValue(ret, _filename, _tagname)

    @staticmethod
    def getmacpower(ret):
        _filename = status.getFileName()
        _tagname = "macpower"
        status.getETValue(ret, _filename, _tagname)
