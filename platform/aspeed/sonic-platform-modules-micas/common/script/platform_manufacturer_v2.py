#!/usr/bin/env python_nos
# -*- coding: utf-8 -*-
import time
import fcntl
import copy
import os
import syslog
import argparse
from platform_util import *
from platform_config import *
from public.platform_common_config import MGMT_VERSION_PATH, S3IP_SYSFS_NAME

MANUFACTURER_ERROR = 1
MANUFACTURER_DEBUG = 2
DEBUGLEVEL = 0
MANUFACTURER_DEBUG_FILE = "/etc/.manufacturer_debug_flag"
args = None
CURRENT_FILE_NAME = os.path.basename(__file__)
LOG_LAST_TIME = {}

pidfile = 0

# Cache for lshw memory information
G_MEMINFO_CACHE = {}
G_MEM_CHILD_MAX_NUM = 32

SOURCE_FROM = 0
INDENT = 4

DMIDECODE_TYPE_LIST = ["processor", "bios"]

BIOS_INFO_DICT = {
    "Vendor": {
        "pattern": "Vendor",
        "separator" : ":"
    },
    "Version": {
        "pattern": "Version",
        "separator" : ":"
    },
    "Release Date": {
        "pattern": "Release",
        "separator" : ":"
    },
}

SSD_INFO_DICT = {
    "sata_ssd_pattern" : {
        "Device Model": {
            "pattern": "\"Device Model\"",
            "separator" : ":"
        },
        "Firmware Version": {
            "pattern": "\"Firmware Version\"",
            "separator" : ":"
        },
        "User Capacity": {
            "pattern": "\"User Capacity\"",
            "separator" : ":"
        },
    },

    "nvme_ssd_pattern" : {
        "Device Model": {
            "pattern": "\"Model Number\"",
            "separator" : ":"
        },
        "Firmware Version": {
            "pattern": "\"Firmware Version\"",
            "separator" : ":"
        },
        "User Capacity": {
            "pattern": "\"Namespace 1 Size/Capacity\"",
            "separator" : ":"
        },
    },
}

ONIE_INFO_DICT = {
    "Build Date" : "onie_build_date",
    "Version" : "onie_version",
    "Sub Version" : "onie_sub_version",
}

CPU_INFO_DICT = {
    "Vendor": {
        "pattern": "Manufacturer",
        "separator" : ":"
    },
    "Device Model": {
        "pattern": "Version",
        "separator" : ":"
    },
    "Core Count": {
        "pattern": "\"Core Count\"",
        "separator" : ":"
    },
    "Thread Count": {
        "pattern": "\"Thread Count\"",
        "separator" : ":"
    },
}

MEM_INFO_DICT = {
    "Total size": {
        "root_key": "memory",
        "sub_key" : "size"
    },
}

CPLD_FIELD = {
    "Device Model": "type",
    "Vendor": "vendor",
    "Description": "alias",
    "Firmware Version": "firmware_version",
}

BMC_FIELD = {
    "Version": "FirmwareVersion",
}

PSU_FIELD = {
    "Hardware Version": "hardware_version",
    "Part Number": "part_number",
    "Serial Number": "serial_number",
    "Firmware Version": "firmware_version",
}

FAN_FIELD = {
    "Hardware Version": "hardware_version",
    "Part Number": "part_number",
    "Serial Number": "serial_number",
    "Firmware Version": {
        "decode_type" : "direct_config",
        "value": "NA"
    },
}

FPGA_FIELD = {
    "Device Model": "type",
    "Vendor": "vendor",
    "Description": "alias",
    "Hardware Version": {
        "decode_type" : "direct_config",
        "value": "NA"
    },
    "Firmware Version": "firmware_version",
}

OTHER_FIELD = {
    "Description": "alias",
    "Firmware Version": "firmware_version",
}

def manufacturer_debug(s):
    global DEBUGLEVEL
    if MANUFACTURER_DEBUG & DEBUGLEVEL:
        syslog.openlog("MANUFACTURER", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def manufacturer_error(s):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        syslog.openlog("MANUFACTURER", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)

def debug_init():
    global DEBUGLEVEL
    try:
        with open(MANUFACTURER_DEBUG_FILE, "r") as fd:
            value = fd.read()
        DEBUGLEVEL = int(value)
    except Exception as e:
        DEBUGLEVEL = 0

# Process Lock
def ApplicationInstance():
    global pidfile
    pidfile = open(os.path.realpath(__file__), "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)  # Create an exclusive lock, and the processes that are locked will not be blocked.
        return True
    except Exception as e:
        return False

def lshw_memory_split():
    # improve performance
    if G_MEMINFO_CACHE:
        return
    cmd = "lshw -c memory"
    status, output = exec_os_cmd(cmd)
    if status or len(output) == 0:
        manufacturer_error("run cmd: {} error, status: {}, msg: {}".format(cmd, status, output))
        return
    memlist = output.strip().split("*-")
    for item in memlist:
        if item.strip().startswith("memory") and "System Memory" not in item:
            continue
        line_index = 0
        for line in item.splitlines():
            line_index += 1
            if line_index == 1:
                memdict_key = line
                G_MEMINFO_CACHE[memdict_key] = {}
            else:
                if ":" not in line:
                    continue
                key = line.split(":", 1)[0].strip()
                value = line.split(":", 1)[1].strip()
                G_MEMINFO_CACHE[memdict_key][key] = value
            if "empty" in item:
                break
    manufacturer_debug("lshw memory info: %s" % G_MEMINFO_CACHE)

def get_mem_child_info(config, indent):
    lshw_memory_split()
    result_str = ""
    for key, val in config.items():
        for sub_key, sub_val in val.items():
            mem_root_key = sub_val.get("root_key", "")
            mem_sub_key = sub_val.get("sub_key", "")
            meminfo_val = G_MEMINFO_CACHE.get(mem_root_key, {}).get(mem_sub_key)
            if meminfo_val is None:
                # meminfo_val = ("ERR get_mem_child_info fail, mem_root_key : %s, mem_sub_key: %s " % (mem_root_key, mem_sub_key))
                # Get all discoverable memory banks
                continue
            # add title
            if key not in result_str:
                result_str += "%s:\n" % (key)
            result_str += "%s%s:%s\n" % (indent * " ", sub_key, meminfo_val)
    return result_str

def get_mem_info():
    lshw_memory_split()
    result_str = ""

    child_cfg = {
        "Bank" + str(i): {
            "Description": {
                "root_key": "bank:" + str(i),
                "sub_key": "description"
            },
            "Vendor": {
                "root_key": "bank:" + str(i),
                "sub_key": "vendor"
            },
            "Clock": {
                "root_key": "bank:" + str(i),
                "sub_key": "clock"
            },
            "Size": {
                "root_key": "bank:" + str(i),
                "sub_key": "size"
            },
        } for i in range(G_MEM_CHILD_MAX_NUM)
    }
    MEM_INFO_DICT['child'] = child_cfg

    for key, val in MEM_INFO_DICT.items():
        if key == "child":
            child_info = get_mem_child_info(val, INDENT)
            result_str += child_info
            continue
        else:
            mem_root_key = val.get("root_key", "")
            mem_sub_key = val.get("sub_key", "")
            meminfo_val = G_MEMINFO_CACHE.get(mem_root_key, {}).get(mem_sub_key)
            if meminfo_val is None:
                # meminfo_val = ("ERR get_mem_info fail, mem_root_key : %s, mem_sub_key: %s " % (mem_root_key, mem_sub_key))
                # Get all discoverable memory banks
                continue
            result_str += "%s:%s\n" % (key, meminfo_val)

    if len(result_str) == 0:
        result_str = "ERR get mem info null"

    return result_str

def get_ssd_split_info(ssd_type, pattern=None, separator=None):
    if pattern:
        cmd = "smartctl -i /dev/%s | grep %s" % (ssd_type, pattern)
    else:
        cmd = "smartctl -i /dev/%s" % ssd_type
    status, output = exec_os_cmd(cmd)
    if status:
        return "exec cmd fail, cmd: %s, reason: %s" % (cmd, output)
    if separator:
        separator_value = output.split(separator)[1].strip()
        return separator_value
    else:
        return output.strip()

def _get_ssd_result_str(ssd_pattern, ssd_list, indent_space_num):
    result_str = ""
    for ssd_name in ssd_list:
        if indent_space_num != 0:
            result_str += "%s:\n" % ssd_name.upper()

        for ssd_print_field, ssd_print_field_config in ssd_pattern.items():
            value = get_ssd_split_info(ssd_name, ssd_print_field_config.get("pattern"), ssd_print_field_config.get("separator"))
            if indent_space_num == 0:
                result_str += "%s:%s\n" % (ssd_print_field, value)
            else:
                result_str += "%s%s:%s\n" % (indent_space_num * " ", ssd_print_field, value)
    return result_str


def is_usb_device(device):
    try:
        cmd = "udevadm info -q property -n /dev/%s | grep ID_BUS" % device
        status, output = exec_os_cmd(cmd)
        if status == 0 and "ID_BUS=usb" in output:
            return True
    except Exception as e:
        pass
    return False


def get_ssd_info():
    devices = os.listdir('/dev')
    sata_ssd_list = []
    for dev in devices:
        if dev.startswith('sd') and len(dev) == 3:
            if not is_usb_device(dev):
                sata_ssd_list.append(dev)
    sata_ssd_list.sort()
    nvme_ssd_list = [dev for dev in devices if dev.startswith('nvme') and len(dev) == 5]
    nvme_ssd_list.sort()
    sata_ssd_num = len(sata_ssd_list)
    nvme_ssd_num = len(nvme_ssd_list)

    sata_ssd_pattern = SSD_INFO_DICT.get("sata_ssd_pattern")
    nvme_ssd_pattern = SSD_INFO_DICT.get("nvme_ssd_pattern")

    indent_space_num = 0
    if sata_ssd_num + nvme_ssd_num > 1:
        indent_space_num = 4

    sata_ssd_result_str = _get_ssd_result_str(sata_ssd_pattern, sata_ssd_list, indent_space_num)
    nvme_ssd_result_str = _get_ssd_result_str(nvme_ssd_pattern, nvme_ssd_list, indent_space_num)

    result_str = sata_ssd_result_str + nvme_ssd_result_str
    if len(result_str) == 0:
        result_str = "ERR get ssd info null"

    return result_str

def get_onie_info_str(info=None):
    result_str = ""
    if info:
        for item in info:
            onie_default_config = ONIE_INFO_DICT.get(item)
            value = get_onie_info(onie_default_config)
            result_str += "%s:%s\n" % (item, value)
    else:
        for onie_type, onie_default_config in ONIE_INFO_DICT.items():
            value = get_onie_info(onie_default_config)
            result_str += "%s:%s\n" % (onie_type, value)

    if len(result_str) == 0:
        result_str = "ERR get onie info null"
    return result_str

def get_bios_role_info_str(config=None):
    # Validate config type: must be dict or None (None is handled in next step)
    if not isinstance(config, (dict, type(None))):
        return ""

    # Return empty string if config is None
    if config is None:
        return ""

    # Get "value" field from config and validate it's not None
    value_key = config.get("value")
    if value_key is None:
        return ""  # Return empty if "value" is missing or None

    # Call get_value with valid value_key (avoid passing None)
    ret, val = get_value(value_key)
    if not ret:
        return ""  # Return empty if get_value fails

    # Get "decode" map and ensure it's a dict (fallback to empty dict if invalid)
    decode_map = config.get("decode", {})
    if not isinstance(decode_map, dict):
        decode_map = {}  # Force to empty dict if "decode" is not a dictionary

    # Get decoded value (return "NA" if not found) and format result string
    decode_value = decode_map.get(val, "NA")
    return f"Boot from: {decode_value}"

def get_dmidecode_split_info(device_type, pattern=None, separator=None):
    if device_type not in DMIDECODE_TYPE_LIST:
        return "dmidecode not support device_type: %s" % device_type
    if pattern:
        cmd = "dmidecode --type %s | grep %s" % (device_type, pattern)
    else:
        cmd = "dmidecode --type %s" % device_type
    status, output = exec_os_cmd(cmd)
    if status:
        return "exec cmd fail, cmd: %s, reason: %s" % (cmd, output)
    if separator:
        separator_value = output.split(separator)[1].strip()
        return separator_value
    else:
        return output.strip()

def get_dmidecode_info(device_type, print_fields=None):
    #get default dmidecode config
    if device_type == "bios":
        device_default_configs = BIOS_INFO_DICT
    elif device_type == "processor":
        device_default_configs = CPU_INFO_DICT
    else:
        return "unsupport dmidecode device_type %s" % device_type

    result_str = ""
    #if config define which fields to print
    if print_fields:
        for item in print_fields:
            device_default_config = device_default_configs.get(item)
            value = get_dmidecode_split_info(device_type, device_default_config.get("pattern"), device_default_config.get("separator"))
            result_str += "%s:%s\n" % (item, value)
    else:
        for device, device_default_config in device_default_configs.items():
            value = get_dmidecode_split_info(device_type, device_default_config.get("pattern"), device_default_config.get("separator"))
            result_str += "%s:%s\n" % (device, value)

    if len(result_str) == 0:
        result_str = "ERR get %s info null" % device_type
    return result_str

def print_title(title, indent):
    indent_str = indent * " "
    if indent == 0:
        print("\n%s%s:" % (indent_str ,title))
    else:
        print("%s%s:" % (indent_str ,title))

def deal_config_field(config):
    if config.get("field"):
        return

    dev_type =  config.get("type")
    if dev_type == DEV_TYPE_CPLD:
        config["field"] = CPLD_FIELD
    elif dev_type == DEV_TYPE_PSU:
        config["field"] = PSU_FIELD
    elif dev_type == DEV_TYPE_FAN:
        config["field"] = FAN_FIELD
    elif dev_type == DEV_TYPE_FPGA:
        config["field"] = FPGA_FIELD
    elif dev_type == DEV_TYPE_BMC:
        config["field"] = BMC_FIELD
    elif dev_type == DEV_TYPE_MISC:
        config["field"] = OTHER_FIELD



def get_info_str(config):
    result_str = ""
    deal_config_field(config)
    if config.get("source") == GET_BY_RESTFUL:
        ret, restful_val = get_multiple_info_from_restful(config)
    elif config.get("source") == GET_BY_S3IP:
        ret, restful_val = get_multiple_info_from_s3ip(config)
    else:
        return "unsupport source %s." % config.get("source")

    if not ret:
        return restful_val

    for key, val in restful_val.items():
        result_str += "%s:%s\n" % (key, val)
    return result_str

def get_info_from_common(config):
    way = config.get("gettype")
    if way == 'cmd':
        cmd = config.get("cmd")
        ret, val = exec_os_cmd(cmd)
        if ret:
            return False, ("cmd read exec %s failed, log: %s" % (cmd, val))
        else:
            return True, val
    elif way == 'direct_config':
        return True, config.get("value")
    elif way == 'func':
        return True, eval(config.get("funcname"))(config.get("params"))
    else:
        return False, "not support read type: %s" % way

def deal_itmes(item_list):
    for item in item_list:
        ret, log = set_value(item)
        if not ret:
            print("deal items error:%s" % log)

def write_mgmt_version_file(version):
    dir_path = os.path.dirname(MGMT_VERSION_PATH)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

    if not os.path.exists(MGMT_VERSION_PATH):
        with open(MGMT_VERSION_PATH, 'w') as file:
            file.write('')

    with open(MGMT_VERSION_PATH, 'r+') as file:
        content = file.read().strip()
        if content != version:
            file.seek(0)  # set point to head
            file.truncate()  # clear the content
            file.write(f"{version}\n")
            log = "%s: old mgmt version: %s not equal new mgmt version: %s, write new version to file: %s" % (CURRENT_FILE_NAME, content, version, MGMT_VERSION_PATH)
            log_to_file(log, BSP_COMMON_LOG_PATH)
        else:
            log = "%s: old mgmt version: %s equal new mgmt version: %s, do nothing" % (CURRENT_FILE_NAME, content, version)
            log_to_file(log, BSP_COMMON_LOG_PATH)

def get_info_by_cmd(params):
    version = ""
    try:
        before_deal_list = params.get("before", [])
        deal_itmes(before_deal_list)

        ret, version = exec_os_cmd(params["get_version"])
        if ret != 0:  # Failed to obtain the version number
            version = "ERR " + version

        after_deal_list = params.get("after", [])
        deal_itmes(after_deal_list)

    except Exception as e:
        version = "ERR %s" % (str(e))
    finally:
        finally_deal_list = params.get("finally", [])
        deal_itmes(finally_deal_list)
    return version

def get_and_generate_mgmt_version(params):
    version = get_info_by_cmd(params)
    if "ERR" not in version and args.update:
        write_mgmt_version_file(version)
    return version

def format_string(input_string, line_indent):
    lines = input_string.strip().split('\n')
    # Format each line
    formatted_string = []
    for line in lines:
        # Add indentation after the colon if it exists
        if ':' in line:
            key, value = line.split(':', 1)
            formatted_line = ("%s%s:%s%s" % ((line_indent * " "), key, ((35 - len(key) - line_indent) * " "), value))
        else:
            formatted_line = line

        formatted_string.append(formatted_line)
    # Return the formatted string
    return '\n'.join(formatted_string)

def print_class(key, config, indent):
    get_manuinfo_way = config.get("source", GET_BY_COMMON)
    if get_manuinfo_way == GET_BY_S3IP:
        dev_type = config.get("type")
        num_sysfs = f"/sys/{S3IP_SYSFS_NAME}/{S3IP_PREFIX_DIR_NAME.get(dev_type, dev_type)}/number"
        ret, num = read_sysfs(num_sysfs)
        if ret is False:
            err_msg = "get %s num fail, reason: %s" % (dev_type, num)
            print(format_string(err_msg, indent + INDENT))
            return

        for i in range(1, int(num) + 1):
            print_title("%s%s" % (key, i), indent)
            config["index"] = i
            value = get_info_str(config)
            format_val = format_string(value, indent + INDENT)
            print(format_val)

    elif get_manuinfo_way == GET_BY_RESTFUL:
        num_config = {}
        num_config["type"] = config.get("type")
        num_config["field_name"] = "num"
        ret, num = get_single_info_from_restful(num_config)
        if ret is False:
            err_msg = ("get %s num fail, reason: %s" % (num_config["type"], num))
            print(format_string(err_msg, indent + INDENT))
            return

        for i in range(1, num + 1):
            print_title("%s%s" % (key, i), indent)
            config["index"] = i
            value = get_info_str(config)
            format_val = format_string(value, indent + INDENT)
            print(format_val)
    else:
        print("%sunsupport class: %s" % (" " * indent, get_manuinfo_way))

def print_manufacturer(manuinfo_config, indent = 0):
    for device, print_config in manuinfo_config.items():
        print_title(device, indent)

        if print_config.get("class", 0):
            print_class(device, print_config, indent + INDENT)
            continue

        #save child_config, final push to print_manufacturer
        child_config = print_config.get("child")
        if child_config is not None:
            print_config.pop("child")
            print_manufacturer(child_config, indent + INDENT)
            continue

        get_manuinfo_way = print_config.get("source", GET_BY_COMMON)
        if get_manuinfo_way in DMIDECODE_TYPE_LIST:
            value = get_dmidecode_info(get_manuinfo_way, print_config.get("print"))
            if get_manuinfo_way == "bios":
                value += get_bios_role_info_str(print_config.get("bios_boot", None))
            format_val = format_string(value, indent + INDENT)
            print(format_val)
            continue
        elif get_manuinfo_way == "onie":
            value = get_onie_info_str(print_config.get("print"))
            format_val = format_string(value, indent + INDENT)
            print(format_val)
            continue
        elif get_manuinfo_way == "memory":
            value = get_mem_info()
            format_val = format_string(value, indent + INDENT)
            print(format_val)
            continue
        elif get_manuinfo_way == "ssd":
            value = get_ssd_info()
            format_val = format_string(value, indent + INDENT)
            print(format_val)
            continue
        elif get_manuinfo_way == GET_BY_S3IP:
            value = get_info_str(print_config)
            format_val = format_string(value, indent + INDENT)
            print(format_val)
        elif get_manuinfo_way == GET_BY_COMMON:
            for print_key, print_value in print_config.items():
                ret, value = get_info_from_common(print_value)
                info = "%s:%s" % (print_key, value)
                format_val = format_string(info, indent + INDENT)
                print(format_val)
        elif get_manuinfo_way == GET_BY_RESTFUL:
            ret, value = get_info_str(print_config)
            format_val = format_string(value, indent + INDENT)
            print(format_val)
        else:
            print("%sunsupport way" % (INDENT * " "))

def show_manufacturer_info(manuinfo_config = None):
    # Determine whether the process exists and restrict it to run only once.
    start_time = get_monotonic_time()
    while True:
        ret = ApplicationInstance()
        if ret == True:
            break
        if get_monotonic_time() - start_time > 10:
            print("manufacturer is running.")
            exit(1)
        time.sleep(0.5)
    if manuinfo_config:
        config = manuinfo_config
    else:
        config = copy.deepcopy(MANUINFO_CONF)
    version = config.get("version2", 0)
    if version:
        config.pop("version2")
    print_manufacturer(config)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Print firmware version v2.')
    parser.add_argument('-u','--update', action='store_true', help='update firmware version.')

    args = parser.parse_args()

    debug_init()
    show_manufacturer_info()
