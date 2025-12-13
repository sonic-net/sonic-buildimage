#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 0,
    "dev_monitor": 1,
    "tty_console": 0,
    "reboot_cause": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 1,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 81, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "24c02", "bus": 81, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 82, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "24c02", "bus": 82, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c64", "bus": 75, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c64", "bus": 74, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "24c64", "bus": 73, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "24c64", "bus": 72, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "24c64", "bus": 71, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "24c64", "bus": 70, "loc": 0x50, "attr": "eeprom"},
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
                {"id": "lm75_1", "name": "lm75", "bus": 76, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 76, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 79, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 80, "loc": 0x4e, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 80, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_td4", "bus": 84, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name":"tmp411",
            "device":[
                {"id":"tmp411_1", "name":"tmp411","bus":79, "loc":0x4c, "attr":"hwmon"},
                {"id":"tmp411_2", "name":"tmp411","bus":80, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "ina3221", "bus": 65, "loc": 0x43, "attr": "hwmon"},
            ],
        },
        {
            "name": "tps53622",
            "device": [
                {"id": "tps53622_1", "name": "tps53688", "bus": 65, "loc": 0x67, "attr": "hwmon"},
                {"id": "tps53622_2", "name": "tps53688", "bus": 65, "loc": 0x6c, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 64, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 85, "loc": 0x5b, "attr": "hwmon"},
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
        "config": "CONNECT_BOARD_CPLD",
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
        "config": "FAN_CPLD",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "4",
            "loc": "0x3d",
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
        "config": "MAC_CPLDA",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "2",
            "loc": "0x1d",
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
        "config": "MAC_CPLDB",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "2",
            "loc": "0x2d",
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
        "config": "MAC_CPLDC",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "i2c": {
            "bus": "2",
            "loc": "0x3d",
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
    "fpga_model": {
        "parent": "fpga",
        "config": "XC7A100T-2FGG484C",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "fpga_vendor": {
        "parent": "fpga",
        "config": "XILINX",
        "key": "Vendor",
        "arrt_index": 2,
    },
    "fpga_desc": {
        "parent": "fpga",
        "config": "NA",
        "key": "Description",
        "arrt_index": 3,
    },
    "fpga_hw_version": {
        "parent": "fpga",
        "config": "NA",
        "key": "Hardware Version",
        "arrt_index": 4,
    },
    "fpga_fw_version": {
        "parent": "fpga",
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
    "fpga_date": {
        "parent": "fpga",
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
        }
    },
    "fans": {
        "present": {"path": ["/sys/wb_plat/fan/*/present"], "ABSENT": 0},
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
            "psu2": "PSU2"
        }
    }
}

##################### MAC Voltage adjust####################################
MAC_DEFAULT_PARAM = [
    {
        "name": "mac_core",              # AVS name
        "type": 1,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x82,                 # default value, if rov value not in range
        "sdkreg": "TOP_AVS_SEL_REG",     # SDK register name
        "sdktype": 0,                    # 0: No shift operation required, 1: shift operation required
        "macregloc": 24,                 # Shift right 24 bits
        "mask": 0xff,                    # Use with macregloc
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"bus":2, "loc":0x2d, "offset":0x3f, "gettype":"i2c"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/83-005b/avs_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x72:0.90000,
            0x73:0.89375,
            0x74:0.88750,
            0x75:0.88125,
            0x76:0.87500,
            0x77:0.86875,
            0x78:0.86250,
            0x79:0.85625,
            0x7a:0.85000,
            0x7b:0.84375,
            0x7c:0.83750,
            0x7d:0.83125,
            0x7e:0.82500,
            0x7f:0.81875,
            0x80:0.81250,
            0x81:0.80625,
            0x82:0.80000,
            0x83:0.79375,
            0x84:0.78750,
            0x85:0.78125,
            0x86:0.77500,
            0x87:0.76875,
            0x88:0.76250,
            0x89:0.75625,
            0x8A:0.75000,
            0x8B:0.74375,
            0x8C:0.73750,
            0x8D:0.73125,
            0x8E:0.72500,
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
    {"name": "platform_common dfd_my_type=0x4101", "delay": 0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
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
    {"name": "hw_test", "delay": 0},
]

DEVICE = [
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "wb_mac_bsc_td4", "bus": 84, "loc": 0x44},
    # fan
    {"name": "24c64", "bus": 70, "loc": 0x50},
    {"name": "24c64", "bus": 71, "loc": 0x50},
    {"name": "24c64", "bus": 72, "loc": 0x50},
    {"name": "24c64", "bus": 73, "loc": 0x50},
    {"name": "24c64", "bus": 74, "loc": 0x50},
    {"name": "24c64", "bus": 75, "loc": 0x50},
    # psu
    {"name": "24c02", "bus": 81, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 81, "loc": 0x58},
    {"name": "24c02", "bus": 82, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 82, "loc": 0x58},
    # temp
    {"name": "lm75", "bus": 76, "loc": 0x48},
    {"name": "lm75", "bus": 76, "loc": 0x49},
    {"name": "lm75", "bus": 79, "loc": 0x4b},
    {"name": "tmp411", "bus": 79, "loc": 0x4c},
    {"name": "tmp411", "bus": 80, "loc": 0x4c},
    {"name": "lm75", "bus": 80, "loc": 0x4e},
    {"name": "lm75", "bus": 80, "loc": 0x4f},
    # dcdc
    {"name": "ucd90160", "bus": 64, "loc": 0x5b},
    {"name": "ucd90160", "bus": 85, "loc": 0x5b},
    {"name": "ina3221", "bus": 65, "loc": 0x43},
    {"name": "tps53688", "bus": 65, "loc": 0x67},
    {"name": "tps53688", "bus": 65, "loc": 0x6c},
    #avs
    {"name": "wb_xdpe132g5c", "bus": 83, "loc": 0x5b},
]

OPTOE = [
    {"name": "optoe3", "startbus": 6, "endbus": 61},
]

REBOOT_CTRL_PARAM = {
    "cpu": {"io_addr": 0x920, "rst_val": 0xfe, "rst_delay": 0, "gettype": "io"},
    "mac": {"bus": 2, "loc": 0x1d, "offset": 0x20, "rst_val": 0xfd, "rst_delay": 0, "gettype": "i2c"},
}

# INIT_PARAM_PRE = [
#     {"loc": "43-005b/avs_vout_max", "value": "900000"},
#     {"loc": "43-005b/avs_vout_min", "value": "725000"},
# ]

INIT_PARAM = []

INIT_COMMAND_PRE = []

INIT_COMMAND = [
    # sfp power enable
    "i2cset -f -y 2 0x2d 0x4c 0xff",
    "i2cset -f -y 2 0x2d 0x4d 0xff",
    "i2cset -f -y 2 0x2d 0x35 0xff",
    "i2cset -f -y 2 0x2d 0x36 0xff",
    "i2cset -f -y 2 0x1d 0x39 0xff",
    "i2cset -f -y 2 0x1d 0x3a 0xff",
    "i2cset -f -y 2 0x1d 0x3b 0xff",
    "i2cset -f -y 2 0x3d 0x38 0xff",
    "i2cset -f -y 2 0x3d 0x39 0xff",
    "i2cset -f -y 2 0x3d 0x3a 0xff",
    "i2cset -f -y 2 0x3d 0x3b 0xff",
    # led  enable
    "i2cset -f -y 2 0x2d 0x3a 0xff",
    "i2cset -f -y 2 0x1d 0x3c 0xff"
    "i2cset -f -y 2 0x3d 0x3c 0xff"
]

WARM_UPGRADE_PARAM = {}

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "cold_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x988, "okval": 0},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Power Loss, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size": 1 * 1024 * 1024}
            ]
        },
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x987, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", 
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size":1*1024*1024}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x986, "value": 0x01},
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

UPGRADE_SUMMARY = {}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/75-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/74-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/73-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/72-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/71-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/70-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/81-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/82-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}

AIR_FLOW_CONF = {
    "psu_fan_airflow": {
        "intake": ['DPS-1300AB-6 S', 'GW-CRPS1300D'],
        "exhaust": ['CRPS1300D3R', 'DPS-1300AB-11 C']
    },

    "fanairflow": {
        "intake": ['M1HFAN II-F'],
        "exhaust": ['M1HFAN IV-R']
    },

    "fans": [
        {
            "name": "FAN1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-75/75-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-74/74-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-73/73-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-72/72-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-71/71-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/i2c-70/70-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
    ],

    "psus": [
        {
            "name": "PSU1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/81-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        },
        {
            "name": "PSU2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/82-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        }
    ]
}
