#!/usr/bin/env python_nos
import sys
import os
import time
import syslog
import click
import logging
from platform_config import get_config_param
from platform_util import *
from public.platform_common_config import S3IP_SYSFS_NAME

PLATFORM_FW_CHECK_CONFIG = get_config_param("PLATFORM_FW_CHECK_CONFIG", {})
DEBUG_FILE = "/etc/.platform_fw_check_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_fw_check_debug.log"
logger = setup_logger(LOG_FILE)
LOG_LAST_TIME = {}
SYSLOG_TITLE = "FW_CHECK"
DEV_ALIAS = "alias"
DEV_FIRMWARE_VERSION = "firmware_version"
STATUS_PASS = "pass"
STATUS_FAIL = "fail"
device_find_path_table = {
    "fpga": "/sys/S3IP_SYSFS_NAME/fpga",
    "cpld": "/sys/S3IP_SYSFS_NAME/cpld",
    "misc": "/sys/S3IP_SYSFS_NAME/misc_fw"
}

device_find_restful_table = {
    "fpga": {
        "type" : "fpga",
        "field" : {
            DEV_ALIAS:"alias",
            DEV_FIRMWARE_VERSION: "firmware_version",
        },
    },
    "cpld": {
        "type" : "cpld",
        "field" : {
            DEV_ALIAS:"alias",
            DEV_FIRMWARE_VERSION: "firmware_version",
        },
    },
    "misc": {
        "type" : "misc",
        "field" : {
            DEV_ALIAS:"alias",
            DEV_FIRMWARE_VERSION: "firmware_version",
        },
    },
}
'''
Example 
PLATFORM_FW_CHECK_CONFIG = {
"support_restful": 1,
"dev_list": [
    {"name": "MB_CPLD", "fw_version": "251128", "compare_type": 0, "mask": 0xffffff},
    {"name": "MAC_CPLDA", "fw_version": "251103", "compare_type": 0, "mask": 0xffffff},
    {"name": "MAC_CPLDB", "fw_version": "251031", "compare_type": 0, "mask": 0xffffff},
    {"name": "BMC_CPLD", "fw_version": "251010", "compare_type": 0, "mask": 0xffffff},
    {"name": "FPGA1_CPLD", "fw_version": "251017", "compare_type": 0, "mask": 0xffffff},
    {"name": "FPGA2_CPLD", "fw_version": "251017", "compare_type": 0, "mask": 0xffffff},
    {"name": "FAN_CPLD", "fw_version": "250916", "compare_type": 0, "mask": 0xffffff},
    {"name": "DOM_FPGA", "fw_version": "7a643506", "compare_type": 0},
],
}
'''

def platform_fw_check_debug(s):
    logger.debug(s)


def platform_fw_check_error(s):
    logger.error(s)


def platform_fw_check_info(s):
    logger.info(s)


def platform_fw_check_syslog_warning(s):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        common_syslog_warn(SYSLOG_TITLE, s)
    logger.warning(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def get_devices_from_restful():
    devices = {}

    for device, config in device_find_restful_table.items():
        num_config = {}
        num_config["type"] = config.get("type")
        type = num_config["type"]
        num_config["field_name"] = "num"
        ret, num = get_single_info_from_restful(num_config)
        if ret is False:
            platform_fw_check_error(("get %s num fail, reason: %s" % (num_config["type"], num)))
            return devices

        for i in range(1, num + 1):
            config["index"] = i
            ret, restful_val = get_multiple_info_from_restful(config)
            if ret is False:
                platform_fw_check_info(f"Failed to read '{type}' from restful.")
                continue
            alias = restful_val.get(DEV_ALIAS)
            fw_version = restful_val.get(DEV_FIRMWARE_VERSION)

            if not alias or fw_version is None:
                platform_fw_check_error(f"Restful response missing required fields (index {i}): {restful_val}")
                continue

            alias_upper = alias.upper()
            devices[alias_upper] = fw_version

            platform_fw_check_debug(f"Found restful device: alias='{alias_upper}', firmware version='{fw_version}'")


    return devices

def get_all_devices():
    devices = {}
    suport_restful = PLATFORM_FW_CHECK_CONFIG.get("support_restful", 0)

    for dev_type, base_path in device_find_path_table.items():
        if not os.path.exists(base_path):
            platform_fw_check_debug(f"Path '{base_path}' does not exist, skipping {dev_type} device check.")
            continue

        try:
            dev_dirs = os.listdir(base_path)
        except OSError as e:
            platform_fw_check_info(f"Failed to list directory '{base_path}': {str(e)}, skipping {dev_type} device check.")
            continue

        for dev_dir in dev_dirs:
            dev_path = os.path.join(base_path, dev_dir)
            if not os.path.isdir(dev_path):
                platform_fw_check_debug(f"'{dev_dir}' is not a directory, skipping.")
                continue

            alias_file = os.path.join(dev_path, DEV_ALIAS)
            try:
                with open(alias_file, 'r') as f:
                    alias = f.read().strip().upper()
            except OSError as e:
                platform_fw_check_info(f"Failed to read '{dev_path}/alias': {str(e)}, skipping this device.")
                continue

            fw_ver_file = os.path.join(dev_path, DEV_FIRMWARE_VERSION)
            try:
                with open(fw_ver_file, 'r') as f:
                    fw_ver = f.read().strip()
            except OSError as e:
                platform_fw_check_info(f"Failed to read '{dev_path}/firmware_version': {str(e)}, skipping this device.")
                continue

            devices[alias] = fw_ver
            platform_fw_check_debug(f"Found device: alias='{alias}', type='{dev_type}', firmware version='{fw_ver}'.")

    if suport_restful != 0:
        restful_devices = get_devices_from_restful()
        devices.update(restful_devices)

    return devices


def compare_versions(current_ver: str, required_ver: str, compare_type: int, mask: int = None) -> bool:
    if compare_type == 0:
        try:
            current_int = int(current_ver, 16)
            required_int = int(required_ver, 16)
            if mask is not None:
                current_int &= mask
                required_int &= mask

            platform_fw_check_debug(f"Hex integer comparison: current=0x{current_ver} ({current_int}), required=0x{required_ver} ({required_int}).")
            return current_int < required_int
        except ValueError as e:
            raise ValueError(f"Invalid hex version string: current='{current_ver}', required='{required_ver}' (error: {str(e)}).")
    elif compare_type == 1:
        platform_fw_check_debug(f"String lexicographical comparison: current='{current_ver}', required='{required_ver}'.")
        return current_ver < required_ver
    else:
        raise ValueError(f"Unsupported compare_type '{compare_type}' (only 0 or 1 are allowed).")


def process_fw_check_config(devices: dict, config_list: list) -> None:
    for config in config_list:
        name = config.get("name")
        required_ver = config.get("fw_version")
        compare_type = config.get("compare_type", 0)
        mask = config.get("mask", None)

        if not name or not required_ver:
            platform_fw_check_error(f"Invalid config item: missing 'name' or 'fw_version' (config item: {config}).")
            continue

        name_upper = name.upper()
        if name_upper not in devices:
            platform_fw_check_error(f"Device '{name}' (standardized to '{name_upper}') not found in the system.")
            continue

        current_ver = devices[name_upper]
        try:
            is_lower = compare_versions(current_ver, required_ver, compare_type, mask)
        except ValueError as e:
            platform_fw_check_error(f"Failed to compare versions for device '{name}': {str(e)}.")
            continue

        if is_lower:
            platform_fw_check_syslog_warning(
                f"%%FW_CHECK-4-VERSION_LOW: {name} firmware version '{current_ver}' is lower than the normal version {required_ver}"
            )
        else:
            platform_fw_check_debug(
                f"Firmware version '{current_ver}' of device '{name}' (alias='{name_upper}') "
                f"meets or exceeds the required version '{required_ver}' (compare type: {compare_type})."
            )


def cli_print_format(name, current_ver, required_ver, status):
    """Prints firmware status in a formatted manner."""
    print(f"{name:<20} {current_ver:<35} {required_ver:<35} {status:<10}")


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()


@main.command()
def start():
    platform_fw_check_info("Firmware version check started.")
    devices = get_all_devices()
    if not devices:
        platform_fw_check_error("No valid devices found in /sys/s3ip/ directory.")
        return

    platform_fw_check_debug(f"Found {len(devices)} valid devices in the system.")
    platform_fw_check_debug(f"{devices}")
    config_list = PLATFORM_FW_CHECK_CONFIG.get("dev_list", [])
    if not config_list:
        platform_fw_check_info("No firmware check configuration found.")
        return

    process_fw_check_config(devices, config_list)
    platform_fw_check_info("Firmware version check completed.")
    time.sleep(5)
    sys.exit(0)

@main.command()
def show_firmware_status():
    """Output the firmware check configuration, current version, expected version, and status."""
    devices = get_all_devices()
    config_list = PLATFORM_FW_CHECK_CONFIG.get("dev_list", [])

    if not devices:
        platform_fw_check_error("No valid devices found in /sys/s3ip/ directory.")
        print("No valid devices found in /sys/s3ip/ directory.")
        return

    if not config_list:
        platform_fw_check_error("No firmware check configuration found.")
        print("No firmware check configuration found.")
        return

    cli_print_format("Firmware", "Current", "Target", "State")
    for config in config_list:
        name = config.get("name")
        required_ver = config.get("fw_version")
        name_upper = name.upper()
        compare_type = config.get("compare_type", 0)
        mask = config.get("mask", None)
        if not name or not required_ver:
            cli_print_format(f"Invalid config item: missing 'name' or 'fw_version'.")
            continue

        if name_upper in devices:
            current_ver = devices[name_upper]
            try:
                is_lower = compare_versions(current_ver, required_ver, compare_type, mask)
                status = STATUS_FAIL if is_lower else STATUS_PASS
            except Exception as e:
                platform_fw_check_error(f"Compare failed for {name}: {str(e)}")
                status = "ERROR"
            cli_print_format(name, current_ver, required_ver, status)
        else:
            cli_print_format(name, "N/A", required_ver, "NOT FOUND")


if __name__ == '__main__':
    main()
