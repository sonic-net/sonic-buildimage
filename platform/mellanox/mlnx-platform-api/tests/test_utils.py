#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import threading
import time
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import utils


class TestUtils:
    def test_read_file(self):
        ret = utils.read_str_from_file('not exist', 'default return')
        assert ret == 'default return'

        with pytest.raises(IOError):
            ret = utils.read_str_from_file('not exist', 'default return', raise_exception=True)
            assert ret == 'default return'

        ret = utils.read_int_from_file('not exist', 100)
        assert ret == 100

        with pytest.raises(IOError):
            ret = utils.read_int_from_file('not exist', 200, raise_exception=True)
            assert ret == 200

        ret = utils.read_float_from_file('not exist', 3.14)
        assert ret == 3.14

        with pytest.raises(IOError):
            ret = utils.read_float_from_file('not exist', 2.25, raise_exception=True)
            assert ret == 2.25

    def test_write_file(self):
        file_path = '/tmp/test.txt'
        utils.write_file(file_path, '  hello  ')
        assert utils.read_str_from_file(file_path) == 'hello'

        utils.write_file(file_path, '123 ')
        assert utils.read_int_from_file(file_path) == 123

        utils.write_file(file_path, '3.14 ')
        assert utils.read_float_from_file(file_path) == 3.14

        with pytest.raises(IOError):
            utils.write_file('/not/exist/file', '123', raise_exception=True)

    def test_pre_initialize(self):
        mock_call = mock.MagicMock()

        class A:
            @utils.pre_initialize(mock_call)
            def func(self):
                pass

        A().func()
        assert mock_call.call_count == 1

    def test_pre_initialize_one(self):
        mock_call = mock.MagicMock()

        class A:
            @utils.pre_initialize_one(mock_call)
            def func(self, index):
                pass

        a = A()
        a.func(34)
        mock_call.assert_called_once_with(a, 34)

    def test_read_only_cache(self):
        value = 100

        def func():
            return value

        assert func() == 100
        value = 1000
        assert func() == 1000

        @utils.read_only_cache()
        def func():
            return value

        assert func() == 1000
        value = 10000
        assert func() == 1000

    @mock.patch('sonic_py_common.logger.Logger.log_debug')
    def test_default_return(self, mock_log):
        @utils.default_return(100, log_func=mock_log)
        def func():
            raise RuntimeError('')

        assert func() == 100
        assert mock_log.call_count == 1

    def test_run_command(self):
        output = utils.run_command(['ls'])
        assert output
        assert utils.run_command(['not_a_command']) is None

    def test_run_command_exception(self):
        output = utils.run_command(['ls'])
        assert output

    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir', mock.MagicMock(return_value='.'))
    @mock.patch('sonic_py_common.device_info.get_path_to_hwsku_dir', mock.MagicMock(return_value='/tmp'))
    @mock.patch('sonic_py_common.device_info.get_hwsku', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.utils.load_json_file')
    @mock.patch('os.path.exists')
    def test_extract_RJ45_ports_index(self, mock_exists, mock_load_json):
        mock_exists.return_value = False
        rj45_list = utils.extract_RJ45_ports_index()
        assert rj45_list is None

        mock_exists.return_value = True
        platform_json = {
            'interfaces': {
                "Ethernet0": {
                    "index": "1",
                    "lanes": "0",
                    "breakout_modes": {
                        "1x1000[100,10]": ["etp1"]
                    }
                }
            }
        }
        hwsku_json = {
            'interfaces': {
                "Ethernet0": {
                    "default_brkout_mode": "1x1000[100,10]",
                    "port_type": "RJ45"
                }
            }
        }

        mock_load_json.side_effect = [platform_json, hwsku_json]
        assert utils.extract_RJ45_ports_index() == [0]

    @pytest.mark.parametrize(
        "mock_exists_value, platform_json, hwsku_json, expected_result",
        [
            # Case 1: hwsku.json file does not exist
            (False, {}, {}, None),

            # Case 2: file exists, but no CPO ports
            (
                True,
                {
                    'interfaces': {
                        "Ethernet0": {
                            "index": "1",
                            "lanes": "0",
                            "breakout_modes": {
                                "2x400G[200G]": ["etp1a", "etp1b"]
                            }
                        }
                    }
                },
                {
                    'interfaces': {
                        "Ethernet0": {
                            "default_brkout_mode": "2x400G[200G]",
                            "port_type": "SFP"
                        }
                    }
                },
                None
            ),

            # Case 3: one CPO port
            (
                True,
                {
                    'interfaces': {
                        "Ethernet0": {
                            "index": "1",
                            "lanes": "0",
                            "breakout_modes": {
                                "2x400G[200G]": ["etp1a", "etp1b"]
                            }
                        }
                    }
                },
                {
                    'interfaces': {
                        "Ethernet0": {
                            "default_brkout_mode": "2x400G[200G]",
                            "port_type": "CPO"
                        }
                    }
                },
                [0]
            ),

            # Case 4: multiple CPO ports
            (
                True,
                {
                    'interfaces': {
                        "Ethernet0": {"index": "1"},
                        "Ethernet4": {"index": "5"},
                        "Ethernet8": {"index": "9"}
                    }
                },
                {
                    'interfaces': {
                        "Ethernet0": {"port_type": "CPO"},
                        "Ethernet4": {"port_type": "CPO"},
                        "Ethernet8": {"port_type": "CPO"}
                    }
                },
                [0, 4, 8]
            ),
        ]
    )
    @mock.patch('sonic_py_common.device_info.get_path_to_platform_dir', mock.MagicMock(return_value='.'))
    @mock.patch('sonic_py_common.device_info.get_path_to_hwsku_dir', mock.MagicMock(return_value='/tmp'))
    @mock.patch('sonic_py_common.device_info.get_hwsku', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.utils.load_json_file')
    @mock.patch('os.path.exists')
    def test_extract_cpo_ports_index(self, mock_exists, mock_load_json, mock_exists_value, platform_json, hwsku_json, expected_result):
        mock_exists.return_value = mock_exists_value
        mock_load_json.side_effect = [platform_json, hwsku_json]
        assert utils.extract_cpo_ports_index() == expected_result

    def test_wait_until(self):
        values = []
        assert utils.wait_until(lambda: len(values) == 0, timeout=1)
        assert not utils.wait_until(lambda: len(values) > 0, timeout=1)

        def thread_func(items):
            time.sleep(3)
            items.append(0)

        t = threading.Thread(target=thread_func, args=(values, ))
        t.start()
        assert utils.wait_until(lambda: len(values) > 0, timeout=5)
        t.join()

    def test_load_json_file(self):
        assert utils.load_json_file('some_file') is None

        mock_os_open = mock.mock_open(read_data='')
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            assert utils.load_json_file('some_file') is None

        mock_os_open = mock.mock_open(read_data='{"a": "b"}')
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            data = utils.load_json_file('some_file')
            assert data['a'] == 'b'

    def test_read_key_value_file(self):
        mock_os_open = mock.mock_open(read_data='a:b')
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            assert utils.read_key_value_file('some_file') == {'a':'b'}

        mock_os_open = mock.mock_open(read_data='a=b')
        with mock.patch('sonic_platform.utils.open', mock_os_open):
            assert utils.read_key_value_file('some_file', delimeter='=') == {'a':'b'}
            
    @mock.patch('sonic_platform.utils.time.sleep', mock.MagicMock())
    def test_wait_until_conditions(self):
        conditions = [lambda: True]
        assert utils.wait_until_conditions(conditions, 1)
        conditions = [lambda: False]
        assert not utils.wait_until_conditions(conditions, 1)

    def test_timer(self):
        timer = utils.Timer()
        timer.start()
        mock_cb_1000_run_now = mock.MagicMock()
        mock_cb_1000_run_future = mock.MagicMock()
        mock_cb_1_run_future_once = mock.MagicMock()
        mock_cb_1_run_future_repeat = mock.MagicMock()
        timer.schedule(1000, cb=mock_cb_1000_run_now, repeat=False, run_now=True)
        timer.schedule(1000, cb=mock_cb_1000_run_future, repeat=False, run_now=False)
        timer.schedule(1, cb=mock_cb_1_run_future_once, repeat=False, run_now=False)
        timer.schedule(1, cb=mock_cb_1_run_future_repeat, repeat=True, run_now=False)
        time.sleep(3)
        timer.stop()

        mock_cb_1000_run_now.assert_called_once()
        mock_cb_1000_run_future.assert_not_called()
        mock_cb_1_run_future_once.assert_called_once()
        assert mock_cb_1_run_future_repeat.call_count > 1
