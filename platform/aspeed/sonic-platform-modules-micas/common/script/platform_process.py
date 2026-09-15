#!/usr/bin/env python_nos
import os
import subprocess
import glob
import time
import signal
import click
import shutil
from platform_config import STARTMODULE, AIRFLOW_RESULT_FILE
from platform_config import GLOBALINITPARAM, GLOBALINITCOMMAND, GLOBALINITPARAM_PRE, GLOBALINITCOMMAND_PRE
from platform_util import AliasedGroup, CONTEXT_SETTINGS, wbpciwr, set_value, supervisor_update, check_supervisor_ready, update_mgmt_version
from public.platform_diff_util import platform_process_other_init, platform_process_other_init_pre
from public.platform_common_config import CURRENT_LOAD_APP_WAY, LOAD_APP_BY_COMMON, UNLOAD_APP_BY_COMMON, LOAD_APP_BY_SUPERVISOR, UNLOAD_APP_BY_SUPERVISOR, MGMT_VERSION_PATH
from public.platform_common_config import EXECUTABLE_FILE_PATH, PLATFORM_INFO_DIR, HOST_MACHINE

# Map module names to their corresponding script filenames
module_to_script = {
    "product_name": "product_name.py",
    "drv_update": "drv_update.py",
    "tty_console": "tty_console.py",
    "reboot_cause": "reboot_cause.py",
    "set_fw_mac": "set_fw_mac.py",
    "eeupdate_set_mac": "set_fw_mac_by_eeupdate.py",
    "sfp_highest_temperature": "sfp_highest_temperature.py",
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
    "dfx_xdpe_monitor": "dfx_xdpe_monitor.py",
    "dfx_reg_monitor": "dfx_reg_monitor.py",
    "dfx_clock_monitor": "dfx_clock_monitor.py",
    "dfx_blackbox_record": "dfx_blackbox_record.py",
    "plugins_init": "plugins_init.py",
    "leak_monitor": "leak_monitor.py",
    "set_eth_mac": "set_eth_mac.py",
    "sff_temp_polling": "sff_temp_polling.py",
    "amd_ras": "amd_ras",
    "apml_dimm_temp": "apml_dimm_temp",
    "platform_logic_monitor": "platform_logic_monitor.py",
    "event_notify": "event_notify.py",
    "bmc_pcie_dev_init": "bmc_pcie_dev_init.py",
    "sata_disk_monitor": "sata_disk_monitor.py",
    "md_monitor": "md_monitor.py",
    "platform_fw_check": "platform_fw_check.py",
    "dfx_ucd_monitor": "dfx_ucd_monitor.py",
    "pstore_control": "pstore_control.py",
    "mem_rank_isolate_record": "mem_rank_isolate_record.py",
    "mac_temp_monitor": "mac_temp_monitor.py",
}

PROCESS_APPLIST = [
    {"script_name": script_name}
    for module, script_name in module_to_script.items()
    if STARTMODULE.get(module) == 1
]

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

def generate_air_flow():
    cmd = [f"{EXECUTABLE_FILE_PATH}generate_airflow.py"]
    rets = getPid("generate_airflow.py")
    if len(rets) == 0:
        with open(os.devnull, "w") as devnull:
            subprocess.Popen(cmd, stdout=devnull, stderr=devnull, start_new_session=True)
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
            os.kill(int(ret), signal.SIGTERM)



def otherinit():
    # Subsequent products disable the GLOBALINITPARAM configuration
    platform_process_other_init(GLOBALINITPARAM, GLOBALINITCOMMAND)

def otherinit_pre():
    # Subsequent products disable the GLOBALINITPARAM configuration
    platform_process_other_init_pre(GLOBALINITPARAM_PRE, GLOBALINITCOMMAND_PRE)

def copy_machineconf():
    try:
        shutil.copyfile(HOST_MACHINE, PLATFORM_INFO_DIR + "machine.conf")
        return True
    except Exception:
        return False


def unload_apps(unload_app_way):
    app_list = PROCESS_APPLIST
    #Reverse the list
    app_list = app_list[::-1]
    for app_config in app_list:
        app_config["gettype"] = unload_app_way
        script_name = app_config.get("script_name")
        if unload_app_way == UNLOAD_APP_BY_SUPERVISOR:
            app_config["script_name"] = script_name.split(".", 1)[0].strip()
        ret, log = set_value(app_config)
        if ret is not True:
            print("app:%s unload failed.log:%s" % (script_name, log))
        else:
            print("app:%s unload success.log:%s" % (script_name, log))

def load_apps(load_app_way):
    app_list = PROCESS_APPLIST
    for app_config in app_list:
        app_config["gettype"] = load_app_way
        script_name = app_config.get("script_name")
        if load_app_way == LOAD_APP_BY_SUPERVISOR:
            app_config["script_name"] = script_name.split(".", 1)[0].strip()
        ret, log = set_value(app_config)
        if ret is not True:
            print("app:%s load failed.log:%s" % (script_name, log))
        else:
            print("app:%s load success.log:%s" % (script_name, log))

def start_process():
    copy_machineconf()
    otherinit_pre()
    startGenerate_air_flow()
    if CURRENT_LOAD_APP_WAY == LOAD_APP_BY_SUPERVISOR:
        check_supervisor_ready()
        supervisor_update()
    load_apps(CURRENT_LOAD_APP_WAY)
    update_mgmt_version(STARTMODULE.get('generate_mgmt_version', 0))
    otherinit()

def stop_process():
    stopGenerate_air_flow()
    if CURRENT_LOAD_APP_WAY == LOAD_APP_BY_SUPERVISOR:
        unload_app_way = UNLOAD_APP_BY_SUPERVISOR
    else:
        unload_app_way = UNLOAD_APP_BY_COMMON
    unload_apps(unload_app_way)

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
