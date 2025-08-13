#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import pytest
import sys
import os

from unittest.mock import Mock

@pytest.fixture
def pid_params():
    return {
        "dt": 5,
        "setpoint": 85,
        "Kp": 1.0,
        "Ki": 1.0,
        "Kd": 1.0,
        "min_speed": 30,
        "max_speed": 100
    }

@pytest.fixture
def pid_controller():
    pid_params = {
        "dt": 5,
        "setpoint": 85,
        "Kp": 1.0,
        "Ki": 1.0,
        "Kd": 1.0,
        "min_speed": 30,
        "max_speed": 100
    }
    # Mock out unecessary imports
    sys.modules['sonic_platform_base.sonic_thermal_control.thermal_action_base'] = Mock()
    sys.modules['sonic_platform_base.sonic_thermal_control.thermal_json_object'] = Mock()
    # Import the module under test
    cwd = os.path.dirname(os.path.realpath(__file__))
    sys.path.append(os.path.join(cwd, "../"))
    from thermal_actions import FanPIDController
    return FanPIDController(**pid_params)

def test_pid_controller(pid_controller):
    pid_input_expect = [(85, 30), (86, 36.2), (84, 34.8), (90, 66), (70, 30)]
    for input, expected in pid_input_expect:
        output = pid_controller.compute_speed(input)
        assert abs(expected - output) < 0.1