# Helper functions to access hardware

import os
import struct
import mmap
import subprocess

# Read PCI device

def pci_mem_read(mm, offset):
    mm.seek(offset)
    read_data_stream = mm.read(4)
    return struct.unpack('I',read_data_stream)[0]

def pci_get_value(resource, offset):
    with open(resource, 'r+b') as fd:
        mm = mmap.mmap(fd.fileno(), 0)
        val = pci_mem_read(mm, offset)
        mm.close()
    return val

def pci_mem_write(memmap, offset, data):
    """ Write PCI device """
    memmap.seek(offset)
    memmap.write(struct.pack('I', data))

def pci_set_value(resource, val, offset):
    """ Set a value to PCI device """
    with open(resource, 'w+b') as filed:
        memmap = None
        try:
            memmap = mmap.mmap(filed.fileno(), 0)
            pci_mem_write(memmap, offset, val)
        except EnvironmentError:
            pass
        if memmap is not None:
            memmap.close()

# Read I2C device

def i2c_get(bus, i2caddr, ofs):
    try:
        valx = int(subprocess.check_output(['/usr/sbin/i2cget','-f', '-y', str(bus), str(i2caddr), str(ofs)]), 16)
        return "{:02x}".format(valx)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return -1

