# {C} Copyright 2023 AMD Systems Inc. All rights reserved

import os
import subprocess
import sys
import logging
import time
import re
import json
from sonic_platform.helper import APIHelper
from sonic_py_common import syslogger

QSFP_STAT_CTRL_CPLD_ADDR = "0x2"
apiHelper = APIHelper()

SYSLOG_IDENTIFIER = 'dpu_pensando_util'
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_info(msg, also_print_to_console=False):
    logger_instance.log_info(msg, also_print_to_console)

def log_err(msg, also_print_to_console=True):
    logger_instance.log_error(msg, also_print_to_console)

def run_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status != 0:
        try:
            log_err(cmd + " failed with err " + str(status) + "\n")
        except:
            log_err("platform_init: Failed file ops while logging")
    else:
        log_info(f"command executed : {cmd}")
        log_info(output)
    return output

def fetch_dpu_files():
    docker_id = apiHelper.get_dpu_docker_imageID()
    cmd = "sudo docker cp {}:/tmp/fru.json /home/admin".format(docker_id)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/nic/bin/cpldapp /home/admin".format(docker_id)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/nic/lib/libpal.so /home/admin".format(docker_id)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/nic/lib/liblogger.so /home/admin".format(docker_id)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/nic/etc/VERSION.json /home/admin".format(docker_id)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/usr/bin/mmc /usr/local/bin".format(docker_id)
    run_cmd(cmd)

def set_onie_version():
    version = ''
    try:
        cmd = 'cat /host/machine.conf | grep -i onie_version'
        output = run_cmd(cmd)
        version = output.split('=')[1]
    except:
        pass
    if version == '':
        try:
            cmd = "fw_printenv onie_version"
            output = run_cmd(cmd)
            version = output.split('=')[1]
        except:
            version = "Not Available"
            pass
    try:
        fru_file = "/home/admin/fru.json"
        data = json.load(open(fru_file))
        data["onie_version"] = version
        with open(fru_file, "w") as json_file:
            json.dump(data, json_file, indent=4)
    except:
        pass

def cp_to_shared_mem():
    api_helper_platform = apiHelper.get_platform()
    platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
    cmd = "sudo cp /home/admin/fru.json /usr/share/sonic/device/{}/fru.json".format(platform)
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/cpldapp /usr/share/sonic/device/{}/cpldapp".format(platform)
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/libpal.so /usr/share/sonic/device/{}/libpal.so".format(platform)
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/liblogger.so /usr/share/sonic/device/{}/liblogger.so".format(platform)
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/VERSION.json /usr/share/sonic/device/{}/VERSION.json".format(platform)
    run_cmd(cmd)

def set_cpldapp():
    cmd = "sudo cp /home/admin/cpldapp /usr/local/bin"
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/libpal.so /lib/libpal.so"
    run_cmd(cmd)
    cmd = "sudo cp /home/admin/liblogger.so /lib/liblogger.so"
    run_cmd(cmd)

def set_ubootenv_config():
    cmd = "cat /proc/mtd | grep -e 'ubootenv' | awk '{print $1}' | tr -dc '0-9'"
    mtd_ubootenv = run_cmd(cmd)
    fw_env_config = "/dev/mtd{} 0x0 0x1000 0x10000".format(mtd_ubootenv)
    with open("/etc/fw_env.config","w") as f:
        f.write(fw_env_config)

def configure_iptable_rules():
    try:
        iptable_cfg_cmd = "sudo iptables-legacy -D tcp_inbound -p tcp --dport 11357:11360 -j DROP"
        run_cmd(iptable_cfg_cmd)
    except:
        pass

def pcie_tx_setup():
    dpu_slot_id = int(run_cmd("cpldapp -r 0xA").strip(), 16)
    if dpu_slot_id == 6 or dpu_slot_id == 7:
        run_cmd("docker exec polaris /nic/tools/run-aacs-server.sh -p 9001 export SERDES_DUT_IP=localhost:9001")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl serdes -addr 1:39 -pre 0 -post 0 -atten 10")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl serdes -addr 1:3b -pre 0 -post 0 -atten 10")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl serdes -addr 1:3d -pre 0 -post 0 -atten 10")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl serdes -addr 1:3f -pre 0 -post 0 -atten 10")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl dev -addr 1:39 -v 1")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl dev -addr 1:3b -v 1")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl dev -addr 1:3d -v 1")
        run_cmd("docker exec -e SERDES_ADDR=1:1-1:3f -e SERDES_DUT_IP=localhost:9001 -e SERDES_SBUS_RINGS=4 polaris aapl dev -addr 1:3f -v 1")

def main():
    #All init routines to be written below
    cmd = 'PATH=$PATH:/usr/sbin/'
    run_cmd(cmd)
    try:
        set_ubootenv_config()
    except:
        pass
    time.sleep(10)
    configure_iptable_rules()
    fetch_dpu_files()
    time.sleep(5)
    set_onie_version()
    cp_to_shared_mem()
    set_cpldapp()
    pcie_tx_setup()

if __name__ == "__main__":
    main()

