#!/usr/bin/python3

import sys
import os
from wbutil.baseutil import get_platform_info
from wbutil.baseutil import get_board_id
from wbutil.baseutil import get_sub_version


__all__ = [
    "module_product",
    "MAILBOX_DIR",
    "PLATFORM_GLOBALCONFIG",
    "GLOBALCONFIG",
    "STARTMODULE",
    "MAC_DEFAULT_PARAM",
    "DEV_MONITOR_PARAM",
    "SLOT_MONITOR_PARAM",
    "MANUINFO_CONF",
    "REBOOT_CTRL_PARAM",
    "PMON_SYSLOG_STATUS",
    "REBOOT_CAUSE_PARA",
    "UPGRADE_SUMMARY",
    "FW_UPGRADE_STARTED_FLAG",
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
    "SET_MAC_CONF",
    "SET_FW_MAC_CONF",
    "DRVIER_UPDATE_CONF",
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
    "fanloc",
    "PLATFORM_POWER_CONF",
    "PRODUCT_NAME_CONF",
    "PRODUCT_INFO_CONF",
    "HW_MONITOR_PARAM",
    "POWER_CTRL_CONF",
    "S3IP_DEBUG_FILE_LIST",
    "BSP_COMMON_LOG_PATH",
    "BSP_COMMON_LOG_DIR",
    "UBOOT_INFO_CONF",
    "DFX_XDPE_MONITOR_INFO",
    "DFX_REG_MONITOR_PARAM",
    "PLATFORM_SENSORS_CFG",
    "PLUGINS_DOCKER_STARTED_FLAG",
    "SET_MAC_NEED_REBOOT",
]

platform = "NA"
board_id = "NA"
sub_ver = "NA"

def getdeviceplatform():
    status, val_tmp = get_platform_info()
    if status is True:
        filepath = "/usr/share/sonic/device/" + val_tmp
        return filepath
    return None

def get_product_info():
    global platform
    global board_id
    global sub_ver

    status, val_tmp = get_platform_info()
    if status is True:
        platform = val_tmp

    status, val_tmp = get_board_id()
    if status is True:
        board_id = val_tmp

    status, val_tmp = get_sub_version()
    if status is True:
        sub_ver = val_tmp
    return


get_product_info()
platformpath = getdeviceplatform()
MAILBOX_DIR = "/sys/bus/i2c/devices/"
platform_subver_configfile = (platform + "_" + board_id + "_" + sub_ver + "_config")  # platfrom + board_id + sub_ver
platform_boardid_configfile = (platform + "_" + board_id + "_config") # platfrom + board_id
platform_configfile = (platform + "_config")
# bmc use _platform_config
platform_config_file_bmc = (platform + "_platform_config")
common_productfile = "platform_common"


configfile_pre = "/usr/local/bin/"
sys.path.append(platformpath)
sys.path.append(configfile_pre)

############################################################################################
if os.path.exists(configfile_pre + platform_subver_configfile + ".py"):
    module_product = __import__(platform_subver_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_boardid_configfile + ".py"):
    module_product = __import__(platform_boardid_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_configfile + ".py"):
    module_product = __import__(platform_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_config_file_bmc + ".py"):
    module_product = __import__(platform_config_file_bmc, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + common_productfile + ".py"):
    module_product = __import__(common_productfile, globals(), locals(), [], 0)
else:
    print("config file not exist")
    sys.exit(-1)
############################################################################################


def get_config_param(name, default):
    return getattr(module_product, name, default)


PLATFORM_GLOBALCONFIG = {
    "DRIVERLISTS": module_product.DRIVERLISTS,
    "OPTOE": module_product.OPTOE,
    "DEVS": module_product.DEVICE,
    "BLACKLIST_DRIVERS": module_product.BLACKLIST_DRIVERS,
    "DRIVERLISTS_CHECK":module_product.DRIVERLISTS_CHECK
}
GLOBALCONFIG = PLATFORM_GLOBALCONFIG

# start module parameters
STARTMODULE = module_product.STARTMODULE

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
FW_UPGRADE_STARTED_FLAG = module_product.FW_UPGRADE_STARTED_FLAG

# warm_uprade parameter
WARM_UPGRADE_PARAM = module_product.WARM_UPGRADE_PARAM
WARM_UPG_FLAG = module_product.WARM_UPG_FLAG
WARM_UPGRADE_STARTED_FLAG = module_product.WARM_UPGRADE_STARTED_FLAG

# platform_e2 parameter
PLATFORM_E2_CONF = module_product.PLATFORM_E2_CONF

# generate_airflow parameter
AIR_FLOW_CONF = module_product.AIR_FLOW_CONF
AIRFLOW_RESULT_FILE = module_product.AIRFLOW_RESULT_FILE

# bsp common log dir
BSP_COMMON_LOG_DIR = module_product.BSP_COMMON_LOG_DIR

# bsp common log path
BSP_COMMON_LOG_PATH = module_product.BSP_COMMON_LOG_PATH

# Initialization parameters
GLOBALINITPARAM = module_product.INIT_PARAM
GLOBALINITCOMMAND = module_product.INIT_COMMAND
GLOBALINITPARAM_PRE = module_product.INIT_PARAM_PRE
GLOBALINITCOMMAND_PRE = module_product.INIT_COMMAND_PRE

# Set eth mac address parameters
SET_MAC_CONF = module_product.SET_MAC_CONF
SET_FW_MAC_CONF = module_product.SET_FW_MAC_CONF

# driver update config
DRVIER_UPDATE_CONF = module_product.DRVIER_UPDATE_CONF

# platform power config
PLATFORM_POWER_CONF = module_product.PLATFORM_POWER_CONF

# power control config
POWER_CTRL_CONF = module_product.POWER_CTRL_CONF

# product name config
PRODUCT_NAME_CONF = module_product.PRODUCT_NAME_CONF

# product info config
PRODUCT_INFO_CONF = module_product.PRODUCT_INFO_CONF

HW_MONITOR_PARAM = module_product.HW_MONITOR_PARAM

# s3ip dev present debug file config
S3IP_DEBUG_FILE_LIST = module_product.S3IP_DEBUG_FILE_LIST

# uboot info config
UBOOT_INFO_CONF = module_product.UBOOT_INFO_CONF

# xdpe monitor info
DFX_XDPE_MONITOR_INFO = module_product.DFX_XDPE_MONITOR_INFO

# dfx reg monitor
DFX_REG_MONITOR_PARAM = module_product.DFX_REG_MONITOR_PARAM

PLATFORM_SENSORS_CFG = module_product.PLATFORM_SENSORS_CFG

# plugin start flag
PLUGINS_DOCKER_STARTED_FLAG = module_product.PLUGINS_DOCKER_STARTED_FLAG

SET_MAC_NEED_REBOOT = module_product.SET_MAC_NEED_REBOOT
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
