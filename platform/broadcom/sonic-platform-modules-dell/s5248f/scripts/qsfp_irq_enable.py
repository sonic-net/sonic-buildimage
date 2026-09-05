#!/usr/bin/python

try:
        import struct
        import sys
        from os import *
        from mmap import *

except ImportError as e:
        raise ImportError("%s - required module no found" % str(e))

BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"
PORT_START = 0
PORT_END = 32 


def pci_mem_write(mm, offset, data):
        mm.seek(offset)
        mm.write(struct.pack('I', data))


def pci_set_value(resource, val, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        val = pci_mem_write(mm, offset, val)
        mm.close()
        close(fd)
        return val

for port_num in range(PORT_START, PORT_END+1):
        port_offset = 0x400c + ((port_num) * 16)
        pci_set_value(BASE_RES_PATH, 0x30, port_offset)
