#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import os
import pytest
import sys
import subprocess
from unittest import mock
from sonic_platform.watchdog import Watchdog, WD_COMMON_ERROR

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)


disarm_string = 'primary: /dev/mmcblk0boot0\n'\
        'backup: /dev/mmcblk0boot1\n'\
        'boot-bus-width: x8\n'\
        'reset to x1 after reboot: FALSE\n'\
        'boot watchdog interval: 0\n'\
        'boot watchdog mode: disabled\n'\
        'watchdog-swap: disabled\n'\
        'lifecycle state: Secured (development)\n'\
        'secure boot key free slots: 3'


arm_string = 'primary: /dev/mmcblk0boot0\n'\
        'backup: /dev/mmcblk0boot1\n'\
        'boot-bus-width: x8\n'\
        'reset to x1 after reboot: FALSE\n'\
        'boot watchdog interval: 180\n'\
        'boot watchdog mode: standard\n'\
        'watchdog-swap: disabled\n'\
        'lifecycle state: Secured (development)\n'\
        'secure boot key free slots: 3'


class TestWatchdog:
    @mock.patch('sonic_platform.watchdog.Watchdog.is_armed_for_time')
    @mock.patch('sonic_platform.watchdog.Watchdog.exec_cmd')
    def test_arm(self, mock_exec_cmd, mock_is_armed_for_time):
        # Valid Arm time
        test_wd = Watchdog()
        assert test_wd.arm(0) == WD_COMMON_ERROR
        assert test_wd.arm(-1) == WD_COMMON_ERROR
        assert test_wd.arm(4096) == WD_COMMON_ERROR
        # If watchdog is already armed we do not re-arm it
        mock_exec_cmd.return_value = 'standard'
        mock_is_armed_for_time.return_value = True
        assert test_wd.arm(180) == 180
        mock_exec_cmd.assert_not_called()
        mock_is_armed_for_time.return_value = False
        assert test_wd.arm(180) == 180
        mock_exec_cmd.assert_called_once_with(['mlxbf-bootctl',
                                               '--watchdog-boot-mode',
                                               'standard',
                                               '--watchdog-boot-interval',
                                               '180'])

    @mock.patch('sonic_platform.watchdog.Watchdog.exec_cmd')
    def test_disarm(self, mock_exec_cmd):
        # Valid Arm time
        test_wd = Watchdog()
        assert test_wd.disarm()
        mock_exec_cmd.side_effect = subprocess.CalledProcessError(returncode=1, cmd='mlxbf-bootctl')
        assert not test_wd.disarm()

    @mock.patch('sonic_platform.watchdog.Watchdog.is_armed_for_time')
    @mock.patch('sonic_platform.watchdog.Watchdog.exec_cmd')
    @mock.patch('sonic_platform.watchdog.Watchdog.get_conf_time_and_mode')
    @mock.patch('sonic_platform.watchdog.Watchdog.is_armed')
    def test_get_remaining_time_watchdog(self, mock_is_armed, mock_get_time, mock_exec_cmd, mock_is_armed_for_time):
        watchdog = Watchdog()
        mock_is_armed.return_value = False
        assert watchdog.get_remaining_time() == WD_COMMON_ERROR
        mock_exec_cmd.return_value = True
        mock_is_armed_for_time.return_value = False
        watchdog.arm(180)
        mock_is_armed.return_value = True
        mock_get_time.return_value = 'standard', 100
        first_value = watchdog.get_remaining_time()
        assert first_value > 0
        second_value = watchdog.get_remaining_time()
        assert second_value <= first_value

    @mock.patch('sonic_platform.watchdog.Watchdog.exec_cmd')
    def test_is_armed(self, mock_exec_cmd):
        watchdog = Watchdog()
        mock_exec_cmd.return_value = disarm_string
        assert not watchdog.is_armed()
        assert not watchdog.is_armed_for_time(100)
        assert not watchdog.is_armed_for_time(50)
        assert not watchdog.is_armed_for_time(180)
        mock_exec_cmd.return_value = arm_string
        assert watchdog.is_armed()
        assert not watchdog.is_armed_for_time(100)
        assert not watchdog.is_armed_for_time(50)
        assert watchdog.is_armed_for_time(180)
        change_time_str = arm_string.replace('180', '200')
        mock_exec_cmd.return_value = change_time_str
        assert watchdog.is_armed_for_time(200)

    @mock.patch('sonic_platform.watchdog.Watchdog.exec_cmd')
    def test_conf_time(self, mock_exec_cmd):
        watchdog = Watchdog()
        mock_exec_cmd.return_value = disarm_string
        arm_str, arm_time = watchdog.get_conf_time_and_mode()
        assert arm_str == "disabled"
        assert arm_time == 0
        mock_exec_cmd.return_value = arm_string
        arm_str, arm_time = watchdog.get_conf_time_and_mode()
        assert arm_str == "standard"
        assert arm_time == 180
        mock_exec_cmd.side_effect = subprocess.CalledProcessError(returncode=1, cmd='mlxbf-bootctl')
        arm_str, arm_time = watchdog.get_conf_time_and_mode()
        assert arm_str == "disabled"
        assert arm_time == 0

    def test_exec_cmd(self):
        watchdog = Watchdog()
        assert watchdog.exec_cmd(['echo', 'abc']) == "abc"
        assert watchdog.exec_cmd(['echo', 't1']) == "t1"
        with pytest.raises(subprocess.CalledProcessError):
            watchdog.exec_cmd(['ls', 'non-existent-dir'])
            watchdog.exec_cmd(['cat', 'non-existent-file.txt'])
            watchdog.exec_cmd(['non-existent-cmd'])
