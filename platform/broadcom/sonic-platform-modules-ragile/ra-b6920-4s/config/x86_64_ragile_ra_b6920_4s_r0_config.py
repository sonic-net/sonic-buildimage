#!/usr/bin/python
# -*- coding: UTF-8 -*-
from ragilecommon import *
PCA9548START  = -1
PCA9548BUSEND = -2

RAGILE_CARDID      = 0x0000404d
RAGILE_PRODUCTNAME = "RA-B6920-4S"

STARTMODULE = {
    "fancontrol": 1,
    "avscontrol": 1,
    "sfptempmodule": 0,
    "sfptempmodule_interval": 3,
    "slot_monitor": 1,
    "dev_monitor": 1,
}


DEV_MONITOR_PARAM = {
    "polling_time": 5,
    "psus": [
        {
            "name": "psu1",
            "present": {"gettype": "io", "io_addr": 0xB27, "presentbit": 0, "okval": 0},
            "device": [
                {
                    "id": "psu1pmbus",
                    "name": "fsp1200",
                    "bus": 23,
                    "loc": 0x58,
                    "attr": "hwmon",
                },
            ],
        },
        {
            "name": "psu2",
            "present": {"gettype": "io", "io_addr": 0xB28, "presentbit": 0, "okval": 0},
            "device": [
                {
                    "id": "psu2pmbus",
                    "name": "fsp1200",
                    "bus": 25,
                    "loc": 0x58,
                    "attr": "hwmon",
                },
            ],
        },
        {
            "name": "psu3",
            "present": {"gettype": "io", "io_addr": 0xB29, "presentbit": 0, "okval": 0},
            "device": [
                {
                    "id": "psu3pmbus",
                    "name": "fsp1200",
                    "bus": 24,
                    "loc": 0x58,
                    "attr": "hwmon",
                },
            ],
        },
        {
            "name": "psu4",
            "present": {"gettype": "io", "io_addr": 0xB2A, "presentbit": 0, "okval": 0},
            "device": [
                {
                    "id": "psu4pmbus",
                    "name": "fsp1200",
                    "bus": 26,
                    "loc": 0x58,
                    "attr": "hwmon",
                },
            ],
        },
    ],
    "fans": [
        {
            "name": "fan1",
            "present": {
                "gettype": "i2c",
                "bus": 14,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 0,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan1frue2",
                    "name": "24c02",
                    "bus": 63,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
        {
            "name": "fan2",
            "present": {
                "gettype": "i2c",
                "bus": 13,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 0,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan2frue2",
                    "name": "24c02",
                    "bus": 55,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
        {
            "name": "fan3",
            "present": {
                "gettype": "i2c",
                "bus": 14,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 1,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan3frue2",
                    "name": "24c02",
                    "bus": 64,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
        {
            "name": "fan4",
            "present": {
                "gettype": "i2c",
                "bus": 13,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 1,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan4frue2",
                    "name": "24c02",
                    "bus": 56,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
        {
            "name": "fan5",
            "present": {
                "gettype": "i2c",
                "bus": 14,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 2,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan5frue2",
                    "name": "24c02",
                    "bus": 65,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
        {
            "name": "fan6",
            "present": {
                "gettype": "i2c",
                "bus": 13,
                "loc": 0x0d,
                "offset": 0x30,
                "presentbit": 2,
                "okval": 0
            },
            "device": [
                {
                    "id": "fan6frue2",
                    "name": "24c02",
                    "bus": 57,
                    "loc": 0x50,
                    "attr": "eeprom"
                },
            ],
        },
    ]
}


FRULISTS = {
       "fans":[
           {"name":"fan1","bus":63,"loc":0x50, },
           {"name":"fan2","bus":55,"loc":0x50, },
           {"name":"fan3","bus":64,"loc":0x50, },
           {"name":"fan4","bus":56,"loc":0x50, },
           {"name":"fan5","bus":65,"loc":0x50, },
           {"name":"fan6","bus":57,"loc":0x50, }
       ] ,
       "psus": [
           {"name":"psu1","bus":23,"loc":0x50},
           {"name":"psu2","bus":25,"loc":0x50 },
           {"name":"psu2","bus":24,"loc":0x50 },
           {"name":"psu2","bus":26,"loc":0x50 }
       ]
}

# INIT_PARAM = [
#             {"loc":"3-0030/sfp_led_reset","value": "ff"},
#             {"loc":"3-0031/sfp_led_reset","value": "ff"},
#             {"loc":"4-0030/sfp_led_reset","value": "ff"},
#             {"loc":"4-0031/sfp_led_reset","value": "ff"},
#             {"loc":"5-0030/sfp_led_reset","value": "ff"},
#             {"loc":"5-0031/sfp_led_reset","value": "ff"},
#             {"loc":"6-0030/sfp_led_reset","value": "ff"},
#             {"loc":"6-0031/sfp_led_reset","value": "ff"},
# ]
# INIT_COMMAND = [
#     "grtd_test.py io wr 0xb19 0xff",
# ]

INIT_PARAM = [
    {
        "name": "sfp_led_reset1",
        "bus": 3,
        "devaddr": 0x30,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset2",
        "bus": 3,
        "devaddr": 0x31,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset3",
        "bus": 4,
        "devaddr": 0x30,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset4",
        "bus": 4,
        "devaddr": 0x31,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset5",
        "bus": 5,
        "devaddr": 0x30,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset6",
        "bus": 5,
        "devaddr": 0x31,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset7",
        "bus": 6,
        "devaddr": 0x30,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "sfp_led_reset8",
        "bus": 6,
        "devaddr": 0x31,
        "offset": 0xa0,
        "val": 0xff,
    },
    {
        "name": "mac_power_on",
        "type": "io",
        "offset": 0xb19,
        "val": 0xff
    }
]
#rg_eeprom  = "0-0054/eeprom"
E2_LOC = {"bus":1, "devno":0x56}
E2_PROTECT = {"io_addr":0xb45, "gettype":"io", "open":0, "close":1}

CPLDVERSIONS = [ 
        {"bus":13,  "devno":0x0d, "name":"FAN_CPLD_B"},
        {"bus":14, "devno":0x0d, "name":"FAN_CPLD_A"},
        {"bus":3, "devno":0x30, "name":"LC1_CPLD_1"},
        {"bus":3, "devno":0x31, "name":"LC1_CPLD_2"},
        {"bus":4, "devno":0x30, "name":"LC2_CPLD_1"},
        {"bus":4, "devno":0x31, "name":"LC2_CPLD_2"},
        {"bus":5, "devno":0x30, "name":"LC3_CPLD_1"},
        {"bus":5, "devno":0x31, "name":"LC3_CPLD_2"},
        {"bus":6, "devno":0x30, "name":"LC4_CPLD_1"},
        {"bus":6, "devno":0x31, "name":"LC4_CPLD_2"},
        {"io_addr":0x700, "name":"X86_CPLD", "gettype":"io"},
        {"io_addr":0x900, "name":"MAC_CPLD_B", "gettype":"io"},
        {"io_addr":0xb00, "name":"MAC_CPLD_A", "gettype":"io"},
]

DRIVERLISTS = []


TEMPIDCHANGE = {
  "lm75in": "inlet",
  "lm75out": "outlet",
  "lm75hot": "hot-point",
  "slot1lm75a1": "LINE CARD1 lm751",
  "slot1lm75a2": "LINE CARD1 lm752",
  "slot1lm75a3": "LINE CARD1 lm753",
  "slot2lm75a1": "LINE CARD2 lm751",
  "slot2lm75a2": "LINE CARD2 lm752",
  "slot2lm75a3": "LINE CARD2 lm753",
  "slot3lm75a1": "LINE CARD3 lm751",
  "slot3lm75a2": "LINE CARD3 lm752",
  "slot3lm75a3": "LINE CARD3 lm753",
  "slot4lm75a1": "LINE CARD4 lm751",
  "slot4lm75a2": "LINE CARD4 lm752",
  "slot4lm75a3": "LINE CARD4 lm753",
  "inlet": "lm75in",
  "outlet": "lm75out",
  "hot-point": "lm75hot",
}

DEVICE = []


fanlevel = {
    "tips":["LOW","MIDDLE","HIGH"],
    "level":[51,128,255],
    "low_speed":[1500,4500,9500],
    "high_speed":[3000,7000,14000]
}

fanloc =[ {"name":"FAN1", "location":"2-0020/fan1_pwm" ,
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan1_input"},{"name":"BACK ROTOR", "location":"2-0020/fan2_input"} ]},
          {"name":"FAN2", "location":"2-0020/fan3_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan3_input"},{"name":"BACK ROTOR", "location":"2-0020/fan4_input"} ]},
          {"name":"FAN3", "location":"2-0020/fan5_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan5_input"},{"name":"BACK ROTOR", "location":"2-0020/fan6_input"} ]},
          {"name":"FAN4", "location":"2-0020/fan7_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan7_input"},{"name":"BACK ROTOR", "location":"2-0020/fan8_input"} ]},
          {"name":"FAN5", "location":"2-0020/fan9_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan9_input"},{"name":"BACK ROTOR", "location":"2-0020/fan10_input"} ]},
          {"name":"FAN6", "location":"2-0020/fan11_pwm",
          "childfans":[{"name":"FRONT ROTOR", "location":"2-0020/fan11_input"},{"name":"BACK ROTOR", "location":"2-0020/fan12_input"} ]},
         ]


#################FAN speed args ##############################
MONITOR_TEMP_MIN           = 30
MONITOR_K                  = 14
MONITOR_MAC_IN             = 35
MONITOR_DEFAULT_SPEED      = 0x80
MONITOR_MAX_SPEED          = 0xFF
MONITOR_MIN_SPEED          = 0x33
MONITOR_MAC_ERROR_SPEED    = 0XBB
MONITOR_FAN_TOTAL_NUM      = 6
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

MONITOR_SYS_LED = [
{
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_sys",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
},
]

MONITOR_SYS_FAN_LED =[
{
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_fan",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
},
]

MONITOR_FANS_LED = [
          {"bus":14,"devno":0x0d, "addr":0x3b, "green":0x04, "red":0x02},
          {"bus":13,"devno":0x0d, "addr":0x3b, "green":0x04, "red":0x02},
          {"bus":14,"devno":0x0d, "addr":0x3c, "green":0x04, "red":0x02},
          {"bus":13,"devno":0x0d, "addr":0x3c, "green":0x04, "red":0x02},
          {"bus":14,"devno":0x0d, "addr":0x3d, "green":0x04, "red":0x02},
          {"bus":13,"devno":0x0d, "addr":0x3d, "green":0x04, "red":0x02},
          ]

DEV_LEDS = {
    "SLOTLED":[
        {"name":'slot1',"bus":3,"devno":0x30, "addr":0x1a, "green":0x04, "red":0x02},
        {"name":'slot2',"bus":4,"devno":0x30, "addr":0x1a, "green":0x04, "red":0x02},
        {"name":'slot3',"bus":5,"devno":0x30, "addr":0x1a, "green":0x04, "red":0x02},
        {"name":'slot4',"bus":6,"devno":0x30, "addr":0x1a, "green":0x04, "red":0x02},
    ]
}

MONITOR_SYS_PSU_LED =[
{
        "cmdstr":"/sys/devices/pci0000:00/0000:00:1f.0/broad_front_pwr",
        "yellow":0x06,
        "red":0x02,
        "green":0x04,
        "type":"sysfs",
},
]

MONITOR_FAN_STATUS = [
    {'status':'green' , 'minOkNum':6,'maxOkNum':6},
    {'status':'yellow', 'minOkNum':5,'maxOkNum':5},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':4},
    ]

MONITOR_PSU_STATUS = [
    {'status':'green' , 'minOkNum':4,'maxOkNum':4},
    {'status':'yellow', 'minOkNum':3,'maxOkNum':3},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':2},
    ]

MONITOR_DEV_STATUS = {
    "temperature": [
        {"name":"lm75in",       "location":"/sys/bus/i2c/devices/29-004f/hwmon/*/temp1_input"},
        {"name":"lm75out",      "location":"/sys/bus/i2c/devices/28-004b/hwmon/*/temp1_input"},
        {"name":"lm75hot",      "location":"/sys/bus/i2c/devices/28-004c/hwmon/*/temp1_input"},
        {"name":"cpu",          "location":"/sys/class/hwmon/hwmon0"},
    ],
    "fans": [
        {
            "name":"fan1",
            "presentstatus":{"bus":14, "loc":0x0d, "offset":0x30, 'bit':0},
            "rollstatus": [
                {"name":"motor1","bus":14, "loc":0x0d, "offset":0x31, 'bit':0},
                {"name":"motor2","bus":14, "loc":0x0d, "offset":0x34, 'bit':0},
            ]
        },
        {
            "name":"fan2",
            "presentstatus":{"bus":13, "loc":0x0d, "offset":0x30, 'bit':0},
            "rollstatus": [
                {"name":"motor1","bus":13, "loc":0x0d, "offset":0x31, 'bit':0},
                {"name":"motor2","bus":13, "loc":0x0d, "offset":0x34, 'bit':0},
            ]
        },
        {
            "name":"fan3",
            "presentstatus":{"bus":14, "loc":0x0d, "offset":0x30, 'bit':1},
            "rollstatus": [
                {"name":"motor1","bus":14, "loc":0x0d, "offset":0x31, 'bit':1},
                {"name":"motor2","bus":14, "loc":0x0d, "offset":0x34, 'bit':1},
            ]
        },
        {
            "name":"fan4",
            "presentstatus":{"bus":13, "loc":0x0d, "offset":0x30, 'bit':1},
            "rollstatus": [
                {"name":"motor1","bus":13, "loc":0x0d, "offset":0x31, 'bit':1},
                {"name":"motor2","bus":13, "loc":0x0d, "offset":0x34, 'bit':1},
            ]
        },
        {
            "name":"fan5",
            "presentstatus":{"bus":14, "loc":0x0d, "offset":0x30, 'bit':2},
            "rollstatus": [
                {"name":"motor1","bus":14, "loc":0x0d, "offset":0x31, 'bit':2},
                {"name":"motor2","bus":14, "loc":0x0d, "offset":0x34, 'bit':2},
            ]
        },
        {
            "name":"fan6",
            "presentstatus":{"bus":13, "loc":0x0d, "offset":0x30, 'bit':2},
            "rollstatus": [
                {"name":"motor1","bus":13, "loc":0x0d, "offset":0x31, 'bit':2},
                {"name":"motor2","bus":13, "loc":0x0d, "offset":0x34, 'bit':2},
            ]
        },
    ],
    "psus": [
        {"name":"psu1", "io_addr":0xb27, "gettype":"io",  'presentbit': 0,  'statusbit':1, 'alertbit':2},
        {"name":"psu2", "io_addr":0xb28, "gettype":"io",  'presentbit': 0,  'statusbit':1, 'alertbit':2},
        {"name":"psu3", "io_addr":0xb29, "gettype":"io",  'presentbit': 0,  'statusbit':1, 'alertbit':2},
        {"name":"psu4", "io_addr":0xb2a, "gettype":"io",  'presentbit': 0,  'statusbit':1, 'alertbit':2}
    ],
    "slots": [
        {"name":"slot1", "io_addr":0xb2c, "gettype":"io",  'presentbit': 4},
        {"name":"slot2", "io_addr":0xb2c, "gettype":"io",  'presentbit': 5},
        {"name":"slot3", "io_addr":0xb2c, "gettype":"io",  'presentbit': 6},
        {"name":"slot4", "io_addr":0xb2c, "gettype":"io",  'presentbit': 7}
    ],
    "mac_temp" : {
        "loc" : [
                "28-004c/hwmon/*/temp2_input",
                "29-004c/hwmon/*/temp2_input",
            ],
    },
}

MONITOR_DEV_STATUS_DECODE = {
    'fanpresent' :  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'fanroll'    :  {0:'STALL'  , 1:'ROLL',   'okval':1},
    'psupresent' :  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'psuoutput'  :  {0:'FAULT'  , 1:'NORMAL', 'okval':1},
    'psualert'   :  {0:'FAULT'  , 1:'NORMAL', 'okval':1},
    'slotpresent':  {0:'PRESENT', 1:'ABSENT', 'okval':0},
}

SLOT_MONITOR_PARAM = {
    "polling_time" : 0.5,
    "slots": [
        {"name":"slot1",
         "present":{"gettype":"io", "io_addr":0xb2c,"presentbit": 4,"okval":0},
         "act":[
             {"io_addr":0xb19, "value":0x01, "mask":0xfe, "gettype":"io"},
             {"bus":3, "loc":0x30, "offset":0xa0, "value":0xff,"gettype":"i2c"},
             {"bus":3, "loc":0x31, "offset":0xa0, "value":0xff,"gettype":"i2c"},
         ],
        },
        {"name":"slot2",
         "present":{"gettype":"io", "io_addr":0xb2c,"presentbit": 5,"okval":0},
         "act":[
             {"io_addr":0xb19, "value":0x02, "mask":0xfd, "gettype":"io"},
             {"bus":4, "loc":0x30, "offset":0xa0, "value":0xff,"gettype":"i2c"},
             {"bus":4, "loc":0x31, "offset":0xa0, "value":0xff,"gettype":"i2c"},
         ],
        },
        {"name":"slot3",
         "present":{"gettype":"io", "io_addr":0xb2c,"presentbit": 6,"okval":0},
         "act":[
             {"io_addr":0xb19, "value":0x04, "mask":0xfb, "gettype":"io"},
             {"bus":5, "loc":0x30, "offset":0xa0, "value":0xff,"gettype":"i2c"},
             {"bus":5, "loc":0x31, "offset":0xa0, "value":0xff,"gettype":"i2c"},
         ],
        },
        {"name":"slot4",
         "present":{"gettype":"io", "io_addr":0xb2c,"presentbit": 7,"okval":0},
         "act":[
             {"io_addr":0xb19, "value":0x08, "mask":0xf7, "gettype":"io"},
             {"bus":6, "loc":0x30, "offset":0xa0, "value":0xff,"gettype":"i2c"},
             {"bus":6, "loc":0x31, "offset":0xa0, "value":0xff,"gettype":"i2c"},
         ],
        },
    ],
}

#####################MAC AVS ARGS (B6920-4S) ####################################
MAC_AVS_PARAM ={
    0x72:0x0384 ,
    0x73:0x037e ,
    0x74:0x0378 ,
    0x75:0x0372 ,
    0x76:0x036b ,
    0x77:0x0365 ,
    0x78:0x035f ,
    0x79:0x0359 ,
    0x7a:0x0352 ,
    0x7b:0x034c ,
    0x7c:0x0346 ,
    0x7d:0x0340 ,
    0x7e:0x0339 ,
    0x7f:0x0333 ,
    0x80:0x032d ,
    0x81:0x0327 ,
    0x82:0x0320 ,
    0x83:0x031a ,
    0x84:0x0314 ,
    0x85:0x030e ,
    0x86:0x0307 ,
    0x87:0x0301 ,
    0x88:0x02fb ,
    0x89:0x02f5 ,
    0x8A:0x02ee 
}


MAC_DEFAULT_PARAM = {
  "type": 1,
  "default":0x73,
  "loopaddr":0x00,
  "loop":0x01,
  "open":0x00,
  "close":0x40,
  "bus":30,
  "devno":0x64,
  "addr":0x21,
  "protectaddr":0x10,
  "sdkreg":"DMU_PCU_OTP_CONFIG_4",
  "sdkcmd": "scdcmd",
  "sdkcmdargs": ["-t", 5],
  "sdktype": 1,
  "macregloc":8,
  "mask": 0xff
}
