#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 1,
    "dev_monitor": 1,
    "pmon_syslog": 1,
    "tty_console": 1,
    "macledreset": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 1,
    "reboot_cause": 1,
}

MAC_LED_RESET = {"pcibus": 8, "slot": 0, "fn": 0, "bar": 0, "offset": 64, "reset": 0x98}

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
        "config": "CONNECT_CPLD-FAN",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "2",
            "loc": "0x0d",
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
        "config": "MAC_CPLD1",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "8",
            "loc": "0x30",
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
        "config": "MAC_CPLD2",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "8",
            "loc": "0x31",
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
        "next": "asic"
    },
    "fpga_model": {
        "parent": "fpga",
        "config": "XC7A15T-2FGG484C",
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
    "asic": {
        "key": "ASIC",
    },
    "sdk_model": {
        "parent": "asic",
        "cmd": "bcmcmd -t 1 att",
        "pattern": r"^Attach",
        "regular": r"(?<=\()[^)]*(?=\))",
        "key": "Device Model",
        "arrt_index": 1,
    },
    "sdk_version": {
        "parent": "asic",
        "cmd": "bcmcmd -t 1 version | grep Release",
        "pattern": r".*Release",
        "separator": ":",
        "key": "SDK Version",
        "arrt_index": 2,
    },
    "pci_version": {
        "parent": "asic",
        "cmd": "bcmcmd -t 1 \"pciephy fw version\" |grep \"PCIe FW version\"",
        "pattern": r".*PCIe FW version",
        "separator": ":",
        "key": "PCIe Firmware Version",
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
            "fan4": "FAN4"
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
        "default": 0x74,                 # default value, if rov value not in range
        "sdkreg": "TOP_AVS_SEL_REG",     # SDK register name
        "sdktype": 0,                    # 0: No shift operation required, 1: shift operation required
        "macregloc": 24,                 # Shift right 24 bits
        "mask": 0xff,                    # Use with macregloc
        "rov_source": 1,                 # 0:get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"bus": 6, "loc": 0x0d, "offset": 0xc3, "gettype": "i2c"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/7-0064/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x08: 0.888,
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
    {"name": "platform_common dfd_my_type=0x404a", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
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
    {"name": "wb_optoe", "delay": 0},
    {"name": "wb_at24", "delay": 0},
    {"name": "wb_mac_bsc", "delay": 0},
    {"name": "wb_pmbus_core", "delay": 0},
    {"name": "wb_isl68137", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "wb_ina3221", "delay": 0},
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
]

DEVICE = [
    {"name": "wb_24c02", "bus": 0, "loc": 0x56},
    {"name": "wb_mac_bsc_td3", "bus": 3, "loc": 0x44},
    # fan
    {"name": "wb_24c02", "bus": 16, "loc": 0x50},
    {"name": "wb_24c02", "bus": 17, "loc": 0x50},
    {"name": "wb_24c02", "bus": 18, "loc": 0x50},
    {"name": "wb_24c02", "bus": 19, "loc": 0x50},
    # psu
    {"name": "wb_24c02", "bus": 24, "loc": 0x50},
    {"name": "wb_dps550", "bus": 24, "loc": 0x58},
    {"name": "wb_24c02", "bus": 25, "loc": 0x50},
    {"name": "wb_dps550", "bus": 25, "loc": 0x58},
    # temp
    {"name": "wb_lm75", "bus": 3, "loc": 0x48},
    {"name": "wb_lm75", "bus": 3, "loc": 0x49},
    {"name": "wb_lm75", "bus": 3, "loc": 0x4a},
    {"name": "wb_lm75", "bus": 3, "loc": 0x4b},
    {"name": "wb_lm75", "bus": 3, "loc": 0x4c},
    # dcdc
    {"name": "wb_ina3221", "bus": 7, "loc": 0x40},
    {"name": "wb_ina3221", "bus": 7, "loc": 0x41},
    {"name": "wb_ina3221", "bus": 7, "loc": 0x42},
    {"name": "wb_ina3221", "bus": 7, "loc": 0x43},
    {"name": "wb_tps53622", "bus": 7, "loc": 0x60},
    {"name": "wb_tps53622", "bus": 7, "loc": 0x6c},
    {"name": "wb_isl68127", "bus": 7, "loc": 0x64},
]

OPTOE = [
    {"name": "wb_optoe2", "startbus": 32, "endbus": 79},
    {"name": "wb_optoe1", "startbus": 80, "endbus": 87},
]

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "i2c", "bus": 6, "loc": 0x0d, "offset": 0x51, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_dps550", "bus": 24, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "wb_24c02", "bus": 24, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "i2c", "bus": 6, "loc": 0x0d, "offset": 0x51, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_dps550", "bus": 25, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "wb_24c02", "bus": 25, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c02", "bus": 16, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c02", "bus": 17, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "24c02", "bus": 18, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "24c02", "bus": 19, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "wb_24c02", "bus": 0, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "wb_lm75", "bus": 3, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_2", "name": "wb_lm75", "bus": 3, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_3", "name": "wb_lm75", "bus": 3, "loc": 0x4a, "attr": "hwmon"},
                {"id": "lm75_4", "name": "wb_lm75", "bus": 3, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_5", "name": "wb_lm75", "bus": 3, "loc": 0x4c, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_td3", "bus": 3, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "wb_ina3221", "bus": 7, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_2", "name": "wb_ina3221", "bus": 7, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_3", "name": "wb_ina3221", "bus": 7, "loc": 0x42, "attr": "hwmon"},
                {"id": "ina3221_4", "name": "wb_ina3221", "bus": 7, "loc": 0x43, "attr": "hwmon"},
            ],
        },
        {
            "name": "tps53622",
            "device": [
                {"id": "tps53622_1", "name": "wb_tps53622", "bus": 7, "loc": 0x60, "attr": "hwmon"},
                {"id": "tps53622_2", "name": "wb_tps53622", "bus": 7, "loc": 0x6c, "attr": "hwmon"},
            ],
        },
        {
            "name": "isl68127",
            "device": [
                {"id": "isl68127_1", "name": "wb_isl68127", "bus": 7, "loc": 0x64, "attr": "hwmon"},
            ],
        }
    ],
}

INIT_PARAM_PRE = [
    {"loc": "7-0064/hwmon/hwmon*/avs0_vout_max", "value": "900000"},
    {"loc": "7-0064/hwmon/hwmon*/avs0_vout_min", "value": "750000"},
]
INIT_COMMAND_PRE = [
    "i2cset -y -f 6 0x0d 0x91 0x48",
    "i2cset -y -f 6 0x0d 0x92 0x01",  # MAC_PWR_EN
    "i2cset -y -f 6 0x0d 0x94 0x01",  # SFF_PWR_EN
    "i2cset -y -f 6 0x0d 0xbf 0x01",  # enbale tty_console monitor
]

INIT_PARAM = []

INIT_COMMAND = [
    "i2cset -y -f 8 0x30 0x60 0x00",  # enable txdis[1~8]
    "i2cset -y -f 8 0x30 0x61 0x00",  # enable txdis[9~16]
    "i2cset -y -f 8 0x30 0x62 0x00",  # enable txdis[17~24]
    "i2cset -y -f 8 0x31 0x60 0x00",  # enable txdis[24~32]
    "i2cset -y -f 8 0x31 0x61 0x00",  # enable txdis[33~40]
    "i2cset -y -f 8 0x31 0x62 0x00",  # enable txdis[41~48]
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

UPGRADE_SUMMARY = {
    "devtype": 0x404a,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "VME_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain3": {
                "name": "FPGA",
                "is_support_warm_upg": 0,
            },
        },

        "MTD": {
            "chain2": {
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
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpld_test_header.vme", "display_name": "CPLD"},
            ],
            "fpga": [
                {
                    "chain": 3,
                    "file": "/etc/.upgrade_test/fpga_test_header.bin",
                    "display_name": "FPGA",
                },
            ],
        },
    },

    "BMC": {
        "name": "BMC",
        "init_cmd": [
        ],
        "finish_cmd": [],
    },
}


PLATFORM_E2_CONF = {
    "fan": [
        {
            "name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/16-0050/eeprom",
            "e2_decode": [
                {
                    "area": "productInfoArea", "field": "productVersion", "decode_type": "func", "func_name": "fru_decode_hw"
                },
                {
                    "area": "boardInfoArea", "field": "boardextra1", "decode_type": "func", "func_name": "fru_decode_hw"
                },
            ],
        },
        {
            "name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/17-0050/eeprom",
            "e2_decode": [
                {
                    "area": "productInfoArea", "field": "productVersion", "decode_type": "func", "func_name": "fru_decode_hw"
                },
                {
                    "area": "boardInfoArea", "field": "boardextra1", "decode_type": "func", "func_name": "fru_decode_hw"
                },
            ],
        },
        {
            "name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/18-0050/eeprom",
            "e2_decode": [
                {
                    "area": "productInfoArea", "field": "productVersion", "decode_type": "func", "func_name": "fru_decode_hw"
                },
                {
                    "area": "boardInfoArea", "field": "boardextra1", "decode_type": "func", "func_name": "fru_decode_hw"
                },
            ],
        },
        {
            "name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/19-0050/eeprom",
            "e2_decode": [
                {
                    "area": "productInfoArea", "field": "productVersion", "decode_type": "func", "func_name": "fru_decode_hw"
                },
                {
                    "area": "boardInfoArea", "field": "boardextra1", "decode_type": "func", "func_name": "fru_decode_hw"
                },
            ],
        },
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/24-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/25-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/0-0056/eeprom"},
    ],
}

AIR_FLOW_CONF = {
    "psu_fan_airflow": {
        "intake": ['CSU550AP-3-500', 'DPS-550AB-39 A', 'GW-CRPS550N2C', 'CSU550AP-3-300', 'DPS-550AB-39 B', 'CSU550AP-3'],
        "exhaust": ['CSU550AP-3-501', 'DPS-550AB-40 A', 'GW-CRPS550N2RC']
    },

    "fanairflow": {
        "intake": ['M1HFAN I-F'],
        "exhaust": ['M1HFAN I-R']
    },

    "fans": [
        {
            "name": "FAN1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/16-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/17-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/18-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/19-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        }
    ],

    "psus": [
        {
            "name": "PSU1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/24-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        },
        {
            "name": "PSU2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/25-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        }
    ]
}
