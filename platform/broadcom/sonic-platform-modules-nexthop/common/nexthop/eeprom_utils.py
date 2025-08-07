# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import click
import sys

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def get_at24_eeprom_paths(root=""):
    results = []
    i2c_devices_dir = f"{root}/sys/bus/i2c/devices"
    if not os.path.isdir(i2c_devices_dir):
        return results
    for device in os.listdir(i2c_devices_dir):
        name_file_path = os.path.join(i2c_devices_dir, device, "name")
        os.path.isfile(name_file_path)
        with open(name_file_path, "r") as f:
            if f.read().strip() == "24c64":
                eeprom_path = os.path.join(i2c_devices_dir, device, "eeprom")
                if os.path.isfile(eeprom_path):
                    results.append(eeprom_path)
    return results


def echo_available_eeproms():
    click.secho("Use one of the following:", fg="cyan")
    for eeprom_path in get_at24_eeprom_paths():
        click.secho(f"{eeprom_path}", fg="green")

def complete_available_eeproms(ctx, args, incomplete):
    eeproms = get_at24_eeprom_paths()
    return [eeprom for eeprom in eeproms if eeprom.startswith(incomplete)]


def decode_eeprom(eeprom_path: str):
    eeprom_class = eeprom_tlvinfo.TlvInfoDecoder(
        eeprom_path, start=0, status="", ro=True
    )
    eeprom = eeprom_class.read_eeprom()
    # will print out contents
    eeprom_class.decode_eeprom(eeprom)


def check_root_privileges():
    if os.getuid() != 0:
        click.secho("Root privileges required for this operation", fg="red")
        sys.exit(1)


def program_eeprom(
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
    vendor_ext,
):
    eeprom_class = eeprom_tlvinfo.TlvInfoDecoder(
        eeprom_path, start=0, status="", ro=True
    )
    tmp_contents = eeprom_class.read_eeprom()
    cmds = []
    if product_name is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_PRODUCT_NAME} = {product_name}")
    if part_num is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_PART_NUMBER} = {part_num}")
    if serial_num is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_SERIAL_NUMBER} = {serial_num}")
    if mac is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_MAC_BASE} = {mac}")
    if device_version is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_DEVICE_VERSION} = {device_version}")
    if label_revision is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_LABEL_REVISION} = {label_revision}")
    if platform_name is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_PLATFORM_NAME} = {platform_name}")
    if manufacturer_name is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_MANUF_NAME} = {manufacturer_name}")
    if vendor_name is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_VENDOR_NAME} = {vendor_name}")
    if service_tag is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_SERVICE_TAG} = {service_tag}")
    if vendor_ext is not None:
        cmds.append(f"{eeprom_class._TLV_CODE_VENDOR_EXT} = {vendor_ext}")

    tmp_contents = eeprom_class.set_eeprom(tmp_contents, cmds)

    eeprom_class.write_eeprom(tmp_contents)


def clear_eeprom(eeprom_path: str):
    size = os.path.getsize(eeprom_path)
    with open(eeprom_path, "r+b") as f:
        f.write(bytearray([0xFF] * size))


@click.group()
def cli():
    pass


@cli.command("list")
def cli_list():
    echo_available_eeproms()


@cli.command("decode")
@click.argument("eeprom_path", autocompletion=complete_available_eeproms)
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
@click.argument("eeprom_path", autocompletion=complete_available_eeproms)
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
@click.option("--vendor-ext", default=None)
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
    vendor_ext,
):
    check_root_privileges()
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
        vendor_ext,
    )


@cli.command("clear")
@click.argument("eeprom_path", autocompletion=complete_available_eeproms)
def clear(eeprom_path):
    check_root_privileges()
    clear_eeprom(eeprom_path)


if __name__ == "__main__":
    cli()
