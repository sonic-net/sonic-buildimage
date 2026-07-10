#!/usr/bin/env python

import os
import sys
import time

susi4_lib = '/usr/local/lib/python3.9/dist-packages/'
if not susi4_lib in os.environ.setdefault('LD_LIBRARY_PATH', ''):
    os.environ['LD_LIBRARY_PATH'] += (':' + susi4_lib)
    try:
            os.execv(sys.argv[0], sys.argv)
    except Exception as e:
            sys.exit('failed to execute under modified environment!')

from _Susi4 import *

def release_board():
    SusiLibInitialize()

    SusiI2CWriteTransfer(0, 0x36 * 2, 0x0e, '\x00')
    SusiI2CWriteTransfer(0, 0x36 * 2, 0x0f, '\x00')
    SusiI2CWriteTransfer(0, 0x36 * 2, 0x10, '\x00')
    SusiI2CWriteTransfer(0, 0x37 * 2, 0x0e, '\x00')
    SusiI2CWriteTransfer(0, 0x37 * 2, 0x0f, '\x00')
    SusiI2CWriteTransfer(0, 0x37 * 2, 0x10, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x04')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x32, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x04')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x33, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x04')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x34, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x04')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x35, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x08')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x32, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x08')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x33, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x08')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x34, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x08')
    SusiI2CWriteTransfer(0, 0x2c * 2, 0x35, '\x7f')
    SusiI2CWriteTransfer(0, 0x71 * 2, 0x00, '\x00')

    SusiI2CWriteTransfer(0, 0x37 * 2, 0x4, '\x00')
    time.sleep(1)
    SusiI2CWriteTransfer(0, 0x37 * 2, 0x4, '\x01')
    time.sleep(3)
    os.system('echo 1 > /sys/bus/pci/devices/0000\:00\:1c.0/remove')
    time.sleep(1)
    os.system('echo 1 > /sys/bus/pci/rescan')

    SusiLibUninitialize()

if __name__ == '__main__':
    release_board()
