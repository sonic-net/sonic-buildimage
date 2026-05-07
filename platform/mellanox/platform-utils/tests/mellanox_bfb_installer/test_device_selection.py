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
import tempfile
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
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="all", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        assert user_selected_all
        print_usage.assert_not_called()

    def test_returns_all_dpus_when_rshims_all(self):
        """Returns all DPUs when rshims='all'."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1"]):
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="all", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        assert user_selected_all
        print_usage.assert_not_called()

    def test_returns_dpu_list_when_dpus_comma_separated(self):
        """Returns validated DPU list when dpus is comma-separated."""
        from mellanox_bfb_installer import device_selection

        print_usage = mock.MagicMock()
        with mock.patch.object(device_selection.platform_dpu, "list_dpus", return_value=["dpu0", "dpu1", "dpu2"]):
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="dpu0,dpu2", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu2"]
        assert not user_selected_all
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
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="rshim0,rshim1", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        assert not user_selected_all
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
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus="dpu0 , dpu1", rshims=None, script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0", "dpu1"]
        assert not user_selected_all

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
            dpus, user_selected_all = device_selection._user_dpu_selection_to_dpus_from_platform_json(
                dpus=None, rshims="rshim0", script_name="test", print_usage_callback=print_usage
            )
        assert dpus == ["dpu0"]
        assert not user_selected_all

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


@pytest.fixture(scope="module")
def config_paths_for_validation():
    """Provides (existing_file_path, nonexistent_file_path) for _validate_config_files tests."""
    with tempfile.NamedTemporaryFile(delete=False) as f:
        existing = f.name
    nonexistent = os.path.join(tempfile.gettempdir(), "nonexistent_bfb_installer_validate_98765")
    try:
        yield (existing, nonexistent)
    finally:
        os.unlink(existing)


class TestValidateConfigFiles:
    """Tests for _validate_config_files."""

    def test_validate_config_files_success_single_file(self, config_paths_for_validation):
        """_validate_config_files succeeds when all config paths are existing files (single)."""
        from mellanox_bfb_installer import device_selection

        existing_path, _ = config_paths_for_validation
        device_selection._validate_config_files([existing_path])
        # No SystemExit raised

    def test_validate_config_files_success_multiple_files(self, config_paths_for_validation):
        """_validate_config_files succeeds when all config paths are existing files (multiple)."""
        from mellanox_bfb_installer import device_selection

        existing_path, _ = config_paths_for_validation
        # Same file twice is valid for multiple configs
        device_selection._validate_config_files([existing_path, existing_path])
        # No SystemExit raised

    def test_validate_config_files_exits_when_path_not_file(self, config_paths_for_validation):
        """_validate_config_files exits when a config path is not a file."""
        from mellanox_bfb_installer import device_selection

        existing_path, nonexistent_path = config_paths_for_validation
        mock_log = mock.MagicMock()
        with mock.patch.object(device_selection, "logger", mock_log):
            with pytest.raises(SystemExit) as ctx:
                device_selection._validate_config_files([existing_path, nonexistent_path])
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "is not a file" in mock_log.error.call_args[0][0]
        assert nonexistent_path in mock_log.error.call_args[0]


class TestParseConfigPaths:
    """Tests for _parse_config_paths."""

    def test_returns_none_list_when_configs_none(self):
        """_parse_config_paths returns [None] * num_dpus when configs is None."""
        from mellanox_bfb_installer import device_selection

        result = device_selection._parse_config_paths(None, 3, False)
        assert result == [None, None, None]

    def test_success_single_file(self):
        """_parse_config_paths returns a list with the same file for all DPUs when a single file is provided."""
        from mellanox_bfb_installer import device_selection

        config_path = "/path/to/config.json"
        num_dpus = 2
        user_selected_all_dpus = False
        mock_validate = mock.MagicMock()
        with mock.patch.object(device_selection, "_validate_config_files", mock_validate):
            result = device_selection._parse_config_paths(config_path, num_dpus, user_selected_all_dpus)
        assert result == [config_path] * num_dpus
        mock_validate.assert_called_once_with([config_path])

    def test_success_multiple_files_matching_num_dpus(self):
        """_parse_config_paths returns config list when multiple files match num_dpus."""
        from mellanox_bfb_installer import device_selection

        configs_str = "/path/to/config1.json,/path/to/config2.json"
        num_dpus = 2
        user_selected_all_dpus = False
        mock_validate = mock.MagicMock()
        with mock.patch.object(device_selection, "_validate_config_files", mock_validate):
            result = device_selection._parse_config_paths(configs_str, num_dpus, user_selected_all_dpus)
        assert result == ["/path/to/config1.json", "/path/to/config2.json"]
        mock_validate.assert_called_once_with(["/path/to/config1.json", "/path/to/config2.json"])

    def test_exits_when_user_selected_all_dpus_and_multiple_configs(self):
        """_parse_config_paths exits when user_selected_all_dpus is True and more than one config file is provided."""
        from mellanox_bfb_installer import device_selection

        configs_str = "/path/to/config1.json,/path/to/config2.json"
        num_dpus = 2
        user_selected_all_dpus = True
        mock_log = mock.MagicMock()
        with mock.patch.object(device_selection, "logger", mock_log):
            with pytest.raises(SystemExit) as ctx:
                device_selection._parse_config_paths(configs_str, num_dpus, user_selected_all_dpus)
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert 'Cannot specify "all" for dpus and more than one config file!' in mock_log.error.call_args[0][0]

    def test_exits_when_config_count_mismatch(self):
        """_parse_config_paths exits when number of config files does not match num_dpus."""
        from mellanox_bfb_installer import device_selection

        configs_str = "/path/to/config1.json,/path/to/config2.json"
        num_dpus = 3
        mock_log = mock.MagicMock()
        with (mock.patch.object(device_selection, "logger", mock_log),):
            with pytest.raises(SystemExit) as ctx:
                device_selection._parse_config_paths(configs_str, num_dpus, False)
        assert isinstance(ctx.value, SystemExit)
        assert ctx.value.code == 1
        mock_log.error.assert_called_once()
        assert "Number of config files does not match" in mock_log.error.call_args[0][0]
        assert mock_log.error.call_args[0][1] == 2
        assert mock_log.error.call_args[0][2] == 3


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
                configs=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert all(isinstance(t, TargetInfo) for t in result)
        assert result[0].dpu == "dpu0"
        assert result[0].rshim == "rshim0"
        assert result[0].dpu_pci_bus_id == "0000:01:00.0"
        assert result[0].rshim_pci_bus_id == "0000:01:00.1"
        assert result[0].config_path is None
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].dpu_pci_bus_id == "0000:02:00.0"
        assert result[1].rshim_pci_bus_id == "0000:02:00.1"
        assert result[1].config_path is None

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
                configs=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert result[0].dpu == "dpu1"
        assert result[0].rshim == "rshim1"
        assert result[0].dpu_pci_bus_id == "0000:02:00.0"
        assert result[0].rshim_pci_bus_id == "0000:02:00.1"
        assert result[0].config_path is None
        assert result[1].dpu == "dpu3"
        assert result[1].rshim == "rshim3"
        assert result[1].dpu_pci_bus_id == "0000:04:00.0"
        assert result[1].rshim_pci_bus_id == "0000:04:00.1"
        assert result[1].config_path is None

    def test_returns_target_info_with_config_paths(self):
        """get_targets includes config_path when configs provided."""
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
            mock.patch.object(device_selection, "_parse_config_paths"),
        ):
            device_selection._parse_config_paths.return_value = [
                "/etc/config1.json",
                "/etc/config2.json",
            ]
            result = device_selection.get_targets(
                dpus="all",
                rshims=None,
                configs="/etc/config1.json,/etc/config2.json",
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert result[0].config_path == "/etc/config1.json"
        assert result[1].config_path == "/etc/config2.json"

    def test_output_order_matches_user_input_order_for_dpus_and_configs(self):
        """get_targets maintains the order of dpus and configs in the user input."""
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
        # User-provided order (random): indices 3, 1, 0, 2
        dpus_input = "dpu3,dpu1,dpu0,dpu2"
        configs_input = "/cfg/dpu3.json,/cfg/dpu1.json,/cfg/dpu0.json,/cfg/dpu2.json"
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
            mock.patch(
                "mellanox_bfb_installer.device_selection.os.path.isfile",
                return_value=True,
            ),
        ):
            result = device_selection.get_targets(
                dpus=dpus_input,
                rshims=None,
                configs=configs_input,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 4
        assert result[0].dpu == "dpu3"
        assert result[0].rshim == "rshim3"
        assert result[0].config_path == "/cfg/dpu3.json"
        assert result[0].dpu_pci_bus_id == "0000:04:00.0"
        assert result[0].rshim_pci_bus_id == "0000:04:00.1"
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].config_path == "/cfg/dpu1.json"
        assert result[1].dpu_pci_bus_id == "0000:02:00.0"
        assert result[1].rshim_pci_bus_id == "0000:02:00.1"
        assert result[2].dpu == "dpu0"
        assert result[2].rshim == "rshim0"
        assert result[2].config_path == "/cfg/dpu0.json"
        assert result[2].dpu_pci_bus_id == "0000:01:00.0"
        assert result[2].rshim_pci_bus_id == "0000:01:00.1"
        assert result[3].dpu == "dpu2"
        assert result[3].rshim == "rshim2"
        assert result[3].config_path == "/cfg/dpu2.json"
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
                    configs=None,
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
                    configs=None,
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
                configs=None,
                script_name="test",
                print_usage_callback=print_usage,
            )
        assert len(result) == 2
        assert result[0].dpu == "dpu0"
        assert result[0].rshim == "rshim0"
        assert result[0].dpu_pci_bus_id == "0000:01:00.0"
        assert result[0].rshim_pci_bus_id == "0000:02:00.0"
        assert result[0].config_path is None
        assert result[1].dpu == "dpu1"
        assert result[1].rshim == "rshim1"
        assert result[1].dpu_pci_bus_id == "0000:03:00.0"
        assert result[1].rshim_pci_bus_id == "0000:04:00.0"
        assert result[1].config_path is None

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
                configs=None,
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
                    configs=None,
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
