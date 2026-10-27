#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Loads every Nexthop device thermal_policy.json through the real shared thermal library.

thermalctld loads the policy file once at start; if any type in it is unknown, load() raises
and no policy runs. The other unit tests mock the policy type registry, so only this test
notices a policy file that the library can no longer load.
python -m pytest test/unit/sonic_platform/test_thermal_policy_files.py -v
"""

import glob
import importlib
import os
from unittest.mock import patch

import pytest
from fixtures.mock_imports_unit_tests import SHARED_THERMAL_PACKAGE, real_thermal_control_package

REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), *[".."] * 6))
DEVICE_DIR = os.path.join(REPO_ROOT, "device", "nexthop")

def _policy_file_params():
    params = []
    for path in sorted(glob.glob(os.path.join(DEVICE_DIR, "*", "thermal_policy.json"))):
        platform = os.path.basename(os.path.dirname(path))
        params.append(pytest.param(path, id=platform))
    return params


@pytest.mark.parametrize("policy_file", _policy_file_params())
def test_device_thermal_policy_loads(policy_file):
    # Given the platform's policy file, with thermal.control_algo reading the
    # thermal_config.json beside it as it reads /usr/share/sonic/platform on the switch,
    # and a thermal manager with no state from any other case
    with real_thermal_control_package():
        manager_base = importlib.import_module(SHARED_THERMAL_PACKAGE + ".pid_thermal_manager_base")
        common_actions = importlib.import_module(SHARED_THERMAL_PACKAGE + ".common_actions")
        thermal_config = importlib.import_module(SHARED_THERMAL_PACKAGE + ".thermal_config")
        manager = type("PolicyFileThermalManager", (manager_base.PidThermalManagerBase,), {
            "_policy_dict": {},
            "_thermal_info_dict": {},
        })

        # When thermalctld would load it
        with patch.object(thermal_config, "DEFAULT_CONFIG_DIR", os.path.dirname(policy_file)):
            manager.load(policy_file)

        # Then it loads, has policies, and every PID action found its PID domains
        assert manager._load_error is None
        assert manager._policy_dict, "no policies loaded from {}".format(policy_file)
        for policy in manager._policy_dict.values():
            for action in policy.actions.values():
                if isinstance(action, common_actions.ThermalControlAlgorithmAction):
                    assert action._degraded is None, \
                        "policy [{}]: {}".format(policy.name, action._degraded)
