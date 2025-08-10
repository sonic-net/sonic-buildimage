#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "fancontrol": 0,
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "slot_monitor": 1,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "reboot_cause": 1,
}


DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "io", "io_addr": 0xb27, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 23, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "wb_24c02", "bus": 23, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "io", "io_addr": 0xb28, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 25, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "wb_24c02", "bus": 25, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu3",
            "present": {"gettype": "io", "io_addr": 0xb29, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu3pmbus", "name": "wb_fsp1200", "bus": 24, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu3frue2", "name": "wb_24c02", "bus": 24, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu4",
            "present": {"gettype": "io", "io_addr": 0xb2a, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu4pmbus", "name": "wb_fsp1200", "bus": 26, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu4frue2", "name": "wb_24c02", "bus": 26, "loc": 0x50, "attr": "eeprom"},
             ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 14, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "wb_24c02", "bus": 63, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 13, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "wb_24c02", "bus": 55, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 14, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "wb_24c02", "bus": 64, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 13, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "wb_24c02", "bus": 56, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "i2c", "bus": 14, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "wb_24c02", "bus": 65, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "i2c", "bus": 13, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "wb_24c02", "bus": 57, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "slots": [
        {
            "name": "slot1",
            "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "slot1frue2", "name": "wb_24c02", "bus": 3, "loc": 0x56, "attr": "eeprom"},
                {"id": "slot1tlve2", "name": "wb_24c02", "bus": 3, "loc": 0x57, "attr": "eeprom"},
                {"id": "slot1lm75a1", "name": "wb_lm75", "bus": 32, "loc": 0x48, "attr": "hwmon"},
                {"id": "slot1lm75a2", "name": "wb_lm75", "bus": 32, "loc": 0x49, "attr": "hwmon"},
                {"id": "slot1lm75a3", "name": "wb_lm75", "bus": 32, "loc": 0x4D, "attr": "hwmon"},
            ],
        },
        {
            "name": "slot2",
            "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "slot2frue2", "name": "wb_24c02", "bus": 4, "loc": 0x56, "attr": "eeprom"},
                {"id": "slot2tlve2", "name": "wb_24c02", "bus": 4, "loc": 0x57, "attr": "eeprom"},
                {"id": "slot2lm75a1", "name": "wb_lm75", "bus": 48, "loc": 0x48, "attr": "hwmon"},
                {"id": "slot2lm75a2", "name": "wb_lm75", "bus": 48, "loc": 0x49, "attr": "hwmon"},
                {"id": "slot2lm75a3", "name": "wb_lm75", "bus": 48, "loc": 0x4D, "attr": "hwmon"},
            ],
        },
        {
            "name": "slot3",
            "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 6, "okval": 0},
            "device": [
                {"id": "slot3frue2", "name": "wb_24c02", "bus": 5, "loc": 0x56, "attr": "eeprom"},
                {"id": "slot3tlve2", "name": "wb_24c02", "bus": 5, "loc": 0x57, "attr": "eeprom"},
                {"id": "slot3lm75a1", "name": "wb_lm75", "bus": 40, "loc": 0x48, "attr": "hwmon"},
                {"id": "slot3lm75a2", "name": "wb_lm75", "bus": 40, "loc": 0x49, "attr": "hwmon"},
                {"id": "slot3lm75a3", "name": "wb_lm75", "bus": 40, "loc": 0x4D, "attr": "hwmon"},
            ],
        },
        {
            "name": "slot4",
            "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 7, "okval": 0},
            "device": [
                {"id": "slot4frue2", "name": "wb_24c02", "bus": 6, "loc": 0x56, "attr": "eeprom"},
                {"id": "slot4tlve2", "name": "wb_24c02", "bus": 6, "loc": 0x57, "attr": "eeprom"},
                {"id": "slot4lm75a1", "name": "wb_lm75", "bus": 16, "loc": 0x48, "attr": "hwmon"},
                {"id": "slot4lm75a2", "name": "wb_lm75", "bus": 16, "loc": 0x49, "attr": "hwmon"},
                {"id": "slot4lm75a3", "name": "wb_lm75", "bus": 16, "loc": 0x4D, "attr": "hwmon"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "wb_24c02", "bus": 1, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "wb_lm75", "bus": 28, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_2", "name": "wb_lm75", "bus": 29, "loc": 0x4f, "attr": "hwmon"},
                {"id": "lm75_3", "name": "wb_lm75", "bus": 60, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_4", "name": "wb_lm75", "bus": 60, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_5", "name": "wb_lm75", "bus": 68, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_6", "name": "wb_lm75", "bus": 68, "loc": 0x49, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_th3", "bus": 27, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name": "tps53622",
            "device": [
                {"id": "tps53622_1", "name": "wb_tps53622", "bus": 10, "loc": 0x60, "attr": "hwmon"},
                {"id": "tps53622_2", "name": "wb_tps53622", "bus": 10, "loc": 0x6c, "attr": "hwmon"},
            ],
        },
        {
            "name": "tmp411",
            "device": [
                {"id": "tmp411_1", "name": "wb_tmp411", "bus": 28, "loc": 0x4c, "attr": "hwmon"},
                {"id": "tmp411_2", "name": "wb_tmp411", "bus": 29, "loc": 0x4c, "attr": "hwmon"},
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
        "next": "slot"
    },
    "ssd_model": {
        "parent": "ssd",
        "key": "Device Model",
        "cmd": "smartctl -i /dev/sda |grep \"Device Model\"",
        "pattern": r".*Device Model",
        "separator": ":",
        "arrt_index": 1,
    },
    "ssd_fw": {
        "parent": "ssd",
        "key": "Firmware Version",
        "cmd": "smartctl -i /dev/sda |grep \"Firmware Version\"",
        "pattern": r".*Firmware Version",
        "separator": ":",
        "arrt_index": 2,
    },
    "ssd_user_cap": {
        "parent": "ssd",
        "key": "User Capacity",
        "cmd": "smartctl -i /dev/sda |grep \"User Capacity\"",
        "pattern": r".*User Capacity",
        "separator": ":",
        "arrt_index": 3,
    },

    "slot": {
        "key": "SLOT",
        "next": "cpld"
    },
    "slot1": {
        "key": "SLOT1",
        "parent": "slot",
        "arrt_index": 1,
    },
    "slot1_hw_version": {
        "key": "Hardware Version",
        "parent": "slot1",
        "extra": {
            "funcname": "checkSlot",
            "id": "slot1",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "slot2": {
        "key": "SLOT2",
        "parent": "slot",
        "arrt_index": 2,
    },
    "slot2_hw_version": {
        "key": "Hardware Version",
        "parent": "slot2",
        "extra": {
            "funcname": "checkSlot",
            "id": "slot2",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "slot3": {
        "key": "SLOT3",
        "parent": "slot",
        "arrt_index": 3,
    },
    "slot3_hw_version": {
        "key": "Hardware Version",
        "parent": "slot3",
        "extra": {
            "funcname": "checkSlot",
            "id": "slot3",
            "key": "hw_version"
        },
        "arrt_index": 1,
    },
    "slot4": {
        "key": "SLOT4",
        "parent": "slot",
        "arrt_index": 4,
    },
    "slot4_hw_version": {
        "key": "Hardware Version",
        "parent": "slot4",
        "extra": {
            "funcname": "checkSlot",
            "id": "slot4",
            "key": "hw_version"
        },
        "arrt_index": 1,
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
            "loc": "/dev/port",
            "offset": 0x700,
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "MAC_CPLD_A",
        "arrt_index": 3,
    },
    "cpld2_version": {
        "key": "Firmware Version",
        "parent": "cpld2",
        "reg": {
            "loc": "/dev/port",
            "offset": 0xb00,
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "MAC_CPLD_B",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "reg": {
            "loc": "/dev/port",
            "offset": 0x900,
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
        "config": "LCMXO2-2000HC-4MG132C",
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
        "config": "FAN_CPLD_A",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "14",
            "loc": "0x0d",
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
        "config": "LCMXO2-2000HC-4MG132C",
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
        "config": "FAN_CPLD_B",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "13",
            "loc": "0x0d",
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "LC1_CPLD_2",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "i2c": {
            "bus": "3",
            "loc": "0x31",
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "LC1_CPLD_1",
        "arrt_index": 3,
    },
    "cpld7_version": {
        "key": "Firmware Version",
        "parent": "cpld7",
        "i2c": {
            "bus": "3",
            "loc": "0x30",
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
        "config": "LC2_CPLD_2",
        "arrt_index": 3,
    },
    "cpld8_version": {
        "key": "Firmware Version",
        "parent": "cpld8",
        "i2c": {
            "bus": "4",
            "loc": "0x31",
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
        "config": "LC2_CPLD_1",
        "arrt_index": 3,
    },
    "cpld9_version": {
        "key": "Firmware Version",
        "parent": "cpld9",
        "i2c": {
            "bus": "4",
            "loc": "0x30",
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
        "config": "LC3_CPLD_2",
        "arrt_index": 3,
    },
    "cpld10_version": {
        "key": "Firmware Version",
        "parent": "cpld10",
        "i2c": {
            "bus": "5",
            "loc": "0x31",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld11": {
        "key": "CPLD11",
        "parent": "cpld",
        "arrt_index": 11,
    },
    "cpld11_model": {
        "key": "Device Model",
        "parent": "cpld11",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld11_vender": {
        "key": "Vendor",
        "parent": "cpld11",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld11_desc": {
        "key": "Description",
        "parent": "cpld11",
        "config": "LC3_CPLD_1",
        "arrt_index": 3,
    },
    "cpld11_version": {
        "key": "Firmware Version",
        "parent": "cpld11",
        "i2c": {
            "bus": "5",
            "loc": "0x30",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld12": {
        "key": "CPLD12",
        "parent": "cpld",
        "arrt_index": 12,
    },
    "cpld12_model": {
        "key": "Device Model",
        "parent": "cpld12",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld12_vender": {
        "key": "Vendor",
        "parent": "cpld12",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld12_desc": {
        "key": "Description",
        "parent": "cpld12",
        "config": "LC4_CPLD_2",
        "arrt_index": 3,
    },
    "cpld12_version": {
        "key": "Firmware Version",
        "parent": "cpld12",
        "i2c": {
            "bus": "6",
            "loc": "0x31",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },

    "cpld13": {
        "key": "CPLD13",
        "parent": "cpld",
        "arrt_index": 13,
    },
    "cpld13_model": {
        "key": "Device Model",
        "parent": "cpld13",
        "config": "LCMXO3LF-2100C-5BG256C",
        "arrt_index": 1,
    },
    "cpld13_vender": {
        "key": "Vendor",
        "parent": "cpld13",
        "config": "LATTICE",
        "arrt_index": 2,
    },
    "cpld13_desc": {
        "key": "Description",
        "parent": "cpld13",
        "config": "LC4_CPLD_1",
        "arrt_index": 3,
    },
    "cpld13_version": {
        "key": "Firmware Version",
        "parent": "cpld13",
        "i2c": {
            "bus": "6",
            "loc": "0x30",
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

    "i210": {
        "key": "NIC",
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
}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "sffs": {
        "present": {"path": ["/sys/wb_plat/sff/*/present"], "ABSENT":0},
        "nochangedmsgflag": 0,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 1,
        "alias": {
            "eth1": "Eth100GE1-1",
            "eth2": "Eth100GE1-2",
            "eth3": "Eth100GE1-3",
            "eth4": "Eth100GE1-4",
            "eth5": "Eth100GE1-5",
            "eth6": "Eth100GE1-6",
            "eth7": "Eth100GE1-7",
            "eth8": "Eth100GE1-8",
            "eth9": "Eth100GE1-9",
            "eth10": "Eth100GE1-10",
            "eth11": "Eth100GE1-11",
            "eth12": "Eth100GE1-12",
            "eth13": "Eth100GE1-13",
            "eth14": "Eth100GE1-14",
            "eth15": "Eth100GE1-15",
            "eth16": "Eth100GE1-16",
            "eth17": "Eth100GE1-17",
            "eth18": "Eth100GE1-18",
            "eth19": "Eth100GE1-19",
            "eth20": "Eth100GE1-20",
            "eth21": "Eth100GE1-21",
            "eth22": "Eth100GE1-22",
            "eth23": "Eth100GE1-23",
            "eth24": "Eth100GE1-24",
            "eth25": "Eth100GE1-25",
            "eth26": "Eth100GE1-26",
            "eth27": "Eth100GE1-27",
            "eth28": "Eth100GE1-28",
            "eth29": "Eth100GE1-29",
            "eth30": "Eth100GE1-30",
            "eth31": "Eth100GE1-31",
            "eth32": "Eth100GE1-32",
            "eth33": "Eth100GE2-1",
            "eth34": "Eth100GE2-2",
            "eth35": "Eth100GE2-3",
            "eth36": "Eth100GE2-4",
            "eth37": "Eth100GE2-5",
            "eth38": "Eth100GE2-6",
            "eth39": "Eth100GE2-7",
            "eth40": "Eth100GE2-8",
            "eth41": "Eth100GE2-9",
            "eth42": "Eth100GE2-10",
            "eth43": "Eth100GE2-11",
            "eth44": "Eth100GE2-12",
            "eth45": "Eth100GE2-13",
            "eth46": "Eth100GE2-14",
            "eth47": "Eth100GE2-15",
            "eth48": "Eth100GE2-16",
            "eth49": "Eth100GE2-17",
            "eth50": "Eth100GE2-18",
            "eth51": "Eth100GE2-19",
            "eth52": "Eth100GE2-20",
            "eth53": "Eth100GE2-21",
            "eth54": "Eth100GE2-22",
            "eth55": "Eth100GE2-23",
            "eth56": "Eth100GE2-24",
            "eth57": "Eth100GE2-25",
            "eth58": "Eth100GE2-26",
            "eth59": "Eth100GE2-27",
            "eth60": "Eth100GE2-28",
            "eth61": "Eth100GE2-29",
            "eth62": "Eth100GE2-30",
            "eth63": "Eth100GE2-31",
            "eth64": "Eth100GE2-32",
            "eth65": "Eth100GE3-1",
            "eth66": "Eth100GE3-2",
            "eth67": "Eth100GE3-3",
            "eth68": "Eth100GE3-4",
            "eth69": "Eth100GE3-5",
            "eth70": "Eth100GE3-6",
            "eth71": "Eth100GE3-7",
            "eth72": "Eth100GE3-8",
            "eth73": "Eth100GE3-9",
            "eth74": "Eth100GE3-10",
            "eth75": "Eth100GE3-11",
            "eth76": "Eth100GE3-12",
            "eth77": "Eth100GE3-13",
            "eth78": "Eth100GE3-14",
            "eth79": "Eth100GE3-15",
            "eth80": "Eth100GE3-16",
            "eth81": "Eth100GE3-17",
            "eth82": "Eth100GE3-18",
            "eth83": "Eth100GE3-19",
            "eth84": "Eth100GE3-20",
            "eth85": "Eth100GE3-21",
            "eth86": "Eth100GE3-22",
            "eth87": "Eth100GE3-23",
            "eth88": "Eth100GE3-24",
            "eth89": "Eth100GE3-25",
            "eth90": "Eth100GE3-26",
            "eth91": "Eth100GE3-27",
            "eth92": "Eth100GE3-28",
            "eth93": "Eth100GE3-29",
            "eth94": "Eth100GE3-30",
            "eth95": "Eth100GE3-31",
            "eth96": "Eth100GE3-32",
            "eth97": "Eth100GE4-1",
            "eth98": "Eth100GE4-2",
            "eth99": "Eth100GE4-3",
            "eth100": "Eth100GE4-4",
            "eth101": "Eth100GE4-5",
            "eth102": "Eth100GE4-6",
            "eth103": "Eth100GE4-7",
            "eth104": "Eth100GE4-8",
            "eth105": "Eth100GE4-9",
            "eth106": "Eth100GE4-10",
            "eth107": "Eth100GE4-11",
            "eth108": "Eth100GE4-12",
            "eth109": "Eth100GE4-13",
            "eth110": "Eth100GE4-14",
            "eth111": "Eth100GE4-15",
            "eth112": "Eth100GE4-16",
            "eth113": "Eth100GE4-17",
            "eth114": "Eth100GE4-18",
            "eth115": "Eth100GE4-19",
            "eth116": "Eth100GE4-20",
            "eth117": "Eth100GE4-21",
            "eth118": "Eth100GE4-22",
            "eth119": "Eth100GE4-23",
            "eth120": "Eth100GE4-24",
            "eth121": "Eth100GE4-25",
            "eth122": "Eth100GE4-26",
            "eth123": "Eth100GE4-27",
            "eth124": "Eth100GE4-28",
            "eth125": "Eth100GE4-29",
            "eth126": "Eth100GE4-30",
            "eth127": "Eth100GE4-31",
            "eth128": "Eth100GE4-32",
        }
    },
    "slots": {
        "present": {"path": ["/sys/wb_plat/slot/*/status"], "ABSENT":0},
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "slot1": "SLOT1",
            "slot2": "SLOT2",
            "slot3": "SLOT3",
            "slot4": "SLOT4",
        }
    },
    "fans": {
        "present": {"path": ["/sys/wb_plat/fan/*/present"], "ABSENT":0},
        "status": [
            {"path": "/sys/wb_plat/fan/%s/motor0/status", 'okval': 1},
            {"path": "/sys/wb_plat/fan/%s/motor1/status", 'okval': 1},
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
            "fan6": "FAN6"
        }
    },
    "psus": {
        "present" : {"path": ["/sys/wb_plat/psu/*/present"], "ABSENT":0},
        "status" : [
            {"path": "/sys/wb_plat/psu/%s/output", "okval":1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0,
        "alias": {
            "psu1": "PSU1",
            "psu2": "PSU2",
            "psu3": "PSU3",
            "psu4": "PSU4",
        }
    }
}

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "cold_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0xb88, "okval": 0},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size": 1 * 1024 * 1024}
            ]
        },
        {
            "name": "otp_switch_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_switch_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: ASIC, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: ASIC, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size": 1 * 1024 * 1024}
            ],
            "finish_operation": [
                {"gettype": "cmd", "cmd": "rm -rf /etc/.otp_switch_reboot_flag"},
            ]
        },
        {
            "name": "otp_other_reboot",
            "monitor_point": {"gettype": "file_exist", "judge_file": "/etc/.otp_other_reboot_flag", "okval": True},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Thermal Overload: Other, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Thermal Overload: Other, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size": 1 * 1024 * 1024}
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

INIT_PARAM_PRE = [
    {"loc": "30-0064/hwmon/hwmon*/avs1_vout_max", "value": "900000"},
    {"loc": "30-0064/hwmon/hwmon*/avs1_vout_min", "value": "750000"},
]

INIT_COMMAND_PRE = [
    "dfd_debug io_wr 0xb19 0xff",
]

INIT_PARAM = []

INIT_COMMAND = [
    "i2cset -y -f 3 0x30 0xa0 0xff",
    "i2cset -y -f 3 0x31 0xa0 0xff",
    "i2cset -y -f 4 0x30 0xa0 0xff",
    "i2cset -y -f 4 0x31 0xa0 0xff",
    "i2cset -y -f 5 0x30 0xa0 0xff",
    "i2cset -y -f 5 0x31 0xa0 0xff",
    "i2cset -y -f 6 0x30 0xa0 0xff",
    "i2cset -y -f 6 0x31 0xa0 0xff",
]

BLACKLIST_DRIVERS = [
    {"name": "i2c_i801", "delay": 0},
]

DRIVERLISTS = [
    {"name": "wb_i2c_i801", "delay": 0},
    {"name": "wb_gpio_d1500", "delay": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "wb_i2c_algo_bit", "delay": 0},
    {"name": "wb_i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_gpio_device", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=17 gpio_scl=1 gpio_udelay=2", "delay": 0},
    {"name": "platform_common dfd_my_type_i2c_bus=1 dfd_my_type_i2c_addr=0x56", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_i2c_ocores", "delay": 0},
    {"name": "wb_i2c_ocores_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_platform_i2c_dev", "delay": 0},
    {"name": "wb_platform_i2c_dev_device", "delay": 0},
    {"name": "wb_lm75", "delay": 0},
    {"name": "wb_tmp401", "delay": 0},
    {"name": "wb_optoe", "delay": 0},
    {"name": "wb_at24", "delay": 0},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "wb_isl68137", "delay": 0},
    {"name": "wb_mac_bsc", "delay": 0},
    {"name": "wb_tps53622", "delay": 0},
    {"name": "firmware_driver_cpld", "delay": 0},
    {"name": "firmware_driver_ispvme", "delay": 0},
    {"name": "firmware_driver_sysfs", "delay": 0},
    {"name": "wb_firmware_upgrade_device", "delay": 0},
    {"name": "plat_dfd", "delay": 0},
    {"name": "plat_switch", "delay": 0},
    {"name": "plat_fan", "delay": 0},
    {"name": "plat_psu", "delay": 0},
    {"name": "plat_sff", "delay": 0},
    {"name": "plat_sensor", "delay": 0},
    {"name": "plat_slot", "delay": 0},
]

DEVICE = [
    {"name": "wb_lm75", "bus": 28, "loc": 0x4B},
    {"name": "wb_tmp411", "bus": 28, "loc": 0x4C},
    {"name": "wb_tmp411", "bus": 29, "loc": 0x4C},
    {"name": "wb_lm75", "bus": 29, "loc": 0x4F},
    {"name": "wb_lm75", "bus": 68, "loc": 0x48},
    {"name": "wb_lm75", "bus": 68, "loc": 0x49},
    {"name": "wb_lm75", "bus": 60, "loc": 0x48},
    {"name": "wb_lm75", "bus": 60, "loc": 0x49},
    {"name": "wb_lm75", "bus": 32, "loc": 0x48},
    {"name": "wb_lm75", "bus": 32, "loc": 0x49},
    {"name": "wb_lm75", "bus": 32, "loc": 0x4D},
    {"name": "wb_lm75", "bus": 48, "loc": 0x48},
    {"name": "wb_lm75", "bus": 48, "loc": 0x49},
    {"name": "wb_lm75", "bus": 48, "loc": 0x4D},
    {"name": "wb_lm75", "bus": 40, "loc": 0x48},
    {"name": "wb_lm75", "bus": 40, "loc": 0x49},
    {"name": "wb_lm75", "bus": 40, "loc": 0x4D},
    {"name": "wb_lm75", "bus": 16, "loc": 0x48},
    {"name": "wb_lm75", "bus": 16, "loc": 0x49},
    {"name": "wb_lm75", "bus": 16, "loc": 0x4D},
    {"name": "wb_24c02", "bus": 1, "loc": 0x56},
    # fan
    {"name": "wb_24c02", "bus": 63, "loc": 0x50},
    {"name": "wb_24c02", "bus": 55, "loc": 0x50},
    {"name": "wb_24c02", "bus": 64, "loc": 0x50},
    {"name": "wb_24c02", "bus": 56, "loc": 0x50},
    {"name": "wb_24c02", "bus": 65, "loc": 0x50},
    {"name": "wb_24c02", "bus": 57, "loc": 0x50},
    # slot
    {"name": "wb_24c02", "bus": 3, "loc": 0x57},
    {"name": "wb_24c02", "bus": 4, "loc": 0x57},
    {"name": "wb_24c02", "bus": 5, "loc": 0x57},
    {"name": "wb_24c02", "bus": 6, "loc": 0x57},
    {"name": "wb_24c02", "bus": 3, "loc": 0x56},
    {"name": "wb_24c02", "bus": 4, "loc": 0x56},
    {"name": "wb_24c02", "bus": 5, "loc": 0x56},
    {"name": "wb_24c02", "bus": 6, "loc": 0x56},
    # psu
    {"name": "wb_24c02", "bus": 23, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 23, "loc": 0x58},
    {"name": "wb_24c02", "bus": 25, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 25, "loc": 0x58},
    {"name": "wb_24c02", "bus": 24, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 24, "loc": 0x58},
    {"name": "wb_24c02", "bus": 26, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 26, "loc": 0x58},
    {"name": "wb_isl68127", "bus": 30, "loc": 0x64},
    #mac bsc
    {"name": "wb_mac_bsc_th3", "bus": 27, "loc": 0x44},
    {"name": "wb_tps53622", "bus": 10, "loc": 0x60},
    {"name": "wb_tps53622", "bus": 10, "loc": 0x6c},
]

OPTOE = [
    {"name": "wb_optoe1", "startbus": 71, "endbus": 198},
]

SLOT_MONITOR_PARAM = {
    "polling_time": 0.5,
    "slots": [
        {"name": "slot1",
         "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 4, "okval": 0},
         "act": [
             {"io_addr": 0xb19, "value": 0x01, "mask": 0xfe, "gettype": "io"},
             {"bus": 3, "loc": 0x30, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
             {"bus": 3, "loc": 0x31, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
         ],
         },
        {"name": "slot2",
         "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 5, "okval": 0},
         "act": [
             {"io_addr": 0xb19, "value": 0x02, "mask": 0xfd, "gettype": "io"},
             {"bus": 4, "loc": 0x30, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
             {"bus": 4, "loc": 0x31, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
         ],
         },
        {"name": "slot3",
         "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 6, "okval": 0},
         "act": [
             {"io_addr": 0xb19, "value": 0x04, "mask": 0xfb, "gettype": "io"},
             {"bus": 5, "loc": 0x30, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
             {"bus": 5, "loc": 0x31, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
         ],
         },
        {"name": "slot4",
         "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 7, "okval": 0},
         "act": [
             {"io_addr": 0xb19, "value": 0x08, "mask": 0xf7, "gettype": "io"},
             {"bus": 6, "loc": 0x30, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
             {"bus": 6, "loc": 0x31, "offset": 0xa0, "value": 0xff, "gettype": "i2c"},
         ],
         },
    ],
}

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "mac_core",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x73,                  # default value, if rov value not in range
        "rov_source": 0,                  # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"io_addr": 0x932, "gettype": "io"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/30-0064/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x72: 0.900,
            0x73: 0.894,
            0x74: 0.888,
            0x75: 0.882,
            0x76: 0.875,
            0x77: 0.869,
            0x78: 0.863,
            0x79: 0.857,
            0x7a: 0.850,
            0x7b: 0.844,
            0x7c: 0.838,
            0x7d: 0.832,
            0x7e: 0.825,
            0x7f: 0.819,
            0x80: 0.813,
            0x81: 0.807,
            0x82: 0.800,
            0x83: 0.794,
            0x84: 0.788,
            0x85: 0.782,
            0x86: 0.775,
            0x87: 0.769,
            0x88: 0.763,
            0x89: 0.757,
            0x8A: 0.750
        }
    }
]

UPGRADE_SUMMARY = {
    "devtype": 0x404d,
    "max_slot_num": 4,
    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BOARD_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "MTD": {
            "chain1": {
                "name": "BIOS",
                "is_support_warm_upg": 0,
                "filesizecheck": 10240,  # bios check file size, Unit: K
                "init_cmd": [
                    {"io_addr": 0x722, "value": 0x02, "gettype": "io"},
                    {"cmd": "modprobe mtd", "gettype": "cmd"},
                    {"cmd": "modprobe spi_nor", "gettype": "cmd"},
                    {"cmd": "modprobe ofpart", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi writeable=1", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi_platform writeable=1", "gettype": "cmd"},
                ],
                "finish_cmd": [
                    {"io_addr": 0x722, "value": 0x02, "gettype": "io"},
                    {"cmd": "rmmod intel_spi_platform", "gettype": "cmd"},
                    {"cmd": "rmmod intel_spi", "gettype": "cmd"},
                    {"cmd": "rmmod ofpart", "gettype": "cmd"},
                    {"cmd": "rmmod spi_nor", "gettype": "cmd"},
                    {"cmd": "rmmod mtd", "gettype": "cmd"},
                ],
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/board_cpld_test_header.vme", "display_name": "BOARD_CPLD"},
            ],
        },
    },

    "slot1": {
        "subtype": 0x4045,
        "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 4, "okval": 0},
        "VME": {
            "chain0": {
                "name": "SLOT1_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 0, "file": "/etc/.upgrade_test/slot_cpld_test_header.vme", "display_name": "SLOT1_CPLD"},
            ],
        },
    },

    "slot2": {
        "subtype": 0x4045,
        "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 5, "okval": 0},
        "VME": {
            "chain0": {
                "name": "SLOT2_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 0, "file": "/etc/.upgrade_test/slot_cpld_test_header.vme", "display_name": "SLOT2_CPLD"},
            ],
        },
    },

    "slot3": {
        "subtype": 0x4045,
        "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 6, "okval": 0},
        "VME": {
            "chain0": {
                "name": "SLOT3_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 0, "file": "/etc/.upgrade_test/slot_cpld_test_header.vme", "display_name": "SLOT3_CPLD"},
            ],
        },
    },

    "slot4": {
        "subtype": 0x4045,
        "present": {"gettype": "io", "io_addr": 0xb2c, "presentbit": 7, "okval": 0},
        "VME": {
            "chain0": {
                "name": "SLOT4_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 0, "file": "/etc/.upgrade_test/slot_cpld_test_header.vme", "display_name": "SLOT4_CPLD"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/63-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/55-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/64-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/56-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/65-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/57-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/23-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/25-0050/eeprom"},
        {"name": "psu3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/24-0050/eeprom"},
        {"name": "psu4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/26-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
    "slot": [
        {"name": "slot1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/3-0056/eeprom"},
        {"name": "slot2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/4-0056/eeprom"},
        {"name": "slot3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/5-0056/eeprom"},
        {"name": "slot4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/6-0056/eeprom"},
    ],
}
