"""
FPGA utilities for PDDF-based platforms.

This module provides common FPGA-related utility functions that can be shared
across PDDF-based platforms and utilities.
"""

import shlex
import subprocess

# Bits [0:7] represenets the base function provided by an image
_BASE_FUNCTION_MASK = 0xFF


def is_supported_fpga(
    check_fpga_version_cmd: str,
    min_fpga_version_hex_output: str,
) -> bool:
    """
    Run command to check FPGA version against the requirement.

    This function executes a shell command to retrieve the current FPGA version
    and compares it against a minimum required version. Both versions are expected
    to be hexadecimal strings. Only compares bits [0:7] for determining base
    function provided by the image.

    Args:
        check_fpga_version_cmd: Command (argv string, parsed with shlex.split,
                                no shell features) that returns the current FPGA
                                version as a hexadecimal string (e.g., "0x1234").
                                May start with "sudo" if the underlying tool
                                requires elevated privileges.
        min_fpga_version_hex_output: Minimum required FPGA version as a hexadecimal
                                     string (e.g., "0x1000")

    Returns:
        bool: True if the current FPGA version meets or exceeds the minimum
              required version, False otherwise.

              Special cases:
              - Returns True if min_fpga_version_hex_output is empty/None (no requirement)
              - Returns False if check_fpga_version_cmd is empty/None
              - Returns False if the command fails or returns empty output
              - Returns False if version strings cannot be parsed as hexadecimal

    Raises:
        RuntimeError: if command to check FPGA version fails.
        ValueError: if fails to interpret FPGA version as hexadecimal.

    Example:
        >>> is_supported_fpga("fpga read32 0000:e4:00.0 0x0", "0x1000")
        True  # if FPGA version >= 0x1000
    """
    if not min_fpga_version_hex_output:
        return True
    if not check_fpga_version_cmd:
        return False

    try:
        argv = shlex.split(check_fpga_version_cmd)
    except ValueError as e:
        raise ValueError(
            f"Failed to parse FPGA version check command: {check_fpga_version_cmd}"
        ) from e

    try:
        fpga_version_output = subprocess.check_output(argv, text=True)
    except subprocess.CalledProcessError as e:
        raise RuntimeError(
            f"Failed to execute FPGA version check command: {check_fpga_version_cmd}"
        ) from e

    if not fpga_version_output:
        return False

    try:
        fpga_version = int(fpga_version_output.strip(), 16)
        min_fpga_version = int(min_fpga_version_hex_output, 16)
    except ValueError as e:
        raise ValueError(
            f"Cannot check FPGA version, got fpga_version={fpga_version_output}, "
            f"min_fpga_version={min_fpga_version_hex_output}"
        ) from e

    return fpga_version & _BASE_FUNCTION_MASK >= min_fpga_version & _BASE_FUNCTION_MASK

