#!/usr/bin/env python3
import click
import os
import time
import syslog
from ruijieconfig import MAC_DEFAULT_PARAM, MAC_AVS_PARAM, AVS_VOUT_MODE_PARAM
from ruijieutil import getSdkReg
from platform_util import rji2cget, rji2cset, rji2csetWord, rji2cgetWord, write_sysfs, get_value


AVSCTROL_DEBUG_FILE = "/etc/.avscontrol_debug_flag"

AVSCTROLERROR = 1
AVSCTROLDEBUG = 2

debuglevel = 0

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
        return None


def avscontrol_debug(s):
    if AVSCTROLDEBUG & debuglevel:
        syslog.openlog("AVSCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def avscontrol_error(s):
    if AVSCTROLERROR & debuglevel:
        syslog.openlog("AVSCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def avserror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("AVSCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def avsinfo(s):
    syslog.openlog("AVSCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def debug_init():
    global debuglevel
    if os.path.exists(AVSCTROL_DEBUG_FILE):
        debuglevel = debuglevel | AVSCTROLDEBUG | AVSCTROLERROR
    else:
        debuglevel = debuglevel & ~(AVSCTROLDEBUG | AVSCTROLERROR)


def set_avs_value_i2c(dcdc_value):
    avs_bus = MAC_DEFAULT_PARAM["bus"]
    avs_addr = MAC_DEFAULT_PARAM["devno"]
    avs_loop_addr = MAC_DEFAULT_PARAM["loopaddr"]
    avs_loop_val = MAC_DEFAULT_PARAM["loop"]
    vout_mode_addr = MAC_DEFAULT_PARAM["vout_mode_addr"]
    vout_cmd_addr = MAC_DEFAULT_PARAM["vout_cmd_addr"]
    org_loop_value = None
    try:
        status, val = rji2cget(avs_bus, avs_addr, avs_loop_addr)
        if status is not True:
            raise Exception("get original loop value failed.")
        org_loop_value = int(val, 16)

        status, val = rji2cset(avs_bus, avs_addr, avs_loop_addr, avs_loop_val)
        if status is not True:
            raise Exception("set loop value failed.")

        status, val = rji2cget(avs_bus, avs_addr, vout_mode_addr)
        if status is not True:
            raise Exception("get vout mode failed.")
        vout_mode_value = int(val, 16)
        if vout_mode_value not in AVS_VOUT_MODE_PARAM.keys():
            raise Exception("invalid vout mode.")

        vout_cmd_val = int(dcdc_value * AVS_VOUT_MODE_PARAM[vout_mode_value])
        avscontrol_debug("org_loop:0x%x, dcdc_value:%s, vout_mode:0x%x, vout_cmd_val:0x%x." %
                         (org_loop_value, dcdc_value, vout_mode_value, vout_cmd_val))
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
    ret, msg = write_sysfs(loc, wr_val)
    if ret is False:
        avscontrol_error("set_avs_value_sysfs failed, msg: %s" % msg)
    return ret


def set_avs_value(dcdc_value):
    set_avs_way = MAC_DEFAULT_PARAM.get("set_avs", {}).get("gettype")
    if set_avs_way == "sysfs":
        ret = set_avs_value_sysfs(MAC_DEFAULT_PARAM["set_avs"], dcdc_value)
    else:
        ret = set_avs_value_i2c(dcdc_value)
    return ret

def get_dcdc_value(rov_value):
    if rov_value not in MAC_AVS_PARAM.keys():
        if MAC_DEFAULT_PARAM["type"] == 0:
            avsinfo("VID:0x%x out of range, voltage regulate stop." % rov_value)
            return False, None
        dcdc_value = MAC_AVS_PARAM[MAC_DEFAULT_PARAM["default"]]
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
            avserror("%%DEV_MONITOR-AVS: MAC Voltage adjust failed.")
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
    main()
