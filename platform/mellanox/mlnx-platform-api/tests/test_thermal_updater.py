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

from unittest import mock
import copy

from sonic_platform import utils
from sonic_platform.thermal_updater import ThermalUpdater, hw_management_independent_mode_update, hw_management_dpu_thermal_update
from sonic_platform.thermal_updater import ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD, \
                                           ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD, ERROR_READ_THERMAL_DATA


mock_tc_config = """
{
    "dev_parameters": {
        "asic": {
            "pwm_min": 20,
            "pwm_max": 100,
            "val_min": "!70000",
            "val_max": "!105000",
            "poll_time": 3
        },
        "module\\\\d+": {
            "pwm_min": 20,
            "pwm_max": 100,
            "val_min": 60000,
            "val_max": 80000,
            "poll_time": 20
        },
        "dpu\\\\d+_module": {
            "child_sensors_list": ["cx_amb", "voltmon1", "voltmon2"],
            "poll_time": 24
        }
    }
}
"""


class TestThermalUpdater:
    def test_load_tc_config_non_exists(self):
        updater = ThermalUpdater(None)
        updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    def test_load_tc_config_mocked(self):
        updater = ThermalUpdater(None)
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 2

    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_asic', mock.MagicMock())
    @mock.patch('sonic_platform.thermal_updater.ThermalUpdater.update_module', mock.MagicMock())
    @mock.patch('sonic_platform.utils.write_file')
    def test_start_stop(self, mock_write):
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 1
        updater = ThermalUpdater([mock_sfp])
        updater.start()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 0)
        utils.wait_until(updater._timer.is_alive, timeout=5)

        mock_write.reset_mock()
        updater.stop()
        assert not updater._timer.is_alive()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 1)

    @mock.patch('sonic_platform.utils.read_int_from_file')
    def test_update_asic(self, mock_read):
        mock_read.return_value = 8
        updater = ThermalUpdater(None)
        assert updater.get_asic_temp() == 1000
        assert updater.get_asic_temp_warning_threshold() == 1000
        assert updater.get_asic_temp_critical_threshold() == 1000
        updater.update_asic()
        hw_management_independent_mode_update.thermal_data_set_asic.assert_called_once()

        mock_read.return_value = None
        assert updater.get_asic_temp() is None
        assert updater.get_asic_temp_warning_threshold() == ASIC_DEFAULT_TEMP_WARNNING_THRESHOLD
        assert updater.get_asic_temp_critical_threshold() == ASIC_DEFAULT_TEMP_CRITICAL_THRESHOLD

    def test_update_module(self):
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 10
        mock_sfp.get_presence = mock.MagicMock(return_value=True)
        mock_sfp.get_temperature = mock.MagicMock(return_value=55.0)
        mock_sfp.get_temperature_warning_threshold = mock.MagicMock(return_value=70.0)
        mock_sfp.get_temperature_critical_threshold = mock.MagicMock(return_value=80.0)
        updater = ThermalUpdater([mock_sfp])
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 55000, 80000, 70000, 0)

        mock_sfp.get_temperature = mock.MagicMock(return_value=0.0)
        hw_management_independent_mode_update.reset_mock()
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_set_module.assert_called_once_with(0, 11, 0, 0, 0, 0)

        mock_sfp.get_presence = mock.MagicMock(return_value=False)
        updater.update_module()
        hw_management_independent_mode_update.thermal_data_clean_module.assert_called_once_with(0, 11)

    # Smartswitch specific tests
    def test_load_tc_config_non_exists_dpu(self):
        dpu = mock.MagicMock()
        updater = ThermalUpdater(None, dpu_list=[dpu])
        updater.load_tc_config()
        # 3 events - ASIC, DPU, sfp
        assert updater._timer._timestamp_queue.qsize() == 3
        updater = ThermalUpdater(None, dpu_list=[dpu], is_host_mgmt_mode=False)
        updater.load_tc_config()
        # 1 event - DPU
        assert updater._timer._timestamp_queue.qsize() == 1

    def test_load_tc_config_mocked_dpu(self):
        dpu = mock.MagicMock()
        updater = ThermalUpdater(None, dpu_list=[dpu])
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.load_tc_config()
        assert updater._timer._timestamp_queue.qsize() == 3

    @mock.patch('sonic_platform.utils.write_file')
    def test_configuration(self, mock_write):
        dpu = mock.MagicMock()
        mock_sfp = mock.MagicMock()
        mock_sfp.sdk_index = 1
        self.reset_hw_mgmt_mocks()
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        updater = ThermalUpdater([mock_sfp], dpu_list=[dpu])
        """ Expectation on start - Clean is called for sfp, asic, DPU
        suspend -> 1 and load config for all 3 along with start of timer"""
        updater._timer = mock.MagicMock()
        mock_os_open = mock.mock_open(read_data=mock_tc_config)
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            updater.start()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_independent_mode_update.thermal_data_clean_asic.assert_called_once()
        hw_management_independent_mode_update.thermal_data_clean_module.assert_called_once()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 0)
        assert updater._timer.schedule.call_count == 3
        # Called for DPU with time 24/2 = 12
        assert updater._timer.schedule.call_args_list[-1][0][0] == 12
        # Expectation on stop - timer stop and suspend = 1
        mock_write.reset_mock()
        updater.stop()
        updater._timer.stop.assert_called_once()
        mock_write.assert_called_once_with('/run/hw-management/config/suspend', 1)
        mock_write.reset_mock()
        self.reset_hw_mgmt_mocks()
        updater = ThermalUpdater(None, dpu_list=[dpu], is_host_mgmt_mode=False)
        """ Expectation on start - Clean is called for DPU
        load config for DPU along with start of timer"""
        updater._timer = mock.MagicMock()
        updater.start()
        mock_write.assert_not_called()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear.assert_called_once_with(dpu.get_hw_mgmt_id())
        hw_management_independent_mode_update.thermal_data_clean_asic.assert_not_called()
        hw_management_independent_mode_update.thermal_data_clean_module.assert_not_called()
        # Expectation on stop - timer stop
        updater.stop()
        updater._timer.stop.assert_called_once()
        mock_write.assert_not_called()

    def test_update_dpu(self):
        self.reset_hw_mgmt_mocks()
        mock_dpu = mock.MagicMock()
        mock_dpu.get_hw_mgmt_id = mock.MagicMock(return_value=1)
        mock_dpu.get_name = mock.MagicMock(return_value="DPU0")
        mock_dpu.get_oper_status = mock.MagicMock(return_value="Online")
        temp_data = {
            "DDR":  {'temperature': '75.0', 'high_threshold': '95', 'critical_high_threshold': '100'},
            "CPU":  {'temperature': '82.0', 'high_threshold': '90', 'critical_high_threshold': '100'},
            "NVME":  {'temperature': '91', 'high_threshold': '95', 'critical_high_threshold': '98'}
        }
        mock_dpu.get_temperature_dict = mock.MagicMock(return_value=temp_data)
        updater = ThermalUpdater(sfp_list=None, dpu_list=[mock_dpu], is_host_mgmt_mode=False)
        updater.update_dpu()
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.assert_called_once_with(1, 75, 95, 100, 0)
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.assert_called_once_with(1, 82, 90, 100, 0)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.assert_called_once_with(1, 91, 95, 98, 0)
        mock_dpu.get_temperature_dict = mock.MagicMock(return_value={})
        self.reset_hw_mgmt_mocks()
        updater.update_dpu()
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.assert_called_once_with(1, 0, 0, 0, ERROR_READ_THERMAL_DATA)
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.assert_called_once_with(1, 0, 0, 0, ERROR_READ_THERMAL_DATA)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.assert_called_once_with(1, 0, 0, 0, ERROR_READ_THERMAL_DATA)
        func_dict = {
            "DDR": hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set,
            "CPU": hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set,
            "NVME": hw_management_dpu_thermal_update.thermal_data_dpu_drive_set,
                     }
        for value in ["DDR", "CPU", "NVME"]:
            temp_data_without_entry = copy.deepcopy(temp_data)
            # One of the values in DDR, CPU and NVME is set to empty
            temp_data_without_entry[value] = {}
            mock_dpu.get_temperature_dict = mock.MagicMock(return_value=temp_data_without_entry)
            self.reset_hw_mgmt_mocks()
            updater.update_dpu()
            for key, func in func_dict.items():
                if key == value:
                    func.assert_called_once_with(1, 0, 0, 0, ERROR_READ_THERMAL_DATA)
                else:
                    func.assert_called_once_with(
                        1,
                        int(float(temp_data[key]['temperature'])),
                        int(float(temp_data[key]['high_threshold'])),
                        int(float(temp_data[key]['critical_high_threshold'])),
                        0)
            # One of the values in DDR, CPU and NVME is set to a string, can not convert to integer
            for field in ["temperature", "high_threshold", "critical_high_threshold"]:
                temp_data_invalid = copy.deepcopy(temp_data)
                temp_data_orig = copy.deepcopy(temp_data)
                temp_data_invalid[value][field] = "N/A"
                mock_dpu.get_temperature_dict = mock.MagicMock(return_value=temp_data_invalid)
                self.reset_hw_mgmt_mocks()
                updater.update_dpu()
                for key, func in func_dict.items():
                    temp_data_orig[value][field] = 0
                    func.assert_called_once_with(
                        1,
                        int(float(temp_data_orig[key]['temperature'])),
                        int(float(temp_data_orig[key]['high_threshold'])),
                        int(float(temp_data_orig[key]['critical_high_threshold'])),
                        ERROR_READ_THERMAL_DATA if value == key else 0)
        self.reset_hw_mgmt_mocks()
        mock_dpu.get_oper_status = mock.MagicMock(return_value="Offline")
        updater.update_dpu()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear.assert_called_once_with(1)
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear.assert_called_once_with(1)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear.assert_called_once_with(1)
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.assert_not_called()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.assert_not_called()
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.assert_not_called()
        # Clear is called only once
        updater.update_dpu()
        updater.update_dpu()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear.assert_called_once_with(1)
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_clear.assert_called_once_with(1)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear.assert_called_once_with(1)
        self.reset_hw_mgmt_mocks()
        mock_dpu.get_oper_status = mock.MagicMock(return_value="Online")
        mock_dpu.get_temperature_dict = mock.MagicMock(return_value=temp_data)
        updater.update_dpu()
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.assert_called_once_with(1, 75, 95, 100, 0)
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.assert_called_once_with(1, 82, 90, 100, 0)
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.assert_called_once_with(1, 91, 95, 98, 0)
        # Multiple dpus
        mock_dpu1 = mock.MagicMock()
        mock_dpu1.get_hw_mgmt_id = mock.MagicMock(return_value=2)
        mock_dpu1.get_name = mock.MagicMock(return_value="DPU1")
        mock_dpu1.get_oper_status = mock.MagicMock(return_value="Online")
        temp_data_1 = copy.deepcopy(temp_data)
        temp_data_1["DDR"]["temperature"] = "52.0"
        temp_data_1["CPU"]["temperature"] = "20.0"
        temp_data_1["NVME"]["temperature"] = "100.0"
        mock_dpu1.get_temperature_dict = mock.MagicMock(return_value=temp_data_1)
        updater = ThermalUpdater(sfp_list=None, dpu_list=[mock_dpu, mock_dpu1], is_host_mgmt_mode=False)
        self.reset_hw_mgmt_mocks()
        updater.update_dpu()
        assert hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.call_count == 2
        assert hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.call_count == 2
        assert hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.call_count == 2
        assert hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.call_args_list \
            == [mock.call(1, 75, 95, 100, 0), mock.call(2, 52, 95, 100, 0)]
        assert hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.call_args_list \
            == [mock.call(1, 82, 90, 100, 0), mock.call(2, 20, 90, 100, 0)]
        assert hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.call_args_list \
            == [mock.call(1, 91, 95, 98, 0), mock.call(2, 100, 95, 98, 0)]

    def reset_hw_mgmt_mocks(self):
        hw_management_independent_mode_update.reset_mock()
        hw_management_independent_mode_update.thermal_data_clean_module.reset_mock()
        hw_management_independent_mode_update.thermal_data_clean_asic.reset_mock()
        hw_management_independent_mode_update.module_data_set_module_counter.reset_mock()
        hw_management_independent_mode_update.thermal_data_set_asic.reset_mock()
        hw_management_independent_mode_update.thermal_data_set_module.reset_mock()
        hw_management_dpu_thermal_update.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_clean_module.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_clear.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_clear.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_dpu_ddr_set.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_dpu_cpu_core_set.reset_mock()
        hw_management_dpu_thermal_update.thermal_data_dpu_drive_set.reset_mock()
