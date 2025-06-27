import os
import sys
from typing import Counter

import pytest
import tempfile
from dataclasses import dataclass

# Skip test until click version mismatch resolved.
# Host and bookworm container run different click versions.
# The older host version expects a different bash autocomplete API
pytest.skip("Skipping entire module", allow_module_level=True)

from nexthop.fpga_utils import (
    find_pci_devices,
    find_xilinx_fpgas,
    read_32,
    write_32,
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
    root = tempfile.mktemp()

    # When
    for pci_device in pci_devices:
        create_fake_pci_device(root, pci_device)

    # Then
    pci_devices = find_pci_devices(target_vendor_id, target_device_id, root)
    assert Counter(pci_devices) == Counter(expected)


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
    root = tempfile.mktemp()

    # When
    for pci_device in pci_devices:
        create_fake_pci_device(root, pci_device)

    # Then
    xilinx_devices = find_xilinx_fpgas(root)
    assert Counter(xilinx_devices) == Counter(expected)


# file is initialized to zero
def create_fake_resource0(pci_address: str, root: str):
    device_path = os.path.join(root, "sys", "bus", "pci", "devices", pci_address)
    os.makedirs(device_path)
    resource_0_path = os.path.join(device_path, "resource0")
    with open(resource_0_path, "wb") as f:
        f.write(bytearray(0x100000))


def test_read_write():
    # Create fake resource0 to read/write to
    root = tempfile.mktemp()
    pci_address = "0000:00:02.0"
    create_fake_resource0(pci_address, root)

    # file is initially all zeroes
    assert read_32(pci_address, offset=0x100000 - 1 - 4, root=root) == 0
    assert read_32(pci_address, offset=0x0, root=root) == 0
    assert read_32(pci_address, offset=0x4, root=root) == 0
    assert read_32(pci_address, offset=0x8, root=root) == 0

    write_32(pci_address, offset=0x100000 - 1 - 4, val=1, root=root)
    write_32(pci_address, offset=0x0, val=2, root=root)
    write_32(pci_address, offset=0x4, val=3, root=root)
    write_32(pci_address, offset=0x8, val=4, root=root)

    assert read_32(pci_address, offset=0x100000 - 1 - 4, root=root) == 1
    assert read_32(pci_address, offset=0x0, root=root) == 2
    assert read_32(pci_address, offset=0x4, root=root) == 3
    assert read_32(pci_address, offset=0x8, root=root) == 4
