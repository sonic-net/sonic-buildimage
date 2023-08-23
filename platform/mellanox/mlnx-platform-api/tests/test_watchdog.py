#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.chassis import Chassis
from sonic_platform.watchdog import get_watchdog,    \
                                    WatchdogType2,   \
                                    WatchdogType1,   \
                                    is_mlnx_wd_main, \
                                    is_wd_type2


class TestWatchdog:
    @mock.patch('sonic_platform.watchdog.is_mlnx_wd_main')
    @mock.patch('sonic_platform.watchdog.os.listdir')
    def test_get_watchdog_no_device(self, mock_listdir, mock_is_main):
        mock_listdir.return_value = []
        assert get_watchdog() is None

        mock_listdir.return_value = ['invalid']
        mock_is_main.return_value = True
        assert get_watchdog() is None

        mock_listdir.return_value = ['watchdog1']
        mock_is_main.return_value = False
        assert get_watchdog() is None

    @mock.patch('sonic_platform.watchdog.is_mlnx_wd_main')
    @mock.patch('sonic_platform.watchdog.is_wd_type2')
    @mock.patch('sonic_platform.watchdog.os.listdir', mock.MagicMock(return_value=['watchdog1', 'watchdog2']))
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.open_handle', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.fcntl.ioctl', mock.MagicMock())
    @pytest.mark.parametrize('test_para',
                             [(True, WatchdogType2), (False, WatchdogType1)])
    def test_get_watchdog(self, mock_is_type2, mock_is_main, test_para):
        mock_is_main.side_effect = lambda dev: dev == 'watchdog2'
        mock_is_type2.return_value = test_para[0]
        chassis = Chassis()
        watchdog = chassis.get_watchdog()
        assert isinstance(watchdog, test_para[1])
        assert watchdog.watchdog_path == '/dev/watchdog2'

    def test_is_mlnx_wd_main(self):
        mock_os_open = mock.mock_open(read_data='mlx-wdt-main')
        with mock.patch('sonic_platform.watchdog.open', mock_os_open):
            assert is_mlnx_wd_main('')

        mock_os_open = mock.mock_open(read_data='invalid')
        with mock.patch('sonic_platform.watchdog.open', mock_os_open):
            assert not is_mlnx_wd_main('')
        mock_os_open.side_effect = IOError
        with mock.patch('sonic_platform.watchdog.open', mock_os_open):
            assert not is_mlnx_wd_main('')

    @mock.patch('sonic_platform.watchdog.os.path.exists')
    @pytest.mark.parametrize('test_para',
                             [True, False])
    def test_is_wd_type2(self, mock_exists, test_para):
        mock_exists.return_value = test_para
        assert is_wd_type2('') is test_para

    @mock.patch('sonic_platform.utils.read_str_from_file')
    def test_is_armed(self, mock_read):
        watchdog = WatchdogType2('watchdog2')
        mock_read.return_value = 'inactive'
        assert not watchdog.is_armed()
        mock_read.return_value = 'active'
        assert watchdog.is_armed()

    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.open_handle', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.fcntl.ioctl', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.is_armed')
    def test_arm_disarm_watchdog2(self, mock_is_armed):
        watchdog = WatchdogType2('watchdog2')
        assert watchdog.arm(-1) == -1
        mock_is_armed.return_value = False
        watchdog.arm(10)
        mock_is_armed.return_value = True
        watchdog.arm(5)
        watchdog.disarm()

    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.open_handle', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.fcntl.ioctl', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.is_armed')
    def test_arm_disarm_watchdog1(self, mock_is_armed):
        watchdog = WatchdogType1('watchdog1')
        assert watchdog.arm(-1) == -1
        mock_is_armed.return_value = False
        watchdog.arm(10)
        mock_is_armed.return_value = True
        watchdog.arm(5)
        watchdog.disarm()

    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.open_handle', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.fcntl.ioctl', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase._gettimeleft', mock.MagicMock(return_value=10))
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.is_armed')
    def test_get_remaining_time_watchdog2(self, mock_is_armed):
        watchdog = WatchdogType2('watchdog2')
        mock_is_armed.return_value = False
        assert watchdog.get_remaining_time() == -1
        watchdog.arm(10)
        mock_is_armed.return_value = True
        assert watchdog.get_remaining_time() == 10

    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.open_handle', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.fcntl.ioctl', mock.MagicMock())
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase._gettimeleft', mock.MagicMock(return_value=10))
    @mock.patch('sonic_platform.watchdog.WatchdogImplBase.is_armed')
    def test_get_remaining_time_watchdog1(self, mock_is_armed):
        watchdog = WatchdogType1('watchdog2')
        mock_is_armed.return_value = False
        assert watchdog.get_remaining_time() == -1
        watchdog.arm(10)
        mock_is_armed.return_value = True
        assert watchdog.get_remaining_time() > 0
