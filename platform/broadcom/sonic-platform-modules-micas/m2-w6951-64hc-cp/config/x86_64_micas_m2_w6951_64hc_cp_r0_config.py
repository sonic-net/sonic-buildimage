#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "tty_console": 1,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "reboot_cause": 1,
    "generate_airflow": 0,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "direct_config", "value": 0, "okval": 0},
             "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 53, "loc": 0x60, "attr": "hwmon"}
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "direct_config", "value": 0, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 54, "loc": 0x60, "attr": "hwmon"}
            ],
        },
        {
            "name": "psu3",
            "present": {"gettype": "direct_config", "value": 0, "okval": 0},
            "device": [
                {"id": "psu3pmbus", "name": "wb_fsp1200", "bus": 55, "loc": 0x60, "attr": "hwmon"}
            ],
        },
        {
            "name": "psu4",
            "present": {"gettype": "direct_config", "value": 0, "okval": 0},
            "device": [
                {"id": "psu4pmbus", "name": "wb_fsp1200", "bus": 56, "loc": 0x60, "attr": "hwmon"}
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "wb_24c64", "bus": 59, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "wb_24c64", "bus": 60, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "wb_24c64", "bus": 61, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "wb_24c64", "bus": 62, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "wb_24c64", "bus": 63, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xda, "read_len":1, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "wb_24c64", "bus": 64, "loc": 0x50, "attr": "eeprom"},
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
                {"id": "lm75_1", "name": "lm75", "bus": 65, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 66, "loc": 0x4e, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 76, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 77, "loc": 0x4f, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 111, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_6", "name": "lm75", "bus": 112, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name":"ct7318",
            "device":[
                {"id":"ct7318_1", "name":"ct7318","bus":78, "loc":0x4c, "attr":"hwmon"},
                {"id":"ct7318_2", "name":"ct7318","bus":79, "loc":0x4c, "attr":"hwmon"},
                {"id":"ct7318_3", "name":"ct7318","bus":80, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 69, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 70, "loc": 0x5f, "attr": "hwmon"},
                {"id": "ucd90160_3", "name": "ucd90160", "bus": 83, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_4", "name": "ucd90160", "bus": 84, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_5", "name": "ucd90160", "bus": 73, "loc": 0x68, "attr": "hwmon"},
            ],
        },
        {
            "name": "ads7828",
            "device": [
                {"id": "ads7828_1", "name": "ads7828", "bus": 97, "loc": 0x48, "attr": "hwmon"},
                {"id": "ads7828_2", "name": "ads7828", "bus": 97, "loc": 0x4A, "attr": "hwmon"},
                {"id": "ads7828_3", "name": "ads7828", "bus": 13, "loc": 0x48, "attr": "hwmon"},
                {"id": "ads7828_4", "name": "ads7828", "bus": 14, "loc": 0x48, "attr": "hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "ina3221", "bus": 98, "loc": 0x40, "attr": "hwmon"},
            ],
        },
        {
            "name": "isl68137",
            "device": [
                {"id": "raa228228", "name": "isl68137", "bus": 85, "loc": 0x60, "attr": "hwmon"},
                {"id": "raa228228_1", "name": "isl68137", "bus": 70, "loc": 0x72, "attr": "hwmon"},
                {"id": "raa228228_2", "name": "isl68137", "bus": 70, "loc": 0x74, "attr": "hwmon"},
                {"id": "isl69260_1", "name": "isl68137", "bus": 85, "loc": 0x62, "attr": "hwmon"},
                {"id": "isl69260_2", "name": "isl68137", "bus": 85, "loc": 0x74, "attr": "hwmon"},
                {"id": "isl69260_3", "name": "isl68137", "bus": 87, "loc": 0x60, "attr": "hwmon"},
                {"id": "isl69260_4", "name": "isl68137", "bus": 87, "loc": 0x62, "attr": "hwmon"},
                {"id": "isl69260_5", "name": "isl68137", "bus": 87, "loc": 0x74, "attr": "hwmon"},
                {"id": "isl69260_6", "name": "isl68137", "bus": 89, "loc": 0x74, "attr": "hwmon"},
                {"id": "isl69260_7", "name": "isl68137", "bus": 89, "loc": 0x60, "attr": "hwmon"},
                {"id": "isl69260_8", "name": "isl68137", "bus": 90, "loc": 0x60, "attr": "hwmon"},
                {"id": "isl69260_9", "name": "isl68137", "bus": 91, "loc": 0x62, "attr": "hwmon"},
                {"id": "isl69260_10", "name": "isl68137", "bus": 91, "loc": 0x60, "attr": "hwmon"},
                {"id": "isl69260_11", "name": "isl68137", "bus": 92, "loc": 0x60, "attr": "hwmon"},
                {"id": "isl69260_12", "name": "isl68137", "bus": 92, "loc": 0x62, "attr": "hwmon"},
                {"id": "isl69260_13", "name": "isl68137", "bus": 95, "loc": 0x60, "attr": "hwmon"},
            ],
        },
    ],
}

MANUINFO_CONF = {   #tocheck
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
        "devfile": {
            "loc": "/dev/cpld0",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "LCMXO3LF-4300C-6BG324I",
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
        "config": "BASE_CPLD",
        "arrt_index": 3,
    },
    "cpld2_version": {
        "key": "Firmware Version",
        "parent": "cpld2",
         "devfile": {
            "loc": "/dev/cpld1",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "MAC_CPLDA",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "devfile": {
            "loc": "/dev/cpld6",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "MAC_CPLDB",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "devfile": {
            "loc": "/dev/cpld7",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "LCMXO3LF-4300C-6BG324I",
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
        "config": "IO_CPLD",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "devfile": {
            "loc": "/dev/cpld8",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "FAN_CPLD",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "devfile": {
            "loc": "/dev/cpld10",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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

    "e610": {
        "key": "NIC",
        "next": "fpga"
    },
    "i210_model": {
        "parent": "e610",
        "config": "NA",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "i210_vendor": {
        "parent": "e610",
        "config": "INTEL",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "i210_version": {
        "parent": "e610",
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
            "00000050": "XC7A50T_2FGG484C",
            "00000100": "XC7A100T_2FGG484C"
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
            "00000050": "XC7A50T_2FGG484C",
            "00000100": "XC7A100T_2FGG484C"
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
        "config": "IO_FPGA",
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
                    # OE high
                    {"gettype": "cmd", "cmd": "echo 10051 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio10051/direction"},
                    # SEL1 high
                    {"gettype": "cmd", "cmd": "echo 10052 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio10052/direction"},
                    #enable 53134 update
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe9, "value": 0x16},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe1, "value": 0x01},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe2, "value": 0x05},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=55  mosi=54 miso=52 cs=53 bus=0 gpio_chip_name=INTC3001:00"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio10052/value"},
                    {"gettype": "cmd", "cmd": "echo 10052 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio10051/value"},
                    {"gettype": "cmd", "cmd": "echo 10051 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe2, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe1, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe9, "value": 0x17},
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
            "fan6": "FAN6"
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
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x3a, "read_len":1, "okval":0x01},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x3a, "read_len":1, "okval":0x07},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x3a, "read_len":1, "okval":0x08},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "cpu_reboot",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x3a, "read_len":1, "okval":0x06},
            "record": [
                {"record_type":"file", "mode":"cover", "log":"CPU reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "CPU reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
        },
        {
            "name": "bmc_powerdown",
            "monitor_point": {"gettype":"devfile", "path":"/dev/cpld1", "offset":0x3a, "read_len":1, "okval":[0x04, 0x02, 0x09]},
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
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x8a,                 # default value, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"path": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs1", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/85-0060/hwmon/hwmon*/avs0_vout", "gettype": "sysfs", "formula": "int((%f)*1000000)"},
        "mac_avs_param": {
            0x8c: 0.7375,
            0x8a: 0.75,
            0x88: 0.7625,
            0x86: 0.775,
            0x84: 0.7875,
            0x82: 0.8,
            0x80: 0.8125,
            0x7f: 0.825
        }
    },
    {
        "name": "MAC_PT0_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs2", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT1_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs3", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT2_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs4", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT3_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs5", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT4_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs6", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT5_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs7", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT6_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs8", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
    {
        "name": "MAC_PT7_VDDC_V",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default" : 0x8e,
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        # bsc unsupport, use default value 725000
        "cpld_avs": {"loc": "/sys/bus/i2c/devices/75-0044/hwmon/hwmon*/avs9", "gettype": "sysfs"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs1_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x8e : 0.725,
        },
    },
]

DRIVERLISTS = [
    {"name": "wb_ixgbe", "delay": 0, "removable": 0},
    {"name": "ice", "delay": 0, "removable": 0},
    {"name": "ads7828", "delay": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "wb_i2c_designware_core", "delay": 0},  #temp, driver has not moved
    {"name": "wb_i2c_designware_platform sda_fall_ns=320 scl_fall_ns=1006 bus_freq_hz=100000", "delay": 0}, #temp, driver has not moved
    {"name": "i2c_algo_bit", "delay": 0},
    {"name": "i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=116 gpio_scl=115 gpio_chip_name=AMDI0030:00 bus_num=21", "delay": 0},
    {"name": "platform_common dfd_my_type=0x414c", "delay": 0},
    {"name": "wb_logic_dev_common", "delay":0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    #{"name": "wb_io_dev", "delay": 0},
    #{"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_spi_ocores", "delay": 0},
    {"name": "wb_spi_ocores_device", "delay": 0},
    {"name": "wb_spi_dev_device", "delay": 0},
    {"name": "wb_indirect_dev", "delay": 0},
    {"name": "wb_indirect_dev_device", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "i2c_piix4", "delay": 1},
    {"name": "mdio_bitbang", "delay": 0},
    {"name": "mdio_gpio", "delay": 0},
    {"name": "wb_mdio_gpio_device gpio_mdc=131 gpio_mdio=132 gpio_chip_name=AMDI0030:00", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    {"name": "lm75", "delay": 0},
    {"name": "tmp401", "delay": 0},
    {"name": "ct7148", "delay": 0},
    {"name": "wb_rc32312", "delay": 0},
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "ina238", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "ina3221", "delay": 0},
    {"name": "tps53679", "delay": 0},
    {"name": "ucd9000", "delay": 0},
    {"name": "wb_ucd9081", "delay": 0},
    {"name": "xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay":0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "hw_test", "delay": 0},
    #{"name": "firmware_driver_cpld", "delay": 0},
    #{"name": "firmware_driver_ispvme", "delay": 0},
    #{"name": "firmware_driver_sysfs", "delay": 0},
    #{"name": "wb_firmware_upgrade_device", "delay": 0},
    {"name": "isl68137", "delay": 0},
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
    {"name": "curr_sensor_device_driver", "delay": 0},
    {"name": "fpga_device_driver", "delay": 0},
    {"name": "watchdog_device_driver", "delay": 0},
]

DEVICE = [
    # SYS E2
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "24c02", "bus": 1, "loc": 0x57},
    # IO board E2
    {"name": "24c02", "bus": 12, "loc": 0x54},
    # FAN board E2
    {"name": "24c02", "bus": 427, "loc": 0x57},
    # BASE board E2
    {"name": "24c02", "bus": 67, "loc": 0x57},
    # MAC board E2
    {"name": "24c02", "bus": 105, "loc": 0x57},
    # fan
    {"name": "24c64", "bus": 59, "loc": 0x50},
    {"name": "24c64", "bus": 60, "loc": 0x50},
    {"name": "24c64", "bus": 61, "loc": 0x50},
    {"name": "24c64", "bus": 62, "loc": 0x50},
    {"name": "24c64", "bus": 63, "loc": 0x50},
    {"name": "24c64", "bus": 64, "loc": 0x50},
    # psu
    {"name": "wb_fsp1200", "bus": 53, "loc": 0x60},
    {"name": "wb_fsp1200", "bus": 54, "loc": 0x60},
    {"name": "wb_fsp1200", "bus": 55, "loc": 0x60},
    {"name": "wb_fsp1200", "bus": 56, "loc": 0x60},
    # temp
    {"name": "lm75", "bus": 65, "loc": 0x4b},
    {"name": "lm75", "bus": 66, "loc": 0x4e},
    {"name": "lm75", "bus": 76, "loc": 0x4b},
    {"name": "lm75", "bus": 77, "loc": 0x4f},
    {"name": "lm75", "bus": 111, "loc": 0x4b},
    {"name": "lm75", "bus": 112, "loc": 0x4f},
    {"name": "ct7318", "bus": 78, "loc": 0x4c},
    {"name": "ct7318", "bus": 79, "loc": 0x4c},
    {"name": "ct7318", "bus": 80, "loc": 0x4c},
    #dcdc
    {"name": "ucd90160", "bus": 69, "loc": 0x5b},
    {"name": "ucd90160", "bus": 70, "loc": 0x5f},
    {"name": "ucd90160", "bus": 73, "loc": 0x68},
    {"name": "ucd90160", "bus": 83, "loc": 0x5b},
    {"name": "ucd90160", "bus": 84, "loc": 0x5b},
    {"name": "wb_ina238", "bus": 57, "loc": 0x40},
    {"name": "wb_ina238", "bus": 58, "loc": 0x41},
    {"name": "ads7828", "bus": 97, "loc": 0x48},
    {"name": "ads7828", "bus": 97, "loc": 0x4a},
    {"name": "ina3221", "bus": 98, "loc": 0x40},
    {"name": "raa228228", "bus": 70, "loc": 0x72},
    {"name": "raa228228", "bus": 70, "loc": 0x74},
    {"name": "raa228228", "bus": 85, "loc": 0x60},
    {"name": "isl69260", "bus": 85, "loc": 0x62},
    {"name": "isl69260", "bus": 85, "loc": 0x74},
    {"name": "isl69260", "bus": 87, "loc": 0x60},
    {"name": "isl69260", "bus": 87, "loc": 0x62},
    {"name": "isl69260", "bus": 87, "loc": 0x74},
    {"name": "isl69260", "bus": 89, "loc": 0x74},
    {"name": "isl69260", "bus": 89, "loc": 0x60},
    {"name": "isl69260", "bus": 90, "loc": 0x60},
    {"name": "isl69260", "bus": 91, "loc": 0x60},
    {"name": "isl69260", "bus": 91, "loc": 0x62},
    {"name": "isl69260", "bus": 92, "loc": 0x60},
    {"name": "isl69260", "bus": 92, "loc": 0x62},
    {"name": "isl69260", "bus": 95, "loc": 0x60},
    {"name": "ads7828", "bus": 13, "loc": 0x48},
    {"name": "ads7828", "bus": 14, "loc": 0x48},

    #avs
    {"name": "wb_mac_bsc_th6", "bus": 75, "loc": 0x44},
    {"name": "wb_rc32312", "bus": 103, "loc": 0x09},
    {"name": "wb_rc32312", "bus": 104, "loc": 0x09},
]

OPTOE = [
    {"name": "optoe1", "startbus": 265, "endbus": 265},
    {"name": "optoe3", "startbus": 201, "endbus": 264},
]


INIT_PARAM = []

INIT_COMMAND_PRE = []

INIT_COMMAND = [
    # open X86 BMC Serial port
    "dfd_debug sysfs_data_wr /dev/cpld1 0xA9 0x01",
    # QSFP EN
    "dfd_debug sysfs_data_wr /dev/cpld7 0x40 0xbf",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x41 0xbf",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x42 0xbd",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x43 0xbd",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x44 0xbb",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x45 0xbb",
    # KR power_on
    "dfd_debug sysfs_data_wr /dev/cpld7 0x50 0xaf",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x51 0xaf",
    # KR tx-disable enable
    "dfd_debug sysfs_data_wr /dev/cpld7 0x52 0xac",
    "dfd_debug sysfs_data_wr /dev/cpld7 0x53 0xac",

    "echo 580000 > /sys/bus/i2c/devices/85-0060/hwmon/hwmon*/avs0_vout_min",
    "echo 920000 > /sys/bus/i2c/devices/85-0060/hwmon/hwmon*/avs0_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs0_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs0_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs1_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/87-0060/hwmon/hwmon*/avs1_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs0_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs0_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs1_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/89-0060/hwmon/hwmon*/avs1_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs0_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs0_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs1_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/90-0060/hwmon/hwmon*/avs1_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs0_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs0_vout_max",
    "echo 600000 > /sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs1_vout_min",
    "echo 880000 > /sys/bus/i2c/devices/95-0060/hwmon/hwmon*/avs1_vout_max",
]

UPGRADE_SUMMARY = {
    "devtype": 0x414c,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain2": {
                "name": "MAC_CPLDA",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain3": {
                "name": "MAC_CPLDB",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain4": {
                "name": "IO_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain5": {
                "name": "FAN_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                ],
            },
            "chain6": {
                "name": "CPU_CPLD",
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
            "chain3": {
                "name": "IO_FPGA",
                "is_support_warm_upg": 0,
            }
        },

        "SYSFS": {
            "chain2": {
                "name": "BCM53134",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=7  mosi=5 miso=6 cs=8 bus=0 gpio_chip_name=AMDI0030:00", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x9d, "value": 0xa4},
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/fpga_test_0_1_header.bin", "display_name": "MAC_FPGA"},
                {"chain": 3, "file": "/etc/.upgrade_test/fpga_test_0_3_header.bin", "display_name": "IO_FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_0_1_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/cpld_test_0_2_header.vme", "display_name": "MAC_CPLDA"},
                {"chain": 3, "file": "/etc/.upgrade_test/cpld_test_0_3_header.vme", "display_name": "MAC_CPLDB"},
                {"chain": 4, "file": "/etc/.upgrade_test/cpld_test_0_4_header.vme", "display_name": "IO_CPLD"},
                {"chain": 5, "file": "/etc/.upgrade_test/cpld_test_0_5_header.vme", "display_name": "FAN_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/cpld_test_0_6_header.vme", "display_name": "CPU_CPLD"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/59-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/60-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/61-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/62-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/63-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/64-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
    "io": [
        {"name": "IO card", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/12-0054/eeprom"},
    ],
    "fcb": [
        {"name": "FAN board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/427-0057/eeprom"},
    ],
    "base": [
        {"name": "BASE board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/67-0057/eeprom"},
    ],
    "mac": [
        {"name": "MAC board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/105-0057/eeprom"},
    ],
}

AIR_FLOW_CONF = {
}
