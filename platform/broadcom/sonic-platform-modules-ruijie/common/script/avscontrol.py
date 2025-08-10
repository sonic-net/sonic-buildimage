#!/usr/bin/env python3
import sys
import os
import time
import syslog
import glob
import click
from platform_config import MAC_DEFAULT_PARAM
from platform_util import getSdkReg, write_sysfs, get_value, get_format_value


AVSCTROL_DEBUG_FILE = "/etc/.avscontrol_debug_flag"

AVSCTROLERROR = 1
AVSCTROLDEBUG = 2

debuglevel = 0

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
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
    set_avs_way = avs_conf.get("set_avs", {}).get("gettype")
    if set_avs_way != "sysfs":
        msg = "unsupport set avs value type: %s" % set_avs_way
        avscontrol_error(msg)
        return False, msg
    ret, msg = set_avs_value_sysfs(avs_conf["set_avs"], dcdc_value)
    return ret, msg


def get_dcdc_value(avs_conf, rov_value):
    msg = ""
    mac_avs_param = avs_conf.get("mac_avs_param", {})
    if rov_value not in mac_avs_param.keys():
        if avs_conf["type"] == 0:
            msg = "VID:0x%x out of range, voltage regulate stop" % rov_value
            avsinfo(msg)
            return False, msg
        dcdc_value = mac_avs_param[avs_conf["default"]]
        avsinfo("VID:0x%x out of range, use default VID:0x%x" % (rov_value, dcdc_value))
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


def doAvsCtrol(avs_conf):
    retry_time = avs_conf.get("retry", 10)
    for i in range(retry_time):
        debug_init()
        ret, log = doAvsCtrol_single(avs_conf)
        if ret is True:
            return True, log
        time.sleep(1)
    return False, log


def run():
    # wait 30s for device steady
    time.sleep(30)
    errcnt = 0
    msg = ""
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


@main.command()
def start():
    '''start AVS control'''
    avsinfo("%%AVSCONTROL start")
    run()


if __name__ == '__main__':
    main()
