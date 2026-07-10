#
# Copyright (c) 2020-2021 NVIDIA CORPORATION & AFFILIATES.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan Drawer status which are available in the platform
#
#############################################################################

import os

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
    from .led import FanLed, SharedLed
    from . import utils
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# Global logger class instance
logger = Logger()


class MellanoxFanDrawer(FanDrawerBase):
    def __init__(self, index):
        from .fan import FAN_PATH
        super(MellanoxFanDrawer, self).__init__()
        self._index = index + 1
        self._presence_path = os.path.join(FAN_PATH, 'fan{}_status'.format(self._index))
        self._led = None

    def get_index(self):
        return self._index

    def get_led(self):
        return self._led

    def get_presence(self):
        return utils.read_int_from_file(self._presence_path) == 1

    def get_direction(self):
        if not self.get_presence():
            return FanBase.FAN_DIRECTION_NOT_APPLICABLE
        
        from .fan import FAN_DIR, MlnxFan
        return MlnxFan.get_fan_direction(FAN_DIR.format(self._index))

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED

        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return True

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return self._led.get_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self._index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True


class RealDrawer(MellanoxFanDrawer):
    def __init__(self, index):
        super(RealDrawer, self).__init__(index)
        self._name = 'drawer{}'.format(self._index)
        self._led = SharedLed(FanLed(self._index))

    def get_name(self):
        return self._name

    
class VirtualDrawer(MellanoxFanDrawer):
    def __init__(self, index):
        super(VirtualDrawer, self).__init__(index)
        self._led = SharedLed(FanLed(None))

    def get_name(self):
        return 'N/A'

    def get_presence(self):
        return True

    def is_replaceable(self):
        return False
