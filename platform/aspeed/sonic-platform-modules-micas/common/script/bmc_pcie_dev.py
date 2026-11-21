#!/usr/bin/env python3
import os
import sys
import struct
import mmap
import time
import logging
import zlib
import click
import traceback
from platform_util import get_value, setup_logger, BSP_COMMON_LOG_DIR
from platform_config import get_config_param
from public.platform_common_config import BAR_INFO_PATH

DEBUG_FILE = "/etc/.bmc_pcie_dev_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "bmc_pcie_dev_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def pcie_dev_error(s, exc_info=False):
    if exc_info:
        logger.error(s, exc_info=True)
    else:
        logger.error(s)

def pcie_dev_info(s):
    logger.info(s)

def pcie_dev_warn(s):
    logger.warning(s)

RETRY_INTERVAL = 10
TIMEOUT = 1800
ROOT_DESC_HEADER_MAGIC_NUM = 0xDEADBEEF
ROOT_DESC_DATA_MAGIC_NUM = 0xBEAFDEAD
ROOT_DESC_TOTAL_SIZE = 0x400
ROOT_DESC_HEADER_VERSION = 1
# ROOT_DESC_HEADER_SIZE is calculated dynamically now
FUNCTION_BLOCK_HEADER_VERSION = 1

# PCIE_EP memory space structure and length
ROOT_DESC_HEADER_LEN = {
    "magic_num": 4,
    "header_size": 4,
    "version": 4,
    "num_function_blocks": 4,
    "root_data_offset": 4,
    "crc": 4,
}

ROOT_DESC_DATA_LEN = {
    "magic_num": 4,
    "fb_offset": 4,
    "crc": 4,
}

FUNCTION_BLOCK_HEADER_LEN = {
    "timestamp1": 8,
    "timestamp2": 8,
    "function_type": 4,
    "version": 4,
    "data_size": 4,
    "data_offset": 4,
    "data_valid": 1,
    "data_used": 1,
    "direction": 1,
}

# FUNCTION_BLOCK type definitions
FUNCTION_TYPE = {
    "board_info": 0x1,
    "retimer":    0x2,
}

# TLV data types - grouped by Function Block type
BOARD_INFO_TLV_TYPES = {
    "board_id":         0x1,
    "uid":              0x2,
    "rack_sn":          0x3,
    "tray_sn":          0x4,
    "tray_id":          0x5,
    "tray_type":        0x6,
}

RETIMER_TLV_TYPES = {
    "retimer_ready":    0x1,
}

# Actual data writing configuration
PCIE_DEV_MEMORY = {
    "ROOT_DESC_HEADER": {
        "magic_num": ROOT_DESC_HEADER_MAGIC_NUM,
        "version": ROOT_DESC_HEADER_VERSION,
    },
    "ROOT_DESC_DATA": {
        "magic_num": ROOT_DESC_DATA_MAGIC_NUM,
        "board_info_offset": 0x400,
        "retimer_offset": 0x500,
    },
    "BOARD_INFO_HEADER": {
        "function_type": FUNCTION_TYPE["board_info"],
        "version": FUNCTION_BLOCK_HEADER_VERSION,
        "data_valid": 1,
        "data_used": 1,
        "data_size": 0x100,
        "direction": 0,  # 0: BMC to OS, 1: OS to BMC
    },

    "BOARD_INFO_DATA": {
        "TLV_BLOCK1": {"type": BOARD_INFO_TLV_TYPES["board_id"], "length": 4, "value": None},
        "TLV_BLOCK2": {"type": BOARD_INFO_TLV_TYPES["uid"], "length": 1, "value": None},
        "TLV_BLOCK3": {"type": BOARD_INFO_TLV_TYPES["rack_sn"], "length": 4, "value": None},
        "TLV_BLOCK4": {"type": BOARD_INFO_TLV_TYPES["tray_sn"], "length": 4, "value": None},
        "TLV_BLOCK5": {"type": BOARD_INFO_TLV_TYPES["tray_id"], "length": 1, "value": None},
        "TLV_BLOCK6": {"type": BOARD_INFO_TLV_TYPES["tray_type"], "length": 1, "value": None},
    },
    "RETIMER_HEADER": {
        "function_type": FUNCTION_TYPE["retimer"],
        "version": FUNCTION_BLOCK_HEADER_VERSION,
        "data_valid": 1,
        "data_used": 1,
        "data_size": 0x100,
        "direction": 0,  # 0: BMC to OS, 1: OS to BMC
    },

    "RETIMER_DATA": {
        "TLV_BLOCK1": {"type": RETIMER_TLV_TYPES["retimer_ready"], "length": 1, "value": None},
    },
}

# ---------------------- Dynamic Format Generation ----------------------
def get_struct_format(lengths_dict, field_names=None):
    """Generate struct format string and field name list from field length dictionary
    
    Args:
        lengths_dict: Dictionary of field names and their byte lengths
        field_names:  Optional list of field names; if None, uses all keys in lengths_dict
    """
    format_map = {
        1: 'B',  # unsigned char
        2: 'H',  # unsigned short
        4: 'I',  # unsigned int
        8: 'Q',  # unsigned long long
    }
    
    fmt = '<'  # Little-endian byte order
    if field_names is None:
        field_names = list(lengths_dict.keys())
    
    for field_name in field_names:
        length = lengths_dict[field_name]
        if length not in format_map:
            raise ValueError(f"Unsupported field length {length} for field {field_name}")
        fmt += format_map[length]
    
    return fmt, field_names

# Pre-generate format strings and field names
ROOT_DESC_HEADER_FORMAT, ROOT_DESC_HEADER_FIELDS = get_struct_format(ROOT_DESC_HEADER_LEN)
FUNCTION_BLOCK_HEADER_FORMAT, FUNCTION_BLOCK_HEADER_FIELDS = get_struct_format(FUNCTION_BLOCK_HEADER_LEN)

# Dynamically calculate ROOT_DESC_HEADER_SIZE
ROOT_DESC_HEADER_SIZE = struct.calcsize(ROOT_DESC_HEADER_FORMAT)

def get_function_blocks():
    """Dynamically retrieve all Function Block configurations"""
    function_blocks = {}
    for key in PCIE_DEV_MEMORY:
        if key.endswith("_HEADER") and not key.startswith("ROOT_DESC"):
            # Extract Function Block name (remove "_HEADER" and convert to lowercase)
            fb_name = key.replace("_HEADER", "").lower()
            function_blocks[fb_name] = {
                "header": PCIE_DEV_MEMORY[key],
                "data": PCIE_DEV_MEMORY.get(f"{fb_name.upper()}_DATA", {})
            }
    return function_blocks

def get_fb_offsets():
    """Retrieve offsets of all Function Blocks from ROOT_DESC_DATA"""
    offsets = {}
    root_data = PCIE_DEV_MEMORY["ROOT_DESC_DATA"]
    for key, value in root_data.items():
        if key.endswith("_offset"):
            fb_name = key.replace("_offset", "")
            offsets[fb_name] = value
    return offsets

def get_bar_info(info_type):
    """Retrieve BAR information (e.g., BAR space offset and size) from file on OS side; 
    falls back to BAR_DATA_SUMMARY if file does not exist"""
    try:
        with open(BAR_INFO_PATH, 'r') as f:
            lines = f.readlines()
            bar_info = {}
            for line in lines:
                line = line.strip()
                if line:
                    key, value = line.split(':', 1)
                    bar_info[key.strip()] = value.strip()
            
            if info_type in bar_info:
                info = bar_info[info_type]
                # Return string directly for device_path
                if info_type == "device_path":
                    return info
                # Handle hexadecimal or decimal for numeric types
                elif info.startswith("0x") or info.startswith("0X"):
                    return int(info, 16)
                else:
                    return int(info)
            else:
                pcie_dev_warn(f"{info_type} not found in {BAR_INFO_PATH}, trying BAR_DATA_SUMMARY")
                # Fall back to BAR_DATA_SUMMARY if not found in file
                return get_bar_info_from_config(info_type)

    except FileNotFoundError:
        pcie_dev_warn(f"BAR info file {BAR_INFO_PATH} not found, trying BAR_DATA_SUMMARY")
        return get_bar_info_from_config(info_type)
    except Exception as e:
        pcie_dev_error(f"Failed to read BAR info from {BAR_INFO_PATH}: {e}", exc_info=True)
        pcie_dev_warn("Trying BAR_DATA_SUMMARY as fallback")
        return get_bar_info_from_config(info_type)

def get_bar_info_from_config(info_type):
    """Retrieve BAR information from BAR_DATA_SUMMARY configuration on BMC side"""
    try:
        # Handle device_path
        if info_type == "device_path" and "device_path" in BAR_DATA_SUMMARY:
            device_path = BAR_DATA_SUMMARY["device_path"]
            pcie_dev_info(f"Using device_path from BAR_DATA_SUMMARY: {device_path}")
            return device_path
        # Handle bar_size
        elif info_type == "bar_size" and "bar_size" in BAR_DATA_SUMMARY:
            bar_size = BAR_DATA_SUMMARY["bar_size"]
            pcie_dev_info(f"Using bar_size from BAR_DATA_SUMMARY: {bar_size:#x}")
            return bar_size
        # Handle bar_offset
        elif info_type == "bar_offset" and "bar_offset" in BAR_DATA_SUMMARY:
            bar_offset = BAR_DATA_SUMMARY["bar_offset"]
            pcie_dev_info(f"Using bar_offset from BAR_DATA_SUMMARY: {bar_offset:#x}")
            return bar_offset
        else:
            pcie_dev_error(f"{info_type} not found in BAR_DATA_SUMMARY")
            return None
    except Exception as e:
        pcie_dev_error(f"Failed to get {info_type} from BAR_DATA_SUMMARY: {e}", exc_info=True)
        return None

# Retrieve device information
BAR_DATA_SUMMARY = get_config_param("BAR_DATA_SUMMARY", [])
DEVICE_PATH = get_bar_info("device_path")
BAR_OFFSET = get_bar_info("bar_offset")
BAR_SIZE = get_bar_info("bar_size")

# Validate successful retrieval of device information
if not all([DEVICE_PATH, BAR_OFFSET is not None, BAR_SIZE is not None]):
    pcie_dev_error("Failed to get device information")
    sys.exit(1)

# ---------------------- CRC Calculation ----------------------
def calculate_crc(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF

# ---------------------- Configuration Validation ----------------------
def validate_config():
    function_blocks = get_function_blocks()
    fb_offsets = get_fb_offsets()
    
    # Validate matching number of Function Blocks
    num_function_blocks = len(function_blocks)
    if num_function_blocks != len(fb_offsets):
        pcie_dev_error(f"Mismatch in number of Function Blocks: headers={num_function_blocks}, offsets={len(fb_offsets)}")
        pcie_dev_error(f"Function Blocks: {list(function_blocks.keys())}")
        pcie_dev_error(f"FB Offsets: {list(fb_offsets.keys())}")
        return False

    # Validate offset validity
    for fb_name, offset in fb_offsets.items():
        if offset < 0 or offset >= BAR_SIZE:
            pcie_dev_error(f"{fb_name} offset {offset:#x} invalid")
            return False
        
        # Use each Function Block's own data_size
        fb_config = function_blocks.get(fb_name)
        if fb_config:
            data_size = fb_config["header"].get("data_size", 0)
            if offset + data_size > BAR_SIZE:
                pcie_dev_error(f"{fb_name} block exceeds BAR size")
                return False

    # Validate BAR_DATA_SUMMARY configuration
    for fb_name in function_blocks.keys():
        fb_upper = fb_name.upper()
        fb_lower = fb_name.lower()
        
        if fb_upper not in BAR_DATA_SUMMARY and fb_lower not in BAR_DATA_SUMMARY:
            pcie_dev_error(f"{fb_name}: no TLV points configured in BAR_DATA_SUMMARY")
            pcie_dev_error(f"Available keys in BAR_DATA_SUMMARY: {list(BAR_DATA_SUMMARY.keys())}")
            return False

    pcie_dev_info("Configuration validation passed")
    return True

# ---------------------- Device Operations ----------------------
class BarDevice:
    def __init__(self, device_path=DEVICE_PATH, size=BAR_SIZE, offset=BAR_OFFSET):
        self.device_path = device_path
        self.size = size
        self.offset = offset
        self.fd = None
        self.mmap_obj = None

    def __enter__(self):
        try:
            self.fd = os.open(self.device_path, os.O_RDWR | os.O_SYNC)
            self.mmap_obj = mmap.mmap(
                self.fd,
                self.size,
                flags=mmap.MAP_SHARED,
                prot=mmap.PROT_READ | mmap.PROT_WRITE,
                offset=self.offset,
            )
            return self.mmap_obj
        except Exception as e:
            pcie_dev_error(f"Failed to open/mmap device: {e}", exc_info=True)
            self.close()
            raise

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        if self.mmap_obj and not self.mmap_obj.closed:
            try:
                self.mmap_obj.close()
            except Exception as e:
                pcie_dev_error(f"Failed to close mmap object: {e}", exc_info=True)
        if self.fd:
            try:
                os.close(self.fd)
            except Exception as e:
                pcie_dev_error(f"Failed to close file descriptor: {e}", exc_info=True)

# ---------------------- Write Root Descriptor Header ----------------------
def write_root_desc_header(bar):
    function_blocks = get_function_blocks()
    root_header = PCIE_DEV_MEMORY["ROOT_DESC_HEADER"]
    
    # Dynamically calculate field values
    header_size = ROOT_DESC_HEADER_SIZE
    num_function_blocks = len(function_blocks)
    root_data_offset = header_size
    
    # Build field value dictionary - use configured values if available
    header_values = {}
    for field_name in ROOT_DESC_HEADER_FIELDS:
        # Check if field is defined in config and not a special calculated field
        if field_name in root_header and field_name not in ["header_size", "num_function_blocks", "root_data_offset", "crc"]:
            # Use configured value
            header_values[field_name] = root_header[field_name]
        else:
            # Dynamically calculate values for special fields
            if field_name == "header_size":
                header_values[field_name] = header_size
            elif field_name == "num_function_blocks":
                header_values[field_name] = num_function_blocks
            elif field_name == "root_data_offset":
                header_values[field_name] = root_data_offset
            elif field_name == "crc":
                header_values[field_name] = 0  # Initialize to 0, calculate later
            else:
                pcie_dev_warn(f"Root Header: Field {field_name} not configured and no dynamic rule, using 0")
                header_values[field_name] = 0
    
    # Get values for packing (excluding CRC) in field order
    fields_without_crc = [field for field in ROOT_DESC_HEADER_FIELDS if field != "crc"]
    values_without_crc = [header_values[field] for field in fields_without_crc]
    
    # Generate format string for fields without CRC
    format_without_crc, _ = get_struct_format(ROOT_DESC_HEADER_LEN, fields_without_crc)
    
    # Calculate CRC (excluding the CRC field itself)
    header_data_without_crc = struct.pack(format_without_crc, *values_without_crc)
    crc_value = calculate_crc(header_data_without_crc)
    header_values["crc"] = crc_value
    
    #  Pack complete data
    header_data = struct.pack(ROOT_DESC_HEADER_FORMAT, *[header_values[field] for field in ROOT_DESC_HEADER_FIELDS])
    
    bar.seek(0)
    bar.write(header_data)
    pcie_dev_info(f"Root Descriptor Header written with CRC=0x{crc_value:08x}")
    return True

# ---------------------- Write Root Descriptor Data ----------------------
def write_root_desc_data(bar):
    root_header = PCIE_DEV_MEMORY["ROOT_DESC_HEADER"]
    
    root_header_size = ROOT_DESC_HEADER_SIZE
    root_data_size = ROOT_DESC_TOTAL_SIZE - root_header_size
    
    # Retrieve all Function Block offsets
    fb_offsets_dict = get_fb_offsets()
    fb_offsets = [fb_offsets_dict[fb_name] for fb_name in sorted(fb_offsets_dict.keys())]
    
    # Construct Root Descriptor Data (excluding CRC)
    root_data = PCIE_DEV_MEMORY["ROOT_DESC_DATA"]
    magic = struct.pack('<I', root_data["magic_num"])
    offsets = struct.pack(f'<{len(fb_offsets)}I', *fb_offsets)
    
    # Calculate padding size - CRC field is at the end of data
    data_without_crc = magic + offsets
    padding_size = root_data_size - len(data_without_crc) - 4  #  Subtract size of CRC field
    if padding_size < 0:
        pcie_dev_error("Root Descriptor Data too large for allocated space")
        return False
        
    padding = b'\x00' * padding_size
    
    # Calculate CRC (includes magic + offsets + padding)
    data_for_crc = data_without_crc + padding
    crc_value = calculate_crc(data_for_crc)
    
    # Construct complete data: magic + offsets + padding + crc
    root_data_bytes = data_without_crc + padding + struct.pack('<I', crc_value)

    bar.seek(root_header_size)
    bar.write(root_data_bytes)
    pcie_dev_info(f"Root Descriptor Data written with CRC=0x{crc_value:08x}")
    pcie_dev_info(f"Root Descriptor Data: magic={root_data['magic_num']:#x}, offsets={[hex(x) for x in fb_offsets]}")
    return True

# ---------------------- Retrieve Function Block TLV Data ----------------------
def get_function_block_tlv_data(fb_name):
    """On the BMC side, retrieve TLV data from the BAR_DATA_SUMMARY configuration for writing to the BAR space"""
    tlv_points = None
    if fb_name.upper() in BAR_DATA_SUMMARY:
        tlv_points = BAR_DATA_SUMMARY.get(fb_name.upper())
    elif fb_name.lower() in BAR_DATA_SUMMARY:
        tlv_points = BAR_DATA_SUMMARY.get(fb_name.lower())
    
    if not tlv_points:
        pcie_dev_error(f"{fb_name}: no TLV points configured in BAR_DATA_SUMMARY")
        pcie_dev_error(f"Available keys: {list(BAR_DATA_SUMMARY.keys())}")
        return None

    tlv_data = {}
    for tlv_name, config in tlv_points.items():
        try:
            ret, val = get_value(config)
            if not ret:
                pcie_dev_error(f"{fb_name}: failed to get TLV '{tlv_name}', error: {val}")
                return None
            # Special handling for retimer_ready
            if fb_name.lower() == "retimer" and tlv_name == "retimer_ready" and val == False:
                pcie_dev_info(f"{fb_name}: TLV '{tlv_name}' is not ready, skipping this cycle")
                return None
            if isinstance(val, int):
                val_str = hex(val)
            elif isinstance(val, bool):
                val_str = str(val).lower()
            else:
                val_str = val.strip('\x00').strip()
            tlv_data[tlv_name] = val_str
            pcie_dev_info(f"{fb_name}: TLV '{tlv_name}' = '{val_str}'")
        except Exception:
            pcie_dev_error(f"{fb_name}: exception getting TLV '{tlv_name}'", exc_info=True)
            return None

    if not tlv_data:
        pcie_dev_error(f"{fb_name}: no valid TLV data obtained")
        return None
    return tlv_data

# ---------------------- Write Function Block ----------------------
def write_function_block(bar, fb_name, fb_offset, tlv_data):
    function_blocks = get_function_blocks()
    fb_config = function_blocks.get(fb_name)
    if not fb_config:
        pcie_dev_error(f"{fb_name}: configuration not found")
        return False

    header = fb_config["header"]
    fb_data_config = fb_config["data"]

    fb_header_size = struct.calcsize(FUNCTION_BLOCK_HEADER_FORMAT)
    
    fb_data_size = header.get("data_size", 0)
    fb_total_size = fb_header_size + fb_data_size

    # Dynamically calculate data offset
    data_offset = fb_header_size
    data_start_offset = fb_offset + data_offset
    data_end_offset = data_start_offset + fb_data_size
    if data_end_offset > BAR_SIZE:
        pcie_dev_error(f"{fb_name}: data block exceeds BAR range")
        return False

    # Construct TLV data
    tlv_bytes = b''
    for tlv_name, value in tlv_data.items():
        # Find corresponding TLV type
        tlv_type = None
        for tlv_key, tlv_config in fb_data_config.items():
            # Use corresponding TLV type dictionary based on Function Block type
            if fb_name == "board_info":
                if tlv_name in BOARD_INFO_TLV_TYPES and BOARD_INFO_TLV_TYPES[tlv_name] == tlv_config["type"]:
                    tlv_type = tlv_config["type"]
                    break
            elif fb_name == "retimer":
                if tlv_name in RETIMER_TLV_TYPES and RETIMER_TLV_TYPES[tlv_name] == tlv_config["type"]:
                    tlv_type = tlv_config["type"]
                    break
        
        if not tlv_type:
            pcie_dev_warn(f"{fb_name}: skipping unknown TLV '{tlv_name}'")
            continue
            
        val_bytes = value.encode('utf-8')
        tlv_entry = struct.pack('<HH', tlv_type, len(val_bytes)) + val_bytes
        tlv_bytes += tlv_entry

    if len(tlv_bytes) > fb_data_size:
        pcie_dev_warn(f"{fb_name}: TLV data too large, truncating")
        tlv_bytes = tlv_bytes[:fb_data_size]
    else:
        tlv_bytes = tlv_bytes.ljust(fb_data_size, b'\x00')

    #  Get timestamp (in milliseconds) using monotonic clock
    timestamp = int(time.monotonic() * 1000)

    # Dynamically build header data dictionary - use configured values if available in PCIE_DEV_MEMORY
    header_values = {}
    for field_name in FUNCTION_BLOCK_HEADER_FIELDS:
        # Check if field is defined in configuration
        if field_name in header:
            # Use configured value
            header_values[field_name] = header[field_name]
        else:
            # Dynamically calculate field values
            if field_name == "timestamp2":
                header_values[field_name] = timestamp
            elif field_name == "data_offset":
                header_values[field_name] = data_offset
            elif field_name == "timestamp1":
                # Set timestamp1 to 0 initially, update later
                header_values[field_name] = 0
            else:
                pcie_dev_warn(f"{fb_name}: Field {field_name} not configured and no dynamic rule, using 0")
                header_values[field_name] = 0

    # Pack and write header (timestamp1 is 0 at this point)
    header_data = struct.pack(FUNCTION_BLOCK_HEADER_FORMAT, *[header_values[field] for field in FUNCTION_BLOCK_HEADER_FIELDS])
    bar.seek(fb_offset)
    bar.write(header_data)
    
    # Write TLV data
    bar.seek(data_start_offset)
    bar.write(tlv_bytes)
    
    # Finally write timestamp1 to ensure data integrity
    bar.seek(fb_offset)  # Return to start of header
    bar.write(struct.pack('<Q', timestamp))  # Write timestamp1
    
    pcie_dev_info(f"{fb_name}: header written (timestamp1={timestamp}, timestamp2={timestamp})")
    pcie_dev_info(f"{fb_name}: TLV data written, size={len(tlv_bytes)}")
    pcie_dev_info(f"{fb_name}: Function Block written successfully")
    return True

# ---------------------- Read Function Block TLV ----------------------
def read_function_block_tlv(bar, fb_name):
    function_blocks = get_function_blocks()
    fb_config = function_blocks.get(fb_name)
    if not fb_config:
        pcie_dev_error(f"{fb_name}: configuration not found")
        return None

    fb_header_size = struct.calcsize(FUNCTION_BLOCK_HEADER_FORMAT)

    fb_offsets = get_fb_offsets()
    fb_offset = fb_offsets.get(fb_name)
    if fb_offset is None:
        pcie_dev_error(f"{fb_name}: offset not configured")
        return None

    bar.seek(fb_offset)
    header_bytes = bar.read(fb_header_size)
    if len(header_bytes) != fb_header_size:
        pcie_dev_error(f"{fb_name}: failed to read full header")
        return None

    try:
        # Dynamically unpack header fields
        header_values = struct.unpack(FUNCTION_BLOCK_HEADER_FORMAT, header_bytes)
        header_dict = dict(zip(FUNCTION_BLOCK_HEADER_FIELDS, header_values))
        
        # Access values using configured field names
        timestamp1 = header_dict["timestamp1"]
        timestamp2 = header_dict["timestamp2"]
        data_valid = header_dict["data_valid"]
        data_used = header_dict["data_used"]
        data_size = header_dict["data_size"]
        data_offset = header_dict["data_offset"]
        direction = header_dict["direction"]
        
    except struct.error as e:
        pcie_dev_error(f"{fb_name}: unpack header failed: {e}")
        return None

    if data_valid != 1 or data_used != 1:
        pcie_dev_warn(f"{fb_name}: data invalid or not used")
        return None
    if timestamp1 != timestamp2 or timestamp1 == 0:
        pcie_dev_warn(f"{fb_name}: timestamp mismatch")
        return None

    # Check direction field
    direction_str = "BMC_to_OS" if direction == 0 else "OS_to_BMC"
    pcie_dev_info(f"{fb_name}: direction={direction_str} (0x{direction:x})")

    data_start_offset = fb_offset + data_offset
    bar.seek(data_start_offset)
    data_bytes = bar.read(data_size)
    if len(data_bytes) != data_size:
        pcie_dev_warn(f"{fb_name}: failed to read full TLV data block")
        return None

    tlv_data = {}
    pos = 0
    while pos + 4 <= len(data_bytes):
        tlv_type, tlv_len = struct.unpack('<HH', data_bytes[pos : pos + 4])
        if tlv_type == 0 and tlv_len == 0:
            break
        pos += 4
        if pos + tlv_len > len(data_bytes):
            pcie_dev_warn(f"{fb_name}: TLV length exceeds boundary")
            break
        val_bytes = data_bytes[pos : pos + tlv_len]
        pos += tlv_len

        # Reverse lookup tlv_name based on Function Block type
        tlv_name = None
        if fb_name == "board_info":
            tlv_name = next((name for name, t in BOARD_INFO_TLV_TYPES.items() if t == tlv_type), None)
        elif fb_name == "retimer":
            tlv_name = next((name for name, t in RETIMER_TLV_TYPES.items() if t == tlv_type), None)
        
        if not tlv_name:
            pcie_dev_warn(f"{fb_name}: unknown TLV type 0x{tlv_type:x}, skipping")
            continue

        try:
            val_str = val_bytes.decode('utf-8').strip('\x00').strip()
        except Exception as e:
            pcie_dev_warn(f"{fb_name}: decode TLV '{tlv_name}' failed: {e}")
            val_str = val_bytes.hex()

        tlv_data[tlv_name] = val_str

    if not tlv_data:
        pcie_dev_error(f"{fb_name}: no TLV data read")
        return None
    return tlv_data

def read_root_desc_header(bar):
    """Read and print Root Descriptor Header"""
    bar.seek(0)
    header_size = struct.calcsize(ROOT_DESC_HEADER_FORMAT)
    header_data = bar.read(header_size)
    
    if len(header_data) < header_size:
        print("Error: Root Descriptor Header data too short")
        return False
    
    try:
        fields = struct.unpack(ROOT_DESC_HEADER_FORMAT, header_data)
        field_dict = dict(zip(ROOT_DESC_HEADER_FIELDS, fields))
        
        print("Root Descriptor Header:")
        for field_name in ROOT_DESC_HEADER_FIELDS:
            value = field_dict[field_name]
            print(f"  {field_name}: {value:#x}")
        
        # Validate CRC - calculate using first 5 fields (excluding CRC field itself)
        fields_without_crc = [field for field in ROOT_DESC_HEADER_FIELDS if field != "crc"]
        format_without_crc, _ = get_struct_format(ROOT_DESC_HEADER_LEN, fields_without_crc)
        values_without_crc = [field_dict[field] for field in fields_without_crc]
        
        data_for_crc = struct.pack(format_without_crc, *values_without_crc)
        calculated_crc = calculate_crc(data_for_crc)
        stored_crc = field_dict["crc"]
        
        if calculated_crc == stored_crc:
            print(f"  CRC: VALID (0x{calculated_crc:08x})")
        else:
            print(f"  CRC: INVALID (calculated: 0x{calculated_crc:08x}, stored: 0x{stored_crc:08x})")
        return True
            
    except struct.error as e:
        print(f"Error unpacking Root Descriptor Header: {e}")
        return False

def read_root_desc_data(bar):
    """Read and print Root Descriptor Data"""
    root_header_size = ROOT_DESC_HEADER_SIZE
    root_data_size = ROOT_DESC_TOTAL_SIZE - root_header_size
    
    bar.seek(root_header_size)
    
    # Read entire Root Descriptor Data region
    root_data_bytes = bar.read(root_data_size)
    if len(root_data_bytes) < root_data_size:
        print("Error: Failed to read full Root Descriptor Data")
        return False
    
    # Parse structure: magic + offsets + padding + crc
    fb_offsets_dict = get_fb_offsets()
    num_blocks = len(fb_offsets_dict)
    
    # magic: first 4 bytes
    magic_data = root_data_bytes[:4]
    magic_num = struct.unpack('<I', magic_data)[0]
    
    # offsets: next 4*num_blocks bytes
    offsets_start = 4
    offsets_end = offsets_start + 4 * num_blocks
    offsets_data = root_data_bytes[offsets_start:offsets_end]
    offsets = struct.unpack(f'<{num_blocks}I', offsets_data)
    
    # crc: last 4 bytes
    crc_data = root_data_bytes[-4:]
    crc_value = struct.unpack('<I', crc_data)[0]
    
    # Data used for CRC calculation: magic + offsets + padding (excluding CRC itself)
    data_for_crc = root_data_bytes[:-4]
    calculated_crc = calculate_crc(data_for_crc)
    
    print("Root Descriptor Data:")
    print(f"  magic_num: {magic_num:#x}")
    
    # Display offsets with Function Block names
    fb_names = sorted(fb_offsets_dict.keys())
    for i, (fb_name, offset) in enumerate(zip(fb_names, offsets), 1):
        print(f"  {fb_name}_offset: {offset:#x}")
    
    print(f"  crc: {crc_value:#x}")
    
    if calculated_crc == crc_value:
        print(f"  CRC: VALID (0x{calculated_crc:08x})")
    else:
        print(f"  CRC: INVALID (calculated: 0x{calculated_crc:08x}, stored: 0x{crc_value:08x})")
    return True

def read_function_block_header(bar, fb_name):
    """Read and print header of specified Function Block"""
    fb_offsets = get_fb_offsets()
    fb_offset = fb_offsets.get(fb_name)
    if fb_offset is None:
        print(f"Error: {fb_name} offset not found")
        return False
    
    fb_header_size = struct.calcsize(FUNCTION_BLOCK_HEADER_FORMAT)
    
    bar.seek(fb_offset)
    header_bytes = bar.read(fb_header_size)
    
    if len(header_bytes) != fb_header_size:
        print(f"Error: Failed to read full header for {fb_name}")
        return False
    
    try:
        # Dynamically unpack header fields
        header_values = struct.unpack(FUNCTION_BLOCK_HEADER_FORMAT, header_bytes)
        header_dict = dict(zip(FUNCTION_BLOCK_HEADER_FIELDS, header_values))
        
        print(f"Function Block {fb_name} Header:")
        for field_name in FUNCTION_BLOCK_HEADER_FIELDS:
            value = header_dict[field_name]
            if field_name in ["timestamp1", "timestamp2"]:
                print(f"  {field_name}: {value}")
            elif field_name in ["function_type"]:
                print(f"  {field_name}: {value:#x}")
            elif field_name in ["direction"]:
                direction_str = "BMC_to_OS" if value == 0 else "OS_to_BMC"
                print(f"  {field_name}: {direction_str} (0x{value:x})")
            else:
                print(f"  {field_name}: {value}")
        return True
        
    except struct.error as e:
        print(f"Error unpacking {fb_name} header: {e}")
        return False

# ---------------------- CLI ----------------------
@click.group()
def cli():
    """BAR space read/write tool for PCIe device communication"""
    pass

def get_available_fbs():
    """Retrieve list of available Function Blocks"""
    function_blocks = get_function_blocks()
    return list(function_blocks.keys())

def get_fb_tlv_names(fb_name):
    """Retrieve list of available TLV names for specified Function Block"""
    function_blocks = get_function_blocks()
    fb_config = function_blocks.get(fb_name)
    if not fb_config:
        return []
    
    # Select corresponding TLV type dictionary based on Function Block type
    if fb_name == "board_info":
        tlv_type_dict = BOARD_INFO_TLV_TYPES
    elif fb_name == "retimer":
        tlv_type_dict = RETIMER_TLV_TYPES
    else:
        return []
    
    return list(tlv_type_dict.keys())

@cli.command()
@click.option('--verbose', '-v', is_flag=True, help='Enable verbose output')
def write(verbose):
    """Poll to get data and write to BAR space
    
    This command continuously polls for data and writes it to the PCIe device's 
    BAR space. It will retry every 10 seconds until all Function Blocks are 
    successfully written or timeout is reached.
    
    Available Function Blocks:
    \b
    - board_info: Board information (board_id, uid, rack_sn, tray_sn, tray_id, tray_type)
    - retimer: Retimer information (retimer_ready)
    """
    if verbose:
        logger.setLevel(logging.DEBUG)
        pcie_dev_info("Verbose mode enabled")
    
    available_fbs = get_available_fbs()
    pcie_dev_info(f"Starting BAR space write process with Function Blocks: {available_fbs}")
    
    if not validate_config():
        sys.exit(1)
        
    start_time = time.time()
    function_blocks = get_function_blocks()
    fb_offsets = get_fb_offsets()
    
    fb_write_status = {fb_name: False for fb_name in function_blocks.keys()}

    try:
        pcie_dev_info(f"[Initial] Opening device file: {DEVICE_PATH}")
        with BarDevice() as bar:
            if not write_root_desc_header(bar):
                pcie_dev_error("Failed to write root descriptor header")
                sys.exit(1)
                
            if not write_root_desc_data(bar):
                pcie_dev_error("Failed to write root descriptor data")
                sys.exit(1)
    except Exception as e:
        pcie_dev_error(f"Failed to write root descriptors: {e}", exc_info=True)
        sys.exit(1)

    # Then enter loop to write only Function Blocks
    while True:
        if all(fb_write_status.values()):
            pcie_dev_info("All Function Blocks written successfully, exiting")
            break

        try:
            pcie_dev_info(f"[Polling] Opening device file: {DEVICE_PATH}")
            with BarDevice() as bar:
                any_fb_written = False

                for fb_name in function_blocks.keys():
                    if fb_write_status[fb_name]:
                        pcie_dev_info(f"{fb_name} already written, skipping")
                        continue

                    fb_offset = fb_offsets.get(fb_name)
                    if fb_offset is None:
                        pcie_dev_error(f"{fb_name} offset not configured, skipping")
                        continue

                    tlv_data = get_function_block_tlv_data(fb_name)
                    if tlv_data is None:
                        pcie_dev_info(f"{fb_name} TLV data not ready, retry next cycle")
                        continue

                    if not write_function_block(bar, fb_name, fb_offset, tlv_data):
                        pcie_dev_error(f"Failed to write {fb_name}")
                        continue
                        
                    fb_write_status[fb_name] = True
                    any_fb_written = True

                if any_fb_written:
                    pcie_dev_info("At least one Function Block written in this cycle")
                else:
                    pcie_dev_info(f"No valid writes this cycle, sleeping {RETRY_INTERVAL} seconds")
                    time.sleep(RETRY_INTERVAL)

                if time.time() - start_time > TIMEOUT:
                    pcie_dev_error(f"Timeout {TIMEOUT} seconds reached, exiting")
                    sys.exit(1)

        except Exception as e:
            pcie_dev_error(f"Exception occurred: {e}", exc_info=True)
            time.sleep(RETRY_INTERVAL)

@cli.command()
@click.argument('fb_name', type=click.Choice(get_available_fbs(), case_sensitive=False))
@click.argument('tlv_name', required=False)
def read(fb_name, tlv_name):
    """Read specified Function Block's TLV data
    
    \b
    FB_NAME: Function Block identifier (board_info, retimer, etc.)
    TLV_NAME: TLV data field name to read (optional)
    
    Examples:
    \b
      bmc_pcie_dev.py read board_info               # Read all board_info TLV data
      bmc_pcie_dev.py read board_info uid           # Read only board_info.uid
      bmc_pcie_dev.py read retimer                  # Read all retimer TLV data
      bmc_pcie_dev.py read retimer retimer_ready    # Read only retimer.retimer_ready
    """
    fb_name = fb_name.lower()
    
    try:
        with BarDevice() as bar:
            tlv_data = read_function_block_tlv(bar, fb_name)

        if not tlv_data:
            pcie_dev_error(f"No TLV data found for {fb_name}")
            sys.exit(1)

        # If tlv_name is specified, only output the value of that TLV
        if tlv_name:
            if tlv_name not in tlv_data:
                pcie_dev_error(f"TLV '{tlv_name}' not found in {fb_name}")
                # Display available TLV names
                available_tlvs = get_fb_tlv_names(fb_name)
                if available_tlvs:
                    print(f"TLV '{tlv_name}' not found in {fb_name}")
                    print(f"Available TLV names for {fb_name}:")
                    for tlv in available_tlvs:
                        print(f"  - {tlv}")
                sys.exit(1)
            print(tlv_data[tlv_name])
        else:
            # If no tlv_name is specified, output all TLV data
            for tlv_key, tlv_value in tlv_data.items():
                print(f"{tlv_key}: {tlv_value}")

    except Exception as e:
        pcie_dev_error(f"Read error: {e}", exc_info=True)
        sys.exit(1)

@cli.command()
def read_root_header():
    """Read and print Root Descriptor Header
    
    Displays the Root Descriptor Header information including:
    magic number, header size, version, number of function blocks, 
    root data offset, and CRC validation.
    """
    try:
        with BarDevice() as bar:
            if not read_root_desc_header(bar):
                sys.exit(1)
    except Exception as e:
        pcie_dev_error(f"Read root header error: {e}", exc_info=True)
        sys.exit(1)

@cli.command()
def read_root_data():
    """Read and print Root Descriptor Data
    
    Displays the Root Descriptor Data information including:
    magic number, function block offsets, and CRC validation.
    """
    try:
        with BarDevice() as bar:
            if not read_root_desc_data(bar):
                sys.exit(1)
    except Exception as e:
        pcie_dev_error(f"Read root data error: {e}", exc_info=True)
        sys.exit(1)

@cli.command()
@click.argument('fb_name', type=click.Choice(get_available_fbs(), case_sensitive=False))
def read_fb_header(fb_name):
    """Read and print Function Block Header
    
    \b
    FB_NAME: Function Block identifier (board_info, retimer, etc.)
    
    Displays Function Block Header information including:
    timestamps, function type, version, data size, data offset,
    data valid flag, and data used flag.
    
    Examples:
    \b
      bmc_pcie_dev.py read-fb-header board_info
      bmc_pcie_dev.py read-fb-header retimer
    """
    fb_name = fb_name.lower()
    
    try:
        with BarDevice() as bar:
            if not read_function_block_header(bar, fb_name):
                sys.exit(1)
    except Exception as e:
        pcie_dev_error(f"Read function block header error: {str(e)}", exc_info=True)
        sys.exit(1)

if __name__ == '__main__':
    cli()