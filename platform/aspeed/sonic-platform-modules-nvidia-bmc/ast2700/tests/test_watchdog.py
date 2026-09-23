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
from pathlib import Path

from sonic_platform_base.bmc_watchdog import BMCWatchdog
from sonic_platform_base.watchdog_base import WatchdogBase

from sonic_platform.watchdog import Watchdog

# Repo root: tests/ -> ast2700/ -> <pkg>/ -> aspeed/ -> platform/ -> <repo>/
_REPO_ROOT = Path(__file__).resolve().parents[5]
_PLATFORM_JSON = (
    _REPO_ROOT
    / "device/aspeed/arm64-aspeed_nvidia_ast2700_bmc-r0/platform.json"
)


# The daemon's own constants, from
# platform/aspeed/aspeed-platform-services/scripts/hw-watchdog-mgrd.py.
# Asserted as literals rather than imported from the base class, so that a
# sonic-platform-common bump which changed them would fail here instead of
# silently shipping a client that talks to the wrong socket.
DAEMON_SOCKET_PATH = "/run/hw-watchdog-mgrd/hw-watchdog-mgrd.sock"
DAEMON_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"


class TestWatchdog:
    """
    The subclass adds no behaviour, so there is nothing here about arm,
    disarm, is_armed or get_remaining_time -- that is BMCWatchdog's job and
    sonic-platform-common's tests/bmc_watchdog_test.py already covers it.
    What only this repo can check is the wiring and the daemon path contract.
    """

    def test_is_bmc_watchdog_subclass(self):
        wd = Watchdog()
        assert isinstance(wd, BMCWatchdog)
        assert isinstance(wd, WatchdogBase)

    def test_defaults_match_daemon_contract(self):
        wd = Watchdog()
        assert wd.socket_path == DAEMON_SOCKET_PATH
        assert wd.sysfs_path == DAEMON_SYSFS_PATH

    def test_paths_are_overridable(self, tmp_path):
        wd = Watchdog(socket_path="/tmp/wd.sock", sysfs_path=str(tmp_path))
        assert wd.socket_path == "/tmp/wd.sock"
        assert wd.sysfs_path == str(tmp_path)


class TestPlatformJsonWatchdogPolicy:
    """
    Assert that the hw-watchdog-mgrd policy section is correctly placed and
    valued in the device platform.json.

    The daemon reads the top-level "watchdog" key via
    sonic_py_common.device_info.get_platform_json_data().  Misnesting it
    (e.g. inside "chassis") causes the daemon to silently fall back to its
    own defaults, which today happen to be identical — so the misnest would
    be invisible until someone deliberately sets a non-default value.
    This test guards that contract at the point where the JSON is authored.

    If the file is missing this test FAILS (not skips): a skipping test
    asserts nothing about the device tree.
    """

    def _load(self):
        assert _PLATFORM_JSON.is_file(), (
            f"platform.json not found at {_PLATFORM_JSON}; "
            "the device tree must ship this file"
        )
        with _PLATFORM_JSON.open() as fh:
            return json.load(fh)

    def test_watchdog_section_exists_at_top_level(self):
        data = self._load()
        assert "watchdog" in data, (
            '"watchdog" key is missing from the top level of platform.json; '
            "hw-watchdog-mgrd will silently use its own defaults"
        )

    def test_watchdog_not_nested_under_chassis(self):
        data = self._load()
        chassis = data.get("chassis", {})
        assert "watchdog" not in chassis, (
            '"watchdog" is nested inside "chassis" — misnested; '
            "hw-watchdog-mgrd reads only the top-level key"
        )

    def test_watchdog_boot_arm_is_true(self):
        data = self._load()
        wd = data["watchdog"]
        assert wd.get("boot_arm") is True, (
            f'"boot_arm" must be JSON true (bool), got {wd.get("boot_arm")!r}'
        )

    def test_watchdog_shutdown_protect_is_true(self):
        data = self._load()
        wd = data["watchdog"]
        assert wd.get("shutdown_protect") is True, (
            f'"shutdown_protect" must be JSON true (bool), '
            f'got {wd.get("shutdown_protect")!r}'
        )
