#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
