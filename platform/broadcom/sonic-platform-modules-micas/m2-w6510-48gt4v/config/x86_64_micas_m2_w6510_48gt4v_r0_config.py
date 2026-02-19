#!/usr/bin/python
#
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
    "drv_update": 0,
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
        "config": "LCMXO3LF-6900C-5BG256C",
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
        "config": "PORT_CPLD",
        "arrt_index": 3,
    },
    "cpld3_version": {
        "key": "Firmware Version",
        "parent": "cpld3",
        "reg": {
            "loc": "/dev/port",
            "offset": 0xb00,
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
            "funcname": "getCustPsu",
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
            "funcname": "getCustPsu",
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
    {"name": "platform_common dfd_my_type=0x40b2", "delay": 0},
    {"name": "wb_pcie_dev", "delay": 0},
    {"name": "wb_lpc_drv", "delay": 0},
    {"name": "wb_lpc_drv_device", "delay": 0},
    {"name": "wb_io_dev", "delay": 0},
    {"name": "wb_io_dev_device", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_i2c_mux_pca9641", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "wb_i2c_mux_pca954x_device", "delay": 0},
    {"name": "wb_i2c_dev_device", "delay": 0},
    {"name": "lm75", "delay": 0},
    {"name": "optoe", "delay": 0},
    {"name": "at24", "delay": 0},
    {"name": "wb_mac_bsc", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "xdpe12284", "delay": 0},
    {"name": "ina3221", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
    {"name": "wb_wdt_device", "delay": 0},
    {"name": "plat_dfd", "delay": 0},
    {"name": "plat_switch", "delay": 0},
    {"name": "plat_fan", "delay": 0},
    {"name": "plat_psu", "delay": 0},
    {"name": "plat_sff", "delay": 0},
    {"name": "hw_test", "delay": 0},
]

DEVICE = [
    {"name": "24c02", "bus": 2, "loc": 0x56},
    {"name": "wb_mac_bsc_td3_x2", "bus": 18, "loc": 0x44},
    # fan
    {"name": "24c02", "bus": 8, "loc": 0x53},
    {"name": "24c02", "bus": 9, "loc": 0x53},

    # psu
    {"name": "24c02", "bus": 7, "loc": 0x56},
    {"name": "24c02", "bus": 7, "loc": 0x57},
    # temp
    {"name": "tmp275", "bus": 6, "loc": 0x48},
    {"name": "tmp275", "bus": 6, "loc": 0x49},
    # dcdc
    {"name": "xdpe12284", "bus": 0, "loc": 0x5e},
    {"name": "xdpe12284", "bus": 0, "loc": 0x68},
    {"name": "xdpe12284", "bus": 0, "loc": 0x6e},
    {"name": "xdpe12284", "bus": 17, "loc": 0x58},
    {"name": "ina3221", "bus": 3, "loc": 0x40},
    {"name": "ina3221", "bus": 3, "loc": 0x41},
    {"name": "ina3221", "bus": 3, "loc": 0x42},
    # port
    {"name": "optoe2", "bus": 11, "loc": 0x50},
    {"name": "optoe2", "bus": 12, "loc": 0x50},
    {"name": "optoe2", "bus": 13, "loc": 0x50},
    {"name": "optoe2", "bus": 14, "loc": 0x50},
]

REBOOT_CTRL_PARAM = {
    "cpu": {"io_addr": 0x910, "rst_val": 0x10, "rst_delay": 0, "gettype": "io"},
    "mac": {"io_addr": 0x930, "rst_val": 0xbf, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
    "phy": {"io_addr": 0x930, "rst_val": 0xf7, "rst_delay": 1, "unlock_rst_val": 0xff, "unlock_rst_delay": 1, "gettype": "io"},
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
                {"id": "tmp275_1", "name": "tmp275", "bus": 6, "loc": 0x48, "attr": "hwmon"},
                {"id": "tmp275_2", "name": "tmp275", "bus": 6, "loc": 0x49, "attr": "hwmon"},
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
                {"id": "ina3221_1", "name": "ina3221", "bus": 3, "loc": 0x40, "attr": "hwmon"},
                {"id": "ina3221_2", "name": "ina3221", "bus": 3, "loc": 0x41, "attr": "hwmon"},
                {"id": "ina3221_3", "name": "ina3221", "bus": 3, "loc": 0x42, "attr": "hwmon"},
            ],
        },
        {
            "name": "xdpe12284",
            "device": [
                {"id": "xdpe12284_1", "name": "xdpe12284", "bus": 0, "loc": 0x68, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "xdpe12284", "bus": 0, "loc": 0x6e, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "xdpe12284", "bus": 0, "loc": 0x5e, "attr": "hwmon"},
                {"id": "xdpe12284_2", "name": "xdpe12284", "bus": 17, "loc": 0x58, "attr": "hwmon"},
            ],
        },
    ],
}

# INIT_PARAM_PRE = [
#     {"loc": "17-0058/hwmon/hwmon*/avs0_vout_max", "value": "875000"},
#     {"loc": "17-0058/hwmon/hwmon*/avs0_vout_min", "value": "800000"},
# ]
INIT_COMMAND_PRE = [
    # close tx_disable
    "dfd_debug io_wr 0xb0e 0x59",
    "dfd_debug io_wr 0xb90 0x00",
    "dfd_debug io_wr 0xb0e 0x4e",
]

INIT_PARAM = []

INIT_COMMAND = []

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
    "devtype": 0x40b2,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 1,
            },
            "chain2": {
                "name": "CTRL_CPLD",
                "is_support_warm_upg": 0,
            },
            "chain3": {
                "name": "PORT_CPLD",
                "is_support_warm_upg": 0,
            },
        },

        "MTD": {
            "chain1": {
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

        "TEST": {
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/cpu_cpld_test_header.vme", "display_name": "CPU_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/ctrl_cpld_test_header.vme", "display_name": "CTRL_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/port_cpld_test_header.vme", "display_name": "PORT_CPLD"},
            ],
        },
    },
}


PLATFORM_E2_CONF = {
    "fan": [
        {"name": "fan1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/8-0053/eeprom"},
        {"name": "fan2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/9-0053/eeprom"},
    ],
    "psu": [
        {"name": "psu1", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0056/eeprom"},
        {"name": "psu2", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0057/eeprom"},
    ],
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/2-0056/eeprom"},
    ],
}

AIR_FLOW_CONF = {
    "psu_fan_airflow": {
        "intake": ['PA150II-F', 'PD150II-F'],
        "exhaust": ['PA150II-R', 'PD150II-R']
    },

    "fanairflow": {
        "intake": ['M1LFAN I-F'],
        "exhaust": ['M1LFAN I-R']
    },

    "fans": [
        {
            "name": "FAN1", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/8-0053/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
        {
            "name": "FAN2", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/9-0053/eeprom",
            "area": "productInfoArea", "field": "productName", "decode": "fanairflow"
        },
    ],

    "psus": [
        {
            "name": "PSU1", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0056/eeprom",
            "field": "product_name", "decode": "psu_fan_airflow"
        },
        {
            "name": "PSU2", "e2_type": "custfru", "e2_path": "/sys/bus/i2c/devices/7-0057/eeprom",
            "field": "product_name", "decode": "psu_fan_airflow"
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
