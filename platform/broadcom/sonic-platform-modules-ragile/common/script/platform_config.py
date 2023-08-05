#!/usr/bin/python3

import sys
import os
from wbutil.baseutil import get_machine_info
from wbutil.baseutil import get_platform_info
from wbutil.baseutil import get_board_id

__all__ = [
    "MAILBOX_DIR",
    "PLATFORM_GLOBALCONFIG",
    "GLOBALCONFIG",
    "STARTMODULE",
    "MAC_LED_RESET",
    "MAC_DEFAULT_PARAM",
    "DEV_MONITOR_PARAM",
    "SLOT_MONITOR_PARAM",
    "MANUINFO_CONF",
    "REBOOT_CTRL_PARAM",
    "PMON_SYSLOG_STATUS",
    "REBOOT_CAUSE_PARA",
    "UPGRADE_SUMMARY",
    "WARM_UPGRADE_PARAM",
    "WARM_UPG_FLAG",
    "WARM_UPGRADE_STARTED_FLAG",
    "PLATFORM_E2_CONF",
    "AIR_FLOW_CONF",
    "AIRFLOW_RESULT_FILE",
    "GLOBALINITPARAM",
    "GLOBALINITCOMMAND",
    "GLOBALINITPARAM_PRE",
    "GLOBALINITCOMMAND_PRE",
    "MONITOR_CONST",
    "PSU_FAN_FOLLOW",
    "MONITOR_SYS_LED",
    "MONITOR_FANS_LED",
    "MONITOR_SYS_FAN_LED",
    "MONITOR_SYS_PSU_LED",
    "MONITOR_FAN_STATUS",
    "MONITOR_PSU_STATUS",
    "MONITOR_DEV_STATUS",
    "MONITOR_DEV_STATUS_DECODE",
    "DEV_LEDS",
    "fanloc"
]


def getdeviceplatform():
    x = get_platform_info(get_machine_info())
    if x is not None:
        filepath = "/usr/share/sonic/device/" + x
        return filepath
    return None


platform = get_platform_info(get_machine_info())
board_id = get_board_id(get_machine_info())
platformpath = getdeviceplatform()
MAILBOX_DIR = "/sys/bus/i2c/devices/"
grtd_productfile = (platform + "_config").replace("-", "_")
common_productfile = "platform_common"
platform_configfile = (platform + "_" + board_id + "_config").replace("-", "_")  # platfrom + board_id
configfile_pre = "/usr/local/bin/"
sys.path.append(platformpath)
sys.path.append(configfile_pre)

############################################################################################
if os.path.exists(configfile_pre + platform_configfile + ".py"):
    module_product = __import__(platform_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + grtd_productfile + ".py"):
    module_product = __import__(grtd_productfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + common_productfile + ".py"):
    module_product = __import__(common_productfile, globals(), locals(), [], 0)
else:
    print("config file not exist")
    sys.exit(-1)
############################################################################################

PLATFORM_GLOBALCONFIG = {
    "DRIVERLISTS": module_product.DRIVERLISTS,
    "OPTOE": module_product.OPTOE,
    "DEVS": module_product.DEVICE,
    "BLACKLIST_DRIVERS": module_product.BLACKLIST_DRIVERS
}
GLOBALCONFIG = PLATFORM_GLOBALCONFIG

# start module parameters
STARTMODULE = module_product.STARTMODULE

# mac led reset parameter
MAC_LED_RESET = module_product.MAC_LED_RESET

# avscontrol parameter
MAC_DEFAULT_PARAM = module_product.MAC_DEFAULT_PARAM

# dev_monitor parameter
DEV_MONITOR_PARAM = module_product.DEV_MONITOR_PARAM

# slot_monitor parameter
SLOT_MONITOR_PARAM = module_product.SLOT_MONITOR_PARAM

# platform_manufacturer parameter
MANUINFO_CONF = module_product.MANUINFO_CONF

# reboot_ctrl parameter
REBOOT_CTRL_PARAM = module_product.REBOOT_CTRL_PARAM

# pmon_syslog parameter
PMON_SYSLOG_STATUS = module_product.PMON_SYSLOG_STATUS

# reboot_cause parameter
REBOOT_CAUSE_PARA = module_product.REBOOT_CAUSE_PARA

# upgrade parameter
UPGRADE_SUMMARY = module_product.UPGRADE_SUMMARY

# warm_uprade parameter
WARM_UPGRADE_PARAM = module_product.WARM_UPGRADE_PARAM
WARM_UPG_FLAG = module_product.WARM_UPG_FLAG
WARM_UPGRADE_STARTED_FLAG = module_product.WARM_UPGRADE_STARTED_FLAG

# platform_e2 parameter
PLATFORM_E2_CONF = module_product.PLATFORM_E2_CONF

# generate_airflow parameter
AIR_FLOW_CONF = module_product.AIR_FLOW_CONF
AIRFLOW_RESULT_FILE = module_product.AIRFLOW_RESULT_FILE

# Initialization parameters
GLOBALINITPARAM = module_product.INIT_PARAM
GLOBALINITCOMMAND = module_product.INIT_COMMAND
GLOBALINITPARAM_PRE = module_product.INIT_PARAM_PRE
GLOBALINITCOMMAND_PRE = module_product.INIT_COMMAND_PRE

################################ fancontrol parameter###################################


class MONITOR_CONST:
    TEMP_MIN = module_product.MONITOR_TEMP_MIN
    K = module_product.MONITOR_K
    MAC_IN = module_product.MONITOR_MAC_IN
    DEFAULT_SPEED = module_product.MONITOR_DEFAULT_SPEED
    MAX_SPEED = module_product.MONITOR_MAX_SPEED
    MIN_SPEED = module_product.MONITOR_MIN_SPEED
    MAC_ERROR_SPEED = module_product.MONITOR_MAC_ERROR_SPEED
    FAN_TOTAL_NUM = module_product.MONITOR_FAN_TOTAL_NUM
    MAC_UP_TEMP = module_product.MONITOR_MAC_UP_TEMP
    MAC_LOWER_TEMP = module_product.MONITOR_MAC_LOWER_TEMP
    MAC_MAX_TEMP = module_product.MONITOR_MAC_MAX_TEMP

    MAC_WARNING_THRESHOLD = module_product.MONITOR_MAC_WARNING_THRESHOLD
    OUTTEMP_WARNING_THRESHOLD = module_product.MONITOR_OUTTEMP_WARNING_THRESHOLD
    BOARDTEMP_WARNING_THRESHOLD = module_product.MONITOR_BOARDTEMP_WARNING_THRESHOLD
    CPUTEMP_WARNING_THRESHOLD = module_product.MONITOR_CPUTEMP_WARNING_THRESHOLD
    INTEMP_WARNING_THRESHOLD = module_product.MONITOR_INTEMP_WARNING_THRESHOLD

    MAC_CRITICAL_THRESHOLD = module_product.MONITOR_MAC_CRITICAL_THRESHOLD
    OUTTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_OUTTEMP_CRITICAL_THRESHOLD
    BOARDTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_BOARDTEMP_CRITICAL_THRESHOLD
    CPUTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_CPUTEMP_CRITICAL_THRESHOLD
    INTEMP_CRITICAL_THRESHOLD = module_product.MONITOR_INTEMP_CRITICAL_THRESHOLD
    CRITICAL_NUM = module_product.MONITOR_CRITICAL_NUM
    SHAKE_TIME = module_product.MONITOR_SHAKE_TIME
    MONITOR_INTERVAL = module_product.MONITOR_INTERVAL
    MONITOR_LED_INTERVAL = module_product.MONITOR_LED_INTERVAL
    MONITOR_FALL_TEMP = module_product.MONITOR_FALL_TEMP
    MONITOR_PID_FLAG = module_product.MONITOR_PID_FLAG
    MONITOR_PID_MODULE = module_product.MONITOR_PID_MODULE

    MONITOR_MAC_SOURCE_SYSFS = module_product.MONITOR_MAC_SOURCE_SYSFS
    MONITOR_MAC_SOURCE_PATH = module_product.MONITOR_MAC_SOURCE_PATH


PSU_FAN_FOLLOW = module_product.PSU_FAN_FOLLOW
MONITOR_SYS_LED = module_product.MONITOR_SYS_LED
MONITOR_FANS_LED = module_product.MONITOR_FANS_LED
MONITOR_SYS_FAN_LED = module_product.MONITOR_SYS_FAN_LED
MONITOR_SYS_PSU_LED = module_product.MONITOR_SYS_PSU_LED
MONITOR_FAN_STATUS = module_product.MONITOR_FAN_STATUS
MONITOR_PSU_STATUS = module_product.MONITOR_PSU_STATUS
MONITOR_DEV_STATUS = module_product.MONITOR_DEV_STATUS
MONITOR_DEV_STATUS_DECODE = module_product.MONITOR_DEV_STATUS_DECODE
DEV_LEDS = module_product.DEV_LEDS
fanloc = module_product.fanloc
