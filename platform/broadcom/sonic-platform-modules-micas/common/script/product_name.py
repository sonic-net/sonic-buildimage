#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import os
import syslog
from platform_config import PRODUCT_NAME_CONF
from wbutil.baseutil import get_machine_info
from wbutil.baseutil import get_onie_machine
from wbutil.baseutil import get_sub_version

BOARD_ID_PATH = "/sys/module/platform_common/parameters/dfd_my_type"
PRODUCT_DEBUG_FILE = "/etc/.product_debug_flag"
PRODUCT_RESULT_FILE = "/tmp/.productname"

PRODUCTERROR = 1
PRODUCTDEBUG = 2

debuglevel = 0


def product_info(s):
    syslog.openlog("PRODUCT", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def product_error(s):
    syslog.openlog("PRODUCT", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def product_debug(s):
    if PRODUCTDEBUG & debuglevel:
        syslog.openlog("PRODUCT", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def product_debug_error(s):
    if PRODUCTERROR & debuglevel:
        syslog.openlog("PRODUCT", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def debug_init():
    global debuglevel
    try:
        with open(PRODUCT_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def get_td4_mac_id(loc):
    if not os.path.exists(loc):
        msg = "mac id path: %s, not exists" % loc
        product_error(msg)
        return False, msg
    with open(loc) as fd:
        id_str = fd.read().strip()
    mac_id = "0x%x" % int(id_str, 10)
    return True, mac_id


def get_func_value(funcname, params=None):
    try:
        allowed_func_map = {
            "get_td4_mac_id": get_td4_mac_id,
            "get_product_name_default": get_product_name_default,
            "get_board_id_default": get_board_id_default,
        }
        func = allowed_func_map.get(funcname)
        if func is None:
            msg = "unsupported funcname: %s" % funcname
            product_error(msg)
            return False, msg

        if params is not None:
            result = func(params)
        else:
            result = func()

        if isinstance(result, tuple) and len(result) == 2:
            status, ret = result
        else:
            status, ret = True, result
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
    return "0x%x" % int(id_str, 10)


def deal_method(method):
    try:
        gettype = method.get("gettype")
        if gettype == "config":
            result = method.get("value")
            product_debug("get info use config value: %s" % result)
            return True, result

        if gettype == "func":
            funcname = method.get("funcname")
            params = method.get("params")
            status, ret = get_func_value(funcname, params)
            if status is False:
                product_error("get info func: %s, params: %s failed, ret: %s" % (funcname, params, ret))
                return status, ret
            decode_val = method.get("decode")
            result = decode_val.get(ret) if decode_val is not None else ret
            product_debug("get info func: %s, params: %s, ret: %s, result: %s" % (funcname, params, ret, result))
            return True, result

        msg = "unsupport get info method: %s " % gettype
        product_error(msg)
        return False, msg
    except Exception as e:
        return False, str(e)


def get_product_name():
    get_product_name_method = PRODUCT_NAME_CONF.get("get_product_name_method")
    if get_product_name_method is None:
        product_name = get_product_name_default()
        product_debug("get product name use default method, product name: %s" % product_name)
        return product_name

    status, ret = deal_method(get_product_name_method)
    if status is False:
        product_error("get product name faield, msg: %s" % ret)
        return None
    product_debug("get product name success, product name: %s" % ret)
    return ret


def get_board_id():
    get_board_id_method = PRODUCT_NAME_CONF.get("get_board_id_method")
    if get_board_id_method is None:
        board_id = get_board_id_default()
        product_debug("get board id use default method, board id: %s" % board_id)
        return board_id

    status, ret = deal_method(get_board_id_method)
    if status is False:
        product_error("get board id faield, msg: %s" % ret)
        return None
    product_debug("get board id success, board id: %s" % ret)
    return ret


def save_product_name():
    product_name = get_product_name()
    board_id = get_board_id()
    sub_ver = get_sub_version()
    if sub_ver != "NA":
        name = "%s_%s_%s\n" % (product_name, board_id, sub_ver)
    else:
        name = "%s_%s\n" % (product_name, board_id)
    product_info("save product name: %s" % name)
    with open(PRODUCT_RESULT_FILE, "w") as fd:
        fd.write(name)


if __name__ == '__main__':
    debug_init()
    product_debug("enter main")
    save_product_name()
