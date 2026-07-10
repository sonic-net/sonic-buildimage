#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import time
import syslog
import click
from platform_util import write_sysfs, wbi2cset, io_wr, wbi2csetWord
from platform_config import REBOOT_CTRL_PARAM


REBOOTCTLDEBUG = 0

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


def rebootctrlwarning(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("REBOOTCTRL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def rebootctrlcritical(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("REBOOTCTRL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)


def rebootctrlerror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("REBOOTCTRL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def rebootctrldebug(s):
    # s = s.decode('utf-8').encode('gb2312')
    if REBOOTCTLDEBUG == 1:
        syslog.openlog("REBOOTCTRL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


class RebootCtrl():
    def __init__(self):
        self.config = REBOOT_CTRL_PARAM.copy()

    def set_value(self, config, val):
        way = config.get("gettype")
        if way == 'sysfs':
            loc = config.get("loc")
            value = config.get(val)
            rebootctrldebug("sysfs type.loc:0x%x, value:0x%x" % (loc, value))
            return write_sysfs(loc, "0x%02x" % value)
        if way == "i2c":
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get(val)
            rebootctrldebug("i2c type.bus:0x%x, addr:0x%x, offset:0x%x, value:0x%x" % (bus, addr, offset, value))
            return wbi2cset(bus, addr, offset, value)
        if way == "io":
            io_addr = config.get('io_addr')
            value = config.get(val)
            rebootctrldebug("io type.io_addr:0x%x, value:0x%x" % (io_addr, value))
            ret = io_wr(io_addr, value)
            if ret is not True:
                return False, ("write 0x%x failed" % io_addr)
            return True, ("write 0x%x success" % io_addr)
        if way == 'i2cword':
            bus = config.get("bus")
            addr = config.get("loc")
            offset = config.get("offset")
            value = config.get(val)
            rebootctrldebug("i2cword type.bus:0x%x, addr:0x%x, offset:0x%x, value:0x%x" % (bus, addr, offset, value))
            return wbi2csetWord(bus, addr, offset, value)
        return False, "unsupport way: %s" % way

    def reset_operate(self, config):
        ret, log = self.set_value(config, "rst_val")
        rst_delay = config.get("rst_delay", 0)
        time.sleep(rst_delay)
        return ret, log

    def unlock_reset_operate(self, config):
        ret, log = self.set_value(config, "unlock_rst_val")
        unlock_rst_delay = config.get("unlock_rst_delay", 0)
        time.sleep(unlock_rst_delay)
        return ret, log

    def do_rebootctrl(self, option):
        if self.config is None:
            rebootctrlerror("Reset failed, REBOOT_CTRL_PARAM cfg get failed.")
            return
        try:
            name_conf = self.config.get(option, None)
            if name_conf is None:
                print("Reset %s not support" % option)
                return
            try:
                click.confirm("Are you sure you want to reset " + option + "?",
                              default=False, abort=True, show_default=True)
            except Exception as e:
                print("Aborted, msg: %s" % str(e))
                return
            print("Reset %s start" % option)
            ret, log = self.reset_operate(name_conf)
            if ret is False:
                rebootctrlerror(log)
                print("Reset %s failed" % option)
                return
            if "unlock_rst_val" in name_conf:
                ret, log = self.unlock_reset_operate(name_conf)
                if ret is False:
                    rebootctrlerror(log)
                    print("%s unlock reset failed" % option)
                    return
            print("Reset %s success" % option)
        except Exception:
            rebootctrlerror("do_rebootctrl Exception error")
        return


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''reboot_ctrl reset [option]'''


@main.command()
@click.argument('option', required=True)
def reset(option):
    '''reset device'''
    rebootctrldebug("reboot ctrl option %s" % option)
    rebootctrl = RebootCtrl()
    rebootctrl.do_rebootctrl(option)


if __name__ == '__main__':
    main()
