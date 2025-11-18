#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Shared test helpers for chassis testing.

This module contains common utilities used by both unit and integration tests
for chassis functionality. It helps avoid code duplication between test files.
"""

import json
import os

from contextlib import contextmanager
from unittest.mock import patch, mock_open


def dirs_of_led_ctrl_lock_path():
    """Returns a list of directories that lead to LED_CTRL_LOCK_PATH.

    FOR USED BY INTEGRATION TEST ONLY.
    """
    from sonic_platform_pddf_base.pddfapi import LED_CTRL_LOCK_PATH

    res = []
    dir = os.path.split(LED_CTRL_LOCK_PATH)[0]
    while dir != "/":
        res.append(dir)
        dir = os.path.split(dir)[0]
    return res


@contextmanager
def setup_patch_for_chassis_init(pddf_plugin_data: dict):
    """A context manager helper to patch open() and os.path.exists() for Chassis.__init__().

    FOR USED BY INTEGRATION TEST ONLY.
    """
    original_open = open
    original_exists = os.path.exists

    DIRS_OF_LED_CTRL_LOCK_PATH = dirs_of_led_ctrl_lock_path()

    # Prepare side effects to satisfy Chassis.__init__().
    def open_side_effect(file, *args, **kwargs):
        if file == "/usr/share/sonic/platform/pddf/pd-plugin.json":  # For PddfChassis.__init__()
            return mock_open(read_data=json.dumps(pddf_plugin_data))()
        elif file == "/usr/share/sonic/platform/pddf/pddf-device.json":  # For PddfApi.__init__().
            return mock_open(
                read_data=json.dumps({"PLATFORM": {}})  # For PddfApi.get_platform().
            )()

        # Fallback to the real open() for other files
        return original_open(file, *args, **kwargs)

    def exists_side_effect(path):
        if path == "/usr/share/sonic/platform":  # For PddfChassis.__init__().
            return True
        elif path in DIRS_OF_LED_CTRL_LOCK_PATH:  # For PddfApi.__init__().
            return True

        # Fallback to the real exists() for other paths
        return original_exists(path)

    with patch("builtins.open", side_effect=open_side_effect), patch(
        "os.path.exists", side_effect=exists_side_effect
    ):
        yield
