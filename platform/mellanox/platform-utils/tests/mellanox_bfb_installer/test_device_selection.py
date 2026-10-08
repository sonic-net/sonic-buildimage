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
Unit tests for mellanox_bfb_installer.device_selection module.
"""

import os
import sys
from unittest import mock

import pytest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


class TestUserDpuSelectionToDpusFromPlatformJson:
    """Tests for _user_dpu_selection_to_dpus_from_platform_json."""

    def test_exits_when_no_dpus_found(self):
        """Exits when list_dpus returns empty."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=[]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus="all", rshims=None, script_name="test_script", print_usage_callback=print_usage
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert (
            "No DPUs found! Make sure to run the %s script from the Smart Switch host device/switch!"
            in mock_log.error.call_args[0][0]
        )
        assert mock_log.error.call_args[0][1] == "test_script"

    def test_exits_when_dpu_param_empty_string_or_whitespace(self):
        """Exits when dpus is empty string or whitespace-only."""
        from mellanox_bfb_installer import device_selection

        expected_msg = "If dpu parameter is provided, it cannot be empty!"
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]):
            for empty_val in ("", " ", "  "):
                print_usage = mock.MagicMock()
                mock_log = mock.MagicMock()
                with (
                    mock.patch.object(device_selection, "logger", mock_log),
                    mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
                ):
                    with pytest.raises(SystemExit) as ctx:
                        device_selection._user_dpu_selection_to_dpus_from_platform_json(
                            dpus=empty_val, rshims=None, script_name="test", print_usage_callback=print_usage
                        )
                    assert isinstance(ctx.value, SystemExit)
                    assert ctx.value.code == 1
                    mock_log.error.assert_called_once()
                    assert mock_log.error.call_args[0][0] == expected_msg
                    print_usage.assert_called_once()

    def test_exits_when_rshim_param_empty_string_or_whitespace(self):
        """Exits when rshims is empty string or whitespace-only."""
        from mellanox_bfb_installer import device_selection

        expected_msg = "If rshim parameter is provided, it cannot be empty!"
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]):
            for empty_val in ("", " ", "  "):
                print_usage = mock.MagicMock()
                mock_log = mock.MagicMock()
                with mock.patch.object(device_selection, "logger", mock_log):
                    with pytest.raises(SystemExit) as ctx:
                        device_selection._user_dpu_selection_to_dpus_from_platform_json(
                            dpus=None, rshims=empty_val, script_name="test", print_usage_callback=print_usage
                        )
                    assert isinstance(ctx.value, SystemExit)
                    assert ctx.value.code == 1
                    mock_log.error.assert_called_once()
                    assert mock_log.error.call_args[0][0] == expected_msg
                    print_usage.assert_called_once()

    def test_exits_when_both_dpus_and_rshims_provided(self):
        """Exits when both dpus and rshims are provided."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus="dpu0", rshims="rshim0", script_name="test", print_usage_callback=print_usage
                )
            assert isinstance(ctx.value, SystemExit)
            assert ctx.value.code == 1
            print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert "Both dpu and rshim selection provided" in mock_log.error.call_args[0][0]

    def test_returns_all_dpus_when_dpus_all(self):
        """_user_dpu_selection_to_dpus_from_platform_json returns all DPUs when dpus='all'."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="all", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        print_usage.assert_not_called()

    def test_returns_all_dpus_when_rshims_all(self):
        """Returns all DPUs when rshims='all'."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="all", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        print_usage.assert_not_called()

    def test_returns_dpu_list_when_dpus_comma_separated(self):
        """Returns validated DPU list when dpus is comma-separated."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1", "dpu2"]):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="dpu0,dpu2", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu2"]
        print_usage.assert_not_called()

    def test_exits_when_dpu_not_in_platform(self):
        """Exits when requested DPU is not in platform.json list."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus="dpu0,dpu99", rshims=None, script_name="test", print_usage_callback=print_usage
                )
            assert isinstance(ctx.value, SystemExit)
            assert ctx.value.code == 1
            print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert "DPU" in mock_log.error.call_args[0][0]
        assert "not found" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == "dpu99"

    @pytest.mark.parametrize(
        "dpus_arg",
        [
            ",",
            "dpu1,",
            ",dpu1",
            "dpu1,,dpu2",
        ],
    )
    def test_exits_when_dpus_list_has_empty_segments_from_commas(self, dpus_arg):
        """Comma-only, leading/trailing, or doubled commas produce empty DPU names and exit."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        expected_msg = "If providing a list of DPUs, it cannot contain empty strings! (Check for extra commas.)"
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1", "dpu2"]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus=dpus_arg, rshims=None, script_name="test", print_usage_callback=print_usage
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert mock_log.error.call_args[0][0] == expected_msg

    @pytest.mark.parametrize(
        "rshims_arg",
        [
            ",",
            "rshim1,",
            ",rshim1",
            "rshim1,,rshim2",
        ],
    )
    def test_exits_when_rshims_list_has_empty_segments_from_commas(self, rshims_arg):
        """Comma-only, leading/trailing, or doubled commas produce empty rshim names and exit."""
        from mellanox_bfb_installer import device_selection

        def rshim2dpu_mock(rshim):
            return {"rshim0": "dpu0", "rshim1": "dpu1", "rshim2": "dpu2"}.get(rshim)

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        expected_msg = "If providing a list of rshims, it cannot contain empty strings! (Check for extra commas.)"
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1", "dpu2"]),
            mock.patch.object(device_selection, "rshim2dpu", side_effect=rshim2dpu_mock),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus=None, rshims=rshims_arg, script_name="test", print_usage_callback=print_usage
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert mock_log.error.call_args[0][0] == expected_msg

    def test_returns_dpus_via_rshims_comma_separated(self):
        """Returns DPU list when rshims is comma-separated and all map to DPUs."""
        from mellanox_bfb_installer import device_selection

        def rshim2dpu_mock(rshim):
            return {"rshim0": "dpu0", "rshim1": "dpu1", "rshim2": "dpu2"}.get(rshim)

        print_usage = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1", "dpu2"]),
            mock.patch.object(device_selection, "rshim2dpu", side_effect=rshim2dpu_mock),
        ):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="rshim0,rshim1", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        print_usage.assert_not_called()

    def test_exits_when_rshim_has_no_dpu_mapping(self):
        """Exits when rshim has no corresponding DPU in platform.json."""
        from mellanox_bfb_installer import device_selection

        def rshim2dpu_mock(rshim):
            return {"rshim0": "dpu0"}.get(rshim)

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(device_selection, "rshim2dpu", side_effect=rshim2dpu_mock),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus=None, rshims="rshim0,rshim99", script_name="test", print_usage_callback=print_usage
                )
            assert isinstance(ctx.value, SystemExit)
            assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "No DPU in platform.json exists with rshim" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == "rshim99"

    def test_returns_dpu_list_when_dpus_comma_separated_with_spaces(self):
        """Returns DPU list when dpus has extra spaces around commas."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="dpu0 , dpu1", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]

    def test_returns_single_dpu_when_rshims_single(self):
        """Returns single DPU when rshims is a single value."""
        from mellanox_bfb_installer import device_selection

        def rshim2dpu_mock(rshim):
            return {"rshim0": "dpu0"}.get(rshim)

        print_usage = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(device_selection, "rshim2dpu", side_effect=rshim2dpu_mock),
        ):
            dpus = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="rshim0", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0"]

    def test_exits_when_both_none(self):
        """_user_dpu_selection_to_dpus_from_platform_json exits when both dpus and rshims are None."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus=None, rshims=None, script_name="test", print_usage_callback=print_usage
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert "No dpus specified!" in mock_log.error.call_args[0][0]

    def test_exits_when_dpus_list_has_duplicates(self):
        """Exits when the same DPU is selected more than once through --dpu."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus="dpu0,dpu1,dpu0", rshims=None, script_name="test", print_usage_callback=print_usage
                )
        assert ctx.value.code == 1
        print_usage.assert_called_once()
        mock_log.error.assert_called_once()
        assert "cannot be selected more than once" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == "dpu0"

    def test_exits_when_rshims_resolve_to_the_same_dpu(self):
        """Exits when distinct rshims map to a single DPU, which would flash it twice."""
        from mellanox_bfb_installer import device_selection

        def rshim2dpu_mock(rshim):
            return {"rshim0": "dpu0", "rshim0-alias": "dpu0"}.get(rshim)

        print_usage = mock.MagicMock()
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0"]),
            mock.patch.object(device_selection, "rshim2dpu", side_effect=rshim2dpu_mock),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection._user_dpu_selection_to_dpus_from_platform_json(
                    dpus=None,
                    rshims="rshim0,rshim0-alias",
                    script_name="test",
                    print_usage_callback=print_usage,
                )
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "cannot be selected more than once" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == "dpu0"


class TestGetTargets:
    """Tests for get_targets."""

    def test_returns_target_info_list_when_dpus_all(self):
        """get_targets returns list of TargetInfo when dpus='all'."""
        from mellanox_bfb_installer import device_selection
        from mellanox_bfb_installer.device_selection import TargetInfo
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:01:00.1",
            },
            "dpu1": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:02:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.1",
            },
        }
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0", "dpu1"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: {"dpu0": "rshim0", "dpu1": "rshim1"}.get(dpu),
            ),
        ):
            result = device_selection.get_targets(
                dpus="all",
                rshims=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert all(isinstance(t, TargetInfo) for t in result)
        assert result[0].dpu == "dpu0"
        assert result[0].rshim == "rshim0"
        assert result[0].dpu_pci_bus_id == "0000:01:00.0"
        assert result[0].rshim_pci_bus_id == "0000:01:00.1"
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].dpu_pci_bus_id == "0000:02:00.0"
        assert result[1].rshim_pci_bus_id == "0000:02:00.1"

    def test_returns_returns_targets_for_specified_dpus(self):
        """get_targets returns TargetInfo for specified dpus."""
        from mellanox_bfb_installer import device_selection
        from mellanox_bfb_installer.device_selection import TargetInfo
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        all_dpus = ["dpu0", "dpu1", "dpu2", "dpu3"]
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:01:00.1",
            },
            "dpu1": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:02:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.1",
            },
            "dpu2": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:03:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:03:00.1",
            },
            "dpu3": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:04:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:04:00.1",
            },
        }
        dpu2rshim_map = {f"dpu{i}": f"rshim{i}" for i in range(4)}
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=all_dpus,
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: dpu2rshim_map.get(dpu),
            ),
        ):
            result = device_selection.get_targets(
                dpus="dpu1,dpu3",
                rshims=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert result[0].dpu == "dpu1"
        assert result[0].rshim == "rshim1"
        assert result[0].dpu_pci_bus_id == "0000:02:00.0"
        assert result[0].rshim_pci_bus_id == "0000:02:00.1"
        assert result[1].dpu == "dpu3"
        assert result[1].rshim == "rshim3"
        assert result[1].dpu_pci_bus_id == "0000:04:00.0"
        assert result[1].rshim_pci_bus_id == "0000:04:00.1"

    def test_output_order_matches_user_input_order(self):
        """get_targets maintains the DPU order provided by the user."""
        from mellanox_bfb_installer import device_selection
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        all_dpus = ["dpu0", "dpu1", "dpu2", "dpu3"]
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:01:00.1",
            },
            "dpu1": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:02:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.1",
            },
            "dpu2": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:03:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:03:00.1",
            },
            "dpu3": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:04:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:04:00.1",
            },
        }
        dpu2rshim_map = {f"dpu{i}": f"rshim{i}" for i in range(4)}
        # User-provided order (random): indices 3, 1, 0, 2
        dpus_input = "dpu3,dpu1,dpu0,dpu2"
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=all_dpus,
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: dpu2rshim_map.get(dpu),
            ),
        ):
            result = device_selection.get_targets(
                dpus=dpus_input,
                rshims=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 4
        assert result[0].dpu == "dpu3"
        assert result[0].rshim == "rshim3"
        assert result[0].dpu_pci_bus_id == "0000:04:00.0"
        assert result[0].rshim_pci_bus_id == "0000:04:00.1"
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].dpu_pci_bus_id == "0000:02:00.0"
        assert result[1].rshim_pci_bus_id == "0000:02:00.1"
        assert result[2].dpu == "dpu0"
        assert result[2].rshim == "rshim0"
        assert result[2].dpu_pci_bus_id == "0000:01:00.0"
        assert result[2].rshim_pci_bus_id == "0000:01:00.1"
        assert result[3].dpu == "dpu2"
        assert result[3].rshim == "rshim2"
        assert result[3].dpu_pci_bus_id == "0000:03:00.0"
        assert result[3].rshim_pci_bus_id == "0000:03:00.1"

    def test_exits_when_dpu_has_no_rshim_mapping(self):
        """get_targets exits when dpu2rshim returns None."""
        from mellanox_bfb_installer import device_selection
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.0",
            },
        }
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(device_selection, "dpu2rshim", return_value=None),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection.get_targets(
                    dpus="dpu0",
                    rshims=None,
                    script_name="test",
                    print_usage_callback=print_usage,
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "No rshim mapping found" in mock_log.error.call_args[0][0]

    def test_exits_when_dpu_not_detected_on_pci(self):
        """get_targets exits when DPU is not in get_dpus_detected_pci_bus_ids."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        # bus_ids is empty - dpu0 not detected
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value={},
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: {"dpu0": "rshim0"}.get(dpu),
            ),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection.get_targets(
                    dpus="dpu0",
                    rshims=None,
                    script_name="test",
                    print_usage_callback=print_usage,
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "no devices detected on the PCI bus" in mock_log.error.call_args[0][0]

    def test_returns_target_info_when_rshims_specified(self):
        """get_targets returns TargetInfo list when rshims specified instead of dpus."""
        from mellanox_bfb_installer import device_selection
        from mellanox_bfb_installer.device_selection import TargetInfo
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.0",
            },
            "dpu1": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:03:00.0",
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:04:00.0",
            },
        }
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0", "dpu1"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: {"dpu0": "rshim0", "dpu1": "rshim1"}.get(dpu),
            ),
            mock.patch.object(
                device_selection,
                "rshim2dpu",
                side_effect=lambda r: {"rshim0": "dpu0", "rshim1": "dpu1"}.get(r),
            ),
        ):
            result = device_selection.get_targets(
                dpus=None,
                rshims="rshim0,rshim1",
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert result[0].dpu == "dpu0"
        assert result[0].rshim == "rshim0"
        assert result[0].dpu_pci_bus_id == "0000:01:00.0"
        assert result[0].rshim_pci_bus_id == "0000:02:00.0"
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].dpu_pci_bus_id == "0000:03:00.0"
        assert result[1].rshim_pci_bus_id == "0000:04:00.0"

    def test_returns_target_info_when_dpu_pci_bus_id_none(self):
        """get_targets succeeds when dpu_pci_bus_id is None (ISOLATED MODE)."""
        from mellanox_bfb_installer import device_selection
        from mellanox_bfb_installer.device_selection import TargetInfo
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        # dpu_pci_bus_id (PCIE_INT) is None/absent; rshim_pci_bus_id required
        bus_ids = {
            "dpu0": {
                # PCIE_INT missing - ISOLATED MODE
                DpuInterfaceEnum.RSHIM_PCIE_INT.value: "0000:02:00.0",
            },
        }
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: {"dpu0": "rshim0"}.get(dpu),
            ),
        ):
            result = device_selection.get_targets(
                dpus="dpu0",
                rshims=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 1
        assert result[0].dpu == "dpu0"
        assert result[0].rshim == "rshim0"
        assert result[0].dpu_pci_bus_id is None
        assert result[0].rshim_pci_bus_id == "0000:02:00.0"

    def test_exits_when_rshim_not_detected_on_pci(self):
        """get_targets exits when rshim_pci_bus_id is missing."""
        from mellanox_bfb_installer import device_selection
        from sonic_platform.device_data import DpuInterfaceEnum

        print_usage = mock.MagicMock()
        # rshim_bus_info missing
        bus_ids = {
            "dpu0": {
                DpuInterfaceEnum.PCIE_INT.value: "0000:01:00.0",
                # RSHIM_PCIE_INT missing
            },
        }
        mock_log = mock.MagicMock()
        with (
            mock.patch.object(
                device_selection.platform_dpu,
                "list_dpus",
                return_value=["dpu0"],
            ),
            mock.patch.object(
                device_selection.platform_dpu,
                "get_dpus_detected_pci_bus_ids",
                return_value=bus_ids,
            ),
            mock.patch.object(
                device_selection,
                "dpu2rshim",
                side_effect=lambda dpu: {"dpu0": "rshim0"}.get(dpu),
            ),
            mock.patch.object(device_selection, "logger", mock_log),
        ):
            with pytest.raises(SystemExit) as ctx:
                device_selection.get_targets(
                    dpus="dpu0",
                    rshims=None,
                    script_name="test",
                    print_usage_callback=print_usage,
                )
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "rshim %s is not detected on the PCI bus" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == "dpu0"
        assert mock_log.error.call_args[0][2] == "rshim0"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
