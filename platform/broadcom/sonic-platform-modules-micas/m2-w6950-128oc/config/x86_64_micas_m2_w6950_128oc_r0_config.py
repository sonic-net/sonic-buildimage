#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 0,
    "tty_console": 0,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "reboot_cause": 0,
    "product_name": 1,
    "dfx_xdpe_monitor": 0,
    "mgmt_ledctrl": 1,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/psu/psu1/present", "okval": 1},
             "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 67, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "24c02", "bus": 67, "loc": 0x50, "attr": "eeprom"}
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/psu/psu2/present", "okval": 1},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 68, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "24c02", "bus": 68, "loc": 0x50, "attr": "eeprom"}
            ],
        },
        {
            "name": "psu3",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/psu/psu3/present", "okval": 1},
            "device": [
                {"id": "psu3pmbus", "name": "wb_fsp1200", "bus": 69, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu3frue2", "name": "24c02", "bus": 69, "loc": 0x50, "attr": "eeprom"}
            ],
        },
        {
            "name": "psu4",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/psu/psu4/present", "okval": 1},
            "device": [
                {"id": "psu4pmbus", "name": "wb_fsp1200", "bus": 70, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu4frue2", "name": "24c02", "bus": 70, "loc": 0x50, "attr": "eeprom"}
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan1/present", "okval": 1},
            "device": [
                {"id": "fan1frue2", "name": "24c64", "bus": 77, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan2/present", "okval": 1},
            "device": [
                {"id": "fan2frue2", "name": "24c64", "bus": 85, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan3/present", "okval": 1},
            "device": [
                {"id": "fan3frue2", "name": "24c64", "bus": 78, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan4/present", "okval": 1},
            "device": [
                {"id": "fan4frue2", "name": "24c64", "bus": 86, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan5/present", "okval": 1},
            "device": [
                {"id": "fan5frue2", "name": "24c64", "bus": 79, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan6/present", "okval": 1},
            "device": [
                {"id": "fan6frue2", "name": "24c64", "bus": 87, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan7",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan7/present", "okval": 1},
            "device": [
                {"id": "fan7frue2", "name": "24c64", "bus": 80, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan7",
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/fan/fan8/present", "okval": 1},
            "device": [
                {"id": "fan8frue2", "name": "24c64", "bus": 88, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "24c02", "bus": 1, "loc": 0x56, "attr": "eeprom"},
                {"id": "eeprom_2", "name": "24c02", "bus": 1, "loc": 0x57, "attr": "eeprom"},
                {"id": "eeprom_3", "name": "24c02", "bus": 51, "loc": 0x57, "attr": "eeprom"},
                {"id": "eeprom_4", "name": "24c02", "bus": 97, "loc": 0x57, "attr": "eeprom"},
                {"id": "eeprom_5", "name": "24c02", "bus": 99, "loc": 0x57, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "lm75", "bus": 54, "loc": 0x4a, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 55, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 56, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 76, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 84, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_6", "name": "lm75", "bus": 93, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_7", "name": "lm75", "bus": 101, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_8", "name": "lm75", "bus": 123, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_9", "name": "lm75", "bus": 124, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 53, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 54, "loc": 0x5f, "attr": "hwmon"},
                {"id": "ucd90160_3", "name": "ucd90160", "bus": 92, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_4", "name": "ucd90160", "bus": 100, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_5", "name": "ucd90160", "bus": 128, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_6", "name": "ucd90160", "bus": 129, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_7", "name": "ucd90160", "bus": 130, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_8", "name": "ucd90160", "bus": 57, "loc": 0x68, "attr": "hwmon"},
            ],
        },
        {
            "name": "ads7828",
            "device": [
                {"id": "ads7828_1", "name": "ads7828", "bus": 60, "loc": 0x48, "attr": "hwmon"},
            ],
        },
    ],
    "binddevs": [
        {
            "name": "wb-vr-common",
            "driver_type": "platform",
            "device": [
                {"id": "vr_1", "name": "wb-vr-common.1",   "bus": 54,  "loc": 0x72, "attr": "hwmon"},
                {"id": "vr_2", "name": "wb-vr-common.2",   "bus": 54,  "loc": 0x74, "attr": "hwmon"},
                {"id": "vr_3", "name": "wb-vr-common.3",   "bus": 94,  "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_4", "name": "wb-vr-common.4",   "bus": 95,  "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_5", "name": "wb-vr-common.5",   "bus": 96,  "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_6", "name": "wb-vr-common.6",   "bus": 97,  "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_7", "name": "wb-vr-common.7",   "bus": 102, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_8", "name": "wb-vr-common.8",   "bus": 103, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_9", "name": "wb-vr-common.9",   "bus": 104, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_10", "name": "wb-vr-common.10", "bus": 105, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_11", "name": "wb-vr-common.11", "bus": 107, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_12", "name": "wb-vr-common.12", "bus": 108, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_13", "name": "wb-vr-common.13", "bus": 109, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_14", "name": "wb-vr-common.14", "bus": 110, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_15", "name": "wb-vr-common.15", "bus": 111, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_16", "name": "wb-vr-common.16", "bus": 112, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_17", "name": "wb-vr-common.17", "bus": 113, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_18", "name": "wb-vr-common.18", "bus": 114, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_19", "name": "wb-vr-common.19", "bus": 115, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_20", "name": "wb-vr-common.20", "bus": 116, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_21", "name": "wb-vr-common.21", "bus": 117, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_22", "name": "wb-vr-common.22", "bus": 118, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_23", "name": "wb-vr-common.23", "bus": 119, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_24", "name": "wb-vr-common.24", "bus": 120, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_25", "name": "wb-vr-common.25", "bus": 121, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_26", "name": "wb-vr-common.26", "bus": 122, "loc": 0x60, "attr": "hwmon"},
                {"id": "vr_27", "name": "wb-vr-common.27", "bus": 136, "loc": 0x60, "attr": "hwmon"},
            ],
        },
    ],
}


MANUINFO_CONF = {
    "bios": {
        "key": "BIOS",
        "head": True,
        "next": "onie"
    },
    "bios_vendor": {
        "parent": "bios",
        "key": "Vendor",
        "cmd": "dmidecode -t 0 |grep Vendor",
        "pattern": r".*Vendor",
        "separator": ":",
        "arrt_index": 1,
    },
    "bios_version": {
        "parent": "bios",
        "key": "Version",
        "cmd": "dmidecode -t 0 |grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "bios_date": {
        "parent": "bios",
        "key": "Release Date",
        "cmd": "dmidecode -t 0 |grep Release",
        "pattern": r".*Release Date",
        "separator": ":",
        "arrt_index": 3,
    },
    "onie": {
        "key": "ONIE",
        "next": "cpu"
    },
    "onie_date": {
        "parent": "onie",
        "key": "Build Date",
        "file": "/host/machine.conf",
        "pattern": r"^onie_build_date",
        "separator": "=",
        "arrt_index": 1,
    },
    "onie_version": {
        "parent": "onie",
        "key": "Version",
        "file": "/host/machine.conf",
        "pattern": r"^onie_version",
        "separator": "=",
        "arrt_index": 2,
    },

    "cpu": {
        "key": "CPU",
        "next": "ssd"
    },
    "cpu_vendor": {
        "parent": "cpu",
        "key": "Vendor",
        "cmd": "dmidecode --type processor |grep Manufacturer",
        "pattern": r".*Manufacturer",
        "separator": ":",
        "arrt_index": 1,
    },
    "cpu_model": {
        "parent": "cpu",
        "key": "Device Model",
        "cmd": "dmidecode --type processor | grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "cpu_core": {
        "parent": "cpu",
        "key": "Core Count",
        "cmd": "dmidecode --type processor | grep \"Core Count\"",
        "pattern": r".*Core Count",
        "separator": ":",
        "arrt_index": 3,
    },
    "cpu_thread": {
        "parent": "cpu",
        "key": "Thread Count",
        "cmd": "dmidecode --type processor | grep \"Thread Count\"",
        "pattern": r".*Thread Count",
        "separator": ":",
        "arrt_index": 4,
    },
    "ssd": {
        "key": "SSD",
        "next": "cpld"
    },
    "ssd_model": {
        "parent": "ssd",
        "key": "Model Number",
        "cmd": "smartctl -i /dev/nvme0n1 |grep \"Model Number\"",
        "pattern": r".*Model Number",
        "separator": ":",
        "arrt_index": 1,
    },
    "ssd_fw": {
        "parent": "ssd",
        "key": "Firmware Version",
        "cmd": "smartctl -i /dev/nvme0n1 |grep \"Firmware Version\"",
        "pattern": r".*Firmware Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "ssd_user_cap": {
        "parent": "ssd",
        "key": "User Capacity",
        "cmd": "smartctl -i /dev/nvme0n1 |grep \"Namespace 1 Size/Capacity\"",
        "pattern": r".*Namespace 1 Size/Capacity",
        "separator": ":",
        "arrt_index": 3,
    },

    "cpld": {
        "key": "CPLD",
        "next": "psu"
    },

    "cpld1": {
        "key": "CPLD1",
        "parent": "cpld",
        "arrt_index": 1,
    },
    "cpld1_model": {
        "key": "Device Model",
        "parent": "cpld1",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld1_vender": {
        "key": "Vendor",
        "parent": "cpld1",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld1_desc": {
        "key": "Description",
        "parent": "cpld1",
        "config": "CPU_CPLD",
        "arrt_index": 3,
    },
    "cpld1_version": {
        "key": "Firmware Version",
        "parent": "cpld1",
        "reg": {
            "loc": "/dev/cpld0",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld2": {
        "key": "CPLD2",
        "parent": "cpld",
        "arrt_index": 2,
    },
    "cpld2_model": {
        "key": "Device Model",
        "parent": "cpld2",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld2_vender": {
        "key": "Vendor",
        "parent": "cpld2",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld2_desc": {
        "key": "Description",
        "parent": "cpld2",
        "config": "MGMT_CPLD",
        "arrt_index": 3,
    },
    "cpld2_version": {
        "key": "Firmware Version",
        "parent": "cpld2",
        "reg": {
            "loc": "/dev/cpld1",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld3": {
        "key": "CPLD3",
        "parent": "cpld",
        "arrt_index": 3,
    },
    "cpld3_model": {
        "key": "Device Model",
        "parent": "cpld3",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld3_vender": {
        "key": "Vendor",
        "parent": "cpld3",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld3_desc": {
        "key": "Description",
        "parent": "cpld3",
        "config": "MAC_CPLD_A",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "reg": {
            "loc": "/dev/cpld2",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld4": {
        "key": "CPLD4",
        "parent": "cpld",
        "arrt_index": 4,
    },
    "cpld4_model": {
        "key": "Device Model",
        "parent": "cpld4",
        "config": "LCMXO3LF-4300C-5BG324C",
        "arrt_index": 1,
    },
    "cpld4_vender": {
        "key": "Vendor",
        "parent": "cpld4",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld4_desc": {
        "key": "Description",
        "parent": "cpld4",
        "config": "MAC_CPLD_B",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "reg": {
            "loc": "/dev/cpld3",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld5": {
        "key": "CPLD5",
        "parent": "cpld",
        "arrt_index": 5,
    },
    "cpld5_model": {
        "key": "Device Model",
        "parent": "cpld5",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld5_vender": {
        "key": "Vendor",
        "parent": "cpld5",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld5_desc": {
        "key": "Description",
        "parent": "cpld5",
        "config": "MAC_CPLD_C",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "reg": {
            "loc": "/dev/cpld4",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld6": {
        "key": "CPLD6",
        "parent": "cpld",
        "arrt_index": 6,
    },
    "cpld6_model": {
        "key": "Device Model",
        "parent": "cpld6",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld6_vender": {
        "key": "Vendor",
        "parent": "cpld6",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld6_desc": {
        "key": "Description",
        "parent": "cpld6",
        "config": "UPORT_CPLD",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "reg": {
            "loc": "/dev/cpld5",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld7": {
        "key": "CPLD7",
        "parent": "cpld",
        "arrt_index": 7,
    },
    "cpld7_model": {
        "key": "Device Model",
        "parent": "cpld7",
        "config": "LCMXO3LF-6900C-5BG400C",
        "arrt_index": 1,
    },
    "cpld7_vender": {
        "key": "Vendor",
        "parent": "cpld7",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld7_desc": {
        "key": "Description",
        "parent": "cpld7",
        "config": "DPORT_CPLD",
        "arrt_index": 3,
    },
    "cpld7_version": {
        "key": "Firmware Version",
        "parent": "cpld7",
        "reg": {
            "loc": "/dev/cpld6",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld8": {
        "key": "CPLD8",
        "parent": "cpld",
        "arrt_index": 8,
    },
    "cpld8_model": {
        "key": "Device Model",
        "parent": "cpld8",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld8_vender": {
        "key": "Vendor",
        "parent": "cpld8",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld8_desc": {
        "key": "Description",
        "parent": "cpld8",
        "config": "UFAN_CPLD",
        "arrt_index": 3,
    },
    "cpld8_version": {
        "key": "Firmware Version",
        "parent": "cpld8",
        "reg": {
            "loc": "/dev/cpld7",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld9": {
        "key": "CPLD9",
        "parent": "cpld",
        "arrt_index": 9,
    },
    "cpld9_model": {
        "key": "Device Model",
        "parent": "cpld9",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld9_vender": {
        "key": "Vendor",
        "parent": "cpld9",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld9_desc": {
        "key": "Description",
        "parent": "cpld9",
        "config": "DFAN_CPLD",
        "arrt_index": 3,
    },
    "cpld9_version": {
        "key": "Firmware Version",
        "parent": "cpld9",
        "reg": {
            "loc": "/dev/cpld8",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld10": {
        "key": "CPLD10",
        "parent": "cpld",
        "arrt_index": 10,
    },
    "cpld10_model": {
        "key": "Device Model",
        "parent": "cpld10",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld10_vender": {
        "key": "Vendor",
        "parent": "cpld10",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld10_desc": {
        "key": "Description",
        "parent": "cpld10",
        "config": "MAC_CPLD_D",
        "arrt_index": 3,
    },
    "cpld10_version": {
        "key": "Firmware Version",
        "parent": "cpld10",
        "reg": {
            "loc": "/dev/cpld9",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "psu": {
        "key": "PSU",
        "next": "fan"
    },

    "psu1": {
        "parent": "psu",
        "key": "PSU1",
        "arrt_index": 1,
    },
    "psu1_hw_version": {
        "key": "Hardware Version",
        "parent": "psu1",
        "extra": {
            "funcname": "getPsu",
            "id": "psu1",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "psu1_fw_version": {
        "key": "Firmware Version",
        "parent": "psu1",
        "config": "NA",
        "arrt_index": 2,
    },

    "psu2": {
        "parent": "psu",
        "key": "PSU2",
        "arrt_index": 2,
    },
    "psu2_hw_version": {
        "key": "Hardware Version",
        "parent": "psu2",
        "extra": {
            "funcname": "getPsu",
            "id": "psu2",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "psu2_fw_version": {
        "key": "Firmware Version",
        "parent": "psu2",
        "config": "NA",
        "arrt_index": 2,
    },

    "psu3": {
        "parent": "psu",
        "key": "PSU3",
        "arrt_index": 3,
    },
    "psu3_hw_version": {
        "key": "Hardware Version",
        "parent": "psu3",
        "extra": {
            "funcname": "getPsu",
            "id": "psu3",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "psu3_fw_version": {
        "key": "Firmware Version",
        "parent": "psu3",
        "config": "NA",
        "arrt_index": 2,
    },

    "psu4": {
        "parent": "psu",
        "key": "PSU4",
        "arrt_index": 4,
    },
    "psu4_hw_version": {
        "key": "Hardware Version",
        "parent": "psu4",
        "extra": {
            "funcname": "getPsu",
            "id": "psu4",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "psu4_fw_version": {
        "key": "Firmware Version",
        "parent": "psu4",
        "config": "NA",
        "arrt_index": 2,
    },
    "fan": {
        "key": "FAN",
        "next": "i210"
    },
    "fan1": {
        "key": "FAN1",
        "parent": "fan",
        "arrt_index": 1,
    },
    "fan1_hw_version": {
        "key": "Hardware Version",
        "parent": "fan1",
        "extra": {
            "funcname": "checkFan",
            "id": "fan1",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan1_fw_version": {
        "key": "Firmware Version",
        "parent": "fan1",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan2": {
        "key": "FAN2",
        "parent": "fan",
        "arrt_index": 2,
    },
    "fan2_hw_version": {
        "key": "Hardware Version",
        "parent": "fan2",
        "extra": {
            "funcname": "checkFan",
            "id": "fan2",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan2_fw_version": {
        "key": "Firmware Version",
        "parent": "fan2",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan3": {
        "key": "FAN3",
        "parent": "fan",
        "arrt_index": 3,
    },
    "fan3_hw_version": {
        "key": "Hardware Version",
        "parent": "fan3",
        "extra": {
            "funcname": "checkFan",
            "id": "fan3",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan3_fw_version": {
        "key": "Firmware Version",
        "parent": "fan3",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan4": {
        "key": "FAN4",
        "parent": "fan",
        "arrt_index": 4,
    },
    "fan4_hw_version": {
        "key": "Hardware Version",
        "parent": "fan4",
        "extra": {
            "funcname": "checkFan",
            "id": "fan4",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan4_fw_version": {
        "key": "Firmware Version",
        "parent": "fan4",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan5": {
        "key": "FAN5",
        "parent": "fan",
        "arrt_index": 5,
    },
    "fan5_hw_version": {
        "key": "Hardware Version",
        "parent": "fan5",
        "extra": {
            "funcname": "checkFan",
            "id": "fan5",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan5_fw_version": {
        "key": "Firmware Version",
        "parent": "fan5",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan6": {
        "key": "FAN6",
        "parent": "fan",
        "arrt_index": 6,
    },
    "fan6_hw_version": {
        "key": "Hardware Version",
        "parent": "fan6",
        "extra": {
            "funcname": "checkFan",
            "id": "fan6",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan6_fw_version": {
        "key": "Firmware Version",
        "parent": "fan6",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan7": {
        "key": "FAN7",
        "parent": "fan",
        "arrt_index": 7,
    },
    "fan7_hw_version": {
        "key": "Hardware Version",
        "parent": "fan7",
        "extra": {
            "funcname": "checkFan",
            "id": "fan7",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan7_fw_version": {
        "key": "Firmware Version",
        "parent": "fan7",
        "config": "NA",
        "arrt_index": 2,
    },

    "fan8": {
        "key": "FAN8",
        "parent": "fan",
        "arrt_index": 8,
    },
    "fan8_hw_version": {
        "key": "Hardware Version",
        "parent": "fan8",
        "extra": {
            "funcname": "checkFan",
            "id": "fan8",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "fan8_fw_version": {
        "key": "Firmware Version",
        "parent": "fan8",
        "config": "NA",
        "arrt_index": 2,
    },
    "i210": {
        "key": "NIC",
        "next": "fpga"
    },
    "i210_model": {
        "parent": "i210",
        "config": "NA",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "i210_vendor": {
        "parent": "i210",
        "config": "INTEL",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "i210_version": {
        "parent": "i210",
        "cmd": "ethtool -i eth0",
        "pattern": r"firmware-version",
        "separator": ":",
        "key": "Firmware Version",
        "arrt_index": 3,
    },
    "fpga": {
        "key": "FPGA",
        "next": "others"
    },

    "fpga1": {
        "key": "FPGA1",
        "parent": "fpga",
        "arrt_index": 1,
    },
    "fpga1_model": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":0x10,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "00000050": "XC7A50T-2FGG484I",
            "00000100": "XC7A100T-2FGG484I"
        },
        "key": "Device Model",
        "arrt_index": 1,
    },
    "fpga1_vender": {
        "parent": "fpga1",
        "config": "XILINX",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "fpga1_desc": {
        "key": "Description",
        "parent": "fpga1",
        "config": "MAC_FPGA",
        "arrt_index": 3,
    },
    "fpga1_hw_version": {
        "parent": "fpga1",
        "config": "NA",
        "key": "Hardware Version",
        "arrt_index": 4,
    },
    "fpga1_fw_version": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":0,
            "len":4,
            "bit_width":4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga1_date": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":4,
            "len":4,
            "bit_width":4
        },
        "key": "Build Date",
        "arrt_index": 6,
    },

    "fpga2": {
        "key": "FPGA2",
        "parent": "fpga",
        "arrt_index": 2,
    },
    "fpga2_model": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset":0x10,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "00000050": "XC7A50T-2FGG484I",
            "00000100": "XC7A100T-2FGG484I"
        },
        "key": "Device Model",
        "arrt_index": 2,
    },
    "fpga2_vender": {
        "parent": "fpga2",
        "config": "XILINX",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "fpga2_desc": {
        "key": "Description",
        "parent": "fpga2",
        "config": "UPORT_FPGA",
        "arrt_index": 3,
    },
    "fpga2_hw_version": {
        "parent": "fpga2",
        "config": "NA",
        "key": "Hardware Version",
        "arrt_index": 4,
    },
    "fpga2_fw_version": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset":0,
            "len":4,
            "bit_width":4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga2_date": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset":4,
            "len":4,
            "bit_width":4
        },
        "key": "Build Date",
        "arrt_index": 6,
    },

    "fpga3": {
        "key": "FPGA3",
        "parent": "fpga",
        "arrt_index": 3,
    },
    "fpga3_model": {
        "parent": "fpga3",
        "devfile": {
            "loc": "/dev/fpga2",
            "offset":0x10,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "00000050": "XC7A50T-2FGG484I",
            "00000100": "XC7A100T-2FGG484I"
        },
        "key": "Device Model",
        "arrt_index": 3,
    },
    "fpga3_vender": {
        "parent": "fpga3",
        "config": "XILINX",
        "key": "Vendor",
        "arrt_index": 3,
    },
    "fpga3_desc": {
        "key": "Description",
        "parent": "fpga3",
        "config": "DPORT_FPGA",
        "arrt_index": 3,
    },
    "fpga3_hw_version": {
        "parent": "fpga3",
        "config": "NA",
        "key": "Hardware Version",
        "arrt_index": 4,
    },
    "fpga3_fw_version": {
        "parent": "fpga3",
        "devfile": {
            "loc": "/dev/fpga2",
            "offset":0,
            "len":4,
            "bit_width":4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga3_date": {
        "parent": "fpga3",
        "devfile": {
            "loc": "/dev/fpga2",
            "offset":4,
            "len":4,
            "bit_width":4
        },
        "key": "Build Date",
        "arrt_index": 6,
    },
    "others": {
        "key": "OTHERS",
    },
    "53134": {
        "parent": "others",
        "key": "CPU-BMC-SWITCH",
        "arrt_index": 1,
    },
    "53134_model": {
        "parent": "53134",
        "config": "BCM53134O",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "53134_vendor": {
        "parent": "53134",
        "config": "Broadcom",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "53134_hw_version": {
        "parent": "53134",
        "key": "Hardware Version",
        "func": {
            "funcname": "get_bcm5387_version",
            "params": {
                "before": [
                    # enable tocm_jtag_en
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                    # OE high
                    {"gettype": "cmd", "cmd": "echo 596 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio596/direction"},
                    # SEL1 high
                    {"gettype": "cmd", "cmd": "echo 597 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio597/direction"},
                    # enable 53134 update
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x01},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x05},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0xb2},
                    # {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=7  mosi=5 miso=6 cs=8 bus=0 gpio_chip_name=AMDI0030:00"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46 eeprom_name=gt93c56a"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio597/value"},
                    {"gettype": "cmd", "cmd": "echo 597 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio596/value"},
                    {"gettype": "cmd", "cmd": "echo 596 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    # {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    # disable tocm_jtag_en
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
        },
        "arrt_index": 3,
    },

}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "sffs": {
        "present": {"path": ["/sys/s3ip/transceiver/*/present"], "ABSENT": 0},
        "nochangedmsgflag": 0,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 1,
        "alias": {
            "sff1": "Ethernet1",
            "sff2": "Ethernet2",
            "sff3": "Ethernet3",
            "sff4": "Ethernet4",
            "sff5": "Ethernet5",
            "sff6": "Ethernet6",
            "sff7": "Ethernet7",
            "sff8": "Ethernet8",
            "sff9": "Ethernet9",
            "sff10": "Ethernet10",
            "sff11": "Ethernet11",
            "sff12": "Ethernet12",
            "sff13": "Ethernet13",
            "sff14": "Ethernet14",
            "sff15": "Ethernet15",
            "sff16": "Ethernet16",
            "sff17": "Ethernet17",
            "sff18": "Ethernet18",
            "sff19": "Ethernet19",
            "sff20": "Ethernet20",
            "sff21": "Ethernet21",
            "sff22": "Ethernet22",
            "sff23": "Ethernet23",
            "sff24": "Ethernet24",
            "sff25": "Ethernet25",
            "sff26": "Ethernet26",
            "sff27": "Ethernet27",
            "sff28": "Ethernet28",
            "sff29": "Ethernet29",
            "sff30": "Ethernet30",
            "sff31": "Ethernet31",
            "sff32": "Ethernet32",
            "sff33": "Ethernet33",
            "sff34": "Ethernet34",
            "sff35": "Ethernet35",
            "sff36": "Ethernet36",
            "sff37": "Ethernet37",
            "sff38": "Ethernet38",
            "sff39": "Ethernet39",
            "sff40": "Ethernet40",
            "sff41": "Ethernet41",
            "sff42": "Ethernet42",
            "sff43": "Ethernet43",
            "sff44": "Ethernet44",
            "sff45": "Ethernet45",
            "sff46": "Ethernet46",
            "sff47": "Ethernet47",
            "sff48": "Ethernet48",
            "sff49": "Ethernet49",
            "sff50": "Ethernet50",
            "sff51": "Ethernet51",
            "sff52": "Ethernet52",
            "sff53": "Ethernet53",
            "sff54": "Ethernet54",
            "sff55": "Ethernet55",
            "sff56": "Ethernet56",
            "sff57": "Ethernet57",
            "sff58": "Ethernet58",
            "sff59": "Ethernet59",
            "sff60": "Ethernet60",
            "sff61": "Ethernet61",
            "sff62": "Ethernet62",
            "sff63": "Ethernet63",
            "sff64": "Ethernet64",
            "sff65": "Ethernet65",
            "sff66": "Ethernet66",
            "sff67": "Ethernet67",
            "sff68": "Ethernet68",
            "sff69": "Ethernet69",
            "sff70": "Ethernet70",
            "sff71": "Ethernet71",
            "sff72": "Ethernet72",
            "sff73": "Ethernet73",
            "sff74": "Ethernet74",
            "sff75": "Ethernet75",
            "sff76": "Ethernet76",
            "sff77": "Ethernet77",
            "sff78": "Ethernet78",
            "sff79": "Ethernet79",
            "sff80": "Ethernet80",
            "sff81": "Ethernet81",
            "sff82": "Ethernet82",
            "sff83": "Ethernet83",
            "sff84": "Ethernet84",
            "sff85": "Ethernet85",
            "sff86": "Ethernet86",
            "sff87": "Ethernet87",
            "sff88": "Ethernet88",
            "sff89": "Ethernet89",
            "sff90": "Ethernet90",
            "sff91": "Ethernet91",
            "sff92": "Ethernet92",
            "sff93": "Ethernet93",
            "sff94": "Ethernet94",
            "sff95": "Ethernet95",
            "sff96": "Ethernet96",
            "sff97": "Ethernet97",
            "sff98": "Ethernet98",
            "sff99": "Ethernet99",
            "sff100": "Ethernet100",
            "sff101": "Ethernet101",
            "sff102": "Ethernet102",
            "sff103": "Ethernet103",
            "sff104": "Ethernet104",
            "sff105": "Ethernet105",
            "sff106": "Ethernet106",
            "sff107": "Ethernet107",
            "sff108": "Ethernet108",
            "sff109": "Ethernet109",
            "sff110": "Ethernet110",
            "sff111": "Ethernet111",
            "sff112": "Ethernet112",
            "sff113": "Ethernet113",
            "sff114": "Ethernet114",
            "sff115": "Ethernet115",
            "sff116": "Ethernet116",
            "sff117": "Ethernet117",
            "sff118": "Ethernet118",
            "sff119": "Ethernet119",
            "sff120": "Ethernet120",
            "sff121": "Ethernet121",
            "sff122": "Ethernet122",
            "sff123": "Ethernet123",
            "sff124": "Ethernet124",
            "sff125": "Ethernet125",
            "sff126": "Ethernet126",
            "sff127": "Ethernet127",
            "sff128": "Ethernet128",
            "sff129": "Ethernet129"
        }
    },
    "fans": {
        "present": {"path": ["/sys/s3ip/fan/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/s3ip/fan/%s/status", 'okval': 1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "fan1": "FAN1",
            "fan2": "FAN2",
            "fan3": "FAN3",
            "fan4": "FAN4",
            "fan5": "FAN5",
            "fan6": "FAN6",
            "fan7": "FAN7",
            "fan8": "FAN8"
        }
    },
    "psus": {
        "present": {"path": ["/sys/s3ip/psu/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/s3ip/psu/%s/out_status", "okval":1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "psu1": "PSU1",
            "psu2": "PSU2",
            "psu3": "PSU3",
            "psu4": "PSU4"
        }
    }
}

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "cold_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x09},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x05},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":0x06},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "cpu_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":[0x03, 0x04]},
            "record": [
                {"record_type": "file", "mode":"cover", "log":"CPU reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "CPU reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_powerdown",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x1d, "read_len":1, "okval":[0x02, 0x07, 0x0a]},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "otp_switch_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_switch_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: ASIC, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: ASIC, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "cmd", "cmd": "rm -rf /etc/.otp_switch_reboot_flag"},
            ]
        },
        {
            "name": "otp_other_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_other_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: Other, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: Other, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "cmd", "cmd": "rm -rf /etc/.otp_other_reboot_flag"},
            ]
        },
    ],
    "other_reboot_cause_record": [
        {"record_type": "file", "mode": "cover", "log": "Other, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
        {"record_type": "file", "mode": "add", "log": "Other, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
    ],
}

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "MAC_CORE_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8a,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs1", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/136-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8c : 0.794245,
            0x8a : 0.851800,
            0x88 : 0.864700,
            0x86 : 0.877600,
            0x84 : 0.83898,
            0x82 : 0.851795,
            0x80 : 0.864745,
        },
    },
    {
        "name": "MAC_PT0_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs2", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/107-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT1_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs3", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/107-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT2_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs4", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/108-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT3_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs5", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/108-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT4_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs6", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/109-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT5_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs7", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/109-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT6_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs8", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/110-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
    {
        "name": "MAC_PT7_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/131-0044/hwmon/hwmon*/avs9", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/110-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92 : 0.7000,
            0x90 : 0.7125,
            0x8e : 0.7250,
            0x8c : 0.7375,
            0x8a : 0.7500,
            0x88 : 0.7625,
            0x86 : 0.7750,
            0x84 : 0.7875,
            0x82 : 0.8000,
            0x80 : 0.8125,
        },
    },
]

DRIVERLISTS = [
    {"name": "wb_ixgbe", "delay": 0, "removable": 0},
    {"name": "ice", "delay": 0, "removable": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "wb_i2c_designware_core", "delay": 0},
    {"name": "wb_i2c_designware_platform sda_fall_ns=320 scl_fall_ns=1006 bus_freq_hz=100000", "delay": 0},
    # {"name": "i2c_algo_bit", "delay": 0},
    # {"name": "i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    # {"name": "wb_i2c_gpio_device gpio_sda=116 gpio_scl=115 gpio_chip_name=AMDI0030:00 bus_num=2", "delay": 0},
    {"name": "i2c_piix4", "delay": 0.1},
    {"name": "platform_common dfd_my_type=0x414e", "delay": 0},
    {"name": "wb_logic_dev_common", "delay":0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_spi_gpio", "delay": 0},
    {"name": "wb_spi_gpio_device_cpld", "delay": 0},
    #{"name": "wb_io_dev", "delay": 0},
    #{"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_spi_dev_device", "delay": 0.1},
    # {"name": "wb_indirect_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    # {"name": "wb_indirect_dev_device", "delay": 0},
    {"name": "wb_cpld_i2c_bus_device", "delay": 0.1},
    {"name": "wb_cpld_pca954x_device", "delay": 0},
    {
        "init": [
            # disable mdio to avoid 81349 mdio error
            {"path":"/dev/cpld1", "offset":0x41, "value":0x0, "gettype":"devfile"},
        ],
        "name": "mdio_bitbang", "delay": 0
    },
    {"name": "mdio_gpio", "delay": 0},
    {"name": "wb_mdio_gpio_device gpio_mdc=131 gpio_mdio=132 gpio_chip_name=AMDI0030:00","delay": 0.1},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    #{"name": "wb_eeprom_93xx46", "delay": 0},
    {"name": "lm75", "delay": 0},
    {"name": "tmp401", "delay": 0},
    {"name": "ads7828", "delay": 0},
    {"name": "wb_rc32312", "delay": 0},
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "ucd9000", "delay": 0},
    {"name": "wb_ucd9081", "delay": 0}, 
    {"name": "xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay":0},
    {"name": "isl68137", "delay": 0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "wb_vr_common", "delay": 0},
    {"name": "wb_vr_common_dev_device", "delay": 0},
    #{"name": "firmware_driver_cpld", "delay": 0},
    #{"name": "firmware_driver_ispvme", "delay": 0},
    #{"name": "firmware_driver_sysfs", "delay": 0},
    #{"name": "wb_firmware_upgrade_device", "delay": 0},
    {"name": "hw_test", "delay": 0},
    {"name": "s3ip_sysfs", "delay": 0},
    {"name": "wb_switch_driver", "delay": 0},
    {"name": "syseeprom_device_driver", "delay": 0},
    {"name": "fan_device_driver", "delay": 0},
    {"name": "cpld_device_driver", "delay": 0},
    {"name": "sysled_device_driver", "delay": 0},
    {"name": "psu_device_driver", "delay": 0},
    {"name": "transceiver_device_driver", "delay": 0},
    {"name": "temp_sensor_device_driver", "delay": 0},
    {"name": "vol_sensor_device_driver", "delay": 0},
    #{"name": "curr_sensor_device_driver", "delay": 0},
    {"name": "fpga_device_driver", "delay": 0},
    {"name": "wb_mdio_dev", "delay": 0},
    {"name": "wb_switch_dev", "delay": 0},
    {"name": "wb_switch_dev_device", "delay": 0},
    
]

DEVICE = [
    # CPU & MGMT board
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "24c02", "bus": 1, "loc": 0x57},
    {"name": "24c02", "bus": 51, "loc": 0x57},
    {"name": "ucd90160", "bus": 53, "loc": 0x5b},
    {"name": "lm75", "bus": 54, "loc": 0x4a}, # TMP1075DR
    {"name": "ucd90160", "bus": 54, "loc": 0x5f},
    {"name": "lm75", "bus": 55, "loc": 0x4b}, # CT75
    {"name": "lm75", "bus": 56, "loc": 0x4b}, # CT75
    {"name": "ucd90160", "bus": 57, "loc": 0x68},
    {"name": "ads7828", "bus": 60, "loc": 0x48},
    # psu
    {"name": "24c02",   "bus": 67, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 67, "loc": 0x58},
    {"name": "24c02",   "bus": 68, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 68, "loc": 0x58},
    {"name": "24c02",   "bus": 69, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 69, "loc": 0x58},
    {"name": "24c02",   "bus": 70, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 70, "loc": 0x58},
    # fcb
    {"name": "lm75", "bus": 76, "loc": 0x4b}, # CT75
    {"name": "24c64", "bus": 77, "loc": 0x50},
    {"name": "24c64", "bus": 78, "loc": 0x50},
    {"name": "24c64", "bus": 79, "loc": 0x50},
    {"name": "24c64", "bus": 80, "loc": 0x50},
    {"name": "lm75", "bus": 84, "loc": 0x4b}, # CT75
    {"name": "24c64", "bus": 85, "loc": 0x50},
    {"name": "24c64", "bus": 86, "loc": 0x50},
    {"name": "24c64", "bus": 87, "loc": 0x50},
    {"name": "24c64", "bus": 88, "loc": 0x50},
    # uport
    {"name": "24c02", "bus": 91, "loc": 0x57},
    {"name": "ucd90160", "bus": 92, "loc": 0x5b},
    {"name": "lm75", "bus": 93, "loc": 0x4b}, # CT75
    # dport
    {"name": "24c02", "bus": 99, "loc": 0x57},
    {"name": "ucd90160", "bus": 100, "loc": 0x5b},
    {"name": "lm75", "bus": 101, "loc": 0x4b}, # CT75
    # mac board
    {"name": "lm75", "bus": 123, "loc": 0x4b}, # CT75
    {"name": "lm75", "bus": 124, "loc": 0x4f}, # CT75
    {"name": "ucd90160", "bus": 128, "loc": 0x5b},
    {"name": "ucd90160", "bus": 129, "loc": 0x5b},
    {"name": "ucd90160", "bus": 130, "loc": 0x5b},
    {"name": "wb_mac_bsc_th6", "bus": 131, "loc": 0x44},
    {"name": "wb_rc32312", "bus": 134, "loc": 0x09},
    {"name": "wb_rc32312", "bus": 135, "loc": 0x09},
]

OPTOE = [
    {"name": "optoe3", "startbus": 201, "endbus": 328},
    {"name": "optoe1", "startbus": 329, "endbus": 329},
    {"name": "optoe2", "startbus": 330, "endbus": 330},
]

INIT_PARAM_PRE = [
    # MAC_CORE min and max value
    # {"loc": "136-0060/hwmon/hwmon*/avs0_vout_min", "value": "747100", "gettype": "sysfs"},
    # {"loc": "136-0060/hwmon/hwmon*/avs0_vout_max", "value": "828900", "gettype": "sysfs"},
    # {"loc": "136-0060/hwmon/hwmon*/avs0_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "136-0060/hwmon/hwmon*/avs0_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "107-0060/hwmon/hwmon*/avs0_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "107-0060/hwmon/hwmon*/avs0_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "108-0060/hwmon/hwmon*/avs0_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "108-0060/hwmon/hwmon*/avs0_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "109-0060/hwmon/hwmon*/avs0_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "109-0060/hwmon/hwmon*/avs0_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "110-0060/hwmon/hwmon*/avs0_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "110-0060/hwmon/hwmon*/avs0_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "107-0060/hwmon/hwmon*/avs1_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "107-0060/hwmon/hwmon*/avs1_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "108-0060/hwmon/hwmon*/avs1_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "108-0060/hwmon/hwmon*/avs1_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "109-0060/hwmon/hwmon*/avs1_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "109-0060/hwmon/hwmon*/avs1_vout_max", "value": "900000", "gettype": "sysfs"},
    # {"loc": "110-0060/hwmon/hwmon*/avs1_vout_min", "value": "600000", "gettype": "sysfs"},
    # {"loc": "110-0060/hwmon/hwmon*/avs1_vout_max", "value": "900000", "gettype": "sysfs"},
]

INIT_COMMAND = [
    # UPORT OSFP_VDD3V3 EN
    "dfd_debug sysfs_data_wr /dev/cpld5 0x23 0xdd",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x24 0xdb",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x25 0xdb",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x26 0xd9",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x27 0xd9",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x28 0xd7",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x29 0xd7",
    "dfd_debug sysfs_data_wr /dev/cpld5 0x2a 0xd5",

    # MAC OSFP_VDD3V3 EN
    "dfd_debug sysfs_data_wr /dev/cpld4 0x30 0xcf",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x31 0xcf",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x32 0xcd",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x33 0xcd",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x34 0xcb",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x35 0xcb",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x36 0xc9",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x37 0xc9",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x38 0xc7",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x39 0xc7",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3a 0xc5",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3b 0xc5",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3c 0xc3",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3d 0xc3",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3e 0xc1",
    "dfd_debug sysfs_data_wr /dev/cpld4 0x3f 0xc1",
    # DPORT OSFP_VDD3V3 EN
    "dfd_debug sysfs_data_wr /dev/cpld6 0x23 0xdd",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x24 0xdb",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x25 0xdb",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x26 0xd9",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x27 0xd9",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x28 0xd7",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x29 0xd7",
    "dfd_debug sysfs_data_wr /dev/cpld6 0x2a 0xd5",
    # UPORT efuse en
    "dfd_debug sysfs_data_wr /dev/fpga1 0x58 0xff 0xff 0xff 0xff",
    # MAC efuse EN
    "dfd_debug sysfs_data_wr /dev/cpld9 0x20 0xDF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x21 0xDF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x22 0xDD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x23 0xDD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x24 0xDB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x25 0xDB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x26 0xD9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x27 0xD9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x28 0xD7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x29 0xD7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2A 0xD5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2B 0xD5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2C 0xD3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2D 0xD3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2E 0xD1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x2F 0xD1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x30 0xCF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x31 0xCF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x32 0xCD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x33 0xCD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x34 0xCB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x35 0xCB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x36 0xC9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x37 0xC9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x38 0xC7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x39 0xC7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3A 0xC5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3B 0xC5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3C 0xC3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3D 0xC3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3E 0xC1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x3F 0xC1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x40 0xBF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x41 0xBF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x42 0xBD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x43 0xBD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x44 0xBB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x45 0xBB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x46 0xB9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x47 0xB9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x48 0xB7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x49 0xB7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4A 0xB5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4B 0xB5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4C 0xB3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4D 0xB3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4E 0xB1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x4F 0xB1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x50 0xAF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x51 0xAF",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x52 0xAD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x53 0xAD",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x54 0xAB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x55 0xAB",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x56 0xA9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x57 0xA9",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x58 0xA7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x59 0xA7",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5A 0xA5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5B 0xA5",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5C 0xA3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5D 0xA3",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5E 0xA1",
    "dfd_debug sysfs_data_wr /dev/cpld9 0x5F 0xA1",
    # DOORT efuse en
    "dfd_debug sysfs_data_wr /dev/fpga2 0x58 0xff 0xff 0xff 0xff",
    # Enable port129 power
    "dfd_debug sysfs_data_wr /dev/cpld1 0x5d 0xa3",
    # saphy bcm81394 mdio access enable
    "dfd_debug sysfs_data_wr /dev/cpld1 0x41 0x03",
    # write CPU_READY to release sys led control
    "dfd_debug sysfs_data_wr /dev/cpld1 0x8f 0x01",
]

UPGRADE_SUMMARY = {
    "devtype": 0x414e,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain2": {
                "name": "MGMT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain3": {
                "name": "MAC_CPLD_AB",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain4": {
                "name": "MAC_CPLD_C",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain5": {
                "name": "UPORT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain6": {
                "name": "DPORT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain7": {
                "name": "UFAN_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain8": {
                "name": "DFAN_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain9": {
                "name": "MAC_CPLD_D",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "MAC_FPGA",
                "is_support_warm_upg": 0,
            },
            "chain2": {
                "name": "MAC_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
            },
            "chain3": {
                "name": "UPORT_FPGA",
                "is_support_warm_upg": 0,
            },
            "chain4": {
                "name": "UPORT_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
            },
            "chain5": {
                "name": "DPORT_FPGA",
                "is_support_warm_upg": 0,
            },
            "chain6": {
                "name": "DPORT_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
            },
        },

        "SYSFS": {
            "chain8": {
                "name": "BCM53134",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    # {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=7  mosi=5 miso=6 cs=8 bus=0 gpio_chip_name=AMDI0030:00", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46 eeprom_name=gt93c56a", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    # {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },

        #"BASH": {
        #    "chain1": {
        #        "name": "SATA",
        #        "upgrade_way": UPGRADE_BY_HDPARM,
        #        "dev_name": "sda",
        #    },
        #},

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/mac_fpga_test_header.bin", "display_name": "MAC_FPGA"},
                {"chain": 3, "file": "/etc/.upgrade_test/uport_fpga_test_header.bin", "display_name": "UPORT_FPGA"},
                {"chain": 5, "file": "/etc/.upgrade_test/dport_fpga_test_header.bin", "display_name": "DPORT_FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpu_cpld_test_header.vme", "display_name": "CPU_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/mgmt_cpld_test_header.vme", "display_name": "MGMT_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/mac_cpldab_test_header.vme", "display_name": "MAC_CPLD_AB"},
                {"chain": 4, "file": "/etc/.upgrade_test/mac_cpldc_test_header.vme", "display_name": "MAC_CPLD_C"},
                {"chain": 5, "file": "/etc/.upgrade_test/uport_cpld_test_header.vme", "display_name": "UPORT_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/dport_cpld_test_header.vme", "display_name": "DPORT_CPLD"},
                {"chain": 7, "file": "/etc/.upgrade_test/ufan_cpld_test_header.vme", "display_name": "UFAN_CPLD"},
                {"chain": 8, "file": "/etc/.upgrade_test/dfan_cpld_test_header.vme", "display_name": "DFAN_CPLD"},
                {"chain": 9, "file": "/etc/.upgrade_test/mac_cpldd_test_header.vme", "display_name": "MAC_CPLD_D"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/77-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/85-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/78-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/86-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/79-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/87-0050/eeprom"},
        {"name": "fan7", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/80-0050/eeprom"},
        {"name": "fan8", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/88-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/67-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/68-0050/eeprom"},
        {"name": "psu3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/69-0050/eeprom"},
        {"name": "psu4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/70-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
    "mgmt": [
        {"name": "MGMT card", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/51-0057/eeprom"},
    ],
    "uport": [
        {"name": "Uport board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/91-0057/eeprom"},
    ],
    "dport": [
        {"name": "Dport board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/99-0057/eeprom"},
    ],
    "mac": [
        {"name": "MAC board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/1-0057/eeprom"},
    ],
}

AIR_FLOW_CONF = {
}

# DFX_XDPE_MONITOR_INFO = {
#     "device_list": [
#         {
#             "name": "wb_isl69260",
#             "loc": "70-0070",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_isl69260",
#             "loc": "70-006e",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_isl69260",
#             "loc": "70-005e",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_isl69260",
#             "loc": "70-0068",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe1a2g5b",
#             "loc": "85-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "86-0062",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "87-0062",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "88-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "89-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "90-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "91-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "92-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "93-0062",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "94-0062",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "95-0060",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "96-0074",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "97-0074",
#             "page": [0, 1],
#         },
#         {
#             "name": "wb_xdpe12284c",
#             "loc": "98-0074",
#             "page": [0, 1],
#         },
#     ]
# }

MGMT_LED_CONFIG = {
    "interval": 3,
    "mgmt_led_list": [
        {
            "name": "sfp_phy",
            "link_path": "/sys/logic_dev/sfp_phy/link_status",
            "led_path": "/sys/s3ip/sysled/lan_led_status",
        },
    ]
}
