#!/usr/bin/python
# -*- coding: UTF-8 -*-
from ragilecommon import *
PCA9548START  = -1
PCA9548BUSEND = -2

RAGILE_CARDID      = 0x0000404c
RAGILE_PRODUCTNAME = "RA-B6910-64C"

fanlevel = {
    "tips": ["LOW", "MEDIUM", "HIGH"],
    "level": [75, 150, 255],
    "low_speed": [750, 4250, 6750],
    "high_speed": [4500, 7500, 10000],
}

# fit with pddf
fanloc = [
    {
        "name": "FAN1/FAN2/FAN3/FAN4",
        "location": "2-0066/fan1_pwm",
        "childfans": [
            {"name": "FAN1", "location": "2-0066/fan1_input"},
            {"name": "FAN2", "location": "2-0066/fan2_input"},
            {"name": "FAN3", "location": "2-0066/fan3_input"},
            {"name": "FAN4", "location": "2-0066/fan4_input"},
        ],
    },
]

CPLDVERSIONS = [
    {"bus": 2, "devno": 0x33, "name": "MAC BOARD CPLD-A"},
    {"bus": 2, "devno": 0x35, "name": "MAC BOARD CPLD-B"},
    {"bus": 2, "devno": 0x37, "name": "CONNECT BOARD CPLD-A"},
    {"bus": 0, "devno": 0x0d, "name": "CPU BOARD CPLD"},
]


MONITOR_TEMP_MIN           = 34
MONITOR_K                  = 14
MONITOR_MAC_IN             = 35
MONITOR_DEFAULT_SPEED      = 0x80
MONITOR_MAX_SPEED          = 0xFF
MONITOR_MIN_SPEED          = 0x33
MONITOR_MAC_ERROR_SPEED    = 0XBB
MONITOR_FAN_TOTAL_NUM      = 3
MONITOR_MAC_UP_TEMP        = 40
MONITOR_MAC_LOWER_TEMP     = -40
MONITOR_MAC_MAX_TEMP       = 100

MONITOR_FALL_TEMP = 2
MONITOR_MAC_WARNING_THRESHOLD        = 100
MONITOR_OUTTEMP_WARNING_THRESHOLD    = 85
MONITOR_BOARDTEMP_WARNING_THRESHOLD  = 85
MONITOR_CPUTEMP_WARNING_THRESHOLD    = 85
MONITOR_INTEMP_WARNING_THRESHOLD     = 70

MONITOR_MAC_CRITICAL_THRESHOLD       = 105
MONITOR_OUTTEMP_CRITICAL_THRESHOLD   = 90
MONITOR_BOARDTEMP_CRITICAL_THRESHOLD = 90
MONITOR_CPUTEMP_CRITICAL_THRESHOLD   = 100
MONITOR_INTEMP_CRITICAL_THRESHOLD    = 80
MONITOR_CRITICAL_NUM                 = 2
MONITOR_SHAKE_TIME                   = 10
MONITOR_INTERVAL                     = 60


MONITOR_SYS_LED = [{"bus": 2, "devno": 0x35, "addr": 0xb2, "yellow": 0x03, "red": 0x02, "green": 0x01}]

MONITOR_SYS_FAN_LED =[
    {"bus": 2, "devno": 0x35, "addr": 0xb4, "yellow": 0x06, "red": 0x02, "green": 0x04},
]

MONITOR_FANS_LED = [
    {"bus": 2, "devno": 0x32, "addr": 0x23, "green": 0x09, "red": 0x0a},
    {"bus": 2, "devno": 0x32, "addr": 0x24, "green": 0x09, "red": 0x0a},
    {"bus": 2, "devno": 0x32, "addr": 0x25, "green": 0x09, "red": 0x0a}
]

MONITOR_SYS_PSU_LED = [
    {"bus": 2, "devno": 0x35, "addr": 0xb3, "yellow": 0x06, "red": 0x02, "green": 0x04},
]

MONITOR_FAN_STATUS = [
    {'status': 'green' , 'minOkNum': 3, 'maxOkNum': 3},
    {'status': 'yellow', 'minOkNum': 2, 'maxOkNum': 2},
    {'status': 'red'   , 'minOkNum': 0, 'maxOkNum': 1},
]

MONITOR_PSU_STATUS = [
    {'status': 'green' , 'minOkNum': 2, 'maxOkNum': 2},
    {'status': 'yellow', 'minOkNum': 1, 'maxOkNum': 1},
    {'status': 'red'   , 'minOkNum': 0, 'maxOkNum': 0},
]

MONITOR_DEV_STATUS = {
    "temperature": [
        {"name": "lm75in",       "location": "/sys/bus/i2c/devices/2-0048/hwmon/*/temp1_input"},
        {"name": "lm75out",      "location": "/sys/bus/i2c/devices/2-0049/hwmon/*/temp1_input"},
        {"name": "lm75hot",      "location": "/sys/bus/i2c/devices/2-004a/hwmon/*/temp1_input"},
        {"name": "cpu",          "location": "/sys/class/hwmon/hwmon0"},
    ],
    "fans": [
        {
            "name": "fan1",
            "presentstatus": {"bus": 2, "loc": 0x37, "offset": 0x30, 'bit': 0},
            "rollstatus": [
                {"name": "motor1", "bus": 2, "loc": 0x37, "offset": 0x31, 'bit': 0},
            ]
        },
        {
            "name": "fan2",
            "presentstatus": {"bus": 2, "loc": 0x37, "offset": 0x30, 'bit': 1},
            "rollstatus": [
                {"name": "motor1", "bus": 2, "loc": 0x37, "offset": 0x31, 'bit': 1},
            ]
        },
        {
            "name": "fan3",
            "presentstatus": {"bus": 2, "loc": 0x37, "offset": 0x30, 'bit': 2},
            "rollstatus": [
                {"name": "motor1", "bus": 2, "loc": 0x37, "offset": 0x31, 'bit': 2},
            ]
        },
    ],
    "psus": [
        {"name": "psu1", "bus": 2, "loc": 0x37, "offset": 0x51, "gettype": "i2c", 'presentbit': 0, 'statusbit': 1, 'alertbit': 2},
        {"name": "psu2", "bus": 2, "loc": 0x37, "offset": 0x51, "gettype": "i2c", 'presentbit': 4, 'statusbit': 5, 'alertbit': 6},
    ],
}

MONITOR_DEV_STATUS_DECODE = {
    'fanpresent': {0: 'PRESENT', 1: 'ABSENT', 'okval': 0},
    'fanroll'   : {0: 'STALL'  , 1: 'ROLL',   'okval': 1},
    'psupresent': {0: 'PRESENT', 1: 'ABSENT', 'okval': 0},
    'psuoutput' : {0: 'FAULT'  , 1: 'NORMAL', 'okval': 1},
    'psualert'  : {0: 'FAULT'  , 1: 'NORMAL', 'okval': 1},
}
###################################################################


MAC_AVS_PARAM = {
    0x68: 0x03c0,
    0x69: 0x03bc,
    0x6a: 0x03b6,
    0x6b: 0x03b0,
    0x6c: 0x03aa,
    0x6d: 0x03a3,
    0x6e: 0x039d,
    0x6f: 0x0397,
    0x70: 0x0391,
    0x71: 0x038a,
    0x72: 0x0384,
    0x73: 0x037e,
    0x74: 0x0379,
    0x75: 0x0371,
    0x76: 0x036b,
    0x77: 0x0365,
    0x78: 0x035f,
    0x79: 0x0358,
    0x7a: 0x0352,
    0x7b: 0x034c,
    0x7c: 0x0348
}

MAC_DEFAULT_PARAM = {
    "type":  1,
    "default": 0x74,
    "loopaddr": 0x00,
    "loop": 0x00,
    "open": 0x00,
    "close": 0x40,
    "bus": 2,
    "devno": 0x60,
    "addr": 0x21,
    "protectaddr": 0x10,
    "sdkreg": "DMU_PCU_OTP_CONFIG_8",
    "sdkcmd":  "scdcmd",
    "sdkcmdargs":  ["-t", 5],
    "sdktype":  1,
    "macregloc": 24,
    "mask":  0xff
}


DEVICE = []
DRIVERLISTS = []

"""
DRIVERLISTS = [
        {"name": "i2c_dev", "delay": 0},
        {"name": "i2c_algo_bit", "delay": 0},
        {"name": "i2c_gpio", "delay": 0},
        {"name": "i2c_mux", "delay": 0},
        {"name": "i2c_mux_pca9641", "delay": 0},
        {"name": "i2c_mux_pca954x force_create_bus=1", "delay": 0},  # force_deselect_on_exit=1
        {"name": "lm75", "delay": 0},
        {"name": "optoe", "delay": 0},
        {"name": "at24", "delay": 0},
        {"name": "rg_sff", "delay": 0},
        {"name": "ragile_b6510_platform", "delay": 0},
        {"name": "ragile_platform", "delay": 0},
        {"name": "rg_avs", "delay": 0},
        {"name": "rg_cpld", "delay": 0},
        {"name": "rg_fan", "delay": 0},
        {"name": "rg_psu", "delay": 0},
        {"name": "pmbus_core", "delay": 0},
        {"name": "csu550", "delay": 0},
        {"name": "rg_gpio_xeon", "delay": 0},
        {"name": "firmware_driver", "delay": 0},
        {"name": "firmware_bin", "delay": 0},
        {"name": "ragile_common dfd_my_type=0x404c", "delay": 0},
        {"name": "lpc_dbg", "delay": 0},
]

DEVICE = [
        {"name": "pca9641", "bus": 0, "loc": 0x10},
        {"name": "pca9548", "bus": 2, "loc": 0x70},
        {"name": "lm75",    "bus": 2, "loc": 0x48},
        {"name": "lm75",    "bus": 2, "loc": 0x49},
        {"name": "lm75",    "bus": 2, "loc": 0x4a},
        {"name": "24c02",   "bus": 2, "loc": 0x57},
        {"name": "rg_cpld", "bus": 0, "loc": 0x32},
        {"name": "rg_cpld", "bus": 1, "loc": 0x34},
        {"name": "rg_cpld", "bus": 1, "loc": 0x36},
        {"name": "rg_cpld", "bus": 2, "loc": 0x33},
        {"name": "rg_cpld", "bus": 2, "loc": 0x35},
        {"name": "rg_cpld", "bus": 2, "loc": 0x37},
        {"name": "rg_avs",  "bus": 2, "loc": 0x60},
        {"name": "pca9548", "bus": 1, "loc": 0x70},
        {"name": "pca9548", "bus": 1, "loc": 0x71},
        {"name": "pca9548", "bus": 1, "loc": 0x72},
        {"name": "pca9548", "bus": 1, "loc": 0x73},
        {"name": "pca9548", "bus": 1, "loc": 0x74},
        {"name": "pca9548", "bus": 1, "loc": 0x75},
        {"name": "pca9548", "bus": 1, "loc": 0x76},
        {"name": "pca9548", "bus": 1, "loc": 0x77},
        {"name": "rg_fan",  "bus": 3, "loc": 0x53},
        {"name": "rg_fan",  "bus": 4, "loc": 0x53},
        {"name": "rg_fan",  "bus": 5, "loc": 0x53},

        {"name": "rg_psu",  "bus": 7, "loc": 0x50},
        {"name": "dps550",  "bus": 7, "loc": 0x58},
        {"name": "rg_psu",  "bus": 8, "loc": 0x53},
        {"name": "dps550",  "bus": 8, "loc": 0x5b},

        {"name": "optoe1", "bus": 11, "loc": 0x50},
        {"name": "optoe1", "bus": 12, "loc": 0x50},
        {"name": "optoe1", "bus": 13, "loc": 0x50},
        {"name": "optoe1", "bus": 14, "loc": 0x50},
        {"name": "optoe1", "bus": 15, "loc": 0x50},
        {"name": "optoe1", "bus": 16, "loc": 0x50},
        {"name": "optoe1", "bus": 17, "loc": 0x50},
        {"name": "optoe1", "bus": 18, "loc": 0x50},
        {"name": "optoe1", "bus": 19, "loc": 0x50},
        {"name": "optoe1", "bus": 20, "loc": 0x50},
        {"name": "optoe1", "bus": 21, "loc": 0x50},
        {"name": "optoe1", "bus": 22, "loc": 0x50},
        {"name": "optoe1", "bus": 23, "loc": 0x50},
        {"name": "optoe1", "bus": 24, "loc": 0x50},
        {"name": "optoe1", "bus": 25, "loc": 0x50},
        {"name": "optoe1", "bus": 26, "loc": 0x50},
        {"name": "optoe1", "bus": 27, "loc": 0x50},
        {"name": "optoe1", "bus": 28, "loc": 0x50},
        {"name": "optoe1", "bus": 29, "loc": 0x50},
        {"name": "optoe1", "bus": 30, "loc": 0x50},
        {"name": "optoe1", "bus": 31, "loc": 0x50},
        {"name": "optoe1", "bus": 32, "loc": 0x50},
        {"name": "optoe1", "bus": 33, "loc": 0x50},
        {"name": "optoe1", "bus": 34, "loc": 0x50},
        {"name": "optoe1", "bus": 35, "loc": 0x50},
        {"name": "optoe1", "bus": 36, "loc": 0x50},
        {"name": "optoe1", "bus": 37, "loc": 0x50},
        {"name": "optoe1", "bus": 38, "loc": 0x50},
        {"name": "optoe1", "bus": 39, "loc": 0x50},
        {"name": "optoe1", "bus": 40, "loc": 0x50},
        {"name": "optoe1", "bus": 41, "loc": 0x50},
        {"name": "optoe1", "bus": 42, "loc": 0x50},
        {"name": "optoe1", "bus": 43, "loc": 0x50},
        {"name": "optoe1", "bus": 44, "loc": 0x50},
        {"name": "optoe1", "bus": 45, "loc": 0x50},
        {"name": "optoe1", "bus": 46, "loc": 0x50},
        {"name": "optoe1", "bus": 47, "loc": 0x50},
        {"name": "optoe1", "bus": 48, "loc": 0x50},
        {"name": "optoe1", "bus": 49, "loc": 0x50},
        {"name": "optoe1", "bus": 50, "loc": 0x50},
        {"name": "optoe1", "bus": 51, "loc": 0x50},
        {"name": "optoe1", "bus": 52, "loc": 0x50},
        {"name": "optoe1", "bus": 53, "loc": 0x50},
        {"name": "optoe1", "bus": 54, "loc": 0x50},
        {"name": "optoe1", "bus": 55, "loc": 0x50},
        {"name": "optoe1", "bus": 56, "loc": 0x50},
        {"name": "optoe1", "bus": 57, "loc": 0x50},
        {"name": "optoe1", "bus": 58, "loc": 0x50},
        {"name": "optoe1", "bus": 59, "loc": 0x50},
        {"name": "optoe1", "bus": 60, "loc": 0x50},
        {"name": "optoe1", "bus": 61, "loc": 0x50},
        {"name": "optoe1", "bus": 62, "loc": 0x50},
        {"name": "optoe1", "bus": 63, "loc": 0x50},
        {"name": "optoe1", "bus": 64, "loc": 0x50},
        {"name": "optoe1", "bus": 65, "loc": 0x50},
        {"name": "optoe1", "bus": 66, "loc": 0x50},
        {"name": "optoe1", "bus": 67, "loc": 0x50},
        {"name": "optoe1", "bus": 68, "loc": 0x50},
        {"name": "optoe1", "bus": 69, "loc": 0x50},
        {"name": "optoe1", "bus": 70, "loc": 0x50},
        {"name": "optoe1", "bus": 71, "loc": 0x50},
        {"name": "optoe1", "bus": 72, "loc": 0x50},
        {"name": "optoe1", "bus": 73, "loc": 0x50},
        {"name": "optoe1", "bus": 74, "loc": 0x50},
]

INIT_PARAM = [
            {"loc": "1-0034/sfp_enable", "value": "01"},
            {"loc": "2-0035/sfp_enable2", "value": "ff"},
            {"loc": "2-0033/mac_led", "value": "ff"},
            {"loc": "1-0034/sfp_txdis1", "value": "00"},
            {"loc": "1-0034/sfp_txdis2", "value": "00"},
            {"loc": "1-0034/sfp_txdis3", "value": "00"},
            {"loc": "1-0036/sfp_txdis4", "value": "00"},
            {"loc": "1-0036/sfp_txdis5", "value": "00"},
            {"loc": "1-0036/sfp_txdis6", "value": "00"},

            {"loc": "1-0034/sfp_led1_yellow", "value": "00"},
            {"loc": "1-0034/sfp_led3_yellow", "value": "00"},
            {"loc": "1-0036/sfp_led4_yellow", "value": "00"},
            {"loc": "1-0036/sfp_led6_yellow", "value": "00"},
            {"loc": "1-0034/sfp_led2_yellow", "value": "00"},
            {"loc": "1-0034/sfp_led8_yellow", "value": "00"},
            {"loc": "1-0036/sfp_led5_yellow", "value": "00"},
            {"loc": "1-0036/sfp_led7_yellow", "value": "00"},
]
"""

INIT_PARAM = [
    {
        "name": "sfp_enable",
        "bus": 1,
        "devaddr": 0x34,
        "offset": 0xa1,
        "val": 0x01,
    },
    {
        "name": "sfp_eanble2",
        "bus": 2,
        "devaddr": 0x35,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "mac_led",
        "bus": 2,
        "devaddr": 0x33,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led1_yellow",
        "bus": 1,
        "devaddr": 0x34,
        "offset": 0xa6,
        "val": 0x00,
    },
    {
        "name": "sfp_led2_yellow",
        "bus": 1,
        "devaddr": 0x34,
        "offset": 0xa7,
        "val": 0x00,
    },
    {
        "name": "sfp_led3_yellow",
        "bus": 1,
        "devaddr": 0x34,
        "offset": 0xa8,
        "val": 0x00,
    },
    {
        "name": "sfp_led4_yellow",
        "bus": 1,
        "devaddr": 0x36,
        "offset": 0xa6,
        "val": 0x00,
    },
    {
        "name": "sfp_led5_yellow",
        "bus": 1,
        "devaddr": 0x36,
        "offset": 0xa7,
        "val": 0x00,
    },
    {
        "name": "sfp_led6_yellow",
        "bus": 1,
        "devaddr": 0x36,
        "offset": 0xa8,
        "val": 0x00,
    },
    {
        "name": "sfp_led7_yellow",
        "bus": 1,
        "devaddr": 0x36,
        "offset": 0xa9,
        "val": 0x00,
    },
    {
        "name": "sfp_led8_yellow",
        "bus": 1,
        "devaddr": 0x34,
        "offset": 0xa8,
        "val": 0x00,
    },
    {
        "name": "fan_speed_set",
        "bus": 0,
        "devaddr": 0x32,
        "offset": 0x15,
        "val": 0x80,
    },
]
