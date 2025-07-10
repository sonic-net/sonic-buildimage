# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import ctypes
import mmap
import os
from typing import Union


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
        page_size = mmap.PAGESIZE
        aligned_offset = offset & ~(page_size - 1)
        offset_in_page = offset - aligned_offset
        mm = mmap.mmap(
            f.fileno(), length=page_size, offset=aligned_offset, access=mmap.ACCESS_WRITE
        )
        mm_buf = (ctypes.c_char * page_size).from_buffer(mm)
        base_ptr = ctypes.cast(mm_buf, ctypes.POINTER(ctypes.c_uint32))
        base_ptr[offset_in_page // 4] = ctypes.c_uint32(val).value


def read_32(pci_address: str, offset: int, root="") -> int:
    file_path = get_resource_0_path(pci_address, root)
    with open(file_path, "r+b") as f:
        mm = mmap.mmap(
            f.fileno(), length=os.path.getsize(file_path), access=mmap.ACCESS_READ
        )
        return int.from_bytes(mm[offset : offset + 4], byteorder="little")
