#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
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
        {
            "name": "psu1",
            "present": {"gettype": "io", "io_addr": 0x958, "presentbit": 2, "okval": 0},
             "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 42, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "24c02", "bus": 42, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "io", "io_addr": 0x958, "presentbit": 6, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 43, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "24c02", "bus": 43, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c64", "bus": 52, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c64", "bus": 53, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "24c64", "bus": 54, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x5b, "read_len":1, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "24c64", "bus": 55, "loc": 0x50, "attr": "eeprom"},
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
                {"id": "lm75_1", "name": "lm75", "bus": 51, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 56, "loc": 0x4e, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 58, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 75, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 76, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name":"ct7318",
            "device":[
                {"id":"ct7318_1", "name":"ct7318","bus":77, "loc":0x4c, "attr":"hwmon"},
                {"id":"ct7318_2", "name":"ct7318","bus":78, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 68, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 69, "loc": 0x5f, "attr": "hwmon"},
                {"id": "ucd90160_3", "name": "ucd90160", "bus": 82, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_4", "name": "ucd90160", "bus": 83, "loc": 0x5b, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd9081",
            "device": [
                {"id": "wb_ucd9081_1", "name": "wb_ucd9081", "bus": 72, "loc": 0x68, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "xdpe12284", "bus": 90, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "xdpe12284", "bus": 91, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_3", "name": "xdpe12284", "bus": 92, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_4", "name": "xdpe12284", "bus": 93, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_5", "name": "xdpe12284", "bus": 94, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_6", "name": "xdpe12284", "bus": 95, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_7", "name": "xdpe12284", "bus": 96, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_8", "name": "xdpe12284", "bus": 97, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_9", "name": "xdpe12284", "bus": 69, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_10", "name": "xdpe12284", "bus": 69, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_11", "name": "xdpe12284", "bus": 69, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_12", "name": "xdpe12284", "bus": 69, "loc": 0x68, "attr": "hwmon"},
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
        "config": "CONNECT_CPLD",
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
        "config": "LCMXO3LF-4300C-6BG256C",
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
        "config": "MAC_CPLDC",
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
        "config": "LCMXO3LF-4300C-6BG324I",
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
        "config": "MGMT_CPLD",
        "arrt_index": 3,
    },
    "cpld6_version": {
        "key": "Firmware Version",
        "parent": "cpld6",
        "devfile": {
            "loc": "/dev/cpld9",
            "offset":0,
            "len":4,
            "bit_width":1
        },
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
        "config": "FAN_CPLD",
        "arrt_index": 3,
    },
    "cpld7_version": {
        "key": "Firmware Version",
        "parent": "cpld7",
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
        "config": "XC7A50T-2FGG484I",
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
            "bus": 6,
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
            "bus": 6,
            "slot": 0,
            "fn": 0,
            "bar": 0,
            "offset": 4
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
                    {"gettype": "cmd", "cmd": "echo 323 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio323/direction"},
                    # SEL1 high
                    {"gettype": "cmd", "cmd": "echo 324 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio324/direction"},
                    #enable 53134 update
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": 0x01},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": 0x06},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=55  mosi=54 miso=52 cs=53 bus=0 gpio_chip_name=INTC3001:00"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio324/value"},
                    {"gettype": "cmd", "cmd": "echo 324 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio323/value"},
                    {"gettype": "cmd", "cmd": "echo 323 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x01},
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
            "fan4": "FAN4"
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
            "psu2": "PSU2"
        }
    }
}

REBOOT_CTRL_PARAM = {
    "cpu": {"path":"/dev/cpld1", "offset":0x17, "rst_val":0xfd, "rst_delay":0, "gettype":"devfile"},
    "mac": [
        {"gettype": "cmd", "cmd": "setpci -s 14:02.0 0x50.W=0x0050", "rst_delay":0.1},
        {"path":"/dev/cpld6", "offset":0x16, "rst_val":0x00, "rst_delay":1, "gettype": "devfile"},
        {"path":"/dev/cpld6", "offset":0x16, "rst_val":0x01, "rst_delay":0, "gettype": "devfile"},
        {"gettype": "cmd", "cmd": "setpci -s 14:02.0 0x50.W=0x0060", "rst_delay":0.1},
    ],
    "phy": {"path":"/dev/cpld1", "offset":0x18, "rst_val":0x1e, "rst_delay":0, "gettype":"devfile"},
    "power": [
        {"bus": 42, "loc": 0x58, "offset": 0x02, "rst_val": 0x48, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 42, "loc": 0x58, "offset": 0x01, "rst_val": 0x40, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 43, "loc": 0x58, "offset": 0x02, "rst_val": 0x48, "rst_delay":0.1, "gettype": "i2c"},
        {"bus": 43, "loc": 0x58, "offset": 0x01, "rst_val": 0x40, "rst_delay":0.1, "gettype": "i2c"},
    ],

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
                {"record_type":"file", "mode":"cover", "log":"CPU reboot, ", "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
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
        "name": "mac_core",              # AVS name
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x82,                 # default value, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"path": "/dev/cpld6", "offset": 0x30, "read_len": 1, "gettype": "devfile"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/84-0040/avs0_vout_command", "gettype": "sysfs", "formula": None},
        "mac_avs_param": {
            0x92: 0xBF4,
            0x90: 0xC29,
            0x8e: 0xC56,
            0x8c: 0xC8B,
            0x8a: 0xCBD,
            0x88: 0xCEA,
            0x86: 0xD14,
            0x84: 0xD44,
            0x82: 0xD71
        }
    }
]

DRIVERLISTS = [
    {"name": "i2c_i801", "delay": 1},
    {"name": "i2c_dev", "delay": 0},
    {"name": "i2c_algo_bit", "delay": 0},
    {"name": "i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=181 gpio_scl=180 gpio_chip_name=INTC3001:00 bus_num=1", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40d7", "delay": 0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_indirect_dev", "delay": 0},
    {"name": "wb_indirect_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "mdio_bitbang", "delay": 0},
    {"name": "mdio_gpio", "delay": 0},
    {"name": "wb_mdio_gpio_device gpio_mdc=69 gpio_mdio=70 gpio_chip_name=INTC3001:00", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    {"name": "lm75", "delay": 0},
    {"name": "tmp401", "delay": 0},
    {"name": "ct7148", "delay": 0},
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "ina3221", "delay": 0},
    {"name": "tps53679", "delay": 0},
    {"name": "ucd9000", "delay": 0},
    {"name": "wb_ucd9081", "delay": 0}, 
    {"name": "xdpe12284", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay":0},
    {"name": "wb_xdpe132g5c", "delay": 0},
    {"name": "plat_dfd", "delay": 0},
    {"name": "plat_switch", "delay": 0},
    {"name": "plat_fan", "delay": 0},
    {"name": "plat_psu", "delay": 0},
    {"name": "plat_sff", "delay": 0},
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
    {"name": "curr_sensor_device_driver", "delay": 0},
    {"name": "fpga_device_driver", "delay": 0},
    {"name": "watchdog_device_driver", "delay": 0},
]

DEVICE = [
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "24c02", "bus": 57, "loc": 0x57},
    {"name": "24c02", "bus": 61, "loc": 0x57},
    {"name": "24c02", "bus": 66, "loc": 0x57},
    # fan
    {"name": "24c64", "bus": 52, "loc": 0x50},
    {"name": "24c64", "bus": 53, "loc": 0x50},
    {"name": "24c64", "bus": 54, "loc": 0x50},
    {"name": "24c64", "bus": 55, "loc": 0x50},
    # psu
    {"name": "24c02", "bus": 42, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 42, "loc": 0x58},
    {"name": "24c02", "bus": 43, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 43, "loc": 0x58},
    # temp
    {"name": "lm75", "bus": 51, "loc": 0x4b},
    {"name": "lm75", "bus": 56, "loc": 0x4e},
    {"name": "lm75", "bus": 58, "loc": 0x4b},
    {"name": "lm75", "bus": 75, "loc": 0x4b},
    {"name": "lm75", "bus": 76, "loc": 0x4f},
    {"name": "ct7318", "bus": 77, "loc": 0x4c},
    {"name": "ct7318", "bus": 78, "loc": 0x4c},
    #dcdc
    {"name": "ucd90160", "bus": 68, "loc": 0x5b},
    {"name": "ucd90160", "bus": 69, "loc": 0x5f},
    {"name": "xdpe12284", "bus": 69, "loc": 0x70},
    {"name": "xdpe12284", "bus": 69, "loc": 0x6e},
    {"name": "xdpe12284", "bus": 69, "loc": 0x5e},
    {"name": "xdpe12284", "bus": 69, "loc": 0x68},
    {"name": "wb_ucd9081", "bus": 72, "loc": 0x68},
    {"name": "wb_ucd90160", "bus": 82, "loc": 0x5b},
    {"name": "wb_ucd90160", "bus": 83, "loc": 0x5b},
    {"name": "xdpe12284", "bus": 90, "loc": 0x70},
    {"name": "xdpe12284", "bus": 91, "loc": 0x70},
    {"name": "xdpe12284", "bus": 92, "loc": 0x70},
    {"name": "xdpe12284", "bus": 93, "loc": 0x70},
    {"name": "xdpe12284", "bus": 94, "loc": 0x70},
    {"name": "xdpe12284", "bus": 95, "loc": 0x70},
    {"name": "xdpe12284", "bus": 96, "loc": 0x70},
    {"name": "xdpe12284", "bus": 97, "loc": 0x70},
    #avs
    {"name": "wb_xdpe132g5c_pmbus", "bus": 84, "loc": 0x40},
    {"name": "wb_xdpe132g5c", "bus": 84, "loc": 0x10},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 85, "loc": 0x4d},
    {"name": "wb_xdpe132g5c", "bus": 85, "loc": 0x1d},
    {"name": "wb_xdpe132g5c_pmbus", "bus": 86, "loc": 0x4d},
    {"name": "wb_xdpe132g5c", "bus": 86, "loc": 0x1d},
]

OPTOE = [
    {"name": "optoe2", "startbus": 59, "endbus": 60},
    {"name": "optoe3", "startbus": 106, "endbus": 169},
]


INIT_PARAM = []

INIT_COMMAND_PRE = []

INIT_COMMAND = [
    # open X86 BMC Serial port
    "dfd_debug sysfs_data_wr /dev/cpld1 0x41 0x01",
    # enable stream light
    "dfd_debug sysfs_data_wr /dev/cpld6 0xef 0x01",
    "dfd_debug sysfs_data_wr /dev/cpld7 0xef 0x01",
    "dfd_debug sysfs_data_wr /dev/cpld8 0x80 0xff",
    "dfd_debug sysfs_data_wr /dev/cpld8 0x81 0xff",
    # KR power_on
    "dfd_debug sysfs_data_wr /dev/cpld9 0x80 0x03",
    # KR tx-disable enable
    "dfd_debug sysfs_data_wr /dev/cpld9 0x58 0x00",
    ]

WARM_UPGRADE_PARAM = {
    "slot0": {
        "VME": {
            "chain1": [
                {
                    "name": "BASE_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/base_cpld_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0xcb, "value": 0x01, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3b, "value": 0x00, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x36, "value": 0x2c, "delay": 0.1}, #bmc_ready
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x17, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x19, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x1b, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x1c, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x21, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x23, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x25, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x2b, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x2c, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x2d, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x31, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x32, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x33, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x36, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x38, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3e, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3f, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x40, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x41, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x43, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x44, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x57, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x5a, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x65, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x66, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x67, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x70, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x71, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x72, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x73, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x74, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x75, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x90, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x91, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x92, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x93, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x94, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x95, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xa5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb6, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb7, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xb8, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xba, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xbb, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xbc, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc6, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc7, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xd2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xd3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xd4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xd5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xd8, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe6, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe7, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xe8, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 180,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3b, "value": 0x03,"delay": 20}, #after bmc_ready delay time
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0xcb, "value": 0x00, "delay": 0.1},
                    ],
                },
            ],
            "chain2": [
                {
                    "name": "MAC_CPLD_A",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cplda_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x01, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x28, "value": 0x00, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x24, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x25, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x70, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x71, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x72, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x76, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x77, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x78, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd6, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd7, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd8, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xd9, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xda, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xdb, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xe0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xe1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0xef, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld6", "offset": 0x28, "value": 0x01, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x00, "delay": 0.1},
                    ],
                },
                {
                    "name": "MAC_CPLD_B",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cpldb_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x02, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x28, "value": 0x00, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x70, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x71, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x72, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x73, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x74, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x80, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x81, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x82, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x83, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x84, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd4, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd5, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd6, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd7, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd8, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xd9, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xda, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xdb, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xdc, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xde, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xdf, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xe0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xe1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xe2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xe3, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0xef, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld7", "offset": 0x28, "value": 0x01, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x00, "delay": 0.1},
                    ],
                }
            ],
            "chain3": [
                {
                    "name": "MAC_CPLD_C",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cpldc_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x04, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x46, "value": 0x00, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x13, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x14, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x15, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x16, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x17, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x1a, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x20, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x21, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x32, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x33, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x4a, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x4b, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x52, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x59, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x62, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x63, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x80, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x81, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld8", "offset": 0x46, "value": 0x01, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x00, "delay": 0.1},
                    ],
                }
            ],
            "chain4": [
                {
                    "name": "FAN_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/fan_cpld_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x08, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x40, "value": 0x00, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x20, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x22, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x31, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x53, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x56, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x58, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x90, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x91, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x92, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x93, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xd0, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xd1, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xd2, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0xd3, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld10", "offset": 0x40, "value": 0x00, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x00, "delay": 0.1},
                    ],
                }
            ],
            "chain5": [
                {
                    "name": "MGMT_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mgmt_cpld_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x10, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x1c, "value": 0x00, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x21, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x24, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x30, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x40, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x58, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x70, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x71, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x80, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0xa8, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0xba, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"skip": 1},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld9", "offset": 0x1c, "value": 0x01, "delay": 1},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0x00, "delay": 0.1},
                    ],
                }
            ],
            "chain6": [
                {
                    "name": "CPU_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/cpu_cpld_refresh_header.vme",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"cmd": "echo 497 > /sys/class/gpio/export", "gettype": "cmd"},
                        {"cmd": "echo out > /sys/class/gpio/gpio497/direction", "gettype": "cmd"},
                        {"cmd": "echo 1 > /sys/class/gpio/gpio497/value", "gettype": "cmd"},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0xcc, "value": 0x01, "delay": 0.1},
                    ],
                    "rw_recover_reg": [
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x17, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x21, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x41, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x51, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x52, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x53, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x54, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x60, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x61, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x62, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x74, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x81, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x82, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x83, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x84, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x85, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x86, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x87, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x88, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x89, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x8a, "read_len": 1, "value": None, "delay": 0.1},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0xcb, "read_len": 1, "value": None, "delay": 0.1},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 180,
                    "access_check_reg": {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x10, "read_len": 1, "value": 0x36, "okval": 0xff},
                    "finish_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                        {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0xcc, "value": 0x00, "delay": 0.1},
                        {"cmd": "echo 0 > /sys/class/gpio/gpio497/value", "gettype": "cmd"},
                        {"cmd": "echo 497 > /sys/class/gpio/unexport", "gettype": "cmd"},
                    ],
                },
            ],
        },
        "SPI-LOGIC-DEV": {
            "chain1": [
                {"name": "MAC_FPGA",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"cmd": "setpci -s 00:12.0 0x114.W=0xffff", "gettype": "cmd"},
                        {"cmd": "setpci -s 00:12.0 0x50.W=0x0050", "gettype": "cmd"},  # link_disable
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc6, "value": 0x00},
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0xc6, "value": 0x01, "delay": 1},
                    ],
                    "after_upgrade_delay": 10,
                    "after_upgrade_delay_timeout": 10,
                    "refresh_finish_flag_check": {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x63, "value": 0x01, "read_len": 1},
                    "access_check_reg": {
                        "path": "/dev/fpga0", "offset": 0x8, "value": [0x55, 0xaa, 0x5a, 0xa5], "read_len": 4, "gettype": "devfile",
                        "polling_cmd": [
                            {"cmd": "setpci -s 00:12.0 0x50.W=0x0060", "gettype": "cmd", "delay": 1},  # retrain_link
                            {"cmd": "setpci -s 00:12.0 0x52.w", "gettype": "cmd", "okval": 0x12,
                                "timeout": 10, "mask": 0xff},  # check link status
                            {"cmd": "rmmod wb_fpga_pcie", "gettype": "cmd", "delay": 0.1},
                            {"cmd": "modprobe wb_fpga_pcie", "gettype": "cmd", "delay": 2},
                        ],
                        "polling_delay": 0.1
                    },
                    "finish_cmd": [
                        {"cmd": "setpci -s 00:12.0 0x50.W=0x0060", "gettype": "cmd"},  # retrain_link
                        {"cmd": "setpci -s 00:12.0 0x110.W=0xffff", "gettype": "cmd"},
                        {"cmd": "setpci -s 00:12.0 0x114.W=0x0000", "gettype": "cmd"},
                        {"file": WARM_UPG_FLAG, "gettype": "remove_file"},
                    ],
                 },
            ]
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
    "devtype": 0x40d7,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain2": {
                "name": "MAC_CPLDAB",
                "is_support_warm_upg": 1,
            },
            "chain3": {
                "name": "MAC_CPLDC",
                "is_support_warm_upg": 1,
            },
            "chain4": {
                "name": "FAN_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain5": {
                "name": "MGMT_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain6": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 1,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "MAC_FPGA",
                "is_support_warm_upg": 1,
            }
        },

        "SYSFS": {
            "chain2": {
                "name": "BCM53134",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x00, "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=55  mosi=54 miso=52 cs=53 bus=0 gpio_chip_name=INTC3001:00", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x3d, "value": 0x01, "delay": 0.1},
                ],
            },
        },

        "MTD": {
            "chain4": {
                "name": "BIOS",
                "filesizecheck": 20480,  # bios check file size, Unit: K
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe mtd", "gettype": "cmd"},
                    {"cmd": "modprobe spi_nor", "gettype": "cmd"},
                    {"cmd": "modprobe ofpart", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi writeable=1", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi_pci", "gettype": "cmd"},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod intel_spi_pci", "gettype": "cmd"},
                    {"cmd": "rmmod intel_spi", "gettype": "cmd"},
                    {"cmd": "rmmod ofpart", "gettype": "cmd"},
                    {"cmd": "rmmod spi_nor", "gettype": "cmd"},
                    {"cmd": "rmmod mtd", "gettype": "cmd"},
                ],
            },
        },

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/fpga_test_header.bin", "display_name": "MAC_FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_0_1_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/cpld_test_0_2_header.vme", "display_name": "MAC_CPLDAB"},
                {"chain": 3, "file": "/etc/.upgrade_test/cpld_test_0_3_header.vme", "display_name": "MAC_CPLDC"},
                {"chain": 4, "file": "/etc/.upgrade_test/cpld_test_0_4_header.vme", "display_name": "FAN_CPLD"},
                {"chain": 5, "file": "/etc/.upgrade_test/cpld_test_0_5_header.vme", "display_name": "MGMT_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/cpld_test_0_6_header.vme", "display_name": "CPU_CPLD"},
            ],
        },
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/52-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/53-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/54-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/55-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/43-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}
