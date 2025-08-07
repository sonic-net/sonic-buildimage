# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import click

from typing import Union

from nexthop.fpga_lib import (
    find_xilinx_fpgas,
    write_32,
    read_32,
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


@click.group()
def cli():
    check_root_privileges()


@cli.command("write32")
@click.argument("pci_address", autocompletion=complete_available_fpgas)
@click.argument("offset")
@click.argument("value")
def write32(pci_address, offset, value):
    check_valid_pci_address(pci_address)
    write_32(pci_address, int(offset, 16), int(value, 16))
    click.secho("success", fg="green")


@cli.command("read32")
@click.argument("pci_address", autocompletion=complete_available_fpgas)
@click.argument("offset")
def read32(offset, pci_address):
    check_valid_pci_address(pci_address)
    val = read_32(pci_address, int(offset, 16))
    click.secho(f"0x{val:08x}", fg="green")


@cli.command("list")
def cli_list():
    echo_available_fpgas()


if __name__ == "__main__":
    cli()
