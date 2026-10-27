#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
The 4210-r0021, 4220-r0 and 5010-r0 thermal policies, run through the shared thermal library.

These platforms used to have their own policy types (fandrawer.N.present, fan.set_speed,
fan.set_max_speed and a thermal.control_algo with inline PID settings). The policy files were
rewritten onto the common types; these tests pin down that, for every fan drawer count, the
rewritten files do what the old ones did, and that each platform's PID tuning still drives
the fans.

The unit-test fixtures mock sonic_platform_base, and the thermal manager and type registry
with it. These tests need the real ones, so each test runs inside the fixtures'
real_thermal_control_package(), which loads the real sonic_thermal_control package afresh
from the sonic-platform-common source tree and restores the mocks afterwards.
"""

import importlib
import os
import types

import pytest

from fixtures.mock_imports_unit_tests import SHARED_THERMAL_PACKAGE, real_thermal_control_package

_REPO_ROOT = os.path.normpath(os.path.join(
    os.path.dirname(__file__), "..", "..", "..", "..", "..", ".."))
_DEVICE_DIR = os.path.join(_REPO_ROOT, "device", "nexthop")

FAN_MIN = 30.0  # fan_limits.min in each platform's thermal_config.json
FANS_PER_DRAWER = 2
FAKE_SETPOINT = 80.0
INTERVAL = 5  # top-level "interval" of each policy file

PLATFORMS = ["x86_64-nexthop_4210-r0021", "x86_64-nexthop_4220-r0", "x86_64-nexthop_5010-r0"]

# --- expected behaviour, per drawer count ---------------------------------------------------
#
# Read off the policy files as they were before the migration (e035cf880), with the semantics
# of the platform's old types (5a0e9e5fb^:.../common/sonic_platform/thermal_conditions.py and
# thermal_actions.py):
#   - fandrawer.N.present matched exactly N present drawers;
#   - default.operation matched more than two present drawers (not "always");
#   - fan.set_max_speed stored a per-fan cap that Fan.set_speed clamped to, and the PID read
#     it back as its output ceiling; the cap policy always ran before the PID policy;
#   - fan.set_speed set every fan to the given speed (capped by the stored max).
# So ("fixed", X) means every fan is set to X%, ("pid", Y) means PID control with ceiling Y%,
# and None means no policy matches and nothing touches the fans.
NONE = None
EXPECTED = {
    # 8 drawers. One..Eight: set_max_speed 100; One and Two also set_speed 100.
    "x86_64-nexthop_4210-r0021": {
        0: NONE,
        1: ("fixed", 100), 2: ("fixed", 100),
        3: ("pid", 100), 4: ("pid", 100), 5: ("pid", 100), 6: ("pid", 100),
        7: ("pid", 100), 8: ("pid", 100),
    },
    # 4 drawers. One: 100/100, Two: 95/95, Three: max 80, Four: max 75.
    "x86_64-nexthop_4220-r0": {
        0: NONE,
        1: ("fixed", 100), 2: ("fixed", 95),
        3: ("pid", 80), 4: ("pid", 75),
    },
    # 4 drawers. One and Two: 100/100, Three and Four: max 100.
    "x86_64-nexthop_5010-r0": {
        0: NONE,
        1: ("fixed", 100), 2: ("fixed", 100),
        3: ("pid", 100), 4: ("pid", 100),
    },
}


# --- the real library -----------------------------------------------------------------------

@pytest.fixture
def lib():
    """The real sonic_thermal_control modules, freshly imported so each test has its own
    type registry and manager state."""
    with real_thermal_control_package():
        yield types.SimpleNamespace(
            manager_base=importlib.import_module(SHARED_THERMAL_PACKAGE + ".pid_thermal_manager_base"),
            actions=importlib.import_module(SHARED_THERMAL_PACKAGE + ".common_actions"),
            config=importlib.import_module(SHARED_THERMAL_PACKAGE + ".thermal_config"),
        )


class _Clock:
    """time.monotonic stand-in that advances one policy interval per call."""

    def __init__(self):
        self.now = 1000.0

    def monotonic(self):
        self.now += INTERVAL
        return self.now


@pytest.fixture
def load_platform(lib, monkeypatch):
    """Load a platform's policy file the way thermalctld does, with its thermal_config.json
    read from the device directory instead of /usr/share/sonic/platform."""
    monkeypatch.setattr(lib.actions, "time", _Clock())

    def load(platform):
        platform_dir = os.path.join(_DEVICE_DIR, platform)
        monkeypatch.setattr(lib.config, "DEFAULT_CONFIG_DIR", platform_dir)

        class Manager(lib.manager_base.PidThermalManagerBase):
            _policy_dict = {}
            _thermal_info_dict = {}

        Manager.load(os.path.join(platform_dir, "thermal_policy.json"))
        return Manager

    return load


# --- fake hardware --------------------------------------------------------------------------

class FakeFan:
    def __init__(self):
        self.speeds = []

    def set_speed(self, speed):
        self.speeds.append(speed)
        return True


class FakeDrawer:
    def __init__(self, present):
        self._present = present
        self.fans = [FakeFan() for _ in range(FANS_PER_DRAWER)]

    def get_presence(self):
        return self._present


class FakeThermal:
    def __init__(self, domain, temperature, setpoint=FAKE_SETPOINT):
        self._domain = domain
        self.temperature = temperature
        self._setpoint = setpoint

    def get_name(self):
        return "fake-{}".format(self._domain)

    def get_pid_domain(self):
        return self._domain

    def get_pid_setpoint(self):
        return self._setpoint

    def get_temperature(self):
        return self.temperature

    def get_high_threshold(self):
        return None

    def get_high_critical_threshold(self):
        return None


class FakeChassis:
    def __init__(self, num_drawers, num_present, thermals, manager):
        self.drawers = [FakeDrawer(i < num_present) for i in range(num_drawers)]
        self.thermals = thermals
        self._manager = manager

    def get_all_fan_drawers(self):
        return self.drawers

    def get_all_fans(self):
        return [fan for drawer in self.drawers for fan in drawer.fans]

    def get_all_thermals(self):
        return self.thermals

    def get_all_sfps(self):
        return []

    def get_all_psus(self):
        return []

    def get_thermal_manager(self):
        return self._manager


def _num_drawers(platform):
    return max(EXPECTED[platform])


def _domains(lib, platform):
    config = lib.config.load_thermal_config(os.path.join(_DEVICE_DIR, platform))
    return sorted(lib.config.get_pid_domains(config))


def _speeds_after_one_pass(load_platform, lib, platform, present, temperature):
    """Run one policy pass with every configured domain at the given temperature and return
    the set of speeds the fans were last set to (empty if nothing touched them)."""
    manager = load_platform(platform)
    thermals = [FakeThermal(domain, temperature) for domain in _domains(lib, platform)]
    chassis = FakeChassis(_num_drawers(platform), present, thermals, manager)
    manager.run_policy(chassis)
    fans = chassis.get_all_fans()
    if not any(fan.speeds for fan in fans):
        return set()
    return {fan.speeds[-1] for fan in fans}


# --- equivalence with the old policy files --------------------------------------------------

CASES = [(platform, present, expected)
         for platform, table in EXPECTED.items()
         for present, expected in table.items()]


@pytest.mark.parametrize("platform,present,expected", CASES,
                         ids=["{}-{}".format(p.split("_")[-1], n) for p, n, _ in CASES])
def test_drawer_count_gives_old_outcome(load_platform, lib, platform, present, expected):
    """
    A fixed speed ignores temperature; PID runs the fans at its ceiling when far too hot and
    at the fan minimum when far too cold. Running both extremes tells the three apart.
    """
    # Given the platform's policy with `present` of its fan drawers present
    # When one pass runs with every PID domain far above, then far below, its setpoint
    hot = _speeds_after_one_pass(load_platform, lib, platform, present, FAKE_SETPOINT + 50)
    cold = _speeds_after_one_pass(load_platform, lib, platform, present, FAKE_SETPOINT - 50)

    # Then the fans end up where the old policy put them
    if expected is NONE:
        assert hot == set() and cold == set(), "no policy should match"
        return
    kind, speed = expected
    if kind == "fixed":
        assert hot == {speed} and cold == {speed}
    else:
        assert hot == {speed}, "PID should saturate at its ceiling"
        assert cold == {FAN_MIN}, "PID should fall to the fan minimum"


# --- PID tuning smoke test ------------------------------------------------------------------

PASSES = 6


def _run_passes(load_platform, lib, platform, temperatures):
    """Run several passes with all drawers present and return the fan speed after each."""
    manager = load_platform(platform)
    thermals = [FakeThermal(domain, temperatures[domain]) for domain in _domains(lib, platform)]
    chassis = FakeChassis(_num_drawers(platform), _num_drawers(platform), thermals, manager)
    history = []
    for _ in range(PASSES):
        manager.run_policy(chassis)
        speeds = {fan.speeds[-1] for fan in chassis.get_all_fans()}
        assert len(speeds) == 1, "every fan should get the same speed"
        history.append(speeds.pop())
    return history


@pytest.mark.parametrize("platform", PLATFORMS)
def test_pid_settles_at_setpoint(load_platform, lib, platform):
    # Given all fan drawers present and every PID domain exactly at its setpoint
    temperatures = {domain: FAKE_SETPOINT for domain in _domains(lib, platform)}

    # When several passes run
    history = _run_passes(load_platform, lib, platform, temperatures)

    # Then the fans hold one speed, above the minimum
    assert len(set(history)) == 1
    assert history[0] > FAN_MIN


@pytest.mark.parametrize("platform", PLATFORMS)
def test_pid_rises_when_each_domain_runs_hot(load_platform, lib, platform):
    domains = _domains(lib, platform)
    assert domains, "the platform must configure at least one PID domain"
    settled = _run_passes(load_platform, lib, platform,
                          {domain: FAKE_SETPOINT for domain in domains})[-1]
    for hot_domain in domains:
        # Given one domain a little above its setpoint and the rest at theirs
        temperatures = {domain: FAKE_SETPOINT for domain in domains}
        temperatures[hot_domain] = FAKE_SETPOINT + 2

        # When several passes run
        history = _run_passes(load_platform, lib, platform, temperatures)

        # Then that domain alone pushes the fans above the settled speed, and keeps
        # pushing while it stays hot
        assert history[0] > settled, hot_domain
        assert history == sorted(history), hot_domain
        assert history[-1] > history[0], hot_domain
