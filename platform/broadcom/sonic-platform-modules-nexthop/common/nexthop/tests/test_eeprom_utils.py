import os
import sys
import tempfile
from typing import Counter

import pytest

# Skip test until click version mismatch resolved.
# Host and bookworm container run different click versions.
# The older host version expects a different bash autocomplete API
pytest.skip("Skipping entire module", allow_module_level=True)

CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(CWD, "../"))
sys.path.append(os.path.join(CWD, "../../"))

# Add path to sonic-platform-common, a dependency of nexthop.eeprom_utils which
# nexthop.eeprom_utils which resides outside of this submodule.
# This test must run within the sonic-buildimage tree.
sys.path.append(os.path.join(CWD, "../../../../../../src/sonic-platform-common"))
from nexthop.eeprom_utils import (
    clear_eeprom,
    decode_eeprom,
    get_at24_eeprom_paths,
    program_eeprom,
)


def create_fake_i2c_device(
    device_name: str, file_to_content: dict[str, str], root: str
):
    device_dir = os.path.join(root, "sys/bus/i2c/devices", device_name)
    os.makedirs(device_dir, exist_ok=True)
    for file_name, content in file_to_content.items():
        with open(os.path.join(device_dir, file_name), "x") as f:
            f.write(f"{content}\n")


def test_get_find_at24_eeprom_paths():
    # Given
    root = tempfile.mktemp()
    create_fake_i2c_device("i2c-0", {"name": "i2c-pci-3"}, root)
    create_fake_i2c_device("11-0050", {"name": "24c64", "eeprom": ""}, root)
    create_fake_i2c_device("8-0050", {"name": "24c64"}, root)
    create_fake_i2c_device("9-0050", {"name": "24c64", "eeprom": "32423"}, root)

    # When
    eeprom_paths = get_at24_eeprom_paths(root)

    # Then
    assert Counter(eeprom_paths) == Counter(
        [
            os.path.join(root, "sys/bus/i2c/devices/11-0050/eeprom"),
            os.path.join(root, "sys/bus/i2c/devices/9-0050/eeprom"),
        ]
    )


EEPROM_SIZE = 8192


def create_fake_eeprom(path_to_file):
    with open(path_to_file, "w+b") as f:
        f.write(bytearray([0xFF] * EEPROM_SIZE))


def test_program_and_decode(capsys):
    # Given
    root = tempfile.mktemp()
    os.makedirs(root)
    eeprom_path = os.path.join(root, "eeprom")
    create_fake_eeprom(eeprom_path)

    # When
    program_eeprom(
        eeprom_path=eeprom_path,
        product_name="NH-4010",
        part_num="ABC",
        serial_num="XYZ",
        mac="00:E1:4C:68:00:C4",
        device_version="0",
        label_revision="P0",
        platform_name="x86_64-nexthop_4010-r0",
        manufacturer_name="Nexthop",
        vendor_name="Nexthop",
        service_tag="www.nexthop.ai",
        vendor_ext="123",
    )

    expected = """\
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 101
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   7 NH-4010
Part Number          0x22   3 ABC
Serial Number        0x23   3 XYZ
Base MAC Address     0x24   6 00:E1:4C:68:00:C4
Device Version       0x26   1 0
Label Revision       0x27   2 P0
Platform Name        0x28  22 x86_64-nexthop_4010-r0
Manufacturer         0x2B   7 Nexthop
Vendor Name          0x2D   7 Nexthop
Service Tag          0x2F  14 www.nexthop.ai
Vendor Extension     0xFD   1 0x7B
CRC-32               0xFE   4 0xA8941891
"""

    # Then
    decode_eeprom(eeprom_path)
    out, _ = capsys.readouterr()
    assert expected in out


def test_clear(capsys):
    # Given
    root = tempfile.mktemp()
    os.makedirs(root)
    eeprom_path = os.path.join(root, "eeprom")
    create_fake_eeprom(eeprom_path)
    program_eeprom(
        eeprom_path=eeprom_path,
        product_name="NH-4010",
        part_num="ABC",
        serial_num="XYZ",
        mac="00:E1:4C:68:00:C4",
        device_version="0",
        label_revision="P0",
        platform_name="x86_64-nexthop_4010-r0",
        manufacturer_name="Nexthop",
        vendor_name="Nexthop",
        service_tag="www.nexthop.ai",
        vendor_ext="123",
    )

    # When
    clear_eeprom(eeprom_path)

    # Then
    decode_eeprom(eeprom_path)
    out, _ = capsys.readouterr()
    assert "EEPROM does not contain data in a valid TlvInfo format" in out
