#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import binascii
import sys
import os
import json
import re
import shutil
import click

from eepromutil.fru import ipmifru, BoardInfoArea, ProductInfoArea
from eepromutil.cust_fru import CustFru
from eepromutil.fantlv import fan_tlv
from eepromutil.wedge import Wedge
from eepromutil.wedge_v5 import WedgeV5
import eepromutil.onietlv as ot
from platform_config import PLATFORM_E2_CONF
from platform_util import byteTostr, dev_file_read, exec_os_cmd

PYTHON_VERSION = sys.version_info.major
GENERATE_RAWDATA_NUM = 0
OUTPUT_DIR = "output/"
SUPPORT_E2_TYPE = ("onie_tlv", "fru", "fantlv", "custfru", "wedge", "wedge_v5")
E2_NAME_CLICK_HELP = "Display eeprom information of specified name, such as "

for e2_key, e2_conf_list in sorted(PLATFORM_E2_CONF.items()):
    E2_NAME_CLICK_HELP += e2_key + ", "
    for e2_conf in e2_conf_list:
        e2_name = e2_conf["name"]
        if e2_name == e2_key:
            continue
        E2_NAME_CLICK_HELP += e2_name + ", "

E2_NAME_CLICK_HELP = E2_NAME_CLICK_HELP.rstrip(", ")


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


class ExtraFunc(object):
    @staticmethod
    def decode_mac(encodedata):
        if encodedata is None:
            return None
        ret = ":".join("%02x" % ord(data) for data in encodedata)
        return ret.upper()

    @staticmethod
    def decode_mac_number(encodedata):
        if encodedata is None:
            return None
        return (ord(encodedata[0]) << 8) | (ord(encodedata[1]) & 0x00ff)

    @staticmethod
    def fru_decode_mac_number(params):
        ipmi_fru = params.get("fru")
        area = params.get("area")
        field = params.get("field")
        area_info = getattr(ipmi_fru, area, None)
        if area_info is not None:
            raw_mac_number = getattr(area_info, field, None)
            mac_number = ExtraFunc.decode_mac_number(raw_mac_number)
            ipmi_fru.setValue(area, field, mac_number)

    @staticmethod
    def fru_decode_mac(params):
        ipmi_fru = params.get("fru")
        area = params.get("area")
        field = params.get("field")
        area_info = getattr(ipmi_fru, area, None)
        if area_info is not None:
            raw_mac = getattr(area_info, field, None)
            decoded_mac = ExtraFunc.decode_mac(raw_mac)
            ipmi_fru.setValue(area, field, decoded_mac)

    @staticmethod
    def fru_decode_hw(params):
        ipmi_fru = params.get("fru")
        area = params.get("area")
        field = params.get("field")
        area_info = getattr(ipmi_fru, area, None)
        if area_info is not None:
            raw_hw = getattr(area_info, field, None)
            decode_hw = str(int(raw_hw, 16))
            ipmi_fru.setValue(area, field, decode_hw)


def set_onie_value(params):
    onie = params.get("onie")
    field = params.get("field")
    config_value = params.get("config_value")
    for index, onie_item in enumerate(onie):
        if onie_item.get("name") == field:
            if "value" in onie_item.keys():
                onie[index]["value"] = config_value


def onie_eeprom_decode(onie, e2_decode):
    for e2_decode_item in e2_decode:
        field = e2_decode_item.get("field")
        decode_type = e2_decode_item.get("decode_type")
        if decode_type == 'func':
            params = {
                "onie": onie,
                "field": field
            }
            func_name = e2_decode_item.get("func_name")
            if func_name is not None:
                run_func(func_name, params)
        elif decode_type == 'config':
            config_value = e2_decode_item.get("config_value")
            if config_value is not None:
                params = {
                    "onie": onie,
                    "field": field,
                    "config_value": config_value
                }
                set_onie_value(params)
        else:
            print("unsupport decode type")
            continue


def onie_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        onietlv = ot.onie_tlv()
        rets = onietlv.decode(eeprom)
        if e2_decode is not None:
            onie_eeprom_decode(rets, e2_decode)
        print("===================%s===================" % e2_name)
        print("%-20s %-5s %-5s  %s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            if item["code"] == 0xfd:
                print("%-20s 0x%-02X   %s" % (item["name"], item["code"], item["lens"]))
            else:
                print("%-20s 0x%-02X   %-5s %s" % (item["name"], item["code"], item["lens"], item["value"]))
        return True, ""
    except Exception as e:
        return False, str(e)


def set_fantlv_value(params):
    fantlv_dict = params.get("fantlv")
    field = params.get("field")
    config_value = params.get("config_value")
    for index, fantlv_item in enumerate(fantlv_dict):
        if fantlv_item.get("name") == field:
            if "value" in fantlv_item.keys():
                fantlv_dict[index]["value"] = config_value


def fantlv_eeprom_decode(fantlv_dict, e2_decode):
    for e2_decode_item in e2_decode:
        field = e2_decode_item.get("field")
        decode_type = e2_decode_item.get("decode_type")
        if decode_type == 'func':
            params = {
                "fantlv": fantlv_dict,
                "field": field
            }
            func_name = e2_decode_item.get("func_name")
            if func_name is not None:
                run_func(func_name, params)
        elif decode_type == 'config':
            config_value = e2_decode_item.get("config_value")
            if config_value is not None:
                params = {
                    "fantlv": fantlv_dict,
                    "field": field,
                    "config_value": config_value
                }
                set_fantlv_value(params)
        else:
            print("unsupport decode type")
            continue


def fantlv_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        tlv = fan_tlv()
        rets = tlv.decode(eeprom)
        if len(rets) == 0:
            return False, "fan tlv eeprom info error."
        if e2_decode is not None:
            fantlv_eeprom_decode(rets, e2_decode)
        print("===================%s===================" % e2_name)
        print("%-15s %-5s %-5s  %s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            print("%-15s 0x%-02X   %-5s %s" % (item["name"], item["code"], item["lens"], item["value"]))
        return True, ""
    except Exception as e:
        return False, str(e)


def run_func(funcname, params):
    try:
        func = getattr(ExtraFunc, funcname)
        func(params)
    except Exception as e:
        print(str(e))

def set_fru_value(params):
    ipmi_fru = params.get("fru")
    area = params.get("area")
    field = params.get("field")
    config_value = params.get("config_value")
    ipmi_fru.setValue(area, field, config_value)


def fru_eeprom_decode(ipmi_fru, e2_decode):
    for e2_decode_item in e2_decode:
        area = e2_decode_item.get("area")
        field = e2_decode_item.get("field")
        decode_type = e2_decode_item.get("decode_type")
        if decode_type == 'func':
            params = {
                "fru": ipmi_fru,
                "area": area,
                "field": field
            }
            func_name = e2_decode_item.get("func_name")
            if func_name is not None:
                run_func(func_name, params)
        elif decode_type == 'config':
            config_value = e2_decode_item.get("config_value")
            if config_value is not None:
                params = {
                    "fru": ipmi_fru,
                    "area": area,
                    "field": field,
                    "config_value": config_value
                }
                set_fru_value(params)
        else:
            print("unsupport decode type")
            continue


def fru_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        ipmi_fru = ipmifru()
        ipmi_fru.decodeBin(eeprom)
        if e2_decode is not None:
            fru_eeprom_decode(ipmi_fru, e2_decode)
        print("===================%s==============" % e2_name)
        print("=================board=================")
        print(ipmi_fru.boardInfoArea)
        print("=================product=================")
        print(ipmi_fru.productInfoArea)
        return True, ""
    except Exception as e:
        return False, str(e)


def custfru_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        custfru = CustFru()
        custfru.decode(eeprom)
        print("===================%s==============" % e2_name)
        print(custfru)
        return True, ""
    except Exception as e:
        return False, str(e)


def wedge_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        wegde = Wedge()
        wegde.decode(eeprom)
        print("===================%s==============" % e2_name)
        print(wegde)
        return True, ""
    except Exception as e:
        return False, str(e)


def wedge_v5_eeprom_show(e2_name, eeprom, e2_decode=None):
    try:
        wegdev5 = WedgeV5()
        rets    = wegdev5.decode(eeprom)
        print("===================%s===================" % e2_name)
        print("%-32s %-5s %-5s  %s" % ("TLV name","Type","Length","Value"))
        for item in rets:
            print("%-32s %-5d   %-5s %s" % (item["name"],item["code"],item["lens"],item["value"]))
        return True, ""
    except Exception as e:
        return False, str(e)


def eeprom_parse_by_type(e2_type, name, binval, e2_decode):
    if e2_type == "onie_tlv":
        return onie_eeprom_show(name, binval, e2_decode)

    if e2_type == "fru":
        return fru_eeprom_show(name, binval, e2_decode)

    if e2_type == "fantlv":
        return fantlv_eeprom_show(name, binval, e2_decode)

    if e2_type == "custfru":
        return custfru_eeprom_show(name, binval, e2_decode)

    if e2_type == "wedge":
        return wedge_eeprom_show(name, binval, e2_decode)

    if e2_type == "wedge_v5":
        return wedge_v5_eeprom_show(name, binval, e2_decode)

    return False, "Unsupport e2_type: %s" % e2_type


def eeprom_parse_traverse(name, binval, e2_decode=None):
    errmsg = ""
    support_e2_type = ("onie_tlv", "fru", "fantlv", "custfru", "wedge", "wedge_v5")
    for e2_type in support_e2_type:
        status, msg = eeprom_parse_by_type(e2_type, name, binval, e2_decode)
        if status is True:
            return True, ""
        errmsg = "%s    %s\n" % (errmsg, msg)
    return False, errmsg


def eeprom_parse_file(e2_path, e2_size):
    if e2_size is None:
        e2_size = os.path.getsize(e2_path)
    ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
    if ret is False:
        print("eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval_bytes))
        return
    binval = byteTostr(binval_bytes)
    status, msg = eeprom_parse_traverse(e2_path, binval)
    if status is not True:
        print("===================%s===================" % e2_path)
        print("parse [%s] eeprom failed, errmsg:" % e2_path)
        print("%s" % msg)
    return


def eeprom_parse_dir(e2_path, e2_size):
    for root, dirs, names in os.walk(e2_path):
        # root: directory absolute path
        # dirs: folder path collection under directory
        # names: file path collection under directory
        for filename in names:
            # file_path is file absolute path
            file_path = os.path.join(root, filename)
            eeprom_parse_file(file_path, e2_size)
    return


def eeprom_parse(e2_path, e2_size):
    if os.path.isdir(e2_path):
        eeprom_parse_dir(e2_path, e2_size)
    elif os.path.isfile(e2_path):
        eeprom_parse_file(e2_path, e2_size)
    else:
        msg = "path: %s not found" % e2_path
        print(msg)
    return


def eeprom_parse_by_config(eeprom_conf):
    errmsg = ""
    name = eeprom_conf.get("name")
    e2_type = eeprom_conf.get("e2_type")
    e2_path = eeprom_conf.get("e2_path")
    e2_size = eeprom_conf.get("e2_size", 256)
    e2_decode = eeprom_conf.get("e2_decode")
    ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
    if ret is False:
        print("===================%s===================" % name)
        print("eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval_bytes))
        return
    binval = byteTostr(binval_bytes)

    if e2_type is None:
        status, msg = eeprom_parse_traverse(name, binval, e2_decode)
        if status is not True:
            print("===================%s===================" % name)
            print("parse [%s] eeprom failed, errmsg:" % name)
            print("%s" % msg)
        return

    if isinstance(e2_type, str):
        status, msg = eeprom_parse_by_type(e2_type, name, binval, e2_decode)
        if status is not True:
            print("===================%s===================" % name)
            print("parse [%s] eeprom failed, errmsg: %s" % (name, msg))
        return

    if isinstance(e2_type, list):
        for e2_type_item in e2_type:
            status, msg = eeprom_parse_by_type(e2_type_item, name, binval, e2_decode)
            if status is True:
                return
            errmsg = "%s    %s\n" % (errmsg, msg)
        print("===================%s===================" % name)
        print("parse [%s] eeprom failed, errmsg:" % name)
        print("%s" % errmsg)
        return

    print("===================%s===================" % name)
    print("Unsupprot e2_type config: %s" % type(e2_type))
    return


def get_fans_eeprom_info(param):
    fan_eeprom_conf = PLATFORM_E2_CONF.get("fan", [])
    fan_num = len(fan_eeprom_conf)
    if fan_num == 0:
        print("fan number is 0, can't get fan eeprom info")
        return
    if param == 'all':
        for conf in fan_eeprom_conf:
            eeprom_parse_by_config(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    fan_index = int(param, 10) - 1
    if fan_index < 0 or fan_index >= fan_num:
        print("param error, total fan number: %d, fan index: %d" % (fan_num, fan_index + 1))
        return
    eeprom_parse_by_config(fan_eeprom_conf[fan_index])
    return


def get_psus_eeprom_info(param):
    psu_eeprom_conf = PLATFORM_E2_CONF.get("psu", [])
    psu_num = len(psu_eeprom_conf)
    if psu_num == 0:
        print("psu number is 0, can't get psu eeprom info")
        return
    if param == 'all':
        for conf in psu_eeprom_conf:
            eeprom_parse_by_config(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    psu_index = int(param, 10) - 1
    if psu_index < 0 or psu_index >= psu_num:
        print("param error, total psu number: %d, psu index: %d" % (psu_num, psu_index + 1))
        return
    eeprom_parse_by_config(psu_eeprom_conf[psu_index])
    return


def get_slots_eeprom_info(param):
    slot_eeprom_conf = PLATFORM_E2_CONF.get("slot", [])
    slot_num = len(slot_eeprom_conf)
    if slot_num == 0:
        print("slot number is 0, can't get slot eeprom info")
        return
    if param == 'all':
        for conf in slot_eeprom_conf:
            eeprom_parse_by_config(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    slot_index = int(param, 10) - 1
    if slot_index < 0 or slot_index >= slot_num:
        print("param error, total slot number: %d, slot index: %d" % (slot_num, slot_index + 1))
        return
    eeprom_parse_by_config(slot_eeprom_conf[slot_index])
    return


def get_syseeprom_info(param):
    syseeprom_conf = PLATFORM_E2_CONF.get("syseeprom", [])
    syseeprom_num = len(syseeprom_conf)
    if syseeprom_num == 0:
        print("syseeprom number is 0, can't get syseeprom info")
        return
    if param == 'all':
        for conf in syseeprom_conf:
            eeprom_parse_by_config(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    syseeprom_index = int(param, 10) - 1
    if syseeprom_index < 0 or syseeprom_index >= syseeprom_num:
        print("param error, total syseeprom number: %d, syseeprom index: %d" % (syseeprom_num, syseeprom_index + 1))
        return
    eeprom_parse_by_config(syseeprom_conf[syseeprom_index])
    return


def get_all_eeprom_info():
    for e2_key, e2_conf in sorted(PLATFORM_E2_CONF.items()):
        for conf in e2_conf:
            eeprom_parse_by_config(conf)


def get_specified_eeprom_info(e2_type):
    if e2_type not in SUPPORT_E2_TYPE:
        print("Unsupprot e2_type %s" % e2_type)
        return

    match_flag = 0
    for e2_key, e2_conf in sorted(PLATFORM_E2_CONF.items()):
        for conf in e2_conf:
            name = conf.get("name")
            conf_e2_type = conf.get("e2_type")
            e2_path = conf.get("e2_path")
            e2_size = conf.get("e2_size", 256)
            e2_decode = conf.get("e2_decode")
            if conf_e2_type is None or (isinstance(conf_e2_type, list) and e2_type in conf_e2_type):
                ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
                if ret is False:
                    # Since it is not sure whether the E2 type matches, don't print error logs.
                    continue

                binval = byteTostr(binval_bytes)
                status, msg = eeprom_parse_by_type(e2_type, name, binval, e2_decode)
                if status is True:
                    match_flag = 1
                continue

            if isinstance(conf_e2_type, str) and conf_e2_type == e2_type:
                match_flag = 1
                ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
                if ret is False:
                    print("===================%s===================" % name)
                    print("eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval_bytes))
                    continue

                binval = byteTostr(binval_bytes)
                status, msg = eeprom_parse_by_type(e2_type, name, binval, e2_decode)
                if status is not True:
                    print("===================%s===================" % name)
                    print("parse [%s] eeprom failed, errmsg: %s" % (name, msg))
    if match_flag == 0:
        print("The eeprom type [%s] was not found in the machine" % e2_type)
    return


def traverse_eeprom_by_name(e2_name):
    match_flag = False
    for e2_key, e2_conf_list in sorted(PLATFORM_E2_CONF.items()):
        for e2_conf in e2_conf_list:
            if e2_conf["name"] == e2_name:
                match_flag = True
                eeprom_parse_by_config(e2_conf)
    return match_flag


def get_eeprom_info_by_name(e2_name):
    e2_conf_list = PLATFORM_E2_CONF.get(e2_name)
    if e2_conf_list is not None: # display all the eeprom information of e2_conf_list
        for e2_conf in e2_conf_list:
            eeprom_parse_by_config(e2_conf)
        return

    # e2_conf_list is None, traverse the configuration to display the specified name eeprom information
    status = traverse_eeprom_by_name(e2_name)
    if status is False:
        print("Can't find %s eeprom information" % e2_name)
    return


def get_eeprom_config_by_json_file(file_path):
    if not os.path.isfile(file_path):
        msg = "file path: %s not exits" % file_path
        return False, msg
    with open(file_path, 'r') as jsonfile:
        json_dict = json.load(jsonfile)
    return True, json_dict


def write_rawdata_to_file(rawdata,out_file):
    out_file_dir = os.path.dirname(out_file)
    if len(out_file_dir) != 0:
        cmd = "mkdir -p %s" % out_file_dir
        exec_os_cmd(cmd)
    data_array = bytearray()
    for x in rawdata:
        data_array.append(ord(x))
    with open(out_file, 'wb') as fd:
        fd.write(data_array)
    return


def isValidMac(mac):
    if re.match(r"^\s*([0-9a-fA-F]{2,2}:){5,5}[0-9a-fA-F]{2,2}\s*$", mac):
        return True
    return False


def mac_addr_decode(origin_mac):
    mac = origin_mac.replace("0x", "")
    if len(mac) != 12:
        msg = "Invalid MAC address: %s" % origin_mac
        return False, msg
    release_mac = ""
    for i in range(len(mac) // 2):
        if i == 0:
            release_mac += mac[i * 2:i * 2 + 2]
        else:
            release_mac += ":" + mac[i * 2:i * 2 + 2]
    return True, release_mac


def json_list_value_decode(list_value):
    u'''json file'''
    ret = ""
    for item in list_value:
        value = int(item, 16)
        ret += chr(value)
    return ret


def get_ext_tlv_body(tlv_type, tlv_value):
    ret = ""
    ret += (chr(tlv_type))
    ret += (chr(len(tlv_value)))
    if isinstance(tlv_value, str):
        ret += tlv_value
    elif isinstance(tlv_value, list):
        ret += json_list_value_decode(tlv_value)
    else:
        msg = "unsupport onie tlv value type: %s, value: %s" % (type(tlv_value), tlv_value)
        return False, msg
    return True, ret


def generate_ext(ext_dict):
    ret = ""
    iana = ext_dict.get("iana")
    if iana is not None:
        if isinstance(iana, str):
            ret += iana
        elif isinstance(iana, list):
            ret += json_list_value_decode(iana)
        else:
            msg = "unsupport iana type: %s, value: %s" % (type(iana), iana)
            return False, msg
        del ext_dict['iana']

    key_list = sorted(ext_dict.keys())
    for key in key_list:
        if not key.startswith("code_"):   # tlv type format msut be "code_XX"
            continue
        tlv_type = int(key[5:], 16)
        tlv_value = ext_dict[key]
        status, ret_tmp= get_ext_tlv_body(tlv_type, tlv_value)
        if status is False:
            return False, ret_tmp
        ret += ret_tmp
    return True, ret


def check_mac_addr(mac):
    if mac.startswith("0x"):
        status, mac = mac_addr_decode(mac)
        if status is False:
            return False, mac

    if isValidMac(mac) is False:
        msg = "Invalid MAC address: %s" % mac
        return False, msg
    return True, mac


def generate_onie_tlv_value(onie_tlv_dict):
    global GENERATE_RAWDATA_NUM
    _value = {}
    try:
        generate_flag = onie_tlv_dict.get("generate_flag", 0)
        if generate_flag == 0:
            return
        GENERATE_RAWDATA_NUM += 1
        e2_size = onie_tlv_dict.get("e2_size", 256)
        e2_name = onie_tlv_dict.get("e2_name")
        if e2_name is None:
            print("onie tlv config error, e2_name is None, please check")
            return
        onietlv = ot.onie_tlv()
        key_list = sorted(onie_tlv_dict.keys())
        for key in key_list:
            if not key.startswith("code_"):   # tlv type format msut be "code_XX"
                continue
            tlv_type = int(key[5:], 16)
            tlv_value = onie_tlv_dict[key]
            if tlv_type == onietlv.TLV_CODE_MAC_BASE:
                status, ret = check_mac_addr(tlv_value)
                if status is False:
                    print("generate onie tlv eeprom rawdata %s failed, errmsg: %s" % (e2_name, ret))
                    return
                tlv_value = ret
            elif tlv_type == onietlv.TLV_CODE_VENDOR_EXT:
                status, ret= generate_ext(tlv_value)
                if status is False:
                    print("generate onie tlv eeprom rawdata %s failed, errmsg: %s" % (e2_name, ret))
                    return
                tlv_value = ret
            _value[tlv_type] = tlv_value

        rawdata, ret = onietlv.generate_value(_value, e2_size)
        write_rawdata_to_file(rawdata, OUTPUT_DIR + e2_name)
        print("generate onie tlv eeprom rawdata success, output file: %s"% (OUTPUT_DIR + e2_name))
        return
    except Exception as e:
        msg = "generate onie tlv eeprom rawdata %s error, errmsg: %s" % (e2_name, str(e))
        print(msg)
        return


def generate_fan_tlv_value(fan_tlv_dict):
    global GENERATE_RAWDATA_NUM
    _value = {}
    try:
        generate_flag = fan_tlv_dict.get("generate_flag", 0)
        if generate_flag == 0:
            return
        GENERATE_RAWDATA_NUM += 1
        e2_size = fan_tlv_dict.get("e2_size", 256)
        e2_name = fan_tlv_dict.get("e2_name")
        tlv_terminator = fan_tlv_dict.get("tlv_terminator", 0)
        if e2_name is None:
            print("fan tlv config error, e2_name is None, please check")
            return

        fantlv = fan_tlv()
        fantlv.typename = fan_tlv_dict["ProductName"]
        fantlv.typesn = fan_tlv_dict["SerialNumber"]
        fantlv.typehwinfo = fan_tlv_dict["HardwareInfo"]
        fantlv.typedevtype = int(fan_tlv_dict["DevType"], 16)
        if tlv_terminator == 1:
            fantlv.typename += "\x00"
            fantlv.typesn += "\x00"
            fantlv.typehwinfo += "\x00"
        rawdata = fantlv.generate_fan_value(e2_size)
        write_rawdata_to_file(rawdata, OUTPUT_DIR + e2_name)
        print("generate fan tlv eeprom rawdata success, output file: %s"% (OUTPUT_DIR + e2_name))
        return
    except Exception as e:
        msg = "generate fan tlv eeprom rawdata %s error, errmsg: %s" % (e2_name, str(e))
        print(msg)
        return


def generate_fru_value(fru_dict):
    global GENERATE_RAWDATA_NUM
    try:
        generate_flag = fru_dict.get("generate_flag", 0)
        if generate_flag == 0:
            return
        GENERATE_RAWDATA_NUM += 1
        e2_size = fru_dict.get("e2_size", 256)
        e2_name = fru_dict.get("e2_name")
        if e2_name is None:
            print("fru config error, e2_name is None, please check")
            return

        boradispresent = fru_dict.get("boardinfoarea_ispresent", 0)
        productispresent = fru_dict.get("productInfoArea_ispresent", 0)
        if boradispresent == 0 and productispresent == 0:
            print("fru config error, boradispresent = 0 abd productispresent = 0")
            return

        bia = None
        pia = None
        if boradispresent != 0:
            boardinfoarea_conf = fru_dict.get("boardinfoarea")
            if boardinfoarea_conf is None:
                print("fru config error, boardinfoarea ispresent but boardinfoarea config is None")
                return
            bia = BoardInfoArea(name="Board Info Area", size=0)
            bia.isPresent = True
            bia.mfg_date =boardinfoarea_conf.get("mfg_date")
            bia.boardManufacturer =boardinfoarea_conf["Manufacturer"]
            bia.boardProductName = boardinfoarea_conf["ProductName"]
            bia.boardSerialNumber = boardinfoarea_conf["SerialNumber"]
            bia.boardPartNumber = boardinfoarea_conf["PartNumber"]
            bia.fruFileId = boardinfoarea_conf["FRUFileID"]
            for i in range(1,11):
                ext_str = "extra%d" % i
                valtmp = "boardextra%d" % i
                val_t = boardinfoarea_conf.get(ext_str)
                if val_t is None:
                    break
                if isinstance(val_t, list):
                    val_t = json_list_value_decode(val_t)
                setattr(bia, valtmp, val_t)

        if productispresent != 0:
            productinfoarea_conf = fru_dict.get("productInfoArea")
            if productinfoarea_conf is None:
                print("fru config error, productinfoarea ispresent but productinfoarea_conf config is None")
                return

            pia = ProductInfoArea(name="Product Info Area ", size=0)
            pia.isPresent = True
            pia.productManufacturer = productinfoarea_conf["Manufacturer"]
            pia.productName = productinfoarea_conf["ProductName"]
            pia.productPartModelName = productinfoarea_conf["PartModelName"]
            pia.productVersion = productinfoarea_conf["Version"]
            pia.productSerialNumber = productinfoarea_conf["SerialNumber"]
            pia.productAssetTag = productinfoarea_conf.get("AssetTag")
            pia.fruFileId = productinfoarea_conf["FRUFileID"]
            for i in range(1,11):
                ext_str = "extra%d" % i
                valtmp = "productextra%d" % i
                val_t = productinfoarea_conf.get(ext_str)
                if val_t is None:
                    break
                if isinstance(val_t, list):
                    val_t = json_list_value_decode(val_t)
                setattr(pia, valtmp, val_t)

        fru = ipmifru()
        if bia is not None:
            fru.boardInfoArea = bia
        if pia is not None:
            fru.productInfoArea = pia
        fru.recalcute(e2_size)
        write_rawdata_to_file(fru.bindata, OUTPUT_DIR + e2_name)
        print("generate fru eeprom rawdata success, output file: %s"% (OUTPUT_DIR + e2_name))
        return
    except Exception as e:
        msg = "generate fru eeprom rawdata %s error, errmsg: %s" % (e2_name, str(e))
        print(msg)
        return


def generate_wedge_value(wedge_dict):
    global GENERATE_RAWDATA_NUM
    try:
        generate_flag = wedge_dict.get("generate_flag", 0)
        if generate_flag == 0:
            return
        GENERATE_RAWDATA_NUM += 1
        e2_size = wedge_dict.get("e2_size", 256)
        e2_name = wedge_dict.get("e2_name")
        if e2_name is None:
            print("wedge V3 config error, e2_name is None, please check")
            return

        wedge = Wedge()
        wedge.fbw_product_name = wedge_dict.get("product_name", "")
        wedge.fbw_product_number = wedge_dict.get("product_number", "")
        wedge.fbw_assembly_number = wedge_dict.get("assembly_number", "")
        wedge.fbw_facebook_pcba_number = wedge_dict.get("fb_pcba_number", "")
        wedge.fbw_facebook_pcb_number = wedge_dict.get("fb_pcb_number", "")
        wedge.fbw_odm_pcba_number = wedge_dict.get("odm_pcba_number", "")
        wedge.fbw_odm_pcba_serial = wedge_dict.get("odm_pcba_serial", "")
        wedge.fbw_production_state = wedge_dict.get("production_state", 0)
        wedge.fbw_product_version = wedge_dict.get("product_version", 0)
        wedge.fbw_product_subversion = wedge_dict.get("product_subversion", 0)
        wedge.fbw_product_serial = wedge_dict.get("product_serial", "")
        wedge.fbw_product_asset = wedge_dict.get("product_asset", "")
        wedge.fbw_system_manufacturer = wedge_dict.get("system_manufacturer", "")
        wedge.fbw_system_manufacturing_date = wedge_dict.get("system_manufacturing_date", "")
        wedge.fbw_pcb_manufacturer = wedge_dict.get("pcb_manufacturer", "")
        wedge.fbw_assembled = wedge_dict.get("assembled", "")
        wedge.fbw_local_mac = wedge_dict.get("local_mac", "")
        wedge.fbw_mac_base = wedge_dict.get("mac_base", "")
        wedge.fbw_mac_size = wedge_dict.get("mac_size", 3)
        wedge.fbw_location = wedge_dict.get("location", "")

        rawdata = wedge.generate_value(e2_size)
        write_rawdata_to_file(rawdata, OUTPUT_DIR + e2_name)
        print("generate wedge V3 eeprom rawdata success, output file: %s"% (OUTPUT_DIR + e2_name))
        return
    except Exception as e:
        msg = "generate wedge V3 eeprom rawdata %s error, errmsg: %s" % (e2_name, str(e))
        print(msg)
        return


def generate_wedge_v5_value(wedge_v5_dict):
    global GENERATE_RAWDATA_NUM
    _value = {}
    try:
        generate_flag = wedge_v5_dict.get("generate_flag", 0)
        if generate_flag == 0:
            return
        GENERATE_RAWDATA_NUM += 1
        e2_size = wedge_v5_dict.get("e2_size", 256)
        e2_name = wedge_v5_dict.get("e2_name")
        if e2_name is None:
            print("wedge V5 config error, e2_name is None, please check")
            return
        wedgetlv = WedgeV5()
        key_list = sorted(wedge_v5_dict.keys())
        for key in key_list:
            if not key.startswith("code_"):   # tlv type format msut be "code_XX"
                continue
            tlv_type = int(key[5:], 16)
            tlv_value = wedge_v5_dict[key]
            if isinstance(tlv_value, list):
                    tlv_value = json_list_value_decode(tlv_value)
            _value[tlv_type] = tlv_value
        rawdata = wedgetlv.generate_value(_value, e2_size)
        write_rawdata_to_file(rawdata, OUTPUT_DIR + e2_name)
        print("generate wedge V5 eeprom rawdata success, output file: %s"% (OUTPUT_DIR + e2_name))
        return
    except Exception as e:
        msg = "generate wedge V5 eeprom rawdata %s error, errmsg: %s" % (e2_name, str(e))
        print(msg)
        return


def unicode_convert(input):
    if isinstance(input, dict):
        return {unicode_convert(key): unicode_convert(value) for key, value in input.iteritems()}
    if isinstance(input, list):
        return [unicode_convert(element) for element in input]
    if isinstance(input, unicode):
        return input.encode('utf-8')
    return input


def generate_eeprom_rawdata(file):
    status, ret = get_eeprom_config_by_json_file(file)
    if status is False:
        print(ret)
        return
    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
    os.mkdir(OUTPUT_DIR)

    if PYTHON_VERSION == 2:
        ret = unicode_convert(ret)

    onie_tlv_conf_list = ret.get("onie_tlv", [])
    rg_tlv_conf_list = ret.get("fan_tlv", [])
    fru_conf_list = ret.get("fru", [])
    wedge_conf_list = ret.get("wedge", [])
    wedge_v5_conf_list = ret.get("wedge_v5", [])

    for onie_tlv_conf in onie_tlv_conf_list:
        generate_onie_tlv_value(onie_tlv_conf)

    for fan_tlv_conf in rg_tlv_conf_list:
        generate_fan_tlv_value(fan_tlv_conf)

    for fru_conf in fru_conf_list:
        generate_fru_value(fru_conf)

    for wedge_conf in wedge_conf_list:
        generate_wedge_value(wedge_conf)

    for wedge_v5_conf in wedge_v5_conf_list:
        generate_wedge_v5_value(wedge_v5_conf)


    if GENERATE_RAWDATA_NUM == 0:
        print("All generate_flag config is 0, no eeprom rawdata was generated.")
    return


@click.group(cls=AliasedGroup, invoke_without_command=True)
@click.help_option('-h', '--help', help='show help info')
@click.option('-p', '--parse', help='Parse eeprom rawdata of the specified path, support directory and file')
@click.option('-f', '--file', help='JSON file to generate eeprom rawdata')
@click.option('-t', '--type', help='Display eeprom information of specified type, support onie_tlv/fru/fantlv/custfru/wedge/wedge_v5')
@click.option('-s', '--size', type=int, help='Parse eeprom rawdata of the specified path, support directory and file', hidden=True)
@click.option('-n', '--name', help=E2_NAME_CLICK_HELP)
@click.pass_context
def main(ctx, parse, file, type, size, name):
    '''platform eeprom display script'''
    if ctx.invoked_subcommand is None and parse is None and file is None and type is None and name is None:
        cli_ctx = click.Context(main)
        click.echo(cli_ctx.get_help())
        return

    if file is not None:
        generate_eeprom_rawdata(file)

    if parse is not None:
        eeprom_parse(parse, size)

    if type is not None:
        get_specified_eeprom_info(type)

    if name is not None:
        get_eeprom_info_by_name(name)


# fan eeprom info display
@main.command()
@click.argument('fan_index', required=True)
def fan(fan_index):
    '''fan_index(1, 2, 3...)/all'''
    get_fans_eeprom_info(fan_index)


# psu eeprom info display
@main.command()
@click.argument('psu_index', required=True)
def psu(psu_index):
    '''psu_index(1, 2, 3...)/all'''
    get_psus_eeprom_info(psu_index)


# slot eeprom info display
@main.command()
@click.argument('slot_index', required=True)
def slot(slot_index):
    '''slot_index(1, 2, 3...)/all'''
    get_slots_eeprom_info(slot_index)


# syseeprom info display
@main.command()
@click.argument('syseeprom_index', required=True)
def syseeprom(syseeprom_index):
    '''syseeprom_index(1, 2, 3...)/all'''
    get_syseeprom_info(syseeprom_index)


# all eeprom info display
@main.command()
def all():
    '''get all eeprom info'''
    get_all_eeprom_info()


if __name__ == '__main__':
    main()
