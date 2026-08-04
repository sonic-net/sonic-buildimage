#!/usr/bin/env python3
import os
import subprocess
import glob
import time
import click
from platform_config import STARTMODULE, MAC_LED_RESET, AIRFLOW_RESULT_FILE
from platform_config import GLOBALINITPARAM, GLOBALINITCOMMAND, GLOBALINITPARAM_PRE, GLOBALINITCOMMAND_PRE
from platform_util import wbpciwr


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


def log_os_system(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output


def write_sysfs_value(reg_name, value):
    mb_reg_file = "/sys/bus/i2c/devices/" + reg_name
    locations = glob.glob(mb_reg_file)
    if len(locations) == 0:
        print("%s not found" % mb_reg_file)
        return False
    sysfs_loc = locations[0]
    try:
        with open(sysfs_loc, 'w') as fd:
            fd.write(value)
    except Exception:
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


def startAvscontrol():
    if STARTMODULE.get('avscontrol', 0) == 1:
        cmd = "nohup avscontrol.py start >/dev/null 2>&1 &"
        rets = getPid("avscontrol.py")
        if len(rets) == 0:
            os.system(cmd)


def startFanctrol():
    if STARTMODULE.get('fancontrol', 0) == 1:
        cmd = "nohup fancontrol.py start >/dev/null 2>&1 &"
        rets = getPid("fancontrol.py")
        if len(rets) == 0:
            os.system(cmd)


def starthal_fanctrl():
    if STARTMODULE.get('hal_fanctrl', 0) == 1:
        cmd = "nohup hal_fanctrl.py start >/dev/null 2>&1 &"
        rets = getPid("hal_fanctrl.py")
        if len(rets) == 0:
            os.system(cmd)


def starthal_ledctrl():
    if STARTMODULE.get('hal_ledctrl', 0) == 1:
        cmd = "nohup hal_ledctrl.py start >/dev/null 2>&1 &"
        rets = getPid("hal_ledctrl.py")
        if len(rets) == 0:
            os.system(cmd)


def startDevmonitor():
    if STARTMODULE.get('dev_monitor', 0) == 1:
        cmd = "nohup dev_monitor.py start >/dev/null 2>&1 &"
        rets = getPid("dev_monitor.py")
        if len(rets) == 0:
            os.system(cmd)


def startSlotmonitor():
    if STARTMODULE.get('slot_monitor', 0) == 1:
        cmd = "nohup slot_monitor.py start >/dev/null 2>&1 &"
        rets = getPid("slot_monitor.py")
        if len(rets) == 0:
            os.system(cmd)


def startIntelligentmonitor():
    if STARTMODULE.get('intelligent_monitor', 0) == 1:
        cmd = "nohup intelligent_monitor.py >/dev/null 2>&1 &"
        rets = getPid("intelligent_monitor.py")
        if len(rets) == 0:
            os.system(cmd)


def startSignalmonitor():
    if STARTMODULE.get('signal_monitor', 0) == 1:
        cmd = "nohup signal_monitor.py start >/dev/null 2>&1 &"
        rets = getPid("signal_monitor.py")
        if len(rets) == 0:
            os.system(cmd)


def startSff_temp_polling():
    if STARTMODULE.get('sff_temp_polling', 0) == 1:
        cmd = "nohup sfp_highest_temperatue.py >/dev/null 2>&1 &"
        rets = getPid("sfp_highest_temperatue.py")
        if len(rets) == 0:
            os.system(cmd)


def startRebootCause():
    if STARTMODULE.get('reboot_cause', 0) == 1:
        cmd = "nohup reboot_cause.py >/dev/null 2>&1 &"
        rets = getPid("reboot_cause.py")
        if len(rets) == 0:
            os.system(cmd)


def startPMON_sys():
    if STARTMODULE.get('pmon_syslog', 0) == 1:
        cmd = "nohup pmon_syslog.py >/dev/null 2>&1 &"
        rets = getPid("pmon_syslog.py")
        if len(rets) == 0:
            os.system(cmd)


def startSff_polling():
    if STARTMODULE.get('sff_polling', 0) == 1:
        cmd = "nohup sff_polling.py start > /dev/null 2>&1 &"
        rets = getPid("sff_polling.py")
        if len(rets) == 0:
            os.system(cmd)


def generate_air_flow():
    cmd = "nohup generate_airflow.py > /dev/null 2>&1 &"
    rets = getPid("generate_airflow.py")
    if len(rets) == 0:
        os.system(cmd)
        time.sleep(1)


def startGenerate_air_flow():
    if STARTMODULE.get('generate_airflow', 0) == 1:
        for i in range(10):
            generate_air_flow()
            if os.path.exists(AIRFLOW_RESULT_FILE):
                click.echo("%%WB_PLATFORM_PROCESS: generate air flow success")
                return
            time.sleep(1)
        click.echo("%%WB_PLATFORM_PROCESS: generate air flow,failed, %s not exits" % AIRFLOW_RESULT_FILE)
    return


def start_tty_console():
    if STARTMODULE.get('tty_console', 0) == 1:
        cmd = "nohup tty_console.py > /dev/null 2>&1 &"
        rets = getPid("tty_console.py")
        if len(rets) == 0:
            os.system(cmd)


def stopAvscontrol():
    if STARTMODULE.get('avscontrol', 0) == 1:
        rets = getPid("avscontrol.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopFanctrol():
    if STARTMODULE.get('fancontrol', 0) == 1:
        rets = getPid("fancontrol.py")  #
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stophal_fanctrl():
    if STARTMODULE.get('hal_fanctrl', 0) == 1:
        rets = getPid("hal_fanctrl.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stophal_ledctrl():
    if STARTMODULE.get('hal_ledctrl', 0) == 1:
        rets = getPid("hal_ledctrl.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopDevmonitor():
    if STARTMODULE.get('dev_monitor', 0) == 1:
        rets = getPid("dev_monitor.py")  #
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopSlotmonitor():
    if STARTMODULE.get('slot_monitor', 0) == 1:
        rets = getPid("slot_monitor.py")  #
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopIntelligentmonitor():
    if STARTMODULE.get('intelligent_monitor', 0) == 1:
        rets = getPid("intelligent_monitor.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopSignalmonitor():
    if STARTMODULE.get('signal_monitor', 0) == 1:
        rets = getPid("signal_monitor.py")  #
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopSff_temp_polling():
    if STARTMODULE.get('sff_temp_polling', 0) == 1:
        rets = getPid("sfp_highest_temperatue.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopPMON_sys():
    if STARTMODULE.get('pmon_syslog', 0) == 1:
        rets = getPid("pmon_syslog.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopRebootCause():
    if STARTMODULE.get('reboot_cause', 0) == 1:
        rets = getPid("reboot_cause.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopSff_polling():
    if STARTMODULE.get('sff_polling', 0) == 1:
        rets = getPid("sff_polling.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stopGenerate_air_flow():
    if STARTMODULE.get('generate_airflow', 0) == 1:
        rets = getPid("generate_airflow.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def stop_tty_console():
    if STARTMODULE.get('tty_console', 0) == 1:
        rets = getPid("tty_console.py")
        for ret in rets:
            cmd = "kill " + ret
            os.system(cmd)


def otherinit():
    for index in GLOBALINITPARAM:
        write_sysfs_value(index["loc"], index["value"])

    for index in GLOBALINITCOMMAND:
        log_os_system(index)


def otherinit_pre():
    for index in GLOBALINITPARAM_PRE:
        write_sysfs_value(index["loc"], index["value"])

    for index in GLOBALINITCOMMAND_PRE:
        log_os_system(index)


def unload_apps():
    stopSff_polling()
    stopPMON_sys()
    stopSignalmonitor()
    stopIntelligentmonitor()
    stopSlotmonitor()
    stopDevmonitor()
    stopAvscontrol()
    stophal_ledctrl()
    stophal_fanctrl()
    stopFanctrol()
    stopSff_temp_polling()
    stopRebootCause()
    stop_tty_console()
    stopGenerate_air_flow()


def MacLedSet(data):
    '''write pci register'''
    pcibus = MAC_LED_RESET.get("pcibus")
    slot = MAC_LED_RESET.get("slot")
    fn = MAC_LED_RESET.get("fn")
    resource = MAC_LED_RESET.get("bar")
    offset = MAC_LED_RESET.get("offset")
    val = MAC_LED_RESET.get(data, None)
    if val is None:
        click.echo("%%WB_PLATFORM_PROCESS-INIT: MacLedSet wrong input")
        return
    wbpciwr(pcibus, slot, fn, resource, offset, val)


def load_apps():
    otherinit_pre()
    startGenerate_air_flow()
    start_tty_console()
    startRebootCause()
    startSff_temp_polling()
    startFanctrol()
    starthal_fanctrl()
    starthal_ledctrl()
    startAvscontrol()
    startDevmonitor()
    startSlotmonitor()
    startIntelligentmonitor()
    startSignalmonitor()
    startPMON_sys()
    startSff_polling()
    otherinit()
    if STARTMODULE.get("macledreset", 0) == 1:
        MacLedSet("reset")


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


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
    main()
