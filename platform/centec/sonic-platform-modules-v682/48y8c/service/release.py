#!/usr/bin/env python

import os
import time

def release_board():
    if open('/proc/cmdline', 'r').readlines()[0].find('SONIC_BOOT_TYPE=warm') != -1:
        return

    os.system('i2cset -y 0 0x37 0x4 0x0')
    time.sleep(1)
    os.system('i2cset -y 0 0x37 0x4 0x1')
    time.sleep(1)
    os.system('echo 1 > /sys/bus/pci/devices/0000\:00\:1c.0/remove')
    time.sleep(1)
    os.system('echo 1 > /sys/bus/pci/rescan')
    time.sleep(1)
    # EPLD_QSFP_RST
    os.system('i2cset -y 0 0x36 0x5 0xff')
    os.system('i2cset -y 0 0x37 0x5 0xff')
    # EPLD_QSFP_INT_MASK
    os.system('i2cset -y 0 0x36 0xd 0xff')
    os.system('i2cset -y 0 0x37 0xd 0xff')
    # EPLD_PPU_INT_MASK
    os.system('i2cset -y 0 0x36 0xb 0x00')
    # EPLD_SFP_DISABLE1
    os.system('i2cset -y 0 0x36 0xe 0x00')
    os.system('i2cset -y 0 0x37 0xe 0x00')
    # EPLD_SFP_DISABLE2
    os.system('i2cset -y 0 0x36 0xf 0x00')
    os.system('i2cset -y 0 0x37 0xf 0x00')
    # EPLD_SFP_DISABLE3
    os.system('i2cset -y 0 0x36 0x10 0x00')
    os.system('i2cset -y 0 0x37 0x10 0x00')

if __name__ == '__main__':
    release_board()
