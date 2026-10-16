#!/usr/bin/env python
# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
from types import SimpleNamespace

import pytest

from sonic_platform_pddf_base.pddf_component import PddfComponent


def make_component(comp_attr):
    pddf_data = SimpleNamespace(data={"COMPONENT1": {"comp_attr": {"name": "SWITCHCARD_FPGA", **comp_attr}}})
    return PddfComponent(0, pddf_data, {"COMPONENT": {}})


@pytest.mark.parametrize("post_flash_reboot, action_msg", [
    (None, ""),
    ("none", ""),
    ("cold", " and power reboot device to take effect"),
    ("warm", " and reboot device to take effect"),
    ("fast", " and fast reboot device to take effect"),
])
def test_get_firmware_update_notification(post_flash_reboot, action_msg):
    comp_attr = {"type": "fpga"}
    if post_flash_reboot is not None:
        comp_attr["post_flash_reboot"] = post_flash_reboot
    component = make_component(comp_attr)
    assert component.get_firmware_update_notification("/tmp/fw.bin") == \
        f"FPGA will be updated, please wait for completion{action_msg}!"


def test_get_firmware_update_notification_invalid_post_flash_reboot():
    component = make_component({"type": "fpga", "post_flash_reboot": "later"})
    with pytest.raises(RuntimeError, match="Invalid post_flash_reboot value"):
        component.get_firmware_update_notification("/tmp/fw.bin")
