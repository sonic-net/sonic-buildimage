# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import sys

import click

from nexthop_utils.eeprom_utils import (
    Eeprom,
    clear_eeprom,
    get_at24_eeprom_paths,
    program_eeprom,
)


def decode_eeprom(eeprom_path: str):
    eeprom_class = Eeprom(eeprom_path, start=0, status="", ro=True)
    eeprom = eeprom_class.read_eeprom()
    # will print out contents
    eeprom_class.decode_eeprom(eeprom)


def echo_available_eeproms():
    click.secho("Use one of the following:", fg="cyan")
    for eeprom_path in get_at24_eeprom_paths():
        click.secho(f"{eeprom_path}", fg="green")


def complete_available_eeproms(ctx, args, incomplete):
    eeproms = get_at24_eeprom_paths()
    return [eeprom for eeprom in eeproms if eeprom.startswith(incomplete)]


def click_argument_eeprom_path():
    "Returns a click.argument with shell autocomplete to hint available EEPROM paths on the system."
    # click version 8.0 renamed `autocompletion` to `shell_complete`.
    # This is to support both old versions and new versions.
    if hasattr(click.Parameter, "shell_complete"):
        return click.argument("eeprom_path", shell_complete=complete_available_eeproms)
    else:
        return click.argument("eeprom_path", autocompletion=complete_available_eeproms)


def check_root_privileges():
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


@click.group()
def cli():
    pass


@cli.command("list")
def cli_list():
    echo_available_eeproms()


@cli.command("decode")
@click_argument_eeprom_path()
def decode(eeprom_path):
    check_root_privileges()
    decode_eeprom(eeprom_path)


@cli.command("decode-all")
def decode_all():
    check_root_privileges()
    for eeprom_path in get_at24_eeprom_paths():
        click.secho(f"{eeprom_path}", fg="green")
        decode_eeprom(eeprom_path)


@cli.command("program")
@click_argument_eeprom_path()
@click.option("--product-name", default=None)
@click.option("--part-num", default=None)
@click.option("--serial-num", default=None)
@click.option("--mac", default=None)
@click.option("--device-version", default=None)
@click.option("--label-revision", default=None)
@click.option("--platform-name", default=None)
@click.option("--manufacturer-name", default=None)
@click.option("--vendor-name", default=None)
@click.option("--service-tag", default=None)
@click.option(
    "--custom-serial-number",
    default=None,
    help="Custom serial number embedded in vendor extension",
)
@click.option(
    "--regulatory-model-number",
    default=None,
    help="Regulatory model number embedded in vendor extension",
)
def program(
    eeprom_path,
    product_name,
    part_num,
    serial_num,
    mac,
    device_version,
    label_revision,
    platform_name,
    manufacturer_name,
    vendor_name,
    service_tag,
    custom_serial_number,
    regulatory_model_number,
):
    check_root_privileges()
    try:
        program_eeprom(
            eeprom_path,
            product_name,
            part_num,
            serial_num,
            mac,
            device_version,
            label_revision,
            platform_name,
            manufacturer_name,
            vendor_name,
            service_tag,
            custom_serial_number,
            regulatory_model_number,
        )
    except ValueError as e:
        click.secho(f"\nERROR: {e}\n", fg="red")
        sys.exit(1)


@cli.command("clear")
@click_argument_eeprom_path()
def clear(eeprom_path):
    check_root_privileges()
    clear_eeprom(eeprom_path)


if __name__ == "__main__":
    cli()
