# {C} Copyright 2023 AMD Systems Inc. All rights reserved

import os
import subprocess
import sys
import logging
import time
import re
import json
from sonic_platform.helper import APIHelper

QSFP_STAT_CTRL_CPLD_ADDR = "0x2"
apiHelper = APIHelper()

def run_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status != 0:
        try:
            logger = open("/var/log/dpu_pensando_util.log", "a")
            logger.write(cmd + " failed with err " + str(status) + "\n")
            logger.close()
        except:
            print("platform_init: Failed file ops while logging")
    return output

def get_python_lib_dir():
    try:
        cmd = "python3 -V"
        output = run_cmd(cmd)
        pattern = "Python\s[0-9].[0-9]*"
        python_version = re.findall(pattern, output)[0].split()[1]
        return f'python{python_version}'
    except:
        return 'python3.9'

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

def cp_sonic_platform_helper():
    python_dir = get_python_lib_dir()
    lib_path = f"/usr/local/lib/{python_dir}/dist-packages/"
    sonic_platform_path = lib_path + "sonic_platform/"
    sonic_platform_helper_path = sonic_platform_path  + "helper.py"
    if not os.path.exists(sonic_platform_helper_path):
        sonic_platform_old_path = "/usr/lib/python3/dist-packages/sonic_platform"
        cmd = "cp -r " + sonic_platform_old_path + " " + lib_path
        run_cmd(cmd)

def set_ubootenv_config():
    cmd = "cat /proc/mtd | grep -e 'ubootenv' | awk '{print $1}' | tr -dc '0-9'"
    mtd_ubootenv = run_cmd(cmd)
    fw_env_config = "/dev/mtd{} 0x0 0x1000 0x10000".format(mtd_ubootenv)
    with open("/etc/fw_env.config","w") as f:
        f.write(fw_env_config)

def start_pmon():
    cmd = "systemctl is-enabled pmon.timer"
    output = run_cmd(cmd)
    if output == "disabled":
        try:
            cmd = "systemctl enable pmon.timer"
            run_cmd(cmd)
        except:
            pass
    elif output == "static":
        try:
            cmd = "systemctl enable pmon.timer"
            run_cmd(cmd)
        except:
            pass
    else:
        pass
    cmd = "systemctl is-enabled pmon"
    output = run_cmd(cmd)
    if output == "static":
        try:
            cmd = "systemctl start pmon"
            run_cmd(cmd)
        except:
            pass

def write_platform_component_json():
    try:
        fru_path = "/home/admin/fru.json"
        fru_data = json.load(open(fru_path))
        model = fru_data.get("part-number","")
        chassis_name = apiHelper.get_hwsku()
        if model != "":
            chassis_name = chassis_name + " " + model

        api_helper_platform = apiHelper.get_platform()
        platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
        component_file = f"/usr/share/sonic/device/{platform}/platform_components.json"
        component = {
            "component" : {
                "eMMC" : {},
                "cpld" : {}
            }
        }
        if os.path.exists(component_file):
            data = json.load(open(component_file))
            component = data["chassis"][list(data["chassis"].keys())[0]]
        data_dict = {}
        data_dict["chassis"] = {}
        data_dict["chassis"][chassis_name] = component
        with open(component_file, "w") as json_file:
            json.dump(data_dict, json_file, indent=4)
    except:
        pass

def read_qsfp_eeprom_helper(i2c_num,reg):
    eeprom_byte_array = APIHelper().read_eeprom_qsfp(i2c_num,reg)
    api_helper_platform = apiHelper.get_platform()
    platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
    eeprom_file = f"/usr/share/sonic/device/{platform}/qsfp-eeprom-{i2c_num}.bin"
    with open(eeprom_file, "wb") as file:
        file.write(eeprom_byte_array)

def read_qsfp_eeprom():
    cmd = "cpldapp -r {}".format(QSFP_STAT_CTRL_CPLD_ADDR)
    try:
        output = APIHelper().runCMD(cmd)
        reg_val = int(output, 16)
        if ((reg_val >> 4) & 1) > 0:
            read_qsfp_eeprom_helper(1, "0x50")
        if ((reg_val >> 4) & 2) > 0:
            read_qsfp_eeprom_helper(2, "0x50")
    except:
        pass

def stop_disable_telemetry_service():
    cmd = "systemctl stop telemetry.service"
    run_cmd(cmd)

    cmd = "systemctl disable telemetry.service"
    run_cmd(cmd)

def stop_disable_mgmt_framework_service():
    cmd = "systemctl stop mgmt-framework.service"
    run_cmd(cmd)

    cmd = "systemctl disable mgmt-framework.service"
    run_cmd(cmd)

def stop_disable_featured_service():
    cmd = "systemctl stop featured.service"
    run_cmd(cmd)

    cmd = "systemctl disable featured.service"
    run_cmd(cmd)

def rename_oob_mnic0_to_eth0():
    cmd = "dhclient oob_mnic0"
    run_cmd(cmd)

    cmd = "ip link property add dev oob_mnic0 altname eth0"
    run_cmd(cmd)

    cmd = "dhclient -r oob_mnic0"
    run_cmd(cmd)

    cmd = "dhclient eth0 &"
    run_cmd(cmd)

def add_dpu_to_package_json():
    api_helper_platform = apiHelper.get_platform()
    platform = api_helper_platform if api_helper_platform != None else "arm64-elba-asic-r0"
    version_path = "/usr/share/sonic/device/{}/VERSION.json".format(platform)
    package_path = "/var/lib/sonic-package-manager/packages.json"
    try:
        version_data = json.load(open(version_path))
        package_data = json.load(open(package_path))
        dpu_version = version_data["sw"]["version"]
        dpu_sha = version_data["sw"]["sha"]
        dpu_package_data = {}
        dpu_package_data["repository"] = "docker-dpu-pensando"
        dpu_package_data["description"] = "Pensando dpu container"
        dpu_package_data["default-reference"] = dpu_version
        dpu_package_data["installed-version"] = dpu_version
        dpu_package_data["installed"] = True
        dpu_package_data["built-in"] = True
        dpu_package_data["image-id"] = f"sha:{dpu_sha}"
        package_data["dpu"] = dpu_package_data
        with open(package_path, "w") as json_file:
            json.dump(package_data, json_file, indent=4)
    except:
        pass

def start_ssh_keygen():
    try:
        ssh_keygen_cmd = "sudo ssh-keygen -A"
        run_cmd(ssh_keygen_cmd)
    except:
        pass

def sync_system_date():
    try:
        hwclock_cmd = "sudo hwclock -s"
        run_cmd(hwclock_cmd)
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
    start_ssh_keygen()
    time.sleep(10)
    sync_system_date()
    cp_sonic_platform_helper()
    fetch_dpu_files()
    time.sleep(5)
    set_onie_version()
    cp_to_shared_mem()
    set_cpldapp()
    read_qsfp_eeprom()
    rename_oob_mnic0_to_eth0()
    start_pmon()
    add_dpu_to_package_json()
    write_platform_component_json()
    stop_disable_telemetry_service()
    stop_disable_mgmt_framework_service()
    stop_disable_featured_service()
    pcie_tx_setup()

if __name__ == "__main__":
    main()
