#!/usr/bin/python3
# -*- coding: UTF-8 -*-
#-------------------------------------------------------------------------------
# Name:         ruijieconfig.py 
# Purpose:      block the difference between various product/onie version for other module
#
# Author:      rd
#
# Created:     02/07/2018
# Copyright:   (c) rd 2018
#-------------------------------------------------------------------------------
import sys
import os
from rjutil.baseutil import get_machine_info
from rjutil.baseutil import get_platform_info

__all__ = ["getdeviceplatform", "get_rjconfig_info", "MONITOR_CONST", "MAILBOX_DIR", "DEVICE",
           "GLOBALCONFIG", "GLOBALINITPARAM", "GLOBALINITCOMMAND", "MAC_LED_RESET", "STARTMODULE", 
           "fanloc", "RUIJIE_CARDID", "RUIJIE_PRODUCTNAME", "RUIJIE_PART_NUMBER",
           "RUIJIE_LABEL_REVISION", "RUIJIE_MAC_SIZE", "RUIJIE_MANUF_NAME", "RUIJIE_MANUF_COUNTRY",
           "RUIJIE_VENDOR_NAME", "RUIJIE_DIAG_VERSION", "RUIJIE_SERVICE_TAG", "E2_PROTECT", 
           "E2_LOC", "FAN_PROTECT", "FANS_DEF", "MONITOR_FANS_LED", "MONITOR_SYS_FAN_LED",
           "MONITOR_SYS_PSU_LED", "MONITOR_FAN_STATUS", "MONITOR_PSU_STATUS", "MONITOR_DEV_STATUS",
           "MONITOR_DEV_STATUS_DECODE", "DEV_LEDS", "MAC_AVS_PARAM", "MAC_DEFAULT_PARAM",
           "FRULISTS", "rg_eeprom", "i2ccheck_params", "FANCTROLDEBUG", "DEVMONITORDEBUG"]

def getdeviceplatform():
    x = get_platform_info(get_machine_info())
    if x != None:
        filepath = "/usr/share/sonic/device/" + x
    return filepath


platform = get_platform_info(get_machine_info())  #         platform         get platform info             x86_64-ruijie_b6520-64cq-r0
platformpath = getdeviceplatform()             #         platformpath     get mappable docker contents    /usr/share/sonic/device/x86_64-ruijie_b6520-64cq-r0
MAILBOX_DIR = "/sys/bus/i2c/devices/"        # sysfs top contents
grtd_productfile = (platform + "_config").replace("-","_")
common_productfile = "ruijiecommon"
configfile_pre   =  "/usr/local/bin/"   # py's contents， use /usr/local/bin temporarily

sys.path.append(platformpath)
sys.path.append(configfile_pre)

def get_rjconfig_info(attr_key):
    rjconf_filename = platformpath + "/plugins" + "/rj.conf"
    if not os.path.isfile(rjconf_filename):
        return None
    with open(rjconf_filename) as rjconf_file:
        for line in rjconf_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            if tokens[0] == attr_key:
                return tokens[1].strip()
    return None

#####BMC-Password###
OPENBMC_PASSWORD = get_rjconfig_info("OPENBMC_PASSWORD")
OPENBMC_PASSWORD = OPENBMC_PASSWORD if(OPENBMC_PASSWORD != None) else "0penBmc"

############################################################################################
##  if there is no specific file, use common file
module_product = None
if os.path.exists(configfile_pre + grtd_productfile + ".py"):
    module_product  = __import__(grtd_productfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + common_productfile + ".py"):
    module_product  = __import__(common_productfile, globals(), locals(), [], 0)
else:
    print("No Configuration existed, quit")
    exit(-1)
############################################################################################

DEVICE  = module_product.DEVICE

##########Driver loading needs parameters
#get different product configuration
RUIJIE_GLOBALCONFIG ={
    "DRIVERLISTS":module_product.DRIVERLISTS,
    "QSFP": {"startbus":module_product.PCA9548START, "endbus":module_product.PCA9548BUSEND},
    "DEVS": DEVICE
}
GLOBALCONFIG = RUIJIE_GLOBALCONFIG
GLOBALINITPARAM = module_product.INIT_PARAM
GLOBALINITCOMMAND = module_product.INIT_COMMAND

fancontrol_loc        = module_product.fancontrol_loc
fancontrol_config_loc = module_product.fancontrol_config_loc
MAC_LED_RESET          = module_product.MAC_LED_RESET
###########Stat-up module parameters
STARTMODULE = module_product.STARTMODULE
FIRMWARE_TOOLS = module_product.FIRMWARE_TOOLS


##########Manufacturing-Test need parameters
FACTESTMODULE = module_product.FACTESTMODULE
TESTCASE      = module_product.TESTCASE
menuList      = module_product.menuList
alltest       = module_product.alltest
diagtestall   = module_product.diagtestall
looptest      = module_product.looptest
fanloc        = module_product.fanloc
fanlevel      = module_product.fanlevel        # fan adjustment level
TEMPIDCHANGE  = module_product.TEMPIDCHANGE
CPLDVERSIONS  = module_product.CPLDVERSIONS
RUIJIE_CARDID      = module_product.RUIJIE_CARDID
RUIJIE_PRODUCTNAME = module_product.RUIJIE_PRODUCTNAME

RUIJIE_PART_NUMBER = module_product.RUIJIE_PART_NUMBER
RUIJIE_LABEL_REVISION = module_product.RUIJIE_LABEL_REVISION
RUIJIE_ONIE_VERSION = module_product.RUIJIE_ONIE_VERSION
RUIJIE_MAC_SIZE = module_product.RUIJIE_MAC_SIZE
RUIJIE_MANUF_NAME = module_product.RUIJIE_MANUF_NAME
RUIJIE_MANUF_COUNTRY = module_product.RUIJIE_MANUF_COUNTRY
RUIJIE_VENDOR_NAME = module_product.RUIJIE_VENDOR_NAME
RUIJIE_DIAG_VERSION = module_product.RUIJIE_DIAG_VERSION
RUIJIE_SERVICE_TAG = module_product.RUIJIE_SERVICE_TAG

E2_PROTECT         = module_product.E2_PROTECT
E2_LOC             = module_product.E2_LOC
FAN_PROTECT        = module_product.FAN_PROTECT

FANS_DEF           = module_product.FANS_DEF
MONITOR_SYS_LED    = module_product.MONITOR_SYS_LED
MONITOR_FANS_LED =   module_product.MONITOR_FANS_LED
MONITOR_SYS_FAN_LED = module_product.MONITOR_SYS_FAN_LED
MONITOR_SYS_PSU_LED = module_product.MONITOR_SYS_PSU_LED
MONITOR_FAN_STATUS  = module_product.MONITOR_FAN_STATUS
MONITOR_PSU_STATUS  = module_product.MONITOR_PSU_STATUS
MONITOR_DEV_STATUS  = module_product.MONITOR_DEV_STATUS
MONITOR_DEV_STATUS_DECODE  = module_product.MONITOR_DEV_STATUS_DECODE
DEV_MONITOR_PARAM  = module_product.DEV_MONITOR_PARAM
SLOT_MONITOR_PARAM = module_product.SLOT_MONITOR_PARAM


DEV_LEDS            = module_product.DEV_LEDS
MEM_SLOTS            = module_product.MEM_SLOTS

MAC_AVS_PARAM      = module_product.MAC_AVS_PARAM
MAC_DEFAULT_PARAM  = module_product.MAC_DEFAULT_PARAM
E2TYPE    = module_product.E2TYPE
FRULISTS  = module_product.FRULISTS
rg_eeprom    = "%d-%04x/eeprom" % (E2_LOC["bus"], E2_LOC["devno"])
factest_module = module_product.factest_module

LOCAL_LED_CONTROL      = module_product.LOCAL_LED_CONTROL

PCIe_DEV_LIST = module_product.PCIe_DEV_LIST
PCIe_SPEED_ITEM = module_product.PCIe_SPEED_ITEM
i2ccheck_params = module_product.i2ccheck_params


class MONITOR_CONST:
    TEMP_MIN = module_product.MONITOR_TEMP_MIN
    K =module_product.MONITOR_K
    MAC_IN =module_product.MONITOR_MAC_IN
    DEFAULT_SPEED =module_product.MONITOR_DEFAULT_SPEED
    MAX_SPEED =module_product.MONITOR_MAX_SPEED
    MIN_SPEED =module_product.MONITOR_MIN_SPEED
    MAC_ERROR_SPEED =module_product.MONITOR_MAC_ERROR_SPEED
    FAN_TOTAL_NUM =module_product.MONITOR_FAN_TOTAL_NUM
    MAC_UP_TEMP =module_product.MONITOR_MAC_UP_TEMP
    MAC_LOWER_TEMP =module_product.MONITOR_MAC_LOWER_TEMP
    MAC_MAX_TEMP   = module_product.MONITOR_MAC_MAX_TEMP
    
    MAC_WARNING_THRESHOLD =  module_product.MONITOR_MAC_WARNING_THRESHOLD
    OUTTEMP_WARNING_THRESHOLD = module_product.MONITOR_OUTTEMP_WARNING_THRESHOLD
    BOARDTEMP_WARNING_THRESHOLD = module_product.MONITOR_BOARDTEMP_WARNING_THRESHOLD
    CPUTEMP_WARNING_THRESHOLD = module_product.MONITOR_CPUTEMP_WARNING_THRESHOLD
    INTEMP_WARNING_THRESHOLD = module_product.MONITOR_INTEMP_WARNING_THRESHOLD

    MAC_CRITICAL_THRESHOLD = module_product.MONITOR_MAC_CRITICAL_THRESHOLD
    OUTTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_OUTTEMP_CRITICAL_THRESHOLD
    BOARDTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_BOARDTEMP_CRITICAL_THRESHOLD
    CPUTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_CPUTEMP_CRITICAL_THRESHOLD
    INTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_INTEMP_CRITICAL_THRESHOLD
    CRITICAL_NUM       = module_product.MONITOR_CRITICAL_NUM
    SHAKE_TIME = module_product.MONITOR_SHAKE_TIME
    MONITOR_INTERVAL= module_product.MONITOR_INTERVAL
    MONITOR_FALL_TEMP = module_product.MONITOR_FALL_TEMP

    MONITOR_MAC_SOURCE_SYSFS = module_product.MONITOR_MAC_SOURCE_SYSFS
    MONITOR_MAC_SOURCE_PATH = module_product.MONITOR_MAC_SOURCE_PATH

FANCTROLDEBUG = 0 # 1 means enable
DEVMONITORDEBUG = 0 # 1 means enable

