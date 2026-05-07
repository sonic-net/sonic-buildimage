#!/usr/bin/python
# -*- coding: UTF-8 -*-

# CPO 100 board modified configuration, support 1 DC PSUS

from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "dev_monitor": 1,
    "tty_console": 0,
    "reboot_cause": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 0,
    "product_name": 1,
    "generate_mgmt_version": 0,
    "set_fw_mac": 1,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu2",
            "present": {"gettype": "i2c", "bus": 13, "loc": 0x1d, "offset": 0x42, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 10, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "wb_24c02", "bus": 10, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan7",
            "present": {"gettype": "i2c", "bus": 44, "loc": 0x0d, "offset": 0x5b, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan7frue2", "name": "wb_24c64", "bus": 43, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "i2c", "bus": 44, "loc": 0x0d, "offset": 0x5b, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "wb_24c64", "bus": 42, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 44, "loc": 0x0d, "offset": 0x5b, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "wb_24c64", "bus": 41, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 44, "loc": 0x0d, "offset": 0x5b, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "wb_24c64", "bus":40, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan8",
            "present": {"gettype": "i2c", "bus": 52, "loc": 0x0d, "offset": 0x5b, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan8frue2", "name": "wb_24c64", "bus": 51, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "i2c", "bus": 52, "loc": 0x0d, "offset": 0x5b, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "wb_24c64", "bus": 50, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 52, "loc": 0x0d, "offset": 0x5b, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "wb_24c64", "bus": 49, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 52, "loc": 0x0d, "offset": 0x5b, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "wb_24c64", "bus": 48, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "wb_24c02", "bus": 1, "loc": 0x56, "attr": "eeprom"},
                {"id": "eeprom_2", "name": "wb_24c02", "bus": 1, "loc": 0x57, "attr": "eeprom"},
                {"id": "eeprom_3", "name": "wb_24c02", "bus": 2, "loc": 0x51, "attr": "eeprom"},
                {"id": "eeprom_4", "name": "wb_24c02", "bus": 3, "loc": 0x51, "attr": "eeprom"},
                {"id": "eeprom_5", "name": "wb_24c02", "bus": 32, "loc": 0x52, "attr": "eeprom"},
                {"id": "eeprom_6", "name": "wb_24c02", "bus": 12, "loc": 0x57, "attr": "eeprom"},
            ],
        },
        {
            "name": "oe_mcu",
            "device": [
                {"id": "oe_mcu_1", "name": "wb_24c02", "bus": 24, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_2", "name": "wb_24c02", "bus": 25, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_3", "name": "wb_24c02", "bus": 26, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_4", "name": "wb_24c02", "bus": 27, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_5", "name": "wb_24c02", "bus": 28, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_6", "name": "wb_24c02", "bus": 29, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_7", "name": "wb_24c02", "bus": 30, "loc": 0x50, "attr": "eeprom"},
                {"id": "oe_mcu_8", "name": "wb_24c02", "bus": 31, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "tmp275",
            "device": [
                {"id": "tmp275_1", "name": "wb_tmp275", "bus": 34, "loc": 0x4c, "attr": "hwmon"},
                {"id": "tmp275_2", "name": "wb_tmp275", "bus": 35, "loc": 0x4d, "attr": "hwmon"},
                {"id": "tmp275_3", "name": "wb_tmp275", "bus": 8, "loc": 0x48, "attr": "hwmon"},
                {"id": "tmp275_4", "name": "wb_tmp275", "bus": 8, "loc": 0x49, "attr": "hwmon"},
                {"id": "tmp275_5", "name": "wb_tmp275", "bus": 9, "loc": 0x48, "attr": "hwmon"},
                {"id": "tmp275_6", "name": "wb_tmp275", "bus": 9, "loc": 0x49, "attr": "hwmon"},
                {"id": "tmp275_7", "name": "wb_tmp275", "bus": 14, "loc": 0x48, "attr": "hwmon"},
                {"id": "tmp275_8", "name": "wb_tmp275", "bus": 14, "loc": 0x49, "attr": "hwmon"},
                {"id": "tmp275_9", "name": "wb_tmp275", "bus": 14, "loc": 0x4b, "attr": "hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "wb_ina3221", "bus": 33, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_2", "name": "wb_ina3221", "bus": 33, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_3", "name": "wb_ina3221", "bus": 33, "loc": 0x42, "attr": "hwmon"},
                {"id": "ina3221_4", "name": "wb_ina3221", "bus": 12, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_5", "name": "wb_ina3221", "bus": 12, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_6", "name": "wb_ina3221", "bus": 56, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_7", "name": "wb_ina3221", "bus": 56, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_8", "name": "wb_ina3221", "bus": 57, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_9", "name": "wb_ina3221", "bus": 58, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_10", "name": "wb_ina3221", "bus": 59, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_11", "name": "wb_ina3221", "bus": 60, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_12", "name": "wb_ina3221", "bus": 61, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_13", "name": "wb_ina3221", "bus": 62, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_14", "name": "wb_ina3221", "bus": 63, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_15", "name": "wb_ina3221", "bus": 88, "loc": 0x42, "attr": "hwmon"},
                {"id": "ina3221_16", "name": "wb_ina3221", "bus": 89, "loc": 0x42, "attr": "hwmon"},
                {"id": "ina3221_17", "name": "wb_ina3221", "bus": 90, "loc": 0x42, "attr": "hwmon"},
                {"id": "ina3221_18", "name": "wb_ina3221", "bus": 91, "loc": 0x42, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "wb_xdpe12284", "bus": 5, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "wb_xdpe12284", "bus": 5, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_3", "name": "wb_xdpe12284", "bus": 5, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_4", "name": "wb_xdpe12284", "bus": 5, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_5", "name": "wb_xdpe12284", "bus": 67, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_6", "name": "wb_xdpe12284", "bus": 67, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_7", "name": "wb_xdpe12284", "bus": 67, "loc": 0x60, "attr": "hwmon"},
                {"id": "xdpe12284_8", "name": "wb_xdpe12284", "bus": 67, "loc": 0x66, "attr": "hwmon"},
                {"id": "xdpe12284_9", "name": "wb_xdpe12284", "bus": 67, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_10", "name": "wb_xdpe12284", "bus": 67, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_11", "name": "wb_xdpe12284", "bus": 68, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_12", "name": "wb_xdpe12284", "bus": 68, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_13", "name": "wb_xdpe12284", "bus": 68, "loc": 0x60, "attr": "hwmon"},
                {"id": "xdpe12284_14", "name": "wb_xdpe12284", "bus": 68, "loc": 0x66, "attr": "hwmon"},
                {"id": "xdpe12284_15", "name": "wb_xdpe12284", "bus": 68, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_16", "name": "wb_xdpe12284", "bus": 68, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_17", "name": "wb_xdpe12284", "bus": 69, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_18", "name": "wb_xdpe12284", "bus": 69, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_19", "name": "wb_xdpe12284", "bus": 69, "loc": 0x60, "attr": "hwmon"},
                {"id": "xdpe12284_20", "name": "wb_xdpe12284", "bus": 69, "loc": 0x66, "attr": "hwmon"},
                {"id": "xdpe12284_21", "name": "wb_xdpe12284", "bus": 69, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_22", "name": "wb_xdpe12284", "bus": 69, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_23", "name": "wb_xdpe12284", "bus": 70, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_24", "name": "wb_xdpe12284", "bus": 70, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_25", "name": "wb_xdpe12284", "bus": 70, "loc": 0x60, "attr": "hwmon"},
                {"id": "xdpe12284_26", "name": "wb_xdpe12284", "bus": 70, "loc": 0x66, "attr": "hwmon"},
                {"id": "xdpe12284_27", "name": "wb_xdpe12284", "bus": 70, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_28", "name": "wb_xdpe12284", "bus": 70, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_29", "name": "wb_xdpe12284", "bus": 71, "loc": 0x68, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe132g5c",
            "device": [
                {"id": "xdpe132g5c_1", "name": "wb_xdpe132g5c_pmbus", "bus": 64, "loc": 0x40, "attr": "hwmon"},
                {"id": "xdpe132g5c_2", "name": "wb_xdpe132g5c_pmbus", "bus": 65, "loc": 0x40, "attr": "hwmon"},
                {"id": "xdpe132g5c_3", "name": "wb_xdpe132g5c_pmbus", "bus": 66, "loc": 0x40, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "wb_ucd90160", "bus": 5, "loc": 0x5f, "attr": "hwmon"},
            ],
        },
    ],
}

MANUINFO_CONF = {
    "bios": {
        "key": "BIOS",
        "head": True,
        "next": "cpu"
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
    "bios_boot": {
        "parent": "bios",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld1",
            "offset":0x1d,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Master",
            "02": "Slave"
        },
        "arrt_index": 4,
    },

    "cpu": {
        "key": "CPU",
        "next": "cpld"
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


    "cpld": {
        "key": "CPLD",
        "next": "fpga"
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
        "config": "CPU CPLD",
        "arrt_index": 3,
    },
    "cpld1_version": {
        "key": "Firmware Version",
        "parent": "cpld1",
        "reg": {
            "loc": "/dev/port",
            "offset": 0xa00,
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
        "config": "SCM CPLD",
        "arrt_index": 3,
    },
    "cpld2_version": {
        "key": "Firmware Version",
        "parent": "cpld2",
        "reg": {
            "loc": "/dev/port",
            "offset": 0x900,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld2_boot": {
        "parent": "cpld2",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld1",
            "offset":0x06,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Main",
            "02": "Golden"
        },
        "arrt_index": 5,
    },

    "cpld3": {
        "key": "CPLD3",
        "parent": "cpld",
        "arrt_index": 3,
    },
    "cpld3_model": {
        "key": "Device Model",
        "parent": "cpld3",
        "config": "LCMXO3LF-4300C-6BG324I",
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
        "config": "MCB CPLD",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "13",
            "loc": "0x1d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld3_boot": {
        "parent": "cpld3",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld3",
            "offset":0x0d,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Main",
            "02": "Golden"
        },
        "arrt_index": 5,
    },

    "cpld4": {
        "key": "CPLD4",
        "parent": "cpld",
        "arrt_index": 4,
    },
    "cpld4_model": {
        "key": "Device Model",
        "parent": "cpld4",
        "config": "LCMXO3LF-4300C-6BG324I",
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
        "config": "SMB CPLD",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "20",
            "loc": "0x1e",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld4_boot": {
        "parent": "cpld4",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld4",
            "offset":0x0d,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Main",
            "02": "Golden"
        },
        "arrt_index": 5,
    },

    "cpld5": {
        "key": "CPLD5",
        "parent": "cpld",
        "arrt_index": 5,
    },
    "cpld5_model": {
        "key": "Device Model",
        "parent": "cpld5",
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "FCB_T CPLD",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "44",
            "loc": "0x0d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld5_boot": {
        "parent": "cpld5",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld5",
            "offset":0x0d,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Main",
            "02": "Golden"
        },
        "arrt_index": 5,
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
        "config": "FCB_B CPLD",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "i2c": {
            "bus": "52",
            "loc": "0x0d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
    },
    "cpld6_boot": {
        "parent": "cpld6",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/cpld6",
            "offset":0x0d,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "01": "Main",
            "02": "Golden"
        },
        "arrt_index": 5,
    },


    "psu": {
        "key": "PSU",
        "next": "fan"
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

    "fan": {
        "key": "FAN",
        "next": "fpga"
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
        "cmd": "ethtool -i eno1",
        "pattern": r"firmware-version",
        "separator": ":",
        "key": "Firmware Version",
        "arrt_index": 3,
    },

    "fpga": {
        "key": "FPGA",
    },

    "fpga1": {
        "key": "FPGA1",
        "parent": "fpga",
        "arrt_index": 1,
    },
    "fpga1_model": {
        "parent": "fpga1",
        "config": "XC7A50T-2FGG484C",
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
        "config": "IOB FPGA",
        "arrt_index": 3,
    },
    "fpga1_fw_version": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset": 0,
            "len": 4,
            "bit_width": 4
        },
        "key": "Firmware Version",
        "arrt_index": 4,
    },
    "fpga1_date": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset": 4,
            "len": 4,
            "bit_width": 4
        },
        "key": "Build Date",
        "arrt_index": 5,
    },
    "fpga1_boot": {
        "parent": "fpga1",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":0x00,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "00": "Golden",
            "default": "Main"
        },
        "arrt_index": 6,
    },
    "fpga2": {
        "key": "FPGA2",
        "parent": "fpga",
        "arrt_index": 2,
    },
    "fpga2_model": {
        "parent": "fpga2",
        "config": "XC7A50T-2FGG484C",
        "key": "Device Model",
        "arrt_index": 1,
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
        "config": "DOM FPGA",
        "arrt_index": 3,
    },
    "fpga2_fw_version": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset": 0,
            "len": 4,
            "bit_width": 4
        },
        "key": "Firmware Version",
        "arrt_index": 4,
    },
    "fpga2_date": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset": 4,
            "len": 4,
            "bit_width": 4
        },
        "key": "Build Date",
        "arrt_index": 5,
    },
    "fpga2_boot": {
        "parent": "fpga2",
        "key": "Boot From",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset":0x00,
            "len":1,
            "bit_width":1
        },
        "decode": {
            "00": "Golden",
            "default": "Main"
        },
        "arrt_index": 6,
    },
}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "fans": {
        "present": {"path": ["/sys/s3ip/fan/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/s3ip/fan/%s/motor1/status", 'okval': 1},
            {"path": "/sys/s3ip/fan/%s/motor2/status", 'okval': 1},
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
    }
}

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "mac_core",              # AVS name
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"bus": 20, "loc": 0x1e, "offset": 0x20, "gettype": "i2c"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/64-0040/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x92: 0.7471,
            0x90: 0.7600,
            0x8e: 0.7710,
            0x8c: 0.7839,
            0x8a: 0.7961,
            0x88: 0.8071,
            0x86: 0.8181,
            0x84: 0.8291,
            0x82: 0.8401
        }
    }
]

DRIVERLISTS = [
    {"name": "ice", "delay": 0, "removable": 0},
    {"name": "wb_i2c_i801", "delay": 1, "removable": 0},
    {"name": "i2c_dev", "delay": 0, "removable": 0},
    {"name": "wb_i2c_algo_bit", "delay": 0},
    {"name": "wb_i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0, "removable": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=181 gpio_scl=180 gpio_chip_name=INTC3001:00 bus_num=1", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40d9", "delay": 0},
    {"name": "wb_logic_dev_common", "delay":0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_i2c_ocores", "delay": 0},
    {"name": "wb_i2c_ocores_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device_b3", "delay": 1},
    {"name": "wb_i2c_dev_device", "delay": 1},
    {"name": "wb_lm75", "delay": 0},
    {"name": "wb_at24", "delay": 0},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "wb_ina3221", "delay": 0},
    {"name": "wb_ucd9000", "delay": 0},
    {"name": "wb_xdpe132g5c", "delay": 0},
#    {"name": "firmware_driver_sysfs", "delay": 0},
    {"name": "wb_rc32312", "delay": 0},
    {"name": "s3ip_sysfs", "delay": 0},
    {"name": "wb_switch_driver", "delay": 0},
    {"name": "fan_device_driver", "delay": 0},
    {"name": "psu_device_driver", "delay": 0},
    {"name": "vol_sensor_device_driver", "delay": 0},
    {"name": "temp_sensor_device_driver", "delay": 0},
    {"name": "wb_spd", "delay": 0},
    {"name": "wb_lpc_bmc", "delay": 0},
]

DEVICE = [
    # eeprom
    {"name": "wb_24c02", "bus": 1, "loc": 0x56},
    {"name": "wb_24c02", "bus": 1, "loc": 0x57},
    {"name": "wb_24c02", "bus": 2, "loc": 0x51},
    {"name": "wb_24c02", "bus": 3, "loc": 0x51},
    # SCM
    {"name": "wb_24c02", "bus": 32, "loc": 0x52},
    {"name": "wb_ina3221", "bus": 33, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 33, "loc": 0x41},
    {"name": "wb_ina3221", "bus": 33, "loc": 0x42},
    {"name": "wb_tmp275", "bus": 34, "loc": 0x4c},
    {"name": "wb_tmp275", "bus": 35, "loc": 0x4d},
    # CPU
    {"name": "wb_ucd90160", "bus": 5, "loc": 0x5f},
    {"name": "wb_xdpe12284", "bus": 5, "loc": 0x5e},
    {"name": "wb_xdpe12284", "bus": 5, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 5, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 5, "loc": 0x70},
    # fanA
    {"name": "wb_tmp275", "bus": 8, "loc": 0x48},
    {"name": "wb_tmp275", "bus": 8, "loc": 0x49},
    {"name": "wb_24c64", "bus": 40, "loc": 0x50},
    {"name": "wb_24c64", "bus": 41, "loc": 0x50},
    {"name": "wb_24c64", "bus": 42, "loc": 0x50},
    {"name": "wb_24c64", "bus": 43, "loc": 0x50},
    # fanB
    {"name": "wb_tmp275", "bus": 9, "loc": 0x48},
    {"name": "wb_tmp275", "bus": 9, "loc": 0x49},
    {"name": "wb_24c64", "bus": 48, "loc": 0x50},
    {"name": "wb_24c64", "bus": 49, "loc": 0x50},
    {"name": "wb_24c64", "bus": 50, "loc": 0x50},
    {"name": "wb_24c64", "bus": 51, "loc": 0x50},
    # psu
    {"name": "wb_24c02", "bus": 10, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 10, "loc": 0x58},
    # MCB
    {"name": "wb_ina3221", "bus": 12, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 12, "loc": 0x41},
    {"name": "wb_24c02", "bus": 12, "loc": 0x57},
    # SMB
    {"name": "wb_tmp275", "bus": 14, "loc": 0x48},
    {"name": "wb_tmp275", "bus": 14, "loc": 0x49},
    {"name": "wb_tmp275", "bus": 14, "loc": 0x4b},
    {"name": "wb_ina3221", "bus": 56, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 56, "loc": 0x41},
    {"name": "wb_ina3221", "bus": 57, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 58, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 59, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 60, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 61, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 62, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 63, "loc": 0x40},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 64, "loc": 0x40},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 65, "loc": 0x40},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 66, "loc": 0x40},
    # OE0
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x60},
    # OE7
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x66},
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 67, "loc": 0x5e},
    # OE1
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x60},
    # OE2
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x66},
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 68, "loc": 0x5e},
    # OE3
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x60},
    # OE4
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x66},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 69, "loc": 0x5e},
    # OE5
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x70},
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x60},
    # OE6
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x66},
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x6e},
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x5e},
    # MAC_XDPE12284C_06
    {"name": "wb_xdpe12284", "bus": 71, "loc": 0x68},
    # fan DCDC
    {"name": "wb_ina3221", "bus": 88, "loc": 0x42},
    {"name": "wb_ina3221", "bus": 89, "loc": 0x42},
    {"name": "wb_ina3221", "bus": 90, "loc": 0x42},
    # RC32312
    {"name": "wb_rc32312", "bus": 72, "loc": 0x09},
    {"name": "wb_24c64", "bus": 72, "loc": 0x50},
    {"name": "wb_rc32312", "bus": 73, "loc": 0x09},
    {"name": "wb_24c64", "bus": 73, "loc": 0x50},
    # xdpe132g5c i2c
    {"name": "wb_xdpe132g5c", "bus": 64, "loc": 0x10},
    {"name": "wb_xdpe132g5c", "bus": 65, "loc": 0x10},
    {"name": "wb_xdpe132g5c", "bus": 66, "loc": 0x10},
    # LED
    {"name": "wb_ina3221", "bus": 91, "loc": 0x42},
]

OPTOE = [
    {"name": "wb_24c02", "startbus": 24, "endbus": 31},
]

REBOOT_CTRL_PARAM = {}

INIT_PARAM_PRE = [
    # set ina3221 shunt_resistor
    # SCM_VDD12.0_C
    {"loc": "33-0041/hwmon/hwmon*/shunt1_resistor", "value": "1000"},
    # SCM_OCM_V12.0_C
    {"loc": "33-0041/hwmon/hwmon*/shunt3_resistor", "value": "1000"},
    # MAC_PLL0_VDD0.9_C
    {"loc": "56-0040/hwmon/hwmon*/shunt1_resistor", "value": "2000"},
    # MAC_PLL1_VDD0.9_C
    {"loc": "56-0040/hwmon/hwmon*/shunt2_resistor", "value": "2000"},
    # MAC_PLL3_VDD0.9_C
    {"loc": "61-0040/hwmon/hwmon*/shunt1_resistor", "value": "2000"},
    # MAC_PLL2_VDD0.9_C
    {"loc": "61-0040/hwmon/hwmon*/shunt2_resistor", "value": "2000"},
    # FAN1_VDD12_C
    {"loc": "88-0042/hwmon/hwmon*/shunt1_resistor", "value": "1000"},
    # FAN2_VDD12_C
    {"loc": "88-0042/hwmon/hwmon*/shunt2_resistor", "value": "1000"},
    # FAN3_VDD12_C
    {"loc": "88-0042/hwmon/hwmon*/shunt3_resistor", "value": "1000"},
    # FAN4_VDD12_C
    {"loc": "89-0042/hwmon/hwmon*/shunt1_resistor", "value": "1000"},
    # FAN5_VDD12_C
    {"loc": "89-0042/hwmon/hwmon*/shunt2_resistor", "value": "1000"},
    # FAN6_VDD12_C
    {"loc": "89-0042/hwmon/hwmon*/shunt3_resistor", "value": "1000"},
    # FAN7_VDD12_C
    {"loc": "90-0042/hwmon/hwmon*/shunt1_resistor", "value": "1000"},
    # FAN8_VDD12_C
    {"loc": "90-0042/hwmon/hwmon*/shunt2_resistor", "value": "1000"},
    # set avs threshold
    # MAC_CORE_V
    {"loc": "64-0040/hwmon/hwmon*/avs0_vout_min", "value": "630000"},
    {"loc": "64-0040/hwmon/hwmon*/avs0_vout_max", "value": "858000"},
    # MAC_ANALOG0 V0.9_V
    {"loc": "65-0040/hwmon/hwmon*/avs0_vout_min", "value": "810000"},
    {"loc": "65-0040/hwmon/hwmon*/avs0_vout_max", "value": "990000"},
    # MAC_ANALOG0 V0.75_V
    {"loc": "65-0040/hwmon/hwmon*/avs1_vout_min", "value": "675000"},
    {"loc": "65-0040/hwmon/hwmon*/avs1_vout_max", "value": "825000"},
    # MAC_ANALOG1 V0.9_V
    {"loc": "66-0040/hwmon/hwmon*/avs0_vout_min", "value": "810000"},
    {"loc": "66-0040/hwmon/hwmon*/avs0_vout_max", "value": "990000"},
    # MAC_ANALOG1 V0.75_V
    {"loc": "66-0040/hwmon/hwmon*/avs1_vout_min", "value": "675000"},
    {"loc": "66-0040/hwmon/hwmon*/avs1_vout_max", "value": "825000"},
    # # OE0_AVDD_RX_1.8_V
    # {"loc": "67-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "67-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "67-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE0_AVDD_RX_1.2_V
    # {"loc": "67-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "67-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "67-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE0_AVDD_TX_1.8_V
    # {"loc": "67-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "67-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "67-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE0_AVDD_TX_1.2_V
    # {"loc": "67-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "67-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "67-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE0_AVDD_TRX_0.75_V
    # {"loc": "67-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "67-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "67-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE1_AVDD_RX_1.8_V
    # {"loc": "68-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "68-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "68-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE1_AVDD_RX_1.2_V
    # {"loc": "68-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "68-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "68-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE1_AVDD_TX_1.8_V
    # {"loc": "68-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "68-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "68-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE1_AVDD_TX_1.2_V
    # {"loc": "68-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "68-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "68-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE1_AVDD_TRX_0.75_V
    # {"loc": "68-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "68-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "68-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE2_AVDD_RX_1.8_V
    # {"loc": "68-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "68-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "68-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE2_AVDD_RX_1.2_V
    # {"loc": "68-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "68-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "68-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE2_AVDD_TX_1.8_V
    # {"loc": "68-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "68-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "68-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE2_AVDD_TX_1.2_V
    # {"loc": "68-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "68-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "68-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE2_AVDD_TRX_0.75_V
    # {"loc": "68-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "68-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "68-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE3_AVDD_RX_1.8_V
    # {"loc": "69-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "69-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "69-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE3_AVDD_RX_1.2_V
    # {"loc": "69-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "69-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "69-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE3_AVDD_TX_1.8_V
    # {"loc": "69-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "69-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "69-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE3_AVDD_TX_1.2_V
    # {"loc": "69-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "69-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "69-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE3_AVDD_TRX_0.75_V
    # {"loc": "69-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "69-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "69-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE4_AVDD_RX_1.8_V
    # {"loc": "69-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "69-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "69-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE4_AVDD_RX_1.2_V
    # {"loc": "69-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "69-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "69-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE4_AVDD_TX_1.8_V
    # {"loc": "69-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "69-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "69-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE4_AVDD_TX_1.2_V
    # {"loc": "69-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "69-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "69-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE4_AVDD_TRX_0.75_V
    # {"loc": "69-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "69-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "69-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE5_AVDD_RX_1.8_V
    # {"loc": "70-0070/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "70-0070/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "70-0070/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE5_AVDD_RX_1.2_V
    # {"loc": "70-0070/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "70-0070/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "70-0070/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE5_AVDD_TX_1.8_V
    # {"loc": "70-0068/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "70-0068/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "70-0068/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE5_AVDD_TX_1.2_V
    # {"loc": "70-0068/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "70-0068/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "70-0068/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE5_AVDD_TRX_0.75_V
    # {"loc": "70-0060/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "70-0060/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "70-0060/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE6_AVDD_RX_1.8_V
    # {"loc": "70-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "70-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "70-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE6_AVDD_RX_1.2_V
    # {"loc": "70-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "70-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "70-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE6_AVDD_TX_1.8_V
    # {"loc": "70-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "70-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "70-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE6_AVDD_TX_1.2_V
    # {"loc": "70-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "70-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "70-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE6_AVDD_TRX_0.75_V
    # {"loc": "70-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "70-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "70-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # # OE7_AVDD_RX_1.8_V
    # {"loc": "67-006e/hwmon/hwmon*/avs0_vout_min", "value": "1710000"},
    # {"loc": "67-006e/hwmon/hwmon*/avs0_vout_max", "value": "1890000"},
    # {"loc": "67-006e/hwmon/hwmon*/avs0_vout", "value": "1800000"},
    # # OE7_AVDD_RX_1.2_V
    # {"loc": "67-006e/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "67-006e/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "67-006e/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE7_AVDD_TX_1.8_V
    # {"loc": "67-0066/hwmon/hwmon*/avs0_vout_min", "value": "1750000"},
    # {"loc": "67-0066/hwmon/hwmon*/avs0_vout_max", "value": "1950000"},
    # {"loc": "67-0066/hwmon/hwmon*/avs0_vout", "value": "1850000"},
    # # OE7_AVDD_TX_1.2_V
    # {"loc": "67-0066/hwmon/hwmon*/avs1_vout_min", "value": "1140000"},
    # {"loc": "67-0066/hwmon/hwmon*/avs1_vout_max", "value": "1260000"},
    # {"loc": "67-0066/hwmon/hwmon*/avs1_vout", "value": "1200000"},
    # # OE7_AVDD_TRX_0.75_V
    # {"loc": "67-005e/hwmon/hwmon*/avs0_vout_min", "value": "713000"},
    # {"loc": "67-005e/hwmon/hwmon*/avs0_vout_max", "value": "788000"},
    # {"loc": "67-005e/hwmon/hwmon*/avs0_vout", "value": "750000"},
    # RLML_VDD V3.3_V *1.5
    {"loc": "71-0068/hwmon/hwmon*/avs0_vout_min", "value": "1980000"},
    {"loc": "71-0068/hwmon/hwmon*/avs0_vout_max", "value": "2420000"},
    # RLMH_VDD V3.3_V *1.5
    {"loc": "71-0068/hwmon/hwmon*/avs1_vout_min", "value": "1980000"},
    {"loc": "71-0068/hwmon/hwmon*/avs1_vout_max", "value": "2420000"},
    # OCM_XDPE_VCCIN_V
    {"loc": "5-0070/hwmon/hwmon*/avs0_vout_min", "value": "1350000"},
    {"loc": "5-0070/hwmon/hwmon*/avs0_vout_max", "value": "2200000"},
    # OCM_XDPE_P1V8_V
    {"loc": "5-0070/hwmon/hwmon*/avs1_vout_min", "value": "1690000"},
    {"loc": "5-0070/hwmon/hwmon*/avs1_vout_max", "value": "1910000"},
    # OCM_XDPE_P1V05_V
    {"loc": "5-006e/hwmon/hwmon*/avs0_vout_min", "value": "954000"},
    {"loc": "5-006e/hwmon/hwmon*/avs0_vout_max", "value": "1160000"},
    # OCM_XDPE_VNN_PCH_V
    {"loc": "5-006e/hwmon/hwmon*/avs1_vout_min", "value": "540000"},
    {"loc": "5-006e/hwmon/hwmon*/avs1_vout_max", "value": "1320000"},
    # OCM_XDPE_VNN_NAC_V
    {"loc": "5-0068/hwmon/hwmon*/avs0_vout_min", "value": "540000"},
    {"loc": "5-0068/hwmon/hwmon*/avs0_vout_max", "value": "1320000"},
    # OCM_XDPE_VCC_ANA_V
    {"loc": "5-0068/hwmon/hwmon*/avs1_vout_min", "value": "900000"},
    {"loc": "5-0068/hwmon/hwmon*/avs1_vout_max", "value": "1100000"},
    # OCM_XDPE_P1V2_VDDQ_V
    {"loc": "5-005e/hwmon/hwmon*/avs0_vout_min", "value": "1120000"},
    {"loc": "5-005e/hwmon/hwmon*/avs0_vout_max", "value": "1280000"},
]

INIT_PARAM = []

INIT_COMMAND_PRE = []

INIT_COMMAND = [
    # enable fan watchdog
    {"path": "/dev/cpld5", "offset": 0x22, "value": [0x01], "gettype": "devfile"},
    {"path": "/dev/cpld6", "offset": 0x22, "value": [0x01], "gettype": "devfile"},
]

UPGRADE_SUMMARY = {
    "devtype": 0x40d9,

    "slot0": {
        "subtype": 0,
        "SPI-LOGIC-DEV": {
            "chain1":{
                "name":"IOB_FPGA",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain2":{
                "name":"DOM_FPGA",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain3":{
                "name":"SCM_CPLD",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain4":{
                "name":"MCB_CPLD",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain5":{
                "name":"SMB_CPLD",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain6":{
                "name":"FCB_B_CPLD",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain7":{
                "name":"FCB_T_CPLD",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
            "chain8":{
                "name":"MAC_PCIe",
                "is_support_warm_upg":0,
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
                ]
            },
        },

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/0x40d9/iob_fpga_test_header.bin", "display_name": "IOB_FPGA"},
                {"chain": 2, "file": "/etc/.upgrade_test/0x40d9/dom_fpga_test_header.bin", "display_name": "DOM_FPGA"},
            ],
            "cpld": [
                {"chain": 3, "file": "/etc/.upgrade_test/0x40d9/scm_cpld_spi_test_header.bin", "display_name": "SCM_CPLD_SPI"},
                {"chain": 4, "file": "/etc/.upgrade_test/0x40d9/mcb_cpld_spi_test_header.bin", "display_name": "MCB_CPLD_SPI"},
                {"chain": 5, "file": "/etc/.upgrade_test/0x40d9/smb_cpld_spi_test_header.bin", "display_name": "SMB_CPLD_SPI"},
                {"chain": 6, "file": "/etc/.upgrade_test/0x40d9/fcb_b_cpld_spi_test_header.bin", "display_name": "FCB_B_CPLD_SPI"},
                {"chain": 7, "file": "/etc/.upgrade_test/0x40d9/fcb_t_cpld_spi_test_header.bin", "display_name": "FCB_T_CPLD_SPI"},
                #{"chain": 8, "file": "/etc/.upgrade_test/0x40d9/mac_pcie_spi_test_header.bin", "display_name": "MAC_PCIe"},
            ],
        },
    },

    "BMC": {
        "name": "BMC",
        "init_cmd": [
            # firmware upgrade set sysled blue/amber alternate flashing
            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]}
        ],
        "finish_cmd": [],
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/40-0050/eeprom"},
        {"name": "fan2", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/48-0050/eeprom"},
        {"name": "fan3", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/41-0050/eeprom"},
        {"name": "fan4", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/49-0050/eeprom"},
        {"name": "fan5", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom"},
        {"name": "fan6", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/50-0050/eeprom"},
        {"name": "fan7", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/43-0050/eeprom"},
        {"name": "fan8", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/51-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu2", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/10-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
    "scm": [
        {"name": "scm", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/32-0052/eeprom"},
    ],
    "smb": [
        {"name": "smb", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/3-0051/eeprom"},
    ],
    "chassis": [
        {"name": "chassis", "e2_type": ["wedge_v5","fru"], "e2_path": "/sys/bus/i2c/devices/2-0051/eeprom"},
    ],
}

PLATFORM_POWER_CONF = [
    {
        "name": "Input power total",
        "unit": "W",
        "children": [
            {
                "name": "PSU2 input",
                "pre_check": {
                    "loc": "/sys/s3ip/psu/psu2/present",
                    "gettype": "sysfs", "mask": 0x01, "okval": 1,
                    "not_ok_msg": "ABSENT"
                },
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/10-0058/hwmon/hwmon*/power1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
        ]
    },
    {
        "name": "Output power total",
        "unit": "W",
        "children": [
            {
                "name": "PSU2 output",
                "pre_check": {
                    "loc": "/sys/s3ip/psu/psu2/present",
                    "gettype": "sysfs", "mask": 0x01, "okval": 1,
                    "not_ok_msg": "ABSENT"
                },
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/10-0058/hwmon/hwmon*/power2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
        ]
    },
    {
        "name": "MAC power consumption",
        "unit": "W",
        "children": [
            {
                "name": "MAC_CORE",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/64-0040/hwmon/hwmon*/power3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*2/1000000)"
            },
            {
                "name": "MAC_ANALOG0 V0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/65-0040/hwmon/hwmon*/power3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "MAC_ANALOG0 V0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/65-0040/hwmon/hwmon*/power4_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "MAC_ANALOG1 V0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/power3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "MAC_ANALOG1 V0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/66-0040/hwmon/hwmon*/power4_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "MAC_PLL0_VDD0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_PLL1_VDD0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_PLL2_VDD0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_PLL3_VDD0.9",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in3_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_VDD_0.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/58-0040/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_VDD_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_VDD_1.5",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "MAC_VDD_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/61-0040/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/61-0040/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            }
        ]
    },
    {
        "name": "CPO OE0 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE0_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE0_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0070/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE0_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE0_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0068/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE0_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0060/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0060/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE1 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE1_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE1_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0070/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE1_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE1_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0068/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE1_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0060/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0060/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE2 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE2_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE2_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-006e/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE2_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE2_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-0066/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE2_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-005e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/68-005e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE3 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE3_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE3_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0070/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE3_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE3_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0068/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE3_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0060/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0060/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE4 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE4_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE4_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-006e/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE4_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE4_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0066/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE4_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-005e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-005e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE5 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE5_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE5_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0070/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE5_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE5_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE5_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0060/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0060/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE6 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE6_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE6_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-006e/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE6_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE6_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0066/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE6_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-005e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-005e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "CPO OE7 power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE7_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.97+220)/1000000)"
            },
            {
                "name": "OE7_AVDD_RX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-006e/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+180)/1000000)"
            },
            {
                "name": "OE7_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+177)/1000000)"
            },
            {
                "name": "OE7_AVDD_TX_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopb_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-0066/hwmon/hwmon*/loopb_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.974+126)/1000000)"
            },
            {
                "name": "OE7_AVDD_TRX_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-005e/hwmon/hwmon*/loopa_vout",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/67-005e/hwmon/hwmon*/loopa_iout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*(float(%s)*0.984+140)/1000000)"
            },
        ]
    },
    {
        "name": "RLM power consumption",
        "unit": "W",
        "children": [
            {
                "name": "RLML_VDD V3.3",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopa_pout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*1.5/1000000)"
            },
            {
                "name": "RLMH_VDD V3.3",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/loopb_pout",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*1.5/1000000)"
            },
        ]
    },
    {
        "name": "CPU sub-module power consumption",
        "unit": "W",
        "val_conf": [
            {
               "gettype": "sysfs",
               "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/in3_input",
               "int_decode": 10,
            },
            {
               "gettype": "sysfs",
               "loc": "/sys/bus/i2c/devices/33-0041/hwmon/hwmon*/curr3_input",
               "int_decode": 10,
            }
        ],
        "format": "float(float(%s)*float(%s)/1000000)"
    },
    {
        "name": "FAN power consumption total",
        "unit": "W",
        "children": [
            {
                "name": "FAN1 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN2 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN3 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN4 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in3_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN5 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/in3_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/88-0042/hwmon/hwmon*/curr3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN6 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN7 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/89-0042/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "FAN8 power consumption",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/90-0042/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },

        ]
    },
]

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
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

POWER_CTRL_CONF = {
    "oe": [
        {
            "name": "OE0",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x01},
                    "off": [0x00],
                    "on": [0x01],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x81, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_7", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x01, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x81, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x81, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x81, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_7", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x01, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfe, "value": 0x01, "delay": 0.01},
            ],
        },
        {
            "name": "OE1",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x02},
                    "off": [0x00],
                    "on": [0x02],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x82, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_4", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x02, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x82, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x82, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x82, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_4", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x02, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfd, "value": 0x02, "delay": 0.01},
            ],
        },
        {
            "name": "OE2",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x04},
                    "off": [0x00],
                    "on": [0x04],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x83, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_3", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x04, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x83, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x83, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x83, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_3", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x04, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xfb, "value": 0x04, "delay": 0.01},
            ],
        },
        {
            "name": "OE3",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x08},
                    "off": [0x00],
                    "on": [0x08],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_0", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x84, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_0", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x08, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_0", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x84, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_0", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x84, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x84, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/72-0009/out_en_ctrl_0", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x08, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xf7, "value": 0x08, "delay": 0.01},
            ],
        },
        {
            "name": "OE4",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x10},
                    "off": [0x00],
                    "on": [0x10],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_1", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x85, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_1", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x10, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_1", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x85, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_1", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x85, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x85, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_1", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x10, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xef, "value": 0x10, "delay": 0.01},
            ],
        },
        {
            "name": "OE5",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x20},
                    "off": [0x00],
                    "on": [0x20],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x86, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_3", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x20, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x86, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_3", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x86, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x86, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_3", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x20, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xdf, "value": 0x20, "delay": 0.01},
            ],
        },
        {
            "name": "OE6",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x40},
                    "off": [0x00],
                    "on": [0x40],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x87, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_4", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x40, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x87, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_4", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x87, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x87, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_4", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x40, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0xbf, "value": 0x40, "delay": 0.01},
            ],
        },
        {
            "name": "OE7",
            "status": [
                {
                    "val_conf": {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x22, "mask": 0x80},
                    "off": [0x00],
                    "on": [0x80],
                }
            ],
            "on": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x88, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_7", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x80, "delay": 0.001},
            ],
            "off": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x88, "value": 0xfe},
            ],
            "cycle": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x00, "delay": 0.001},
                # disable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_7", "value": 0x00, "delay": 0.001},
                # power off OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x88, "value": 0xfe, "delay": 0.001},
                # power on OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x88, "value": 0xff, "delay": 0.001},
                # enable OE REFCLK
                {"gettype": "sysfs", "loc": "/sys/bus/i2c/devices/73-0009/out_en_ctrl_7", "value": 0x01, "delay": 0.001},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x80, "delay": 0.001},
            ],
            "reset": [
                # reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x00},
                # un reset OE
                {"gettype": "i2c", "bus": 20, "loc": 0x1e, "offset": 0x72, "mask": 0x7f, "value": 0x80, "delay": 0.01},
            ],
        }
    ]
}

SET_FW_MAC_CONF = [
    {
        "name": "eth0",
        "e2_name": "syseeprom",
        "get_act_mac": {"cmd": "ethtool -e eth0 offset 0 length 6 |grep 0x0000 |  awk -F':' '{gsub(/ /, \":\", $2);print $2}'| sed 's/^[[:space:]]*//' | sed 's/:\s*$//'", "gettype": "cmd_str"},
        "set_mac_cfg": {
            "type": 1,
            "cmd": {"cmd": "ethtool -E eth0 magic 0x15338086 offset %s value 0x%s length 1", "gettype": "cmd", "delay": 0.1},
        },
        "check_mac_cfg": {"cmd": "ethtool -e eth0 offset 0 length 6 |grep 0x0000 |  awk -F':' '{gsub(/ /, \":\", $2);print $2}'| sed 's/^[[:space:]]*//' | sed 's/:\s*$//'", "gettype": "cmd_str"},
    }
]

SET_MAC_CONF = SET_FW_MAC_CONF
PRODUCT_NAME_CONF = {}
