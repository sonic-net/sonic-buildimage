#!/usr/bin/python
import json
import sys
import struct
import glob
from ctypes import *
import os
# from sonic_py_common import device_info
# import sonic_platform
#
CACHE_ROOT = '/var/cache/sonic/decode-syseeprom'
CACHE_FILE = 'syseeprom_cache'

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


class TLVINFO_HEADER(Structure):
    _fields_ = [("signature", c_char * 8),
                ("version", c_ubyte),
                ("totallen", c_ushort)]

    def dump(self):
        return struct.pack('8s', self.signature) + \
            struct.pack('B', self.version) + \
            struct.pack('>H', self.totallen)


class TLVINFO_DATA:
    data = []

    def add_tlv_str(self, types, value):
        self.data.append(struct.pack('B', types) + struct.pack('B', len(value)) + value.encode())

    def add_tlv_mac(self, types, value):
        self.data.append(struct.pack('B', types) + struct.pack('B', len(value)))
        for v in value:
            self.data.append(struct.pack('B', int(v, 16)))

    def add_tlv_val(self, types, value):
        self.data.append(struct.pack('B', types) + struct.pack('B', 1))
        self.data.append(struct.pack('B', int(value, 16)))

    def dump(self):
        r = b''
        for m in self.data:
            r += m
        return r + struct.pack('B', TLV_CODE_CRC_32) + struct.pack('B', 4)


def crc32(crc, p, len):
    crc = 0xffffffff & ~crc
    for i in range(len):
        crc = crc ^ p[i]
        for j in range(8):
            crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1))
    return 0xffffffff & ~crc


def crc(header, data):
    r = b''
    for m in header:
        r += bytes(m)
    for m in data:
        r += bytes(m)
    c = crc32(0, bytearray(r), len(r))
    s = struct.pack('>I', c)
    return s


def usage():
    usage_str = '''
    write tlv eeprom to eeprom

    Usage:
        syseeprom-write.py <JSON_Path>

    e.g.:
        syseeprom-write.py ../config/common/syseeprom_test.json
    '''
    print(usage_str)

def main():

    if not sys.argv:
        usage()
        exit()

    path = sys.argv[1]

    current_dir = os.path.dirname(os.path.abspath(__file__))
    syseeprom_config = os.path.join(current_dir, path)
    with open(syseeprom_config, 'r') as f:
        syseeprom_test = json.load(f)

    tlvinfo_header = TLVINFO_HEADER(b'TlvInfo', 1, 0)
    tlvinfo_data = TLVINFO_DATA()
    #onie_machine = os.popen("cat /host/machine.conf | grep 'onie_machine=' | sed 's/onie_machine=//'").read().strip()
    #tlvinfo_data.add_tlv_str(TLV_CODE_ONIE_MACHINE, onie_machine)

    eth0_mac_str = syseeprom_test["Base MAC Address"]
    eth0_mac = eth0_mac_str.split(':')
    tlvinfo_data.add_tlv_mac(TLV_CODE_MAC_BASE, eth0_mac)
    tlvinfo_data.add_tlv_val(TLV_CODE_DEVICE_VERSION, syseeprom_test["Device Version"])

    tlvinfo_data.add_tlv_str(TLV_CODE_ONIE_VERSION, syseeprom_test["ONIE Version"])
    tlvinfo_data.add_tlv_str(TLV_CODE_PLATFORM_NAME, syseeprom_test["Platform Name"])
    tlvinfo_data.add_tlv_str(TLV_CODE_SERIAL_NUMBER, syseeprom_test["Serial Number"])
    tlvinfo_data.add_tlv_str(TLV_CODE_PRODUCT_NAME, syseeprom_test["Product Name"])
    tlvinfo_data.add_tlv_str(TLV_CODE_PART_NUMBER, syseeprom_test["Part Number"])
    tlvinfo_data.add_tlv_str(TLV_CODE_MANUF_NAME, syseeprom_test["Manufacturer"])
    tlvinfo_data.add_tlv_str(TLV_CODE_MANUF_DATE, syseeprom_test["Manufacture Date"])
    tlvinfo_data.add_tlv_str(TLV_CODE_MANUF_COUNTRY, syseeprom_test["Manufacture Country"])
    tlvinfo_data.add_tlv_str(TLV_CODE_VENDOR_NAME, syseeprom_test["Vendor Name"])
    if "Diag Version" in syseeprom_test:
        tlvinfo_data.add_tlv_str(TLV_CODE_DIAG_VERSION, syseeprom_test["Diag Version"])
    if "Service Tag" in syseeprom_test:
        tlvinfo_data.add_tlv_str(TLV_CODE_SERVICE_TAG, syseeprom_test["Service Tag"])
    tlvinfo_data.add_tlv_str(TLV_CODE_LABEL_REVISION, syseeprom_test["Label Revision"])
    tlv_mac_size = []
    tlv_mac_size.append("00")
    mac_size = syseeprom_test["MAC Addresses"]
    tlv_mac_size.append(str(hex(int(mac_size))))
    tlvinfo_data.add_tlv_mac(TLV_CODE_MAC_SIZE, tlv_mac_size)
    #tlvinfo_data.add_tlv_str(TLV_CODE_VENDOR_EXT, syseeprom_test["Vendor Extension"])

    tlvinfo_header.totallen = len(tlvinfo_data.dump()) + 4

    try:
        f = open('sys_eeprom.bin', 'wb+')
        f.write(tlvinfo_header.dump())
        f.write(tlvinfo_data.dump())
        f.write(crc(tlvinfo_header.dump(), tlvinfo_data.dump()))
        f.close()

        os.system("sudo cat sys_eeprom.bin > /sys_switch/syseeprom")
        # os.system("decode-syseeprom --init")
        # init_syseeprom()
        os.system("sudo rm -f /var/cache/sonic/decode-syseeprom/syseeprom_cache")
    except BaseException:
        print('Unable to write file ./sys_eeprom.bin')


if __name__ == "__main__":
    main()
