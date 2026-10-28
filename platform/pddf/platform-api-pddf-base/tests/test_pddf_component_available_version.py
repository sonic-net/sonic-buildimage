#!/usr/bin/env python
# Copyright (c) 2026, Hewlett Packard Enterprise Development LP.
# SPDX-License-Identifier: Apache-2.0
"""
Unit tests for the generic 'available_version' runtime hook added to
sonic_platform_pddf_base.pddf_component.PddfComponent.get_available_firmware_version().

fwutil calls get_available_firmware_version() only when a component omits the
static "version" in platform_components.json. The base class now honors an
optional "available_version" command in pddf-device.json so platforms can
compute the target version at runtime. When the command is absent it must keep
returning the historical "Unknown".
"""

import sys
import types

import pytest


def _install_sonic_platform_base_stubs():
    """Minimal stubs so the real pddf_component module imports without the
    full sonic_platform_base package present."""
    if "sonic_platform_base" not in sys.modules:
        sys.modules["sonic_platform_base"] = types.ModuleType("sonic_platform_base")

    cb = types.ModuleType("sonic_platform_base.component_base")
    cb.ComponentBase = type("ComponentBase", (), {"__init__": lambda self: None})
    cb.FW_AUTO_ERR_BOOT_TYPE = -1
    cb.FW_AUTO_ERR_IMAGE = -2
    cb.FW_AUTO_ERR_UNKNOWN = -3
    cb.FW_AUTO_UPDATED = 2
    cb.FW_AUTO_SCHEDULED = 3
    sys.modules["sonic_platform_base.component_base"] = cb

    db = types.ModuleType("sonic_platform_base.device_base")
    db.DeviceBase = type("DeviceBase", (), {"__init__": lambda self: None})
    sys.modules["sonic_platform_base.device_base"] = db


_install_sonic_platform_base_stubs()

try:
    from sonic_platform_pddf_base import pddf_component as pc
except Exception as exc:  # pragma: no cover - environment-dependent
    pytest.skip("pddf_component not importable: %s" % exc, allow_module_level=True)


class _FakeResult:
    def __init__(self, returncode=0, stdout=b""):
        self.returncode = returncode
        self.stdout = stdout


class _FakePddf:
    """Records the last command and returns a canned result (or raises)."""
    def __init__(self, result=None, raise_exc=None):
        self.result = result
        self.raise_exc = raise_exc
        self.last_cmd = None

    def get_cmd_output(self, cmd):
        self.last_cmd = cmd
        if self.raise_exc is not None:
            raise self.raise_exc
        return self.result


def _make_component(cmd, pddf):
    comp = pc.PddfComponent.__new__(pc.PddfComponent)
    # Instance-level _get_cmd shadows the class method; returns our cmd for the
    # "available_version" attr and None otherwise.
    comp._get_cmd = lambda name: cmd if name == "available_version" else None
    comp.pddf_obj = pddf
    return comp


def test_returns_command_output_when_rc_zero():
    pddf = _FakePddf(_FakeResult(0, b"0710-001\n"))
    comp = _make_component("get_ssd_available_firmware_version ata5", pddf)
    assert comp.get_available_firmware_version("/usr/sbin/fw/ssd/") == "0710-001"


def test_formats_brace_placeholder_with_image_path():
    pddf = _FakePddf(_FakeResult(0, b"SBR10021"))
    comp = _make_component("resolve_ver {}", pddf)
    out = comp.get_available_firmware_version("/usr/sbin/fw/ssd/")
    assert out == "SBR10021"
    assert pddf.last_cmd == "resolve_ver /usr/sbin/fw/ssd/"


def test_unknown_when_command_absent():
    pddf = _FakePddf(_FakeResult(0, b"should-not-be-used"))
    comp = pc.PddfComponent.__new__(pc.PddfComponent)
    comp._get_cmd = lambda name: None      # no available_version cmd
    comp.pddf_obj = pddf
    assert comp.get_available_firmware_version("/img") == "Unknown"
    assert pddf.last_cmd is None           # command never run


def test_unknown_when_rc_nonzero():
    pddf = _FakePddf(_FakeResult(1, b"garbage"))
    comp = _make_component("some_cmd", pddf)
    assert comp.get_available_firmware_version("/img") == "Unknown"


def test_unknown_when_output_empty():
    pddf = _FakePddf(_FakeResult(0, b"   \n"))
    comp = _make_component("some_cmd", pddf)
    assert comp.get_available_firmware_version("/img") == "Unknown"


def test_unknown_when_command_raises():
    pddf = _FakePddf(raise_exc=RuntimeError("boom"))
    comp = _make_component("some_cmd", pddf)
    assert comp.get_available_firmware_version("/img") == "Unknown"
