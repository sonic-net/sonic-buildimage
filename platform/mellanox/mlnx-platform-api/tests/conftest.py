#
# Copyright (c) 2021-2023 NVIDIA CORPORATION & AFFILIATES.
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

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

os.environ["PLATFORM_API_UNIT_TESTING"] = "1"

from sonic_platform import utils

@pytest.fixture(scope='function', autouse=True)
def auto_recover_mock():
    """Auto used fixture to recover some critical mocked functions
    """
    origin_os_path_exists = os.path.exists
    origin_read_int_from_file = utils.read_int_from_file
    origin_read_str_from_file = utils.read_str_from_file
    origin_read_float_from_file = utils.read_float_from_file
    origin_write_file = utils.write_file
    yield
    os.path.exists = origin_os_path_exists
    utils.read_int_from_file = origin_read_int_from_file
    utils.read_str_from_file = origin_read_str_from_file
    utils.write_file = origin_write_file
    utils.read_float_from_file = origin_read_float_from_file
