#!/usr/bin/env python3
import os
import subprocess
import glob
import time
import click
import shutil
from platform_config import STARTMODULE, AIRFLOW_RESULT_FILE, MGMT_VERSION_PATH
from platform_config import GLOBALINITPARAM, GLOBALINITCOMMAND, GLOBALINITPARAM_PRE, GLOBALINITCOMMAND_PRE
from platform_util import wbpciwr, set_value, exec_os_cmd


CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}

# Map module names to their corresponding script filenames
module_to_script = {
    "product_name": "product_name.py",
    "drv_update": "drv_update.py",
    "tty_console": "tty_console.py",
    "reboot_cause": "reboot_cause.py",
    "set_fw_mac": "set_fw_mac.py",
    "sff_temp_polling": "sfp_highest_temperatue.py",
    "fancontrol": "fancontrol.py",
    "hal_fanctrl": "hal_fanctrl.py",
    "hal_ledctrl": "hal_ledctrl.py",
    "avscontrol": "avscontrol.py",
    "dev_monitor": "dev_monitor.py",
    "slot_monitor": "slot_monitor.py",
    "intelligent_monitor": "intelligent_monitor.py",
    "signal_monitor": "signal_monitor.py",
    "pmon_syslog": "pmon_syslog.py",
    "sff_polling": "sff_polling.py",
    "get_mac_temperature": "get_mac_temperature.py",
}

PROCESS_APPLIST = [
    {"script_name": script_name}
    for module, script_name in module_to_script.items()
    if STARTMODULE.get(module) == 1
]

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

def Generate_mgmt_version():
    cmd = "nohup platform_manufacturer.py -u > /dev/null 2>&1 &"
    rets = getPid("platform_manufacturer.py")
    if len(rets) == 0:
        exec_os_cmd(cmd)

def startGenerate_mgmt_version():
    if STARTMODULE.get('generate_mgmt_version', 1) == 1: #default value 1 to generate
        for i in range(10):
            Generate_mgmt_version()
            if os.path.exists(MGMT_VERSION_PATH):
                #click.echo("%%WB_PLATFORM_PROCESS: generate mgmt_version success")
                return
            time.sleep(1)
        click.echo("%%WB_PLATFORM_PROCESS: generate mgmt_version,failed, %s not exits" % MGMT_VERSION_PATH)
    return

def generate_air_flow():
    cmd = "nohup generate_airflow.py > /dev/null 2>&1 &"
    rets = getPid("generate_airflow.py")
    if len(rets) == 0:
        exec_os_cmd(cmd)
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



def stopGenerate_air_flow():
    if STARTMODULE.get('generate_airflow', 0) == 1:
        rets = getPid("generate_airflow.py")
        for ret in rets:
            cmd = "kill " + ret
            exec_os_cmd(cmd)



def otherinit():
    for index in GLOBALINITPARAM:
        if isinstance(index, dict):
            loc = index.get("loc", None)
            value = index.get("value", None)
            if loc and value:
                write_sysfs_value(loc, value)
            else:
                click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, loc or value is None, config %s" % index)
        else:
            click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, the config %s is not a dict type" % index)

    for index in GLOBALINITCOMMAND:
        if isinstance(index, dict):
            set_value(index)
        else:
            log_os_system(index)


def otherinit_pre():
    for index in GLOBALINITPARAM_PRE:
        if isinstance(index, dict):
            loc = index.get("loc", None)
            value = index.get("value", None)
            if loc and value:
                write_sysfs_value(loc, value)
            else:
                click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, loc or value is None, config %s" % index)
        else:
            click.echo("%%WB_PLATFORM_PROCESS: failed to initialize the parameter, the config %s is not a dict type" % index)

    for index in GLOBALINITCOMMAND_PRE:
        if isinstance(index, dict):
            set_value(index)
        else:
            log_os_system(index)

def copy_machineconf():
    try:
        shutil.copyfile("/host/machine.conf", "/etc/sonic/machine.conf")
        return True
    except Exception:
        return False


def unload_apps():
    app_list = PROCESS_APPLIST
    #Reverse the list
    app_list = app_list[::-1]
    for app_config in app_list:
        app_config["gettype"] = "unload_process"
        script_name = app_config.get("script_name")
        ret, log = set_value(app_config)
        if ret is not True:
            print("app:%s unload failed.log:%s" % (script_name, log))
        else:
            print("app:%s unload success." % script_name)

def load_apps():
    app_list = PROCESS_APPLIST
    for app_config in app_list:
        app_config["gettype"] = "load_process"
        script_name = app_config.get("script_name")
        ret, log = set_value(app_config)
        if ret is not True:
            print("app:%s load failed.log:%s" % (script_name, log))
        else:
            print("app:%s load success." % script_name)

def start_process():
    copy_machineconf()
    otherinit_pre()
    startGenerate_air_flow()
    load_apps()
    startGenerate_mgmt_version()
    otherinit()

def stop_process():
    stopGenerate_air_flow()
    unload_apps()

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


@main.command()
def start():
    '''start process '''
    start_process()


@main.command()
def stop():
    '''stop process '''
    stop_process()


@main.command()
def restart():
    '''restart process'''
    stop_process()
    start_process()


if __name__ == '__main__':
    main()
