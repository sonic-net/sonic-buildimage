#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from .device_data import DeviceDataManager
from . import utils
from sonic_py_common import logger

import atexit
import functools
import re
import sys
import time
import os

sys.path.append('/run/hw-management/bin')

try:
    import hw_management_independent_mode_update
except ImportError:
    # Only mock if running under pytest (check if pytest is imported)
    if 'pytest' in sys.modules:
        from unittest import mock
        hw_management_independent_mode_update = mock.MagicMock()
        hw_management_independent_mode_update.module_data_set_module_counter = mock.MagicMock()
        hw_management_independent_mode_update.thermal_data_set_asic = mock.MagicMock()
        hw_management_independent_mode_update.thermal_data_set_module = mock.MagicMock()
        hw_management_independent_mode_update.thermal_data_clean_asic = mock.MagicMock()
        hw_management_independent_mode_update.thermal_data_clean_module = mock.MagicMock()
    else:
        raise


SFP_TEMPERATURE_SCALE = 1000
ASIC_TEMPERATURE_SCALE = 125
ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD = 105000
ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD = 120000

ERROR_READ_THERMAL_DATA = 254000

TC_CONFIG_FILE = '/run/hw-management/config/tc_config.json'
logger = logger.Logger('thermal-updater')

# Register a clean-up routine that will run when the process exits
def clean_thermal_data(sfp_list):
    if not sfp_list:
        return
    hw_management_independent_mode_update.module_data_set_module_counter(len(sfp_list))
    for sfp in sfp_list:
        try:
            sw_control = sfp.is_sw_control()
            if not sw_control:
                continue

            hw_management_independent_mode_update.thermal_data_clean_module(
                0,
                sfp.sdk_index + 1
            )
        except Exception as e:
            logger.log_warning(f'Cleanup skipped for module {sfp.sdk_index + 1}: {e}')

class ThermalUpdater:
    def __init__(self, sfp_list, update_asic=True):
        self._sfp_list = sfp_list
        self._sfp_status = {}
        self._timer = utils.Timer()
        self._update_asic = update_asic

        atexit.register(functools.partial(clean_thermal_data, self._sfp_list))

    def wait_for_sysfs_nodes(self):
        """
        Wait for temperature sysfs nodes to be present before proceeding.
        Returns:
            bool: True if wait success else timeout
        """
        start_time = time.time()
        logger.log_notice('Waiting for temperature sysfs nodes to be present...')
        conditions = []

        # ASIC temperature sysfs node
        asic_count = DeviceDataManager.get_asic_count()
        for asic_index in range(asic_count):
            conditions.append(lambda idx=asic_index: os.path.exists(f'/sys/module/sx_core/asic{idx}/temperature/input'))

        # Module temperature sysfs nodes
        sfp_count = len(self._sfp_list) if self._sfp_list else 0
        result = DeviceDataManager.wait_sysfs_ready(sfp_count)
        end_time = time.time()
        elapsed_time = end_time - start_time

        if result:
            logger.log_notice(f'Temperature sysfs nodes are ready. Wait time: {elapsed_time:.4f} seconds')
        else:
            logger.log_error(f'Timeout waiting for temperature sysfs nodes. Wait time: {elapsed_time:.4f} seconds')

        return result

    def _find_matching_key(self, dev_parameters, pattern):
        """
        Find the first key in dev_parameters that matches the given regex pattern.
        Returns the matching key and its value, or (None, None) if no match found.
        """
        for key in dev_parameters.keys():
            if re.match(pattern, key):
                return key, dev_parameters[key]
        return None, None

    def load_tc_config(self):
        asic_poll_interval = 1
        sfp_poll_interval = 10
        data = utils.load_json_file(TC_CONFIG_FILE, log_func=None)
        if not data:
            logger.log_error(f'{TC_CONFIG_FILE} does not exist, use default polling interval')

        if data:
            dev_parameters = data.get('dev_parameters')
            if not dev_parameters:
                logger.log_error('dev_parameters not configured or empty, using default intervals')
            else:
                # Find ASIC parameter using regex pattern
                asic_key, asic_parameter = self._find_matching_key(dev_parameters, r'asic\\d*')
                if asic_parameter is not None:
                    asic_poll_interval_config = asic_parameter.get('poll_time')
                    if asic_poll_interval_config:
                        asic_poll_interval = int(asic_poll_interval_config)
                        logger.log_notice(f'ASIC parameter found with key "{asic_key}", poll_time: {asic_poll_interval_config}, interval: {asic_poll_interval}')
                    else:
                        logger.log_error(f'ASIC poll_time not configured in "{asic_key}", using default interval: {asic_poll_interval}')
                else:
                    logger.log_error(f'ASIC parameter not found (pattern: asic\\d*), using default interval: {asic_poll_interval}')
                # Find Module parameter using regex pattern
                module_key, module_parameter = self._find_matching_key(dev_parameters, r'module\\d+')
                if module_parameter is not None:
                    sfp_poll_interval_config = module_parameter.get('poll_time')
                    if sfp_poll_interval_config:
                        sfp_poll_interval = int(sfp_poll_interval_config)
                        logger.log_notice(f'Module parameter found with key "{module_key}", poll_time: {sfp_poll_interval_config}, interval: {sfp_poll_interval}')
                    else:
                        logger.log_error(f'Module poll_time not configured in "{module_key}", using default interval: {sfp_poll_interval}')
                else:
                    logger.log_error(f'Module parameter not found (pattern: module\\d+), using default interval: {sfp_poll_interval}')

        if self._update_asic:
            logger.log_notice(f'ASIC polling interval: {asic_poll_interval}')
            self._timer.schedule(asic_poll_interval, self.update_asic)
        logger.log_notice(f'Module polling interval: {sfp_poll_interval}')
        self._timer.schedule(sfp_poll_interval, self.update_module)

    def start(self):
        self.control_tc(False)
        self.load_tc_config()

        # Wait for temperature sysfs nodes to be ready before starting the timer
        if not self.wait_for_sysfs_nodes():
            logger.log_error('Failed to start thermal updater: temperature sysfs nodes not available')
            self.control_tc(True)  # Suspend TC to protect the system
            return False

        self._timer.start()

    def stop(self):
        self._timer.stop()
        self.control_tc(True)

    def control_tc(self, suspend):
        logger.log_notice(f'Set hw-management-tc to {"suspend" if suspend else "resume"}')
        utils.write_file('/run/hw-management/config/suspend', 1 if suspend else 0)

    def get_asic_temp(self, asic_index=0):
        temperature = utils.read_int_from_file(f'/sys/module/sx_core/asic{asic_index}/temperature/input', default=None)
        return temperature * ASIC_TEMPERATURE_SCALE if temperature is not None else None

    def get_asic_temp_warning_threshold(self, asic_index=0):
        emergency = utils.read_int_from_file(f'/sys/module/sx_core/asic{asic_index}/temperature/emergency', default=None, log_func=None)
        return emergency * ASIC_TEMPERATURE_SCALE if emergency is not None else ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD

    def get_asic_temp_critical_threshold(self, asic_index=0):
        critical = utils.read_int_from_file(f'/sys/module/sx_core/asic{asic_index}/temperature/critical', default=None, log_func=None)
        return critical * ASIC_TEMPERATURE_SCALE if  critical is not None else ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD

    def update_single_module(self, sfp):
        try:
            presence = sfp.get_presence()
            pre_presence = self._sfp_status.get(sfp.sdk_index)
            if presence:
                sw_control, temperature, warning_thresh, critical_thresh = sfp.get_temperature_info()
                if not sw_control:
                    return
                fault = ERROR_READ_THERMAL_DATA if (temperature is None or warning_thresh is None or critical_thresh is None) else 0
                temperature = 0 if temperature is None else temperature * SFP_TEMPERATURE_SCALE
                warning_thresh = 0 if warning_thresh is None else warning_thresh * SFP_TEMPERATURE_SCALE
                critical_thresh = 0 if critical_thresh is None else critical_thresh * SFP_TEMPERATURE_SCALE

                hw_management_independent_mode_update.thermal_data_set_module(
                    0, # ASIC index always 0 for now
                    sfp.sdk_index + 1,
                    int(temperature),
                    int(critical_thresh),
                    int(warning_thresh),
                    fault
                )
            else:
                if pre_presence != presence:
                    # thermal control service requires to
                    # set value 0 to all temperature files when module is not present
                    hw_management_independent_mode_update.thermal_data_set_module(
                        0,  # ASIC index always 0 for now
                        sfp.sdk_index + 1,
                        0,
                        0,
                        0,
                        0
                    )

            if pre_presence != presence:
                self._sfp_status[sfp.sdk_index] = presence
        except Exception as e:
            logger.log_error(f'Failed to update module {sfp.sdk_index} thermal data - {e}')
            hw_management_independent_mode_update.thermal_data_set_module(
                0, # ASIC index always 0 for now
                sfp.sdk_index + 1,
                0,
                0,
                0,
                ERROR_READ_THERMAL_DATA
            )

    def update_module(self):
        for sfp in self._sfp_list:
            self.update_single_module(sfp)

    def update_asic(self):
        try:
            for asic_index in range(DeviceDataManager.get_asic_count()):
                asic_temp = self.get_asic_temp(asic_index)
                warn_threshold = self.get_asic_temp_warning_threshold(asic_index)
                critical_threshold = self.get_asic_temp_critical_threshold(asic_index)
                fault = 0
                if asic_temp is None:
                    logger.log_error(f'Failed to read ASIC {asic_index} temperature, send fault to hw-management-tc')
                    asic_temp = warn_threshold
                    fault = ERROR_READ_THERMAL_DATA

                hw_management_independent_mode_update.thermal_data_set_asic(
                    asic_index,
                    asic_temp,
                    critical_threshold,
                    warn_threshold,
                    fault
                )
        except Exception as e:
            logger.log_error(f'Failed to update ASIC thermal data - {e}')
            hw_management_independent_mode_update.thermal_data_set_asic(
                asic_index,
                0,
                0,
                0,
                ERROR_READ_THERMAL_DATA
            )
