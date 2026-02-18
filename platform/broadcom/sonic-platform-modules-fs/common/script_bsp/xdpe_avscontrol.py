#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import click
import os
import subprocess
import time
import syslog
import traceback
import logging
from platform_util import *

DEBUG_FILE = "/etc/.avscontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "xdpe_avscontrol_debug.log"
logger = setup_logger(LOG_FILE)

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))


def avscontrol_debug(s):
    logger.debug(s)

def avscontrol_error(s):
    logger.error(s)

def avserror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("AVSCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)
    logger.error(s)

def avsinfo(s):
    syslog.openlog("AVSCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)
    logger.info(s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def byteTostr(val):
    strtmp = ''
    for i in range(len(val)):
        strtmp += chr(val[i])
    return strtmp

def readsysfs(location):
    try:
        locations = glob.glob(location)
        with open(locations[0], 'rb') as fd1:
            retval = fd1.read()
        retval = byteTostr(retval)
        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
    except Exception as e:
        return False, (str(e) + " location[%s]" % location)
    return True, retval


def writesysfs(location, value):
    try:
        if not os.path.isfile(location):
            print(location, 'not found !')
            return False, ("location[%s] not found !" % location)
        with open(location, 'w') as fd1:
            fd1.write(value)
    except Exception as e:
        return False, (str(e) + " location[%s]" % location)
    return True, ("set location[%s] %s success !" % (location, value))

def set_value(config, value):
    way = config.get("gettype")
    if way == 'sysfs':
        loc = config.get("loc")
        return writesysfs(loc, "0x%x" % value)
    elif way == "i2c":
        bus = config.get("bus")
        addr = config.get("loc")
        offset = config.get("offset")
        return rji2cset(bus, addr, offset, value)
    return False, None


def set_avs_value_i2c(dcdc_value):
    u'''Write the voltage regulator chip to adjust the voltage'''
    avs_bus = MAC_DEFAULT_PARAM["bus"]
    avs_addr = MAC_DEFAULT_PARAM["devno"]
    avs_loop_addr = MAC_DEFAULT_PARAM["loopaddr"]
    avs_loop_val = MAC_DEFAULT_PARAM["loop"]
    vout_mode_addr = MAC_DEFAULT_PARAM["vout_mode_addr"]
    vout_cmd_addr = MAC_DEFAULT_PARAM["vout_cmd_addr"]
    org_loop_value = None
    try:
        # Gets the original loop value
        status, val = rji2cget(avs_bus, avs_addr, avs_loop_addr)
        if status is not True:
            raise Exception("get original loop value failed.")
        org_loop_value = int(val, 16)

        # Switch loop value
        status, val = rji2cset(avs_bus, avs_addr, avs_loop_addr, avs_loop_val)
        if status is not True:
            raise Exception("set loop value failed.")

        # get vout_mode
        status, val = rji2cget(avs_bus, avs_addr, vout_mode_addr)
        if status is not True:
            raise Exception("get vout mode failed.")
        vout_mode_value = int(val, 16)
        if vout_mode_value not in AVS_VOUT_MODE_PARAM.keys():
            raise Exception("invalid vout mode.")

        # compute vout_cmd
        vout_cmd_val = int(dcdc_value * AVS_VOUT_MODE_PARAM[vout_mode_value])
        avscontrol_debug("org_loop:0x%x, dcdc_value:%s, vout_mode:0x%x, vout_cmd_val:0x%x." %
                         (org_loop_value, dcdc_value, vout_mode_value, vout_cmd_val))
        # set vout_cmd
        rji2csetWord(avs_bus, avs_addr, vout_cmd_addr, vout_cmd_val)
        status, val = rji2cgetWord(avs_bus, avs_addr, vout_cmd_addr)
        if status is not True or strtoint(val) != vout_cmd_val:
            raise Exception("set vout command data failed. status:%s, write value:0x%x, read value:0x%x" %
                            (status, vout_cmd_val, strtoint(val)))
        avscontrol_debug("set vout command data success.")

    except Exception as e:
        avscontrol_error(str(e))
        status = False

    if org_loop_value is not None:
        rji2cset(avs_bus, avs_addr, avs_loop_addr, org_loop_value)
    return status


def set_avs_value_sysfs(conf, dcdc_value):
    loc = conf.get("loc")
    formula = conf.get("formula", None)
    avscontrol_debug("set_avs_value_sysfs, loc: %s, origin dcdc value: %s, formula: %s" %
        (loc, dcdc_value, formula))
    if formula is not None:
        dcdc_value = eval(formula % (dcdc_value))
    wr_val = str(dcdc_value)
    avscontrol_debug("set_avs_value_sysfs, write val: %s" % wr_val)
    ret, msg = writesysfs(loc, wr_val)
    if ret is False:
        avscontrol_error("set_avs_value_sysfs failed, msg: %s" % msg)
    return ret


def set_avs_value(dcdc_value):
    set_avs_way = MAC_DEFAULT_PARAM.get("set_avs", {}).get("gettype")
    if set_avs_way == "sysfs": # The sysfs mode is used for voltage regulation
        ret = set_avs_value_sysfs(MAC_DEFAULT_PARAM["set_avs"], dcdc_value)
    else: # The I2C mode is adopted by default
        ret = set_avs_value_i2c(dcdc_value)
    return ret

def get_dcdc_value(rov_value):
    if rov_value not in MAC_AVS_PARAM.keys():
        if MAC_DEFAULT_PARAM["type"] == 0:  # Out of range. Out of tune
            avsinfo("VID:0x%x out of range, voltage regulate stop." % rov_value)
            return False, None
        dcdc_value = MAC_AVS_PARAM[MAC_DEFAULT_PARAM["default"]]  # Use default when not in range
        avsinfo("VID:0x%x out of range, use default VID:0x%x." % (rov_value, dcdc_value))
    else:
        dcdc_value = MAC_AVS_PARAM[rov_value]
    return True, dcdc_value


def get_rov_value_cpld():
    cpld_avs_config = MAC_DEFAULT_PARAM["cpld_avs"]
    return get_value(cpld_avs_config)


def get_rov_value_sdk():
    name = MAC_DEFAULT_PARAM["sdkreg"]
    ret, status = getSdkReg(name)
    if ret == False:
        return False, None
    status = strtoint(status)
    # shift operation
    if MAC_DEFAULT_PARAM["sdktype"] != 0:
        status = (
            status >> MAC_DEFAULT_PARAM["macregloc"]) & MAC_DEFAULT_PARAM["mask"]
    macavs = status
    return True, macavs


def doAvsCtrol():
    try:
        rov_source = MAC_DEFAULT_PARAM["rov_source"]
        if rov_source == 0:
            ret, rov_value = get_rov_value_cpld()  # get rov from cpld reg
        else:
            ret, rov_value = get_rov_value_sdk()  # get rov from sdk reg
        if ret is False:
            avscontrol_error("get_rov_value error: %s", rov_value)
            return False
        avscontrol_debug("rov_value:0x%x." % rov_value)
        ret, dcdc_value = get_dcdc_value(rov_value)
        if ret is False:
            return False
        ret = set_avs_value(dcdc_value)
        return ret
    except Exception as e:
        avscontrol_error(str(e))
    return False


def run():
    index = 0
    # wait 30s for device steady
    time.sleep(30)
    while True:
        debug_init()
        ret = doAvsCtrol()
        if ret is True:
            avsinfo("%%AVSCONTROL success")
            time.sleep(5)
            exit(0)
        index += 1
        if index >= 10:
            avserror("%%DEV_MONITOR-3-AVS: MAC Voltage adjust failed.")
            exit(-1)
        time.sleep(1)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass


@main.command()
def start():
    '''start AVS control'''
    avsinfo("%%AVSCONTROL start")
    run()


if __name__ == '__main__':
    debug_init()
    main()
