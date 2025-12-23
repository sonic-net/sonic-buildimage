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
    api_helper_platform = apiHelper.get_platform()
    platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
    docker_id = apiHelper.get_dpu_docker_imageID()
    cmd = "sudo docker cp {}:/tmp/fru.json /usr/share/sonic/device/{}/fru.json".format(docker_id, platform)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/nic/etc/VERSION.json /usr/share/sonic/device/{}/VERSION.json".format(docker_id, platform)
    run_cmd(cmd)
    cmd = "sudo docker cp {}:/usr/bin/mmc /usr/local/bin".format(docker_id)
    run_cmd(cmd)
    try:
        slot_id = apiHelper.run_docker_cmd("cpldapp -r 0xA").strip()
        file = "/usr/share/sonic/device/{}/dpu_slot_id".format(apiHelper.get_platform())
        with open(file, "w") as f:
            f.write(slot_id)
    except Exception as e:
        log_err("failed to setup slot_id at platform dir due to {}".format(e))

    try:
        board_id = apiHelper.run_docker_cmd("cpldapp -r 0x80").strip()
        file = "/usr/share/sonic/device/{}/dpu_board_id".format(apiHelper.get_platform())
        with open(file, "w") as f:
            f.write(board_id)
    except Exception as e:
        log_err("failed to setup board_id at platform dir due to {}".format(e))


def setup_platform_components_json(slot_id):
    try:
        api_helper_platform = apiHelper.get_platform()
        platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
        filename = "/usr/share/sonic/device/{}/platform_components.json".format(platform)
        with open(filename, "r") as f:
            data = json.load(f)

        def replace_keys(obj):
            if isinstance(obj, dict):
                return {key.replace("-0", f"-{slot_id}"): replace_keys(value) for key, value in obj.items()}
            elif isinstance(obj, list):
                return [replace_keys(item) for item in obj]
            return obj

        updated_data = replace_keys(data)

        with open(filename, "w") as f:
            json.dump(updated_data, f, indent=4)
        log_info("successfully updated platform_components.json")
    except Exception as e:
        log_err("failed to setup platform_components.json due to {}".format(e))

def config_setup():
    try:
        from sonic_platform.chassis import Chassis
        slot_id = Chassis().get_my_slot()
    except Exception as e:
        log_err("failed to get slot id due to {}".format(e))

    try:
        cmd = f'sonic-cfggen -a "{{\\"INTERFACE\\": {{\\"Ethernet0\\": {{}},\\"Ethernet0|18.{slot_id}.202.1/31\\": {{}}}}}}" --write-to-db'
        run_cmd(cmd)
    except Exception as e:
        log_err("failed to set Ethernet0 ip due to {}".format(e))

    setup_platform_components_json(slot_id)

    try:
        run_cmd("mkdir -p /host/images")
        run_cmd("mkdir -p /data")
        run_cmd("chmod +x /boot/install_file")
    except Exception as e:
        log_err("failed to setup fwutil due to {}".format(e))


def set_onie_version():
    api_helper_platform = apiHelper.get_platform()
    platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
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
        fru_file = "/usr/share/sonic/device/{}/fru.json".format(platform)
        data = json.load(open(fru_file))
        data["onie_version"] = version
        with open(fru_file, "w") as json_file:
            json.dump(data, json_file, indent=4)
    except:
        pass

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
    dpu_slot_id = int(apiHelper.run_docker_cmd("cpldapp -r 0xA").strip(), 16)
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
    time.sleep(5)
    configure_iptable_rules()
    fetch_dpu_files()
    config_setup()
    time.sleep(5)
    set_onie_version()
    pcie_tx_setup()

if __name__ == "__main__":
    main()

