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
        return int(subprocess.check_output(['/usr/sbin/i2cget', '-y', str(bus), str(i2caddr), str(ofs)]), 16)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return -1

def io_reg_read(io_resource, offset):
    fd = os.open(io_resource, os.O_RDONLY)
    if fd < 0:
        print('file open failed %s' % io_resource)
        return -1
    if os.lseek(fd, offset, os.SEEK_SET) != offset:
        print('lseek failed on %s' % io_resource)
        return -1
    buf = os.read(fd, 1)
    reg_val1 = ord(buf)
    os.close(fd)
    return reg_val1

def io_reg_write(io_resource, offset, val):
    fd = os.open(io_resource, os.O_RDWR)
    if fd < 0:
        print('file open failed %s' % io_resource)
        return False
    if os.lseek(fd, offset, os.SEEK_SET) != offset:
        print('lseek failed on %s' % io_resource)
        return False
    ret = os.write(fd, struct.pack('B', val))
    if ret != 1:
        print('write failed %d' % ret)
        return False
    os.close(fd)
    return True
