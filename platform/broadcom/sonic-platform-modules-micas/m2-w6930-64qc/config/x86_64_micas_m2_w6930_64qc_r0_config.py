#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "fancontrol": 0,
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 0,
    "tty_console": 1,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "reboot_cause": 1,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {"name": "psu1",
         "present": {"gettype": "io", "io_addr": 0x964, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 83, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu1frue2", "name": "24c02", "bus": 83, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu2",
         "present": {"gettype": "io", "io_addr": 0x964, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 84, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu2frue2", "name": "24c02", "bus": 84, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu3",
         "present": {"gettype": "io", "io_addr": 0x965, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "psu3pmbus", "name": "wb_fsp1200", "bus": 86, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu3frue2", "name": "24c02", "bus": 86, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu4",
         "present": {"gettype": "io", "io_addr": 0x965, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "psu4pmbus", "name": "wb_fsp1200", "bus": 85, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu4frue2", "name": "24c02", "bus": 85, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "fans": [
        {"name": "fan1",
         "present": {"gettype": "i2c", "bus": 92, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan1frue2", "name": "24c64", "bus": 95, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan2",
         "present": {"gettype": "i2c", "bus": 101, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan5frue2", "name": "24c64", "bus": 104, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan3",
         "present": {"gettype": "i2c", "bus": 92, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan2frue2", "name": "24c64", "bus": 96, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan4",
         "present": {"gettype": "i2c", "bus": 101, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan6frue2", "name": "24c64", "bus": 105, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan5",
         "present": {"gettype": "i2c", "bus": 92, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan3frue2", "name": "24c64", "bus": 97, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan6",
         "present": {"gettype": "i2c", "bus": 101, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan7frue2", "name": "24c64", "bus": 106, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan7",
         "present": {"gettype": "i2c", "bus": 92, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan4frue2", "name": "24c64", "bus": 98, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan8",
         "present": {"gettype": "i2c", "bus": 101, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan8frue2", "name": "24c64", "bus": 107, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "24c02", "bus": 1, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "lm75", "bus": 79, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 93, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 94, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 102, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 117, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_6", "name": "lm75", "bus": 118, "loc": 0x4f, "attr": "hwmon"},
                {"id": "lm75_7", "name": "lm75", "bus": 198, "loc": 0x4b, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_th4", "bus": 122, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name":"tmp411",
            "device":[
                {"id":"tmp411_1", "name":"tmp411","bus":119, "loc":0x4c, "attr":"hwmon"},
                {"id":"tmp411_2", "name":"tmp411","bus":120, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "ina3221", "bus": 78, "loc": 0x43, "attr": "hwmon"},
            ],
        },
        {
            "name": "tps53622",
            "device": [
                {"id": "tps53622_1", "name": "tps53688", "bus": 78, "loc": 0x67, "attr": "hwmon"},
                {"id": "tps53622_2", "name": "tps53688", "bus": 78, "loc": 0x6c, "attr": "hwmon"},
                {"id": "tps53622_3", "name": "tps53688", "bus": 131, "loc": 0x67, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 77, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 128, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_3", "name": "ucd90160", "bus": 129, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_4", "name": "ucd90160", "bus": 130, "loc": 0x5b, "attr": "hwmon"},
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
        "config": "CONNECT_CPLD",
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
        "config": "MAC_CPLDA",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "109",
            "loc": "0x1d",
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
        "config": "LCMXO3LF-2100C-5BG256C",
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
        "config": "MAC_CPLDB",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "110",
            "loc": "0x2d",
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
        "config": "PORT_CPLDA",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "111",
            "loc": "0x3d",
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
        "config": "PORT_CPLDB",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "i2c": {
            "bus": "112",
            "loc": "0x4d",
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
        "config": "FAN_CPLDA",
        "arrt_index": 3,
    },
    "cpld7_version": {
        "key": "Firmware Version",
        "parent": "cpld7",
        "i2c": {
            "bus": "92",
            "loc": "0x0d",
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
        "config": "FAN_CPLDB",
        "arrt_index": 3,
    },
    "cpld8_version": {
        "key": "Firmware Version",
        "parent": "cpld8",
        "i2c": {
            "bus": "101",
            "loc": "0x0d",
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
    },

    "fpga1": {
        "key": "FPGA1",
        "parent": "fpga",
        "arrt_index": 1,
    },
    "fpga1_model": {
        "parent": "fpga1",
        "config": "XC7A100T-2FGG484C",
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
        "pci": {
            "bus": 8,
            "slot": 0,
            "fn": 0,
            "bar": 0,
            "offset": 0
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga1_date": {
        "parent": "fpga1",
        "pci": {
            "bus": 8,
            "slot": 0,
            "fn": 0,
            "bar": 0,
            "offset": 4
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
        "config": "XC7A100T-2FGG484C",
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
        "config": "PORT_FPGA",
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
            "offset": 0,
            "len": 4,
            "bit_width": 4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
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
        "arrt_index": 6,
    },
    "others": {
        "key": "OTHERS",
    },
    "5387": {
        "parent": "others",
        "key": "CPU-BMC-SWITCH",
        "arrt_index": 1,
    },
    "5387_model": {
        "parent": "5387",
        "config": "BCM5387",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "5387_vendor": {
        "parent": "5387",
        "config": "Broadcom",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "5387_hw_version": {
        "parent": "5387",
        "key": "Hardware Version",
        "func": {
            "funcname": "get_bcm5387_version",
            "params": {
                "before": [
                    # OE pull high
                    {"gettype": "cmd", "cmd": "echo 50 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio50/direction"},
                    # SEL1 set high
                    {"gettype": "cmd", "cmd": "echo 48 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio48/direction"},
                    # select update 5387
                    {"gettype": "io", "io_addr": 0x918, "value": 0x06},
                    # enable 5387
                    {"gettype": "io", "io_addr": 0x943, "value": 0x00},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=65 miso=32 mosi=67 bus=0"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46 spi_bus_num=0 spi_cs_gpio=6"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio48/value"},
                    {"gettype": "cmd", "cmd": "echo 48 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio50/value"},
                    {"gettype": "cmd", "cmd": "echo 50 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "io", "io_addr": 0x943, "value": 0x01},
                    {"gettype": "io", "io_addr": 0x918, "value": 0x00},
                ],
            },
        },
        "arrt_index": 3,
    },
}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "sffs": {
        "present": {"path": ["/sys/wb_plat/sff/*/present"], "ABSENT": 0},
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
        }
    },
    "fans": {
        "present": {"path": ["/sys/wb_plat/fan/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/wb_plat/fan/%s/motor0/status", 'okval': 1},
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
            "fan8": "FAN8",
        }
    },
    "psus": {
        "present": {"path": ["/sys/wb_plat/psu/*/present"], "ABSENT": 0},
        "status": [
            {"path": "/sys/wb_plat/psu/%s/output", "okval": 1},
            {"path": "/sys/wb_plat/psu/%s/alert", "okval": 0},
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
            "monitor_point": {"gettype": "io", "io_addr": 0x910, "okval": 0},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size": 1 * 1024 * 1024}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x91a, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", 
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size":1*1024*1024}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x919, "value": 0xfc},
            ]
        },
        {
            "name": "bmc_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x91b, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC reboot, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x919, "value": 0xfa},
            ]
        },
        {
            "name": "bmc_powerdown",
            "monitor_point": {"gettype": "io", "io_addr": 0x91c, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC powerdown, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x919, "value": 0xf6},
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

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "mac_core",              # AVS name
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x82,                 # default value, if rov value not in range
        "sdkreg": "TOP_AVS_SEL_REG",     # SDK register name
        "sdktype": 0,                    # 0: No shift operation required, 1: shift operation required
        "macregloc": 24,                 # Shift right 24 bits
        "mask": 0xff,                    # Use with macregloc
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"bus": 109, "loc": 0x1d, "offset": 0x24, "gettype": "i2c"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/126-0010/avs_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x7e: 0.90090,
            0x82: 0.87820,
            0x86: 0.85640,
            0x8A: 0.83370,
            0x8E: 0.80960,
        }
    }
]

BLACKLIST_DRIVERS = [
    {"name": "i2c_i801", "delay": 0},
]

DRIVERLISTS = [
    {"name": "i2c_i801", "delay": 1},
    {"name": "wb_gpio_d1500", "delay": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "i2c_algo_bit", "delay": 0},
    {"name": "i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_gpio_device", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=17 gpio_scl=1 gpio_udelay=2", "delay": 0},
    {"name": "platform_common dfd_my_type=0x20000055", "delay": 0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_spi_ocores", "delay": 0},
    {"name": "wb_spi_ocores_device", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_spi_dev_device", "delay": 0},
    {"name": "wb_spi_dev_platform_device", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "lm75", "delay": 0},
    {"name": "tmp401", "delay": 0},
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
    {"name": "wb_mac_bsc", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "ina3221", "delay": 0},
    {"name": "tps53679", "delay": 0},
    {"name": "ucd9000", "delay": 0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "plat_dfd", "delay": 0},
    {"name": "plat_switch", "delay": 0},
    {"name": "plat_fan", "delay": 0},
    {"name": "plat_psu", "delay": 0},
    {"name": "plat_sff", "delay": 0},
    {"name": "wb_spi_master", "delay": 0},
]

DEVICE = [
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "wb_mac_bsc_th4", "bus": 122, "loc": 0x44},
    # fan
    {"name": "24c64", "bus": 95, "loc": 0x50},
    {"name": "24c64", "bus": 96, "loc": 0x50},
    {"name": "24c64", "bus": 97, "loc": 0x50},
    {"name": "24c64", "bus": 98, "loc": 0x50},
    {"name": "24c64", "bus": 104, "loc": 0x50},
    {"name": "24c64", "bus": 105, "loc": 0x50},
    {"name": "24c64", "bus": 106, "loc": 0x50},
    {"name": "24c64", "bus": 107, "loc": 0x50},
    # psu
    {"name": "24c02", "bus": 83, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 83, "loc": 0x58},
    {"name": "24c02", "bus": 84, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 84, "loc": 0x58},
    {"name": "24c02", "bus": 86, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 86, "loc": 0x58},
    {"name": "24c02", "bus": 85, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 85, "loc": 0x58},
    # temp
    {"name": "lm75", "bus": 79, "loc": 0x4b},
    {"name": "lm75", "bus": 93, "loc": 0x48},
    {"name": "lm75", "bus": 94, "loc": 0x49},
    {"name": "lm75", "bus": 102, "loc": 0x48},
    {"name": "lm75", "bus": 103, "loc": 0x49},
    {"name": "lm75", "bus": 117, "loc": 0x4b},
    {"name": "lm75", "bus": 118, "loc": 0x4f},
    {"name": "tmp411", "bus": 119, "loc": 0x4c},
    {"name": "tmp411", "bus": 120, "loc": 0x4c},
    {"name": "lm75", "bus": 198, "loc": 0x4b},
    #dcdc
    {"name": "ucd90160", "bus": 77, "loc": 0x5b},
    {"name": "tps53688", "bus": 78, "loc": 0x67},
    {"name": "tps53688", "bus": 78, "loc": 0x6c},
    {"name": "ina3221", "bus": 78, "loc": 0x43},
    {"name": "ucd90160", "bus": 128, "loc": 0x5b},
    {"name": "ucd90160", "bus": 129, "loc": 0x5b},
    {"name": "ucd90160", "bus": 130, "loc": 0x5b},
    {"name": "tps53688", "bus": 131, "loc": 0x67},
    #avs
    {"name": "wb_xdpe132g5c", "bus": 126, "loc": 0x10},
]

OPTOE = [
    {"name": "optoe3", "startbus": 133, "endbus": 196},
]

REBOOT_CTRL_PARAM = {
    "cpu": {"io_addr": 0x920, "rst_val": 0xfe, "rst_delay": 0, "gettype": "io"},
    "mac": {"bus": 109, "loc": 0x1d, "offset": 0x11, "rst_val": 0xfd, "rst_delay": 0, "gettype": "i2c"},
    "phy": {"io_addr": 0x921, "rst_val": 0xef, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
    "power": {"io_addr": 0x923, "rst_val": 0x01, "rst_delay": 0, "gettype": "io"},
}

# INIT_PARAM_PRE = [
#     {"loc": "126-0010/avs_vout_max", "value": "900900"},
#     {"loc": "126-0010/avs_vout_min", "value": "809600"},
# ]

INIT_PARAM = []

INIT_COMMAND_PRE = [
    # sfp power enable
    "dfd_debug io_wr 0x939 0x01",
    "i2cset -f -y 109 0x1d 0x73 0xff",
    "i2cset -f -y 110 0x2d 0x79 0xff",
    "i2cset -f -y 110 0x2d 0x7a 0xff",
    "i2cset -f -y 110 0x2d 0x7b 0xff",
    "i2cset -f -y 111 0x3d 0x76 0xff",
    "i2cset -f -y 111 0x3d 0x77 0xff",
    "i2cset -f -y 112 0x4d 0x79 0xff",
    "i2cset -f -y 112 0x4d 0x7a 0xff",
    "i2cset -f -y 112 0x4d 0x7b 0xff",
    # enable tty_console monitor
    "dfd_debug io_wr 0x966 0x01",
]

INIT_COMMAND = [
    # enable led
    "i2cset -f -y 109 0x1d 0xc4 0x1",
    "i2cset -f -y 110 0x2d 0xcc 0x1",
    "i2cset -f -y 111 0x3d 0xc8 0x1",
    "i2cset -f -y 112 0x4d 0xc9 0x1",

    # port led off
    #MAC CPLD_U14 register: 
    #port 50, 49, 54, 53, 58, 57, 62, 61
    "i2cset -f -y 109 0x1d 0xc0 0xff",
    "i2cset -f -y 109 0x1d 0xc1 0xff",
    "i2cset -f -y 109 0x1d 0xc2 0xff",
    "i2cset -f -y 109 0x1d 0xc3 0xff",

    #MAC CPLD_U14 register:
    #port 2, 1, 6, 5, 10, 9, 14, 13, 18, 17, 22, 21, 26, 25, 30, 29, 34, 33, 37, 38, 42, 41, 46, 45
    "i2cset -f -y 110 0x2d 0xc0 0xff",
    "i2cset -f -y 110 0x2d 0xc1 0xff",
    "i2cset -f -y 110 0x2d 0xc2 0xff",
    "i2cset -f -y 110 0x2d 0xc3 0xff",
    "i2cset -f -y 110 0x2d 0xc4 0xff",
    "i2cset -f -y 110 0x2d 0xc5 0xff",
    "i2cset -f -y 110 0x2d 0xc6 0xff",
    "i2cset -f -y 110 0x2d 0xc7 0xff",
    "i2cset -f -y 110 0x2d 0xc8 0xff",
    "i2cset -f -y 110 0x2d 0xc9 0xff",
    "i2cset -f -y 110 0x2d 0xca 0xff",
    "i2cset -f -y 110 0x2d 0xcb 0xff",
    
    #PORT CPLD_U5 register:
    #port:4, 3, 8, 7, 12, 11, 16, 15, 23, 20, 27, 24, 31, 28, 32
    "i2cset -f -y 111 0x3d 0xc0 0xff",
    "i2cset -f -y 111 0x3d 0xc1 0xff",
    "i2cset -f -y 111 0x3d 0xc2 0xff",
    "i2cset -f -y 111 0x3d 0xc3 0xff",
    "i2cset -f -y 111 0x3d 0xc4 0xff",
    "i2cset -f -y 111 0x3d 0xc5 0xff",
    "i2cset -f -y 111 0x3d 0xc6 0xff",
    "i2cset -f -y 111 0x3d 0xc7 0xff",
    
    #PORT CPLD_U9 register
    #port: 35, 19, 39, 36, 43, 40, 47, 44, 51, 48, 55, 52, 59, 56, 63, 60, 64
    "i2cset -f -y 112 0x4d 0xc0 0xff",
    "i2cset -f -y 112 0x4d 0xc1 0xff",
    "i2cset -f -y 112 0x4d 0xc2 0xff",
    "i2cset -f -y 112 0x4d 0xc3 0xff",
    "i2cset -f -y 112 0x4d 0xc4 0xff",
    "i2cset -f -y 112 0x4d 0xc5 0xff",
    "i2cset -f -y 112 0x4d 0xc6 0xff",
    "i2cset -f -y 112 0x4d 0xc7 0xff",
    "i2cset -f -y 112 0x4d 0xc8 0xff",
    "sleep 0.5",
    
    #MAC CPLD_U14 register: 
    #port 50, 49, 54, 53, 58, 57, 62, 61
    "i2cset -f -y 109 0x1d 0xc0 0x0",
    "i2cset -f -y 109 0x1d 0xc1 0x0",
    "i2cset -f -y 109 0x1d 0xc2 0x0",
    "i2cset -f -y 109 0x1d 0xc3 0x0",
    
    #MAC CPLD_U14 register:
    #port 2, 1, 6, 5, 10, 9, 14, 13, 18, 17, 22, 21, 26, 25, 30, 29, 34, 33, 37, 38, 42, 41, 46, 45
    "i2cset -f -y 110 0x2d 0xc0 0x0",
    "i2cset -f -y 110 0x2d 0xc1 0x0",
    "i2cset -f -y 110 0x2d 0xc2 0x0",
    "i2cset -f -y 110 0x2d 0xc3 0x0",
    "i2cset -f -y 110 0x2d 0xc4 0x0",
    "i2cset -f -y 110 0x2d 0xc5 0x0",
    "i2cset -f -y 110 0x2d 0xc6 0x0",
    "i2cset -f -y 110 0x2d 0xc7 0x0",
    "i2cset -f -y 110 0x2d 0xc8 0x0",
    "i2cset -f -y 110 0x2d 0xc9 0x0",
    "i2cset -f -y 110 0x2d 0xca 0x0",
    "i2cset -f -y 110 0x2d 0xcb 0x0",
    
    #PORT CPLD_U5 register
    #port:4, 3, 8, 7, 12, 11, 16, 15, 23, 20, 27, 24, 31, 28, 32
    "i2cset -f -y 111 0x3d 0xc0 0x0",
    "i2cset -f -y 111 0x3d 0xc1 0x0",
    "i2cset -f -y 111 0x3d 0xc2 0x0",
    "i2cset -f -y 111 0x3d 0xc3 0x0",
    "i2cset -f -y 111 0x3d 0xc4 0x0",
    "i2cset -f -y 111 0x3d 0xc5 0x0",
    "i2cset -f -y 111 0x3d 0xc6 0x0",
    "i2cset -f -y 111 0x3d 0xc7 0x0",
    
    #PORT CPLD_U9 register
    #port: 35, 19, 39, 36, 43, 40, 47, 44, 51, 48, 55, 52, 59, 56, 63, 60, 64
    "i2cset -f -y 112 0x4d 0xc0 0x0",
    "i2cset -f -y 112 0x4d 0xc1 0x0",
    "i2cset -f -y 112 0x4d 0xc2 0x0",
    "i2cset -f -y 112 0x4d 0xc3 0x0",
    "i2cset -f -y 112 0x4d 0xc4 0x0",
    "i2cset -f -y 112 0x4d 0xc5 0x0",
    "i2cset -f -y 112 0x4d 0xc6 0x0",
    "i2cset -f -y 112 0x4d 0xc7 0x0",
    "i2cset -f -y 112 0x4d 0xc8 0x0",
    ]

WARM_UPGRADE_PARAM = {
    "slot0": {
        "VME": {
            "chain1": [
                {"name": "BASE_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/base_cpld_u55_transf_header.vme",
                    "init_cmd": [
                        {"bus": 109, "loc": 0x1d, "offset": 0x1c, "value": 0xff, "gettype": "i2c"},
                        {"io_addr": 0x916, "value": 0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"io_addr": 0x918, "value": None, "gettype": "io"},
                        {"io_addr": 0x929, "value": None, "gettype": "io"},
                        {"io_addr": 0x940, "value": None, "gettype": "io"},
                        {"io_addr": 0x941, "value": None, "gettype": "io"},
                        {"io_addr": 0x942, "value": None, "gettype": "io"},
                        {"io_addr": 0x943, "value": None, "gettype": "io"},
                        {"io_addr": 0x944, "value": None, "gettype": "io"},
                        {"io_addr": 0x945, "value": None, "gettype": "io"},
                        {"io_addr": 0x946, "value": None, "gettype": "io"},
                        {"io_addr": 0x948, "value": None, "gettype": "io"},
                        {"io_addr": 0x949, "value": None, "gettype": "io"},
                        {"io_addr": 0x94a, "value": None, "gettype": "io"},
                        {"io_addr": 0x953, "value": None, "gettype": "io"},
                        {"io_addr": 0x954, "value": None, "gettype": "io"},
                        {"io_addr": 0x955, "value": None, "gettype": "io"},
                        {"io_addr": 0x958, "value": None, "gettype": "io"},
                        {"io_addr": 0x959, "value": None, "gettype": "io"},
                        {"io_addr": 0x960, "value": None, "gettype": "io"},
                        {"io_addr": 0x961, "value": None, "gettype": "io"},
                        {"io_addr": 0x962, "value": None, "gettype": "io"},
                        {"io_addr": 0x963, "value": None, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 30,
                    "after_upgrade_delay_timeout": 60,
                    "refresh_finish_flag_check": {"io_addr": 0x91d, "value": 0x5a, "gettype": "io"},
                    "access_check_reg": {"io_addr": 0x955, "value": 0xaa, "gettype": "io"},
                    "finish_cmd": [
                        {"bus": 109, "loc": 0x1d, "offset": 0x1c, "value": 0, "gettype": "i2c"},
                    ],
                 },
            ],

            "chain2": [
                {"name": "FAN_CPLD_1",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/ufan_cpld_u13_transf_header.vme",
                    "rw_recover_reg": [],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"bus": 92, "loc": 0x0d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                 },
            ],

            "chain3": [
                {"name": "FAN_CPLD_2",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/dfan_cpld_u13_transf_header.vme",
                    "rw_recover_reg": [],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"bus": 101, "loc": 0x0d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                 },
            ],

            "chain4": [
                {"name": "MAC_CPLD_1",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cpld_u14_transf_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x13, "value": 0xff, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x18, "value": 0x00, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 109, "loc": 0x1d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x12, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x16, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x18, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x1a, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x1b, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x1c, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x23, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x51, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x52, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x56, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x57, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x58, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x59, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x5a, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x70, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x72, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x73, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xc0, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xc1, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xc2, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xc3, "value": None, "gettype": "i2c"},
                        {"bus": 109, "loc": 0x1d, "offset": 0xc4, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 109, "loc": 0x1d, "offset": 0x18, "value": 0x01, "gettype": "i2c"},
                    "access_check_reg": {"bus": 109, "loc": 0x1d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"bus": 110, "loc": 0x2d, "offset": 0x13, "value": 0, "gettype": "i2c"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },

                {"name": "MAC_CPLD_2",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cpld_u30_transf_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"bus": 109, "loc": 0x1d, "offset": 0x1b, "value": 0xff, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x11, "value": 0x00, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 110, "loc": 0x2d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x52, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x53, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x56, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x57, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x58, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x59, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x5a, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x5b, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x5c, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x5d, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x5e, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x70, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x71, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x72, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x76, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x77, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x78, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x79, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x7a, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0x7b, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc0, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc1, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc2, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc3, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc4, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc5, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc6, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc7, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc8, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xc9, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xca, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xcb, "value": None, "gettype": "i2c"},
                        {"bus": 110, "loc": 0x2d, "offset": 0xcc, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 110, "loc": 0x2d, "offset": 0x11, "value": 0x01, "gettype": "i2c"},
                    "access_check_reg": {"bus": 110, "loc": 0x2d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"bus": 109, "loc": 0x1d, "offset": 0x1b, "value": 0, "gettype": "i2c"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },
            ],

            "chain5": [
                {"name": "PORT_CPLD_1",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/port_cpld_u5_transf_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x13, "value": 0xff, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x18, "value": 0x00, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 111, "loc": 0x3d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x16, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x18, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x1a, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x1b, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x51, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x53, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x56, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x57, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x58, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x59, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x5a, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x5b, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x5c, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x70, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x71, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x74, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x75, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x76, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x77, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc0, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc1, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc2, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc3, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc4, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc5, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc6, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc7, "value": None, "gettype": "i2c"},
                        {"bus": 111, "loc": 0x3d, "offset": 0xc8, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 111, "loc": 0x3d, "offset": 0x18, "value": 0x01, "gettype": "i2c"},
                    "access_check_reg": {"bus": 111, "loc": 0x3d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"bus": 112, "loc": 0x4d, "offset": 0x13, "value": 0, "gettype": "i2c"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },

                {"name": "PORT_CPLD_2",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/port_cpld_u9_transf_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"bus": 111, "loc": 0x3d, "offset": 0x1b, "value": 0xff, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x11, "value": 0x00, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 112, "loc": 0x4d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x50, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x52, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x53, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x56, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x57, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x58, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x59, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x5a, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x5b, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x5c, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x5d, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x5e, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x70, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x71, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x72, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x76, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x77, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x78, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x79, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x7a, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0x7b, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc0, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc1, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc2, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc3, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc4, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc5, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc6, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc7, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc8, "value": None, "gettype": "i2c"},
                        {"bus": 112, "loc": 0x4d, "offset": 0xc9, "value": None, "gettype": "i2c"},

                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 112, "loc": 0x4d, "offset": 0x11, "value": 0x01, "gettype": "i2c"},
                    "access_check_reg": {"bus": 112, "loc": 0x4d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"bus": 111, "loc": 0x3d, "offset": 0x1b, "value": 0, "gettype": "i2c"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },
            ],

            "chain6": [
                {"name": "CPU_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_cpu_cpld_header.vme",
                    "init_cmd": [
                        {"cmd": "echo 7 > /sys/class/gpio/export", "gettype": "cmd"},
                        {"cmd": "echo high > /sys/class/gpio/gpio7/direction", "gettype": "cmd"},
                        {"io_addr": 0x7cc, "value": 0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"io_addr": 0x705, "value": None, "gettype": "io"},
                        {"io_addr": 0x713, "value": None, "gettype": "io"},
                        {"io_addr": 0x715, "value": None, "gettype": "io"},
                        {"io_addr": 0x721, "value": None, "gettype": "io"},
                        {"io_addr": 0x722, "value": None, "gettype": "io"},
                        {"io_addr": 0x772, "value": None, "gettype": "io"},
                        {"io_addr": 0x774, "value": None, "gettype": "io"},
                        {"io_addr": 0x776, "value": None, "gettype": "io"},
                        {"io_addr": 0x778, "value": None, "gettype": "io"},
                        {"io_addr": 0x77a, "value": None, "gettype": "io"},
                        {"io_addr": 0x77c, "value": None, "gettype": "io"},
                        {"io_addr": 0x780, "value": None, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"io_addr": 0x705, "value": 0x5a, "gettype": "io"},
                    "finish_cmd": [
                        {"io_addr": 0x7cc, "value": 0xff, "gettype": "io"},
                        {"cmd": "echo 0 > /sys/class/gpio/gpio7/value", "gettype": "cmd"},
                        {"cmd": "echo 7 > /sys/class/gpio/unexport", "gettype": "cmd"},
                    ],
                },
            ],
        },

        "MTD": {
            "chain1": [
                {"name": "MAC_FPGA",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"cmd": "setpci -s 00:03.2 0xA0.W=0x0050", "gettype": "cmd"}, # link_disable
                        {"io_addr": 0x948, "value": 0x0, "gettype": "io"},
                        {"bus": 58, "loc": 0x1c, "offset": 0x23, "value": 0x00, "gettype": "i2c"},
                        {"bus": 58, "loc": 0x1c, "offset": 0x23, "value": 0x01, "gettype": "i2c", "delay": 0.1},
                    ],
                    "after_upgrade_delay": 10,
                    "after_upgrade_delay_timeout": 180,
                    "refresh_finish_flag_check": {"bus": 58, "loc": 0x1c, "offset": 0x23, "value": 0x07, "gettype": "i2c"},
                    "access_check_reg": {
                        "path": "/dev/fpga0", "offset": 0x8, "value": [0x55, 0xaa, 0x5a, 0xa5], "read_len":4, "gettype":"devfile",
                        "polling_cmd":[
                            {"cmd": "setpci -s 00:03.2 0xA0.W=0x0060", "gettype": "cmd"}, # retrain_link
                            {"cmd": "rmmod wb_fpga_pcie", "gettype": "cmd"},
                            {"cmd": "modprobe wb_fpga_pcie", "gettype": "cmd", "delay": 0.1},
                        ],
                        "polling_delay": 0.1
                    },
                    "finish_cmd": [
                        {"cmd": "setpci -s 00:03.2 0xA0.W=0x0060", "gettype": "cmd"}, # retrain_link
                        {"io_addr": 0x948, "value": 0x1, "gettype": "io"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },
            ],
            "chain2": [
                {"name": "PORT_FPGA",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"io_addr": 0x948, "value": 0x0, "gettype": "io"},
                        {"bus": 66, "loc": 0x3c, "offset": 0x21, "value": 0x00, "gettype": "i2c"},
                        {"bus": 66, "loc": 0x3c, "offset": 0x21, "value": 0x01, "gettype": "i2c", "delay": 0.1},
                    ],
                    "after_upgrade_delay": 10,
                    "after_upgrade_delay_timeout": 180,
                    "refresh_finish_flag_check": {"bus": 66, "loc": 0x3c, "offset": 0x21, "value": 0x07, "gettype": "i2c"},
                    "access_check_reg": {
                        "path": "/dev/fpga1", "offset": 0x8, "value": [0x55, 0xaa, 0x5a, 0xa5], "read_len":4, "gettype":"devfile",
                        "polling_cmd":[
                            {"cmd": "rmmod wb_fpga_pcie", "gettype": "cmd"},
                            {"cmd": "modprobe wb_fpga_pcie", "gettype": "cmd", "delay": 0.1},
                        ],
                        "polling_delay": 0.1
                    },
                    "finish_cmd": [
                        {"io_addr": 0x948, "value": 0x1, "gettype": "io"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },
            ],
        },
    },
    "stop_services_cmd": [
        "/usr/local/bin/platform_process.py stop",
    ],
    "start_services_cmd": [
        "/usr/local/bin/platform_process.py start",
    ],
}

UPGRADE_SUMMARY = {
    "devtype": 0x20000055,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain2": {
                "name": "FANA_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain3": {
                "name": "FANB_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain4": {
                "name": "MAC_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain5": {
                "name": "PORT_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain6": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 1,
            },
        },

        "MTD": {
            "chain1": {
                "name": "MAC_FPGA",
                "is_support_warm_upg": 1,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=65 miso=32 mosi=67 bus=0", "gettype": "cmd"},
                    {"cmd": "echo 50 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio50/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 48 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio48/direction", "gettype": "cmd", "delay": 0.1},
                    {"io_addr": 0x918, "value": 0x2, "gettype": "io"},
                    {"io_addr": 0x946, "value": 0xfe, "gettype": "io"},
                    {"cmd": "modprobe wb_spi_nor_device spi_bus_num=0", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"io_addr": 0x946, "value": 0xff, "gettype": "io"},
                    {"io_addr": 0x918, "value": 0x0, "gettype": "io"},
                    {"cmd": "echo 0 > /sys/class/gpio/gpio48/value", "gettype": "cmd"},
                    {"cmd": "echo 48 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 0 > /sys/class/gpio/gpio50/value", "gettype": "cmd"},
                    {"cmd": "echo 50 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
            "chain2": {
                "name": "PORT_FPGA",
                "is_support_warm_upg": 1,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=65 miso=32 mosi=67 bus=0", "gettype": "cmd"},
                    {"cmd": "echo 50 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio50/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 48 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio48/direction", "gettype": "cmd", "delay": 0.1},
                    {"io_addr": 0x918, "value": 0x3, "gettype": "io"},
                    {"io_addr": 0x946, "value": 0xfb, "gettype": "io"},
                    {"cmd": "modprobe wb_spi_nor_device spi_bus_num=0", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"io_addr": 0x946, "value": 0xff, "gettype": "io"},
                    {"io_addr": 0x918, "value": 0x0, "gettype": "io"},
                    {"cmd": "echo 0 > /sys/class/gpio/gpio48/value", "gettype": "cmd"},
                    {"cmd": "echo 48 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 0 > /sys/class/gpio/gpio50/value", "gettype": "cmd"},
                    {"cmd": "echo 50 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
            "chain4": {
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
                    {"cmd": "rmmod intel_spi_platform", "gettype": "cmd"},
                    {"cmd": "rmmod intel_spi", "gettype": "cmd"},
                    {"cmd": "rmmod ofpart", "gettype": "cmd"},
                    {"cmd": "rmmod spi_nor", "gettype": "cmd"},
                    {"cmd": "rmmod mtd", "gettype": "cmd"},
                ],
            },
        },
        
        "SYSFS": {
            "chain3": {
                "name": "BCM5387",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=65 miso=32 mosi=67 bus=0", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46 spi_bus_num=0 spi_cs_gpio=6", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },
        
        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/fpga_test_0_1_header.bin", "display_name": "MAC_FPGA"},
                {"chain": 2, "file": "/etc/.upgrade_test/fpga_test_0_2_header.bin", "display_name": "PORT_FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_0_1_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/cpld_test_0_2_header.vme", "display_name": "FANA_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/cpld_test_0_3_header.vme", "display_name": "FANB_CPLD"},
                {"chain": 4, "file": "/etc/.upgrade_test/cpld_test_0_4_header.vme", "display_name": "MAC_CPLD"},
                {"chain": 5, "file": "/etc/.upgrade_test/cpld_test_0_5_header.vme", "display_name": "PORT_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/cpld_test_0_6_header.vme", "display_name": "CPU_CPLD"},
            ],
        },
    },

    "BMC": {
        "name": "BMC",
        "init_cmd": [
            {"cmd": "ipmitool raw 0x32 0x03 0x02", "gettype": "cmd", "ignore_result": 1},
        ],
        "finish_cmd": [],
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/95-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/104-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/96-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/105-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/97-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/106-0050/eeprom"},
        {"name": "fan7", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/98-0050/eeprom"},
        {"name": "fan8", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/107-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/83-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/84-0050/eeprom"},
        {"name": "psu3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/86-0050/eeprom"},
        {"name": "psu4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/85-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}
