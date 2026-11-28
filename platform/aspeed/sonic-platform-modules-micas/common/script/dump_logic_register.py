#!/usr/bin/env python_nos
import os
import sys
from platform_util import *
import logging
from public.platform_common_config import S3IP_SYSFS_NAME

DEBUG_FILE = "/etc/.bsp_dump_logic_register_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "bsp_dump_logic_register.log"
logger = setup_logger(LOG_FILE)

CPLD_DIR = "/sys/S3IP_SYSFS_NAME/cpld"
FPGA_DIR = "/sys/S3IP_SYSFS_NAME/fpga"
DEV_FILE_PREFIX = "/dev/"
LOGIC_DUMP_DEV_TABLE = ["cpld", "fpga"]
ARGV_LEN_MIN = 2
ARGV_LEN_MAX = 5
LOGCI_READ_REG_LEN_DEF = 1


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def dump_logic_register_debug(s):
    logger.debug(s)


def dump_logic_register_error(s):
    logger.error(s)


def error_exit(message):
    logger.error(message)
    print(f"Error: {message}")


def get_devices(base_dir, device_type):
    """get CPLD or FPGA dev"""
    if not os.path.isdir(base_dir):
        error_exit(f"{device_type.upper()} directory not found: {base_dir}")
        return None

    try:
        number_path = os.path.join(base_dir, "number")
        ret, device_count = read_sysfs(number_path)
        if not ret:
            error_exit(f"Failed to read {device_type.upper()} count from {number_path}")
            return None

        device_count = int(device_count)
        devices = []

        # from 1 to N find dev
        for i in range(1, device_count + 1):
            device_path = os.path.join(base_dir, f"{device_type}{i}")
            alias_path = os.path.join(device_path, "alias")

            if os.path.exists(device_path):
                ret, alias_name = read_sysfs(alias_path)
                alias_name = alias_name if ret else "Unknown"
                # from 0 begin
                devices.append((f"{device_type}{i - 1}", alias_name))

        dump_logic_register_debug(f"{device_type.upper()} devices: {devices}")
        return devices

    except Exception as e:
        dump_logic_register_error(f"Failed to get {device_type.upper()} devices: {str(e)}")
        return None


def read_register(dev_path, reg_addr=None, reg_len=LOGCI_READ_REG_LEN_DEF):
    try:
        if reg_addr is None:
            status, result = exec_os_cmd(f"hexdump -C -v {dev_path}")
        else:
            status, result = exec_os_cmd(f"hexdump -C -v {dev_path} -s {reg_addr} -n {reg_len}")

        return result
    except Exception as e:
        dump_logic_register_error(f"Failed to read register from {dev_path}: {str(e)}")
        return f"Failed to read register from {dev_path}: {str(e)}"


def adjust_device_name(dev, offset=0):
    """Adjust the index of a device name by the given offset."""
    dev_base = ''.join(filter(str.isalpha, dev))
    dev_index = ''.join(filter(str.isdigit, dev))
    try:
        adjusted_index = int(dev_index) + offset
        return f"{dev_base}{adjusted_index}"
    except ValueError:
        return dev


def dump_all_registers(devices, device_type):
    for dev, alias in devices:
        adjusted_dev = adjust_device_name(dev, offset=1)
        dev_path = os.path.join(DEV_FILE_PREFIX, dev)
        if not os.path.exists(dev_path):
            print(f"Device file not found for {dev}: {dev_path}")
            continue
        print(f"\nDumping all registers for {device_type.upper()} {adjusted_dev} ({alias}):")
        print("-" * 50)
        try:
            output = read_register(dev_path)
            print(output)
        except Exception as e:
            print(f"Failed to dump registers for {adjusted_dev}: {e}")


def dump_device_register(device, alias, reg_addr=None, reg_len=LOGCI_READ_REG_LEN_DEF):
    dev_path = os.path.join(DEV_FILE_PREFIX, device)
    if not os.path.exists(dev_path):
        error_exit(f"Device file not found: {dev_path}")
        return

    try:
        adjusted_dev = adjust_device_name(device, offset=1)
        if reg_addr:
            print(f"\nDumping register {reg_addr} for {adjusted_dev} ({alias}):")
            print("-" * 50)
            output = read_register(dev_path, reg_addr, reg_len=reg_len)
        else:
            print(f"\nDumping all registers for {adjusted_dev} ({alias}):")
            print("-" * 50)
            output = read_register(dev_path)
        print(output)
    except Exception as e:
        error_exit(f"Failed to dump registers for {adjusted_dev}: {e}")
        return


def help_print():
    print("Usage:")
    print("  dump_logic_register.py")
    print("  dump_logic_register.py all")
    print("  dump_logic_register.py cpld all")
    print("  dump_logic_register.py cpldX [<reg_addr> [<reg_len>]]")
    print("  dump_logic_register.py fpga all")
    print("  dump_logic_register.py fpgaX [<reg_addr> [<reg_len>]]")


def main():
    if len(sys.argv) == 1:
        cpld_devices = get_devices(CPLD_DIR, "cpld")
        fpga_devices = get_devices(FPGA_DIR, "fpga")

        if cpld_devices:
            print("Available CPLD devices:")
            for dev, alias in cpld_devices:
                adjusted_dev = adjust_device_name(dev, offset=1)
                print(f"{adjusted_dev}: {alias}")

        if fpga_devices:
            print("\nAvailable FPGA devices:")
            for dev, alias in fpga_devices:
                adjusted_dev = adjust_device_name(dev, offset=1)
                print(f"{adjusted_dev}: {alias}")

        return

    if len(sys.argv) < ARGV_LEN_MIN or len(sys.argv) > ARGV_LEN_MAX:
        help_print()
        return

    cpld_devices = get_devices(CPLD_DIR, "cpld")
    fpga_devices = get_devices(FPGA_DIR, "fpga")

    logic_name = sys.argv[1].lower()
    if logic_name == "all":
        if cpld_devices:
            dump_all_registers(cpld_devices, "cpld")
        if fpga_devices:
            dump_all_registers(fpga_devices, "fpga")
    elif logic_name.startswith("cpld"):
        if logic_name == "cpld":
            if len(sys.argv) == 3 and sys.argv[2].lower() == "all":
                if cpld_devices:
                    dump_all_registers(cpld_devices, "cpld")
                else:
                    error_exit("No CPLD devices found.")
            else:
                help_print()
            return

        try:
            index = int(logic_name[4:])
        except ValueError:
            error_exit(f"Invalid CPLD index: {logic_name}. Available range: 1 to {len(cpld_devices)}")
            return

        if index < 1 or index > len(cpld_devices):
            error_exit(f"Invalid CPLD index: {index}. Available range: 1 to {len(cpld_devices)}")
            return

        reg_addr = None
        reg_len = LOGCI_READ_REG_LEN_DEF
        if len(sys.argv) == 3:
            reg_addr = sys.argv[2]
        elif len(sys.argv) == 4:
            reg_addr = sys.argv[2]
            reg_len = sys.argv[3]

        device_name, alias = cpld_devices[index - 1]
        dump_device_register(device_name, alias, reg_addr, reg_len)
    elif logic_name.startswith("fpga"):
        if logic_name == "fpga":
            if len(sys.argv) == 3 and sys.argv[2].lower() == "all":
                if fpga_devices:
                    dump_all_registers(fpga_devices, "fpga")
                else:
                    error_exit("No FPGA devices found.")
            else:
                help_print()
            return

        try:
            index = int(logic_name[4:])
        except ValueError:
            error_exit(f"Invalid FPGA index: {logic_name}. Available range: 1 to {len(fpga_devices)}")
            return

        if index < 1 or index > len(fpga_devices):
            error_exit(f"Invalid FPGA index: {index}. Available range: 1 to {len(fpga_devices)}")
            return

        reg_addr = None
        reg_len = LOGCI_READ_REG_LEN_DEF
        if len(sys.argv) == 3:
            reg_addr = sys.argv[2]
        elif len(sys.argv) == 4:
            reg_addr = sys.argv[2]
            reg_len = sys.argv[3]

        device_name, alias = fpga_devices[index - 1]
        dump_device_register(device_name, alias, reg_addr, reg_len)
    else:
        error_exit(f"Unknown command: {logic_name}")
        help_print()


if __name__ == "__main__":
    main()
