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

import os
from typing import List, Tuple

try:
    from sonic_platform_base.liquid_cooling_base import LiquidCoolingBase
    from sonic_platform.leakage_sensor import LeakageSensor
    from sonic_platform.utils import hw_mgmt_path
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


def _sorted_numeric_subdirs(parent: str) -> List[int]:
    """Return numeric subdirectory names under `parent` as a sorted `int` list."""
    if not os.path.isdir(parent):
        return []
    out = []
    for name in os.listdir(parent):
        if not name.isdigit():
            continue
        if os.path.isdir(os.path.join(parent, name)):
            out.append(int(name))
    out.sort()
    return out


def _discover_leakage_hw_indices() -> List[Tuple[int, int]]:
    """
    Discover `(a2d, channel)` indices under `/var/run/hw-management/leakage`.

    The layout is `leakage/<a2d>/<channel>/` with positive integer path components.
    Returns an ordered list of `(a2d, channel)` pairs for every present channel
    directory. If `leakage` is missing or empty, returns an empty list.
    """
    root = hw_mgmt_path("leakage")
    pairs = []
    for a2d in _sorted_numeric_subdirs(root):
        base = os.path.join(root, str(a2d))
        for channel in _sorted_numeric_subdirs(base):
            pairs.append((a2d, channel))
    return pairs


class LiquidCooling(LiquidCoolingBase):
    def __init__(self):
        sensors = [LeakageSensor(a2d, channel) for a2d, channel in _discover_leakage_hw_indices()]

        profiles_by_type = {}
        for sensor in sensors:
            profile = sensor.get_leak_profile()
            if profile is None:
                continue
            profiles_by_type.setdefault(profile.get_type(), profile)
        super().__init__(len(sensors), sensors, profiles=list(profiles_by_type.values()))
