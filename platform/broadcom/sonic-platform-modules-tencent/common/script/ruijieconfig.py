#!/usr/bin/python3
#-------------------------------------------------------------------------------
# Name:        ruijieconfig.py
#
# Author:      sonic_rd@ruijie.com.cn
#
# Created:     02/07/2018
# Copyright:   2001-2022 Ruijie Network. All rights reserved.
#-------------------------------------------------------------------------------
import sys
import os
from rjutil.baseutil import get_machine_info
from rjutil.baseutil import get_platform_info
from rjutil.baseutil import get_board_id


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
common_productfile = "ruijiecommon"
platform_configfile = (platform + "_" + board_id + "_config").replace("-", "_") # platfrom + board_id
configfile_pre = "/usr/local/bin/"
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


############################################################################################
if os.path.exists(configfile_pre + platform_configfile + ".py"):
    module_product = __import__(platform_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + grtd_productfile + ".py"):
    module_product = __import__(grtd_productfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + common_productfile + ".py"):
    module_product = __import__(common_productfile, globals(), locals(), [], 0)
else:
    print("config file not exist")
    exit(-1)
############################################################################################

DEVICE  = module_product.DEVICE

RUIJIE_GLOBALCONFIG ={
    "DRIVERLISTS":module_product.DRIVERLISTS,
    "OPTOE": module_product.OPTOE,
    "DEVS": DEVICE,
    "BLACKLIST_DRIVERS": module_product.BLACKLIST_DRIVERS
}
GLOBALCONFIG = RUIJIE_GLOBALCONFIG
GLOBALINITPARAM = module_product.INIT_PARAM
GLOBALINITCOMMAND = module_product.INIT_COMMAND
GLOBALINITPARAM_PRE = module_product.INIT_PARAM_PRE
GLOBALINITCOMMAND_PRE = module_product.INIT_COMMAND_PRE

STARTMODULE = module_product.STARTMODULE

DEV_MONITOR_PARAM  = module_product.DEV_MONITOR_PARAM
PMON_SYSLOG_STATUS = module_product.PMON_SYSLOG_STATUS

MAC_AVS_PARAM      = module_product.MAC_AVS_PARAM
MAC_DEFAULT_PARAM  = module_product.MAC_DEFAULT_PARAM

MANUINFO_CONF = module_product.MANUINFO_CONF
AVS_VOUT_MODE_PARAM = module_product.AVS_VOUT_MODE_PARAM
