#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the xdpe_user_data_version utility's version decoding."""

import importlib.machinery
import importlib.util
import os
import sys

import pytest

from unittest.mock import patch

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True


@pytest.fixture
def xdpe_module():
    """Load the (extension-less) xdpe_user_data_version script as a module."""
    test_dir = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(test_dir, "../../../common/utils/xdpe_user_data_version")
    loader = importlib.machinery.SourceFileLoader("xdpe_user_data_version", script_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    yield module


def _run(module, block_data):
    """Invoke the reader against a fake SMBus; block_data may be an exception to raise."""
    calls = []

    class _FakeBus:
        def __enter__(self):
            return self

        def __exit__(self, *exc_info):
            return False

        def read_block_data(self, addr, register, force=None):
            calls.append((addr, register, force))
            if isinstance(block_data, Exception):
                raise block_data
            return block_data

    with (
        patch.object(module, "SMBus", lambda bus_no: _FakeBus()),
        patch.object(module, "syslog") as mock_syslog,
    ):
        result = module.read_user_data_00(93, 0x70)
    return result, calls, mock_syslog


# The SMBus block returns the 2-byte stamp LSB first, so [0x01, 0x02] is 0x0201.
# IND70 3.2 stores it byte-swapped: high byte revision, low byte item version.
@pytest.mark.parametrize(
    "block_data,expected",
    [
        ([0x01, 0x01], "1.1"),  # swt_dcdc0_rev1_..._v0x01_r0x01 stamps 0101
        ([0x01, 0x02], "1.2"),  # swt_dcdc0_rev2_..._v0x01_r0x02 stamps 0201
        ([0x01, 0x03], "1.3"),  # swt_dcdc0_rev3_..._v0x01_r0x03 stamps 0301
    ],
)
def test_shipped_config_stamps(xdpe_module, block_data, expected):
    assert _run(xdpe_module, block_data)[0] == expected


def test_version_is_the_low_byte(xdpe_module):
    """A version bump moves the low byte, not the high one."""
    assert _run(xdpe_module, [0x02, 0x03])[0] == "2.3"


def test_unprogrammed_stamp_reads_zero(xdpe_module):
    assert _run(xdpe_module, [0x00, 0x00])[0] == "0.0"


def test_read_is_forced_at_the_user_data_register(xdpe_module):
    """The pmbus driver stays bound, so the block read must be forced."""
    _, calls, _ = _run(xdpe_module, [0x01, 0x02])
    assert calls == [(0x70, xdpe_module.USER_DATA_00, True)]


@pytest.mark.parametrize(
    "error",
    [
        OSError("no such device"),  # bus or device missing
        OSError(95, "Operation not supported"),  # adapter without SMBus block read
        ValueError("bad block length"),  # smbus2 rejecting the device's count byte
    ],
)
def test_failed_read_reports_na_and_warns(xdpe_module, error):
    """Any read failure degrades to "N/A" rather than breaking fwutil's output."""
    result, _, mock_syslog = _run(xdpe_module, error)
    assert result == "N/A"
    assert mock_syslog.syslog.call_count == 1


@pytest.mark.parametrize("block_data", [[], [0x01]])
def test_truncated_read_reports_na(xdpe_module, block_data):
    assert _run(xdpe_module, block_data)[0] == "N/A"
