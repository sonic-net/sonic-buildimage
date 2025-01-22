#!/usr/bin/python3
from ruijiecommon import *


STARTMODULE = {
    "xdpe_avscontrol":1,
    "dev_monitor": 1,
    "hal_fanctrl":1,
    "hal_ledctrl":1,
    "sff_temp_polling":1,
    "rg_pmon_syslog":1,
}

DEV_MONITOR_PARAM = {
    "polling_time" : 10,
    "psus": [
        {"name": "psu1",
         "present": {"gettype": "io", "io_addr": 0x964, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "psu1pmbus", "name": "rg_fsp1200", "bus": 79, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu1frue2", "name": "24c02", "bus": 79, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu2",
         "present": {"gettype": "io", "io_addr": 0x964, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "psu2pmbus", "name": "rg_fsp1200", "bus": 80, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu2frue2", "name": "24c02", "bus": 80, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu3",
         "present": {"gettype": "io", "io_addr": 0x965, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "psu3pmbus", "name": "rg_fsp1200", "bus": 82, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu3frue2", "name": "24c02", "bus": 82, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu4",
         "present": {"gettype": "io", "io_addr": 0x965, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "psu4pmbus", "name": "rg_fsp1200", "bus": 81, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu4frue2", "name": "24c02", "bus": 81, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "fans": [
        {"name": "fan1",
         "present": {"gettype": "i2c", "bus": 87, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan1frue2", "name": "24c64", "bus": 90, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan2",
         "present": {"gettype": "i2c", "bus": 95, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan2frue2", "name": "24c64", "bus": 98, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan3",
         "present": {"gettype": "i2c", "bus": 87, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan3frue2", "name": "24c64", "bus": 91, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan4",
         "present": {"gettype": "i2c", "bus": 95, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan4frue2", "name": "24c64", "bus": 99, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan5",
         "present": {"gettype": "i2c", "bus": 87, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan5frue2", "name": "24c64", "bus": 92, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan6",
         "present": {"gettype": "i2c", "bus": 95, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan6frue2", "name": "24c64", "bus": 100, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan7",
         "present": {"gettype": "i2c", "bus": 87, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan7frue2", "name": "24c64", "bus": 93, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan8",
         "present": {"gettype": "i2c", "bus": 95, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan8frue2", "name": "24c64", "bus": 101, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "others": [
        {"name":"eeprom",
         "device":[
             {"id":"eeprom_1", "name":"24c02","bus":1, "loc":0x56, "attr":"eeprom"},
         ],
        },
        {"name":"lm75",
         "device":[
             {"id":"lm75_1", "name":"lm75","bus":63, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_2", "name":"lm75","bus":64, "loc":0x4f, "attr":"hwmon"},
             {"id":"lm75_3", "name":"lm75","bus":88, "loc":0x48, "attr":"hwmon"},
             {"id":"lm75_4", "name":"lm75","bus":89, "loc":0x49, "attr":"hwmon"},
             {"id":"lm75_5", "name":"lm75","bus":96, "loc":0x48, "attr":"hwmon"},
             {"id":"lm75_6", "name":"lm75","bus":97, "loc":0x49, "attr":"hwmon"},
             {"id":"lm75_7", "name":"lm75","bus":107, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_8", "name":"lm75","bus":109, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_9", "name":"lm75","bus":69, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_10", "name":"lm75","bus":70, "loc":0x4f, "attr":"hwmon"},
             {"id":"lm75_11", "name":"lm75","bus":114, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_12", "name":"lm75","bus":115, "loc":0x4f, "attr":"hwmon"},
         ],
        },
        {"name":"mac_bsc",
         "device":[
             {"id":"mac_bsc_1", "name":"rg_mac_bsc_th4","bus":74, "loc":0x44, "attr":"hwmon"},
         ],
        },
        {"name":"ina3221",
         "device":[
             {"id":"ina3221_1", "name":"rg_ina3221","bus":106, "loc":0x43, "attr":"hwmon"},
         ],
        },
        {"name":"tps53622",
         "device":[
             {"id":"tps53622_1", "name":"rg_tps53622","bus":106, "loc":0x60, "attr":"hwmon"},
             {"id":"tps53622_2", "name":"rg_tps53622","bus":106, "loc":0x6c, "attr":"hwmon"},
         ],
        },
        {"name":"ucd90160",
         "device":[
             {"id":"ucd90160_1", "name":"rg_ucd90160","bus":62, "loc":0x5b, "attr":"hwmon"},
             {"id":"ucd90160_2", "name":"rg_ucd90160","bus":105, "loc":0x5b, "attr":"hwmon"},
             {"id":"ucd90160_3", "name":"rg_ucd90160","bus":73, "loc":0x5b, "attr":"hwmon"},
             {"id":"ucd90160_4", "name":"rg_ucd90160","bus":113, "loc":0x5b, "attr":"hwmon"},
         ],
        },
        {"name":"tmp411",
         "device":[
             {"id":"tmp411_1", "name":"tmp411","bus":71, "loc":0x4c, "attr":"hwmon"},
             {"id":"tmp411_2", "name":"tmp411","bus":72, "loc":0x4c, "attr":"hwmon"},
         ],
        },
    ],
}

CPLDVERSIONS = [ ]
MANUINFO_CONF = {
    "bios": {
        "key": "BIOS",
        "head": True,
        "next": "bmc"
    },
    "bios_vendor": {
        "parent": "bios",
        "key": "Vendor",
        "cmd": "dmidecode -t 0 |grep Vendor",
        "pattern": r".*Vendor",
        "separator": ":",
        "arrt_index" : 1,
    },
    "bios_version": {
        "parent": "bios",
        "key": "Version",
        "cmd": "dmidecode -t 0 |grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index" : 2,
    },
    "bios_date": {
        "parent": "bios",
        "key": "Release Date",
        "cmd": "dmidecode -t 0 |grep Release",
        "pattern": r".*Release Date",
        "separator": ":",
        "arrt_index" : 3,
    },

    "bmc": {
        "key": "BMC",
        "next": "onie"
    },
    "bmc_version": {
        "parent": "bmc",
        "key": "Version",
        "cmd": "ipmitool mc info |grep \"Firmware Revision\"",
        "pattern": r".*Firmware Revision",
        "separator": ":",
        "arrt_index" : 1,
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
        "arrt_index" : 1,
    },
    "onie_version": {
        "parent": "onie",
        "key": "Version",
        "file": "/host/machine.conf",
        "pattern": r"^onie_version",
        "separator": "=",
        "arrt_index" : 2,
    },
    "onie_sub_version": {
        "parent": "onie",
        "key": "Sub Version",
        "file": "/host/machine.conf",
        "pattern": r"^onie_sub_version",
        "separator": "=",
        "arrt_index" : 3,
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
        "arrt_index" : 1,
    },
    "cpu_model": {
        "parent": "cpu",
        "key": "Device Model",
        "cmd": "dmidecode --type processor | grep Version",
        "pattern": r".*Version",
        "separator": ":",
        "arrt_index" : 2,
    },
    "cpu_core": {
        "parent": "cpu",
        "key": "Core Count",
        "cmd": "dmidecode --type processor | grep \"Core Count\"",
        "pattern": r".*Core Count",
        "separator": ":",
        "arrt_index" : 3,
    },
    "cpu_thread": {
        "parent": "cpu",
        "key": "Thread Count",
        "cmd": "dmidecode --type processor | grep \"Thread Count\"",
        "pattern": r".*Thread Count",
        "separator": ":",
        "arrt_index" : 4,
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
        "arrt_index" : 1,
    },
    "ssd_fw": {
        "parent": "ssd",
        "key": "Firmware Version",
        "cmd": "smartctl -i /dev/sda |grep \"Firmware Version\"",
        "pattern": r".*Firmware Version",
        "separator": ":",
        "arrt_index" : 2,
    },
    "ssd_user_cap": {
        "parent": "ssd",
        "key": "User Capacity",
        "cmd": "smartctl -i /dev/sda |grep \"User Capacity\"",
        "pattern": r".*User Capacity",
        "separator": ":",
        "arrt_index" : 3,
    },

    "cpld": {
        "key": "CPLD",
        "next": "psu"
    },

    "cpld1": {
        "key": "CPLD1",
        "parent": "cpld",
        "arrt_index" : 1,
    },
    "cpld1_model": {
        "key": "Device Model",
        "parent": "cpld1",
        "config" : "LCMXO3LF-2100C-5BG256C",
        "arrt_index" : 1,
    },
    "cpld1_vender": {
        "key": "Vendor",
        "parent": "cpld1",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld1_desc": {
        "key": "Description",
        "parent": "cpld1",
        "config" : "CPU_CPLD",
        "arrt_index" : 3,
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
        "arrt_index" : 4,
    },

    "cpld2": {
        "key": "CPLD2",
        "parent": "cpld",
        "arrt_index" : 2,
    },
    "cpld2_model": {
        "key": "Device Model",
        "parent": "cpld2",
        "config" : "LCMXO3LF-4300C-6BG324I",
        "arrt_index" : 1,
    },
    "cpld2_vender": {
        "key": "Vendor",
        "parent": "cpld2",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld2_desc": {
        "key": "Description",
        "parent": "cpld2",
        "config" : "BASE_CPLD",
        "arrt_index" : 3,
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
        "arrt_index" : 4,
    },

    "cpld3": {
        "key": "CPLD3",
        "parent": "cpld",
        "arrt_index" : 3,
    },
    "cpld3_model": {
        "key": "Device Model",
        "parent": "cpld3",
        "config" : "LCMXO3LF-4300C-6BG324I",
        "arrt_index" : 1,
    },
    "cpld3_vender": {
        "key": "Vendor",
        "parent": "cpld3",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld3_desc": {
        "key": "Description",
        "parent": "cpld3",
        "config" : "UPORT_CPLD",
        "arrt_index" : 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "60",
            "loc": "0x3d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "cpld4": {
        "key": "CPLD4",
        "parent": "cpld",
        "arrt_index" : 4,
    },
    "cpld4_model": {
        "key": "Device Model",
        "parent": "cpld4",
        "config" : "LCMXO3LF-2100C-5BG256C",
        "arrt_index" : 1,
    },
    "cpld4_vender": {
        "key": "Vendor",
        "parent": "cpld4",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld4_desc": {
        "key": "Description",
        "parent": "cpld4",
        "config" : "UFCB_CPLD",
        "arrt_index" : 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "87",
            "loc": "0x0d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "cpld5": {
        "key": "CPLD5",
        "parent": "cpld",
        "arrt_index" : 5,
    },
    "cpld5_model": {
        "key": "Device Model",
        "parent": "cpld5",
        "config" : "LCMXO3LF-2100C-5BG256C",
        "arrt_index" : 1,
    },
    "cpld5_vender": {
        "key": "Vendor",
        "parent": "cpld5",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld5_desc": {
        "key": "Description",
        "parent": "cpld5",
        "config" : "DFCB_CPLD",
        "arrt_index" : 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "95",
            "loc": "0x0d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "cpld6": {
        "key": "CPLD6",
        "parent": "cpld",
        "arrt_index" : 6,
    },
    "cpld6_model": {
        "key": "Device Model",
        "parent": "cpld6",
        "config" : "LCMXO3LF-4300C-6BG324I",
        "arrt_index" : 1,
    },
    "cpld6_vender": {
        "key": "Vendor",
        "parent": "cpld6",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld6_desc": {
        "key": "Description",
        "parent": "cpld6",
        "config" : "MAC_CPLDA",
        "arrt_index" : 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "i2c": {
            "bus": "77",
            "loc": "0x1d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "cpld7": {
        "key": "CPLD7",
        "parent": "cpld",
        "arrt_index" : 7,
    },
    "cpld7_model": {
        "key": "Device Model",
        "parent": "cpld7",
        "config" : "LCMXO3LF-4300C-6BG324I",
        "arrt_index" : 1,
    },
    "cpld7_vender": {
        "key": "Vendor",
        "parent": "cpld7",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld7_desc": {
        "key": "Description",
        "parent": "cpld7",
        "config" : "MAC_CPLDB",
        "arrt_index" : 3,
    },
    "cpld7_version": {
        "key": "Firmware Version",
        "parent": "cpld7",
        "i2c": {
            "bus": "77",
            "loc": "0x2d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "cpld8": {
        "key": "CPLD8",
        "parent": "cpld",
        "arrt_index" : 8,
    },
    "cpld8_model": {
        "key": "Device Model",
        "parent": "cpld8",
        "config" : "LCMXO3LF-4300C-6BG324I",
        "arrt_index" : 1,
    },
    "cpld8_vender": {
        "key": "Vendor",
        "parent": "cpld8",
        "config" : "LATTICE",
        "arrt_index" : 2,
    },
    "cpld8_desc": {
        "key": "Description",
        "parent": "cpld8",
        "config" : "DPORT_CPLD",
        "arrt_index" : 3,
    },
    "cpld8_version": {
        "key": "Firmware Version",
        "parent": "cpld8",
        "i2c": {
            "bus": "111",
            "loc": "0x3d",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index" : 4,
    },

    "psu": {
        "key": "PSU",
        "next": "fan"
    },

    "psu1": {
        "parent": "psu",
        "key": "PSU1",
        "arrt_index" : 1,
    },
    "psu1_hw_version": {
        "key": "Hardware Version",
        "parent": "psu1",
        "extra": {
            "funcname": "getPsu",
            "id": "psu1",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "psu1_fw_version": {
        "key": "Firmware Version",
        "parent": "psu1",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "psu2": {
        "parent": "psu",
        "key": "PSU2",
        "arrt_index" : 2,
    },
    "psu2_hw_version": {
        "key": "Hardware Version",
        "parent": "psu2",
        "extra": {
            "funcname": "getPsu",
            "id": "psu2",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "psu2_fw_version": {
        "key": "Firmware Version",
        "parent": "psu2",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "psu3": {
        "parent": "psu",
        "key": "PSU3",
        "arrt_index" : 3,
    },
    "psu3_hw_version": {
        "key": "Hardware Version",
        "parent": "psu3",
        "extra": {
            "funcname": "getPsu",
            "id": "psu3",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "psu3_fw_version": {
        "key": "Firmware Version",
        "parent": "psu3",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "psu4": {
        "parent": "psu",
        "key": "PSU4",
        "arrt_index" : 4,
    },
    "psu4_hw_version": {
        "key": "Hardware Version",
        "parent": "psu4",
        "extra": {
            "funcname": "getPsu",
            "id": "psu4",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "psu4_fw_version": {
        "key": "Firmware Version",
        "parent": "psu4",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan": {
        "key": "FAN",
        "next": "i210"
    },

    "fan1": {
        "key": "FAN1",
        "parent": "fan",
        "arrt_index" : 1,
    },
    "fan1_hw_version": {
        "key": "Hardware Version",
        "parent": "fan1",
        "extra": {
            "funcname": "checkFan",
            "id": "fan1",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan1_fw_version": {
        "key": "Firmware Version",
        "parent": "fan1",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan2": {
        "key": "FAN2",
        "parent": "fan",
        "arrt_index" : 2,
    },
    "fan2_hw_version": {
        "key": "Hardware Version",
        "parent": "fan2",
        "extra": {
            "funcname": "checkFan",
            "id": "fan2",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan2_fw_version": {
        "key": "Firmware Version",
        "parent": "fan2",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan3": {
        "key": "FAN3",
        "parent": "fan",
        "arrt_index" : 3,
    },
    "fan3_hw_version": {
        "key": "Hardware Version",
        "parent": "fan3",
        "extra": {
            "funcname": "checkFan",
            "id": "fan3",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan3_fw_version": {
        "key": "Firmware Version",
        "parent": "fan3",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan4": {
        "key": "FAN4",
        "parent": "fan",
        "arrt_index" : 4,
    },
    "fan4_hw_version": {
        "key": "Hardware Version",
        "parent": "fan4",
        "extra": {
            "funcname": "checkFan",
            "id": "fan4",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan4_fw_version": {
        "key": "Firmware Version",
        "parent": "fan4",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan5": {
        "key": "FAN5",
        "parent": "fan",
        "arrt_index" : 5,
    },
    "fan5_hw_version": {
        "key": "Hardware Version",
        "parent": "fan5",
        "extra": {
            "funcname": "checkFan",
            "id": "fan5",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan5_fw_version": {
        "key": "Firmware Version",
        "parent": "fan5",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan6": {
        "key": "FAN6",
        "parent": "fan",
        "arrt_index" : 6,
    },
    "fan6_hw_version": {
        "key": "Hardware Version",
        "parent": "fan6",
        "extra": {
            "funcname": "checkFan",
            "id": "fan6",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan6_fw_version": {
        "key": "Firmware Version",
        "parent": "fan6",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan7": {
        "key": "FAN7",
        "parent": "fan",
        "arrt_index" : 7,
    },
    "fan7_hw_version": {
        "key": "Hardware Version",
        "parent": "fan7",
        "extra": {
            "funcname": "checkFan",
            "id": "fan7",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan7_fw_version": {
        "key": "Firmware Version",
        "parent": "fan7",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "fan8": {
        "key": "FAN8",
        "parent": "fan",
        "arrt_index" : 8,
    },
    "fan8_hw_version": {
        "key": "Hardware Version",
        "parent": "fan8",
        "extra": {
            "funcname": "checkFan",
            "id": "fan8",
            "key": "hw_version"
        },
        "arrt_index" : 1,
    },
    "fan8_fw_version": {
        "key": "Firmware Version",
        "parent": "fan8",
        "config" : "NA",
        "arrt_index" : 2,
    },

    "i210": {
        "key": "NIC",
        "next": "fpga"
    },
    "i210_model": {
        "parent": "i210",
        "config": "NA",
        "key": "Device Model",
        "arrt_index" : 1,
    },
    "i210_vendor": {
        "parent": "i210",
        "config": "INTEL",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "i210_version": {
        "parent": "i210",
        "cmd": "ethtool -i eth0",
        "pattern": r"firmware-version",
        "separator": ":",
        "key": "Firmware Version",
        "arrt_index" : 3,
    },

    "fpga": {
        "key": "FPGA",
        "next": "others"
    },

    "fpga1": {
        "key": "FPGA1",
        "parent": "fpga",
        "arrt_index" : 1,
    },
    "fpga1_model": {
        "parent": "fpga1",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":0x98,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "0x00000000": "XC7A100T-2FGG484C",
            "0x00000200": "XC7A200T-2FBG484I"
        },
        "key": "Device Model",
        "arrt_index" : 1,
    },
    "fpga1_vender": {
        "parent": "fpga1",
        "config" : "XILINX",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "fpga1_desc": {
        "key": "Description",
        "parent": "fpga1",
        "config" : "UPORT_FPGA",
        "arrt_index" : 3,
    },
    "fpga1_hw_version": {
        "parent": "fpga1",
        "config" : "NA",
        "key": "Hardware Version",
        "arrt_index" : 4,
    },
    "fpga1_fw_version": {
        "parent": "fpga1",
        "pci": {
            "bus"    : 0x0a,
            "slot"   : 0,
            "fn"     : 0,
            "bar"    : 0,
            "offset" : 0
        },
        "key": "Firmware Version",
        "arrt_index" : 5,
    },

    "fpga2": {
        "key": "FPGA2",
        "parent": "fpga",
        "arrt_index" : 2,
    },
    "fpga2_model": {
        "parent": "fpga2",
        "devfile": {
            "loc": "/dev/fpga1",
            "offset":0xb0,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "0x00000000": "XC7A100T-2FGG484C",
            "0x00000200": "XC7A200T-2FBG484I"
        },
        "key": "Device Model",
        "arrt_index" : 1,
    },
    "fpga2_vender": {
        "parent": "fpga2",
        "config" : "XILINX",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "fpga2_desc": {
        "key": "Description",
        "parent": "fpga2",
        "config" : "MAC_FPGA",
        "arrt_index" : 3,
    },
    "fpga2_hw_version": {
        "parent": "fpga2",
        "config" : "NA",
        "key": "Hardware Version",
        "arrt_index" : 4,
    },
    "fpga2_fw_version": {
        "parent": "fpga2",
        "pci": {
            "bus"    : 8,
            "slot"   : 0,
            "fn"     : 0,
            "bar"    : 0,
            "offset" : 0
        },
        "key": "Firmware Version",
        "arrt_index" : 5,
    },

    "fpga3": {
        "key": "FPGA3",
        "parent": "fpga",
        "arrt_index" : 3,
    },
    "fpga3_model": {
        "parent": "fpga3",
        "devfile": {
            "loc": "/dev/fpga2",
            "offset":0x98,
            "len":4,
            "bit_width":4
        },
        "decode": {
            "0x00000000": "XC7A100T-2FGG484C",
            "0x00000200": "XC7A200T-2FBG484I"
        },
        "key": "Device Model",
        "arrt_index" : 1,
    },
    "fpga3_vender": {
        "parent": "fpga3",
        "config" : "XILINX",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "fpga3_desc": {
        "key": "Description",
        "parent": "fpga3",
        "config" : "DPORT_FPGA",
        "arrt_index" : 3,
    },
    "fpga3_hw_version": {
        "parent": "fpga3",
        "config" : "NA",
        "key": "Hardware Version",
        "arrt_index" : 4,
    },
    "fpga3_fw_version": {
        "parent": "fpga3",
        "pci": {
            "bus"    : 0xb,
            "slot"   : 0,
            "fn"     : 0,
            "bar"    : 0,
            "offset" : 0
        },
        "key": "Firmware Version",
        "arrt_index" : 5,
    },

    "others": {
        "key": "OTHERS",
    },
    "5387": {
        "parent": "others",
        "key": "CPU-BMC-SWITCH",
        "arrt_index" : 1,
    },
    "5387_model": {
        "parent": "5387",
        "config": "BCM53134O",
        "key": "Device Model",
        "arrt_index" : 1,
    },
    "5387_vendor": {
        "parent": "5387",
        "config": "Broadcom",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "5387_hw_version": {
        "parent": "5387",
        "key": "Hardware Version",
        "func": {
            "funcname": "get_bcm5387_version",
            "params" : {
                "before": [
                    {"dealtype": "shell", "cmd": "echo 50 > /sys/class/gpio/export"},
                    {"dealtype": "shell", "cmd": "echo high > /sys/class/gpio/gpio50/direction"},
                    {"dealtype": "shell", "cmd": "echo 48 > /sys/class/gpio/export"},
                    {"dealtype": "shell", "cmd": "echo high > /sys/class/gpio/gpio48/direction"},
                    {"dealtype": "io_wr", "io_addr": 0x918, "value":0x06},
                    {"dealtype": "io_wr", "io_addr": 0x943, "value":0x00},
                ],
                "get_version" : "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"dealtype": "shell", "cmd": "echo 0 > /sys/class/gpio/gpio48/value"},
                    {"dealtype": "shell", "cmd": "echo 48 > /sys/class/gpio/unexport"},
                    {"dealtype": "shell", "cmd": "echo 0 > /sys/class/gpio/gpio50/value"},
                    {"dealtype": "shell", "cmd": "echo 50 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"dealtype": "io_wr", "io_addr": 0x943, "value":0x01},
                    {"dealtype": "io_wr", "io_addr": 0x918, "value":0x00},
                ],
            },
        },
        "arrt_index" : 3,
    },
}

MAC_AVS_PARAM = {
    0x7e: 0.882564,
    0x82: 0.859436,
    0x86: 0.836776,
    0x8A: 0.813531,
    0x8E: 0.789233,
}

AVS_VOUT_MODE_PARAM ={
    0x18:256,        # 2^8
    0x17:512,        # 2^9
    0x16:1024,       # 2^10
    0x15:2048,       # 2^11
    0x14:4096,       # 2^12
}

MAC_DEFAULT_PARAM = {
  "type": 0,
  "default":0x82,
  "bus":75,
  "devno":0x10,
  "loopaddr":0xff,
  "loop":0x06,
  "vout_cmd_addr":0x42,
  "vout_mode_addr":0x40,
  "sdktype": 0,
  "macregloc":24 ,
  "mask": 0xff,
  "rov_source":0,
  "cpld_avs":{"bus":77, "loc":0x1d, "offset":0x24, "gettype":"i2c"},
  "set_avs": {"loc": "/sys/bus/i2c/devices/75-0010/avs_vout", "gettype": "sysfs",  "formula": "int((%f)*1000000)"},
}

##
DRIVERLISTS = [
        {"name": "rg_gpio_d1500", "delay": 0},
        {"name": "i2c_dev", "delay": 0},
        {"name": "i2c_gpio", "delay":0},
        {"name": "i2c_mux", "delay":0},
        {"name": "rg_fpga_pcie", "delay": 0},
        {"name": "rg_pcie_dev", "delay": 0},
        {"name": "rg_lpc_drv", "delay": 0},
        {"name": "rg_io_dev", "delay": 0},
        {"name": "rg_i2c_dev", "delay": 0},
        {"name": "rg_fpga_i2c_bus_drv", "delay": 0},
        {"name": "rg_fpga_pca954x_drv", "delay": 0},
        {"name": "rg_wdt", "delay": 0},
        {"name": "rg_gpio_device", "delay": 0},
        {"name": "rg_i2c_gpio_device", "delay":0},
        {"name": "rg_pcie_dev_device", "delay": 0},
        {"name": "rg_lpc_drv_device", "delay": 0},
        {"name": "rg_io_dev_device", "delay": 0},
        {"name": "rg_fpga_i2c_bus_device", "delay": 0},
        {"name": "rg_fpga_pca954x_device", "delay": 0},
        {"name": "rg_i2c_dev_device", "delay": 0},
        {"name": "rg_wdt_device", "delay": 0},
        {"name": "ruijie_common dfd_my_type_i2c_bus=1 dfd_my_type_i2c_addr=0x56", "delay": 1},
        {"name": "rg_spi_gpio_device", "delay": 0},
        {"name": "rg_eeprom_93xx46", "delay": 0},
        {"name": "lm75", "delay":0},
        {"name": "tmp401", "delay":0},
        {"name": "rg_optoe", "delay": 0},
        {"name": "at24", "delay": 0},
        {"name": "rg_mac_bsc", "delay": 0},
        {"name": "rg_pmbus_core", "delay":0},
        {"name": "rg_csu550", "delay": 0},
        {"name": "rg_ina3221", "delay": 0},
        {"name": "rg_tps53622", "delay": 0},
        {"name": "rg_ucd9000", "delay": 0},
        {"name": "rg_xdpe132g5c", "delay": 0},
        {"name": "s3ip_sysfs", "delay": 0},
        {"name": "rg_switch_driver", "delay": 0},
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
        {"name": "rg_plat_dfd", "delay":0},
        {"name": "rg_plat_switch", "delay":0},
        {"name": "rg_plat_fan", "delay":0},
        {"name": "rg_plat_psu", "delay":0},
        {"name": "rg_plat_sff", "delay":0},
        {"name": "rg_plat_sensor", "delay": 0},
]

DEVICE = [
        # GPIO-I2C
        {"name": "24c02", "bus":1, "loc":0x56 },
        # UP port board
        {"name": "rg_ucd90160", "bus": 62, "loc": 0x5b},
        {"name": "lm75", "bus": 63, "loc": 0x4b},
        {"name": "lm75", "bus": 64, "loc": 0x4f},
        # MAC board
        # PSU
        {"name": "24c02", "bus":79, "loc":0x50},
        {"name": "rg_fsp1200","bus":79, "loc":0x58 },
        {"name": "24c02", "bus":80, "loc": 0x50},
        {"name": "rg_fsp1200","bus":80, "loc":0x58 },
        {"name": "24c02", "bus":81, "loc":0x50},
        {"name": "rg_fsp1200","bus":81, "loc":0x58 },
        {"name": "24c02", "bus":82, "loc": 0x50},
        {"name": "rg_fsp1200","bus":82, "loc":0x58 },
        # FAN
        {"name": "24c64","bus":90,"loc":0x50 },
        {"name": "24c64","bus":91,"loc":0x50 },
        {"name": "24c64","bus":92,"loc":0x50 },
        {"name": "24c64","bus":93,"loc":0x50 },
        {"name": "24c64","bus":98,"loc":0x50 },
        {"name": "24c64","bus":99,"loc":0x50 },
        {"name": "24c64","bus":100,"loc":0x50 },
        {"name": "24c64","bus":101,"loc":0x50 },
        # fan temp
        {"name": "lm75", "bus": 88, "loc": 0x48},
        {"name": "lm75", "bus": 89, "loc": 0x49},
        {"name": "lm75", "bus": 96, "loc": 0x48},
        {"name": "lm75", "bus": 97, "loc": 0x49},
        # base temp
        {"name": "lm75", "bus": 107, "loc": 0x4b},
        {"name": "lm75", "bus": 109, "loc": 0x4b},
        {"name": "lm75", "bus": 69, "loc": 0x4b},
        {"name": "lm75", "bus": 70, "loc": 0x4f},
        {"name": "tmp411", "bus": 71, "loc": 0x4c},
        {"name": "tmp411", "bus": 72, "loc": 0x4c},
        # base dcdc
        {"name": "rg_ucd90160", "bus": 105, "loc": 0x5b},
        {"name": "rg_tps53622", "bus": 106, "loc": 0x60},
        {"name": "rg_tps53622", "bus": 106, "loc": 0x6c},
        {"name": "rg_ina3221", "bus": 106, "loc": 0x43},
        {"name": "rg_ucd90160", "bus": 73, "loc": 0x5b},
        # mac bsc
        {"name": "rg_mac_bsc_th4", "bus":74, "loc":0x44 },
        # DOWN port board
        {"name": "rg_ucd90160", "bus": 113, "loc": 0x5b},
        {"name": "lm75", "bus": 114, "loc": 0x4b},
        {"name": "lm75", "bus": 115, "loc": 0x4f},
        # xdpe avs
        {"name": "rg_xdpe132g5c", "bus": 75, "loc": 0x10},
        {"name": "rg_xdpe132g5c", "bus": 76, "loc": 0x5a},
]

OPTOE = [
        {"name": "rg_optoe1", "startbus": 124, "endbus": 251},
]

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
    "sffs": {
        "present": {"path": ["/sys/s3ip/transceiver/*/present"], "ABSENT":0},
        "nochangedmsgflag": 0,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 1,
        "alias": {
            "eth1": "Eth200GE1",
            "eth2": "Eth200GE2",
            "eth3": "Eth200GE3",
            "eth4": "Eth200GE4",
            "eth5": "Eth200GE5",
            "eth6": "Eth200GE6",
            "eth7": "Eth200GE7",
            "eth8": "Eth200GE8",
            "eth9": "Eth200GE9",
            "eth10": "Eth200GE10",
            "eth11": "Eth200GE11",
            "eth12": "Eth200GE12",
            "eth13": "Eth200GE13",
            "eth14": "Eth200GE14",
            "eth15": "Eth200GE15",
            "eth16": "Eth200GE16",
            "eth17": "Eth200GE17",
            "eth18": "Eth200GE18",
            "eth19": "Eth200GE19",
            "eth20": "Eth200GE20",
            "eth21": "Eth200GE21",
            "eth22": "Eth200GE22",
            "eth23": "Eth200GE23",
            "eth24": "Eth200GE24",
            "eth25": "Eth200GE25",
            "eth26": "Eth200GE26",
            "eth27": "Eth200GE27",
            "eth28": "Eth200GE28",
            "eth29": "Eth200GE29",
            "eth30": "Eth200GE30",
            "eth31": "Eth200GE31",
            "eth32": "Eth200GE32",
            "eth33": "Eth200GE33",
            "eth34": "Eth200GE34",
            "eth35": "Eth200GE35",
            "eth36": "Eth200GE36",
            "eth37": "Eth200GE37",
            "eth38": "Eth200GE38",
            "eth39": "Eth200GE39",
            "eth40": "Eth200GE40",
            "eth41": "Eth200GE41",
            "eth42": "Eth200GE42",
            "eth43": "Eth200GE43",
            "eth44": "Eth200GE44",
            "eth45": "Eth200GE45",
            "eth46": "Eth200GE46",
            "eth47": "Eth200GE47",
            "eth48": "Eth200GE48",
            "eth49": "Eth200GE49",
            "eth50": "Eth200GE50",
            "eth51": "Eth200GE51",
            "eth52": "Eth200GE52",
            "eth53": "Eth200GE53",
            "eth54": "Eth200GE54",
            "eth55": "Eth200GE55",
            "eth56": "Eth200GE56",
            "eth57": "Eth200GE57",
            "eth58": "Eth200GE58",
            "eth59": "Eth200GE59",
            "eth60": "Eth200GE60",
            "eth61": "Eth200GE61",
            "eth62": "Eth200GE62",
            "eth63": "Eth200GE63",
            "eth64": "Eth200GE64",
            "eth65": "Eth200GE65",
            "eth66": "Eth200GE66",
            "eth67": "Eth200GE67",
            "eth68": "Eth200GE68",
            "eth69": "Eth200GE69",
            "eth70": "Eth200GE70",
            "eth71": "Eth200GE71",
            "eth72": "Eth200GE72",
            "eth73": "Eth200GE73",
            "eth74": "Eth200GE74",
            "eth75": "Eth200GE75",
            "eth76": "Eth200GE76",
            "eth77": "Eth200GE77",
            "eth78": "Eth200GE78",
            "eth79": "Eth200GE79",
            "eth80": "Eth200GE80",
            "eth81": "Eth200GE81",
            "eth82": "Eth200GE82",
            "eth83": "Eth200GE83",
            "eth84": "Eth200GE84",
            "eth85": "Eth200GE85",
            "eth86": "Eth200GE86",
            "eth87": "Eth200GE87",
            "eth88": "Eth200GE88",
            "eth89": "Eth200GE89",
            "eth90": "Eth200GE90",
            "eth91": "Eth200GE91",
            "eth92": "Eth200GE92",
            "eth93": "Eth200GE93",
            "eth94": "Eth200GE94",
            "eth95": "Eth200GE95",
            "eth96": "Eth200GE96",
            "eth97": "Eth200GE97",
            "eth98": "Eth200GE98",
            "eth99": "Eth200GE99",
            "eth100": "Eth200GE100",
            "eth101": "Eth200GE101",
            "eth102": "Eth200GE102",
            "eth103": "Eth200GE103",
            "eth104": "Eth200GE104",
            "eth105": "Eth200GE105",
            "eth106": "Eth200GE106",
            "eth107": "Eth200GE107",
            "eth108": "Eth200GE108",
            "eth109": "Eth200GE109",
            "eth110": "Eth200GE110",
            "eth111": "Eth200GE111",
            "eth112": "Eth200GE112",
            "eth113": "Eth200GE113",
            "eth114": "Eth200GE114",
            "eth115": "Eth200GE115",
            "eth116": "Eth200GE116",
            "eth117": "Eth200GE117",
            "eth118": "Eth200GE118",
            "eth119": "Eth200GE119",
            "eth120": "Eth200GE120",
            "eth121": "Eth200GE121",
            "eth122": "Eth200GE122",
            "eth123": "Eth200GE123",
            "eth124": "Eth200GE124",
            "eth125": "Eth200GE125",
            "eth126": "Eth200GE126",
            "eth127": "Eth200GE127",
            "eth128": "Eth200GE128",
        }
    },
    "fans": {
        "present": {"path": ["/sys/s3ip/fan/*/status"], "ABSENT":0},
        "status": [
            {"path": "/sys/s3ip/fan/%s/status", 'okval': 1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0
    },
    "psus": {
        "present" : {"path": ["/sys/s3ip/psu/*/present"], "ABSENT":0},
        "status" : [
            {"path": "/sys/s3ip/psu/%s/out_status", "okval":1},
        ],
        "nochangedmsgflag": 1,
        "nochangedmsgtime": 60,
        "noprintfirsttimeflag": 0
    },
    "temps": {
        "temps_list":[
            {"name":"air_inlet", "input_path":"/sys/bus/i2c/devices/107-004b/hwmon/*/temp1_input", "input_accuracy":1000, "warning":43, "critical":53},
            {"name":"MAC_air_outlet", "input_path":"/sys/bus/i2c/devices/70-004f/hwmon/*/temp1_input", "input_accuracy":1000, "warning":70, "critical":75},
            {"name":"CPU_TEMP", "input_path":"/sys/devices/platform/coretemp.0/hwmon/*/temp1_input", "input_accuracy":1000, "warning":100, "critical":102},
            {"name":"SWITCH_TEMP", "input_path":"/sys/bus/i2c/devices/74-0044/hwmon/*/temp99_input", "input_accuracy":1000, "warning":100, "critical":105},
        ],
        "over_temps_polling_seconds": 60,
    },
}

INIT_PARAM = [ ]
INIT_COMMAND = [
    "i2cset -f -y 60 0x3d 0x80 0xff",
    "i2cset -f -y 77 0x1d 0x7c 0xff",
    "i2cset -f -y 111 0x3d 0x80 0xff",
    "i2cset -f -y 60 0x3d 0xd0 0xff",
    "i2cset -f -y 77 0x1d 0xca 0xff",
    "i2cset -f -y 77 0x2d 0xd6 0xff",
    "i2cset -f -y 111 0x3d 0xd0 0xff",
]

INIT_PARAM_PRE = [
    {"loc":"75-0010/avs_vout_max","value": "882564"},
    {"loc":"75-0010/avs_vout_min","value": "789233"},
]
INIT_COMMAND_PRE = [ ]
