#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Unit tests for mellanox_bfb_installer.platform_dpu module.
"""

import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestPlatformDpu(unittest.TestCase):
    """Tests for platform_dpu module."""

    def test_validate_platform_passes_when_platform_and_path_exist(self):
        """validate_platform does not raise when get_platform, get_path_to_platform_dir are set and path is a file."""
        from mellanox_bfb_installer import platform_dpu

        mock_device_info = mock.MagicMock()
        mock_device_info.get_platform.return_value = "x86_64-mlnx_msn3700-r0"
        mock_device_info.get_path_to_platform_dir.return_value = "/usr/share/sonic/device/x86_64-mlnx_msn3700-r0"
        mock_device_info.PLATFORM_JSON_FILE = "platform.json"
        with (
            mock.patch.object(platform_dpu, "device_info", mock_device_info),
            mock.patch.object(platform_dpu.os.path, "isfile", return_value=True) as mock_isfile,
        ):
            platform_dpu.validate_platform()
        mock_device_info.get_platform.assert_called_once()
        # get_path_to_platform_dir is called twice: once in the if check, once when building path
        self.assertGreaterEqual(mock_device_info.get_path_to_platform_dir.call_count, 1)
        mock_isfile.assert_called_once_with("/usr/share/sonic/device/x86_64-mlnx_msn3700-r0/platform.json")

    def test_validate_platform_exits_when_get_platform_falsy(self):
        """validate_platform logs and raises SystemExit(1) when get_platform returns falsy."""
        from mellanox_bfb_installer import platform_dpu

        mock_device_info = mock.MagicMock()
        mock_device_info.get_platform.return_value = None
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "device_info", mock_device_info),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            with self.assertRaises(SystemExit) as ctx:
                platform_dpu.validate_platform()
        self.assertEqual(ctx.exception.code, 1)
        mock_log.error.assert_called_once()
        self.assertIn("PLATFORM", mock_log.error.call_args[0][0])

    def test_validate_platform_exits_when_get_path_to_platform_dir_falsy(self):
        """validate_platform logs and raises SystemExit(1) when get_path_to_platform_dir returns falsy."""
        from mellanox_bfb_installer import platform_dpu

        mock_device_info = mock.MagicMock()
        mock_device_info.get_platform.return_value = "x86_64-mlnx"
        mock_device_info.get_path_to_platform_dir.return_value = None
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "device_info", mock_device_info),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            with self.assertRaises(SystemExit) as ctx:
                platform_dpu.validate_platform()
        self.assertEqual(ctx.exception.code, 1)
        mock_log.error.assert_called_once()
        self.assertIn("PLATFORM directory", mock_log.error.call_args[0][0])

    def test_validate_platform_exits_when_path_not_file(self):
        """validate_platform logs and raises SystemExit(1) when platform.json path is not a file."""
        from mellanox_bfb_installer import platform_dpu

        mock_device_info = mock.MagicMock()
        mock_device_info.get_platform.return_value = "x86_64-mlnx"
        mock_device_info.get_path_to_platform_dir.return_value = "/usr/share/sonic/device/x86_64-mlnx"
        mock_device_info.PLATFORM_JSON_FILE = "platform.json"
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "device_info", mock_device_info),
            mock.patch.object(platform_dpu.os.path, "isfile", return_value=False),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            with self.assertRaises(SystemExit) as ctx:
                platform_dpu.validate_platform()
        self.assertEqual(ctx.exception.code, 1)
        mock_log.error.assert_called_once()
        self.assertIn("platform.json", mock_log.error.call_args[0][0])

    def test_list_dpus_returns_keys_from_dpus_data(self):
        """list_dpus returns list of DPU names when get_platform_dpus_data returns a dict."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(platform_dpu, "_get_dpus_data", return_value={"dpu0": {}, "dpu1": {}}):
            result = platform_dpu.list_dpus()
        self.assertEqual(sorted(result), ["dpu0", "dpu1"])

    def test_list_dpus_returns_empty_when_no_dpus(self):
        """list_dpus returns [] when get_platform_dpus_data returns None or empty."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(platform_dpu, "_get_dpus_data", return_value=None):
            self.assertEqual(platform_dpu.list_dpus(), [])
        with mock.patch.object(platform_dpu, "_get_dpus_data", return_value={}):
            self.assertEqual(platform_dpu.list_dpus(), [])

    def test_dpu2rshim_returns_rshim_for_dpu(self):
        """dpu2rshim returns rshim name from DeviceDataManager.get_dpu_interface."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(platform_dpu.DeviceDataManager, "get_dpu_interface", return_value="rshim0"):
            result = platform_dpu.dpu2rshim("dpu0")
        self.assertEqual(result, "rshim0")

    def test_rshim2dpu_returns_dpu_for_rshim(self):
        """rshim2dpu returns DPU name when DPUS has matching rshim_info."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(
            platform_dpu,
            "_get_dpus_data",
            return_value={"dpu0": {"rshim_info": "rshim0"}, "dpu1": {"rshim_info": "rshim1"}},
        ):
            self.assertEqual(platform_dpu.rshim2dpu("rshim0"), "dpu0")
            self.assertEqual(platform_dpu.rshim2dpu("rshim1"), "dpu1")
            self.assertIsNone(platform_dpu.rshim2dpu("rshim99"))

    def test_run_lspci_d_n_returns_stdout_on_success(self):
        """_run_lspci_d_n returns subprocess stdout when returncode is 0."""
        from mellanox_bfb_installer import platform_dpu

        mock_result = mock.MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "0000:08:00.0 0200: 15b3:a2dc (rev 01)\n"
        with mock.patch.object(platform_dpu.subprocess, "run") as mock_run:
            mock_run.return_value = mock_result
            out = platform_dpu._run_lspci_d_n()
        self.assertEqual(out, "0000:08:00.0 0200: 15b3:a2dc (rev 01)\n")
        mock_run.assert_called_once()
        self.assertEqual(mock_run.call_args[0][0], ["lspci", "-D", "-n"])
        self.assertEqual(
            mock_run.call_args[1],
            {"capture_output": True, "text": True, "timeout": 10},
        )

    def test_run_lspci_d_n_returns_empty_on_nonzero_returncode(self):
        """_run_lspci_d_n returns empty string when subprocess returncode is non-zero."""
        from mellanox_bfb_installer import platform_dpu

        mock_result = mock.MagicMock()
        mock_result.returncode = 1
        mock_result.stdout = "error"
        with mock.patch.object(platform_dpu.subprocess, "run", return_value=mock_result):
            out = platform_dpu._run_lspci_d_n()
        self.assertEqual(out, "")

    def test_run_lspci_d_n_returns_empty_on_exception(self):
        """_run_lspci_d_n returns empty string when subprocess raises."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(platform_dpu.subprocess, "run", side_effect=OSError("lspci not found")):
            out = platform_dpu._run_lspci_d_n()
        self.assertEqual(out, "")

    def test_get_dpus_detected_pci_bus_ids_returns_both_devices_when_present(self):
        """get_dpus_detected_pci_bus_ids returns both CX7 and RSHIM when lspci shows both."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        lspci_output = "0000:01:00.0 0200: 15b3:a2dc (rev 01)\n" "0000:01:00.1 0200: 15b3:c2d5 (rev 01)\n"

        def get_dpu_interface(dpu, iface):
            if iface == DpuInterfaceEnum.PCIE_INT.value:
                return "0000:01:00.0" if dpu == "dpu0" else None
            if iface == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                return "0000:01:00.1" if dpu == "dpu0" else None
            return None

        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        self.assertEqual(
            result,
            {
                "dpu0": {
                    DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                    DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:01:00.1",
                }
            },
        )

    def test_get_dpus_detected_pci_bus_ids_returns_empty_when_no_dpus(self):
        """get_dpus_detected_pci_bus_ids returns {} when list_dpus is empty."""
        from mellanox_bfb_installer import platform_dpu

        with mock.patch.object(platform_dpu, "list_dpus", return_value=[]):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()
        self.assertEqual(result, {})

    def test_get_dpus_detected_pci_bus_ids_includes_only_detected_devices(self):
        """get_dpus_detected_pci_bus_ids includes only devices present in lspci output."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        # Only RSHIM present; CX7 not in lspci
        lspci_output = "0000:02:00.0 0200: 15b3:c2d5 (rev 01)\n"

        def get_dpu_interface(dpu, iface):
            if iface == DpuInterfaceEnum.PCIE_INT.value:
                return "0000:01:00.0" if dpu == "dpu0" else None
            if iface == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                return "0000:02:00.0" if dpu == "dpu0" else None
            return None

        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        self.assertEqual(
            result,
            {
                "dpu0": {
                    # Only RSHIM present; CX7 not in lspci
                    DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.0",
                }
            },
        )

    def test_get_dpus_detected_pci_bus_ids_isolated_mode_logs_warning(self):
        """get_dpus_detected_pci_bus_ids correctly handles when BFSOC (rshim) at CX7 (dpu) bus id (ISOLATED MODE)."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        # BFSOC (rshim) at 0000:01:00.0 where platform expects CX7
        lspci_output = "0000:01:00.0 0200: 15b3:c2d5 (rev 01)\n"

        def get_platform_json_dpu_interface(dpu, iface):
            if iface == DpuInterfaceEnum.PCIE_INT.value:
                return "0000:01:00.0" if dpu == "dpu0" else None
            if iface == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                return "0000:02:00.0" if dpu == "dpu0" else None
            return None

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_platform_json_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        mock_log.warning.assert_called_once()
        self.assertEqual(
            "DPU %s: is running in ISOLATED MODE. PCI device detected at bus id %s is device id %s, not a CX7 %s",
            mock_log.warning.call_args[0][0],
        )
        self.assertEqual(mock_log.warning.call_args[0][1], "dpu0")
        self.assertEqual(mock_log.warning.call_args[0][2], "0000:01:00.0")
        self.assertEqual(mock_log.warning.call_args[0][3], "15b3:c2d5")
        self.assertEqual(mock_log.warning.call_args[0][4], "15b3:a2dc")
        # Device still recorded as RSHIM_PCIE_INT at the bus id where it was detected,
        # which is different than the platform.json.
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.RSHIM_PCIE_INT.value], "0000:01:00.0")

    def test_get_dpus_detected_pci_bus_ids_cx7_at_rshim_slot_logs_error(self):
        """get_dpus_detected_pci_bus_ids correctly handles when CX7 (dpu) at BFSOC (rshim) bus id."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        # CX7 (ethernet) at 0000:02:00.0 where platform expects BFSOC (rshim)
        lspci_output = "0000:02:00.0 0200: 15b3:a2dc (rev 01)\n"

        def get_dpu_interface(dpu, iface):
            if iface == DpuInterfaceEnum.PCIE_INT.value:
                return "0000:01:00.0" if dpu == "dpu0" else None
            if iface == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                return "0000:02:00.0" if dpu == "dpu0" else None
            return None

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        mock_log.error.assert_called_once()
        self.assertEqual(
            "PCI device detected for DPU %s at bus id %s is the CX7 device id %s, not "
            "the expected BFSOC / rshim device id %s",
            mock_log.error.call_args[0][0],
        )
        self.assertEqual(mock_log.error.call_args[0][1], "dpu0")
        self.assertEqual(mock_log.error.call_args[0][2], "0000:02:00.0")
        self.assertEqual(mock_log.error.call_args[0][3], "15b3:a2dc")
        self.assertEqual(mock_log.error.call_args[0][4], "15b3:c2d5")
        # Device still recorded as PCIE_INT at the slot where it was detected,
        # which is different than the platform.json. This shouldn't happen in
        # real life, but we should handle it gracefully.
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.PCIE_INT.value], "0000:02:00.0")

    def test_get_dpus_detected_pci_bus_ids_skips_invalid_lspci_line(self):
        """get_dpus_detected_pci_bus_ids skips lspci lines with fewer than 3 tokens."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        lspci_output = "0000:01:00.0 0200: 15b3:a2dc (rev 01)\nbad line\n0000:01:00.1 0200: 15b3:c2d5 (rev 01)\n"

        def get_dpu_interface(dpu, iface):
            if iface == DpuInterfaceEnum.PCIE_INT.value:
                return "0000:01:00.0" if dpu == "dpu0" else None
            if iface == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                return "0000:01:00.1" if dpu == "dpu0" else None
            return None

        mock_log = mock.MagicMock()
        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
            mock.patch.object(platform_dpu, "logger", mock_log),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        mock_log.warning.assert_called_once()
        self.assertIn("Invalid `lspci -D -n` output line", mock_log.warning.call_args[0][0])
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.PCIE_INT.value], "0000:01:00.0")
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.RSHIM_PCIE_INT.value], "0000:01:00.1")

    def test_get_dpus_detected_pci_bus_ids_multiple_dpus(self):
        """get_dpus_detected_pci_bus_ids returns correct mapping for multiple DPUs."""
        from mellanox_bfb_installer import platform_dpu
        from sonic_platform.device_data import DpuInterfaceEnum

        lspci_output = (
            "0000:01:00.0 0200: 15b3:a2dc (rev 01)\n"
            "0000:01:00.1 0200: 15b3:c2d5 (rev 01)\n"
            "0000:03:00.0 0200: 15b3:a2dc (rev 01)\n"
            "0000:03:00.1 0200: 15b3:c2d5 (rev 01)\n"
        )

        def get_dpu_interface(dpu, iface):
            mapping = {
                ("dpu0", DpuInterfaceEnum.PCIE_INT.value): "0000:01:00.0",
                ("dpu0", DpuInterfaceEnum.RSHIM_PCIE_INT.value): "0000:01:00.1",
                ("dpu1", DpuInterfaceEnum.PCIE_INT.value): "0000:03:00.0",
                ("dpu1", DpuInterfaceEnum.RSHIM_PCIE_INT.value): "0000:03:00.1",
            }
            return mapping.get((dpu, iface))

        with (
            mock.patch.object(platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]),
            mock.patch.object(
                platform_dpu.DeviceDataManager,
                "get_dpu_interface",
                side_effect=get_dpu_interface,
            ),
            mock.patch.object(platform_dpu, "_run_lspci_d_n", return_value=lspci_output),
        ):
            result = platform_dpu.get_dpus_detected_pci_bus_ids()

        self.assertEqual(len(result), 2)
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.PCIE_INT.value], "0000:01:00.0")
        self.assertEqual(result["dpu0"][DpuInterfaceEnum.RSHIM_PCIE_INT.value], "0000:01:00.1")
        self.assertEqual(result["dpu1"][DpuInterfaceEnum.PCIE_INT.value], "0000:03:00.0")
        self.assertEqual(result["dpu1"][DpuInterfaceEnum.RSHIM_PCIE_INT.value], "0000:03:00.1")

    def test_remove_cx7_pci_device_logs_and_writes_remove_when_present(self):
        """remove_cx7_pci_device logs and writes to sysfs remove path."""
        from mellanox_bfb_installer import platform_dpu

        mock_log = mock.MagicMock()
        bus_id = "0000:08:00.0"
        with (
            mock.patch.object(platform_dpu, "logger", mock_log),
            mock.patch("builtins.open", mock.mock_open()) as mock_open,
        ):
            platform_dpu.remove_cx7_pci_device(bus_id, "rshim0: ")
            mock_log.info.assert_called_once_with("%sRemoving CX PCI device %s", "rshim0: ", bus_id)
            mock_open.assert_called_once_with(f"/sys/bus/pci/devices/{bus_id}/remove", "w")
            mock_open().write.assert_called_once_with("1")

    def test_remove_cx7_pci_device_logs_error_when_open_fails(self):
        """remove_cx7_pci_device logs error and does not raise when open raises OSError."""
        from mellanox_bfb_installer import platform_dpu

        mock_log = mock.MagicMock()
        bus_id = "0000:08:00.0"
        err = OSError(2, "No such file or directory")
        with (
            mock.patch.object(platform_dpu, "logger", mock_log),
            mock.patch("builtins.open", side_effect=err),
        ):
            platform_dpu.remove_cx7_pci_device(bus_id, "rshim0: ")
        # Does not raise.
        mock_log.info.assert_called_once_with("%sRemoving CX PCI device %s", "rshim0: ", bus_id)
        mock_log.error.assert_called_once_with("Failed to remove PCI device %s: %s", bus_id, err)


if __name__ == "__main__":
    unittest.main()
