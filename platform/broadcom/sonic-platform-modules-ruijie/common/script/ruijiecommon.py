# -*- coding: UTF-8 -*-
#-------------------------------------------------------------------------------
# Name:        Ruijie python common module
# Purpose:     called by other modules
#
# Author:      rd
#
# Created:     02/07/2018
# Copyright:   (c) rd 2018
#-------------------------------------------------------------------------------

################################driver-load-adaption#######################################################
#   need to export interface
###################################################################################################

__all__ = ["fancontrol_loc", "fancontrol_config_loc", "GLOBALCONFIG", "MONITOR_CONST", 
           "RUIJIE_PART_NUMBER", "RUIJIE_LABEL_REVISION", "RUIJIE_ONIE_VERSION", "RUIJIE_MAC_SIZE",
           "RUIJIE_MANUF_NAME", "RUIJIE_MANUF_COUNTRY", "RUIJIE_VENDOR_NAME", "RUIJIE_DIAG_VERSION",
           "RUIJIE_SERVICE_TAG", "DEV_LEDS", "MEM_SLOTS", "LOCAL_LED_CONTROL", "FIRMWARE_TOOLS", 
           "STARTMODULE", "i2ccheck_params", "FANS_DEF", "factest_module", "MONITOR_TEMP_MIN",
           "MONITOR_K", "MONITOR_MAC_IN", "MONITOR_DEFAULT_SPEED", "MONITOR_MAX_SPEED",
           "MONITOR_MIN_SPEED", "MONITOR_MAC_ERROR_SPEED","MONITOR_FAN_TOTAL_NUM", 
           "MONITOR_MAC_UP_TEMP", "MONITOR_MAC_LOWER_TEMP","MONITOR_MAC_MAX_TEMP", 
           "MONITOR_FALL_TEMP","MONITOR_MAC_WARNING_THRESHOLD", "MONITOR_OUTTEMP_WARNING_THRESHOLD",
           "MONITOR_BOARDTEMP_WARNING_THRESHOLD", "MONITOR_CPUTEMP_WARNING_THRESHOLD",
           "MONITOR_INTEMP_WARNING_THRESHOLD", "MONITOR_MAC_CRITICAL_THRESHOLD",
           "MONITOR_OUTTEMP_CRITICAL_THRESHOLD", "MONITOR_BOARDTEMP_CRITICAL_THRESHOLD",
           "MONITOR_CPUTEMP_CRITICAL_THRESHOLD", "MONITOR_INTEMP_CRITICAL_THRESHOLD",
           "MONITOR_CRITICAL_NUM", "MONITOR_SHAKE_TIME", "MONITOR_INTERVAL",
           "MONITOR_MAC_SOURCE_SYSFS", "MONITOR_MAC_SOURCE_PATH", "MAC_AVS_PARAM",
           "MAC_DEFAULT_PARAM", "MONITOR_SYS_LED", "MONITOR_SYS_FAN_LED", "MONITOR_FANS_LED",
           "MONITOR_SYS_PSU_LED", "MONITOR_FAN_STATUS", "MONITOR_PSU_STATUS", "MONITOR_DEV_STATUS",
           "MONITOR_DEV_STATUS_DECODE", "DEV_MONITOR_PARAM", "SLOT_MONITOR_PARAM", "fanloc",
           "PCA9548START", "PCA9548BUSEND", "RUIJIE_CARDID", "RUIJIE_PRODUCTNAME", "FAN_PROTECT",
           "rg_eeprom", "E2_LOC", "E2_PROTECT", "MAC_LED_RESET", "INIT_PARAM", "INIT_COMMAND",
           "CPLDVERSIONS", "DRIVERLISTS", "DEVICE", "E2TYPE", "FRULISTS", "fanlevel_6510",
           "fanlevel_6520", "fanlevel", "TEMPIDCHANGE", "FACTESTMODULE", "item1",
           "test_sys_reload_item", "test_sys_item", "test_temp_item", "test_mem_item",
           "test_hd_item", "test_rtc_item", "test_i2c_item", "test_cpld_item",
           "test_portframe_item", "test_sysled_item", "test_fan_item", "test_power_item",
           "test_usb_item", "test_prbs_item", "test_portbroadcast_item", "test_debug_level",
           "test_log_level", "test_setmac", "test_setrtc", "log_level_critical", "log_level_debug",
           "log_level_error", "log_level_info", "log_level_notset", "log_level_warning",
           "test_e2_setmac_item", "test_bmc_setmac_item", "test_fan_setmac_item", "alltest",
           "looptest", "diagtestall", "menuList", "TESTCASE", "PCIe_DEV_LIST", "PCIe_SPEED_ITEM"]

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
#start-up module
STARTMODULE  = {
                "fancontrol":1,
                "avscontrol":1
                    }

i2ccheck_params = {"busend":"i2c-66","retrytime":6}

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

#################fan adjustment parameters ##############################
MONITOR_TEMP_MIN           = 38    # temperature before speed-adjustment
MONITOR_K                  = 11    # adjustment algorithm
MONITOR_MAC_IN             = 35    # temperature difference between mac and chip(backup)
MONITOR_DEFAULT_SPEED      = 0x60  # default speed
MONITOR_MAX_SPEED          = 0xFF  # maximum speed
MONITOR_MIN_SPEED          = 0x33  # minimum speed
MONITOR_MAC_ERROR_SPEED    = 0XBB  # MAC abnormal speed 
MONITOR_FAN_TOTAL_NUM      = 4     # 3+1 redundancy design, report to syslog if there is a error
MONITOR_MAC_UP_TEMP        = 50    # MAC compared with inlet up
MONITOR_MAC_LOWER_TEMP     = -50   # MAC compared with outlet down
MONITOR_MAC_MAX_TEMP       = 100   # 

MONITOR_FALL_TEMP = 4               # adjustment reduced temperature
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
MONITOR_SHAKE_TIME                = 20 #anti-shake times
MONITOR_INTERVAL                   = 60

MONITOR_MAC_SOURCE_SYSFS = 0 #1 get mac temperature from sysfs ,0 get mac temperature from bcmcmd 
MONITOR_MAC_SOURCE_PATH = None #sysfs path

###################################################################


#####################MAC调压parameters (B6510)####################################
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

# default 6520 configuration
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
  "sdkreg":"DMU_PCU_OTP_CONFIG_8", # SDK register name
  "sdktype": 1,                    # type 0 represents no shift operation / 1 represents shift operation
  "macregloc":24 ,                 # shift operation
  "mask": 0xff                     # mask after shift
}

MONITOR_SYS_LED = [
        {"bus":2,"devno":0x33, "addr":0xb2, "yellow":0x06, "red":0x02,"green":0x04},
        {"bus":2,"devno":0x32, "addr":0x72, "yellow":0x06, "red":0x02,"green":0x04}]

MONITOR_SYS_FAN_LED =[
          {"bus":2,"devno":0x33, "addr":0xb4, "yellow":0x06, "red":0x02,"green":0x04},
    ]

MONITOR_FANS_LED = [
          {"bus":2,"devno":0x32, "addr":0x23, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x24, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x25, "green":0x09, "red":0x0a},
          {"bus":2,"devno":0x32, "addr":0x26, "green":0x09, "red":0x0a}]


MONITOR_SYS_PSU_LED =[
          {"bus":2,"devno":0x33, "addr":0xb3, "yellow":0x06, "red":0x02,"green":0x04},
    ]

MONITOR_FAN_STATUS = [
    {'status':'green' , 'minOkNum':4,'maxOkNum':4},
    {'status':'yellow', 'minOkNum':3,'maxOkNum':3},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':2},
    ]

MONITOR_PSU_STATUS = [
    {'status':'green' , 'minOkNum':2,'maxOkNum':2},
    {'status':'yellow', 'minOkNum':1,'maxOkNum':1},
    {'status':'red'   , 'minOkNum':0,'maxOkNum':0},
    ]

MONITOR_DEV_STATUS = {}
MONITOR_DEV_STATUS_DECODE = {}
DEV_MONITOR_PARAM = {}
SLOT_MONITOR_PARAM = {}


fanloc = {"name":"fanset","location":"0-0032/fan_speed_set"}
#####################MAC-Voltage-Adjustment-Parameters####################################


####================================Adaption-Area================================
#### RUIJIE_COMMON common configuration head 
#### “platform”    specific configuration head 
####
PCA9548START  = 11
PCA9548BUSEND = 74

RUIJIE_CARDID      = 0x00004040
RUIJIE_PRODUCTNAME = "ruijie_b6510"

FAN_PROTECT = {"bus":0, "devno":0x32, "addr":0x19, "open":0x00, "close":0x0f}
rg_eeprom  = "2-0057/eeprom"
E2_LOC = {"bus":2, "devno":0x57}
E2_PROTECT ={"bus":2, "devno":0x33, "addr":0xb0, "open":0, "close":1}
MAC_LED_RESET = {"pcibus":8, "slot":0, "fn":0, "bar":0, "offset":64, "reset":0x98}

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
            {"loc":fanloc["location"], "value":"80"}
]

INIT_COMMAND = [
]

CPLDVERSIONS = [
        {"loc":"2-0033/cpld_version","des":"MAC Board 上CPLDA"},
        {"loc":"2-0035/cpld_version","des":"MAC Board 上CPLDB"},
        {"loc":"2-0037/cpld_version","des":"CPU Board 上cpld"}
]

## Driver List
## 
DRIVERLISTS = [
        "i2c_dev",
        "i2c_algo_bit",
        "i2c_gpio",
        "i2c_mux",
        "i2c_mux_pca9641",
        "i2c_mux_pca954x",  #  force_deselect_on_exit=1
        "eeprom",
        "at24",
        "ruijie_platform",
        "rg_cpld",
        "rg_fan",
        "rg_psu",
        "csu550",
        "rg_gpio_xeon",
        #IPMIdriver
        "ipmi_msghandler",
        "ipmi_devintf",
        "ipmi_si",
]

DEVICE = [
        {"name":"pca9641","bus":0 ,"loc":0x10 },
        {"name":"pca9548","bus":2 ,"loc":0x70 },
        {"name":"lm75","bus": 2,   "loc":0x48 },
        {"name":"lm75","bus": 2,   "loc":0x49 },
        {"name":"lm75","bus": 2,   "loc":0x4a },
        {"name":"24c02","bus":2 , "loc":0x57 },
        {"name":"rg_cpld","bus":2 ,"loc":0x33 },
        {"name":"rg_cpld","bus":2 ,"loc":0x35 },
        {"name":"rg_cpld","bus":2 ,"loc":0x37 },
        {"name":"pca9548","bus":1,"loc":0x70 },
        {"name":"pca9548","bus":1,"loc":0x71 },
        {"name":"pca9548","bus":1,"loc":0x72 },
        {"name":"pca9548","bus":1,"loc":0x73 },
        {"name":"pca9548","bus":1,"loc":0x74 },
        {"name":"pca9548","bus":1,"loc":0x75 },
        {"name":"pca9548","bus":1,"loc":0x76 },
        {"name":"pca9548","bus":1,"loc":0x77 },
        {"name":"rg_fan","bus":3,"loc":0x53 },
        {"name":"rg_fan","bus":4,"loc":0x53 },
        {"name":"rg_fan","bus":5,"loc":0x53 },
        #{"name":"rg_fan","bus":6,"loc":0x53 }, #specific fan
        {"name":"rg_psu","bus":7 ,"loc":0x50 },
        {"name":"csu550","bus":7 ,"loc":0x58 },
        {"name":"rg_psu","bus":8 ,"loc":0x53 },
        {"name":"csu550","bus":8 ,"loc":0x5b },
]

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

fanlevel = fanlevel_6520

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




