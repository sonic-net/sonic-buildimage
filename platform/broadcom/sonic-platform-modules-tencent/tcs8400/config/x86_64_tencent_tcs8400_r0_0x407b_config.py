#!/usr/bin/python3
from ruijiecommon import *


STARTMODULE  = {
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
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "psu1pmbus", "name": "rg_fsp1200", "bus": 41, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu1frue2", "name": "24c02", "bus": 41, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu2",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x1d, "offset": 0x34, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "psu2pmbus", "name": "rg_fsp1200", "bus": 42, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu2frue2", "name": "24c02", "bus": 42, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "fans": [
        {"name": "fan1",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 5, "okval": 0},
         "device": [
             {"id": "fan1frue2", "name": "24c64", "bus": 35, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan2",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "fan2frue2", "name": "24c64", "bus": 34, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan3",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan3frue2", "name": "24c64", "bus": 33, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan4",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan4frue2", "name": "24c64", "bus": 32, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan5",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan5frue2", "name": "24c64", "bus": 31, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan6",
         "present": {"gettype": "i2c", "bus": 4, "loc": 0x3d, "offset": 0x37, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan6frue2", "name": "24c64", "bus": 30, "loc": 0x50, "attr": "eeprom"},
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
             {"id":"lm75_1", "name":"lm75","bus":36, "loc":0x48, "attr":"hwmon"},
             {"id":"lm75_2", "name":"lm75","bus":36, "loc":0x49, "attr":"hwmon"},
             {"id":"lm75_3", "name":"lm75","bus":39, "loc":0x4b, "attr":"hwmon"},
             {"id":"lm75_4", "name":"lm75","bus":40, "loc":0x4e, "attr":"hwmon"},
             {"id":"lm75_5", "name":"lm75","bus":40, "loc":0x4f, "attr":"hwmon"},
         ],
        },
        {"name":"mac_bsc",
         "device":[
             {"id":"mac_bsc_1", "name":"rg_mac_bsc_td4","bus":44, "loc":0x44, "attr":"hwmon"},
         ],
        },
        {"name":"ina3221",
         "device":[
             {"id":"ina3221_1", "name":"rg_ina3221","bus":25, "loc":0x43, "attr":"hwmon"},
         ],
        },
        {"name":"tps53622",
         "device":[
             {"id":"tps53622_1", "name":"rg_tps53622","bus":25, "loc":0x60, "attr":"hwmon"},
             {"id":"tps53622_2", "name":"rg_tps53622","bus":25, "loc":0x6c, "attr":"hwmon"},
         ],
        },
        {"name":"ucd90160",
         "device":[
             {"id":"ucd90160_1", "name":"rg_ucd90160","bus":24, "loc":0x5b, "attr":"hwmon"},
             {"id":"ucd90160_2", "name":"rg_ucd90160","bus":45, "loc":0x5b, "attr":"hwmon"},
         ],
        },
        {"name":"tmp411",
         "device":[
             {"id":"tmp411_1", "name":"tmp411","bus":39, "loc":0x4c, "attr":"hwmon"},
             {"id":"tmp411_2", "name":"tmp411","bus":40, "loc":0x4c, "attr":"hwmon"},
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
        "config" : "LCMXO3LF-2100C-5BG256C",
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
        "config" : "LCMXO3LF-2100C-5BG256C",
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
        "config" : "MAC_CPLDA",
        "arrt_index" : 3,
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
        "config" : "MAC_CPLDB",
        "arrt_index" : 3,
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
        "config" : "FAN_CPLD",
        "arrt_index" : 3,
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
    "fpga_model": {
        "parent": "fpga",
        "devfile": {
            "loc": "/dev/fpga0",
            "offset":0xd8,
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
    "fpga_vendor": {
        "parent": "fpga",
        "config" : "XILINX",
        "key": "Vendor",
        "arrt_index" : 2,
    },
    "fpga_desc": {
        "parent": "fpga",
        "config" : "NA",
        "key": "Description",
        "arrt_index" : 3,
    },
    "fpga_hw_version": {
        "parent": "fpga",
        "config" : "NA",
        "key": "Hardware Version",
        "arrt_index" : 4,
    },
    "fpga_fw_version": {
        "parent": "fpga",
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
                    {"dealtype": "io_wr", "io_addr": 0x94d, "value":0xfe},
                ],
                "get_version" : "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "finally": [
                    {"dealtype": "io_wr", "io_addr": 0x94d, "value":0xff},
                ],
            },
        },
        "arrt_index" : 3,
    },
}

MAC_AVS_PARAM ={
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

AVS_VOUT_MODE_PARAM ={
    0x18:256,        # 2^8
    0x17:512,        # 2^9
    0x16:1024,       # 2^10
    0x15:2048,       # 2^11
    0x14:4096,       # 2^12
}

MAC_DEFAULT_PARAM = {
  "type": 1,
  "default":0x82,
  "bus":43,
  "devno":0x5b,
  "loopaddr":0xff,
  "loop":0x06,
  "vout_cmd_addr":0x42,
  "vout_mode_addr":0x40,
  "sdktype": 0,
  "macregloc":24 ,
  "mask": 0xff,
  "rov_source":0,
  "cpld_avs":{"bus":2, "loc":0x2d, "offset":0x3f, "gettype":"i2c"},
  "set_avs": {"loc": "/sys/bus/i2c/devices/43-005b/avs_vout", "gettype": "sysfs",  "formula": "int((%f)*1000000)"},
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
]

DEVICE = [
        {"name": "24c02","bus":1,"loc":0x56 },
        {"name": "rg_mac_bsc_td4","bus":44,"loc":0x44 },
        {"name": "24c64","bus":30,"loc":0x50 },
        {"name": "24c64","bus":31,"loc":0x50 },
        {"name": "24c64","bus":32,"loc":0x50 },
        {"name": "24c64","bus":33,"loc":0x50 },
        {"name": "24c64","bus":34,"loc":0x50 },
        {"name": "24c64","bus":35,"loc":0x50 },
        {"name": "24c02", "bus":41, "loc":0x50},
        {"name":"rg_fsp1200","bus":41, "loc":0x58 },
        {"name": "24c02", "bus":42, "loc": 0x50},
        {"name":"rg_fsp1200","bus":42, "loc":0x58 },
        {"name": "lm75", "bus": 36, "loc": 0x48},
        {"name": "lm75", "bus": 36, "loc": 0x49},
        {"name": "lm75", "bus": 39, "loc": 0x4b},
        {"name": "tmp411", "bus": 39, "loc": 0x4c},
        {"name": "tmp411", "bus": 40, "loc": 0x4c},
        {"name": "lm75", "bus": 40, "loc": 0x4e},
        {"name": "lm75", "bus": 40, "loc": 0x4f},
        # dcdc
        {"name": "rg_ucd90160", "bus": 24, "loc": 0x5b},
        {"name": "rg_ucd90160", "bus": 45, "loc": 0x5b},
        {"name": "rg_ina3221", "bus": 25, "loc": 0x43},
        {"name": "rg_tps53622", "bus": 25, "loc": 0x60},
        {"name": "rg_tps53622", "bus": 25, "loc": 0x6c},
        # xdpe avs
        {"name": "rg_xdpe132g5c", "bus": 43, "loc": 0x5b},
]

OPTOE = [
        {"name": "rg_optoe1", "startbus": 46, "endbus": 69},
        {"name": "rg_optoe3", "startbus": 70, "endbus": 77},
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
            "eth25": "Eth400GE25",
            "eth26": "Eth400GE26",
            "eth27": "Eth400GE27",
            "eth28": "Eth400GE28",
            "eth29": "Eth400GE29",
            "eth30": "Eth400GE30",
            "eth31": "Eth400GE31",
            "eth32": "Eth400GE32",
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
            {"name":"air_inlet_TL", "input_path":"/sys/bus/i2c/devices/40-004f/hwmon/*/temp1_input", "input_accuracy":1000, "warning":43, "critical":53},
            {"name":"air_outlet_L", "input_path":"/sys/bus/i2c/devices/36-0048/hwmon/*/temp1_input", "input_accuracy":1000, "warning":70, "critical":75},
            {"name":"CPU_TEMP", "input_path":"/sys/devices/platform/coretemp.0/hwmon/*/temp1_input", "input_accuracy":1000, "warning":100, "critical":102},
            {"name":"SWITCH_TEMP", "input_path":"/sys/bus/i2c/devices/44-0044/hwmon/*/temp1_input", "input_accuracy":1000, "warning":100, "critical":105},
        ],
        "over_temps_polling_seconds": 60,
    },
}

INIT_PARAM = [ ]
INIT_COMMAND = [
    "i2cset -f -y 2 0x2d 0x45 0xff",
    "i2cset -f -y 2 0x2d 0x46 0xff",
    "i2cset -f -y 2 0x2d 0x34 0xff",
    "i2cset -f -y 2 0x2d 0x35 0xff",
    "i2cset -f -y 2 0x1d 0x39 0xff",
    "i2cset -f -y 2 0x1d 0x3a 0xff",
    "i2cset -f -y 2 0x2d 0x3a 0xff",
    "i2cset -f -y 2 0x1d 0x3b 0xff",
]

INIT_PARAM_PRE = [
    {"loc":"43-005b/avs_vout_max","value": "900000"},
    {"loc":"43-005b/avs_vout_min","value": "725000"},
]
INIT_COMMAND_PRE = [ ]
