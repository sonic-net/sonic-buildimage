#!/usr/bin/env python
# -*- coding: UTF-8 -*-
from platform_default_config import *

STARTMODULE = {
    "dev_monitor": 1,
}

DRV_INIT_COMMAND = [
    # Circumvent the bug that the first register of cpld-i2c controller is written to not take effect
    {"gettype": "devfile", "path": "/dev/cpld11", "offset": 0, "value": 0x4e},
    # apml enable
    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x40, "value": 0xbe},
]

INIT_PARAM = [
    # set bmc led green
    {"gettype": "sysfs", "loc": "/sys/s3ip/sysled/bmc_led_status", "value": 0x01},
]

#############start: variables needed to be adapted @platform_driver_late.service binded platform_driver_late.py    ###########
# platform_driver_late.py exec flow:
# step1: load drv according to var@DRIVERLISTS_CHECK, and do pre check before load drv, if check fail, logging log
DRIVERLISTS_CHECK = [
    {
        "name": "apml_sbrmi", "delay": 1,
        "pre_check": {
            "gettype": "sysfs",
            "loc": "/sys/s3ip/system/cpu_board_status",
            "okval": 2,
        },
    },
]
###############end: variables needed to be adapted @platform_driver_late.service binded platform_driver_late.py ###########

DEV_MONITOR_PARAM = {
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x57, "read_len": 1, "presentbit": 1, "okval": 0},
             "device": [
                {"id": "psu1pmbus", "name": "wb_fsp1200", "bus": 67, "loc": 0x58, "attr": "hwmon"},
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x57, "read_len": 1, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "psu2pmbus", "name": "wb_fsp1200", "bus": 68, "loc": 0x58, "attr": "hwmon"},
            ],
        },
        {
            "name": "psu3",
            "present": {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x58, "read_len": 1, "presentbit": 1, "okval": 0},
            "device": [
                {"id": "psu3pmbus", "name": "wb_fsp1200", "bus": 69, "loc": 0x58, "attr": "hwmon"},
            ],
        },
        {
            "name": "psu4",
            "present": {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x58, "read_len": 1, "presentbit": 5, "okval": 0},
            "device": [
                {"id": "psu4pmbus", "name": "wb_fsp1200", "bus": 70, "loc": 0x58, "attr": "hwmon"},
            ],
        },
    ],
    "others": [
        {
            "name": "eeprom",
            "device": [
                {"id": "eeprom_1", "name": "24c02", "bus": 1, "loc": 0x50, "attr": "eeprom"},
                {"id": "eeprom_2", "name": "24c02", "bus": 2, "loc": 0x53, "attr": "eeprom"},
                {"id": "eeprom_3", "name": "24c02", "bus": 2, "loc": 0x57, "attr": "eeprom"},
                {"id": "eeprom_4", "name": "24c02", "bus": 8, "loc": 0x56, "attr": "eeprom"},
            ],
        },
    ]
}

DRIVERLISTS = [
    {"name": "platform_common dfd_my_type=0x414e", "delay": 0},
    {"name": "wb_logic_dev_common", "delay": 0},
    {"name": "hw_test", "delay": 0},
    {"name": "wb_spi_dev", "delay": 0},
    {"name": "wb_indirect_dev", "delay": 0},
    {"name": "wb_i2c_dev", "delay": 0},
    {"name": "wb_fpga_i2c_bus_drv", "delay": 0},
    {"name": "wb_fpga_pca954x_drv", "delay": 0},
    {"name": "wb_ast2700_mac1_rgmii_fix", "delay": 0},
    {"name": "wb_i2c_mux_pca954x", "delay": 0},
    {"name": "pmbus_core", "delay": 0},
    {"name": "wb_csu550", "delay": 0},
    {"name": "firmware_driver_ispvme", "delay": 0},
    {"name": "firmware_driver_sysfs", "delay": 0},
    {"name": "wb_wdt", "delay": 0},
#    {"name": "wb_dev_resume", "delay": 0},
    {"name": "s3ip_sysfs", "delay": 0},
    {"name": "wb_switch_driver", "delay": 0},
    {"name": "cpld_device_driver", "delay": 0},
    {"name": "system_device_driver", "delay": 0},
    {"name": "syseeprom_device_driver", "delay": 0},
    {"name": "sysled_device_driver", "delay": 0},
    {"name": "fpga_device_driver", "delay": 0},
    {"name": "sol_device_driver", "delay": 0},
]

DEVICE = [
    # eeprom
    {"name": "24c02", "bus": 1, "loc": 0x50},
    {"name": "24c02", "bus": 2, "loc": 0x53},
    {"name": "24c02", "bus": 2, "loc": 0x57},
    {"name": "24c02", "bus": 8, "loc": 0x56},
    # psu
    {
        "init": [
            {"gettype": "devfile", "path": "/dev/fpga0", "offset": 0x9c, "value": 0x02},
        ],
        "name": "wb_psu", "bus": 67, "loc": 0x58, "delay": 0.1,
    },
    {"name": "wb_psu", "bus": 68, "loc": 0x58},
    {"name": "wb_psu", "bus": 69, "loc": 0x58},
    {"name": "wb_psu", "bus": 70, "loc": 0x58},
]

UPGRADE_SUMMARY = {
    "devtype": 0x414e,

    "slot0": {
        "subtype": 0,
        "VME": {
            "chain1": {
                "name": "CPU_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x01},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain2": {
                "name": "MGMT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    #{"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    #{"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                ],
                "finish_cmd": [
                    #{"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    {"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    #{"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    #{"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain3": {
                "name": "MAC_CPLD_AB",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x44, "value": 0xba},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x1},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x44, "value": 0xbb},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain4": {
                "name": "MAC_CPLD_C",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x43, "value": 0xbc},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x43, "value": 0xbd},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain5": {
                "name": "UPORT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x47, "value": 0xb8},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x6},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x47, "value": 0xb9},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain6": {
                "name": "DPORT_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x49, "value": 0xb6},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x8},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x49, "value": 0xb7},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain7": {
                "name": "UFAN_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4a, "value": 0xb4},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x4},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4a, "value": 0xb5},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain8": {
                "name": "DFAN_CPLD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4b, "value": 0xb4},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x5},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4b, "value": 0xb5},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
            "chain9": {
                "name": "MAC_CPLDD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x02},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x0a},
                ],
                "finish_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
        },

        "SYSFS": {
            "chain8": {
                "name": "BCM53134",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "modprobe wb_spi_93xx46 spi_bus_num=4 eeprom_name=gt93c56a", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": " rmmod wb_spi_93xx46", "gettype": "cmd", "delay": 0.1},
                ],
            },
        },

        "SPI-LOGIC-DEV": {
            "chain1": {
                "name": "MAC_FPGA",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 1,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xba]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x03]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xbb]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain2": {
                "name": "MAC_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 2,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xba]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x03]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xbb]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain3": {
                "name": "UPROT_FPGA",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 3,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb8]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x07]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb9]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain4": {
                "name": "UPROT_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 4,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb8]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x07]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb9]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain5": {
                "name": "DPROT_FPGA",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 5,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb6]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x09]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb7]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain6": {
                "name": "DPROT_FPGA_SHAOPIAN",
                "is_support_warm_upg": 0,
                "upgrade_way": 1, # UPDATE_HEADER
                "new_file_type": "MTD",
                "new_chain": 6,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb6]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x09]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb7]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
        },

        "MTD": {
            "chain1": {
                "name": "MAC_FPGA_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xba]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x03]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xbb]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain2": {
                "name": "MAC_FPGA_SHAOPIAN_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xba]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x03]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x45, "value": [0xbb]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain3": {
                "name": "UPROT_FPGA_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb8]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x07]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb9]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain4": {
                "name": "UPROT_FPGA_SHAOPIAN_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb8]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x07]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x46, "value": [0xb9]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain5": {
                "name": "DPROT_FPGA_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb6]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x09]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb7]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain6": {
                "name": "DPROT_FPGA_SHAOPIAN_MTD",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x02]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb6]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb2]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xad]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x09]},
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": [0x00]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x48, "value": [0xb7]},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": [0xb3]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": [0xac]},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": [0x00]},
                ],
            },
            "chain7": [
                {
                    "chip_select": "master",
                    "name": "Master_BIOS",
                    "is_support_warm_upg": 0,
                    "init_cmd": [
                        # switch master BIOS flash to BMC
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0xef]},
                        # Adjust log printing level
                        {"cmd": "echo 4 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # bind scm
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/bind",
                            "gettype": "cmd", "delay":0.1
                        },
                    ],
                    "finish_cmd": [
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # Restore log printing level
                        {"cmd": "echo 7 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # switch BIOS flash to CPU,
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0x1e]},
                    ],
                },
                {
                    "chip_select": "slave",
                    "name": "Slave_BIOS",
                    "is_support_warm_upg": 0,
                    "init_cmd": [
                        # switch master BIOS flash to BMC
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0xf7]},
                        # Adjust log printing level
                        {"cmd": "echo 4 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # bind scm
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/bind",
                            "gettype": "cmd", "delay":0.1
                        },
                    ],
                    "finish_cmd": [
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # Restore log printing level
                        {"cmd": "echo 7 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # switch BIOS flash to CPU
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0x1e]},
                    ],
                },
                {
                    "chip_select": "force_master",
                    "traverse_skip": 1,
                    "name": "Force_master_BIOS",
                    "is_support_warm_upg": 0,
                    "init_cmd": [
                        # switch master BIOS flash to BMC
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0xef]},
                        # Adjust log printing level
                        {"cmd": "echo 4 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # bind scm
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/bind",
                            "gettype": "cmd", "delay":0.1
                        },
                    ],
                    "finish_cmd": [
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # Restore log printing level
                        {"cmd": "echo 7 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # switch BIOS flash to CPU
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0x1e]},
                    ],
                },
                {
                    "chip_select": "force_slave",
                    "traverse_skip": 1,
                    "name": "Force_slave_BIOS",
                    "is_support_warm_upg": 0,
                    "init_cmd": [
                        # switch master BIOS flash to BMC
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0xf7]},
                        # Adjust log printing level
                        {"cmd": "echo 4 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # bind scm
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/bind",
                            "gettype": "cmd", "delay":0.1
                        },
                    ],
                    "finish_cmd": [
                        # unbind scm if needed
                        {
                            "cmd": "echo spi1.1 > /sys/bus/spi/drivers/spi-nor/unbind",
                            "gettype": "cmd",
                            "pre_check": {
                                "gettype": "file_exist",
                                "judge_file": "/sys/bus/spi/drivers/spi-nor/spi1.1",
                                "okval": True
                            }
                        },
                        # Restore log printing level
                        {"cmd": "echo 7 4 1 7 > /proc/sys/kernel/printk", "gettype": "cmd"},
                        # switch BIOS flash to CPU
                        {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x79, "value": [0x1e]},
                    ],
                },
            ],
            "chain9": {
                "name": "MAC_PCIE",
                "is_support_warm_upg": 0,
                "init_cmd": [
                    {"cmd": "echo 582 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio582/direction", "gettype": "cmd", "delay": 0.1},
                    {"cmd": "echo 579 > /sys/class/gpio/export", "gettype": "cmd"},
                    {"cmd": "echo high > /sys/class/gpio/gpio579/direction", "gettype": "cmd", "delay": 0.1},
                    #{"cmd": "echo 580 > /sys/class/gpio/export", "gettype": "cmd"},
                    #{"cmd": "echo low > /sys/class/gpio/gpio580/direction", "gettype": "cmd", "delay": 0.1},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb2},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x2},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x2},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4e, "value": 0xb1},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xad},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0x2},
                    {"cmd": "modprobe wb_spi_gpio_device miso=188 mosi=189 sck=186 cs=187 gpio_chip_name=14c0b000.gpio bus=5", "gettype": "cmd"},
                    {"cmd": "modprobe wb_spi_nor_device spi_chip_select=0 spi_bus_num=5", "gettype": "cmd", "delay": 0.1},
                ],
                "finish_cmd": [
                    {"cmd": "rmmod wb_spi_nor_device", "gettype": "cmd"},
                    {"cmd": "rmmod wb_spi_gpio_device", "gettype": "cmd"},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x54, "value": 0},
                    {"gettype": "devfile", "path": "/dev/cpld4", "offset": 0x53, "value": 0xac},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4e, "value": 0xb0},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0},
                    {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4d, "value": 0xb3},
                    #{"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio580/value"},
                    #{"gettype": "cmd", "cmd": "echo 580 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio579/value"},
                    {"gettype": "cmd", "cmd": "echo 579 > /sys/class/gpio/unexport"},
                    {"gettype": "cmd", "cmd": "echo 0 > /sys/class/gpio/gpio582/value"},
                    {"gettype": "cmd", "cmd": "echo 582 > /sys/class/gpio/unexport"},
                ],
            },
        },

        "TEST": {
            "fpga": [
                {
                    "chain": 1,
                    "file": "/etc/.upgrade_test/0x414e/mac_fpga_test_header.bin",
                    "display_name": "MAC_FPGA",
                },
                {
                    "chain": 3,
                    "file": "/etc/.upgrade_test/0x414e/uport_fpga_test_header.bin",
                    "display_name": "UPORT_FPGA",
                },
                {
                    "chain": 5,
                    "file": "/etc/.upgrade_test/0x414e/dport_fpga_test_header.bin",
                    "display_name": "DPORT_FPGA",
                },
            ],
            "cpld": [
                {"chain": 1, "file": "/etc/.upgrade_test/0x414e/cpu_cpld_test_header.vme", "display_name": "CPU_CPLD"},
                {"chain": 2, "file": "/etc/.upgrade_test/0x414e/mgmt_cpld_test_header.vme", "display_name": "MGMT_CPLD"},
                {"chain": 3, "file": "/etc/.upgrade_test/0x414e/mac_cpldab_test_header.vme", "display_name": "MAC_CPLD_AB"},
                {"chain": 4, "file": "/etc/.upgrade_test/0x414e/mac_cpldc_test_header.vme", "display_name": "MAC_CPLD_C"},
                {"chain": 5, "file": "/etc/.upgrade_test/0x414e/uport_cpld_test_header.vme", "display_name": "UPORT_CPLD"},
                {"chain": 6, "file": "/etc/.upgrade_test/0x414e/dport_cpld_test_header.vme", "display_name": "DPORT_CPLD"},
                {"chain": 7, "file": "/etc/.upgrade_test/0x414e/ufan_cpld_test_header.vme", "display_name": "UFAN_CPLD"},
                {"chain": 8, "file": "/etc/.upgrade_test/0x414e/dfan_cpld_test_header.vme", "display_name": "DFAN_CPLD"},
                {"chain": 9, "file": "/etc/.upgrade_test/0x414e/mac_cpldd_test_header.vme", "display_name": "MAC_CPLD_D"},
            ],
            "bios": [
                {"chain": 7, "file": "/etc/.upgrade_test/0x414e/bios_test_header.bin", "display_name": "Master_BIOS", "chip_select": "master",},
                {"chain": 7, "file": "/etc/.upgrade_test/0x414e/bios_test_header.bin", "display_name": "Slave_BIOS", "chip_select": "slave",},
            ],
            "mac": [
                {"chain": 9, "file": "/etc/.upgrade_test/0x414e/mac_test_header.bin", "display_name": "MAC_PCIE"},
            ],
        },
    },
}

MANUINFO_CONF = {
    "version2": 1,

    "CPLD": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "cpld",
    },

    "FPGA": {
        "class" : 1,
        "source" : "s3ip",
        "type" : "fpga",
    },

    "OTHERS" : {
        "child" : {
            "CPU-BMC-SWITCH" : {
                "Device Model" : {"gettype": "direct_config", "value": "BCM53134O"},
                "Vendor" : {"gettype": "direct_config", "value": "Broadcom"},
                "Hardware Version" : {
                    "gettype": "func", 
                    "funcname": "get_and_generate_mgmt_version",
                    "params": {
                        "before": [
                            {"gettype": "cmd", "cmd": "echo 555 > /sys/class/gpio/export"},
                            {"gettype": "cmd", "cmd": "echo low > /sys/class/gpio/gpio555/direction"},
                            #enable 53134 update
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x02},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x05},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0xb2},
                            {"gettype": "cmd", "cmd": "modprobe wb_spi_93xx46 spi_bus_num=4 eeprom_name=gt93c56a","delay": 0.1},
                        ],
                        "after": [
                            {"gettype": "cmd", "cmd": "echo high > /sys/class/gpio/gpio555/direction"},
                            {"gettype": "cmd", "cmd": "echo 555 > /sys/class/gpio/unexport"},
                        ],
                        "get_version": "md5sum /sys/bus/spi/devices/spi4.0/eeprom | awk '{print $1}'",
                        "finally": [
                            {"gettype": "cmd", "cmd": "rmmod wb_spi_93xx46"},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x4c, "value": 0xb3},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x55, "value": 0x00},
                            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "value": 0x00},
                        ],
                    },
                },
            },
        },
    },
}

PLATFORM_E2_CONF = {
    "syseeprom": [
        {"name": "syseeprom", "e2_type": "onie_tlv", "e2_path": "/sys/bus/i2c/devices/2-0057/eeprom"},
    ],
    "bmc": [
        {"name": "BMC card", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/8-0056/eeprom"},
    ],
    "cpu": [
        {"name": "CPU board", "e2_type": "fru", "e2_path": "/sys/bus/i2c/devices/1-0050/eeprom"},
    ]
}
