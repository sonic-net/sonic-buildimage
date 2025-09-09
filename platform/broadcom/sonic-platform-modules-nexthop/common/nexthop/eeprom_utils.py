# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import click
import sys
import struct

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NEXTHOP_IANA = "63074"
NEXTHOP_IANA_SIZE = 4  # IANA is 4 bytes
CUSTOM_FIELD_CODE_SIZE = 1  # Custom field code is 1 byte
CUSTOM_SECONDARY_SERIAL_NUM_CODE = 0x01
VENDOR_EXT_STR = "Vendor Extension"

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def decoder(self, s, t):
        if t[0] != self._TLV_CODE_VENDOR_EXT:
            return eeprom_tlvinfo.TlvInfoDecoder.decoder(self, s, t)

        custom_data = t[2:2 + t[1]]

        # Check minimum length (1 + 4 + 1 = 6 bytes minimum)
        if len(custom_data) < 6:
            name = VENDOR_EXT_STR
            return name, "Invalid format - too short"

        # Parse the structure
        iana_bytes = custom_data[1:5]  # 4 bytes for IANA
        custom_field_code = custom_data[5]

        # Verify IANA (convert 4 bytes to integer, big-endian)
        iana_value = struct.unpack('>I', iana_bytes)[0]
        expected_iana = int(NEXTHOP_IANA)

        if iana_value != expected_iana:
            name = VENDOR_EXT_STR
            return name, f"Invalid IANA: {iana_value}, expected {expected_iana}"

        # Verify custom field code
        if custom_field_code != CUSTOM_SECONDARY_SERIAL_NUM_CODE:
            name = VENDOR_EXT_STR
            return name, f"Invalid field code: 0x{custom_field_code:02x}"

        # Extract and decode the actual payload
        payload = custom_data[6:]
        custom_value = payload.decode("utf-8", errors="replace")
        name = "Custom Serial Number"
        return name, custom_value


def format_vendor_ext(custom_payload, custom_field_code):
    """
    Format vendor extension field according to Nexthop specification:
    - Total bytes of custom payload (1 byte)
    - NEXTHOP IANA (4 bytes)
    - Custom field code (1 byte)
    - Custom payload (variable length)

    Args:
        custom_payload (bytes): The custom payload data
        custom_field_code (int): Custom field code (default: 0x01)

    Returns:
        str: Formatted vendor extension string in hex format
    """

    # Ensure custom_payload is bytes
    if not isinstance(custom_payload, bytes):
        raise TypeError("custom_payload must be bytes")

    payload_bytes = custom_payload

    # Calculate total bytes: IANA (4) + custom field code (1) + payload length
    payload_len = len(payload_bytes)

    # Build the vendor extension data
    vendor_ext_data = bytearray()

    # Add total bytes (1 byte)
    vendor_ext_data.append(payload_len)

    # Add NEXTHOP IANA (4 bytes) - convert string to integer then to 4 bytes
    iana_int = int(NEXTHOP_IANA)
    vendor_ext_data.extend(struct.pack('>I', iana_int))  # Big-endian 4-byte integer

    # Add custom field code (1 byte)
    vendor_ext_data.append(custom_field_code & 0xFF)

    # Add custom payload
    vendor_ext_data.extend(payload_bytes)

    # Convert to hex string format expected by eeprom_tlvinfo
    return ' '.join(f'0x{b:02x}' for b in vendor_ext_data)


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
    eeprom_class = Eeprom(
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
    custom_serial_number
):
    eeprom_class = Eeprom(
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
    if custom_serial_number is not None:
        # Format vendor_ext according to Nexthop custom serial number specification
        custom_serial_number_bytes = custom_serial_number.encode('utf-8')
        formatted_vendor_ext_with_custom_serial_number = \
            format_vendor_ext(custom_serial_number_bytes, CUSTOM_SECONDARY_SERIAL_NUM_CODE)
        cmds.append(f"{eeprom_class._TLV_CODE_VENDOR_EXT} = {formatted_vendor_ext_with_custom_serial_number}")

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
@click.option("--custom-serial-number", default=None, help="Custom serial number embedded in vendor extension")
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
    custom_serial_number
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
        custom_serial_number
    )


@cli.command("clear")
@click.argument("eeprom_path", autocompletion=complete_available_eeproms)
def clear(eeprom_path):
    check_root_privileges()
    clear_eeprom(eeprom_path)


if __name__ == "__main__":
    cli()
