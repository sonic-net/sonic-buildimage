#!/usr/bin/env python3

"""
Transceiver initialization script
This script deasserts reset and disables low power mode for all transceivers specified in pddf-device.json
"""

import sys
import time
from pathlib import Path
from sonic_py_common import logger
from nexthop.pddf_config_parser import load_pddf_device_config, extract_xcvr_list

SYSLOG_IDENTIFIER = "transceiver-init"
MAX_WAIT_TIME = 30  # Maximum time to wait for transceiver control in seconds

# Initialize logger
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()


def log_info(message):
    sonic_logger.log_info(message)


def log_error(message):
    sonic_logger.log_error(message)


def wait_for(function, max_wait_time, description):
    log_info(f"Waiting for {description}.")
    for elapsed_time in range(max_wait_time):
        if function():
            return True
        time.sleep(1)

    log_error(f"Timed out waiting for {description}.")
    return False


def is_xcvr_control_available(xcvr_list):
    """Check if transceiver control files are available"""
    for xcvr in xcvr_list:
        bus = xcvr["bus"]
        addr = xcvr["addr"]
        lpmode_path = Path(f"/sys/bus/i2c/devices/{bus}-{addr}/xcvr_lpmode")
        reset_path = Path(f"/sys/bus/i2c/devices/{bus}-{addr}/xcvr_reset")

        if not lpmode_path.exists() or not reset_path.exists():
            return False
    return True


def init_xcvrs(xcvr_list):
    """Initialize transceivers by deasserting reset and disabling low power mode"""
    status = True

    log_info(
        f"Deasserting reset and disabling low power mode for {len(xcvr_list)} transceivers."
    )

    for xcvr in xcvr_list:
        bus = xcvr["bus"]
        addr = xcvr["addr"]
        name = xcvr["name"]

        # Deassert reset (write 0)
        try:
            with open(f"/sys/bus/i2c/devices/{bus}-{addr}/xcvr_reset", "w") as f:
                f.write("0")
        except Exception:
            log_error(f"Failed to disable {name} xcvr reset for bus {bus}.")
            status = False
            continue

        # Disable low power mode (write 0)
        try:
            with open(f"/sys/bus/i2c/devices/{bus}-{addr}/xcvr_lpmode", "w") as f:
                f.write("0")
        except Exception:
            log_error(f"Failed to disable {name} xcvr low power mode for bus {bus}.")
            status = False
            continue

    return status


def main():
    log_info("Starting transceiver initialization")

    try:
        config = load_pddf_device_config()
    except Exception as e:
        log_error(f"Failed to load PDDF configuration: {str(e)}")
        sys.exit(1)

    # Extract transceiver information
    xcvr_list = extract_xcvr_list(config)
    if not xcvr_list:
        log_error("No transceiver information found in PDDF configuration")
        sys.exit(1)
    log_info(f"Found {len(xcvr_list)} transceivers in configuration")

    # Wait for transceiver control to be available
    if not wait_for(
        lambda: is_xcvr_control_available(xcvr_list),
        MAX_WAIT_TIME,
        "transceiver control",
    ):
        sys.exit(1)

    # Initialize transceivers
    if not init_xcvrs(xcvr_list):
        sys.exit(1)

    log_info("Transceiver initialization completed successfully")
    sys.exit(0)


if __name__ == "__main__":
    main()
