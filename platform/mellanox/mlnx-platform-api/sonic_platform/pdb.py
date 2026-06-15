#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
# provides the PDBs status which are available in the platform
#
#############################################################################

import os
from sonic_platform_base.pdb_base import PdbBase
from sonic_py_common.logger import Logger
from . import utils
from .thermal import Thermal

logger = Logger()
PDB_PATH = '/var/run/hw-management'
LED_POWER_STATE_FILE = os.path.join(PDB_PATH, 'led', 'led_power')
DEFAULT_TEMP_SCALE = 1000
LED_DISPLAY_NA = 'N/A'


class Pdb(PdbBase):
    """Mellanox PDB; sysfs under /var/run/hw-management (power, thermal, environment)."""

    def __init__(self, pdb_index):
        super(Pdb, self).__init__()
        self.index = pdb_index + 1
        self._name = "PDB {}".format(self.index)

        # Status: system/pdb{N}_pwr_status
        self._pwr_status = os.path.join(PDB_PATH, 'system', 'pdb{}_pwr_status'.format(self.index))

        # Temperature: thermal/pdb_hotswap{N}_temp1_input, _temp1_max, _temp1_crit
        self._thermal_list = []
        temp_path = os.path.join(PDB_PATH, 'thermal', 'pdb_hotswap{}_temp1_input'.format(self.index))
        high_path = os.path.join(PDB_PATH, 'thermal', 'pdb_hotswap{}_temp1_max'.format(self.index))
        crit_path = os.path.join(PDB_PATH, 'thermal', 'pdb_hotswap{}_temp1_crit'.format(self.index))
        if os.path.exists(temp_path):
            self._thermal_list.append(Thermal(
                'PDB-{} Temp'.format(self.index), temp_path,
                high_path if os.path.exists(high_path) else None,
                crit_path if os.path.exists(crit_path) else None,
                None, None, DEFAULT_TEMP_SCALE, 1))
        else:
            logger.log_error(f"PDB {self.index} temperature file {temp_path} does not exist")

        # Environment: pdb_hotswap{N}_* for in/out voltage, curr1, power1, power1_max; pwr_conv{N}_* for curr2, power2
        env_dir = os.path.join(PDB_PATH, 'environment')
        self._in_voltage = os.path.join(env_dir, 'pdb_pwr_conv{}_in1_input'.format(self.index))
        self._in_current = os.path.join(env_dir, 'pdb_pwr_conv{}_curr1_input'.format(self.index))
        self._in_power = os.path.join(env_dir, 'pdb_pwr_conv{}_power1_input'.format(self.index))
        self._power_max = os.path.join(env_dir, 'pdb_hotswap{}_power1_max'.format(self.index))
        self._out_voltage = os.path.join(env_dir, 'pdb_pwr_conv{}_in2_input'.format(self.index))
        self._out_current = os.path.join(env_dir, 'pdb_pwr_conv{}_curr2_input'.format(self.index))
        self._out_power = os.path.join(env_dir, 'pdb_pwr_conv{}_power2_input'.format(self.index))

    def get_name(self):
        return self._name

    def get_presence(self):
        return True

    def get_status(self):
        if not self.get_presence():
            return False
        return utils.read_int_from_file(self._pwr_status, log_func=None) == 1

    def get_powergood_status(self):
        return self.get_status()

    def get_model(self):
        return 'N/A'

    def get_serial(self):
        return 'N/A'

    def get_revision(self):
        return 'N/A'

    def is_replaceable(self):
        return False

    def get_status_led(self):
        """
        Power LED aggregate state from hw-management (matches ``cat .../led/led_power``).
        Returns ``N/A`` when the file is missing/unreadable or content is empty/none.
        """
        text = utils.read_str_from_file(LED_POWER_STATE_FILE, default='', log_func=None).strip()
        if not text or text.lower() == 'none':
            return LED_DISPLAY_NA
        return text

    def get_temperature(self):
        if self._thermal_list:
            return self._thermal_list[0].get_temperature()
        return None

    def get_output_current(self):
        if os.path.exists(self._out_current):
            val = utils.read_int_from_file(self._out_current, log_func=logger.log_info)
            return float(val) / 1000 if val is not None else None
        return None

    def get_output_power(self):
        if os.path.exists(self._out_power):
            val = utils.read_int_from_file(self._out_power, log_func=logger.log_info)
            return float(val) / 1000000 if val is not None else None
        return None

    def get_output_voltage(self):
        if os.path.exists(self._out_voltage):
            val = utils.read_int_from_file(self._out_voltage, log_func=logger.log_info)
            return float(val) / 1000 if val is not None else None
        return None

    def get_input_current(self):
        if os.path.exists(self._in_current):
            val = utils.read_int_from_file(self._in_current, log_func=logger.log_info)
            return float(val) / 1000 if val is not None else None
        return None

    def get_input_power(self):
        if os.path.exists(self._in_power):
            val = utils.read_int_from_file(self._in_power, log_func=logger.log_info)
            return float(val) / 1000000 if val is not None else None
        return None

    def get_input_voltage(self):
        if os.path.exists(self._in_voltage):
            val = utils.read_int_from_file(self._in_voltage, log_func=logger.log_info)
            return float(val) / 1000 if val is not None else None
        return None

    def get_maximum_supplied_power(self):
        if os.path.exists(self._power_max):
            val = utils.read_int_from_file(self._power_max, log_func=logger.log_info)
            return float(val) / 1000000 if val is not None else None
        return None
