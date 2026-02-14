#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

"""dpuctl Tests Implementation"""
import os
import sys
import json
from tabulate import tabulate

from click.testing import CliRunner
from smart_switch.dpuctl.main import dpuctl, dpuctl_get_status, dpuctl_power_off, dpuctl_power_on, dpuctl_reset
from sonic_platform.dpuctlplat import DpuCtlPlat
from tests.dpuctl_inputs.dpuctl_test_inputs import testData, status_output

if sys.version_info.major == 3:
    from unittest.mock import MagicMock, patch


test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)
scripts_path = os.path.join(modules_path, "scripts")
test_ip = os.path.join(modules_path, "tests")
example_platform = os.path.join(str(test_ip), "dpuctl_inputs", "platform.json")


def create_dpu_list():
    """Create dpu object list for Function calls"""
    existing_dpu_list = ['dpu0', 'dpu1', 'dpu2', 'dpu3']
    dpuctl_dict = {}
    for dpu_name in existing_dpu_list:
        dpuctl_dict[dpu_name] = DpuCtlPlat(dpu_name)
    context = {
        "dpuctl_dict": dpuctl_dict,
    }
    return context


obj = create_dpu_list()


def dpuctl_command_exec(exec_cmd, command_name):
    """General Command Execution and return value
        validation function for all the APIs"""
    test_data_checks = testData[command_name]
    runner = CliRunner()
    assertion_checks = test_data_checks['AssertionError']
    for args in assertion_checks['arg_list']:
        result = runner.invoke(exec_cmd, args, catch_exceptions=False, obj=obj)
        assert "AssertionError" in result.output

    result_checks = test_data_checks['Returncheck']
    for index_value in range(len(result_checks['arg_list'])):
        print(index_value)
        args = result_checks['arg_list'][index_value]
        return_code = result_checks['rc'][index_value]
        return_message = result_checks['return_message'][index_value]
        return_message = return_message.replace('"', "'").lower()
        result = runner.invoke(exec_cmd, args, catch_exceptions=False, obj=obj)
        assert result.exit_code == return_code
        assert return_message == result.output.replace('"', "'").lower()


class Testdpuctl:
    """Tests for dpuctl Platform API Wrapper"""
    @classmethod
    def setup_class(cls):
        """Setup function for all tests for dpuctl implementation"""
        os.environ["PATH"] += os.pathsep + scripts_path
        os.environ["MLNX_PLATFORM_API_DPUCTL_UNIT_TESTING"] = "2"

    @patch('multiprocessing.Process.start', MagicMock(return_value=True))
    @patch('multiprocessing.Process.join', MagicMock(return_value=True))
    def test_dpuctl_power_off(self):
        """Tests for dpuctl click Implementation for Power Off API"""
        exec_cmd = dpuctl_power_off
        dpuctl_command_exec(exec_cmd, "PW_OFF")

    @patch('multiprocessing.Process.start', MagicMock(return_value=True))
    @patch('multiprocessing.Process.join', MagicMock(return_value=True))
    def test_dpuctl_power_on(self):
        """Tests for dpuctl click Implementation for Power On API"""
        exec_cmd = dpuctl_power_on
        dpuctl_command_exec(exec_cmd, "PW_ON")

    @patch('multiprocessing.Process.start', MagicMock(return_value=True))
    @patch('multiprocessing.Process.join', MagicMock(return_value=True))
    def test_dpuctl_reset(self):
        """Tests for dpuctl click Implementation for Reset API"""
        exec_cmd = dpuctl_reset
        dpuctl_command_exec(exec_cmd, "RST")

    def test_dpuctl_status(self):
        """Tests for dpuctl click Implementation for Status API"""
        mock_file_list = ['shtdn_ready', '_ready', 'boot_progress', 'pwr_force']
        mock_return_value = [0, 1, 5, 1]
        assert len(mock_file_list) == len(mock_return_value)

        def mock_read_int_from_file(file_path, default=0, raise_exception=False, log_func=None):
            for index, value in enumerate(mock_file_list):
                if file_path.endswith(value):
                    return mock_return_value[index]
            return 0

        with patch("sonic_platform.utils.read_int_from_file", wraps=mock_read_int_from_file):
            cmd = dpuctl_get_status
            runner = CliRunner()
            result = runner.invoke(cmd, catch_exceptions=False, obj=obj)
            print(result.output)
            assert result.output == status_output[0]
            result = runner.invoke(cmd, ['dpu1'], catch_exceptions=False, obj=obj)
            assert result.output == status_output[1]
            result = runner.invoke(cmd, ['dpu0'], catch_exceptions=False, obj=obj)
            assert result.output == status_output[2]
            result = runner.invoke(cmd, ['dpu5'], catch_exceptions=False, obj=obj)
            assert result.output == status_output[3]
            result = runner.invoke(cmd, ['dpu10'], catch_exceptions=False, obj=obj)
            assert result.output == status_output[4]
            mock_return_value = [1, 0, 0, 1]
            result = runner.invoke(cmd, catch_exceptions=False, obj=obj)
            assert result.output == status_output[5]
            header = ["DPU", "dpu ready", "dpu shutdown ready", "boot progress", "Force Power Required"]
            boot_prog_map = {
                0: "Reset/Boot-ROM",
                1: "BL2 (from ATF image on eMMC partition)",
                2: "BL31 (from ATF image on eMMC partition)",
                3: "UEFI (from UEFI image on eMMC partition)",
                4: "OS Starting",
                5: "OS is running",
                6: "Low-Power Standby",
                7: "FW Update in progress",
                8: "OS Crash Dump in progress",
                9: "OS Crash Dump is complete",
                10: "FW Fault Crash Dump in progress",
                11: "FW Fault Crash Dump is complete",
                15: "Software is inactive"
            }
            for key in boot_prog_map.keys():
                mock_return_value = [0, 1, key, 1]
                result = runner.invoke(cmd, ['dpu1'], catch_exceptions=False, obj=obj)
                expected_value = f"{key} - {boot_prog_map.get(key)}"
                expected_data = [[status_output[6][0], status_output[6][1], status_output[6][2], expected_value, status_output[6][3]]]
                expected_res = tabulate(expected_data, header)
                assert result.output == expected_res + "\n"
            mock_return_value = [5, 5, 25, 6]
            expected_data = [["dpu1", "5 - N/A", "5 - N/A", "25 - N/A", "6 - N/A"]]
            result = runner.invoke(cmd, ['dpu1'], catch_exceptions=False, obj=obj)
            expected_res = tabulate(expected_data, header)
            assert result.output == expected_res + "\n"
