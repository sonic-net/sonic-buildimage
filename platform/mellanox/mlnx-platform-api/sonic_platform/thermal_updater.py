#
# Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES.
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

SFP_TEMPERATURE_SCALE = 1000
ASIC_TEMPERATURE_SCALE = 125
ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD = 105000
ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD = 120000
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
logger = logger.Logger('thermal-updater')


class ThermalUpdater:
    def __init__(self, sfp_list, dpu_list=[], is_host_mgmt_mode=True):
        # Default initialization is in host mgmt mode without dpus
        self._sfp_list = sfp_list
        self._sfp_status = {}
        self._timer = utils.Timer()
        self._dpu_list = dpu_list
        self._dpu_status = dpu_list
        self.dpus_exist = False
        if len(self._dpu_list) > 0:
            self.dpus_exist = True
        self._dpu_status = {}
        self.dev_parameters = None
        self.data = None
        self.read_checked = False
        self.configure_functions(self.dpus_exist, is_host_mgmt_mode)

    def configure_functions(self, dpu, independent_mode):
        self.start = self.start_independent_mode
        self.stop = self.stop_independent_mode
        self.load_tc_config = self.load_tc_config_asic_sfp
        self.clean_thermal_data = self.clean_thermal_data_asic_sfp
        if dpu:
            self.clean_thermal_data = self.clean_all
            self.load_tc_config = self.load_tc_config_all
            if not independent_mode:
                self.start = self.start_no_independent
                self.stop = self.stop_no_independent_mode
                self.load_tc_config = self.load_tc_config_dpu
                self.clean_thermal_data = self.clean_thermal_data_dpu

    def read_tc_config_data(self):
        if self.read_checked:
            return self.data
        data = utils.load_json_file(TC_CONFIG_FILE, log_func=None)
        if not data:
            logger.log_notice(f'{TC_CONFIG_FILE} does not exist, use default polling interval')
        self.data = data
        self.read_checked = True
        return self.data

    def load_tc_config_asic_sfp(self):
        asic_poll_interval = 1
        sfp_poll_interval = 10
        data = self.read_tc_config_data()

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

    def load_tc_config_dpu(self):
        dpu_poll_interval = 3
        data = self.read_tc_config_data()
        if data:
            dev_parameters = data.get('dev_parameters', {})
            dpu_parameter = dev_parameters.get('dpu\\d+_module', {})
            dpu_poll_interval_config = dpu_parameter.get('poll_time')
            dpu_poll_interval = int(dpu_poll_interval_config) / 2 if dpu_poll_interval_config else dpu_poll_interval
        logger.log_notice(f'DPU polling interval: {dpu_poll_interval}')
        self._timer.schedule(dpu_poll_interval, self.update_dpu)

    def load_tc_config_all(self):
        self.load_tc_config_asic_sfp()
        self.load_tc_config_dpu()

    def start_independent_mode(self):
        self.clean_thermal_data()
        self.control_tc(False)
        self.load_tc_config()
        self._timer.start()

    def start_no_independent(self):
        self.clean_thermal_data()
        self.load_tc_config()
        self._timer.start()

    def stop_independent_mode(self):
        self._timer.stop()
        self.control_tc(True)

    def stop_no_independent_mode(self):
        self._timer.stop()

    def control_tc(self, suspend):
        logger.log_notice(f'Set hw-management-tc to {"suspend" if suspend else "resume"}')
        utils.write_file('/run/hw-management/config/suspend', 1 if suspend else 0)

    def clean_all(self):
        self.clean_thermal_data_asic_sfp()
        self.clean_thermal_data_dpu()

    def clean_thermal_data_asic_sfp(self):
        hw_management_independent_mode_update.module_data_set_module_counter(len(self._sfp_list))
        hw_management_independent_mode_update.thermal_data_clean_asic(0)
        for sfp in self._sfp_list:
            hw_management_independent_mode_update.thermal_data_clean_module(
                0,
                sfp.sdk_index + 1
            )

    def clean_thermal_data_dpu(self):
        for dpu in self._dpu_list:
            self.thermal_data_dpu_clear(dpu.get_hw_mgmt_id())

    def thermal_data_dpu_clear(self, dpu_index):
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear(dpu_index)
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear(dpu_index)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear(dpu_index)

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
                temperature = sfp.get_temperature()
                if temperature == 0:
                    warning_thresh = 0
                    critical_thresh = 0
                    fault = 0
                else:
                    warning_thresh = sfp.get_temperature_warning_threshold()
                    critical_thresh = sfp.get_temperature_critical_threshold()
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
                    hw_management_independent_mode_update.thermal_data_clean_module(0, sfp.sdk_index + 1)

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
                else:
                    if pre_oper_status != dpu_oper_status:
                        self.thermal_data_dpu_clear(dpu.get_hw_mgmt_id())
                if pre_oper_status != dpu_oper_status:
                    self._dpu_status[dpu.get_name()] = dpu_oper_status
            except Exception as e:
                logger.log_error(f'Failed to update DPU {dpu.get_hw_mgmt_id()} thermal data - {e}')
                self.update_dpu_temperature(dpu, fault_state=True)

    def update_module(self):
        for sfp in self._sfp_list:
            self.update_single_module(sfp)

    def update_dpu(self):
        for dpu in self._dpu_list:
            self.update_single_dpu(dpu)

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
