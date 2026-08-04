#!/usr/bin/python
# -*- coding: UTF-8 -*-
from  ragilecommon import *
PCA9548START  = -1
PCA9548BUSEND = -2


RAGILE_CARDID      = 0x0000404b
RAGILE_PRODUCTNAME = "RA-B6510-32C"

fanlevel = {
    "tips":["LOW","MIDDLE","HIGH"],
    "level":[51,150,255],
    "low_speed":[500,7500,17000],
    "high_speed":[11000,22500,28500]
}

fanloc =[ {"name":"FAN1", "location":"2-000d/fan1_pwm" ,
          "childfans":[{"name":"FRONT ROTOR", "location":"2-000d/fan1_input"},{"name":"BACK ROTOR", "location":"2-000d/fan2_input"} ]},
          {"name":"FAN2", "location":"2-000d/fan3_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-000d/fan3_input"},{"name":"BACK ROTOR", "location":"2-000d/fan4_input"} ]},
          {"name":"FAN3", "location":"2-000d/fan5_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-000d/fan5_input"},{"name":"BACK ROTOR", "location":"2-000d/fan6_input"} ]},
          {"name":"FAN4", "location":"2-000d/fan7_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-000d/fan7_input"},{"name":"BACK ROTOR", "location":"2-000d/fan8_input"} ]},
          {"name":"FAN5", "location":"2-000d/fan9_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-000d/fan9_input"},{"name":"BACK ROTOR", "location":"2-000d/fan10_input"} ]},
         ]


STARTMODULE  = {
                "fancontrol":1,
                "avscontrol":1,
                "avscontrol_restful":0,
                "macledreset":1,
                "dev_monitor": 1,
                    }

DEV_MONITOR_PARAM = {
    "polling_time" : 10,
    "psus": [
        {"name": "psu1",
         "present": {"gettype":"io", "io_addr":0x951,"presentbit": 0,"okval":0},
         "device": [
             {"id": "psu1pmbus", "name": "fsp1200", "bus": 24, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu1frue2", "name": "24c02", "bus": 24, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "psu2",
         "present": {"gettype":"io", "io_addr":0x951,"presentbit": 4,"okval":0},
         "device": [
             {"id": "psu2pmbus", "name": "fsp1200", "bus": 25, "loc": 0x58, "attr": "hwmon"},
             {"id": "psu2frue2", "name": "24c02", "bus": 25, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
    "fans": [
        {"name": "fan1",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 0, "okval": 0},
         "device": [
             {"id": "fan1frue2", "name": "24c02", "bus": 16, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan2",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 1, "okval": 0},
         "device": [
             {"id": "fan2frue2", "name": "24c02", "bus": 17, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan3",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 2, "okval": 0},
         "device": [
             {"id": "fan3frue2", "name": "24c02", "bus": 18, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan4",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 3, "okval": 0},
         "device": [
             {"id": "fan4frue2", "name": "24c02", "bus": 19, "loc": 0x50, "attr": "eeprom"},
         ],
         },
        {"name": "fan5",
         "present": {"gettype": "i2c", "bus": 2, "loc": 0x0d, "offset": 0x30, "presentbit": 4, "okval": 0},
         "device": [
             {"id": "fan5frue2", "name": "24c02", "bus": 20, "loc": 0x50, "attr": "eeprom"},
         ],
         },
    ],
}


MONITOR_TEMP_MIN           = 38
MONITOR_K                  = 11
MONITOR_MAC_IN             = 35
MONITOR_DEFAULT_SPEED      = 0x60
MONITOR_MAX_SPEED          = 0xFF
MONITOR_MIN_SPEED          = 0x33
MONITOR_MAC_ERROR_SPEED    = 0XBB
MONITOR_FAN_TOTAL_NUM      = 5
MONITOR_MAC_UP_TEMP        = 50
MONITOR_MAC_LOWER_TEMP     = -50
MONITOR_MAC_MAX_TEMP       = 100   # 

MONITOR_FALL_TEMP = 4
MONITOR_MAC_WARNING_THRESHOLD =  100 #100
MONITOR_OUTTEMP_WARNING_THRESHOLD = 85
MONITOR_BOARDTEMP_WARNING_THRESHOLD = 85
MONITOR_CPUTEMP_WARNING_THRESHOLD = 85
MONITOR_INTEMP_WARNING_THRESHOLD =  70  #70

MONITOR_MAC_CRITICAL_THRESHOLD = 105  #105
MONITOR_OUTTEMP_CRITICAL_THRESHOLD = 90 #90
MONITOR_BOARDTEMP_CRITICAL_THRESHOLD = 90 #90
MONITOR_CPUTEMP_CRITICAL_THRESHOLD = 100 #100
MONITOR_INTEMP_CRITICAL_THRESHOLD = 80  # 80 
MONITOR_CRITICAL_NUM              = 3
MONITOR_SHAKE_TIME                = 20
MONITOR_INTERVAL                   = 60

MONITOR_SYS_LED = [
    {
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_sys",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
    },
]

MONITOR_SYS_FAN_LED = [
    {
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_fan",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
    },
]
MONITOR_FANS_LED = [
          {"bus":2,"devno":0x0d, "addr":0x3b, "green":0x04, "red":0x02},
          {"bus":2,"devno":0x0d, "addr":0x3c, "green":0x04, "red":0x02},
          {"bus":2,"devno":0x0d, "addr":0x3d, "green":0x04, "red":0x02},
          {"bus":2,"devno":0x0d, "addr":0x3e, "green":0x04, "red":0x02},
          {"bus":2,"devno":0x0d, "addr":0x3f, "green":0x04, "red":0x02}]

E2_LOC = {"bus":1, "devno":0x56}
MAC_LED_RESET = {"pcibus":8, "slot":0, "fn":0, "bar":0, "offset":64, "reset":0x98}

CPLDVERSIONS = [
        {"io_addr": 0x0700, "name": "CPU BOARD CPLD", "gettype": "io"},
        {"io_addr": 0x0900, "name": "CONNECT BOARD CPLD", "gettype": "io"},
        {"bus":2, "devno":0x0d, "name":"CONNECT BOARD CPLD-FAN"},
        {"bus":8, "devno":0x30, "name":"MAC BOARD CPLD_1"},
        {"bus":8, "devno":0x31, "name":"MAC BOARD CPLD_2"},
]

MONITOR_SYS_PSU_LED = [
    {
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_pwr",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
    },
]

MONITOR_FAN_STATUS = [
    {'status':'green' , 'minOkNum':5,'maxOkNum':5},
    {'status':'yellow', 'minOkNum':4,'maxOkNum':4},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':3},
    ]

MONITOR_PSU_STATUS = [
    {'status':'green' , 'minOkNum':2,'maxOkNum':2},
    {'status':'yellow', 'minOkNum':1,'maxOkNum':1},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':0},
    ]


MONITOR_DEV_STATUS = {
    "temperature": [
        {"name":"lm75in",       "location":"/sys/bus/i2c/devices/3-004b/hwmon/*/temp1_input"},
        {"name":"lm75out",      "location":"/sys/bus/i2c/devices/3-004c/hwmon/*/temp1_input"},
        {"name":"lm75hot",      "location":"/sys/bus/i2c/devices/3-0049/hwmon/*/temp1_input"},
        {"name":"cpu",          "location":"/sys/class/hwmon/hwmon0"},
    ],
    "fans": [
        {
            "name":"fan1",
            "presentstatus":{"bus":2, "loc":0x0d, "offset":0x30, 'bit':0},
            "rollstatus": [
                {"name":"motor1","bus":2, "loc":0x0d, "offset":0x31, 'bit':0},
                {"name":"motor2","bus":2, "loc":0x0d, "offset":0x34, 'bit':0},
            ]
        },
        {
            "name":"fan2",
            "presentstatus":{"bus":2, "loc":0x0d, "offset":0x30, 'bit':1},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x0d, "offset":0x31, 'bit':1},
                {"name":"motor2","bus":2, "loc":0x0d, "offset":0x34, 'bit':1},
            ]
        },
        {
            "name":"fan3",
            "presentstatus":{"bus":2, "loc":0x0d, "offset":0x30, 'bit':2},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x0d, "offset":0x31, 'bit':2},
                {"name":"motor2","bus":2, "loc":0x0d, "offset":0x34, 'bit':2},
            ]
        },
        {
            "name":"fan4",
            "presentstatus":{"bus":2, "loc":0x0d, "offset":0x30, 'bit':3},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x0d, "offset":0x31, 'bit':3},
                {"name":"motor2","bus":2, "loc":0x0d, "offset":0x34, 'bit':3},
            ]
        },
        {
            "name":"fan5",
            "presentstatus":{"bus":2, "loc":0x0d, "offset":0x30, 'bit':4},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x0d, "offset":0x31, 'bit':4},
                {"name":"motor2","bus":2, "loc":0x0d, "offset":0x34, 'bit':4},
            ]
        },
    ],
     "psus": [
        {"name":"psu1", "io_addr":0x951, "gettype":"io",  'presentbit': 0,  'statusbit':1, 'alertbit':2},
        {"name":"psu2", "io_addr":0x951, "gettype":"io",  'presentbit': 4,  'statusbit':5, 'alertbit':6},
     ],
    "mac_temp" : {
        "loc" : [
                "3-0044/hwmon/*/temp99_input",
            ],
    },
}

MONITOR_DEV_STATUS_DECODE = {
    'fanpresent':  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'fanroll'   :  {0:'STALL'  , 1:'ROLL',   'okval':1},
    'psupresent':  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'psuoutput' :  {0:'FAULT'  , 1:'NORMAL', 'okval':1},
    'psualert'  :  {0:'FAULT'  , 1:'NORMAL', 'okval':1},
}
###################################################################



MAC_AVS_PARAM ={
    0x72:0x0384,
    0x73:0x037e,
    0x74:0x0378,
    0x75:0x0372,
    0x76:0x036b,
    0x77:0x0365,
    0x78:0x035f,
    0x79:0x0359,
    0x7a:0x0352,
    0x7b:0x034c,
    0x7c:0x0346,
    0x7d:0x0340,
    0x7e:0x0339,
    0x7f:0x0333,
    0x80:0x032d,
    0x81:0x0327,
    0x82:0x0320,
    0x83:0x031a,
    0x84:0x0314,
    0x85:0x030e,
    0x86:0x0307,
    0x87:0x0301,
    0x88:0x02fb,
    0x89:0x02f5,
    0x8A:0x02ee
}

MAC_DEFAULT_PARAM = {
  "type": 1,
  "default":0x7a,
  "loopaddr":0x00,
  "loop":0x00,
  "open":0x00,
  "close":0x40,
  "bus":7,
  "devno":0x64,
  "addr":0x21,
  "protectaddr":0x10,
  "sdkreg":"TOP_AVS_SEL_REG",
  "sdkcmd": "scdcmd",
  "sdkcmdargs": ["-t", 5],
  "sdktype": 0,
  "macregloc":24 ,
  "mask": 0xff
}

DEVICE = []
DRIVERLISTS = []

INIT_PARAM = [
    {
        "name": "mac_power_on",
        "type": "io",
        "offset": 0x994,
        "val": 0x01
    }
]

INIT_COMMAND = [
]
