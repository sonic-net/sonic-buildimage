#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import glob
import os
import json
from enum import Enum
from sonic_py_common import device_info

from . import utils


class SfpData:

    def __init__(self, name, thermals=None):
        self.name = name
        self.thermals = thermals


class DeviceDataManager:

    def __init__(self):
        self._platform = device_info.get_platform()
        self._platform_dir = device_info.get_path_to_platform_dir()

        platform_json = os.path.join(self._platform_dir, device_info.PLATFORM_JSON_FILE)
        if not os.path.isfile(platform_json):
            raise RuntimeError("{} file does not exists".format(platform_json))

        platform_data = json.loads(open(platform_json).read())
        self._chassis = platform_data.get('chassis')
        if not self._chassis:
            raise RuntimeError("Chassis data is not found in {} file".format(device_info.PLATFORM_JSON_FILE))

        self._interfaces = platform_data.get('interfaces')
        if not self._interfaces:
            raise RuntimeError("Interfaces data is not found in {} file".format(device_info.PLATFORM_JSON_FILE))

    def get_platform_name(self):
        return self._chassis['name']

    def get_fan_drawer_count(self):
        return len(self._chassis['fan_drawers'])

    def get_fan_count(self):
        return len(self._chassis['fans'])

    def get_psu_count(self):
        return len(self._chassis['psus'])

    def get_sfp_count(self):
        return len(self._chassis['sfps'])

    def get_sfp_data(self, sfp_index):
        if sfp_index >= len(self._chassis['sfps']):
            raise RuntimeError("Wrong SFP index: {}".format(sfp_index))
        sfp_data = self._chassis['sfps'][sfp_index]
        return SfpData(**sfp_data)
