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

import json
import os
from pathlib import Path
import subprocess
import sys
import time

import pytest

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
PKG_PARENT_DIR = os.path.dirname(TESTS_DIR)

# Make the top-level `sonic_platform` package importable from source.
if PKG_PARENT_DIR not in sys.path:
    sys.path.insert(0, PKG_PARENT_DIR)

from sonic_platform import leakage_sensor as leakage_mod  # noqa: E402
from sonic_platform import liquid_cooling as lc_mod  # noqa: E402
from sonic_platform import utils as platform_utils  # noqa: E402
from sonic_platform.leakage_sensor import LeakageSensor  # noqa: E402
from sonic_platform_base.leakage_sensor_test_base import LeakageSensorTestBase  # noqa: E402
from sonic_platform_base.liquid_cooling_base import LeakSeverity  # noqa: E402


class LeakTree:
    def __init__(self, root, store, cooling):
        self.root = root
        self.store = store
        self.cooling = cooling

    def poll(self, sensor):
        return (sensor.is_leak_sensor_ok(), sensor.is_leak(),
                sensor.get_leak_severity(), sensor.is_test_leak())

    def interface(self, cooling=None):
        cooling = self.cooling if cooling is None else cooling
        interface = cooling.get_leak_sensor_test()
        assert isinstance(interface, LeakageSensorTestBase), (
            "get_leak_sensor_test() must return the leak test interface"
        )
        return interface

    def input_file(self, a2d=1, channel=1):
        return self.root / "leakage" / str(a2d) / str(channel) / "input"

    def child(self, action, *, name="supply", enable=True, severity="CRITICAL", pause=False):
        """Start a second interpreter with its own sensors. It shares only files with this one."""
        root, store = self.root, self.store
        return subprocess.Popen(
            [sys.executable, str(Path(__file__).with_name("leak_child.py")),
             str(root), str(store), json.dumps([action, name, enable, severity, pause])],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            env={**os.environ, "PYTHONPATH": os.pathsep.join(
                [PKG_PARENT_DIR, os.environ.get("PYTHONPATH", "")])})

    def child_result(self, child):
        try:
            stdout, stderr = child.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            child.kill()
            child.communicate()
            pytest.fail("sensor child timed out")
        assert child.returncode == 0, stderr
        return json.loads(stdout)

    def wait_for_replace(self, child):
        store = self.store
        ready = Path(str(store) + ".ready")
        deadline = time.monotonic() + 10
        # The child writes the path into the ready file after it creates the
        # file: wait for the content, not only for the file.
        while not (ready.exists() and ready.read_text()) and child.poll() is None and time.monotonic() < deadline:
            time.sleep(0.01)
        assert ready.exists() and ready.read_text(), "writer must reach the atomic marker replacement"
        temporary = Path(ready.read_text())
        assert temporary.parent == store
        assert temporary.read_text().strip() == LeakSeverity.CRITICAL.value
        return temporary


PRODUCTION_LEAK_STORE = leakage_mod.LEAK_TEST_DIR


@pytest.fixture
def production_leak_store():
    """The real LEAK_TEST_DIR, read before the tests redirect it."""
    return PRODUCTION_LEAK_STORE


@pytest.fixture(autouse=True)
def isolated_leak_store(tmp_path, monkeypatch):
    """Point LEAK_TEST_DIR at a temporary directory in every test."""
    monkeypatch.setattr(leakage_mod, "LEAK_TEST_DIR", str(tmp_path / "leak_sim"))


@pytest.fixture
def leak_tree(tmp_path, monkeypatch):
    root = tmp_path / "hardware"
    for a2d, channel, name in [(1, 1, "supply"), (1, 2, "return"), (2, 4, "tray")]:
        directory = root / "leakage" / str(a2d) / str(channel)
        directory.mkdir(parents=True)
        attributes = dict(scale=1, lcrit=0, crit=10, lwarn=20, warn=30,
                          min=40, max=100, input=50, channel_name=name, type="rop")
        for key, value in attributes.items():
            (directory / key).write_text(str(value))
    parent = tmp_path / "runtime"
    parent.mkdir()
    store = parent / "leak_sim"
    monkeypatch.setattr(platform_utils, "HW_MANAGEMENT_ROOT", str(root))
    monkeypatch.setattr(LeakageSensor, "LEAKAGE_CHANNEL_DIR",
                        str(root / "leakage/{a2d_index}/{channel_index}"))
    monkeypatch.setattr(leakage_mod, "LEAK_TEST_DIR", str(store))
    cooling = lc_mod.LiquidCooling()
    assert [s.get_name() for s in cooling.get_all_leak_sensors()] == ["supply", "return", "tray"]
    return LeakTree(root, store, cooling)
