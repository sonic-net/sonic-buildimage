#!/usr/bin/python

# Since the XLR/GTS cards do not have an EEPROM, we create a file which
# will be use like an EEPROM.

import sys
import struct
from ctypes import *
import os
from sonic_py_common.general import getstatusoutput_noshell_pipe

TLV_CODE_PRODUCT_NAME  = 0x21
TLV_CODE_SERIAL_NUMBER = 0x23
TLV_CODE_MAC_BASE      = 0x24
TLV_CODE_PLATFORM_NAME = 0x28
TLV_CODE_ONIE_VERSION  = 0x29
TLV_CODE_MANUF_NAME    = 0x2B
TLV_CODE_CRC_32        = 0xFE

def getmac(interface):
    try:
        mac = open('/sys/class/net/'+interface+'/address').readline()
    except:
        mac = "00:00:00:00:00:00"
    return mac[0:17]
  
class TLVINFO_HEADER(Structure):
    _fields_ = [("signature", c_char*8),
                ("version",   c_ubyte),
                ("totallen",  c_ushort)]
    def dump(self):
        return struct.pack('8s', self.signature) + \
               struct.pack('B', self.version) + \
               struct.pack('>H', self.totallen)

class TLVINFO_DATA:
    data = [];
    def add_tlv_str(self, type, value):
		self.data.append(struct.pack('B', type) + struct.pack('B', len(value)) + value.encode())
    def add_tlv_mac(self, type, value):
		self.data.append(struct.pack('B', type) + struct.pack('B', len(value)))
		for v in value:
		    self.data.append(struct.pack('B', int(v, 16)))
    def dump(self):
		r = '';
		for m in self.data:
			r += bytes(m)
		return r + struct.pack('B', TLV_CODE_CRC_32) + struct.pack('B', 4)

def crc32(crc, p, len):
	crc = 0xffffffff & ~crc
	for i in range(len):
		crc = crc ^ p[i]
		for j in range(8):
			crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1))
	return 0xffffffff & ~crc
  
def crc(header, data):
	r = '';
	for m in header:
		r += bytes(m)
	for m in data:
		r += bytes(m)
	c = crc32(0, bytearray(r), len(r))
	s = struct.pack('>I', c)
	return s
   
def main():

    tlvinfo_header = TLVINFO_HEADER('TlvInfo', 1, 0)

    tlvinfo_data = TLVINFO_DATA()
    tlvinfo_data.add_tlv_str(TLV_CODE_SERIAL_NUMBER, 'S/N')

    _, onie_machine = getstatusoutput_noshell_pipe(["cat", "/host/machine.conf"], ["grep", 'onie_machine='], ["sed", 's/onie_machine=//'])
    if onie_machine == 'bcm_xlr':
        tlvinfo_data.add_tlv_str(TLV_CODE_PRODUCT_NAME,  'BCM9COMX2XMC')
    else:
        tlvinfo_data.add_tlv_str(TLV_CODE_PRODUCT_NAME,  'Unknown')

    tlvinfo_data.add_tlv_str(TLV_CODE_MANUF_NAME,    'Broadcom')
    
    eth0_mac_str = getmac('eth0')
    eth0_mac = eth0_mac_str.split(':')
    tlvinfo_data.add_tlv_mac(TLV_CODE_MAC_BASE, eth0_mac)

    _, brcm_dev = getstatusoutput_noshell_pipe(["lspci"], ["grep", "-m1", 'Ethernet controller: Broadcom '], ["grep", 'Device'], ["sed", 's/(.*//'], ["awk", '{print $NF}'])
    if brcm_dev == 'b960':
        tlvinfo_data.add_tlv_str(TLV_CODE_PLATFORM_NAME, 'BCM956960K')

    onie_version = getstatusoutput_noshell_pipe(["cat", "/host/machine.conf"], ["grep", 'onie_version'], ["sed", 's/onie_version=//'])
    tlvinfo_data.add_tlv_str(TLV_CODE_ONIE_VERSION, onie_version)

    tlvinfo_header.totallen = len(tlvinfo_data.dump())+4;

    try:
        f = open('/usr/share/sonic/device/x86_64-bcm_xlr-r0/sys_eeprom.bin', 'w+')
        f.write(tlvinfo_header.dump())
        f.write(tlvinfo_data.dump())
        f.write(crc(tlvinfo_header.dump(), tlvinfo_data.dump()))
        f.close()
    except:
        print('Unable to write file /usr/share/sonic/device/x86_64-bcm_xlr-r0/sys_eeprom.bin')

if __name__== "__main__":
    main()
