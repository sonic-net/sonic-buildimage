#!/usr/bin/env python_nos
import sys
import os
import time
import syslog
import glob
import click
import logging
import traceback
from platform_config import MAC_DEFAULT_PARAM
from platform_util import AliasedGroup, CONTEXT_SETTINGS, getSdkReg, write_sysfs, get_value, get_format_value, check_value, setup_logger, BSP_COMMON_LOG_DIR, waitForDocker, unload_process_byPid, allow_syslog
from public.platform_common_config import avs_begin_sleep_time, SDKCHECK_PARAMS

DEBUG_FILE = "/etc/.avscontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "avscontrol_debug.log"
logger = setup_logger(LOG_FILE)
LOG_LAST_TIME = {}

def avscontrol_debug(s):
    logger.debug(s)

def avscontrol_error(s):
    logger.error(s)

def avserror(s):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        # s = s.decode('utf-8').encode('gb2312')
        syslog.openlog("AVSCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)
    logger.error(s)


def avsinfo(s):
    logger.info(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def set_avs_value_sysfs(conf, dcdc_value):
    msg = ""
    formula = conf.get("formula", None)
    loc = conf.get("loc")
    locations = glob.glob(loc)
    if len(locations) == 0:
        msg = "avs sysfs loc: %s not found" % loc
        avscontrol_error(msg)
        return False, msg
    sysfs_loc = locations[0]
    avscontrol_debug("set_avs_value_sysfs, loc: %s, origin dcdc value: %s, formula: %s" %
                     (sysfs_loc, dcdc_value, formula))
    if formula is not None:
        dcdc_value = get_format_value(formula % (dcdc_value))
    wr_val = str(dcdc_value)
    avscontrol_debug("set_avs_value_sysfs, write val: %s" % wr_val)
    ret, log = write_sysfs(sysfs_loc, wr_val)
    if ret is False:
        msg = "set_avs_value_sysfs failed, msg: %s" % log
        avscontrol_error(msg)
    return ret, msg

def set_avs_value(avs_conf, dcdc_value):
    if isinstance(avs_conf.get("set_avs"), list):
        for conf in avs_conf.get("set_avs"):
            ret, msg = set_avs_value_once(conf, dcdc_value)
            if ret is False:
                return False, msg
        return True, "set_avs_value success"
    else:
        return set_avs_value_once(avs_conf.get("set_avs"), dcdc_value)

def set_avs_value_once(set_avs, dcdc_value):
    set_avs_way = set_avs.get("gettype")
    if set_avs_way != "sysfs":
        msg = "unsupport set avs value type: %s" % set_avs_way
        avscontrol_error(msg)
        return False, msg
    ret, msg = set_avs_value_sysfs(set_avs, dcdc_value)
    return ret, msg

def get_dcdc_value(avs_conf, rov_value):
    msg = ""
    mac_avs_param = avs_conf.get("mac_avs_param", {})
    avs_version_selector = avs_conf.get("avs_version_selector")
    # If avs_version_selector is not None, get version value from avs_version_selector
    if avs_version_selector is not None:
        mac_avs_param_match_value = avs_conf.get("mac_avs_param_match_value", {})
        ret, version_val = get_value(avs_version_selector)
        if ret is True:
            # If version_val is in mac_avs_param_match_value, get mac_avs_param from mac_avs_param_match_value
            if version_val in mac_avs_param_match_value:
                mac_avs_param = mac_avs_param_match_value[version_val]
                avsinfo("avs_version_selector get value: 0x%x" % (version_val))
            else:
                avsinfo("can't find same version in mac_avs_param_match_value, use default mac_avs_param, version_val: 0x%x" % (version_val))
        else:
            avscontrol_error("get avs_version_selector value failed, use default mac_avs_param")

    if len(mac_avs_param) == 0:
        msg = "mac_avs_param is None or empty, can't get dcdc value"
        avscontrol_error(msg)
        return False, msg

    if rov_value not in mac_avs_param.keys():
        if avs_conf["type"] == 0:
            msg = "VID:0x%x out of range, voltage regulate stop" % rov_value
            avsinfo(msg)
            return False, msg
        dcdc_value = mac_avs_param[avs_conf["default"]]
        avsinfo("VID:0x%x out of range, use default VID:%s" % (rov_value, dcdc_value))
    else:
        dcdc_value = mac_avs_param[rov_value]
    return True, dcdc_value


def get_rov_value_cpld(avs_conf):
    cpld_avs_config = avs_conf["cpld_avs"]
    return get_value(cpld_avs_config)


def get_rov_value_sdk(avs_conf):
    name = avs_conf["sdkreg"]
    ret, status = getSdkReg(name)
    if ret is False:
        return False, status
    status = int(status, 16)
    # shift operation
    if avs_conf["sdktype"] != 0:
        status = (status >> avs_conf["macregloc"]) & avs_conf["mask"]
    macavs = status
    return True, macavs


def doAvsCtrol_single(avs_conf):
    try:
        avs_name = avs_conf.get("name")
        rov_source = avs_conf["rov_source"]
        if rov_source == 0:
            ret, rov_value = get_rov_value_cpld(avs_conf)  # get rov from cpld reg
        else:
            ret, rov_value = get_rov_value_sdk(avs_conf)  # get rov from sdk reg
        if ret is False:
            msg = "%s get rov_value failed, msg: %s" % (avs_name, rov_value)
            avscontrol_error(msg)
            return False, msg
        avscontrol_debug("%s rov_value:  0x%x" % (avs_name, rov_value))
        ret, dcdc_value = get_dcdc_value(avs_conf, rov_value)
        if ret is False:
            msg = "%s get output voltage value failed, msg: %s" % (avs_name, dcdc_value)
            avscontrol_error(msg)
            return False, msg
        ret, msg = set_avs_value(avs_conf, dcdc_value)
        return ret, msg
    except Exception as e:
        msg = "%s avscontrol raise exception, msg: %s" % (avs_name, str(e))
        avscontrol_error(msg)
        return False, msg


def check_avs_ready(avs_ready_check_list):
    '''
    avs_ready_check_list is a secondary list.
    avs_ready_conf_list is a primary list.
    If any condition in the primary list within the secondary list is true, the final result is true.
    If all conditions in a primary list are true, the final result for that primary list is true.
    '''
    avs_ready = False
    for avs_ready_conf_list in avs_ready_check_list:
        for avs_ready_conf in avs_ready_conf_list:
            ret, msg = check_value(avs_ready_conf)
            if ret is False:
                avs_ready = False
                msg = "avs_ready_conf: %s check_value failed, msg: %s" % (avs_ready_conf, msg)
                avscontrol_debug(msg)
                break
            avs_ready = True
            msg = "avs_ready_conf: %s check_value success, msg: %s" % (avs_ready_conf, msg)
            avscontrol_debug(msg)
        if avs_ready is True:
            avscontrol_debug("check_avs_ready ok, avs_ready is True")
            return True
    return False


def check_avs_status(avs_conf):
    avs_name = avs_conf.get("name")
    check_avs_ready_conf = avs_conf.get("check_avs_ready")
    if check_avs_ready_conf is None: # don't need to check avs ready
        msg = "%s check_avs_ready_conf is None, don't need to check avs ready" % avs_name
        avscontrol_debug(msg)
        return True, msg

    retrytime = check_avs_ready_conf.get("retry", 1)
    sleep_time = check_avs_ready_conf.get("sleep_time")
    avs_ready_check_list = check_avs_ready_conf.get("avs_ready_check_list", [])

    loop = 0
    while True:
        loop += 1
        debug_init()
        ret = check_avs_ready(avs_ready_check_list)
        if ret is True:
            msg = "%s check_avs_ready ok, retrytime: %d, loop: %d" % (avs_name, retrytime, loop)
            avscontrol_debug(msg)
            return True, msg
        avscontrol_debug("%s check_avs_ready not ok, retrytime: %d, loop: %d" % (avs_name, retrytime, loop))
        if retrytime > 0 and loop >= retrytime:
            msg = "%s check_avs_ready timeout, retrytime: %d, loop: %d" % (avs_name, retrytime, loop)
            avscontrol_debug(msg)
            return False, msg
        if sleep_time is not None and sleep_time > 0:
            time.sleep(sleep_time)
    return False, "%s check_avs_ready error" % avs_name


def doAvsCtrol(avs_conf):
    debug_init()
    ret, log = check_avs_status(avs_conf)
    if ret is False:
        return False, log

    avs_ctrl_pre_sleep = avs_conf.get("avs_ctrl_pre_sleep")
    if avs_ctrl_pre_sleep is not None and avs_ctrl_pre_sleep > 0:
        time.sleep(avs_ctrl_pre_sleep)

    retry_time = avs_conf.get("retry", 10)
    for i in range(retry_time):
        debug_init()
        ret, log = doAvsCtrol_single(avs_conf)
        if ret is True:
            return True, log
        time.sleep(1)
    return False, log


def run():
    if avs_begin_sleep_time > 0:
        time.sleep(avs_begin_sleep_time)
    errcnt = 0
    msg = ""

    while True:
        try:
            if waitForDocker(SDKCHECK_PARAMS, timeout=0) == True:
                time.sleep(10)
                break
            avscontrol_debug("DEV_MONITOR-AVS: waitting sdk start-up.")
            time.sleep(5)
        except Exception as e:
            traceback.print_exc()
            print(e)

    for item in MAC_DEFAULT_PARAM:
        status, log = doAvsCtrol(item)
        if status is False:
            errcnt += 1
            msg += log

    if errcnt == 0:
        avsinfo("%%AVSCONTROL success")
        sys.exit(0)
    avserror("%%DEV_MONITOR-AVS: MAC Voltage adjust failed.")
    avserror("%%DEV_MONITOR-AVS: errmsg: %s" % msg)
    sys.exit(1)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()

@main.command()
def start():
    '''start AVS control'''
    avsinfo("%%AVSCONTROL start")
    run()

@main.command()
def stop():
    script_name = os.path.basename(sys.argv[0])
    unload_process_byPid(script_name)

if __name__ == '__main__':
    main()
