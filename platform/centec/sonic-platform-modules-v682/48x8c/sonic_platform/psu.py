#!/usr/bin/env python

from __future__ import print_function

from subprocess import Popen, PIPE, STDOUT

try:
    from sonic_platform_base.psu_base import PsuBase
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

class Psu(PsuBase):
    """Centec Platform-specific PSU class"""

    def __init__(self, index):
        self._index = index
        self._fan_list = []

    def get_presence(self):
        cmd = 'i2cget -y 0 0x36 0x1e'
        status = int(Popen(cmd, stdout=PIPE, stderr=STDOUT, shell=True).stdout.readline(), 16)
        presence = ((status & (1 << (3 * (self._index - 1) + 1))) == 0)
        return presence

    def get_powergood_status(self):
        cmd = 'i2cget -y 0 0x36 0x1e'
        status = int(Popen(cmd, stdout=PIPE, stderr=STDOUT, shell=True).stdout.readline(), 16)
        powergood = ((status & (1 << (3 * (self._index - 1) + 2))) != 0)
        return powergood
