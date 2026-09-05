#!/usr/bin/env python3
import click
import os
import subprocess
from ruijieconfig import STARTMODULE, GLOBALINITPARAM, GLOBALINITCOMMAND, GLOBALINITPARAM_PRE, GLOBALINITCOMMAND_PRE


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


def log_os_system(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output


def write_sysfs_value(reg_name, value):
    mb_reg_file = "/sys/bus/i2c/devices/" + reg_name
    if (not os.path.isfile(mb_reg_file)):
        print(mb_reg_file, 'not found !')
        return False
    try:
        with open(mb_reg_file, 'w') as fd:
            fd.write(value)
    except Exception as error:
        return False
    return True


def getPid(name):
    ret = []
    for dirname in os.listdir('/proc'):
        if dirname == 'curproc':
            continue
        try:
            with open('/proc/{}/cmdline'.format(dirname), mode='r') as fd:
                content = fd.read()
        except Exception:
            continue
        if name in content:
            ret.append(dirname)
    return ret


def startxdpe_avscontrol():
    if STARTMODULE.get('xdpe_avscontrol', 0) == 1:
        cmd = "nohup xdpe_avscontrol.py start >/dev/null 2>&1 &"
        rets = getPid("xdpe_avscontrol.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def starthal_fanctrl():
    if STARTMODULE.get('hal_fanctrl', 0) == 1:
        cmd = "nohup hal_fanctrl.py start >/dev/null 2>&1 &"
        rets = getPid("hal_fanctrl.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def starthal_ledctrl():
    if STARTMODULE.get('hal_ledctrl', 0) == 1:
        cmd = "nohup hal_ledctrl.py start >/dev/null 2>&1 &"
        rets = getPid("hal_ledctrl.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def startDevmonitor():
    if STARTMODULE.get('dev_monitor', 0) == 1:
        cmd = "nohup dev_monitor.py start >/dev/null 2>&1 &"
        rets = getPid("dev_monitor.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def startSff_temp_polling():
    if STARTMODULE.get('sff_temp_polling', 0) == 1:
        cmd = "nohup sfp_highest_temperatue.py >/dev/null 2>&1 &"
        rets = getPid("sfp_highest_temperatue.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def startPMON_sys():
    if STARTMODULE.get('rg_pmon_syslog', 0) == 1:
        cmd = "nohup rg_pmon_syslog.py >/dev/null 2>&1 &"
        rets = getPid("rg_pmon_syslog.py")
        if len(rets) == 0:
            os.system(cmd)
    return

def stopxdpe_avscontrol():
    if STARTMODULE.get('xdpe_avscontrol', 0) == 1:
        rets = getPid("xdpe_avscontrol.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def stophal_fanctrl():
    if STARTMODULE.get('hal_fanctrl', 0) == 1:
        rets = getPid("hal_fanctrl.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def stophal_ledctrl():
    if STARTMODULE.get('hal_ledctrl', 0) == 1:
        rets = getPid("hal_ledctrl.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def stopDevmonitor():
    if STARTMODULE.get('dev_monitor', 0) == 1:
        rets = getPid("dev_monitor.py")  #
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def stopSff_temp_polling():
    if STARTMODULE.get('sff_temp_polling', 0) == 1:
        rets = getPid("sfp_highest_temperatue.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def stopPMON_sys():
    if STARTMODULE.get('rg_pmon_syslog', 0) == 1:
        rets = getPid("rg_pmon_syslog.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)
    return


def otherinit():
    for index in GLOBALINITPARAM:
        write_sysfs_value(index["loc"], index["value"])

    for index in GLOBALINITCOMMAND:
        log_os_system(index)
    return


def otherinit_pre():
    for index in GLOBALINITPARAM_PRE:
        write_sysfs_value(index["loc"], index["value"])

    for index in GLOBALINITCOMMAND_PRE:
        log_os_system(index)


def unload_apps():
    stopPMON_sys()
    stopDevmonitor()
    stopxdpe_avscontrol()
    stophal_ledctrl()
    stophal_fanctrl()
    stopSff_temp_polling()


def load_apps():
    otherinit_pre()
    startSff_temp_polling()
    starthal_fanctrl()
    starthal_ledctrl()
    startxdpe_avscontrol()
    startDevmonitor()
    startPMON_sys()
    otherinit()


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass


@main.command()
def start():
    '''load process '''
    load_apps()


@main.command()
def stop():
    '''stop process '''
    unload_apps()


@main.command()
def restart():
    '''restart process'''
    unload_apps()
    load_apps()


if __name__ == '__main__':
    u'''process init operation'''
    main()
