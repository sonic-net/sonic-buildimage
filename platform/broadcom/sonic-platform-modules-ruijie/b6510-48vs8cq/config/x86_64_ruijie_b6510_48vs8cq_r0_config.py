#!/usr/bin/python3
# -*- coding: UTF-8 -*-

fancontrol_loc        = "/usr/local/bin" 
fancontrol_config_loc = "/usr/local/bin" 

GLOBALCONFIG       = "GLOBALCONFIG"
MONITOR_CONST      = "MONITOR_CONST"

RUIJIE_PART_NUMBER    = "RJ000001"
RUIJIE_LABEL_REVISION = "R01"
RUIJIE_ONIE_VERSION   = "2018.02"
RUIJIE_MAC_SIZE       = 3
RUIJIE_MANUF_NAME     = "Ruijie"
RUIJIE_MANUF_COUNTRY  = "CHN"
RUIJIE_VENDOR_NAME    = "Ruijie"
RUIJIE_DIAG_VERSION   = "0.1.0.15"
RUIJIE_SERVICE_TAG    = "www.ruijie.com.cn"

DEV_LEDS = {}
MEM_SLOTS = []

LOCAL_LED_CONTROL = {
    "CLOSE":{},
    "OPEN":{}
}

FIRMWARE_TOOLS = {}

###################################################################################################
#####          fan board ID reference
###################################################################################################
FANS_DEF = {
    0x8100:"M6500-FAN-F",
    0x8101:"M6510-FAN-F",
    0x8102:"M6520-FAN-F",
    0x8103:"M6510-FAN-R"
}

factest_module = {
    "sysinfo_showfanmsg":1,
    "sysinfo_showPsumsg":1,
    "sysinfo_showrestfanmsg":0,
    "sysinfo_showrestpsumsg":0
}

MONITOR_MAC_SOURCE_SYSFS = 0 #1 get mac temperature from sysfs ,0 get mac temperature from bcmcmd 
MONITOR_MAC_SOURCE_PATH = None #sysfs path

###################################################################

SLOT_MONITOR_PARAM = {}

FAN_PROTECT = {"bus":0, "devno":0x32, "addr":0x19, "open":0x00, "close":0x0f}
rg_eeprom  = "2-0057/eeprom"
E2_LOC = {"bus":2, "devno":0x57}
E2_PROTECT ={"bus":2, "devno":0x33, "addr":0xb0, "open":0, "close":1}
MAC_LED_RESET = {"pcibus": 8, "slot": 0, "fn": 0, "bar": 0, "offset": 64, "reset": 0x98}

PCA9548START  = -1
PCA9548BUSEND = -2


RUIJIE_CARDID      = 0x00004040
RUIJIE_PRODUCTNAME = "B6510-48VS8CQ"

STARTMODULE  = {
    "fancontrol":1,
    "avscontrol":1,
    "dev_monitor":1
}

i2ccheck_params = {"busend":"i2c-66","retrytime":6}

DEV_MONITOR_PARAM = {
    "polling_time": 10,
    "psus": [
        {
            "name": "psu1",
            "present": {
                "gettype": "i2c",
                "bus": 2,
                "loc": 0x37,
                "offset": 0x51,
                "presentbit": 0,
                "okval": 0,
            },
            "device": [
                {
                    "id": "psu1pmbus",
                    "name": "dps550",
                    "bus": 7,
                    "loc": 0x58,
                    "attr": "hwmon",
                },
            ],
        },
        {
            "name": "psu2",
            "present": {
                "gettype": "i2c",
                "bus": 2,
                "loc": 0x37,
                "offset": 0x51,
                "presentbit": 4,
                "okval": 0,
            },
            "device": [
                {
                    "id": "psu2pmbus",
                    "name": "dps550",
                    "bus": 8,
                    "loc": 0x5B,
                    "attr": "hwmon",
                },
            ],
        },
    ],
}

fanlevel = {
    "tips":["low","medium","high"],
    "level":[51,150,255],
    "low_speed":[500,7500,17000],
    "high_speed":[11000,22500,28500]
}

fanloc =[ {"name":"FAN1/FAN2/FAN3/FAN4", "location":"0-0032/fan_speed_set",
          "childfans":[{"name":"FAN1", "location":"2-0037/hwmon/hwmon4/fan1_input"},
          {"name":"FAN2", "location":"2-0037/hwmon/hwmon4/fan2_input"},
          {"name":"FAN3", "location":"2-0037/hwmon/hwmon4/fan3_input"},
          {"name":"FAN4", "location":"2-0037/hwmon/hwmon4/fan4_input"} ]},
         ]


#################FAN-Speed-Adjustment-Parameters##############################
MONITOR_TEMP_MIN           = 38    # temperature before speed-adjsutment
MONITOR_K                  = 11    # speed-adjustment algorithm
MONITOR_MAC_IN             = 35    # temperature difference between mac and chip
MONITOR_DEFAULT_SPEED      = 0x60  # default speed
MONITOR_MAX_SPEED          = 0xFF  # maximum speed
MONITOR_MIN_SPEED          = 0x33  # minimum speed
MONITOR_MAC_ERROR_SPEED    = 0XBB  # MAC abnormal speed
MONITOR_FAN_TOTAL_NUM      = 4     # 3+1 redundancy design, report to syslog if there exists a error
MONITOR_MAC_UP_TEMP        = 50    # MAC compared with temperature inlet up
MONITOR_MAC_LOWER_TEMP     = -50   # MAC compared with temperature outlet down
MONITOR_MAC_MAX_TEMP       = 100   # 

MONITOR_FALL_TEMP = 4               # speed-adjustment reduced temperature
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
MONITOR_CRITICAL_NUM              = 3 #retry times
MONITOR_SHAKE_TIME                = 20 #anti-shake intervals
MONITOR_INTERVAL                   = 60

MONITOR_SYS_LED = [
          {"bus":2,"devno":0x33, "addr":0xb2, "yellow":0x03, "red":0x02,"green":0x01},
          {"bus":2,"devno":0x37, "addr":0xb2, "yellow":0x03, "red":0x02,"green":0x01}]

MONITOR_SYS_FAN_LED =[
          {"bus":2,"devno":0x33, "addr":0xb4, "yellow":0x06, "red":0x02,"green":0x04},
    ]
MONITOR_FANS_LED = [
          {"bus":2,"devno":0x32, "addr":0x23, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x24, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x25, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x26, "green":0x09, "red":0x0a}]


CPLDVERSIONS = [ 
        {"bus":2, "devno":0x33, "name":"MAC board CPLD-A"},
        {"bus":2, "devno":0x35, "name":"MAC board CPLD-B"},
        {"bus":2, "devno":0x37, "name":"CONNECT board CPLD-A"},
        {"bus":0, "devno":0x0d, "name":"CPU board CPLD"},
]

MONITOR_SYS_PSU_LED =[
          {"bus":2,"devno":0x33, "addr":0xb3, "yellow":0x06, "red":0x02,"green":0x04},
    ]
    
MONITOR_FAN_STATUS = [
    {'status':'green', 'minOkNum':4,'maxOkNum':4},
    {'status':'yellow', 'minOkNum':3,'maxOkNum':3},
    {'status':'red'  , 'minOkNum':0,'maxOkNum':2},
    ]

MONITOR_PSU_STATUS = [
    {'status':'green', 'minOkNum':2,'maxOkNum':2},
    {'status':'yellow', 'minOkNum':1,'maxOkNum':1},
    {'status':'red'  , 'minOkNum':0,'maxOkNum':0},
    ]


MONITOR_DEV_STATUS = {
    "temperature": [
        {"name":"lm75in",       "location":"/sys/bus/i2c/devices/2-0048/hwmon/*/temp1_input"},
        {"name":"lm75out",      "location":"/sys/bus/i2c/devices/2-0049/hwmon/*/temp1_input"},
        {"name":"lm75hot",      "location":"/sys/bus/i2c/devices/2-004a/hwmon/*/temp1_input"},
        {"name":"cpu",          "location":"/sys/class/hwmon/hwmon0"},
    ],
    "fans": [
        {
            "name":"fan1",
            "presentstatus":{"bus":2, "loc":0x37, "offset":0x30, 'bit':0},
            "rollstatus": [
                {"name":"motor1","bus":2, "loc":0x37, "offset":0x31, 'bit':0},
            ]
        },
        {
            "name":"fan2",
            "presentstatus":{"bus":2, "loc":0x37, "offset":0x30, 'bit':1},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x37, "offset":0x31, 'bit':1},
            ]
        },
        {
            "name":"fan3",
            "presentstatus":{"bus":2, "loc":0x37, "offset":0x30, 'bit':2},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x37, "offset":0x31, 'bit':2},
            ]
        },
        {
            "name":"fan4",
            "presentstatus":{"bus":2, "loc":0x37, "offset":0x30, 'bit':3},
            "rollstatus":[
                {"name":"motor1","bus":2, "loc":0x37, "offset":0x31, 'bit':3},
            ]
        },
    ],
     "psus": [
        {"name":"psu1", "bus":2, "loc":0x37, "offset":0x51, "gettype":"i2c", 'presentbit': 0, 'statusbit':1,'alertbit':2},
        {"name":"psu2", "bus":2, "loc":0x37, "offset":0x51, "gettype":"i2c", 'presentbit': 4, 'statusbit':5,'alertbit':6},
     ],
    "mac_temp" : {
        "flag" : {"bus":2, "loc":0x33, "offset":0xd4, "gettype":"i2c", 'okbit': 0, 'okval':1},
        "loc" : [
                "2-0035/mac_temp_input",
            ],
        "try_bcmcmd" : 0,
    },
}

MONITOR_DEV_STATUS_DECODE = {
    'fanpresent':  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'fanroll'   :  {0:'STALL' , 1:'ROLL',   'okval':1},
    'psupresent':  {0:'PRESENT', 1:'ABSENT', 'okval':0},
    'psuoutput' :  {0:'FAULT' , 1:'NORMAL', 'okval':1},
    'psualert'  :  {0:'FAULT' , 1:'NORMAL', 'okval':1},
}
###################################################################


#####################MAC-Voltage-Adjustment-Parameters(B6510)####################################
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
# 6510 Default Configuration
MAC_DEFAULT_PARAM = {
  "type": 1,                       # type 1 represents default if out of range / 0 represents no voltage-adjustment if out of range
  "default":0x74,                  # should be used with type
  "loopaddr":0x00,                 # AVS loop address
  "loop":0x00,                     # AVS loop value
  "open":0x00,                     # diasble write-protection value
  "close":0x40,                    # enable write-protection value
  "bus":2,                         # AVSI2C bus address
  "devno":0x60,                    # AVS address
  "addr":0x21,                     # AVS voltage-adjustment address
  "protectaddr":0x10,              # AVS write-protection address
  "sdkreg":"TOP_AVS_SEL_REG",      # SDK register name
  "sdktype": 0,                    # type 0 represents no shift operation / 1 represents shift operation
  "macregloc":24,                 # shift operation
  "mask": 0xff                     # mask after shift
}
#####################MAC-Voltage-Adjustment-Parameters####################################

## Drivers List
## 
DRIVERLISTS = [
        {"name":"i2c_dev", "delay":0},
        {"name":"i2c_gpio", "delay":0},
        {"name":"i2c_algo_bit","delay":0},
        {"name":"i2c_mux_pca9641", "delay":0},
        {"name":"i2c_mux_pca954x force_create_bus=1", "delay":0},# force_deselect_on_exit=1
        {"name":"i2c_mux", "delay":0},
        {"name":"lm75", "delay":0},
        {"name":"optoe", "delay":0},
        {"name":"at24", "delay":0},
        {"name":"ruijie_platform", "delay":0},
        {"name":"rg_cpld", "delay":0},
        {"name":"rg_fan", "delay":0},
        {"name":"rg_psu", "delay":0},
        {"name":"pmbus_core", "delay":0},
        {"name":"csu550", "delay":0},
        {"name":"rg_gpio_xeon", "delay":0},
        {"name":"ipmi_devintf", "delay":0},
        {"name":"ipmi_si", "delay":0},
        {"name":"ipmi_msghandler", "delay":0},
]

DEVICE = [
        {"name":"pca9541","bus":0,"loc":0x10 },
        {"name":"pca9548","bus":2,"loc":0x70 },
        {"name":"lm75","bus": 2,   "loc":0x48 },
        {"name":"lm75","bus": 2,   "loc":0x49 },
        {"name":"lm75","bus": 2,   "loc":0x4a },
        {"name":"24c02","bus":2, "loc":0x57 },
        {"name":"rg_cpld","bus":0,"loc":0x32 },
        {"name":"rg_cpld","bus":1,"loc":0x34 },
        {"name":"rg_cpld","bus":1,"loc":0x36 },
        {"name":"rg_cpld","bus":2,"loc":0x33 },
        {"name":"rg_cpld","bus":2,"loc":0x35 },
        {"name":"rg_cpld","bus":2,"loc":0x37 },
        {"name":"pca9548","bus":1,"loc":0x70 },
        {"name":"pca9548","bus":1,"loc":0x71 },
        {"name":"pca9548","bus":1,"loc":0x72 },
        {"name":"pca9548","bus":1,"loc":0x73 },
        {"name":"pca9548","bus":1,"loc":0x74 },
        {"name":"pca9548","bus":1,"loc":0x75 },
        {"name":"pca9548","bus":1,"loc":0x76 },
        {"name":"rg_fan","bus":3,"loc":0x53 },
        {"name":"rg_fan","bus":4,"loc":0x53 },
        {"name":"rg_fan","bus":5,"loc":0x53 },
        {"name":"rg_fan","bus":6,"loc":0x53 }, 
        {"name":"rg_psu","bus":7,"loc":0x50 },
        {"name":"dps550","bus":7,"loc":0x58 },
        {"name":"rg_psu","bus":8,"loc":0x53 },
        {"name":"dps550","bus":8,"loc":0x5b },
]

INIT_PARAM = [
            {"loc":"1-0034/sfp_enable","value": "01"},
            {"loc":"2-0035/sfp_enable2","value":"ff"},
            {"loc":"2-0033/mac_led", "value":"ff"},
            {"loc":"1-0034/sfp_txdis1","value":"00"},
            {"loc":"1-0034/sfp_txdis2","value":"00"},
            {"loc":"1-0034/sfp_txdis3","value":"00"},
            {"loc":"1-0036/sfp_txdis4","value":"00"},
            {"loc":"1-0036/sfp_txdis5","value":"00"},
            {"loc":"1-0036/sfp_txdis6","value":"00"},
            {"loc":fanloc[0]["location"], "value":"80"},
            {"loc":"2-0033/sfp_led1_yellow","value":"ad"},
            {"loc":"2-0035/sfp_led2_yellow","value":"ad"},
]

INIT_COMMAND = [
]

## Driver List
## 

#####################FRU-Info-Adaption#################################
E2TYPE = {"1": "tlveeprom",
          "2": "x86cpueeprom",
          "3": "bmceeprom", 
          "4": "cpueeprom", 
          "5": "maceeprom", 
          "6": "sloteeprom",
          "7": "fanconnecteeprom",
          "8": "M1HFANI-F", 
          "9": "M1HFANI-R", 
          "A": "M2HFANI-F", 
          "B": "M2HFANI-R", 
          "C": "psu"}
FRULISTS = []
################################Manufacturing-Test-Adaption-Area#######################################################
#   need to export interface
fanlevel_6510 = {
     "level":[51,150,255],
     "low_speed":[500,7500,17000],
     "high_speed":[11000,22500,28500]
}

fanlevel_6520 = {
     "level":[75,150,255],
     "low_speed":[750,4250,6750],
     "high_speed":[4500,7500,10000]
}

TEMPIDCHANGE = {
  "lm75in":"inlet",
  "lm75out":"outlet",
  "lm75hot":"hot-point",
  "inlet":"lm75in",
  "outlet":"lm75out",
  "hot-point":"lm75hot",
}

#   Manufacturing-Test module
FACTESTMODULE  = { }

##################################Manufacturing-Test-Menu
item1 = {"name":"Single Test", "deal" :"test_signal", "childid":1}
test_sys_reload_item = {"name":"reset-system", "deal" :"test_sys_reload"}

test_sys_item           = { "name":"Product information test", "deal" :"test_sysinfo"}
test_temp_item          = { "name":"temperature test", "deal" :"test_tempinfo"}
test_mem_item           = { "name":"Memory test", "deal" :"test_cpumemoryinfo"}
test_hd_item            = { "name":"Hard disk test", "deal" :"test_hard"}
test_rtc_item           = { "name":"RTC test ", "deal" :"test_rtc"}
test_i2c_item           = { "name":"I2c test ", "deal" :"test_i2c"}
test_cpld_item          = { "name":"CPLD test", "deal" :"test_cpld"}
test_portframe_item     = { "name":"Port transmit-receive frame test", "deal" :"test_portframe"}
test_sysled_item        = { "name":"System led test", "deal" :"test_led"}
test_fan_item           = { "name":"Fan status test", "deal" :"test_fan"}
test_power_item         = { "name":"PSU status test", "deal" :"test_power"}
test_usb_item           = { "name":"USB test", "deal" :"test_usb"}
test_prbs_item          = { "name":"PRBS test", "deal" :"test_prbs"}
test_portbroadcast_item = { "name":"Port broadcast", "deal" :"test_portbroadcast"}

test_debug_level       = {"name":"Change debug level", "deal" :"test_setdebug"}
test_log_level         = {"name":"Log output level", "deal" :"test_loginfolevel"}
test_setmac            = {"name":"setmac", "deal" :"test_setmac"}
test_setrtc            = {"name":"Set RTC", "deal" :"test_set_rtc"}

log_level_critical    = {"name":"CRITICAL", "deal" :"test_log_critical"}
log_level_debug       = {"name":"DEBUG", "deal" :"test_log_debug"}
log_level_error       = {"name":"ERROR", "deal" :"test_log_error"}
log_level_info        = {"name":"INFO", "deal" :"test_log_info"}
log_level_notset      = {"name":"NOTSET", "deal" :"test_log_notset"}
log_level_warning     = {"name":"WARNING", "deal" :"test_log_warning"}


test_e2_setmac_item   = {"name":"E2SETMAC", "deal" :"test_e2_setmac"}
test_bmc_setmac_item  = {"name":"BMCSETMAC", "deal" :"test_bmc_setmac"}
test_fan_setmac_item  = {"name":"fan SETMAC", "deal" :"test_fan_setmac"}

alltest = [
        test_sys_item,
        test_temp_item,
        test_mem_item,
        test_hd_item,
        test_rtc_item,
        test_i2c_item,
        test_cpld_item,
        test_portframe_item,
        test_sysled_item,
        test_fan_item,
        test_power_item,
        test_usb_item,
        test_prbs_item,
        test_portbroadcast_item
        ]
        
looptest = [
        test_sys_item,
        test_temp_item,
        test_mem_item,
        test_hd_item,
        test_rtc_item,
        test_i2c_item,
        test_cpld_item,
        test_portframe_item,
        test_fan_item, 
        test_power_item,
        test_usb_item,
        test_prbs_item,
        test_portbroadcast_item , 
]

diagtestall = [
]

menuList =[
        {
        "menuid":0, "value":[
            {"name":"Single test", "deal" :"test_signal", "childid":1},
            {"name":"All test", "deal" :"test_all"},
            {"name":"Loop test", "deal" :"test_loop"},
            #{"name":"Check loop-test result", "deal" :"test_loop_read"},
            #{"name":"Delete loop-test result", "deal" :"test_loop_delete"},
#           {"name":"Load configuration", "deal" :"test_config"},
            test_sys_reload_item,
            {"name":"System Configuration", "deal" :"test_sysconfig","childid":2},
        ]
        },
        {
            "menuid":1, "parentid":0, "value":[
                        test_sys_item            ,
                        test_temp_item           ,
                        test_mem_item            ,
                        test_hd_item             ,
                        test_rtc_item            ,
                        test_i2c_item            ,
                        test_cpld_item           ,
                        test_portframe_item      ,
                        test_sysled_item         ,
                        test_fan_item            ,
                        test_power_item          ,
                        test_usb_item            ,
                        test_prbs_item           ,
                        test_portbroadcast_item  ,
            ]},
        {
                "menuid":2, "parentid":0, "value":[
                test_debug_level,
                test_log_level ,
                test_setmac ,
                test_setrtc ,
        ]},
        {
        "menuid":3, "parentid":2, "value":[
                    log_level_critical , 
                    log_level_debug    ,
                    log_level_error    ,
                    log_level_info     ,
                    log_level_notset   ,
                    log_level_warning  ,
        ]},
        {
        "menuid":4, "parentid":2, "value":[
            test_e2_setmac_item ,
            test_bmc_setmac_item,
            test_fan_setmac_item,
        ]},
]


TESTCASE={
    "CPLD":[
        {"name":"CONNECT BOARD CPLD-A" ,"cases":[
              {"name":"cpld32",      "cmd":"grtd_test.py  cpld_check 0 0x32 0xAA"},
              {"name":"cpld37",      "cmd":"grtd_test.py  cpld_check 2 0x37 0xAC"},
            ]
        },
        {"name":"MAC BOARD CPLD-A" ,"cases":[
              {"name":"cpld33",      "cmd":"grtd_test.py  cpld_check 2 0x33 0xAB"},
              {"name":"cpld34",      "cmd":"grtd_test.py  cpld_check 1 0x34 0xAA"},
            ]
        },
        {"name":"MAC BOARD CPLD-B" ,"cases":[
              {"name":"cpld36",      "cmd":"grtd_test.py  cpld_check 1 0x36 0xAA"},
              {"name":"cpld35",      "cmd":"grtd_test.py  cpld_check 2 0x35 0xAB"},
            ]
        },
    ],
    "TEMPERATURE":[
                 {
                 "name":"-->temperature test" , "cases":[
                        {"name":"inlet","cmd":"grtd_test.py  temp 2-0048/hwmon/hwmon1/temp1_input"},
                        {"name":"outlet","cmd":"grtd_test.py  temp 2-0049/hwmon/hwmon2/temp1_input"},
                        {"name":"hot-point","cmd":"grtd_test.py  temp 2-004a/hwmon/hwmon3/temp1_input"},
                    ]
                }
        ],
    "MEMTORY":{
        "cases":[
            {"name":"->memory test 1M","cmd":"memtester 1M 1"},
            {"name":"->memory test 2M","cmd":"memtester 2M 1"},
            {"name":"->memory test 8M","cmd":"memtester 8M 1"},
#           {"name":"->memory test 16M","cmd":"memtester 16M 1"},
#           {"name":"->memory test 256M","cmd":"memtester 256M 1"},
        ]
    },
    "SMARTCTLCMDS":{
        "cases":[
            {"name":"->Check Hard Disk Info",     "cmd":"smartctl -i /dev/sda"},
            {"name":"->Check Hard Disk Monitor Status", "cmd":"smartctl -H /dev/sda"},
        ]
    },
    "LED":[
        {"name":"Light Port Led test","cases":[
              {"name":"-> Red Led Off",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_red,1-0034/sfp_led2_red,1-0034/sfp_led3_red,1-0034/sfp_led8_red,1-0036/sfp_led4_red,1-0036/sfp_led5_red,1-0036/sfp_led6_red,1-0036/sfp_led7_red 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"},
              {"name":"-> Red Led On",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_red,1-0034/sfp_led2_red,1-0034/sfp_led3_red,1-0034/sfp_led8_red,1-0036/sfp_led4_red,1-0036/sfp_led5_red,1-0036/sfp_led6_red,1-0036/sfp_led7_red 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff"},
              {"name":"-> Recovery Red Led Off",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_red,1-0034/sfp_led2_red,1-0034/sfp_led3_red,1-0034/sfp_led8_red,1-0036/sfp_led4_red,1-0036/sfp_led5_red,1-0036/sfp_led6_red,1-0036/sfp_led7_red 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"},
              
              {"name":"-> Yellow Led Off",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_yellow,1-0034/sfp_led2_yellow,1-0034/sfp_led3_yellow,1-0034/sfp_led8_yellow,1-0036/sfp_led4_yellow,1-0036/sfp_led5_yellow,1-0036/sfp_led6_yellow,1-0036/sfp_led7_yellow 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"},
              {"name":"-> Yellow Led On",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_yellow,1-0034/sfp_led2_yellow,1-0034/sfp_led3_yellow,1-0034/sfp_led8_yellow,1-0036/sfp_led4_yellow,1-0036/sfp_led5_yellow,1-0036/sfp_led6_yellow,1-0036/sfp_led7_yellow 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff"},
              {"name":"-> Recovery Yellow Led Off",        "cmd":"grtd_test.py  led loc 1-0034/sfp_led1_yellow,1-0034/sfp_led2_yellow,1-0034/sfp_led3_yellow,1-0034/sfp_led8_yellow,1-0036/sfp_led4_yellow,1-0036/sfp_led5_yellow,1-0036/sfp_led6_yellow,1-0036/sfp_led7_yellow 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"},
            ]
        },
        {"name":"fan 1 Led" ,"cases":[
              {"name":"-> LedOff",        "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x0b"},
              {"name":"-> Red Led ",        "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x0a"},
              {"name":"-> Green Led ",        "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x09"},
              {"name":"-> Yellow Led ",        "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x08"},
              {"name":"-> Red Led Flashing",    "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x0e"},
              {"name":"-> Green Led Flashing",    "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x0d"},
              {"name":"-> Yellow Led Flashing",    "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x0c"},
              {"name":"-> Recovery Green Led ",        "cmd":"grtd_test.py  led loc 0-0032/fan0_led 0x09"},
            ]
        },
        {"name":"fan 2 Led" ,"cases":[
              {"name":"-> LedOff",      "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x0b"},
              {"name":"-> Red Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x0a"},
              {"name":"-> Green Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x09"},
              {"name":"-> Yellow Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x08"},
              {"name":"-> Red Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x0e"},
              {"name":"-> Green Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x0d"},
              {"name":"-> Yellow Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x0c"},
              {"name":"-> Recovery Green Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan1_led 0x09"},
            ]
        },
        {"name":"fan 3 Led" ,"cases":[
              {"name":"-> LedOff",      "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x0b"},
              {"name":"-> Red Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x0a"},
              {"name":"-> Green Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x09"},
              {"name":"-> Yellow Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x08"},
              {"name":"-> Red Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x0e"},
              {"name":"-> Green Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x0d"},
              {"name":"-> Yellow Led Flashing",  "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x0c"},
              {"name":"-> Recovery Green Led ",      "cmd":"grtd_test.py  led loc 0-0032/fan2_led 0x09"},
            ]
        },
        {"name":"Front panel CPU Led", "cases":[
              {"name":"-> LedOff",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x00"},
              {"name":"-> Green Led not Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x01"},
              {"name":"-> Red Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x02"},
              {"name":"-> Yellow Led not Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x03"},
              {"name":"-> Green Led 1/4sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x11"},
              {"name":"-> Green Led 1/2sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x21"},
              {"name":"-> Green Led 1sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x41"},
              {"name":"-> Green Led 2sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x81"},
              {"name":"-> Red Led 1/4sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x12"},
              {"name":"-> Red Led 1/2sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x22"},
              {"name":"-> Red Led 1sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x42"},
              {"name":"-> Red Led 2sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x82"},
              {"name":"-> Yellow Led 1/4sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x13"},
              {"name":"-> Yellow Led 1/2sFlashing  ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x23"},
              {"name":"-> Yellow Led 1sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x43"},
              {"name":"-> Yellow Led 2sFlashing    ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x83"},
              {"name":"-> Recovery Green Led ",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_cpu 0x01"},
            ]
        },
        {"name":"Front panel BMC Led" ,"cases":[
              {"name":"-> LedOff",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x00"},
              {"name":"-> Red Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x01"},
              {"name":"-> Red Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x02"},
              {"name":"-> Green Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x03"},
              {"name":"-> Green Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x04"},
              {"name":"-> Yellow Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x05"},
              {"name":"-> Yellow Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x06"},
              {"name":"-> Recovery Green Led ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_bmc 0x04"},
            ]
        },
        {"name":"Front panel location Led" , "cases":[
                {"name":"-> LedOff","cmd":"grtd_test.py  led loc 2-0035/broad_front_lct 0xff"},
                {"name":"-> LedOn","cmd":"grtd_test.py  led loc 2-0035/broad_front_lct 0xfe"},
                {"name":"->Recovery LedOff","cmd":"grtd_test.py  led loc 2-0035/broad_front_lct 0xff"},
                ]
        },
        
        {"name":"Front panel pwr Led" ,"cases":[
              {"name":"-> LedOff",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x00"},
              {"name":"-> Red Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x01"},
              {"name":"-> Red Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x02"},
              {"name":"-> Green Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x03"},
              {"name":"-> Green Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x04"},
              {"name":"-> Yellow Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x05"},
              {"name":"-> Yellow Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x06"},
              {"name":"-> Recovery Green Led ",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_pwr 0x04"},
            ]
        },
        {"name":"Front panel fan Led" ,"cases":[
              {"name":"-> LedOff",        "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x00"},
              {"name":"-> Red Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x01"},
              {"name":"-> Red Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x02"},
              {"name":"-> Green Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x03"},
              {"name":"-> Green Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x04"},
              {"name":"-> Yellow Led Flashing",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x05"},
              {"name":"-> Yellow Led not Flashing",  "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x06"},
              {"name":"-> Recovery Green Led ",    "cmd":"grtd_test.py  led loc 2-0035/broad_front_fan 0x04"},
            ]
        },

    ],
    "I2C":[
    ####type 1 represents value obtained compated with value
    ####type 2 represents return True or False
            {"name":"I2C device test" ,"cases":[
                  {"name":" PCA9641 test", "cmd":"grtd_test.py  dev_rd  0 10 0","deal_type":2},
                  {"name":" cpld32 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" cpld33 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" cpld34 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" cpld35 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" cpld36 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" cpld37 test", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" inlet LM75", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" outlet LM75", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" hot-point LM75", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" EEPROM", "cmd":"grtd_test.py  dev_rd  0 32 0","deal_type":2},
                  {"name":" Port 1",        "cmd":"grtd_test.py  dev_rd  11 0050 0","deal_type":2},
                  {"name":" Port 2",        "cmd":"grtd_test.py  dev_rd  12 0050 0","deal_type":2},
                  {"name":" Port 3",        "cmd":"grtd_test.py  dev_rd  13 0050 0","deal_type":2},
                  {"name":" Port 4",        "cmd":"grtd_test.py  dev_rd  14 0050 0","deal_type":2},
                  {"name":" Port 5",        "cmd":"grtd_test.py  dev_rd  15 0050 0","deal_type":2},
                  {"name":" Port 6",        "cmd":"grtd_test.py  dev_rd  16 0050 0","deal_type":2},
                  {"name":" Port 7",        "cmd":"grtd_test.py  dev_rd  17 0050 0","deal_type":2},
                  {"name":" Port 8",        "cmd":"grtd_test.py  dev_rd  18 0050 0","deal_type":2},
                  {"name":" Port 9",        "cmd":"grtd_test.py  dev_rd  19 0050 0","deal_type":2},
                  {"name":" Port 10",        "cmd":"grtd_test.py  dev_rd  20 0050 0","deal_type":2},
                  {"name":" Port 11",        "cmd":"grtd_test.py  dev_rd  21 0050 0","deal_type":2},
                  {"name":" Port 12",        "cmd":"grtd_test.py  dev_rd  22 0050 0","deal_type":2},
                  {"name":" Port 13",        "cmd":"grtd_test.py  dev_rd  23 0050 0","deal_type":2},
                  {"name":" Port 14",        "cmd":"grtd_test.py  dev_rd  24 0050 0","deal_type":2},
                  {"name":" Port 15",        "cmd":"grtd_test.py  dev_rd  25 0050 0","deal_type":2},
                  {"name":" Port 16",        "cmd":"grtd_test.py  dev_rd  26 0050 0","deal_type":2},
                  {"name":" Port 17",        "cmd":"grtd_test.py  dev_rd  27 0050 0","deal_type":2},
                  {"name":" Port 18",        "cmd":"grtd_test.py  dev_rd  28 0050 0","deal_type":2},
                  {"name":" Port 19",        "cmd":"grtd_test.py  dev_rd  29 0050 0","deal_type":2}, 
                  {"name":" Port 20",        "cmd":"grtd_test.py  dev_rd  30 0050 0","deal_type":2},
                  {"name":" Port 21",        "cmd":"grtd_test.py  dev_rd  31 0050 0","deal_type":2},
                  {"name":" Port 22",        "cmd":"grtd_test.py  dev_rd  32 0050 0","deal_type":2},
                  {"name":" Port 23",        "cmd":"grtd_test.py  dev_rd  33 0050 0","deal_type":2},
                  {"name":" Port 24",        "cmd":"grtd_test.py  dev_rd  34 0050 0","deal_type":2},
                  {"name":" Port 25",        "cmd":"grtd_test.py  dev_rd  35 0050 0","deal_type":2},
                  {"name":" Port 26",        "cmd":"grtd_test.py  dev_rd  36 0050 0","deal_type":2},
                  {"name":" Port 27",        "cmd":"grtd_test.py  dev_rd  37 0050 0","deal_type":2},
                  {"name":" Port 28",        "cmd":"grtd_test.py  dev_rd  38 0050 0","deal_type":2},
                  {"name":" Port 29",        "cmd":"grtd_test.py  dev_rd  39 0050 0","deal_type":2},
                  {"name":" Port 30",        "cmd":"grtd_test.py  dev_rd  40 0050 0","deal_type":2},
                  {"name":" Port 31",        "cmd":"grtd_test.py  dev_rd  41 0050 0","deal_type":2},
                  {"name":" Port 32",        "cmd":"grtd_test.py  dev_rd  42 0050 0","deal_type":2},
                  {"name":" Port 33",        "cmd":"grtd_test.py  dev_rd  43 0050 0","deal_type":2},
                  {"name":" Port 34",        "cmd":"grtd_test.py  dev_rd  44 0050 0","deal_type":2},
                  {"name":" Port 35",        "cmd":"grtd_test.py  dev_rd  45 0050 0","deal_type":2},
                  {"name":" Port 36",        "cmd":"grtd_test.py  dev_rd  46 0050 0","deal_type":2},
                  {"name":" Port 37",        "cmd":"grtd_test.py  dev_rd  47 0050 0","deal_type":2},
                  {"name":" Port 38",        "cmd":"grtd_test.py  dev_rd  48 0050 0","deal_type":2},
                  {"name":" Port 39",        "cmd":"grtd_test.py  dev_rd  49 0050 0","deal_type":2},
                  {"name":" Port 40",        "cmd":"grtd_test.py  dev_rd  50 0050 0","deal_type":2},
                  {"name":" Port 41",        "cmd":"grtd_test.py  dev_rd  51 0050 0","deal_type":2},
                  {"name":" Port 42",        "cmd":"grtd_test.py  dev_rd  52 0050 0","deal_type":2},
                  {"name":" Port 43",        "cmd":"grtd_test.py  dev_rd  53 0050 0","deal_type":2},
                  {"name":" Port 44",        "cmd":"grtd_test.py  dev_rd  54 0050 0","deal_type":2},
                  {"name":" Port 45",        "cmd":"grtd_test.py  dev_rd  55 0050 0","deal_type":2},
                  {"name":" Port 46",        "cmd":"grtd_test.py  dev_rd  56 0050 0","deal_type":2},
                  {"name":" Port 47",        "cmd":"grtd_test.py  dev_rd  57 0050 0","deal_type":2},
                  {"name":" Port 48",        "cmd":"grtd_test.py  dev_rd  58 0050 0","deal_type":2},
                  {"name":" Port 49",        "cmd":"grtd_test.py  dev_rd  59 0050 0","deal_type":2},
                  {"name":" Port 50",        "cmd":"grtd_test.py  dev_rd  60 0050 0","deal_type":2},
                  {"name":" Port 51",        "cmd":"grtd_test.py  dev_rd  61 0050 0","deal_type":2},
                  {"name":" Port 52",        "cmd":"grtd_test.py  dev_rd  62 0050 0","deal_type":2},
                  {"name":" Port 53",        "cmd":"grtd_test.py  dev_rd  63 0050 0","deal_type":2},
                  {"name":" Port 54",        "cmd":"grtd_test.py  dev_rd  64 0050 0","deal_type":2},
                  {"name":" Port 55",        "cmd":"grtd_test.py  dev_rd  65 0050 0","deal_type":2},
                  {"name":" Port 56",        "cmd":"grtd_test.py  dev_rd  66 0050 0","deal_type":2},
                  {"name":" Port 57",        "cmd":"grtd_test.py  dev_rd  67 0050 0","deal_type":2},
                  {"name":" Port 58",        "cmd":"grtd_test.py  dev_rd  68 0050 0","deal_type":2},
                  {"name":" Port 59",        "cmd":"grtd_test.py  dev_rd  69 0050 0","deal_type":2},
                  {"name":" Port 60",        "cmd":"grtd_test.py  dev_rd  70 0050 0","deal_type":2},
                  {"name":" Port 61",        "cmd":"grtd_test.py  dev_rd  71 0050 0","deal_type":2},
                  {"name":" Port 62",        "cmd":"grtd_test.py  dev_rd  72 0050 0","deal_type":2},
                  {"name":" Port 63",        "cmd":"grtd_test.py  dev_rd  73 0050 0","deal_type":2},
                  {"name":" Port 64",        "cmd":"grtd_test.py  dev_rd  74 0050 0","deal_type":2},
                ]
            },
    ],
}

PCIe_DEV_LIST = []
PCIe_SPEED_ITEM = []

################################Manufacturing-Test-Adaption-Area#######################################################
