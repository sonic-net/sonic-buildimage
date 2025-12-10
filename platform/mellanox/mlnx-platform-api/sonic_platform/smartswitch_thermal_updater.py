#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
from .thermal_updater import ThermalUpdater
from sonic_py_common import logger

import sys

sys.path.append('/run/hw-management/bin')

try:
    import hw_management_dpu_thermal_update
except ImportError:
    # For unit test and for non-smartswitch systems, these functions should not be called
    from unittest import mock
    hw_management_dpu_thermal_update = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_drive_set = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear = mock.MagicMock()
    hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear = mock.MagicMock()

CRIT_THRESH = "critical_high_threshold"
HIGH_THRESH = "high_threshold"
TEMPERATURE_DATA = "temperature"
DPU_STATUS_OFFLINE = "Offline"
DPU_STATUS_ONLINE = "Online"
CPU_FIELD = "CPU"
NVME_FIELD = "NVME"
DDR_FIELD = "DDR"
dpu_func_dict = {
                    CPU_FIELD: hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set,
                    NVME_FIELD: hw_management_dpu_thermal_update.thermal_data_dpu_drive_set,
                    DDR_FIELD: hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set,
                 }

ERROR_READ_THERMAL_DATA = 254000

TC_CONFIG_FILE = '/run/hw-management/config/tc_config.json'
logger = logger.Logger('smart-switch-thermal-updater')


class SmartswitchThermalUpdater(ThermalUpdater):
    def __init__(self, sfp_list, dpu_list, is_host_mgmt_mode=True):
        super().__init__(sfp_list=sfp_list)
        self._dpu_list = dpu_list
        self._dpu_status = {}
        self.host_mgmt_mode = is_host_mgmt_mode

    def load_tc_config_dpu(self):
        dpu_poll_interval = 3
        data = utils.load_json_file(TC_CONFIG_FILE, log_func=None)
        if data:
            dev_parameters = data.get('dev_parameters', {})
            dpu_parameter = dev_parameters.get('dpu\\d+_module', {})
            dpu_poll_interval_config = dpu_parameter.get('poll_time')
            dpu_poll_interval = int(dpu_poll_interval_config) / 2 if dpu_poll_interval_config else dpu_poll_interval
        else:
            logger.log_notice(f'{TC_CONFIG_FILE} does not exist, use default polling interval')
        logger.log_notice(f'DPU polling interval: {dpu_poll_interval}')
        self._timer.schedule(dpu_poll_interval, self.update_dpu)

    def start(self):
        self.clean_thermal_data_dpu()
        self.load_tc_config_dpu()
        if self.host_mgmt_mode:
            super().start()
        else:
            self._timer.start()

    def stop(self):
        if self.host_mgmt_mode:
            super().stop()
        else:
            self._timer.stop()

    def clean_thermal_data_dpu(self):
        for dpu in self._dpu_list:
            self.thermal_data_dpu_clear(dpu.get_hw_mgmt_id())

    def thermal_data_dpu_clear(self, dpu_index):
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear(dpu_index)
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear(dpu_index)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear(dpu_index)

    def get_dpu_temperature_data_from_dict_obj(self, dpu_component_temperature_data, field_name):
        value = dpu_component_temperature_data.get(field_name)
        fault_state = False
        if not value:
            fault_state = True
            return 0, fault_state
        try:
            int_value = int(float(value))
        except ValueError:
            logger.log_error(f"Unable to obtain temperature data for DPU {field_name}: {value}")
            int_value = 0
            fault_state = True
        return int_value, fault_state

    def get_dpu_component_temperature_data(self, dpu_temperature_data, component_name):
        dpu_component_temperature_data = dpu_temperature_data.get(component_name, {})
        output_dict = {}
        output_false_state = False
        for value in [TEMPERATURE_DATA, HIGH_THRESH, CRIT_THRESH]:
            output_dict[value], fault_state = self.get_dpu_temperature_data_from_dict_obj(dpu_component_temperature_data, value)
            output_false_state = output_false_state or fault_state
        return output_dict[TEMPERATURE_DATA], output_dict[HIGH_THRESH], output_dict[CRIT_THRESH], ERROR_READ_THERMAL_DATA if output_false_state else 0

    def update_dpu_temperature(self, dpu, fault_state=False):
        dpu_temperature_data = dpu.get_temperature_dict() if not fault_state else {}
        for key, func in dpu_func_dict.items():
            temp_data, temp_thresh, temp_crit_thresh, fault_val = self.get_dpu_component_temperature_data(dpu_temperature_data, key)
            return_val = func(dpu.get_hw_mgmt_id(), temp_data, temp_thresh, temp_crit_thresh, fault_val)
            if not return_val:
                logger.log_error(f"Unable to update Temperature data to hw-mgmt for {key} for {dpu.get_name()}")

    def update_single_dpu(self, dpu):
        try:
            dpu_oper_status = dpu.get_oper_status()
            pre_oper_status = self._dpu_status.get(dpu.get_name())
            if dpu_oper_status == DPU_STATUS_ONLINE:
                self.update_dpu_temperature(dpu)
            elif pre_oper_status != dpu_oper_status:
                # If dpu is shutdown from previous execution
                self.thermal_data_dpu_clear(dpu.get_hw_mgmt_id())
            if pre_oper_status != dpu_oper_status:
                # If there is a change in oper_status (irrespective of type of change)
                self._dpu_status[dpu.get_name()] = dpu_oper_status
        except Exception as e:
            logger.log_error(f'Failed to update DPU {dpu.get_hw_mgmt_id()} thermal data - {e}')
            self.update_dpu_temperature(dpu, fault_state=True)

    def update_dpu(self):
        for dpu in self._dpu_list:
            self.update_single_dpu(dpu)
