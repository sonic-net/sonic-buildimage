
#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 1,
    "hal_ledctrl": 1,
    "avscontrol": 0,
    "dev_monitor": 1,
    "reboot_cause": 1,
    "pmon_syslog": 1,
    "sff_temp_polling": 1,
    "generate_airflow": 1,
    "set_eth_mac": 1,
    "drv_update": 1,
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
        "config": "LCMXO3LF-1300C-5BG256C",
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
        "config": "LCMXO3LF-2100C-5BG324C",
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
        "config": "CTRL_CPLD",
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
        "config": "LCMXO3LF_2100C_5BG256C",
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
            "bus": "26",
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
        "config": "LCMXO3LF_1300C_5BG256C",
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
            "bus": "26",
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
        "config": "LCMXO3LF_1300C_5BG256C",
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
            "bus": "28",
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
            "offset": 0,
            "len": 4,
            "bit_width": 4
        },
        "key": "Firmware Version",
        "arrt_index": 5,
    },
    "fpga1_date": {
        "parent": "fpga1",
        "pci": {
            "bus": 4,
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
                    {"gettype": "cmd", "cmd": "echo 457 > /sys/class/gpio/export"},
                    {"gettype": "cmd", "cmd": "echo out > /sys/class/gpio/gpio457/direction"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio457/value"},
                    # select update 5387
                    {"gettype": "io", "io_addr": 0x991, "value": 0x6},
                    {"gettype": "io", "io_addr": 0x990, "value": 0x1},
                    {"gettype": "io", "io_addr": 0x9a6, "value": 0x2},
                    {"gettype": "io", "io_addr": 0x9d4, "value": 0x0},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_gpio_device sck=139 miso=88 mosi=89 cs=87 bus=0 gpio_chip_name=INTC3000:00"},
                    {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46 spi_bus_num=0"},
                ],
                "get_version": "md5sum /sys/bus/spi/devices/spi0.0/eeprom | awk '{print $1}'",
                "after": [
                    {"gettype": "cmd", "cmd": "echo 1 > /sys/class/gpio/gpio457/value"},
                    {"gettype": "cmd", "cmd": "echo 457 > /sys/class/gpio/unexport"},
                ],
                "finally": [
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio_device"},
                    {"gettype": "cmd", "cmd": "rmmod wb_spi_gpio"},
                    {"gettype": "io", "io_addr": 0x9d4, "value": 0xff},
                    {"gettype": "io", "io_addr": 0x9a6, "value": 0xff},
                    {"gettype": "io", "io_addr": 0x990, "value": 0xfc},
                    {"gettype": "io", "io_addr": 0x991, "value": 0xf8},
                ],
            },
        },
        "arrt_index" : 3,
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
        "type": 0,                       # 1: used default value, if rov value not in range. 0: do nothing, if rov value not in range
        "default": 0x73,                 # default value, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"bus":26, "loc":0x2d, "offset":0x3f, "gettype":"i2c"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/30-0040/avs0_vout_command","gettype": "sysfs","formula": None},
        "mac_avs_param": {
            0x72:0x0e66 ,
            0x73:0x0e4c ,
            0x74:0x0e33 ,
            0x75:0x0e19 ,
            0x76:0x0e00 ,
            0x77:0x0de6 ,
            0x78:0x0dcc ,
            0x79:0x0db3 ,
            0x7a:0x0d99 ,
            0x7b:0x0d80 ,
            0x7c:0x0d66 ,
            0x7d:0x0d4c ,
            0x7e:0x0d33 ,
            0x7f:0x0d19 ,
            0x80:0x0d00 ,
            0x81:0x0ce6 ,
            0x82:0x0ccc ,
            0x83:0x0cb3 ,
            0x84:0x0c99 ,
            0x85:0x0c80 ,
            0x86:0x0c66 ,
            0x87:0x0c4c ,
            0x88:0x0c33 ,
            0x89:0x0c19 ,
            0x8A:0x0c00
        }
    }
]


DRIVERLISTS = [
    {"name": "i2c_ismt", "delay": 0},
    {"name": "i2c_i801", "delay": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "i2c_algo_bit", "delay": 0},
    {"name": "i2c_gpio", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "wb_i2c_gpio_device gpio_sda=31 gpio_scl=32 gpio_chip_name=INTC3000:00", "delay": 0},
    {"name": "mdio_bitbang", "delay": 0},
    {"name": "mdio_gpio", "delay": 0},
    {"name": "wb_mdio_gpio_device gpio_mdc=33 gpio_mdio=34 gpio_chip_name=INTC3000:00", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40c1", "delay":0},
    {"name": "wb_fpga_pcie", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_pcie_dev_device", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_i2c_bus_device", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "lm75", "delay":0},
    {"name": "tmp401", "delay":0},
    {"name": "optoe", "delay":0},
    {"name": "at24", "delay":0},
    {"name": "wb_mac_bsc", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "ucd9000", "delay": 0},
    {"name": "wb_xdpe132g5c_pmbus", "delay":0},
    {"name": "xdpe12284", "delay": 0},
    {"name": "wb_csu550", "delay":0},
    {"name": "tps53679", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    {"name": "plat_dfd", "delay":0},
    {"name": "plat_switch", "delay":0},
    {"name": "plat_fan", "delay":0},
    {"name": "plat_psu", "delay":0},
    {"name": "plat_sff", "delay":0},
    {"name": "plat_sensor", "delay":0},
    {"name": "hw_test", "delay": 0},
]


DEVICE = [
    {"name":"24c02", "bus": 2, "loc": 0x56},
    {"name": "wb_mac_bsc_th3", "bus": 62, "loc": 0x44},
    {"name": "tps53688", "bus": 42, "loc": 0x68},
    {"name": "tps53688", "bus": 42, "loc": 0x6e},
    # fan
    {"name":"24c64", "bus": 47, "loc": 0x50},#fan6
    {"name":"24c64", "bus": 48, "loc": 0x50},#fan5
    {"name":"24c64", "bus": 49, "loc": 0x50},#fan4
    {"name":"24c64", "bus": 50, "loc": 0x50},#fan3
    {"name":"24c64", "bus": 51, "loc": 0x50},#fan2
    {"name":"24c64", "bus": 52, "loc": 0x50},#fan1
    # psu
    {"name":"24c02", "bus": 58, "loc": 0x50},
    {"name":"wb_fsp1200","bus":58, "loc":0x58},
    {"name":"24c02", "bus": 59, "loc": 0x50},
    {"name":"wb_fsp1200","bus":59, "loc":0x58},

    {"name":"24c02", "bus": 54, "loc": 0x56}, #fan eeprom
    {"name":"24c02", "bus": 55, "loc": 0x57}, #mac eeprom
    #temp
    {"name":"tmp411", "bus": 56, "loc":0x4c},
    {"name":"tmp411", "bus": 57, "loc":0x4c},
    {"name":"lm75", "bus":53, "loc":0x48},
    {"name":"lm75", "bus":53, "loc":0x49},
    {"name":"lm75", "bus":56, "loc":0x4b},
    {"name":"lm75", "bus":57, "loc":0x4f},
    {"name":"lm75", "bus":57, "loc":0x4e},
    # dcdc
    {"name":"ucd90160", "bus": 41, "loc": 0x5b},
    {"name":"ucd90160", "bus": 60, "loc": 0x5b},
    {"name":"wb_xdpe132g5c_pmbus", "bus": 30, "loc": 0x40},
    {"name":"xdpe12284", "bus": 31, "loc": 0x66},
    {"name":"xdpe12284", "bus": 32, "loc": 0x70},
    {"name":"xdpe12284", "bus": 32, "loc": 0x6e},
    {"name":"xdpe12284", "bus": 42, "loc": 0x5e},
    {"name":"xdpe12284", "bus": 42, "loc": 0x68},
    {"name":"xdpe12284", "bus": 42, "loc": 0x6e},
]

OPTOE = [
    {"name": "optoe3", "startbus": 63, "endbus": 94},
    {"name": "optoe2", "startbus": 95, "endbus": 96},
]

REBOOT_CTRL_PARAM = {
    "cpu": {"io_addr": 0x920, "rst_val": 0xfe, "rst_delay": 0, "gettype": "io"},
    "mac": {"io_addr": 0x921, "rst_val": 0xfe, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
    "phy": {"io_addr": 0x923, "rst_val": 0xef, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
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
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size":1*1024*1024}
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

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "i2c", "bus": 26, "loc": 0x1d, "offset": 0x34, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 59, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu1frue2", "name": "24c02", "bus": 59, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "i2c", "bus": 26, "loc": 0x1d, "offset": 0x34, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 58, "loc": 0x58, "attr": "hwmon"},
                {"id": "psu2frue2", "name": "24c02", "bus": 58, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c64", "bus": 52, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 4, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c64", "bus": 51, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan3",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 3, "okval": 0},
            "device": [
                {"id": "fan3frue2", "name": "24c64", "bus": 50, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan4",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 2, "okval": 0},
            "device": [
                {"id": "fan4frue2", "name": "24c64", "bus": 49, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan5",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan5frue2", "name": "24c64", "bus": 48, "loc": 0x50, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan6",
            "present": {"gettype": "i2c", "bus": 28, "loc": 0x3d, "offset": 0x37, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan6frue2", "name": "24c64", "bus": 47, "loc": 0x50, "attr": "eeprom"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "24c02", "bus": 2, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "lm75",
            "device": [
                {"id": "lm75_1", "name": "lm75", "bus": 53, "loc": 0x48, "attr": "hwmon"},
                {"id": "lm75_2", "name": "lm75", "bus": 53, "loc": 0x49, "attr": "hwmon"},
                {"id": "lm75_3", "name": "lm75", "bus": 56, "loc": 0x4b, "attr": "hwmon"},
                {"id": "lm75_4", "name": "lm75", "bus": 57, "loc": 0x4f, "attr": "hwmon"},
                {"id": "lm75_5", "name": "lm75", "bus": 57, "loc": 0x4e, "attr": "hwmon"},
            ],
        },
        {
            "name": "tmp411",
            "device": [
                {"id": "tmp411_1", "name": "tmp411", "bus": 56, "loc": 0x4c, "attr": "hwmon"},
                {"id": "tmp411_2", "name": "tmp411", "bus": 57, "loc": 0x4c, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "xdpe12284", "bus": 31, "loc": 0x66, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "xdpe12284", "bus": 32, "loc": 0x70, "attr": "hwmon"},
                {"id": "xdpe12284_3", "name": "xdpe12284", "bus": 32, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_4", "name": "xdpe12284", "bus": 42, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_5", "name": "xdpe12284", "bus": 42, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_6", "name": "xdpe12284", "bus": 42, "loc": 0x6e, "attr": "hwmon"},
            ],
        },
    ],
}

INIT_PARAM_PRE = []
INIT_COMMAND_PRE = [
    "i2cset -y -f 26 0x1d 0x51 0x03",
    "i2cset -y -f 26 0x1d 0x54 0x03",
]

INIT_PARAM = []

INIT_COMMAND = []


WARM_UPGRADE_PARAM = {
    "slot0": {
        "VME": {
            "chain1": [
                {"name": "BASE_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/base_cpld_transf_header.vme",
                    "init_cmd": [
                        {"cmd": "echo 63 > /sys/class/gpio/export", "gettype": "cmd"},
                        {"cmd": "echo out > /sys/class/gpio/gpio63/direction", "gettype": "cmd"},
                        {"cmd": "echo 0 > /sys/class/gpio/gpio63/value", "gettype": "cmd"},
                        {"io_addr": 0x9cc, "value": 0x0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"io_addr": 0x932, "value": None, "gettype": "io"},
                        {"io_addr": 0x933, "value": None, "gettype": "io"},
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
                        {"io_addr": 0x990, "value": None, "gettype": "io"},
                        {"io_addr": 0x991, "value": None, "gettype": "io"},
                        {"io_addr": 0x9a3, "value": None, "gettype": "io"},
                        {"io_addr": 0x9a4, "value": None, "gettype": "io"},
                        {"io_addr": 0x9a5, "value": None, "gettype": "io"},
                        {"io_addr": 0x9a6, "value": None, "gettype": "io"},
                        {"io_addr": 0x9a7, "value": None, "gettype": "io"},
                        {"io_addr": 0x9ad, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d2, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d3, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d4, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d5, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d6, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d7, "value": None, "gettype": "io"},
                        {"io_addr": 0x9d8, "value": None, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 30,
                    "after_upgrade_delay_timeout": 60,
                    "refresh_finish_flag_check": {"io_addr": 0x9cb, "value": 0x5a, "gettype": "io"},
                    "access_check_reg": {"io_addr": 0x955, "value": 0xaa, "gettype": "io"},
                    "finish_cmd": [
                        {"io_addr": 0x9cc, "value": 0xff, "gettype": "io"},
                        {"cmd": "echo 1 > /sys/class/gpio/gpio63/value", "gettype": "cmd"},
                        {"cmd": "echo 63 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    ],
                 },
            ],

            "chain2": [
                {"name": "MAC_CPLDA",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cplda_transf_header.vme",
                    "init_cmd": [
                        {"io_addr": 0x9a7, "value": 0x1, "gettype": "io"},
                        {"io_addr": 0x9cc, "value": 0x0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 26, "loc": 0x1d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x1c, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x1d, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x1f, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x20, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x22, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x35, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x36, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x37, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x38, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x39, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x4c, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x4d, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x4e, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x4f, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x50, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x51, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x53, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x1d, "offset": 0x5f, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 26, "loc": 0x1d, "offset": 0xcb, "value": 0x5a, "gettype": "i2c"},
                    "access_check_reg": {"bus": 26, "loc": 0x1d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"io_addr": 0x9cc, "value": 0xff, "gettype": "io"},
                        {"io_addr": 0x9a7, "value": 0x0, "gettype": "io"},
                    ],
                 },

                {"name": "MAC_CPLDB",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/mac_cpldb_transf_header.vme",
                    "init_cmd": [
                        {"io_addr": 0x9a7, "value": 0x2, "gettype": "io"},
                        {"io_addr": 0x9cc, "value": 0x0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"bus": 26, "loc": 0x2d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x22, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x23, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x30, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x31, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x32, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x42, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x43, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x44, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x4a, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x4d, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x50, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x54, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x55, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x60, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x61, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x62, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x63, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x64, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0x65, "value": None, "gettype": "i2c"},
                        {"bus": 26, "loc": 0x2d, "offset": 0xf3, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"bus": 26, "loc": 0x2d, "offset": 0xcb, "value": 0x5a, "gettype": "i2c"},
                    "access_check_reg": {"bus": 26, "loc": 0x2d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [
                        {"io_addr": 0x9cc, "value": 0xff, "gettype": "io"},
                        {"io_addr": 0x9a7, "value": 0x0, "gettype": "io"},
                    ],
                },
            ],

            "chain3": [
                {"name": "FAN_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/fan_cpld_transf_header.vme",
                    "init_cmd": [],
                    "rw_recover_reg": [
                        {"bus": 28, "loc": 0x3d, "offset": 0x11, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x12, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x13, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x14, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x15, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x16, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x17, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x18, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x19, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x20, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x21, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x30, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x31, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x33, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x35, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x3a, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x3c, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x3d, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x3e, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x3f, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x40, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x41, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x60, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x61, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x62, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x63, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x64, "value": None, "gettype": "i2c"},
                        {"bus": 28, "loc": 0x3d, "offset": 0x65, "value": None, "gettype": "i2c"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "access_check_reg": {"bus": 28, "loc": 0x3d, "offset": 0xaa, "value": 0x55, "gettype": "i2c"},
                    "finish_cmd": [],
                 },
            ],


            "chain4": [
                {"name": "CPU_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/cpu_cpld_transf_header.vme",
                    "init_cmd": [
                        {"cmd": "echo 114 > /sys/class/gpio/export", "gettype": "cmd"},
                        {"cmd": "echo high > /sys/class/gpio/gpio114/direction", "gettype": "cmd"},
                        {"io_addr": 0x7a5, "value": 0x0, "gettype": "io"},
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
                        {"io_addr": 0x7a5, "value": 0x1, "gettype": "io"},
                        {"cmd": "echo 0 > /sys/class/gpio/gpio114/value", "gettype": "cmd"},
                        {"cmd": "echo 114 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    ],
                 },
            ],

        },

        "MTD": {
            "chain1": [
                {"name": "MAC_FPGA",
                    "init_cmd": [
                        {"file": WARM_UPG_FLAG, "gettype": "creat_file"},
                        {"cmd": "setpci -s 00:0e.0 0xA0.W=0x0050", "gettype": "cmd"}, # link_disable
                        {"io_addr": 0x9b0, "value": 0x3, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 10,
                    "after_upgrade_delay_timeout": 180,
                    "access_check_reg": {
                        "path": "/dev/fpga0", "offset": 0x8, "value": [0x55, 0xaa, 0x5a, 0xa5], "read_len":4, "gettype":"devfile",
                        "polling_cmd":[
                            {"cmd": "setpci -s 00:0e.0 0xA0.W=0x0060", "gettype": "cmd"}, # retrain_link
                            {"cmd": "rmmod wb_fpga_pcie", "gettype": "cmd"},
                            {"cmd": "modprobe wb_fpga_pcie", "gettype": "cmd", "delay": 0.1},
                        ],
                        "polling_delay": 0.1
                    },
                    "finish_cmd": [
                        {"cmd": "setpci -s  00:0e.0 0xA0.W=0x0060", "gettype": "cmd"}, # retrain_link
                        {"io_addr": 0x9b0, "value": 0x2, "gettype": "io"},
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
    "devtype": 0x40c1,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain2": {
                "name": "MAC_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain3": {
                "name": "FAN_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain4": {
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
                    {"cmd": "modprobe wb_spi_gpio_device sck=139 miso=88 mosi=89 cs=87 bus=0 gpio_chip_name=INTC3000:00", "gettype": "cmd"},
                    {"cmd": "echo 457 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo out > /sys/class/gpio/gpio457/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 0 > /sys/class/gpio/gpio457/value", "gettype": "cmd", "delay": 0.1},
                    {"io_addr": 0x991, "value": 0xfa, "gettype": "io"},
                    {"io_addr": 0x990, "value": 0xfd, "gettype": "io"},
                    {"io_addr": 0x9a6, "value": 0xfe, "gettype": "io"},
                    {"io_addr": 0x9d8, "value": 0xfd, "gettype": "io"},
                    {"cmd": "modprobe wb_spi_nor_device spi_bus_num=0", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"io_addr": 0x9d8, "value": 0xff, "gettype": "io"},
                    {"io_addr": 0x9a6, "value": 0xff, "gettype": "io"},
                    {"io_addr": 0x990, "value": 0xfc, "gettype": "io"},
                    {"io_addr": 0x991, "value": 0xf8, "gettype": "io"},
                    {"cmd": "echo 1 > /sys/class/gpio/gpio457/value", "gettype": "cmd"},
                    {"cmd": "echo 457 > /sys/class/gpio/unexport", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },

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

        "SYSFS": {
            "chain2": {
                "name": "BCM53134O",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_gpio", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_gpio_device sck=139 miso=88 mosi=89 cs=87 bus=0 gpio_chip_name=INTC3000:00", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_93xx46 spi_bus_num=0", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [

                    {"cmd": "rmmod wb_spi_93xx46", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },

        "TEST": {
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/base_cpld_test_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/mac_cpld_test_header.vme", "display_name": "MAC_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/fan_cpld_test_header.vme", "display_name": "FAN_CPLD"},
                {"chain": 4, "file": "/etc/.upgrade_test/cpu_cpld_test_header.vme", "display_name": "CPU_CPLD"},
            ],
            "fpga": [
                {"chain": 1,"file": "/etc/.upgrade_test/fpga_test_header.bin","display_name": "FPGA",},
            ],
        },
    },
}


PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/52-0050/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/51-0050/eeprom"},
        {"name": "fan3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/50-0050/eeprom"},
        {"name": "fan4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/49-0050/eeprom"},
        {"name": "fan5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/48-0050/eeprom"},
        {"name": "fan6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/47-0050/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/59-0050/eeprom"},
        {"name": "psu2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/58-0050/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/2-0056/eeprom"},
    ],
}

AIR_FLOW_CONF = {
    "psu_fan_airflow": {
        "intake": ['GW-CRPS1300D', 'DPS-1300AB-6 F', 'DPS-1300AB-6 S'],
        "exhaust": ['CRPS1300D3R', 'DPS-1300AB-11 C']
    },

    "fanairflow": {
        "intake": ['M1HFAN IV-F'],
        "exhaust": ['M1HFAN IV-R']
    },

    "fans": [
        {
            "name": "FAN1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/52-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/51-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN3", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/50-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN4", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/49-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN5", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/48-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN6", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/47-0050/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
    ],

    "psus": [
        {
            "name": "PSU1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/59-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        },
        {
            "name": "PSU2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/58-0050/eeprom",
            "area": "productInfoArea", "field": "productPartModelName", "decode": "psu_fan_airflow"
        }
    ]
}

SET_MAC_CONF = [
    {
        "eth_name": "eth0",
        "e2_name": "syseeprom",
        "e2_type": "onie_tlv",
        "e2_path": "/sys/bus/i2c/devices/2-0056/eeprom",
        "mac_location": {"field": "Base MAC Address"},
        "ifcfg": {
            "ifcfg_file_path": "/etc/network/interfaces.d/ifcfg-eth0-mac", "file_mode": "add",
        }
    }
]

DRVIER_UPDATE_CONF = {
    "reboot_flag": 1,
    "drv_list": [
        {
            "source": "extra/sdhci_pci.ko",
            "target": "kernel/drivers/mmc/host/sdhci-pci.ko",
            "judge_flag": "/sys/module/sdhci_pci/parameters/wb_sdhci_pci"
        },
    ]
}
