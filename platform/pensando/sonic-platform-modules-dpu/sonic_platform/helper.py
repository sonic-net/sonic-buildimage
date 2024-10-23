# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# Module contains an implementation of helper functions that would be
# required for various plugin files
#
#############################################################################

import os
import sys
import subprocess
import re
import subprocess
from sonic_py_common import device_info
import syslog
import json

HOST_CHK_CMD = "docker > /dev/null 2>&1"
QSFP_STAT_CTRL_CPLD_ADDR = "0x2"
DPU_DOCKER_IMAGE_NAME_FILE="/host/dpu-docker-info/image"
DPU_DOCKER_CONTAINER_NAME_FILE = "/host/dpu-docker-info/name"

class APIHelper():

    def __init__(self):
        pass

    def get_platform(self):
        return device_info.get_platform()

    def get_hwsku(self):
        return device_info.get_hwsku()

    def is_host(self):
        return os.system(HOST_CHK_CMD) == 0

    def run_command(self, cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen(
                cmd, shell=False, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
        except:
            status = False
        return status, result

    def get_dpu_docker_container_name(self):
        docker_container_name = None
        try:
            with open(DPU_DOCKER_CONTAINER_NAME_FILE, "r") as file:
                docker_container_name = file.read()
            return docker_container_name.replace('\n', '')
        except:
            return "dpu"

    def get_dpu_docker_image_name(self):
        docker_image_name = None
        try:
            with open(DPU_DOCKER_IMAGE_NAME_FILE, "r") as file:
                docker_image_name = file.read()
            return docker_image_name.replace('\n', '')
        except:
            return "dpu:v1"

    def get_dpu_docker_imageID(self):

        docker_image_name = self.get_dpu_docker_image_name()
        cmd = f'docker ps | grep {docker_image_name}'
        try:
            output = self.runCMD(cmd)
            pattern = f"[a-f0-9]*[\s]*{docker_image_name}"
            docker_image_id = re.findall(pattern, output)[0].split(" ")[0]
            return docker_image_id
        except:
            return None

    def run_docker_cmd(self, inp_cmd):
        docker_image_id = self.get_dpu_docker_imageID()
        if docker_image_id == None:
            return None
        cmd = f"docker exec {docker_image_id} "
        cmd += inp_cmd
        return self.runCMD(cmd)

    def runCMD(self,cmd):
        try:
            ret = subprocess.getoutput(cmd)
        except Exception as e:
            print("Exception in runCMD due to %s, cleaning up and exiting\n" % str(e))
            sys.exit(2)
        return ret

    def get_board_id(self):
        try:
            board_id_hex = self.runCMD("cpldapp -r 0x80")
            if board_id_hex:
                board_id = int(board_id_hex, 16)
                return board_id
        except Exception as e:
            print(f"Failed to get board id due to {e}")
            return 0

    def readline_txt_file(self, path):
        try:
            with open(path, 'r') as f:
                output = f.readline()
            return output.strip('\n')
        except Exception:
            pass
        return ''

    def read_eeprom_qsfp(self, i2c_num, reg):
        cmd = f"i2cdump -y {i2c_num} {reg} b"
        output = self.runCMD(cmd)
        eeprom_dump = [line.split(" ")[1:17] for line in output.split("\n")[1:]]
        eeprom_raw = []
        for line in eeprom_dump:
            eeprom_raw.extend([int(byte,16) for byte in line])
        eeprom_byte_array = bytearray(eeprom_raw)
        return eeprom_byte_array

    def read_eeprom_qsfp_file(self, i2c_num, reg):
        eeprom_file = f"/usr/share/sonic/platform/qsfp-eeprom-{i2c_num}.bin"
        if self.is_host():
            platform = self.get_platform()
            eeprom_file = f"/usr/share/sonic/device/{platform}/qsfp-eeprom-{i2c_num}.bin"
        try:
            with open(eeprom_file, "rb") as file:
                read_data = bytearray(file.read())
                return read_data
        except:
            return self.read_eeprom_qsfp(i2c_num, reg)

    def setup_cpldapp(self):
        cmd = "cp /usr/share/sonic/platform/cpldapp /usr/local/bin"
        self.runCMD(cmd)
        cmd = "cp /usr/share/sonic/platform/libpal.so /lib/libpal.so"
        self.runCMD(cmd)
        cmd = "cp /usr/share/sonic/platform/liblogger.so /lib/liblogger.so"
        self.runCMD(cmd)

    def check_xcvrs_present(self):
        cmd = "cpldapp -r {}".format(QSFP_STAT_CTRL_CPLD_ADDR)
        try:
            output = APIHelper().runCMD(cmd)
            reg_val = int(output, 16)
            if ((reg_val >> 4) & 3) > 0:
                return True
            else:
                return False
        except:
            return False

    def read_port_config(self):
        """
            Reads port_config.ini file and fetches name for sfp port
        """
        sfp_name_map = []
        host_platform_root_path = '/usr/share/sonic/device'
        docker_hwsku_path = '/usr/share/sonic/hwsku'
        host_platform_path = "/".join([host_platform_root_path, self.get_platform()])
        hwsku_path = "/".join([host_platform_path, self.get_hwsku()]) if self.is_host() else docker_hwsku_path
        port_config_path =  "/".join([hwsku_path, "port_config.ini"])
        with open(port_config_path) as f:
            lines = f.read().splitlines()
            for line in lines:
                if line.startswith("#"):
                    continue
                l = line.split()
                sfp_name_map.append(l[2])
        return sfp_name_map
