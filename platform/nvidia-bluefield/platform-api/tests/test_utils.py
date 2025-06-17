#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
from sonic_platform import utils

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)


class TestUtils:
    def test_read_file(self):
        ret = utils.read_float_from_file('not exist', 3.14)
        assert ret == 3.14

        with pytest.raises(IOError):
            ret = utils.read_float_from_file('not exist', 2.25, raise_exception=True)
            assert ret == 2.25
        file_path = '/tmp/test.txt'
        utils.write_file(file_path, '3.09')
        assert utils.read_float_from_file(file_path) == 3.09
        utils.write_file(file_path, '3.00')
        assert utils.read_float_from_file(file_path) == 3

    def test_write_file(self):
        file_path = '/tmp/test.txt'
        utils.write_file(file_path, '3.14')
        assert utils.read_float_from_file(file_path) == 3.14

        with pytest.raises(IOError):
            utils.write_file('/not/exist/file', '123', raise_exception=True)
