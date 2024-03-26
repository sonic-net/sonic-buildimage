#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "dev_monitor": 1,
    "tty_console": 0,
    "reboot_cause": 0,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 0,
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
        {
            "name": "psu1",
            "present": {"gettype": "i2c", "bus": 13, "loc": 0x1d, "offset": 0x42, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 11, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "wb_24c02", "bus": 11, "loc": 0x50, "attr": "eeprom"},
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
}

PMON_SYSLOG_STATUS = {
    "polling_time": 3,
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
            "fan6": "FAN6",
            "fan7": "FAN7",
            "fan8": "FAN8"
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
    {"name": "wb_i2c_i801", "delay": 1},
    {"name": "i2c_dev", "delay": 0},
    {"name": "wb_i2c_algo_bit", "delay": 0},
    {"name": "wb_i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=181 gpio_scl=180 gpio_chip_name=INTC3001:00 bus_num=1", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40bd", "delay": 0},
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
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "wb_lm75", "delay": 0},
    {"name": "wb_at24", "delay": 0},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "wb_ina3221", "delay": 0},
    {"name": "wb_ucd9000", "delay": 0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "firmware_driver_sysfs", "delay": 0},
    {"name": "wb_firmware_upgrade_device", "delay": 0},
    {"name": "plat_dfd", "delay": 0},
    {"name": "plat_switch", "delay": 0},
    {"name": "plat_fan", "delay": 0},
    {"name": "plat_psu", "delay": 0},
    {"name": "plat_sensor", "delay": 0},

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
    {"name": "wb_24c02", "bus": 11, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 11, "loc": 0x58},
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
    {"name": "wb_xdpe132g5c_pmbus", "bus": 69, "loc": 0x40},
    {"name": "wb_xdpe12284", "bus": 70, "loc": 0x68},
    {"name": "wb_xdpe12284", "bus": 71, "loc": 0x68},
    # fan DCDC
    {"name": "wb_ina3221", "bus": 88, "loc": 0x42},
    {"name": "wb_ina3221", "bus": 89, "loc": 0x42},
    {"name": "wb_ina3221", "bus": 90, "loc": 0x42},
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
    # MAC_PLL1_VDD0.9_C
    {"loc": "56-0040/hwmon/hwmon*/shunt1_resistor", "value": "2000"},
    # MAC_PLL0_VDD0.9_C
    {"loc": "56-0040/hwmon/hwmon*/shunt2_resistor", "value": "2000"},
    # MAC_PLL2_VDD0.9_C
    {"loc": "58-0040/hwmon/hwmon*/shunt1_resistor", "value": "2000"},
    # MAC_PLL3_VDD0.9_C
    {"loc": "58-0040/hwmon/hwmon*/shunt3_resistor", "value": "2000"},
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
    # OE_AVDD_0.75_V
    {"loc": "69-0040/hwmon/hwmon*/avs0_vout_min", "value": "675000"},
    {"loc": "69-0040/hwmon/hwmon*/avs0_vout_max", "value": "825000"},
    # OE_AVDD_1.2_V
    {"loc": "69-0040/hwmon/hwmon*/avs1_vout_min", "value": "1080000"},
    {"loc": "69-0040/hwmon/hwmon*/avs1_vout_max", "value": "1320000"},
    # OE_AVDD_TX_1.8_V
    {"loc": "70-0068/hwmon/hwmon*/avs0_vout_min", "value": "1620000"},
    {"loc": "70-0068/hwmon/hwmon*/avs0_vout_max", "value": "1980000"},
    # OE_AVDD_RX_1.8_V
    {"loc": "70-0068/hwmon/hwmon*/avs1_vout_min", "value": "1620000"},
    {"loc": "70-0068/hwmon/hwmon*/avs1_vout_max", "value": "1980000"},
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

INIT_COMMAND = []

UPGRADE_SUMMARY = {
    "devtype": 0x40bd,

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

        "MTD": {
            "chain9": {
                "name": "BIOS",
                "is_support_warm_upg": 0,
                "filesizecheck": 20480,  # bios check file size, Unit: K
                "init_cmd": [
                    # firmware upgrade set sysled blue/amber alternate flashing
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x50, "value": [0x01]},
                    {"cmd": "modprobe mtd", "gettype": "cmd"},
                    {"cmd": "modprobe spi_nor", "gettype": "cmd"},
                    {"cmd": "modprobe ofpart", "gettype": "cmd"},
                    {"cmd": "modprobe spi_intel writeable=1", "gettype": "cmd"},
                    {"cmd": "modprobe spi_intel_pci", "gettype": "cmd"},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod spi_intel_pci", "gettype": "cmd"},
                    {"cmd": "rmmod spi_intel", "gettype": "cmd"},
                    {"cmd": "rmmod ofpart", "gettype": "cmd"},
                    {"cmd": "rmmod spi_nor", "gettype": "cmd"},
                    {"cmd": "rmmod mtd", "gettype": "cmd"},
                ],
            },
        },

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/iob_fpga_test_header.bin", "display_name": "IOB_FPGA"},
                {"chain": 2, "file": "/etc/.upgrade_test/dom_fpga_test_header.bin", "display_name": "DOM_FPGA"},
            ],
            "cpld": [
                {"chain": 3, "file": "/etc/.upgrade_test/scm_cpld_spi_test_header.bin", "display_name": "SCM_CPLD_SPI"},
                {"chain": 4, "file": "/etc/.upgrade_test/mcb_cpld_spi_test_header.bin", "display_name": "MCB_CPLD_SPI"},
                {"chain": 5, "file": "/etc/.upgrade_test/smb_cpld_spi_test_header.bin", "display_name": "SMB_CPLD_SPI"},
                {"chain": 6, "file": "/etc/.upgrade_test/fcb_b_cpld_spi_test_header.bin", "display_name": "FCB_B_CPLD_SPI"},
                {"chain": 7, "file": "/etc/.upgrade_test/fcb_t_cpld_spi_test_header.bin", "display_name": "FCB_T_CPLD_SPI"},
                #{"chain": 8, "file": "/etc/.upgrade_test/mac_pcie_spi_test_header.bin", "display_name": "MAC_PCIe"},
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
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/40-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/48-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/41-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/49-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/50-0050/eeprom"},
        {"name": "fan7", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/43-0050/eeprom"},
        {"name": "fan8", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/51-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/11-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/10-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}

PLATFORM_POWER_CONF = [
    {
        "name": "Input power total",
        "unit": "W",
        "children": [
            {
                "name": "PSU1 input",
                "pre_check": {
                    "loc": "/sys/wb_plat/psu/psu1/present",
                    "gettype": "sysfs", "mask": 0x01, "okval": 1,
                    "not_ok_msg": "ABSENT"
                },
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/11-0058/hwmon/hwmon*/power1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "PSU2 input",
                "pre_check": {
                    "loc": "/sys/wb_plat/psu/psu2/present",
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
                "name": "PSU1 output",
                "pre_check": {
                    "loc": "/sys/wb_plat/psu/psu1/present",
                    "gettype": "sysfs", "mask": 0x01, "okval": 1,
                    "not_ok_msg": "ABSENT"
                },
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/11-0058/hwmon/hwmon*/power2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "PSU2 output",
                "pre_check": {
                    "loc": "/sys/wb_plat/psu/psu2/present",
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
        "name": "CPO OE power consumption",
        "unit": "W",
        "children": [
            {
                "name": "OE_AVDD_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0040/hwmon/hwmon*/power3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "OE_AVDD_1.2",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/69-0040/hwmon/hwmon*/power4_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "OE_AVDD_TX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/power3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "OE_AVDD_RX_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/70-0068/hwmon/hwmon*/power4_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)/1000000)"
            },
            {
                "name": "OE_HCSL_PLL_VDD0_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/in3_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/56-0040/hwmon/hwmon*/curr3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "OE_HCSL_PLL_VDD1_0.75",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/57-0040/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/57-0040/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "OE_AVDD_3.3",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/in1_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/curr1_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "OE_VDD_HCSL_PLL_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/in2_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/59-0040/hwmon/hwmon*/curr2_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
            },
            {
                "name": "OE_VDD_1.8",
                "val_conf": [
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/in3_input",
                       "int_decode": 10,
                    },
                    {
                       "gettype": "sysfs",
                       "loc": "/sys/bus/i2c/devices/60-0040/hwmon/hwmon*/curr3_input",
                       "int_decode": 10,
                    }
                ],
                "unit": "W",
                "format": "float(float(%s)*float(%s)/1000000)"
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
                       "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/power3_input",
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
                       "loc": "/sys/bus/i2c/devices/71-0068/hwmon/hwmon*/power4_input",
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
