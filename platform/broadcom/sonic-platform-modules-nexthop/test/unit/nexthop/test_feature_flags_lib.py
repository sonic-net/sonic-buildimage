#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import json
import os
import tempfile

import pytest


@pytest.fixture(scope="function", autouse=True)
def feature_flags_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import feature_flags_lib

    yield feature_flags_lib


def _write(tmp, flags):
    path = os.path.join(tmp, "feature-flags.json")
    with open(path, "w") as f:
        json.dump(flags, f)
    return path


def _flag(name, comparison, version, bdf_var="switchcard_fpga_1_bdf", reg_offset="0x0", mask="0xfff"):
    return {
        "name": name,
        "bdf_var": bdf_var,
        "reg_offset": reg_offset,
        "mask": mask,
        "comparison": comparison,
        "version": version,
    }


# Live FPGA1 revision register reads 0x303 (masked to 0xfff) for these tests.
PCIE_VARS = {"switchcard_fpga_1_bdf": "0000:03:00.0"}


@pytest.fixture
def read_303(feature_flags_lib_module, monkeypatch):
    monkeypatch.setattr(feature_flags_lib_module.fpga_lib, "read_32", lambda bdf, offset: 0x303)


def test_absent_file_returns_empty(feature_flags_lib_module):
    out = feature_flags_lib_module.get_feature_flag_variables("/no/such/feature-flags.json", PCIE_VARS)
    assert out == {}


def test_empty_list_returns_empty(feature_flags_lib_module):
    with tempfile.TemporaryDirectory() as tmp:
        out = feature_flags_lib_module.get_feature_flag_variables(_write(tmp, []), PCIE_VARS)
    assert out == {}


def test_comparisons_against_live_revision(feature_flags_lib_module, read_303):
    # Register reads 0x303.
    with tempfile.TemporaryDirectory() as tmp:
        flags = [
            _flag("le_304", "LESS_THAN_OR_EQUAL", "0x304"),   # 0x303 <= 0x304 -> True
            _flag("lt_303", "LESS_THAN", "0x303"),            # 0x303 <  0x303 -> False
            _flag("ge_303", "GREATER_THAN_OR_EQUAL", "0x303"),# 0x303 >= 0x303 -> True
            _flag("gt_303", "GREATER_THAN", "0x303"),         # 0x303 >  0x303 -> False
            _flag("eq_303", "EQUAL", "0x303"),                # 0x303 == 0x303 -> True
        ]
        out = feature_flags_lib_module.get_feature_flag_variables(_write(tmp, flags), PCIE_VARS)
    assert out == {"le_304": True, "lt_303": False, "ge_303": True, "gt_303": False, "eq_303": True}


def test_mask_is_applied_before_compare(feature_flags_lib_module, monkeypatch):
    # Raw 0xE303 with mask 0xfff -> 0x303, so the prototype-revision nibble is ignored.
    monkeypatch.setattr(feature_flags_lib_module.fpga_lib, "read_32", lambda bdf, offset: 0xE303)
    with tempfile.TemporaryDirectory() as tmp:
        flags = [_flag("le_304", "LESS_THAN_OR_EQUAL", "0x304", mask="0xfff")]
        out = feature_flags_lib_module.get_feature_flag_variables(_write(tmp, flags), PCIE_VARS)
    assert out == {"le_304": True}


def test_exactly_one_true_for_if_elif_flow(feature_flags_lib_module, read_303):
    # Three mutually-exclusive guards over the same register; exactly one is true.
    # This is the if/elif/elif/else selection the template renders.
    with tempfile.TemporaryDirectory() as tmp:
        flags = [
            _flag("lt_300", "LESS_THAN", "0x300"),  # 0x303 < 0x300 -> False
            _flag("eq_303", "EQUAL", "0x303"),      # 0x303 == 0x303 -> True
            _flag("gt_400", "GREATER_THAN", "0x400"),  # 0x303 > 0x400 -> False
        ]
        out = feature_flags_lib_module.get_feature_flag_variables(_write(tmp, flags), PCIE_VARS)
    assert [name for name, val in out.items() if val] == ["eq_303"]


def test_unknown_comparison_raises(feature_flags_lib_module, read_303):
    with tempfile.TemporaryDirectory() as tmp:
        flags = [_flag("bad", "NOT_EQUAL", "0x303")]
        with pytest.raises(ValueError):
            feature_flags_lib_module.get_feature_flag_variables(_write(tmp, flags), PCIE_VARS)


def test_unresolved_bdf_var_raises(feature_flags_lib_module, read_303):
    with tempfile.TemporaryDirectory() as tmp:
        flags = [_flag("x", "EQUAL", "0x303", bdf_var="missing_bdf")]
        with pytest.raises(RuntimeError):
            feature_flags_lib_module.get_feature_flag_variables(_write(tmp, flags), PCIE_VARS)


def test_missing_field_raises(feature_flags_lib_module, read_303):
    with tempfile.TemporaryDirectory() as tmp:
        bad = _flag("x", "EQUAL", "0x303")
        del bad["mask"]
        with pytest.raises(ValueError):
            feature_flags_lib_module.get_feature_flag_variables(_write(tmp, [bad]), PCIE_VARS)
