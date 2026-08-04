#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import click

from eepromutil.fru import ipmifru
from eepromutil.fantlv import fan_tlv
import eepromutil.onietlv as ot
from platform_config import PLATFORM_E2_CONF
from platform_util import byteTostr, dev_file_read

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


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
    @staticmethod
    def fru_decode_mac_number(params):
        ipmi_fru = params.get("fru")
        area = params.get("area")
        field = params.get("field")
        area_info = getattr(ipmi_fru, area, None)
        if area_info is not None:
            raw_mac_number = getattr(area_info, field, None)
            mac_number = decode_mac_number(raw_mac_number)
            ipmi_fru.setValue(area, field, mac_number)

    @staticmethod
    def fru_decode_mac(params):
        ipmi_fru = params.get("fru")
        area = params.get("area")
        field = params.get("field")
        area_info = getattr(ipmi_fru, area, None)
        if area_info is not None:
            raw_mac = getattr(area_info, field, None)
            decoded_mac = decode_mac(raw_mac)
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


def onie_eeprom_show(eeprom, e2_decode=None):
    try:
        onietlv = ot.onie_tlv()
        rets = onietlv.decode(eeprom)
        if e2_decode is not None:
            onie_eeprom_decode(rets, e2_decode)
        print("%-20s %-5s %-5s  %-20s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            if item["code"] == 0xfd:
                print("%-20s 0x%-02X   %-5s" % (item["name"], item["code"], item["lens"]))
            else:
                print("%-20s 0x%-02X   %-5s %-20s" % (item["name"], item["code"], item["lens"], item["value"]))
    except Exception as e:
        print(str(e))


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


def fantlv_eeprom_show(eeprom, e2_decode=None):
    try:
        tlv = fan_tlv()
        rets = tlv.decode(eeprom)
        if len(rets) == 0:
            print("fan tlv eeprom info error.!")
            return
        if e2_decode is not None:
            fantlv_eeprom_decode(rets, e2_decode)
        print("%-15s %-5s %-5s  %-20s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            print("%-15s 0x%-02X   %-5s %-20s" % (item["name"], item["code"], item["lens"], item["value"]))
    except Exception as e:
        print(str(e))
    return


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


def fru_eeprom_show(eeprom, e2_decode=None):
    try:
        ipmi_fru = ipmifru()
        ipmi_fru.decodeBin(eeprom)
        if e2_decode is not None:
            fru_eeprom_decode(ipmi_fru, e2_decode)
        print("=================board=================")
        print(ipmi_fru.boardInfoArea)
        print("=================product=================")
        print(ipmi_fru.productInfoArea)
    except Exception as e:
        print(str(e))


def eeprom_parase(eeprom_conf):
    name = eeprom_conf.get("name")
    e2_type = eeprom_conf.get("e2_type")
    e2_path = eeprom_conf.get("e2_path")
    e2_size = eeprom_conf.get("e2_size", 256)
    e2_decode = eeprom_conf.get("e2_decode")
    print("===================%s===================" % name)
    ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
    if ret is False:
        print("eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval_bytes))
        return
    binval = byteTostr(binval_bytes)
    if e2_type == "onie_tlv":
        onie_eeprom_show(binval, e2_decode)
    elif e2_type == "fru":
        fru_eeprom_show(binval, e2_decode)
    elif e2_type == "fantlv":
        fantlv_eeprom_show(binval, e2_decode)
    else:
        print("Unknow eeprom type: %s" % e2_type)
    return


def get_fans_eeprom_info(param):
    fan_eeprom_conf = PLATFORM_E2_CONF.get("fan", [])
    fan_num = len(fan_eeprom_conf)
    if fan_num == 0:
        print("fan number is 0, can't get fan eeprom info")
        return
    if param == 'all':
        for conf in fan_eeprom_conf:
            eeprom_parase(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    fan_index = int(param, 10) - 1
    if fan_index < 0 or fan_index >= fan_num:
        print("param error, total fan number: %d, fan index: %d" % (fan_num, fan_index + 1))
        return
    eeprom_parase(fan_eeprom_conf[fan_index])
    return


def get_psus_eeprom_info(param):
    psu_eeprom_conf = PLATFORM_E2_CONF.get("psu", [])
    psu_num = len(psu_eeprom_conf)
    if psu_num == 0:
        print("psu number is 0, can't get psu eeprom info")
        return
    if param == 'all':
        for conf in psu_eeprom_conf:
            eeprom_parase(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    psu_index = int(param, 10) - 1
    if psu_index < 0 or psu_index >= psu_num:
        print("param error, total psu number: %d, psu index: %d" % (psu_num, psu_index + 1))
        return
    eeprom_parase(psu_eeprom_conf[psu_index])
    return


def get_slots_eeprom_info(param):
    slot_eeprom_conf = PLATFORM_E2_CONF.get("slot", [])
    slot_num = len(slot_eeprom_conf)
    if slot_num == 0:
        print("slot number is 0, can't get slot eeprom info")
        return
    if param == 'all':
        for conf in slot_eeprom_conf:
            eeprom_parase(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    slot_index = int(param, 10) - 1
    if slot_index < 0 or slot_index >= slot_num:
        print("param error, total slot number: %d, slot index: %d" % (slot_num, slot_index + 1))
        return
    eeprom_parase(slot_eeprom_conf[slot_index])
    return


def get_syseeprom_info(param):
    syseeprom_conf = PLATFORM_E2_CONF.get("syseeprom", [])
    syseeprom_num = len(syseeprom_conf)
    if syseeprom_num == 0:
        print("syseeprom number is 0, can't get syseeprom info")
        return
    if param == 'all':
        for conf in syseeprom_conf:
            eeprom_parase(conf)
        return
    if not param.isdigit():
        print("param error, %s is not digital or 'all'" % param)
        return
    syseeprom_index = int(param, 10) - 1
    if syseeprom_index < 0 or syseeprom_index >= syseeprom_num:
        print("param error, total syseeprom number: %d, syseeprom index: %d" % (syseeprom_num, syseeprom_index + 1))
        return
    eeprom_parase(syseeprom_conf[syseeprom_index])
    return


def decode_eeprom_info(e2_type, e2_path, e2_size):
    if not e2_size.isdigit():
        print("param error, e2_size %s is not digital" % e2_size)
        return
    e2_size = int(e2_size, 10)
    eeprom_conf = {}
    eeprom_conf["name"] = e2_type
    eeprom_conf["e2_type"] = e2_type
    eeprom_conf["e2_path"] = e2_path
    eeprom_conf["e2_size"] = e2_size
    eeprom_parase(eeprom_conf)
    return


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''platform eeprom display script'''

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

# fru eeprom info decode


@main.command()
@click.argument('e2_path', required=True)
@click.argument('e2_size', required=False, default="256")
def fru(e2_path, e2_size):
    '''e2_path'''
    decode_eeprom_info("fru", e2_path, e2_size)

# fantlv eeprom info decode


@main.command()
@click.argument('e2_path', required=True)
@click.argument('e2_size', required=False, default="256")
def fantlv(e2_path, e2_size):
    '''e2_path'''
    decode_eeprom_info("fantlv", e2_path, e2_size)

# onie_tlv eeprom info decode


@main.command()
@click.argument('e2_path', required=True)
@click.argument('e2_size', required=False, default="256")
def onie_tlv(e2_path, e2_size):
    '''e2_path'''
    decode_eeprom_info("onie_tlv", e2_path, e2_size)


if __name__ == '__main__':
    main()
