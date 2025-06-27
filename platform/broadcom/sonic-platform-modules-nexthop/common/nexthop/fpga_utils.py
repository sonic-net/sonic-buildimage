# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import mmap
import os
import sys
from typing import Union

import click


def find_pci_devices(vendor_id: int, device_id: Union[int, None], root="") -> list[str]:
    "Return pci address strings of devices matching the vendor_id and device_id."
    pci_devices_dir = f"{root}/sys/bus/pci/devices"
    results = []

    for pci_address in os.listdir(pci_devices_dir):
        device_path = os.path.join(pci_devices_dir, pci_address)

        vendor_file = os.path.join(device_path, "vendor")
        device_file = os.path.join(device_path, "device")

        if os.path.exists(vendor_file) and os.path.exists(device_file):
            with open(vendor_file, "r") as v_file, open(device_file, "r") as d_file:
                tmp_vendor_id = int(v_file.read().strip(), 16)
                tmp_device_id = int(d_file.read().strip(), 16)

                if tmp_vendor_id == vendor_id and (
                    device_id is None or tmp_device_id == device_id
                ):
                    results.append(pci_address)

    return results


def find_xilinx_fpgas(root="") -> list[str]:
    "Return list of pci address strings of system Xilinx FPGAs"
    return find_pci_devices(vendor_id=0x10EE, device_id=None, root=root)


def get_resource_0_path(pci_address: str, root="") -> str:
    return f"{root}/sys/bus/pci/devices/{pci_address}/resource0"


def write_32(pci_address: str, offset: int, val: int, root=""):
    file_path = get_resource_0_path(pci_address, root)
    with open(file_path, "r+b") as f:
        mm = mmap.mmap(
            f.fileno(), length=os.path.getsize(file_path), access=mmap.ACCESS_WRITE
        )
        mm[offset : offset + 4] = val.to_bytes(4, byteorder="little")


def read_32(pci_address: str, offset: int, root="") -> int:
    file_path = get_resource_0_path(pci_address, root)
    with open(file_path, "r+b") as f:
        mm = mmap.mmap(
            f.fileno(), length=os.path.getsize(file_path), access=mmap.ACCESS_READ
        )
        return int.from_bytes(mm[offset : offset + 4], byteorder="little")


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
