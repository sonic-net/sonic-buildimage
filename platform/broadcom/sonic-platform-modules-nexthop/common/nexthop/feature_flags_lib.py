#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Generic FPGA-version feature-flag evaluator.

A feature-flags.json artifact (shipped beside pddf-device.json.j2) lists boolean
flags whose value is decided at boot by reading an FPGA revision register and
comparing it (masked) against a threshold. This module evaluates each flag
against the live hardware and returns {name: bool}, which gen_cli feeds to the
pddf-device.json.j2 render as Jinja booleans so a `{% if flag %}` selects the
right register layout per unit.

Every entry is fully self-describing data — bdf_var (which FPGA), reg_offset,
mask, comparison, version — so nothing here is specific to any platform, FPGA,
or flag name. The data is produced by the platform_topology config generator;
this evaluator just applies it.

Example feature-flags.json entry:
    {
      "name": "fan_duty_packed_in_one_word",
      "bdf_var": "switchcard_fpga_1_bdf",
      "reg_offset": "0x0",
      "mask": "0xfff",
      "comparison": "LESS_THAN_OR_EQUAL",
      "version": "0x304"
    }
"""

import json
import operator
import os
import syslog

from nexthop import fpga_lib

# Comparison name -> operator(read_value, threshold). Names match the
# FpgaVersionComparison enum (with its FPGA_VERSION_COMPARISON_ prefix stripped)
# emitted into feature-flags.json by the config generator.
_COMPARISONS = {
    "LESS_THAN": operator.lt,
    "LESS_THAN_OR_EQUAL": operator.le,
    "GREATER_THAN": operator.gt,
    "GREATER_THAN_OR_EQUAL": operator.ge,
    "EQUAL": operator.eq,
}

_REQUIRED_FIELDS = ("name", "bdf_var", "reg_offset", "mask", "comparison", "version")


def evaluate_flag(flag: dict, bdf: str) -> bool:
    """Evaluate a single flag against the live FPGA: read the 32-bit register at
    reg_offset, mask it, and compare against version per the flag's comparison."""
    raw = fpga_lib.read_32(bdf, int(flag["reg_offset"], 16))
    masked = raw & int(flag["mask"], 16)
    threshold = int(flag["version"], 16)
    return _COMPARISONS[flag["comparison"]](masked, threshold)


def get_feature_flag_variables(feature_flags_filepath: str, pcie_vars: dict[str, str]) -> dict[str, bool]:
    """Return {flag_name: bool} for every flag in feature_flags_filepath,
    evaluated against live hardware.

    `pcie_vars` is the already-resolved pcie-variables.yaml mapping (var name ->
    value, from pcie_lib.get_pcie_variables); each flag's bdf_var is looked up
    there to avoid re-running the lookup commands.

    Returns {} when the file is absent (a SKU declaring no flags). Raises on a
    malformed entry, an unknown comparison, an unresolved bdf_var, or an
    unreadable register, so the caller fails loudly rather than rendering the
    wrong (else-branch) layout.
    """
    if not os.path.isfile(feature_flags_filepath):
        return {}

    with open(feature_flags_filepath) as f:
        flags = json.load(f)
    if not flags:
        return {}

    result: dict[str, bool] = {}
    for flag in flags:
        missing = [field for field in _REQUIRED_FIELDS if field not in flag]
        if missing:
            raise ValueError(f"{feature_flags_filepath}: feature flag entry missing field(s) {missing}: {flag}")
        name = flag["name"]
        if flag["comparison"] not in _COMPARISONS:
            raise ValueError(
                f"{feature_flags_filepath}: feature flag '{name}' has unknown comparison "
                f"'{flag['comparison']}' (expected one of {sorted(_COMPARISONS)})"
            )
        bdf = pcie_vars.get(flag["bdf_var"])
        if not bdf:
            raise RuntimeError(
                f"{feature_flags_filepath}: feature flag '{name}' references bdf_var "
                f"'{flag['bdf_var']}' which is not defined in pcie-variables.yaml"
            )
        value = evaluate_flag(flag, bdf)
        result[name] = value
        syslog.syslog(syslog.LOG_INFO, f"feature flag '{name}' = {value} (bdf={bdf})")
    return result
