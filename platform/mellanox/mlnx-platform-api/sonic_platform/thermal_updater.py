#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2023-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from . import utils
from sonic_py_common import logger

import sys
import time

sys.path.append('/run/hw-management/bin')

try:
    import hw_management_independent_mode_update
except ImportError:
    # For unit test only
    from unittest import mock
    hw_management_independent_mode_update = mock.MagicMock()
    hw_management_independent_mode_update.module_data_set_module_counter = mock.MagicMock()
    hw_management_independent_mode_update.thermal_data_set_asic = mock.MagicMock()
    hw_management_independent_mode_update.thermal_data_set_module = mock.MagicMock()
    hw_management_independent_mode_update.thermal_data_clean_asic = mock.MagicMock()
    hw_management_independent_mode_update.thermal_data_clean_module = mock.MagicMock()


SFP_TEMPERATURE_SCALE = 1000
ASIC_TEMPERATURE_SCALE = 125
ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD = 105000
ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD = 120000

ERROR_READ_THERMAL_DATA = 254000

TC_CONFIG_FILE = '/run/hw-management/config/tc_config.json'
logger = logger.Logger('thermal-updater')


class ThermalUpdater:
    def __init__(self, sfp_list):
        self._sfp_list = sfp_list
        self._sfp_status = {}
        self._timer = utils.Timer()

    def load_tc_config(self):
        asic_poll_interval = 1
        sfp_poll_interval = 10
        data = utils.load_json_file(TC_CONFIG_FILE, log_func=None)
        if not data:
            logger.log_notice(f'{TC_CONFIG_FILE} does not exist, use default polling interval')

        if data:
            dev_parameters = data.get('dev_parameters')
            if dev_parameters is not None:
                asic_parameter = dev_parameters.get('asic')
                if asic_parameter is not None:
                    asic_poll_interval_config = asic_parameter.get('poll_time')
                    if asic_poll_interval_config:
                        asic_poll_interval = int(asic_poll_interval_config) / 2
                module_parameter = dev_parameters.get('module\\d+')
                if module_parameter is not None:
                    sfp_poll_interval_config = module_parameter.get('poll_time')
                    if sfp_poll_interval_config:
                        sfp_poll_interval = int(sfp_poll_interval_config) / 2

        logger.log_notice(f'ASIC polling interval: {asic_poll_interval}')
        self._timer.schedule(asic_poll_interval, self.update_asic)
        logger.log_notice(f'Module polling interval: {sfp_poll_interval}')
        self._timer.schedule(sfp_poll_interval, self.update_module)

    def start(self):
        self.clean_thermal_data()
        self.control_tc(False)
        self.load_tc_config()
        self._timer.start()

    def stop(self):
        self._timer.stop()
        self.control_tc(True)

    def control_tc(self, suspend):
        logger.log_notice(f'Set hw-management-tc to {"suspend" if suspend else "resume"}')
        utils.write_file('/run/hw-management/config/suspend', 1 if suspend else 0)

    def clean_thermal_data(self):
        hw_management_independent_mode_update.module_data_set_module_counter(len(self._sfp_list))
        hw_management_independent_mode_update.thermal_data_clean_asic(0)
        for sfp in self._sfp_list:
            hw_management_independent_mode_update.thermal_data_clean_module(
                0,
                sfp.sdk_index + 1
            )

    def get_asic_temp(self):
        temperature = utils.read_int_from_file('/sys/module/sx_core/asic0/temperature/input', default=None)
        return temperature * ASIC_TEMPERATURE_SCALE if temperature is not None else None

    def get_asic_temp_warning_threshold(self):
        emergency = utils.read_int_from_file('/sys/module/sx_core/asic0/temperature/emergency', default=None, log_func=None)
        return emergency * ASIC_TEMPERATURE_SCALE if emergency is not None else ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD

    def get_asic_temp_critical_threshold(self):
        critical = utils.read_int_from_file('/sys/module/sx_core/asic0/temperature/critical', default=None, log_func=None)
        return critical * ASIC_TEMPERATURE_SCALE if  critical is not None else ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD

    def update_single_module(self, sfp):
        try:
            presence = sfp.get_presence()
            pre_presence = self._sfp_status.get(sfp.sdk_index)
            if presence:
                temperature, warning_thresh, critical_thresh = sfp.get_temperature_info()
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
            asic_temp = self.get_asic_temp()
            warn_threshold = self.get_asic_temp_warning_threshold()
            critical_threshold = self.get_asic_temp_critical_threshold()
            fault = 0
            if asic_temp is None:
                logger.log_error('Failed to read ASIC temperature, send fault to hw-management-tc')
                asic_temp = warn_threshold
                fault = ERROR_READ_THERMAL_DATA

            hw_management_independent_mode_update.thermal_data_set_asic(
                0, # ASIC index always 0 for now
                asic_temp,
                critical_threshold,
                warn_threshold,
                fault
            )
        except Exception as e:
            logger.log_error(f'Failed to update ASIC thermal data - {e}')
            hw_management_independent_mode_update.thermal_data_set_asic(
                0, # ASIC index always 0 for now
                0,
                0,
                0,
                ERROR_READ_THERMAL_DATA
            )
