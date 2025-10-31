import os
import sys
import shutil

import pytest
import tempfile

from dataclasses import dataclass
from typing import Counter

from nexthop.fpga_lib import (
    find_pci_devices,
    find_xilinx_fpgas,
    get_resource_0_path,
    read_32,
    write_32,
    compute_bitmask,
    get_field,
    overwrite_field,
)

CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(CWD, "../"))
sys.path.append(os.path.join(CWD, "../../"))


@dataclass
class PciDevice:
    pci_address: str
    vendor_id: int
    device_id: int


def create_fake_pci_device(root: str, pci_device: PciDevice):
    device_path = os.path.join(
        root, "sys", "bus", "pci", "devices", pci_device.pci_address
    )
    os.makedirs(device_path, exist_ok=True)
    file_to_content = {
        "vendor": pci_device.vendor_id,
        "device": pci_device.device_id,
    }
    for file, content in file_to_content.items():
        with open(os.path.join(device_path, file), "w") as f:
            f.write(f"0x{content:04x}\n")
    return device_path


@pytest.mark.parametrize(
    "target_vendor_id, target_device_id, pci_devices, expected",
    [
        # Two devices with the same vendor_id but different device_id
        # We want to query for a specific device_id
        (
            0x1111,
            0x2222,
            [
                PciDevice(
                    pci_address="0000:e3:00.0", vendor_id=0x1111, device_id=0x2222
                ),
                PciDevice(
                    pci_address="0000:00:02.0", vendor_id=0x1111, device_id=0x3333
                ),
            ],
            ["0000:e3:00.0"],
        ),
        # Only query for vendor_id, ignoring device_id
        (
            0x1111,
            None,
            [
                PciDevice(
                    pci_address="0000:e3:00.0", vendor_id=0x1111, device_id=0x2222
                ),
                PciDevice(
                    pci_address="0000:00:02.0", vendor_id=0x1111, device_id=0x3333
                ),
            ],
            ["0000:e3:00.0", "0000:00:02.0"],
        ),
    ],
)
def test_find_pci_devices(target_vendor_id, target_device_id, pci_devices, expected):
    # Given
    root = tempfile.mkdtemp(prefix="test_fpga_lib.")
    try:
        # When
        for pci_device in pci_devices:
            create_fake_pci_device(root, pci_device)

        # Then
        pci_devices = find_pci_devices(target_vendor_id, target_device_id, root)
        assert Counter(pci_devices) == Counter(expected)
    finally:
        shutil.rmtree(root)


@pytest.mark.parametrize(
    "pci_devices, expected",
    [
        # No Xilinx FPGA
        (
            [PciDevice(pci_address="0000:e3:00.0", vendor_id=0x1111, device_id=0x1111)],
            [],
        ),
        # One Xilinx FPGA
        (
            [
                PciDevice("0000:e3:00.0", vendor_id=0x1111, device_id=0x1111),
                PciDevice("0000:00:02.0", vendor_id=0x10EE, device_id=0x7011),
            ],
            ["0000:00:02.0"],
        ),
        # Two Xilinx FPGAs with difference device_id
        (
            [
                PciDevice("0000:e3:00.0", vendor_id=0x1111, device_id=0x1111),
                PciDevice("0000:00:02.0", vendor_id=0x10EE, device_id=0x7011),
                PciDevice("0000:e4:00.6", vendor_id=0x10EE, device_id=0x7012),
            ],
            ["0000:00:02.0", "0000:e4:00.6"],
        ),
    ],
)
def test_find_xilinx_fpgas(pci_devices, expected):
    # Given
    root = tempfile.mkdtemp(prefix="test_fpga_lib.")
    try:
        # When
        for pci_device in pci_devices:
            create_fake_pci_device(root, pci_device)

        # Then
        xilinx_devices = find_xilinx_fpgas(root)
        assert Counter(xilinx_devices) == Counter(expected)
    finally:
        shutil.rmtree(root)


# file is initialized to zero
def create_fake_resource0(pci_address: str, root: str):
    resource_0_path = get_resource_0_path(pci_address, root=root)
    os.makedirs(os.path.dirname(resource_0_path), exist_ok=True)
    with open(resource_0_path, "wb") as f:
        f.write(bytearray(0x100000))


def do_test_read_write(root: str):
    pci_address = "0000:00:02.0"
    create_fake_resource0(pci_address, root)

    # file is initially all zeroes
    assert read_32(pci_address, offset=0x100000 - 4, root=root) == 0
    assert read_32(pci_address, offset=0x0, root=root) == 0
    assert read_32(pci_address, offset=0x4, root=root) == 0
    assert read_32(pci_address, offset=0x8, root=root) == 0
    assert read_32(pci_address, offset=0xC, root=root) == 0

    write_32(pci_address, offset=0x100000 - 4, val=1, root=root)
    write_32(pci_address, offset=0x0, val=2, root=root)
    write_32(pci_address, offset=0x4, val=3, root=root)
    write_32(pci_address, offset=0x8, val=4, root=root)
    with pytest.raises(ValueError):  # Unaligned writes not allowed.
        write_32(pci_address, offset=0xC - 1, val=0xDEADBEEF, root=root)
    write_32(pci_address, offset=0xC, val=0xDEADBEEF, root=root)

    assert read_32(pci_address, offset=0x100000 - 4, root=root) == 1
    assert read_32(pci_address, offset=0x0, root=root) == 2
    assert read_32(pci_address, offset=0x4, root=root) == 3
    assert read_32(pci_address, offset=0x8, root=root) == 4
    assert read_32(pci_address, offset=0xC, root=root) == 0xDEADBEEF


def test_read_write():
    root = tempfile.mkdtemp(prefix="test_fpga_lib.")
    try:
        do_test_read_write(root)
    except AssertionError:
        resource0_path = get_resource_0_path("0000:00:02.0", root=root)
        print(f"\nHexdump of {resource0_path}:")
        with open(resource0_path, "rb") as f:
            content = f.read()
            skip = False
            for i in range(0, len(content), 16):
                chunk = content[i : i + 16]
                if any(b != 0 for b in chunk):
                    hex_str = " ".join(f"{b:02x}" for b in chunk)
                    print(f"{i:08x}: {hex_str}")
                    skip = False
                elif not skip:
                    print(f"{i:08x}: <skipping zeroes>")
                    skip = True
        raise
    finally:
        shutil.rmtree(root)


def test_compute_bitmask():
    # Index: 10987654321098765432109876543210
    assert 0b00000000000000000000000000000001 == compute_bitmask(0, 0)
    assert 0b00000000000000000000000000000011 == compute_bitmask(0, 1)
    assert 0b00000000000000000000000010000000 == compute_bitmask(7, 7)
    assert 0b00000000000000000000001111111100 == compute_bitmask(2, 9)
    assert 0b00000011111111111000000000000000 == compute_bitmask(15, 25)
    assert 0b00000000000111111111110000000000 == compute_bitmask(10, 20)
    assert 0b11000000000000000000000000000000 == compute_bitmask(30, 31)
    assert 0b11111111111111111111111111111111 == compute_bitmask(0, 31)


def test_get_field():
    # Index:           10987654321098765432109876543210
    assert get_field(0b10110011100011110000111110000011, (0, 0)) == 0b1
    assert get_field(0b10110011100011110000111110000011, (1, 2)) == 0b01
    assert get_field(0b10110011100011110000111110000011, (0, 2)) == 0b011
    assert get_field(0b10110011100011110000111110000011, (10, 16)) == 0b1000011
    assert get_field(0b10110011100011110000111110000011, (25, 30)) == 0b011001
    assert get_field(0b10110011100011110000111110000011, (31, 31)) == 0b1
    assert get_field(0xDEADBEEF, (0, 31)) == 0xDEADBEEF


def test_overwrite_field():
    assert overwrite_field(0xFFFFFFFF, (0, 31), field_val=0) == 0
    assert overwrite_field(0b11111111, (0, 5), field_val=0) == 0b11000000
    assert overwrite_field(0b11111111, (2, 4), field_val=0) == 0b11100011
    assert overwrite_field(0b11111111, (3, 7), field_val=0) == 0b00000111

    assert overwrite_field(0b11111111, (2, 4), field_val=0b010) == 0b11101011
    assert overwrite_field(0b00000000, (2, 4), field_val=0b101) == 0b00010100
    assert overwrite_field(0b00000000, (0, 7), field_val=0b11000101) == 0b11000101

    assert overwrite_field(0b0, (0, 1), field_val=0b11) == 0b11
    assert overwrite_field(0b0, (30, 31), field_val=0b11) == (0b11 << 30)


def test_overwrite_field_raises_when_value_exceed_bit_range():
    with pytest.raises(
        ValueError,
        match=r"field_value \(0xff\) must be smaller than or equal to \(0x1f\)",
    ):
        overwrite_field(0xFFFFFFFF, (0, 4), field_val=0xFF)
