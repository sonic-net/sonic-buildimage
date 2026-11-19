#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0


import json
import operator
import os
import syslog

from nexthop import fpga_lib

# Comparison name -> operator(read_value, threshold). Names match the
# VersionComparison enum (with its VERSION_COMPARISON_ prefix stripped)
# emitted into feature-flags.json by the config generator.
_COMPARISONS = {
    "LESS_THAN": operator.lt,
    "LESS_THAN_OR_EQUAL": operator.le,
    "GREATER_THAN": operator.gt,
    "GREATER_THAN_OR_EQUAL": operator.ge,
    "EQUAL": operator.eq,
}

FPGA_VERSION_SOURCE = "fpga_version"
ONIE_DEVICE_VERSION_SOURCE = "onie_device_version"
DEVICE_VERSION_VAR = "device_version"

_REQUIRED_FIELDS = {
    FPGA_VERSION_SOURCE: ("name", "bdf_var", "reg_offset", "mask", "comparison", "version"),
    ONIE_DEVICE_VERSION_SOURCE: ("name", "comparison", "version"),
}


def evaluate_flag(flag: dict, bdf: str) -> bool:
    """Evaluate a single flag against the live FPGA: read the 32-bit register at
    reg_offset, mask it, and compare against version per the flag's comparison."""
    raw = fpga_lib.read_32(bdf, int(flag["reg_offset"], 16))
    masked = raw & int(flag["mask"], 16)
    threshold = int(flag["version"], 16)
    return _COMPARISONS[flag["comparison"]](masked, threshold)


def get_feature_flag_variables(feature_flags_filepath: str, template_vars: dict) -> dict[str, bool]:
    """Return {flag_name: bool} for every flag in feature_flags_filepath,
    evaluated against live hardware.

    `template_vars` holds the already-resolved pcie-variables.yaml mapping (var
    name -> value, from pcie_lib.get_pcie_variables), where each flag's bdf_var
    is looked up to avoid re-running the lookup commands, plus `device_version`
    (gen_cli.get_device_version) for the flags that compare against it.

    Returns {} when the file is absent (a SKU declaring no flags). Raises on a
    malformed entry, an unknown comparison, an unresolved bdf_var or
    device_version, or an unreadable register, so the caller fails loudly rather
    than rendering the wrong (else-branch) layout.
    """
    if not os.path.isfile(feature_flags_filepath):
        return {}

    with open(feature_flags_filepath) as f:
        flags = json.load(f)
    if not flags:
        return {}

    result: dict[str, bool] = {}
    for flag in flags:
        source = flag.get("source")
        if source not in _REQUIRED_FIELDS:
            raise ValueError(
                f"{feature_flags_filepath}: feature flag entry has unknown source {source!r} "
                f"(expected one of {sorted(_REQUIRED_FIELDS)}): {flag}"
            )
        missing = [field for field in _REQUIRED_FIELDS[source] if field not in flag]
        if missing:
            raise ValueError(f"{feature_flags_filepath}: feature flag entry missing field(s) {missing}: {flag}")
        name = flag["name"]
        if flag["comparison"] not in _COMPARISONS:
            raise ValueError(
                f"{feature_flags_filepath}: feature flag '{name}' has unknown comparison "
                f"'{flag['comparison']}' (expected one of {sorted(_COMPARISONS)})"
            )
        if source == ONIE_DEVICE_VERSION_SOURCE:
            device_version = template_vars.get(DEVICE_VERSION_VAR)
            if device_version is None:
                raise RuntimeError(f"{feature_flags_filepath}: feature flag '{name}' needs the device version")
            value = _COMPARISONS[flag["comparison"]](device_version, int(flag["version"], 16))
            syslog.syslog(syslog.LOG_INFO, f"feature flag '{name}' = {value} (device_version={device_version})")
        elif source == FPGA_VERSION_SOURCE:
            bdf = template_vars.get(flag["bdf_var"])
            if not bdf:
                raise RuntimeError(
                    f"{feature_flags_filepath}: feature flag '{name}' references bdf_var "
                    f"'{flag['bdf_var']}' which is not defined in pcie-variables.yaml"
                )
            value = evaluate_flag(flag, bdf)
            syslog.syslog(syslog.LOG_INFO, f"feature flag '{name}' = {value} (bdf={bdf})")
        else:
            raise ValueError(f"{feature_flags_filepath}: feature flag '{name}' has unhandled source {source!r}")
        result[name] = value
    return result
