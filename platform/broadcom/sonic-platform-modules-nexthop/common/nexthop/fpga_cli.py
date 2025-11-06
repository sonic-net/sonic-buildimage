# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import re
import sys
import click
import json

from nexthop.fpga_lib import (
    find_xilinx_fpgas,
    write_32,
    read_32,
    max_value_for_bit_range,
    get_field,
    overwrite_field,
    bdf_to_name,
    name_to_bdf,
)
from  nexthop.pddf_config_parser import load_pddf_device_config

def check_root_privileges():
    """Check if the current user has root privileges and exit if not."""
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


def echo_available_fpgas():
    xilinx_fpgas = find_xilinx_fpgas()
    pddf_config = load_pddf_device_config()
    click.secho("Available FPGAs (you can use either NAME or PCIe ADDRESS):", fg="cyan")
    click.secho(f"{'NAME':<32} PCIe ADDRESS", fg="cyan")
    click.secho(f"{'-' * 32} {'-' * 13}", fg="cyan")

    for fpga_bdf in xilinx_fpgas:
        device_name = bdf_to_name(fpga_bdf, pddf_config)
        if device_name:
            click.secho(f"{device_name:<32} {fpga_bdf}", fg="green")
        else:
            click.secho(f"{'UNKNOWN':<32} {fpga_bdf}", fg="yellow")


def check_valid_pci_address(pci_address):
    xilinx_fpgas = find_xilinx_fpgas()
    if not xilinx_fpgas:
        click.secho("No FPGAs found", fg="red")
        sys.exit(1)

    if pci_address not in xilinx_fpgas:
        click.secho("Invalid pci address", fg="red")
        echo_available_fpgas()
        sys.exit(1)


def complete_available_fpgas(ctx, args, incomplete):
    list_of_fpgas = find_xilinx_fpgas()
    return [fpga for fpga in list_of_fpgas if fpga.startswith(incomplete)]


class AlignedOffset(click.ParamType):
    name = "aligned_offset"

    def convert(self, value, param, ctx) -> int:
        try:
            offset = int(value, 16)
        except ValueError:
            self.fail(f"Offset ({value}) must be a valid hex number", param, ctx)
        if offset % 4 != 0:
            self.fail(f"Offset ({value}) must be 4 byte aligned", param, ctx)
        return offset


class BitRange(click.ParamType):
    name = "bit_range"

    def convert(self, value, param, ctx) -> tuple[int, int]:
        """Converts a string of the form '<start>:<end>' to a tuple (start, end).

        Validates that the start and end are within [0, 31] and that start <= end.
        """
        match = re.search(pattern=r"^(\d+):(\d+)$", string=value)
        if match is None:
            self.fail(
                f"'{value}'. Expected format: '<start>:<end>' (e.g., '16:31').",
                param,
                ctx,
            )
        start, end = int(match.group(1)), int(match.group(2))

        if start > 31 or end > 31:
            self.fail(
                f"Start bit ({start}) and end bit ({end}) must be within [0, 31].",
                param,
                ctx,
            )
        if start > end:
            self.fail(
                f"Start bit ({start}) can't be greater than end bit ({end}).",
                param,
                ctx,
            )

        return start, end


def check_value_fits_in_bit_range(value: int, bit_range: tuple[int, int]):
    "Raises BadParameter if `value` exceeds the max value that `bit_range` can represent."
    if value > (max_value := max_value_for_bit_range(bit_range)):
        raise click.BadParameter(
            f"value (0x{value:0x}) must be smaller than or equal to 0x{max_value:0x}."
        )


@click.group()
def cli():
    check_root_privileges()


def complete_available_fpga_names(ctx, args, incomplete):
    xilinx_fpgas = find_xilinx_fpgas()
    pddf_config = load_pddf_device_config()
    fpga_names = []
    
    for fpga_bdf in xilinx_fpgas:
        device_name = bdf_to_name(fpga_bdf, pddf_config)
        if device_name:
            fpga_names.append(device_name)
    
    incomplete_lower = incomplete.lower()
    return [name for name in fpga_names if name.lower().startswith(incomplete_lower)]


class FpgaNameOrAddress(click.ParamType):
    name = "fpga_name_or_address"

    def convert(self, value, param, ctx) -> str:
        # First check if it's already a valid PCI address
        xilinx_fpgas = find_xilinx_fpgas()
        if value in xilinx_fpgas:
            return value
        
        # Try to convert from name to BDF
        pci_address = name_to_bdf(value)
        if pci_address:
            return pci_address
        
        # If neither works, show error with available options
        self.fail(f"FPGA '{value}' not found. Use 'fpga list' to see available FPGAs.", param, ctx)


def click_argument_target_fpga():
    "Returns a click.argument with shell autocomplete to hint available FPGA names on the system."
    # click version 8.0 renamed `autocompletion` to `shell_complete`.
    # This is to support both old versions and new versions.
    if hasattr(click.Parameter, "shell_complete"):
        return click.argument("pci_address", type=FpgaNameOrAddress(), shell_complete=complete_available_fpga_names)
    else:
        return click.argument("pci_address", type=FpgaNameOrAddress(), autocompletion=complete_available_fpga_names)
    

@cli.command("write32")
@click_argument_target_fpga()
@click.argument("offset", type=AlignedOffset())
@click.argument("value")
@click.option(
    "--bits",
    type=BitRange(),
    default="0:31",
    help="Inclusive range of bits to write to (e.g., '0:31').",
)
def write32(pci_address, offset, value, bits):
    """Write 32-bit value to FPGA register.
    
    TARGET_FPGA can be either the FPGA name (e.g., 'SWITCHCARD_FPGA') or 
    PCIe address (e.g., '0000:e4:00.0'). Use 'fpga list' to see available options.
    """
    value = int(value, 16)
    check_value_fits_in_bit_range(value, bits)
    check_valid_pci_address(pci_address)
    if bits != (0, 31):
        old_value = read_32(pci_address, offset)
        value = overwrite_field(old_value, bits, value)
    write_32(pci_address, offset, value)
    click.secho("success", fg="green")


@cli.command("read32")
@click_argument_target_fpga()
@click.argument("offset", type=AlignedOffset())
@click.option(
    "--bits",
    type=BitRange(),
    default="0:31",
    help="Inclusive range of bits to read from (e.g., '0:31').",
)
def read32(offset, pci_address, bits):
    """Read 32-bit value from FPGA register.
    
    TARGET_FPGA can be either the FPGA name (e.g., 'CPU_CARD_FPGA') or 
    PCIe address (e.g., '0000:e3:00.0'). Use 'fpga list' to see available options.
    """ 
    check_valid_pci_address(pci_address)
    val = read_32(pci_address, offset)
    val = get_field(val, bits)
    click.secho(f"0x{val:08x}", fg="green")


@cli.command("list")
def cli_list():
    echo_available_fpgas()


if __name__ == "__main__":
    cli()
