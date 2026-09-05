#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

from unittest.mock import mock_open, patch

import pytest

from sonic_platform import utils


class TestHwMgmtPath:

    def test_joins_under_root(self):
        assert utils.hw_mgmt_path("thermal", "bmc_temp_input") == (
            utils.HW_MANAGEMENT_ROOT + "/thermal/bmc_temp_input"
        )

    def test_no_parts_returns_root(self):
        assert utils.hw_mgmt_path() == utils.HW_MANAGEMENT_ROOT


class TestReadSysfsText:

    def test_returns_stripped_value(self):
        with patch("builtins.open", mock_open(read_data=" hello\n")):
            assert utils.read_sysfs_text("/fake") == "hello"

    def test_returns_default_on_io_error(self):
        with patch("builtins.open", side_effect=OSError("nope")):
            assert utils.read_sysfs_text("/fake", default="x") == "x"

    def test_default_is_none(self):
        with patch("builtins.open", side_effect=OSError("nope")):
            assert utils.read_sysfs_text("/fake") is None


class TestReadSysfsInt:

    def test_parses_decimal(self):
        with patch("builtins.open", mock_open(read_data="42\n")):
            assert utils.read_sysfs_int("/fake") == 42

    def test_parses_hex_with_base_zero(self):
        with patch("builtins.open", mock_open(read_data="0x10")):
            assert utils.read_sysfs_int("/fake") == 16

    def test_respects_explicit_base(self):
        with patch("builtins.open", mock_open(read_data="10")):
            assert utils.read_sysfs_int("/fake", base=16) == 16

    def test_returns_default_on_io_error(self):
        with patch("builtins.open", side_effect=OSError("nope")):
            assert utils.read_sysfs_int("/fake", default=7) == 7

    def test_returns_default_on_value_error(self):
        with patch("builtins.open", mock_open(read_data="not-a-number")):
            assert utils.read_sysfs_int("/fake", default=-1) == -1


class TestReadSysfsFloat:

    def test_parses_value(self):
        with patch("builtins.open", mock_open(read_data="3.5\n")):
            assert utils.read_sysfs_float("/fake") == pytest.approx(3.5)

    def test_returns_default_on_io_error(self):
        with patch("builtins.open", side_effect=OSError("nope")):
            assert utils.read_sysfs_float("/fake", default=1.25) == 1.25

    def test_returns_default_on_value_error(self):
        with patch("builtins.open", mock_open(read_data="garbage")):
            assert utils.read_sysfs_float("/fake") is None


class TestHwmgmtFlagIsSet:

    def test_true_when_nonzero(self):
        with patch("builtins.open", mock_open(read_data="1\n")):
            assert utils.hwmgmt_flag_is_set("bmc", "reset_cpu") is True

    def test_false_when_zero(self):
        with patch("builtins.open", mock_open(read_data="0")):
            assert utils.hwmgmt_flag_is_set("bmc", "reset_cpu") is False

    def test_false_when_missing(self):
        with patch("builtins.open", side_effect=FileNotFoundError):
            assert utils.hwmgmt_flag_is_set("bmc", "reset_cpu") is False

    def test_false_when_unparseable(self):
        with patch("builtins.open", mock_open(read_data="garbage")):
            assert utils.hwmgmt_flag_is_set("bmc", "reset_cpu") is False
