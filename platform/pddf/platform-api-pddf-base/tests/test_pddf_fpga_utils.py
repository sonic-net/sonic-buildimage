#!/usr/bin/env python
# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
import pytest
import subprocess
from unittest.mock import patch

# Import the module under test
from sonic_platform_pddf_base.pddf_fpga_utils import is_supported_fpga


class TestIsSupportedFpga:
    """Test cases for the is_supported_fpga function."""

    @pytest.mark.parametrize(
        "check_cmd, min_version",
        [
            ("some_command", ""),
            ("fpga read32 0000:e4:00.0 0x0", ""),
        ],
    )
    def test_no_minimum_version_required_returns_true(self, check_cmd, min_version):
        """When no minimum version is specified, should return True."""
        result = is_supported_fpga(check_cmd, min_version)
        assert result is True

    @pytest.mark.parametrize(
        "check_cmd, min_version",
        [
            ("", "0x2000"),
            (None, "0xFFFF"),
        ],
    )
    def test_no_check_command_returns_false(self, check_cmd, min_version):
        """When no check command is provided, should return False."""
        result = is_supported_fpga(check_cmd, min_version)
        assert result is False

    @pytest.mark.parametrize(
        "fpga_version, min_version, expected_result, description",
        [
            # FPGA version exceeds minimum
            ("0x12", "0x13", False, "shortened revision"),
            ("0x13", "0x12", True, "shortened revision"),
            ("0xe805e306", "0xe8050307", False, "test image, does not meet minimum"),
            ("0xe805ffff", "0xe806e306", True, "major version below minimum ok"),
            ("0x1001", "0x1000", True, "version slightly exceeds minimum"),
            ("0x1000", "0x1000", True, "version equals minimum"),
            ("0x0", "0x0", True, "zero equals zero"),
        ],
    )
    @patch("subprocess.check_output")
    def test_fpga_version_comparison(
        self, mock_check_output, fpga_version, min_version, expected_result, description
    ):
        """Test FPGA version comparison against minimum requirement."""
        mock_check_output.return_value = fpga_version

        result = is_supported_fpga("fpga read32 0000:e4:00.0 0x0", min_version)

        assert result is expected_result, f"Failed: {description}"
        mock_check_output.assert_called_once_with(
            ["fpga", "read32", "0000:e4:00.0", "0x0"], text=True
        )

    @patch("subprocess.check_output")
    def test_command_execution_failures(self, mock_check_output):
        """When command execution fails, should return False and print error."""
        mock_check_output.side_effect = subprocess.CalledProcessError(1, "cmd")

        with pytest.raises(RuntimeError):
            is_supported_fpga("invalid_command", "0x1000")

    @pytest.mark.parametrize(
        "fpga_output, min_version",
        [
            ("not_a_hex_value", "0x1000"),
            ("0x2000", "invalid_hex"),
        ],
    )
    @patch("subprocess.check_output")
    def test_invalid_hex_values(self, mock_check_output, fpga_output, min_version):
        """When hex values are invalid, should return False and print error."""
        mock_check_output.return_value = fpga_output

        with pytest.raises(ValueError):
            is_supported_fpga("fpga read32 0000:e4:00.0 0x0", min_version)


