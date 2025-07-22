#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import binascii
import struct
import sys
import os
import copy
import argparse
import json
import re
import collections
from datetime import datetime, timedelta
import shutil
import zlib

from platform_config import PRODUCT_INFO_CONF


VERSION = 'Version: 1.0.1, Date: 20241226'

PRODUCT_INFO_MAX_LEN = 8 * 1024
CRC_LEN = 4


SN = "SerialNO"
MAC_ADDR = "ethaddr"
HW_VERSION = "HardwareVersion"
ETHADDR_SCALE = "ethaddr_scale"
SETMAC_TIME = "setmac_time"


def parse_args():
    parse = argparse.ArgumentParser(description='product_info tool')
    parse.add_argument('-v', '--version', action='version', version=VERSION, help='Display the product_info.py tool version')
    parse.add_argument('-c', '--creat', metavar='', help='Creat product_info bin')
    parse.add_argument('-p', '--parse', metavar='', help='Parse product_info bin')
    parse.add_argument('-f', '--file',  metavar='', help='product_info bin')
    parse.add_argument('-sn', '--serial',  metavar='', help='set serial to product_info bin')
    parse.add_argument('-mac', '--macaddr',  metavar='', help='set macaddr to product_info bin')
    parse.add_argument('-hw', '--hwversion',  metavar='', help='set hwversion to product_info bin')
    parse.add_argument('-es', '--ethscale',  metavar='', help='set ethaddr_scale to product_info bin')
    parse.add_argument('-st', '--setmactime',  metavar='', help='set setmactime to product_info bin')
    return parse


def product_info_parse(path):
    msg = ""
    fd = -1
    info_part_list = []
    product_info_dict = {}

    if not os.path.exists(path):
        msg = path + " not found !"
        print(msg)
        return False, msg

    try:
        size = PRODUCT_INFO_MAX_LEN
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, 0, os.SEEK_SET)

        binval_byte = os.read(fd, size)
        crc = binval_byte[:4]
        infodata = binval_byte[4:]

        # product_info use byte '\0' to split
        parts = infodata.split(b'\0')
        for part in parts:
            val_str = ""
            if part == b'':
                break
            for item in part:
                val_str += chr(item)
            info_part_list.append(val_str)

        for part_item in info_part_list:
            parts = part_item.split('=')
            key = parts[0]
            value = parts[1]
            product_info_dict[key] = value

    except Exception as e:
        msg = str(e)
        print(msg)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    return True, product_info_dict


def product_info_crc32(data_array):
    return '0x%08x' % (binascii.crc32(bytes(data_array)) & 0xffffffff)


def generate_product_info_bin(product_info_dict, file):
    product_info_list = []
    result = ""
    try:
        for key, value in product_info_dict.items():
            pair = f"{key}={value}"
            product_info_list.append(pair)

        result = b'\0'.join([s.encode('utf - 8') for s in product_info_list])
        while len(result) < PRODUCT_INFO_MAX_LEN - CRC_LEN:
            result += b'\0'

        crc32_value = product_info_crc32(result)
        crc32_byte = struct.pack('<I', int(crc32_value, 16))

        product_info_byte = crc32_byte + result
        with open(file, 'wb') as f:
            f.write(product_info_byte)

    except Exception as e:
        msg = str(e)
        print(msg)
        return False, msg


def product_info_creat_bin(file):
    product_info_dict = copy.deepcopy(PRODUCT_INFO_CONF)
    generate_product_info_bin(product_info_dict, file)


def product_info_set_sn(file, sn):
    ret, product_info_dict = product_info_parse(file)
    if ret is False:
        print(product_info_dict)
        return

    product_info_dict[SN] = sn
    generate_product_info_bin(product_info_dict, file)


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


def check_mac_addr(mac):
    if mac.startswith("0x"):
        status, mac = mac_addr_decode(mac)
        if status is False:
            return False, mac

    if isValidMac(mac) is False:
        msg = "Invalid MAC address: %s" % mac
        return False, msg
    return True, mac


def product_info_set_mac_addr(file, mac_addr):
    ret, product_info_dict = product_info_parse(file)
    if ret is False:
        print(product_info_dict)
        return
    ret, mac = check_mac_addr(mac_addr)
    if ret is False:
        print("mac_addr %s check failed, errmsg: %s" % (mac_addr, mac))
        return

    product_info_dict[MAC_ADDR] = mac
    generate_product_info_bin(product_info_dict, file)


def product_info_set_hwversion(file, hw_version):
    ret, product_info_dict = product_info_parse(file)
    if ret is False:
        print(product_info_dict)
        return
    product_info_dict[HW_VERSION] = hw_version
    generate_product_info_bin(product_info_dict, file)


def product_info_set_ethscale(file, ethscale):
    ret, product_info_dict = product_info_parse(file)
    if ret is False:
        print(product_info_dict)
        return
    product_info_dict[ETHADDR_SCALE] = ethscale
    generate_product_info_bin(product_info_dict, file)


def product_info_set_setmactime(file, setmactime):
    ret, product_info_dict = product_info_parse(file)
    if ret is False:
        print(product_info_dict)
        return
    product_info_dict[SETMAC_TIME] = setmactime
    generate_product_info_bin(product_info_dict, file)


def generate_product_info_rawdata(parse):
    try:
        args = parse.parse_args()
        if args.parse is not None:
            ret, product_info_dict = product_info_parse(args.parse)
            if ret is True:
                for key, value in product_info_dict.items():
                    print(key + ': ' + value)
            else:
                print("parse file:%s failed, %s" % (args.parse, product_info_dict))
            sys.exit(0)

        if args.creat is not None:
            product_info_creat_bin(args.creat)
            sys.exit(0)

        if args.serial is not None and args.file is not None:
            product_info_set_sn(args.file, args.serial)

        if args.macaddr is not None and args.file is not None:
            product_info_set_mac_addr(args.file, args.macaddr)

        if args.hwversion is not None and args.file is not None:
            product_info_set_hwversion(args.file, args.hwversion)

        if args.ethscale is not None and args.file is not None:
            product_info_set_ethscale(args.file, args.ethscale)

        if args.setmactime is not None and args.file is not None:
            product_info_set_setmactime(args.file, args.setmactime)

        if args.parse is None and args.creat is None and args.file is None:
            parse.print_help()
        sys.exit(0)
    except Exception as e:
        msg = str(e)
        print(msg)
        sys.exit(1)


if __name__ == '__main__':
    parse = parse_args()
    generate_product_info_rawdata(parse)
