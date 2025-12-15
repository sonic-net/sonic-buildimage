#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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
# provides the liquid cooling status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.liquid_cooling_base import LeakageSensorBase,LiquidCoolingBase
    from sonic_py_common.logger import Logger
    from . import utils
    import os
    import glob
    import logging
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

LIQUID_COOLING_SENSOR_PATH = "/var/run/hw-management/system/"

class LeakageSensor(LeakageSensorBase):
    def __init__(self, name,path):
        super(LeakageSensor, self).__init__(name)
        self.path = path

    def is_leak(self):
        content = utils.read_int_from_file(self.path, 'N/A')

        if content == 1:
            self.leaking = False
            return False
        elif content == 0:
            self.leaking = True
            return True
        else:
            logger.error(f"Failed to read leakage sensor {self.name} value: {content}")
            return 'N/A'

class LiquidCooling(LiquidCoolingBase):
    """Platform-specific Liquid Cooling class"""

    def __init__(self):
        
        self.leakage_sensors_num = utils.read_int_from_file("/var/run/hw-management/config/leakage_counter")
        self.leakage_sensors = []

        sensor_files = glob.glob(os.path.join(LIQUID_COOLING_SENSOR_PATH, "leakage*"))
        sensor_files.sort(key=lambda x: int(x.split("leakage")[-1]))

        for sensor_path in sensor_files[:self.leakage_sensors_num]:
            sensor_name = os.path.basename(sensor_path)
            self.leakage_sensors.append(LeakageSensor(sensor_name, sensor_path))

        super(LiquidCooling, self).__init__(self.leakage_sensors_num, self.leakage_sensors)

