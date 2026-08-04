#!/usr/bin/env python

from __future__ import print_function

import imp
import os

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError("%s - required module not found" % e)

USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

class Psu(PsuBase):
    """Centec Platform-specific PSU class"""

    def __init__(self, index):
        self._index = index
        self._fan_list = []
        
        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                return
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        module_file = "/".join([platform_path, "plugins", "psuutil.py"])
        module = imp.load_source("psuutil", module_file)
        psu_util_class = getattr(module, "PsuUtil")
        self._psuutil = psu_util_class()

    def _get_psuutil(self):
        return self._psuutil

    def get_presence(self):
        return self._get_psuutil().get_psu_presence(self._index)

    def get_powergood_status(self):
        return self._get_psuutil().get_psu_status(self._index)
