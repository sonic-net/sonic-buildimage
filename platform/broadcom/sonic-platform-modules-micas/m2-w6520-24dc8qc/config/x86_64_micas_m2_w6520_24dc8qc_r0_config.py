#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 0,
    "dev_monitor": 1,
    "tty_console": 1,
    "reboot_cause": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 0,
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 41, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "24c02", "bus": 41, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 42, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "24c02", "bus": 42, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c64", "bus": 35, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c64", "bus": 34, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "24c64", "bus": 33, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "24c64", "bus":32, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "24c64", "bus": 31, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "24c64", "bus": 30, "loc": 0x50, "attr": "eeprom"},
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
                {"id": "lm75_1", "name": "lm75", "bus": 36, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 36, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 39, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 40, "loc": 0x4e, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 40, "loc": 0x4f, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_td4", "bus": 44, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name":"tmp411",
            "device":[
                {"id":"tmp411_1", "name":"tmp411","bus":39, "loc":0x4c, "attr":"hwmon"},
                {"id":"tmp411_2", "name":"tmp411","bus":40, "loc":0x4c, "attr":"hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "ina3221", "bus": 25, "loc": 0x43, "attr": "hwmon"},
            ],
        },
        {
            "name": "tps53622",
            "device": [
                {"id": "tps53622_1", "name": "tps53688", "bus": 25, "loc": 0x67, "attr": "hwmon"},
                {"id": "tps53622_2", "name": "tps53688", "bus": 25, "loc": 0x6c, "attr": "hwmon"},
            ],
        },
        {
            "name": "ucd90160",
            "device": [
                {"id": "ucd90160_1", "name": "ucd90160", "bus": 24, "loc": 0x5b, "attr": "hwmon"},
                {"id": "ucd90160_2", "name": "ucd90160", "bus": 45, "loc": 0x5b, "attr": "hwmon"},
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
            "bus": "2",
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
            "bus": "2",
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
        "config": "FAN_CPLD",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "4",
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
                    {"gettype": "io", "io_addr": 0x94d, "value": 0xfe},  # enable 5387 
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=67 miso=32 mosi=65 bus=0"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46 spi_bus_num=0 spi_cs_gpio=6"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "io", "io_addr": 0x94d, "value": 0xff},  # disable 5387
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
            "loc": "/sys/bus/i2c/devices/43-005b/avs_vout",
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
    {"name": "platform_common dfd_my_type=0x20000056", "delay": 0},
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
]

DEVICE = [
    {"name": "24c02", "bus": 1, "loc": 0x56},
    {"name": "wb_mac_bsc_td4", "bus": 44, "loc": 0x44},
    # fan
    {"name": "24c64", "bus": 30, "loc": 0x50},
    {"name": "24c64", "bus": 31, "loc": 0x50},
    {"name": "24c64", "bus": 32, "loc": 0x50},
    {"name": "24c64", "bus": 33, "loc": 0x50},
    {"name": "24c64", "bus": 34, "loc": 0x50},
    {"name": "24c64", "bus": 35, "loc": 0x50},
    # psu
    {"name": "24c02", "bus": 41, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 41, "loc": 0x58},
    {"name": "24c02", "bus": 42, "loc": 0x50},
    {"name": "wb_fsp1200", "bus": 42, "loc": 0x58},
    # temp
    {"name": "lm75", "bus": 36, "loc": 0x48},
    {"name": "lm75", "bus": 36, "loc": 0x49},
    {"name": "lm75", "bus": 39, "loc": 0x4b},
    {"name": "tmp411", "bus": 39, "loc": 0x4c},
    {"name": "tmp411", "bus": 40, "loc": 0x4c},
    {"name": "lm75", "bus": 40, "loc": 0x4e},
    {"name": "lm75", "bus": 40, "loc": 0x4f},
    # dcdc
    {"name": "ucd90160", "bus": 24, "loc": 0x5b},
    {"name": "ucd90160", "bus": 45, "loc": 0x5b},
    {"name": "ina3221", "bus": 25, "loc": 0x43},
    {"name": "tps53688", "bus": 25, "loc": 0x67},
    {"name": "tps53688", "bus": 25, "loc": 0x6c},
    #avs
    {"name": "wb_xdpe132g5c", "bus": 43, "loc": 0x5b},
]

OPTOE = [
    {"name": "optoe1", "startbus": 46, "endbus": 69},
    {"name": "optoe3", "startbus": 70, "endbus": 77},
]

REBOOT_CTRL_PARAM = {
    "cpu": {"io_addr": 0x920, "rst_val": 0xfe, "rst_delay": 0, "gettype": "io"},
    "mac": {"bus": 2, "loc": 0x1d, "offset": 0x20, "rst_val": 0xfd, "rst_delay": 0, "gettype": "i2c"},
    "phy": {"io_addr": 0x923, "rst_val": 0xef, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
    "power": {"io_addr": 0x9ce, "rst_val": 0, "rst_delay": 0, "gettype": "io"},
}

# INIT_PARAM_PRE = [
#     {"loc": "43-005b/avs_vout_max", "value": "900000"},
#     {"loc": "43-005b/avs_vout_min", "value": "725000"},
# ]

INIT_PARAM = []

INIT_COMMAND_PRE = [
    # sfp power enable
    "i2cset -f -y 2 0x2d 0x45 0xff",
    "i2cset -f -y 2 0x2d 0x46 0xff",
    "i2cset -f -y 2 0x2d 0x34 0xff",
    "i2cset -f -y 2 0x2d 0x35 0xff",
    "i2cset -f -y 2 0x1d 0x39 0xff",
    "i2cset -f -y 2 0x1d 0x3a 0xff",
    # enable tty_console monitor
    "dfd_debug io_wr 0x956 0x01",
]

INIT_COMMAND = [
    # led  enable
    "i2cset -f -y 2 0x2d 0x3a 0xff",
    "i2cset -f -y 2 0x1d 0x3b 0xff",

    # port led off
    "i2cset -f -y 2 0x2d 0x3b 0x0",
    "i2cset -f -y 2 0x2d 0x3c 0x0",
    "i2cset -f -y 2 0x2d 0x3d 0x0",
    "i2cset -f -y 2 0x2d 0x3e 0x0",
    "i2cset -f -y 2 0x1d 0x3c 0x0",
    "i2cset -f -y 2 0x1d 0x3d 0x0",
    "i2cset -f -y 2 0x1d 0x3e 0x0",
    "i2cset -f -y 2 0x1d 0x3f 0x0",
]

WARM_UPGRADE_PARAM = {
    "slot0": {
        "VME": {
            "chain1": [
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
                {"name": "CONNECT_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_base_cpld_header.vme",
                    "init_cmd": [
                        {"bus": 2, "loc": 0x2d, "offset": 0xcd, "value": 1, "gettype": "i2c"},
                        {"io_addr": 0x9cc, "value": 0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"io_addr": 0x9aa, "value": None, "gettype": "io"},
                        {"io_addr": 0x955, "value": None, "gettype": "io"},
                        {"io_addr": 0x911, "value": None, "gettype": "io"},
                        {"io_addr": 0x923, "value": None, "gettype": "io"},
                        {"io_addr": 0x924, "value": None, "gettype": "io"},
                        {"io_addr": 0x930, "value": None, "gettype": "io"},
                        {"io_addr": 0x932, "value": None, "gettype": "io"},
                        {"io_addr": 0x933, "value": None, "gettype": "io"},
                        {"io_addr": 0x934, "value": None, "gettype": "io"},
                        {"io_addr": 0x937, "value": None, "gettype": "io"},
                        {"io_addr": 0x938, "value": None, "gettype": "io"},
                        {"io_addr": 0x939, "value": None, "gettype": "io"},
                        {"io_addr": 0x93a, "value": None, "gettype": "io"},
                        {"io_addr": 0x941, "value": None, "gettype": "io"},
                        {"io_addr": 0x942, "value": None, "gettype": "io"},
                        {"io_addr": 0x947, "value": None, "gettype": "io"},
                        {"io_addr": 0x948, "value": None, "gettype": "io"},
                        {"io_addr": 0x949, "value": None, "gettype": "io"},
                        {"io_addr": 0x94d, "value": None, "gettype": "io"},
                        {"io_addr": 0x94e, "value": None, "gettype": "io"},
                        {"io_addr": 0x94f, "value": None, "gettype": "io"},
                        {"io_addr": 0x950, "value": None, "gettype": "io"},
                        {"io_addr": 0x951, "value": None, "gettype": "io"},
                        {"io_addr": 0x952, "value": None, "gettype": "io"},
                        {"io_addr": 0x953, "value": None, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 30,
                    "after_upgrade_delay_timeout": 60,
                    "refresh_finish_flag_check": {"io_addr": 0x9cb, "value": 0x5a, "gettype": "io"},
                    "access_check_reg": {"io_addr": 0x9aa, "value": 0x5a, "gettype": "io"},
                    "finish_cmd": [
                        {"bus": 2, "loc": 0x2d, "offset": 0xcd, "value": 0, "gettype": "i2c"},
                    ],
                 },

                {"name": "MACA_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_maca_cpld_header.vme",
                    "init_cmd": [
                        {"cmd": "touch /etc/sonic/.warm_upg_flag", "gettype": "cmd"},
                        {"cmd": "sync", "gettype": "cmd"},
                        {"path": "/dev/fpga0", "offset": 0xb4, "value": [0x1], "gettype":"devfile", "delay":0.1},
                        {"bus": 2, "loc": 0x1d, "offset": 0xcc, "value": 0, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 2, "loc": 0x1d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x1a, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x1b, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x1c, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x1d, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x1f, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x22, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x35, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x36, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x37, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x38, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x39, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3b, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3c, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3d, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3e, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x3f, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x40, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x41, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x42, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x1d, "offset": 0x44, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 2, "loc": 0x1d, "offset": 0xcb, "value": 0x5a, "gettype": "i2c"},
                    "access_check_reg": {"bus": 2, "loc": 0x1d, "offset": 0xaa, "value": 0x5a, "gettype": "i2c"},
                    "finish_cmd": [
                        {"path": "/dev/fpga0", "offset": 0xb4, "value": [0x0], "gettype":"devfile"},
                        {"cmd": "rm -rf /etc/sonic/.warm_upg_flag", "gettype": "cmd"},
                        {"cmd": "sync", "gettype": "cmd"},
                    ],
                 },

                {"name": "MACB_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_macb_cpld_header.vme",
                    "init_cmd": [
                        {"cmd": "touch /etc/sonic/.warm_upg_flag", "gettype": "cmd"},
                        {"cmd": "sync", "gettype": "cmd"},
                        {"path": "/dev/fpga0", "offset": 0xb0, "value": [0x1], "gettype":"devfile", "delay":0.1},
                        {"bus": 2, "loc": 0x2d, "offset": 0xcc, "value": 0, "gettype": "i2c"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 2, "loc": 0x2d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x1a, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x1b, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x1c, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x1d, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x1f, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x22, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x23, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x30, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x31, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x32, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x33, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x34, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x35, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x3b, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x3c, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x3d, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x3e, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x40, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x42, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x43, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x44, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x45, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x46, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x4c, "value": None, "gettype": "i2c"},
                        {"bus": 2, "loc": 0x2d, "offset": 0x4d, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 2, "loc": 0x2d, "offset": 0xcb, "value": 0x5a, "gettype": "i2c"},
                    "access_check_reg": {"bus": 2, "loc": 0x2d, "offset": 0xaa, "value": 0x5a, "gettype": "i2c"},
                    "finish_cmd": [
                        {"path": "/dev/fpga0", "offset": 0xb0, "value": [0x0], "gettype":"devfile"},
                        {"cmd": "rm -rf /etc/sonic/.warm_upg_flag", "gettype": "cmd"},
                        {"cmd": "sync", "gettype": "cmd"},
                    ],
                 },

                {"name": "FAN_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_fan_cpld_header.vme",
                    "rw_recover_reg": [
                        {"bus": 4, "loc": 0x3d, "offset": 0xaa, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x17, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x19, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x30, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x31, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x33, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x35, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x3c, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x3d, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x3e, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x3f, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x40, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x41, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x60, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x61, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x62, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x63, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x64, "value": None, "gettype": "i2c"},
                        {"bus": 4, "loc": 0x3d, "offset": 0x65, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"bus": 4, "loc": 0x3d, "offset": 0xaa, "value": 0x5a, "gettype": "i2c"},
                 },
            ],
        },

        "SPI-LOGIC-DEV": {
            "chain1": [
                {"name": "FPGA",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"cmd": "setpci -s 00:03.2 0xA0.W=0x0050", "gettype": "cmd"}, # link_disable
                        {"io_addr": 0x9cd, "value": 0, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 10,
                    "after_upgrade_delay_timeout": 180,
                    "refresh_finish_flag_check": {"io_addr": 0x9cd, "value": 0xff, "gettype": "io"},
                    "access_check_reg": {
                        "path": "/dev/fpga0", "offset": 0x8, "value": [0x55, 0xaa, 0x5a, 0xa5], "read_len":4, "gettype":"devfile",
                        "polling_cmd":[
                            {"cmd": "setpci -s 00:03.2 0xA0.W=0x0060", "gettype": "cmd"},# retrain_link
                            {"cmd": "rmmod wb_fpga_pcie", "gettype": "cmd"},
                            {"cmd": "modprobe wb_fpga_pcie", "gettype": "cmd", "delay": 0.1},
                        ],
                        "polling_delay": 0.1
                    },
                    "finish_cmd": [
                        {"cmd": "setpci -s 00:03.2 0xA0.W=0x0060", "gettype": "cmd"},# retrain_link
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
            "monitor_point": {"gettype": "io", "io_addr": 0x989, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ", 
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size":1*1024*1024}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x987, "value": 0xfc},
            ]
        },
        {
            "name": "bmc_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x98a, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC reboot, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC reboot, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x987, "value": 0xfa},
            ]
        },
        {
            "name": "bmc_powerdown",
            "monitor_point": {"gettype": "io", "io_addr": 0x98b, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "BMC powerdown, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "BMC powerdown, ", "path": "/etc/sonic/.reboot/.history-reboot-cause.txt"}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x987, "value": 0xf6},
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

UPGRADE_SUMMARY = {
    "devtype": 0x20000056,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPLD",
                "is_support_warm_upg": 1,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "FPGA",
                "is_support_warm_upg": 1,
            },
        },

        "SYSFS": {
            "chain2": {
                "name": "BCM5387",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=67 miso=32 mosi=65 bus=0", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46 spi_bus_num=0 spi_cs_gpio=6", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },

        "MTD": {
            "chain3": {
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

        "TEST": {
            "fpga": [
                {"chain": 1, "file": "/etc/.upgrade_test/fpga_test_header.bin", "display_name": "FPGA"},
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_header.vme", "display_name": "CPLD"},
            ],
        },
    },

    "BMC": {
        "name": "BMC",
        "init_cmd": [
            # stop BMC stack watchdog
            {"cmd": "ipmitool raw 0x32 0x03 0x02", "gettype": "cmd", "ignore_result": 1},
        ],
        "finish_cmd": [],
    },
}

PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/35-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/34-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/33-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/32-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/31-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/30-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/41-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/42-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/1-0056/eeprom"},
    ],
}
