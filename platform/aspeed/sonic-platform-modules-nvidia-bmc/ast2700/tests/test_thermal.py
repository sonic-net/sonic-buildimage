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

from unittest.mock import patch

import pytest

from sonic_platform import thermal as thermal_mod
from sonic_platform.thermal import ThermalBMC


class TestThermalBMC:

    def test_get_name(self):
        t = ThermalBMC()
        assert t.get_name() == "BMC Ambient"

    def test_sysfs_paths_rooted_under_hw_management(self):
        t = ThermalBMC()
        assert t.input_path.startswith(ThermalBMC.HW_MGMT_ROOT)
        assert t.max_path.startswith(ThermalBMC.HW_MGMT_ROOT)
        assert t.min_path.startswith(ThermalBMC.HW_MGMT_ROOT)
        assert t.input_path.endswith("bmc_temp_input")
        assert t.max_path.endswith("bmc_temp")
        assert t.min_path.endswith("bmc_min")

    @patch.object(thermal_mod, "read_sysfs_float", return_value=37125.0)
    def test_get_temperature_scales_milli_celsius(self, read_float):
        t = ThermalBMC()
        assert t.get_temperature() == pytest.approx(37.125)
        read_float.assert_called_with(t.input_path)

    @patch.object(thermal_mod, "read_sysfs_float", return_value=90000.0)
    def test_get_high_threshold_uses_max_path(self, read_float):
        t = ThermalBMC()
        assert t.get_high_threshold() == pytest.approx(90.0)
        read_float.assert_called_with(t.max_path)

    @patch.object(thermal_mod, "read_sysfs_float", return_value=5000.0)
    def test_get_low_threshold_uses_min_path(self, read_float):
        t = ThermalBMC()
        assert t.get_low_threshold() == pytest.approx(5.0)
        read_float.assert_called_with(t.min_path)

    def test_high_low_critical_thresholds_return_na(self):
        t = ThermalBMC()
        assert t.get_high_critical_threshold() == "N/A"
        assert t.get_low_critical_threshold() == "N/A"
