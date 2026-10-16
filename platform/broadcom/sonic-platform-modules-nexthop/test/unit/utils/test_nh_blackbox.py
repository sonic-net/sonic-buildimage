#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test script for nh_blackbox utility.
This script sets up the necessary mocks and imports to test the CLI tool.
"""

import importlib
import os
import pytest
import textwrap
import sys

from click.testing import CliRunner
from fixtures.test_helpers_common import mock_pddf_data
from unittest.mock import patch, MagicMock

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True


@pytest.fixture
def nh_blackbox_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    # For files without .py extension, we need to use SourceFileLoader explicitly
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    nh_blackbox_path = os.path.join(TEST_DIR, "../../../common/utils/nh_blackbox")
    loader = importlib.machinery.SourceFileLoader("nh_blackbox", nh_blackbox_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    nh_blackbox_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(nh_blackbox_module)

    yield nh_blackbox_module


@pytest.fixture
def dcdc_module():
    """Loads the dcdc module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import dcdc

    yield dcdc


def _make_dcdcs(dcdc_module):
    """Build two real Dcdc instances without touching the host's /sys."""
    pddf_data = mock_pddf_data(
        {
            "DCDC0": {"i2c": {"topo_info": {"parent_bus": "0x5b", "dev_addr": "0x70", "dev_type": "xdpe1a2g5b"}}},
            "DCDC1": {"i2c": {"topo_info": {"parent_bus": "0x5c", "dev_addr": "0x60", "dev_type": "nh_raa228234"}}},
            "PLATFORM": {"num_dcdcs": 2},
        },
    )
    with patch("sonic_platform.dcdc.resolve_i2c_hwmon_paths", return_value=[]):
        return (
            dcdc_module.Xdpe1a2g5bDcdc(0, pddf_data, {"DCDC": {}}),
            dcdc_module.Raa228234Dcdc(1, pddf_data, {"DCDC": {}}),
        )


def _patch_read_statuses_deps(nh_blackbox_module, mock_chassis):
    """Shared mock setup for the dcdc read-statuses CLI tests."""
    return (
        patch.object(
            nh_blackbox_module, "check_root_privileges", autospec=True, return_value=None
        ),
        patch.object(
            nh_blackbox_module, "Chassis", return_value=mock_chassis
        ),
    )


def _mock_chassis_with_dcdcs(statuses):
    """Wraps a {dcdc: DcdcStatusRecord} mapping in a chassis whose _dcdc_list
    yields each record via read_statuses_via_debugfs()."""
    for dcdc, record in statuses.items():
        dcdc.read_statuses_via_debugfs = lambda record=record: record
    chassis = MagicMock()
    chassis._dcdc_list = list(statuses.keys())
    return chassis


def _build_dcdc_statuses(dcdc_module, dcdc0, dcdc1):
    """Shared two-DCDC two-rail input for the raw and verbose CLI tests."""
    return {
        dcdc0: dcdc_module.DcdcStatusRecord(
            name="DCDC0",
            dev_type="xdpe1a2g5b",
            global_statuses={"STATUS_CML": 0x02},
            rail_statuses=[
                {"STATUS_WORD": 0x0801, "STATUS_OTHER": 0x01},
                {"STATUS_WORD": 0x0001, "STATUS_VOUT": 0x80},
            ],
            global_statuses_decoded={"STATUS_CML": ["OTHER_COMMUNICATION_FAULT"]},
            rail_statuses_decoded=[
                {
                    "STATUS_WORD": ["POWER_BAD", "OTHER_STATUS_CHANGE"],
                    "STATUS_OTHER": ["ASSERTED_SMBALERT#"],
                },
                {"STATUS_WORD": ["OTHER_STATUS_CHANGE"], "STATUS_VOUT": ["VOUT_OV_FAULT"]},
            ],
        ),
        dcdc1: dcdc_module.DcdcStatusRecord(
            name="DCDC1",
            dev_type="nh_raa228234",
            global_statuses={},
            # Rail 0: bits 10 (FAN_FAULT) and 9 (OTHER_FAULT) are masked out by
            # nh_raa228234 -> only bit 0 (OTHER_STATUS_CHANGE) decodes.
            # Rail 1: bit 3 (VIN_UV_FAULT) is in the mask, so it decodes.
            rail_statuses=[
                {"STATUS_WORD": 0x0601},
                {"STATUS_WORD": 0x0008},
            ],
            global_statuses_decoded={},
            rail_statuses_decoded=[
                {"STATUS_WORD": ["OTHER_STATUS_CHANGE"]},
                {"STATUS_WORD": ["VIN_UV_FAULT"]},
            ],
        ),
    }


def test_read_statuses_raw(dcdc_module, nh_blackbox_module):
    # Given
    dcdc0, dcdc1 = _make_dcdcs(dcdc_module)
    statuses = _build_dcdc_statuses(dcdc_module, dcdc0, dcdc1)

    mock_chassis = _mock_chassis_with_dcdcs(statuses)

    p_root, p_chassis = _patch_read_statuses_deps(nh_blackbox_module, mock_chassis)
    with p_root, p_chassis:
        # When
        result = CliRunner().invoke(nh_blackbox_module.blackbox, ["dcdc", "read-statuses"])

    # Then
    assert result.exit_code == 0, result.output
    assert result.output == textwrap.dedent(
        """\
        DCDC0 (xdpe1a2g5b):
        global: [STATUS_CML] 0b0000_0010
        rail 0: [STATUS_WORD] 0b0000_1000_0000_0001, [STATUS_OTHER] 0b0000_0001
        rail 1: [STATUS_WORD] 0b0000_0000_0000_0001, [STATUS_VOUT] 0b1000_0000

        DCDC1 (raa228234):
        rail 0: [STATUS_WORD] 0b0000_0110_0000_0001
        rail 1: [STATUS_WORD] 0b0000_0000_0000_1000

        """
    )


def test_read_statuses_verbose(dcdc_module, nh_blackbox_module):
    # Given
    dcdc0, dcdc1 = _make_dcdcs(dcdc_module)
    statuses = _build_dcdc_statuses(dcdc_module, dcdc0, dcdc1)

    mock_chassis = _mock_chassis_with_dcdcs(statuses)

    p_root, p_chassis = _patch_read_statuses_deps(nh_blackbox_module, mock_chassis)
    with p_root, p_chassis:
        # When
        result = CliRunner().invoke(nh_blackbox_module.blackbox, ["dcdc", "read-statuses", "-v"])

    # Then
    assert result.exit_code == 0, result.output
    assert result.output == textwrap.dedent(
        """\
        DCDC0 (xdpe1a2g5b):
        global: [STATUS_CML] 0b0000_0010 -> OTHER_COMMUNICATION_FAULT
        rail 0: [STATUS_WORD] 0b0000_1000_0000_0001 -> POWER_BAD, OTHER_STATUS_CHANGE
                [STATUS_OTHER] 0b0000_0001 -> ASSERTED_SMBALERT#
        rail 1: [STATUS_WORD] 0b0000_0000_0000_0001 -> OTHER_STATUS_CHANGE
                [STATUS_VOUT] 0b1000_0000 -> VOUT_OV_FAULT

        DCDC1 (raa228234):
        rail 0: [STATUS_WORD] 0b0000_0110_0000_0001 -> OTHER_STATUS_CHANGE
        rail 1: [STATUS_WORD] 0b0000_0000_0000_1000 -> VIN_UV_FAULT

        """
    )


def test_read_statuses_no_faults(dcdc_module, nh_blackbox_module):
    """When no bits decode, the per-device blocks are suppressed and a single
    'no active statuses' line is emitted instead."""
    # Given
    dcdc0, _ = _make_dcdcs(dcdc_module)
    statuses = {
        dcdc0: dcdc_module.DcdcStatusRecord(
            name="DCDC0",
            dev_type="xdpe1a2g5b",
            global_statuses={"STATUS_CML": 0x00},
            rail_statuses=[{"STATUS_WORD": 0x0000, "STATUS_OTHER": 0x00}],
            global_statuses_decoded={"STATUS_CML": []},
            rail_statuses_decoded=[{"STATUS_WORD": [], "STATUS_OTHER": []}],
        ),
    }

    mock_chassis = _mock_chassis_with_dcdcs(statuses)

    p_root, p_chassis = _patch_read_statuses_deps(nh_blackbox_module, mock_chassis)
    with p_root, p_chassis:
        # When
        result = CliRunner().invoke(nh_blackbox_module.blackbox, ["dcdc", "read-statuses", "-v"])

    # Then
    assert result.exit_code == 0, result.output
    assert result.output == "No active DCDC statuses found\n"


def test_read_statuses_empty_leading_rail_preserves_index(dcdc_module, nh_blackbox_module):
    """An empty rail emits no line, and a populated later rail keeps its real index."""
    # Given - rail 0 is empty ({}), rail 1 has a status set.
    dcdc0, _ = _make_dcdcs(dcdc_module)
    statuses = {
        dcdc0: dcdc_module.DcdcStatusRecord(
            name="DCDC0",
            dev_type="xdpe1a2g5b",
            global_statuses={},
            rail_statuses=[{}, {"STATUS_WORD": 0x0001}],
            global_statuses_decoded={},
            rail_statuses_decoded=[{}, {"STATUS_WORD": ["OTHER_STATUS_CHANGE"]}],
        ),
    }

    mock_chassis = _mock_chassis_with_dcdcs(statuses)

    p_root, p_chassis = _patch_read_statuses_deps(nh_blackbox_module, mock_chassis)
    with p_root, p_chassis:
        # When
        result = CliRunner().invoke(nh_blackbox_module.blackbox, ["dcdc", "read-statuses"])

    # Then - the populated rail is labeled "rail 1", not "rail 0".
    assert result.exit_code == 0, result.output
    assert result.output == textwrap.dedent(
        """\
        DCDC0 (xdpe1a2g5b):
        rail 1: [STATUS_WORD] 0b0000_0000_0000_0001

        """
    )
