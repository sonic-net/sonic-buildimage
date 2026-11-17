#!/usr/bin/env python
# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
from types import SimpleNamespace
from unittest.mock import patch

import pytest

from sonic_platform_pddf_base.pddf_component import PddfComponent


def make_component(comp_attr):
    pddf_data = SimpleNamespace(data={"COMPONENT1": {"comp_attr": {"name": "SWITCHCARD_FPGA", **comp_attr}}})
    return PddfComponent(0, pddf_data, {"COMPONENT": {}})


@pytest.mark.parametrize("boot_type, action_msg", [
    (None, ""),
    ("none", ""),
    ("cold", " and power reboot device to take effect"),
    ("warm", " and reboot device to take effect"),
    ("fast", " and fast reboot device to take effect"),
])
def test_get_firmware_update_notification(boot_type, action_msg):
    comp_attr = {"type": "fpga"}
    if boot_type is not None:
        comp_attr["boot_type"] = boot_type
    component = make_component(comp_attr)
    assert component.get_firmware_update_notification("/tmp/fw.bin") == \
        f"FPGA will be updated, please wait for completion{action_msg}!"


def test_get_firmware_update_notification_invalid_boot_type():
    component = make_component({"type": "fpga", "boot_type": "later"})
    with pytest.raises(RuntimeError, match="Invalid boot_type value"):
        component.get_firmware_update_notification("/tmp/fw.bin")


@pytest.mark.parametrize("boot_type", ["none", "cold", "warm", "fast"])
def test_notification_matches_update_firmware_reboot(boot_type):
    # The notification must warn about a reboot exactly when update_firmware() performs one.
    component = make_component({"type": "fpga", "boot_type": boot_type})
    with patch.object(component, "_install_firmware", return_value=True), \
            patch("sonic_platform_pddf_base.pddf_component.subprocess.run") as run:
        assert component.update_firmware("/tmp/fw.bin")
    notification = component.get_firmware_update_notification("/tmp/fw.bin")
    assert run.called == ("reboot" in notification)
