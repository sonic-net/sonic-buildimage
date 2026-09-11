#!/usr/bin/env python_nos
import sys
import os
from wbutil.baseutil import get_platform_info
from wbutil.baseutil import get_board_id
from wbutil.baseutil import get_sub_version
from platform_config import BSP_COMMON_LOG_DIR

platform = "NA"
board_id = "NA"
sub_ver = "NA"

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

platform_subver_configfile = (platform + "_" + board_id + "_" + sub_ver + "_tech_config")  # platfrom + board_id + sub_ver
platform_boardid_configfile = (platform + "_" + board_id + "_tech_config")  # platfrom + board_id
platform_configfile = (platform + "_tech_config") # platform
boardid_subver_configfile = (board_id + "_" + sub_ver + "_tech_config")  # board_id + sub_ver
boardid_configfile = (board_id + "_tech_config")  # board_id

configfile_pre = "/usr/local/bin/"
sys.path.append(configfile_pre)

############################################################################################
if os.path.exists(configfile_pre + platform_subver_configfile + ".py"):
    module_product = __import__(platform_subver_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_boardid_configfile + ".py"):
    module_product = __import__(platform_boardid_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_configfile + ".py"):
    module_product = __import__(platform_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + boardid_subver_configfile + ".py"):
    module_product = __import__(boardid_subver_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + boardid_configfile + ".py"):
    module_product = __import__(boardid_configfile, globals(), locals(), [], 0)
else:
    print("platform tech config file not exist, please check")
    sys.exit(-1)
############################################################################################


def get_var(name, default):
    global module_product
    var = getattr(module_product, name, default)
    return var


COLLECT_MODULES = get_var("COLLECT_MODULES", [])
COLLECT_TYPE_MAP = get_var("COLLECT_TYPE_MAP", {})
LOG_LINK_TARGET = BSP_COMMON_LOG_DIR
RUNNINGDATA_TYPE = get_var("RUNNINGDATA_TYPE", "")
COMPONENT_TYPE = get_var("COMPONENT_TYPE", "")
I2C_COLLECT = get_var("I2C_COLLECT", {})
DISK_COLLECT = get_var("DISK_COLLECT", {})
NIC_COLLECT = get_var("NIC_COLLECT ", {})

