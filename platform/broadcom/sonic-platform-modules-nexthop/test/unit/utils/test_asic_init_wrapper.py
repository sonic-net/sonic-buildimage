#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for asic_init_wrapper: ASIC BDF discovery and the PCI
config-space read-modify-write helper. The setpci/lspci mechanics and the
asic-bus classifier live in nexthop.pcie_lib and are tested there; here we
exercise the wrapper's error-handling/logging policy on top of them.
"""

import importlib.machinery
import importlib.util
import os
import sys
from unittest.mock import mock_open, patch

import pytest

sys.dont_write_bytecode = True


@pytest.fixture(scope="function")
def wrapper():
    """Load the asic_init_wrapper.py script as a module with syslog silenced.

    The import is performed inside the fixture so the autouse
    patch_dependencies fixture (which makes `nexthop` importable) is active.
    """
    test_dir = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(test_dir, "../../../common/utils/asic_init_wrapper.py")
    loader = importlib.machinery.SourceFileLoader("asic_init_wrapper", script_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[loader.name] = module
    try:
        spec.loader.exec_module(module)
        with patch.object(module, "syslog", autospec=True):
            yield module
    finally:
        sys.modules.pop(loader.name, None)


class TestIsWarmBootPostKexec:
    def test_warm_boot_detected(self, wrapper):
        cmdline = "BOOT_IMAGE=/boot/vmlinuz SONIC_BOOT_TYPE=warm rw\n"
        with patch("builtins.open", mock_open(read_data=cmdline)):
            assert wrapper.is_warm_boot_post_kexec() is True

    def test_cold_boot(self, wrapper):
        with patch("builtins.open", mock_open(read_data="BOOT_IMAGE=/boot/vmlinuz rw\n")):
            assert wrapper.is_warm_boot_post_kexec() is False

    def test_fast_reboot_is_not_warm(self, wrapper):
        cmdline = "BOOT_IMAGE=/boot/vmlinuz SONIC_BOOT_TYPE=fast-reboot rw\n"
        with patch("builtins.open", mock_open(read_data=cmdline)):
            assert wrapper.is_warm_boot_post_kexec() is False


class TestHandleWarmBootPostKexec:
    """The BDF lookup itself is covered by TestGetPcieDeviceBdfs in test_pcie_lib.py."""

    def test_disables_interrupts_on_every_present_asic(self, wrapper):
        with (
            patch.object(wrapper.pcie_lib, "get_pcie_device_bdfs", autospec=True,
                         return_value=["01:00.0", "0a:00.0"]) as get_bdfs,
            patch.object(wrapper, "asic_present_on_pci_bus", side_effect=[True, False]),
            patch.object(wrapper, "disable_asic_pci_interrupts") as disable,
        ):
            assert wrapper.handle_warm_boot_post_kexec() is True

        get_bdfs.assert_called_once_with(device_type=wrapper.pcie_lib.PcieDeviceType.ASIC)
        disable.assert_called_once_with("01:00.0")

    def test_no_asic_bdfs_falls_back(self, wrapper):
        with patch.object(wrapper.pcie_lib, "get_pcie_device_bdfs", autospec=True, return_value=[]):
            assert wrapper.handle_warm_boot_post_kexec() is False

    def test_no_asic_present_falls_back(self, wrapper):
        with (
            patch.object(wrapper.pcie_lib, "get_pcie_device_bdfs", autospec=True, return_value=["01:00.0"]),
            patch.object(wrapper, "asic_present_on_pci_bus", return_value=False),
        ):
            assert wrapper.handle_warm_boot_post_kexec() is False


class TestDisableAsicPciInterrupts:
    def test_calls_each_disabler_and_tolerates_outcomes(self, wrapper):
        # Exercise all three _log_disable branches in one go: a real change
        # (INTx), an absent capability (MSI-X -> None), and a failure (MSI ->
        # raises). disable_asic_pci_interrupts must not propagate.
        change = wrapper.pcie_lib.PciWordChange(old=0x0142, new=0x0542)
        with (
            patch.object(wrapper.pcie_lib, "disable_intx", autospec=True, return_value=change) as intx,
            patch.object(wrapper.pcie_lib, "disable_msix", autospec=True, return_value=None) as msix,
            patch.object(wrapper.pcie_lib, "disable_msi", autospec=True, side_effect=RuntimeError("boom")) as msi,
        ):
            wrapper.disable_asic_pci_interrupts("01:00.0")
        intx.assert_called_once_with("01:00.0")
        msix.assert_called_once_with("01:00.0")
        msi.assert_called_once_with("01:00.0")


class TestAsicPresentOnPciBus:
    def test_present(self, wrapper):
        with patch.object(wrapper.pcie_lib, "pci_device_present", autospec=True, return_value=True):
            assert wrapper.asic_present_on_pci_bus("01:00.0") is True

    def test_failure_returns_false(self, wrapper):
        with patch.object(wrapper.pcie_lib, "pci_device_present", autospec=True, side_effect=RuntimeError("lspci")):
            assert wrapper.asic_present_on_pci_bus("01:00.0") is False
