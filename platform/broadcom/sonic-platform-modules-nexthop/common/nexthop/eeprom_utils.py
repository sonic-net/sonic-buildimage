# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import click
import sys
import struct

from dataclasses import dataclass
from enum import Enum

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NEXTHOP_IANA = "63074"
NEXTHOP_IANA_SIZE = 4  # IANA is 4 bytes
VENDOR_EXT_STR = "Vendor Extension"


class CustomField(Enum):
    """
    Enum for Nexthop custom EEPROM fields used for "Vendor Extension"
    TLV programming and decoding.
    """

    SECONDARY_SERIAL_NUMBER = (0x01, "Custom Serial Number")
    REGULATORY_MODEL_NUMBER = (0x02, "Regulatory Model Number")

    def __init__(self, code, display_name):
        self.code = code
        self.display_name = display_name

    @classmethod
    def get_by_code(cls, code):
        for field in cls:
            if field.code == code:
                return field
        return None


@dataclass
class CustomFieldStruct:
    iana: bytearray
    code: int
    payload: bytearray


def big_endian_to_int(bytes: bytearray) -> int:
    return struct.unpack(">I", bytes)[0]


def tlv_to_custom_field_struct(t: bytearray) -> tuple[CustomFieldStruct | None, str]:
    """
    Parses the given TLV in the form of bytes into a CustomFieldStruct.
    Returns (None, error_message) if the given TLV bytes is not a valid Vendor Extension TLV.

    Vendor Extension TLV schema:
    Byte | Name
    -------------
    0    |  Type
    1    |  Payload length (including IANA, Custom field code, and Custom payload)
    2-5  |  IANA
    6    |  Custom field code
    7+   |  Custom payload (variable length)
    """
    # Check minimum TLV length (1 + 1 + 4 + 1 = 7 bytes minimum)
    if len(t) < 7:
        return None, "Invalid format - too short"

    if t[0] != eeprom_tlvinfo.TlvInfoDecoder._TLV_CODE_VENDOR_EXT:
        return None, "Not a Vendor Extension TLV"

    # Check minimum payload length (4 + 1 = 5 bytes minimum)
    if t[1] < 5:
        return None, "Invalid payload length - too short"

    # Parse the structure
    iana_bytes = t[2:6]
    custom_field_code = t[6]
    payload = t[7 : 2 + t[1]]
    # ============= FOR BACKWARD COMPATIBILITY =============
    # Some early units may have a garbage value at byte 2
    # of the "Custom Serial Number" (code 0x01) TLV. So,
    # IANA, custom field code, and custom payload are all
    # shifted right by 1 byte. This special check is so we
    # can decode the "Custom Serial Number" for those units.
    # Although, the value of payload length at byte 1 is
    # still correct.
    if (
        big_endian_to_int(iana_bytes) != int(NEXTHOP_IANA)
        and len(t) >= 8
        and t[1] >= 6
        and big_endian_to_int(t[3:7]) == int(NEXTHOP_IANA)
        and t[7] == CustomField.SECONDARY_SERIAL_NUMBER.code
    ):
        iana_bytes = t[3:7]
        custom_field_code = t[7]
        payload = t[8 : 2 + t[1]]
    # ======================================================
    return CustomFieldStruct(iana_bytes, custom_field_code, payload), ""


class NexthopEepromDecodeVisitor(eeprom_tlvinfo.EepromDecodeVisitor):
    """
    Custom visitor class, which lengthen the "TLV Name" column to 25 characters.
    """
    def visit_header(self, eeprom_id, version, header_length):
        if eeprom_id is not None:
            print("TlvInfo Header:")
            print("   Id String:    %s" % eeprom_id)
            print("   Version:      %d" % version)
            print("   Total Length: %d" % header_length)
            print("TLV Name                  Code Len Value")
            print("------------------------- ---- --- -----")

    def visit_tlv(self, name, code, length, value):
         print("%-25s 0x%02X %3d %s" % (name, code, length, value))


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def decoder(self, s, t):
        # Vendor Extension TLV schema:
        # Byte | Name
        # -------------
        # 0    |  Type
        # 1    |  Payload length (including IANA, Custom field code, and Custom payload)
        # 2-5  |  IANA
        # 6    |  Custom field code
        # 7+   |  Custom payload (variable length)
        if t[0] != self._TLV_CODE_VENDOR_EXT:
            return eeprom_tlvinfo.TlvInfoDecoder.decoder(self, s, t)

        # Parse the structure
        custom_field_struct, err = tlv_to_custom_field_struct(t)
        if custom_field_struct is None:
            name = VENDOR_EXT_STR
            return name, err

        # Verify IANA
        iana = big_endian_to_int(custom_field_struct.iana)
        if iana != int(NEXTHOP_IANA):
            name = VENDOR_EXT_STR
            return (
                name,
                f"Invalid IANA: {iana}, expected {NEXTHOP_IANA}",
            )

        # Verify custom field code
        custom_field = CustomField.get_by_code(custom_field_struct.code)
        if custom_field is None:
            name = VENDOR_EXT_STR
            return name, f"Invalid field code: 0x{custom_field_struct.code:02x}"

        # Extract and decode the actual payload
        custom_value = custom_field_struct.payload.decode("utf-8", errors="replace")
        name = custom_field.display_name
        return name, custom_value

    def _encode_arg_to_tlv_bytes(self, arg: str) -> bytearray:
        """
        Convert a string in the format of "{type} = {payload}", where payload is
        space-separated hexidecimal numbers, to a TLV bytearray consisting of
        type (1 byte) + payload length (1 byte) + payload (variable length).
        """
        type, payload = arg.split("=")
        type = int(type.strip(), base=0)
        payload = payload.strip()
        tlv = self.encoder((type,), payload)
        return tlv

    def _find_tlv_vendor_ext_start_offset(
        self, e: bytearray, iana: bytearray, custom_field_code: int
    ) -> int | None:
        """
        If exists in the given EEPROM data, returns the start index of the
        Vendor Extension TLV which contains the given IANA (bytes [2,5]) and
        the given custom field code (6th byte).
        Returns None if not found.
        """
        tlv_start = 0
        # Iterate through TLVs until we find the matching Vendor Extension TLV
        while tlv_start < len(e) and self.is_valid_tlv(e[tlv_start:]):
            # Check if this is the Vendor Extension TLV we're looking for.
            tlv_end = tlv_start + 2 + e[tlv_start + 1]
            custom_field_struct, _ = tlv_to_custom_field_struct(e[tlv_start:tlv_end])
            if (
                custom_field_struct is not None
                and custom_field_struct.iana == iana
                and custom_field_struct.code == custom_field_code
            ):
                return tlv_start

            # Move to the next TLV.
            tlv_start = tlv_end
        return None

    def _remove_header_and_checksum_tlv(self, e: bytearray) -> bytearray:
        """
        Returns the given EEPROM data with the header and the checksum TLV removed.
        """
        # Skip header.
        tlv_start = 0
        if self._TLV_HDR_ENABLED:
            tlv_start = self._TLV_INFO_HDR_LEN
            total_length = (e[9] << 8) | e[10]
            e_end = self._TLV_INFO_HDR_LEN + total_length
        else:
            tlv_start = self.eeprom_start
            e_end = min(self._TLV_INFO_MAX_LEN, self.eeprom_max_len)

        # Include all TLVs but not the checksum TLV which is always the last TLV.
        # TLV schema:
        # Byte | Name
        # ------------
        # 0    |  Type
        # 1    |  Payload length
        # 2+   |  Payload (variable length)
        new_e = bytearray()
        while (
            tlv_start < len(e)
            and tlv_start < e_end
            and self.is_valid_tlv(e[tlv_start:])
            and e[tlv_start] != self._TLV_CODE_CRC_32
        ):
            tlv_end = tlv_start + 2 + e[tlv_start + 1]
            new_e += e[tlv_start:tlv_end]
            tlv_start = tlv_end
        return new_e

    def set_eeprom(self, e: bytearray, cmd_args: list[str]) -> bytearray:
        """
        Overrides parent class method to support multiple vendor extension TLVs
        with the same type (0xFD) but different IANA + custom field code.
        For example, Nexthop's "Custom Serial Number" (0x01) and
        "Regulatory Model Number" (0x02) can co-exist even though they
        share the same type (0xFD).

        Returns the new contents of the EEPROM where the given `cmd_args` are applied
        to the given EEPROM data `e`. Each command must be in the format of
        "{type} = {payload}", where payload is space-separated hexidecimal numbers
        and type must be parsable into an integer.
        """
        # Split commands into two lists: one for vendor ext, one for everything else.
        cmd_args_for_vendor_ext = []
        cmd_args_without_vendor_ext = []
        for arg_str in cmd_args:
            for arg in arg_str.split(","):
                tlv_bytes = self._encode_arg_to_tlv_bytes(arg)
                # type is at 0th byte
                if tlv_bytes[0] == self._TLV_CODE_VENDOR_EXT:
                    cmd_args_for_vendor_ext.append(arg)
                else:
                    cmd_args_without_vendor_ext.append(arg)

        # Use parent class to set non-vendor ext TLV (where duplicate type is not allowed).
        if cmd_args_without_vendor_ext:
            click.secho("EEPROM data with updated fields:", fg="green")
            e = eeprom_tlvinfo.TlvInfoDecoder.set_eeprom(
                self, e, cmd_args_without_vendor_ext
            )

        if not cmd_args_for_vendor_ext:
            return e

        # Remove header and checksum which will be re-calculated and re-added later
        e = self._remove_header_and_checksum_tlv(e)
        # Set vendor ext TLVs. Duplicate type (0xFD) is allowed,
        # but duplicate (IANA + custom field code) is not allowed
        # and will be overwritten.
        for arg in cmd_args_for_vendor_ext:
            tlv_bytes = self._encode_arg_to_tlv_bytes(arg)
            old_tlv_start = self._find_tlv_vendor_ext_start_offset(
                e, iana=tlv_bytes[2:6], custom_field_code=tlv_bytes[6]
            )
            if old_tlv_start is not None:
                # Replace existing TLV with new one
                # TLV schema:
                # Byte | Name
                # ------------
                # 0    |  Type
                # 1    |  Payload length
                # 2+   |  Payload (variable length)
                old_tlv_end = old_tlv_start + 2 + e[old_tlv_start + 1]
                e = e[:old_tlv_start] + tlv_bytes + e[old_tlv_end:]
            else:
                e += tlv_bytes
        # Add back header
        if self._TLV_HDR_ENABLED:
            # Assuming CRC-32 is used, its length is 6 bytes (type + length + 4-byte value)
            total_len = len(e) + 6
            # Header schema:
            # Byte | Name
            # ------------
            # 0-7  |  ID String
            # 8    |  Header version
            # 9-10 |  Total length (for the bytes to follow; not including header)
            e = (
                self._TLV_INFO_ID_STRING
                + bytearray([self._TLV_INFO_VERSION])
                + bytearray([(total_len >> 8) & 0xFF, total_len & 0xFF])
                + e
            )
        # Add back checksum TLV. We assume CRC-32 is used, so it matches the length in the header.
        # Checksum is calculated from the start of header to the end of checksum TL but not V.
        assert self._TLV_CODE_CRC_32 != self._TLV_CODE_UNDEFINED
        e = e + bytearray([self._TLV_CODE_CRC_32]) + bytearray([4])
        e += self.encode_checksum(self.calculate_checksum(e))

        # Print out the contents.
        click.secho("EEPROM data with updated vendor extensions:", fg="green")
        self.decode_eeprom(e)

        if len(e) > min(self._TLV_INFO_MAX_LEN, self.eeprom_max_len):
            click.secho(
                f"\nERROR: There is not enough room in the EEPROM to save data.\n",
                fg="red",
            )
            sys.exit(1)

        return e

    def decode_eeprom(self, e):
        visitor = NexthopEepromDecodeVisitor()
        self.visit_eeprom(e, visitor)


def format_vendor_ext(custom_payload, custom_field_code):
    """
    Format vendor extension field according to Nexthop specification:
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

    # Build the vendor extension data
    vendor_ext_data = bytearray()

    # Add NEXTHOP IANA (4 bytes) - convert string to integer then to 4 bytes
    iana_int = int(NEXTHOP_IANA)
    vendor_ext_data.extend(struct.pack(">I", iana_int))  # Big-endian 4-byte integer

    # Add custom field code (1 byte)
    vendor_ext_data.append(custom_field_code & 0xFF)

    # Add custom payload
    vendor_ext_data.extend(custom_payload)

    # Convert to hex string format expected by eeprom_tlvinfo
    return " ".join(f"0x{b:02x}" for b in vendor_ext_data)


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


def click_argument_eeprom_path():
    "Returns a click.argument with shell autocomplete to hint available EEPROM paths on the system."
    # click version 8.0 renamed `autocompletion` to `shell_complete`.
    # This is to support both old versions and new versions.
    if hasattr(click.Parameter, "shell_complete"):
        return click.argument("eeprom_path", shell_complete=complete_available_eeproms)
    else:
        return click.argument("eeprom_path", autocompletion=complete_available_eeproms)


def decode_eeprom(eeprom_path: str):
    eeprom_class = Eeprom(eeprom_path, start=0, status="", ro=True)
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
    custom_serial_number,
    regulatory_model_number,
):
    eeprom_class = Eeprom(eeprom_path, start=0, status="", ro=True)
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

    # Vendor extension fields. See Nexthop custom EEPROM fields in `CustomField` enum above.
    if custom_serial_number is not None:
        payload_hex_str = format_vendor_ext(
            custom_serial_number.encode("utf-8"),
            CustomField.SECONDARY_SERIAL_NUMBER.code,
        )
        cmds.append(f"{eeprom_class._TLV_CODE_VENDOR_EXT} = {payload_hex_str}")
    if regulatory_model_number is not None:
        payload_hex_str = format_vendor_ext(
            regulatory_model_number.encode("utf-8"),
            CustomField.REGULATORY_MODEL_NUMBER.code,
        )
        cmds.append(f"{eeprom_class._TLV_CODE_VENDOR_EXT} = {payload_hex_str}")

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


@cli.command("clear")
@click_argument_eeprom_path()
def clear(eeprom_path):
    check_root_privileges()
    clear_eeprom(eeprom_path)


if __name__ == "__main__":
    cli()
