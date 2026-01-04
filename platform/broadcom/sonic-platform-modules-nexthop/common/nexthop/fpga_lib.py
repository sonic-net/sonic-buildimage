# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import ctypes
import mmap
import os
import json

from typing import Union
from  nexthop.pddf_config_parser import load_pddf_device_config


def bdf_to_name(device_bdf, pddf_config=None):
    """  Get device name for a given BDF address """

    if pddf_config is None:
        pddf_config = load_pddf_device_config()
    
    for device_config in pddf_config.values():
        if isinstance(device_config, dict) and 'dev_info' in device_config:
            dev_info = device_config['dev_info']
            
            if (dev_info.get('device_type') == 'MULTIFPGAPCIE' and 
                dev_info.get('device_bdf') == device_bdf):
                return dev_info.get('device_name')
    
    return None


def name_to_bdf(device_name, pddf_config=None):
    """  Get BDF address for a given device name """

    if pddf_config is None:
        pddf_config = load_pddf_device_config()
    
    for device_config in pddf_config.values():
        if isinstance(device_config, dict) and 'dev_info' in device_config:
            dev_info = device_config['dev_info']
            
            if (dev_info.get('device_type') == 'MULTIFPGAPCIE' and 
                dev_info.get('device_name') == device_name):
                return dev_info.get('device_bdf')
    
    return None


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
    if offset % 4 != 0:
        raise ValueError(f"Offset ({offset}) must be 32-bit aligned.")
    file_path = get_resource_0_path(pci_address, root)
    with open(file_path, "r+b") as f:
        page_size = mmap.PAGESIZE
        aligned_offset = offset & ~(page_size - 1)
        offset_in_page = offset - aligned_offset
        mm = mmap.mmap(
            f.fileno(),
            length=page_size,
            offset=aligned_offset,
            access=mmap.ACCESS_WRITE,
        )
        mm_buf = (ctypes.c_char * page_size).from_buffer(mm)
        base_ptr = ctypes.cast(mm_buf, ctypes.POINTER(ctypes.c_uint32))
        base_ptr[offset_in_page // 4] = ctypes.c_uint32(val).value


def read_32(pci_address: str, offset: int, root="") -> int:
    if offset % 4 != 0:
        raise ValueError(f"Offset ({offset}) must be 32-bit aligned.")
    file_path = get_resource_0_path(pci_address, root)
    with open(file_path, "r+b") as f:
        with mmap.mmap(
            f.fileno(), length=os.path.getsize(file_path), access=mmap.ACCESS_READ
        ) as mm:
            return int.from_bytes(mm[offset : offset + 4], byteorder="little")


def compute_bitmask(start: int, end: int) -> int:
    "Returns a bitmask with 1s from `start` (inclusive) to `end` (inclusive)."
    if start > end:
        raise ValueError(f"Start bit ({start}) can't be greater than end bit ({end}).")
    # Create a mask with 'num_bits' ones at the rightmost position
    num_bits = end - start + 1
    mask_of_ones = (1 << num_bits) - 1
    # Shift the mask to the desired starting position
    return mask_of_ones << start


def get_field(reg_val: int, bit_range: tuple[int, int]) -> int:
    "Returns the value of the field positioned at `bit_range` in the 32-bit register."
    start, end = bit_range
    mask = compute_bitmask(start, end)
    return (reg_val & mask) >> start


def max_value_for_bit_range(bit_range: tuple[int, int]) -> int:
    "Returns the max value that can be represented within `bit_range`."
    start, end = bit_range
    num_bits = end - start + 1
    return (1 << num_bits) - 1


def overwrite_field(reg_val: int, bit_range: tuple[int, int], field_val: int) -> int:
    "Returns the `reg_val` where the field at `bit_range` is being overwritten with `field_val`."
    if field_val > (max_field_val := max_value_for_bit_range(bit_range)):
        raise ValueError(
            f"field_value (0x{field_val:0x}) must be smaller than or equal to (0x{max_field_val:0x})."
        )
    start, end = bit_range

    # Clear the existing field in the register value.
    bitmask_ignore_field = ctypes.c_uint32(~compute_bitmask(start, end)).value
    cleared_reg = reg_val & bitmask_ignore_field

    # Position the new field value.
    positioned_field_val = field_val << start

    # Combine the cleared register value with the new field value.
    return cleared_reg | positioned_field_val
