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
import tempfile
from pathlib import Path
from typing import List, Optional, Tuple

try:
    from sonic_platform_base.leakage_sensor_test_base import LeakageSensorTestBase
    from sonic_platform_base.liquid_cooling_base import LeakSeverity, LiquidCoolingBase
    from sonic_platform import leakage_sensor
    from sonic_platform.leakage_sensor import LeakageSensor
    from sonic_platform.utils import hw_mgmt_path
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()


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


def _store_test_leak(sensor: LeakageSensor, severity: Optional[LeakSeverity]):
    """
    Write the marker of `sensor` with `severity`, or remove it if `severity`
    is None. The marker is replaced atomically by a rename. Raises OSError.
    """
    marker, store = sensor._test_leak_marker(), leakage_sensor.LEAK_TEST_DIR
    if severity is None:
        Path(marker).unlink(missing_ok=True)
        return
    if sensor._get_test_leak() == severity:
        return
    # Create only the leaf, never /run/bmc itself.
    Path(store).mkdir(0o755, exist_ok=True)
    os.chmod(store, 0o755)  # override the umask
    fd, tmp = tempfile.mkstemp(dir=store)
    try:
        with os.fdopen(fd, "w") as tmp_file:
            tmp_file.write(severity.value)
        os.replace(tmp, marker)
    except BaseException:
        os.remove(tmp)
        raise


class LeakageSensorTest(LeakageSensorTestBase):
    """
    Leak test interface of the NVIDIA BMC. Test leaks are marker files that
    each LeakageSensor reads. Every request is logged to syslog.
    """

    def __init__(self, sensors):
        super().__init__()
        names = [sensor.get_name() for sensor in sensors]
        # A name shared by two channels is ambiguous and cannot be addressed.
        self._sensors = {name: sensor for name, sensor in zip(names, sensors) if names.count(name) == 1}

    def set_test_leak(self, sensor_name, enable, *, severity=LeakSeverity.MINOR):
        request = f"inject severity={getattr(severity, 'value', severity)}" if enable else "withdraw"
        sensor = self._sensors.get(sensor_name)
        if sensor is None:
            logger.log_notice(f"leak test: sensor {sensor_name!r}: {request}: refused, no single sensor has this name")
            return False
        try:
            _store_test_leak(sensor, LeakSeverity(severity) if enable else None)
        except (OSError, ValueError) as exc:
            logger.log_notice(f"leak test: sensor {sensor_name}: {request}: failed: {exc}")
            return False
        logger.log_notice(f"leak test: sensor {sensor_name}: {request}: done")
        return True

    def is_test_leak_enabled(self, sensor_name):
        sensor = self._sensors.get(sensor_name)
        return sensor is not None and sensor._get_test_leak() is not None

    def clear_test_leaks(self):
        """Remove every marker, including files that belong to no sensor."""
        store = leakage_sensor.LEAK_TEST_DIR
        try:
            for name in os.listdir(store):
                try:
                    os.remove(os.path.join(store, name))
                except OSError:
                    pass
            cleared = not os.listdir(store)
        except FileNotFoundError:
            cleared = True
        except OSError:
            cleared = False
        result = "done" if cleared else "failed, the store is not empty"
        logger.log_notice(f"leak test: withdraw all: {result}")
        return cleared


class LiquidCooling(LiquidCoolingBase):
    def __init__(self):
        sensors = [LeakageSensor(a2d, channel) for a2d, channel in _discover_leakage_hw_indices()]
        self._leak_test = LeakageSensorTest(sensors)

        profiles_by_type = {}
        for sensor in sensors:
            profile = sensor.get_leak_profile()
            if profile is None:
                continue
            profiles_by_type.setdefault(profile.get_type(), profile)
        super().__init__(len(sensors), sensors, profiles=list(profiles_by_type.values()))

    def get_leak_sensor_test(self):
        return self._leak_test
