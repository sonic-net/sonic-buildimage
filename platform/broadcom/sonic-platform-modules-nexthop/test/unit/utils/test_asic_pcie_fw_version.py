#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the asic_pcie_fw_version utility's header parser."""

import importlib.machinery
import importlib.util
import os
import struct
import sys

import pytest


sys.dont_write_bytecode = True


@pytest.fixture(scope="function")
def asic_pcie_fw_version_module():
    """Load the (extension-less) asic_pcie_fw_version script as a module."""
    test_dir = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(test_dir, "../../../common/utils/asic_pcie_fw_version")
    loader = importlib.machinery.SourceFileLoader("asic_pcie_fw_version", script_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    # Register the module before executing so dataclass introspection can
    # resolve cls.__module__ via sys.modules.
    sys.modules[loader.name] = module
    try:
        spec.loader.exec_module(module)
        yield module
    finally:
        sys.modules.pop(loader.name, None)


def _build_header(magic: int, loader_version: int, fw_version_bytes: bytes = b"") -> bytes:
    """Build a 32-byte FW header with the layout the parser expects."""
    header = bytearray(32)
    struct.pack_into("<I", header, 0, magic)
    struct.pack_into("<I", header, 4, loader_version)
    fw_slot = fw_version_bytes[:12].ljust(12, b"\x00")
    header[16:28] = fw_slot
    return bytes(header)


def test_parse_fw_header_dnx_real_world(asic_pcie_fw_version_module):
    """DNX path: matches the 20.2.0 sample from the PR's verification on humm223."""
    mod = asic_pcie_fw_version_module
    loader_version = (20 << 16) | (2 << 4) | 0  # 20.2.0
    fw_data = _build_header(mod.FW_MAGIC_NUMBER, loader_version)

    assert mod._parse_fw_header(fw_data, pciephy_index=0) == "20.2.0"


def test_parse_fw_header_xgs_real_world(asic_pcie_fw_version_module):
    """XGS path: matches the 2.1406_fwD004_00 sample from the PR's verification on blkt156."""
    mod = asic_pcie_fw_version_module
    loader_version = (2 << 16) | 1406
    fw_data = _build_header(mod.FW_MAGIC_NUMBER, loader_version, b"D004_00")

    assert mod._parse_fw_header(fw_data, pciephy_index=None) == "2.1406_fwD004_00"


def test_parse_fw_header_invalid_magic(asic_pcie_fw_version_module):
    mod = asic_pcie_fw_version_module
    fw_data = _build_header(0xDEADBEEF, 0x00140020)
    assert mod._parse_fw_header(fw_data, pciephy_index=0) is None


def test_parse_fw_header_wrong_length(asic_pcie_fw_version_module):
    mod = asic_pcie_fw_version_module
    short = _build_header(mod.FW_MAGIC_NUMBER, 0x00140020)[:16]
    assert mod._parse_fw_header(short, pciephy_index=0) is None
    assert mod._parse_fw_header(b"", pciephy_index=None) is None


def test_parse_fw_header_xgs_invalid_version_string(asic_pcie_fw_version_module):
    """XGS rejects version strings that don't match the expected charset."""
    mod = asic_pcie_fw_version_module
    loader_version = (2 << 16) | 1406
    fw_data = _build_header(mod.FW_MAGIC_NUMBER, loader_version, b"bad string!")
    assert mod._parse_fw_header(fw_data, pciephy_index=None) is None


def test_parse_fw_header_xgs_empty_version_string(asic_pcie_fw_version_module):
    mod = asic_pcie_fw_version_module
    loader_version = (2 << 16) | 1406
    fw_data = _build_header(mod.FW_MAGIC_NUMBER, loader_version, b"")
    assert mod._parse_fw_header(fw_data, pciephy_index=None) is None
