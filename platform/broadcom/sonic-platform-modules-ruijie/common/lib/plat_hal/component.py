#!/usr/bin/env python3
#######################################################
#
# component.py
# Python implementation of the Class fan
#
#######################################################
from plat_hal.devicebase import devicebase
from plat_hal.osutil import osutil


class component(devicebase):
    __user_reg = None

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.version_file = conf.get('VersionFile', None)
            self.comp_id = conf.get("comp_id", None)
            self.desc = conf.get("desc", None)
            self.slot = conf.get("slot", None)

    def get_version(self):
        version = "NA"
        try:
            ret, version = self.get_value(self.version_file)
            if ret is False:
                return version
            pattern = self.version_file.get('pattern', None)
            version = osutil.std_match(version, pattern)
        except Exception:
            return version
        return version
