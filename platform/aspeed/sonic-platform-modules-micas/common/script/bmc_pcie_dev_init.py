#!/usr/bin/env python3
import os
import sys
import glob
import mmap
import logging
import subprocess
from public.platform_common_config import BOARD_ID_PATH, BAR_INFO_PATH
from platform_util import setup_logger, BSP_COMMON_LOG_DIR

# ---------------------- Log Configuration ----------------------
DEBUG_FILE = "/etc/.bmc_pcie_dev_init_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "bmc_pcie_dev_init_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def pcie_dev_init_error(s):
    logger.error(s)

def pcie_dev_init_info(s):
    logger.info(s)


# Vendor ID and Device ID of the target PCI device (4-digit lowercase hexadecimal, without the "0x" prefix)
TARGET_VENDOR_ID = "1a03"
TARGET_DEVICE_ID = "2402"

BOARD_ID_DATA_OFFSET = 1062  # Offset of the board_id relative to the start address of BAR0
BOARD_ID_READ_SIZE = 11      # Board ID Length (in Bytes)

DEVICE_PATH = "/dev/mem"

def find_pci_devices(vendor_id, device_id):
    """Search for matching PCI devices and return a list of device addresses"""
    devices = []
    for dev_path in glob.glob("/sys/bus/pci/devices/*"):
        try:
            with open(os.path.join(dev_path, "vendor")) as f:
                v = f.read().strip()
            with open(os.path.join(dev_path, "device")) as f:
                d = f.read().strip()
            if v.lower() == "0x" + vendor_id and d.lower() == "0x" + device_id:
                devices.append(os.path.basename(dev_path))
        except Exception:
            continue
    return devices

def get_bar0_info(dev_addr):
    """Read the physical address and size of BAR0 from the PCI device"""
    try:
        # Read the physical address of BAR0
        with open(f"/sys/bus/pci/devices/{dev_addr}/resource") as f:
            lines = f.readlines()
            if len(lines) > 0:
                bar0_line = lines[0].split()
                start_addr = int(bar0_line[0], 16)
                end_addr = int(bar0_line[1], 16)
                bar_size = end_addr - start_addr + 1
                return start_addr, bar_size
        return None, None
    except Exception as e:
        pcie_dev_init_error(f"Failed to get BAR0 info: {e}")
        return None, None

def read_phys_mem(phys_addr, offset, size):
    """Read size bytes from the physical address phys_addr + offset"""
    PAGE_SIZE = 4096
    read_addr = phys_addr + offset
    page_start = read_addr & ~(PAGE_SIZE - 1)
    page_offset = read_addr - page_start
    length = page_offset + size

    try:
        with open("/dev/mem", "rb") as f:
            mm = mmap.mmap(f.fileno(), length, prot=mmap.PROT_READ, offset=page_start)
            data = mm[page_offset:page_offset + size]
            mm.close()
            return data
    except PermissionError:
        pcie_dev_init_error("Permission denied: please run as root.")
    except Exception as e:
        pcie_dev_init_error(f"Error reading /dev/mem: {e}")
    return None

def write_bar_info(bar_offset, bar_size, filepath):
    """Write BAR information to the specified file"""
    try:
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        with open(filepath, "w") as f:
            f.write(f"device_path:{DEVICE_PATH}\n")
            f.write(f"bar_offset:{bar_offset}\n")
            f.write(f"bar_size:{bar_size}\n")
        pcie_dev_init_info(f"BAR info written to {filepath} successfully.")
        pcie_dev_init_info(f"BAR offset: {bar_offset}, BAR size: {bar_size}")
    except Exception as e:
        pcie_dev_init_error(f"Failed to write BAR info file: {e}")

def write_board_id(data, filepath):
    """Write the board_id to the specified file, ignoring non-ASCII characters"""
    try:
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        if isinstance(data, bytes):
            text = data.decode('ascii', errors='ignore').strip()
        else:
            text = str(data)
        with open(filepath, "w") as f:
            f.write(text + "\n")
        pcie_dev_init_info(f"Board ID written to {filepath} successfully.")
        pcie_dev_init_info(f"Board ID content: {text}")
    except Exception as e:
        pcie_dev_init_error(f"Failed to write board ID file: {e}")

def run_bmc_script():
    """Execute the Python script on the BMC side"""
    try:
        pcie_dev_init_info("Running BMC PCIe device script...")
        result = subprocess.run(
            ["python3", "/usr/local/bin/bmc_pcie_dev.py", "write"],
            capture_output=True,
            text=True,
            check=True
        )
        pcie_dev_init_info(f"BMC script executed successfully: {result.stdout}")
        return True
    except subprocess.CalledProcessError as e:
        pcie_dev_init_error(f"BMC script failed with error: {e}")
        pcie_dev_init_error(f"Stderr: {e.stderr}")
        return False
    except FileNotFoundError:
        pcie_dev_init_error("BMC script 'bmc_pcie_dev.py' not found")
        return False
    except Exception as e:
        pcie_dev_init_error(f"Unexpected error running BMC script: {e}")
        return False

def handle_os_side(dev_addr):
    """Process the logic on the OS side"""
    pcie_dev_init_info("Running in OS mode...")
    
    # Retrieve BAR0 information
    bar0_addr, bar_size = get_bar0_info(dev_addr)
    if bar0_addr is None or bar_size is None:
        pcie_dev_init_error("Cannot get BAR0 information.")
        return False

    # Write the bar_info data to a file
    write_bar_info(f"0x{bar0_addr:x}", f"0x{bar_size:x}", BAR_INFO_PATH)
    
    # Read the board_id data
    pcie_dev_init_info(f"Reading {BOARD_ID_READ_SIZE} bytes from BAR0 address 0x{bar0_addr:x} + offset 0x{BOARD_ID_DATA_OFFSET:x}...")
    data = read_phys_mem(bar0_addr, BOARD_ID_DATA_OFFSET, BOARD_ID_READ_SIZE)
    if data is None:
        pcie_dev_init_error("Failed to read board_id data.")
        return False

    pcie_dev_init_info(f"Board ID data read (hex): {data.hex()}")
    
    # Write to the board_id file
    write_board_id(data, BOARD_ID_PATH)
    
    return True

def main():
    # Verify root privileges
    if os.geteuid() != 0:
        pcie_dev_init_error("This script must be run as root")
        sys.exit(1)

    pcie_dev_init_info(f"Searching PCI devices with vendor_id={TARGET_VENDOR_ID}, device_id={TARGET_DEVICE_ID}...")
    devices = find_pci_devices(TARGET_VENDOR_ID, TARGET_DEVICE_ID)
    
    if devices:
        # OS-side logic
        pcie_dev_init_info(f"Found {len(devices)} matching PCI device(s). Running in OS mode.")
        dev_addr = devices[0]
        pcie_dev_init_info(f"Using device: {dev_addr}")
        
        if not handle_os_side(dev_addr):
            pcie_dev_init_error("OS side processing failed.")
            sys.exit(1)
            
    else:
        # BMC-side logic
        pcie_dev_init_info("No matching PCI devices found. Running in BMC mode.")
        
        if not run_bmc_script():
            pcie_dev_init_error("BMC side processing failed.")
            sys.exit(1)

    pcie_dev_init_info("Script completed successfully.")

if __name__ == "__main__":
    main()