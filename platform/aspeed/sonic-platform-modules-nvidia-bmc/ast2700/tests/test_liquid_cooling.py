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

from unittest.mock import MagicMock, patch

import pytest

from sonic_platform_base.liquid_cooling_base import LiquidCoolingBase
from sonic_platform import liquid_cooling as lc_mod
from sonic_platform.liquid_cooling import LiquidCooling


def _fake_sensor(profile_type):
    """Build a stand-in for `LeakageSensor` with the given profile type."""
    sensor = MagicMock()
    profile = MagicMock()
    profile.get_type.return_value = profile_type
    sensor.get_leak_profile.return_value = profile
    return sensor, profile


class TestSortedNumericSubdirs:

    def test_returns_empty_for_missing_dir(self):
        with patch("os.path.isdir", return_value=False):
            assert lc_mod._sorted_numeric_subdirs("/nope") == []

    def test_returns_sorted_ints_skipping_non_numeric(self):
        names = ["2", "10", "1", "skip", ".hidden", "3"]
        is_dir_for = {"/p/" + n for n in names if n.isdigit()}
        with patch("os.path.isdir", side_effect=lambda p: p == "/p" or p in is_dir_for), \
             patch("os.listdir", return_value=names):
            assert lc_mod._sorted_numeric_subdirs("/p") == [1, 2, 3, 10]

    def test_skips_files_named_as_integers(self):
        # A regular file whose name is a digit must not be picked up.
        with patch("os.path.isdir", side_effect=lambda p: p == "/p"), \
             patch("os.listdir", return_value=["1", "2"]):
            assert lc_mod._sorted_numeric_subdirs("/p") == []


class TestDiscoverLeakageHwIndices:

    def test_empty_when_root_missing(self):
        with patch.object(lc_mod, "_sorted_numeric_subdirs", return_value=[]):
            assert lc_mod._discover_leakage_hw_indices() == []

    def test_enumerates_a2d_and_channels(self):
        def fake_subdirs(parent):
            # Layout: two a2d directories at the root; the first has two channels,
            # the second has one.
            if parent.endswith("/leakage"):
                return [0, 1]
            if parent.endswith("/leakage/0"):
                return [0, 1]
            if parent.endswith("/leakage/1"):
                return [2]
            return []

        with patch.object(lc_mod, "_sorted_numeric_subdirs", side_effect=fake_subdirs):
            pairs = lc_mod._discover_leakage_hw_indices()

        assert pairs == [(0, 0), (0, 1), (1, 2)]


class TestLiquidCoolingConstruction:

    def test_no_sensors_when_discovery_empty(self):
        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[]):
            lc = LiquidCooling()
        assert isinstance(lc, LiquidCoolingBase)
        assert lc.get_num_leak_sensors() == 0
        assert lc.get_all_profiles() == []

    def test_registers_one_profile_per_distinct_type(self):
        s1, p_cond_a = _fake_sensor("conductive")
        s2, p_cond_b = _fake_sensor("conductive")
        s3, p_opt = _fake_sensor("optical")

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (0, 1), (1, 0)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s1, s2, s3]):
            lc = LiquidCooling()

        assert lc.get_num_leak_sensors() == 3
        profiles = lc.get_all_profiles()
        # One profile per distinct type; the first sensor of that type wins.
        types = {p.get_type() for p in profiles}
        assert types == {"conductive", "optical"}
        assert p_cond_a in profiles
        assert p_opt in profiles
        assert p_cond_b not in profiles

    def test_skips_sensors_returning_no_profile(self):
        s_with, profile = _fake_sensor("conductive")
        s_without = MagicMock()
        s_without.get_leak_profile.return_value = None

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (0, 1)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s_with, s_without]):
            lc = LiquidCooling()

        # Both sensors are kept, but only one profile is registered.
        assert lc.get_num_leak_sensors() == 2
        assert lc.get_all_profiles() == [profile]

    def test_sensors_are_passed_to_base(self):
        s1, _ = _fake_sensor("conductive")
        s2, _ = _fake_sensor("conductive")

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (1, 0)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s1, s2]):
            lc = LiquidCooling()

        assert lc.get_all_leak_sensors() == [s1, s2]
