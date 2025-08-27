# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import re
import sys
import click

from nexthop.fpga_lib import (
    find_xilinx_fpgas,
    write_32,
    read_32,
    max_value_for_bit_range,
    get_field,
    overwrite_field,
)


def check_root_privileges():
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


def echo_available_fpgas():
    xilinx_fpgas = find_xilinx_fpgas()
    click.secho("Use one of the following:", fg="cyan")
    for xilinx_fpga in xilinx_fpgas:
        click.secho(f"{xilinx_fpga}", fg="green")


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


def click_argument_pci_address():
    "Returns a click.argument with shell autocomplete to hint available FPGAs on the system."
    # click version 8.0 renamed `autocompletion` to `shell_complete`.
    # This is to support both old versions and new versions.
    if hasattr(click.Parameter, "shell_complete"):
        return click.argument("pci_address", shell_complete=complete_available_fpgas)
    else:
        return click.argument("pci_address", autocompletion=complete_available_fpgas)


@cli.command("write32")
@click_argument_pci_address()
@click.argument("offset", type=AlignedOffset())
@click.argument("value")
@click.option(
    "--bits",
    type=BitRange(),
    default="0:31",
    help="Inclusive range of bits to write to (e.g., '0:31').",
)
def write32(pci_address, offset, value, bits):
    value = int(value, 16)
    check_value_fits_in_bit_range(value, bits)
    check_valid_pci_address(pci_address)
    if bits != (0, 31):
        old_value = read_32(pci_address, offset)
        value = overwrite_field(old_value, bits, value)
    write_32(pci_address, offset, value)
    click.secho("success", fg="green")


@cli.command("read32")
@click_argument_pci_address()
@click.argument("offset", type=AlignedOffset())
@click.option(
    "--bits",
    type=BitRange(),
    default="0:31",
    help="Inclusive range of bits to read from (e.g., '0:31').",
)
def read32(offset, pci_address, bits):
    check_valid_pci_address(pci_address)
    val = read_32(pci_address, offset)
    val = get_field(val, bits)
    click.secho(f"0x{val:08x}", fg="green")


@cli.command("list")
def cli_list():
    echo_available_fpgas()


if __name__ == "__main__":
    cli()
