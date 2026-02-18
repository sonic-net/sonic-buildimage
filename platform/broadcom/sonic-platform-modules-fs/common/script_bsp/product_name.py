#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import os
import syslog
import logging
from platform_config import PRODUCT_NAME_CONF
from wbutil.baseutil import get_machine_info
from wbutil.baseutil import get_onie_machine
from wbutil.baseutil import get_sub_version
from platform_util import setup_logger, BSP_COMMON_LOG_DIR

BOARD_ID_PATH = "/sys/module/platform_common/parameters/dfd_my_type"
PRODUCT_RESULT_FILE = "/tmp/.productname"
DEBUG_FILE = "/etc/.logger.debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "product_name_debug.log"
logger = setup_logger(LOG_FILE)

def product_info(s):
    logger.info(s)

def product_error(s):
    logger.error(s)

def product_debug(s):
    logger.debug(s)

def product_debug_error(s):
    logger.error(s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

################################## Custom interface storage area for each product begin ###################################

'''tcs84 reads the MAC chip ID to distinguish products'''
def get_td4_mac_id(loc):
    if not os.path.exists(loc):
        msg = "mac id path: %s, not exists" % loc
        product_error(msg)
        return False, msg
    with open(loc) as fd:
        id_str = fd.read().strip()
    id = "0x%x" % (int(id_str, 10))
    return True, id

################################## Custom interface storage area for each product end #####################################

def get_func_value(funcname, params = None):
    try:
        if params is not None:
            status, ret = eval(funcname)(params)
        else:
            status, ret = eval(funcname)
        return status, ret
    except Exception as e:
        product_error(str(e))
    return False, str(e)


def get_product_name_default():
    onie_machine = get_onie_machine(get_machine_info())
    if onie_machine is not None:
        ret = onie_machine.strip().split("_", 1)
        if len(ret) != 2:
            product_error("unknow onie machine: %s" % onie_machine)
            return None
        product_name = ret[1]
        product_debug("get product name: %s success" % product_name)
        return product_name
    product_error("onie machine is None, can't get product name")
    return None


def get_board_id_default():
    if not os.path.exists(BOARD_ID_PATH):
        product_error("board id path: %s, not exists" % BOARD_ID_PATH)
        return None
    with open(BOARD_ID_PATH) as fd:
        id_str = fd.read().strip()
    return "0x%x" % (int(id_str, 10))


def deal_method(method):
    try:
        gettype = method.get("gettype")
        if gettype == "config": # Obtain the value from the configuration file
            result = method.get("value")
            product_debug("get info use config value: %s" % result)
            return True, result

        if gettype == "func": # Obtained by a custom function
            funcname = method.get("funcname")
            params = method.get("params")
            status, ret = get_func_value(funcname, params)
            if status is False:
                product_error("get info func: %s, params: %s failed, ret: %s" %
                    (funcname, params, ret))
                return status, ret
            decode_val = method.get("decode")
            if decode_val is not None:
                result = decode_val.get(ret)
            else:
                result = ret
            product_debug("get info func: %s, params: %s, ret: %s, result: %s" %
                (funcname, params, ret, result))
            return True, result
        msg = "unsupport get info method: %s " % gettype
        product_error(msg)
        return False, msg
    except Exception as e:
        return False, str(e)


def get_product_name():
    # Get product name
    get_product_name_method = PRODUCT_NAME_CONF.get("get_product_name_method")
    if get_product_name_method is None: # Use the default method to get the product name
        product_name = get_product_name_default()
        product_debug("get product name use default method, product name: %s" % (product_name))
        return product_name

    status, ret = deal_method(get_product_name_method)
    if status is False:
        product_error("get product name faield, msg: %s" % ret)
        return None
    product_debug("get product name success, product name: %s" % (ret))
    return ret


def get_board_id():
    # Get the card ID
    get_board_id_method = PRODUCT_NAME_CONF.get("get_board_id_method")
    if get_board_id_method is None: # Use the default method to get the card ID
        board_id = get_board_id_default()
        product_debug("get board id use default method, board id: %s" % (board_id))
        return board_id

    status, ret = deal_method(get_board_id_method)
    if status is False:
        product_error("get board id faield, msg: %s" % ret)
        return None
    product_debug("get board id success, board id: %s" % (ret))
    return ret


def save_product_name():
    # Get product name
    product_name = get_product_name()
    board_id = get_board_id()
    status, sub_ver = get_sub_version()
    if status is False:
        product_debug("get sub_ver faield, msg: %s" % sub_ver)
        name = "%s_%s\n" % (product_name, board_id)
    else:
        name = "%s_%s_%s\n" % (product_name, board_id, sub_ver)

    product_info("save product name: %s" % name)
    with open(PRODUCT_RESULT_FILE, "w") as fd:
        fd.write(name)


if __name__ == '__main__':
    debug_init()
    product_info("enter main")
    save_product_name()
