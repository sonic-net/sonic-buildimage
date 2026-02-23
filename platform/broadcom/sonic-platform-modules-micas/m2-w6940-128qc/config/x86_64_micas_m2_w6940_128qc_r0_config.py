#!/usr/bin/python
# -*- coding: UTF-8 -*-
from platform_common import *

STARTMODULE = {
    "hal_fanctrl": 0,
    "hal_ledctrl":0,
    "avscontrol": 0,
    "dev_monitor": 0,
    "reboot_cause": 0,
    "pmon_syslog": 0,
    "sff_temp_polling": 0,
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
        "config": "BASE CPLD",
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
        "config": "LC CPLD",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "i2c": {
            "bus": "17",
            "loc": "0x30",
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
        "config": "MAC CPLDA",
        "arrt_index": 3,
    },
    "cpld4_version": {
        "key": "Firmware Version",
        "parent": "cpld4",
        "i2c": {
            "bus": "18",
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
        "config": "MAC CPLDB",
        "arrt_index": 3,
    },
    "cpld5_version": {
        "key": "Firmware Version",
        "parent": "cpld5",
        "i2c": {
            "bus": "19",
            "loc": "0x30",
            "offset": 0,
            "size": 4
        },
        "callback": "cpld_format",
        "arrt_index": 4,
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
        "config": "XC7A50T-2FGG484I",
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
            "bus": 3,
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
            "bus": 3,
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
            "sff49": "Ethernet49",
            "sff50": "Ethernet50",
            "sff51": "Ethernet51",
            "sff52": "Ethernet52"
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
            "fan2": "FAN2"
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
        "default": 0x01,                 # default value, if rov value not in range
        "rov_source": 0,                 # 0: get rov value from cpld, 1: get rov value from SDK
        "cpld_avs": {"io_addr": 0x956, "gettype": "io"},
        "set_avs": {
            "loc": "/sys/bus/i2c/devices/17-0058/hwmon/hwmon*/avs0_vout",
            "gettype": "sysfs", "formula": "int((%f)*1000000)"
        },
        "mac_avs_param": {
            0x08: 0.875,
            0x04: 0.850,
            0x02: 0.825,
            0x01: 0.800
        }
    }
]


DRIVERLISTS = [
    {"name": "r8169", "delay": 0, "removable": 0},
    {"name": "ice", "delay": 0, "removable": 0},
    {"name": "i2c_i801", "delay": 0},
    {"name": "i2c_dev", "delay": 0},
    {"name": "i2c_mux", "delay": 0},
    {"name": "platform_common dfd_my_type=0x40c7", "delay": 0},
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
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
]

DEVICE = [
    {"name": "24c02", "bus": 0, "loc": 0x56},
]

OPTOE = [
    {"name": "optoe3", "startbus": 25, "endbus": 152},
]

REBOOT_CTRL_PARAM = {
    #"cpu": {"io_addr": 0x910, "rst_val": 0x10, "rst_delay": 0, "gettype": "io"},
    #"mac": {"io_addr": 0x930, "rst_val": 0xbf, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
    #"phy": {"io_addr": 0x930, "rst_val": 0xf7, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "io", "io_addr": 0xb10, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "psu1frue2", "name": "24c02", "bus": 7, "loc": 0x56, "attr": "eeprom"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "io", "io_addr": 0xb10, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "psu2frue2", "name": "24c02", "bus": 7, "loc": 0x57, "attr": "eeprom"},
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {"gettype": "io", "io_addr": 0x994, "presentbit": 0, "okval": 0},
            "device": [
                {"id": "fan1frue2", "name": "24c02", "bus": 8, "loc": 0x53, "attr": "eeprom"},
            ],
        },
        {
            "name": "fan2",
            "present": {"gettype": "io", "io_addr": 0x994, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "fan2frue2", "name": "24c02", "bus": 9, "loc": 0x53, "attr": "eeprom"},
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
            "name": "tmp275",
            "device": [
                {"id": "tmp275_1", "name": "wb_tmp275", "bus": 6, "loc": 0x48, "attr": "hwmon"},
                {"id": "tmp275_2", "name": "wb_tmp275", "bus": 6, "loc": 0x49, "attr": "hwmon"},
            ],
        },
        {
            "name": "mac_bsc",
            "device": [
                {"id": "mac_bsc_1", "name": "wb_mac_bsc_td3_x2", "bus": 18, "loc": 0x44, "attr": "hwmon"},
            ],
        },
        {
            "name": "ina3221",
            "device": [
                {"id": "ina3221_1", "name": "wb_ina3221", "bus": 3, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_2", "name": "wb_ina3221", "bus": 3, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_3", "name": "wb_ina3221", "bus": 3, "loc": 0x42, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "wb_xdpe12284", "bus": 0, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "wb_xdpe12284", "bus": 0, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "wb_xdpe12284", "bus": 0, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "wb_xdpe12284", "bus": 17, "loc": 0x58, "attr": "hwmon"},
            ],
        },
    ],
}

INIT_PARAM_PRE = []

INIT_COMMAND_PRE = []

INIT_PARAM = []

INIT_COMMAND = [
    # set sysled
    "dfd_debug io_wr 0x950 0x04",
    # mac led reset
    "dfd_debug sysfs_data_wr /dev/fpga0 0x40 0x98 0x00 0x00 0x00",
    "dfd_debug sysfs_data_wr /dev/fpga0 0x44 0x98 0x00 0x00 0x00",
    "dfd_debug sysfs_data_wr /dev/fpga0 0x48 0x98 0x00 0x00 0x00",
    "dfd_debug sysfs_data_wr /dev/fpga0 0x4c 0x98 0x00 0x00 0x00",
    # enable root port PCIe AER
    "setpci -s 00:10.0 0x5c.b=0x1f",
    "setpci -s 00:12.0 0x5c.b=0x1f",
    "setpci -s 00:14.0 0x5c.b=0x1f",
    "setpci -s 14:02.0 0x5c.b=0x1f",
    "setpci -s 14:03.0 0x5c.b=0x1f",
    "setpci -s 14:04.0 0x5c.b=0x1f",
    "setpci -s 14:05.0 0x5c.b=0x1f"
]

REBOOT_CAUSE_PARA = {
    "reboot_cause_list": [
        {
            "name": "wdt_reboot",
            "monitor_point": {"gettype": "io", "io_addr": 0x76b, "okval": 1},
            "record": [
                {"record_type": "file", "mode": "cover", "log": "Watchdog, ",
                    "path": "/etc/sonic/.reboot/.previous-reboot-cause.txt"},
                {"record_type": "file", "mode": "add", "log": "Watchdog, ",
                    "path": "/etc/sonic/.reboot/.history-reboot-cause.txt", "file_max_size":1*1024*1024}
            ],
            "finish_operation": [
                {"gettype": "io", "io_addr": 0x76b, "value": 0x00},
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


WARM_UPGRADE_PARAM = {
    "slot0": {
        "VME": {
            "chain1": [
                {"name": "CPU_CPLD",
                    "refresh_file_judge_flag": 1,
                    "refresh_file": "/etc/.cpld_refresh/refresh_cpu_cpld_header.vme",
                    "init_cmd": [
                        {"cmd": "echo 98 > /sys/class/gpio/export", "gettype": "cmd"},
                        {"cmd": "echo high > /sys/class/gpio/gpio98/direction", "gettype": "cmd"},
                        {"io_addr": 0x7a5, "value": 0, "gettype": "io"},
                    ],
                    "rw_recover_reg": [
                        {"io_addr": 0x721, "value": None, "gettype": "io"},
                        {"io_addr": 0x765, "value": None, "gettype": "io"},
                        {"io_addr": 0x766, "value": None, "gettype": "io"},
                        {"io_addr": 0x768, "value": None, "gettype": "io"},
                    ],
                    "after_upgrade_delay": 1,
                    "after_upgrade_delay_timeout": 30,
                    "refresh_finish_flag_check": {"io_addr":0x7a5, "value":0x01, "gettype":"io"},
                    "access_check_reg": {"io_addr": 0x705, "value": 0x5a, "gettype": "io"},
                    "finish_cmd": [
                        {"cmd": "echo 0 > /sys/class/gpio/gpio98/value", "gettype": "cmd"},
                        {"cmd": "echo 98 > /sys/class/gpio/unexport", "gettype": "cmd"},
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
    "devtype": 0x40c7,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain2": {
                "name": "BASE_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain3": {
                "name": "LC_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain4": {
                "name": "MAC_CPLDA",
                "is_support_warm_upg": 0,
            },
            "chain5": {
                "name": "MAC_CPLDB",
                "is_support_warm_upg": 0,
            },
            "chain6": {
                "name": "FCB_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain7": {
                "name": "MISC_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain8": {
                "name": "PCIE_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "FPGA",
                "is_support_warm_upg": 0,
            },
        },

        "MTD": {
            "chain2": {
                "name": "BIOS",
                "is_support_warm_upg": 0,
                "filesizecheck": 20480,  # bios check file size, Unit: K
                "init_cmd": [
                    {"cmd": "modprobe mtd", "gettype": "cmd"},
                    {"cmd": "modprobe spi_nor", "gettype": "cmd"},
                    {"cmd": "modprobe ofpart", "gettype": "cmd"},
                    {"cmd": "modprobe spi_intel writeable=1", "gettype": "cmd"},
                    {"cmd": "modprobe intel_spi_pci", "gettype": "cmd"},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod intel_spi_pci", "gettype": "cmd"},
                    {"cmd": "rmmod spi_intel", "gettype": "cmd"},
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
                {"chain": 1, "file": "/etc/.upgrade_test/cpu_cpld_test_header.vme", "display_name": "CPU_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/base_cpld_test_header.vme", "display_name": "BASE_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/lc_cpld_test_header.vme", "display_name": "LC_CPLD"},
                {"chain": 4, "file": "/etc/.upgrade_test/mac_cplda_test_header.vme", "display_name": "MAC_CPLDA"},
                {"chain": 5, "file": "/etc/.upgrade_test/mac_cpldb_test_header.vme", "display_name": "MAC_CPLDB"},
                {"chain": 6, "file": "/etc/.upgrade_test/fcb_cpld_test_header.vme", "display_name": "FCB_CPLD"},
                {"chain": 7, "file": "/etc/.upgrade_test/misc_cpld_test_header.vme", "display_name": "MISC_CPLD"},
                #{"chain": 8, "file": "/etc/.upgrade_test/pcie_cpld_test_header.vme", "display_name": "PCIe_CPLD"},
            ],
        },
    },
}


PLATFORM_E2_CONF = {
    "fan": [
        #{"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/8-0053/eeprom"},
        #{"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/9-0053/eeprom"},
    ],
    "psu": [
        #{"name": "psu1", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0056/eeprom"},
        #{"name": "psu2", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0057/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/0-0056/eeprom"},
    ],
}