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
DOCKER_HWSKU_PATH = '/usr/share/sonic/platform'
CPLD_REV_MASK = 0x7
CPLD_REV_SHIFT = 4

class APIHelper():

    mtfuji_board_id = 130
    mtfuji_rev_v1 = "V1"
    mtfuji_rev_v2 = "V2"

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
        result = []
        try:
            p = subprocess.Popen(
                cmd,
                shell=False,
                universal_newlines=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                env=os.environ
            )

            for line in iter(p.stdout.readline, ''):
                print(line.strip())
                result.append(line.strip())

            for line in iter(p.stderr.readline, ''):
                print(line.strip())
                result.append(line.strip())

            p.wait()

            if p.returncode != 0:
                status = False  # Non-zero return code indicates failure

        except FileNotFoundError as e:
            status = False
            print(f"Command not found: {e}")
        except Exception as e:
            status = False
            print(f"Error running command: {e}")

        return status, "\n".join(result)

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
            if self.is_host():
                board_id_hex = self.run_docker_cmd("cpldapp -r 0x80")
                if board_id_hex:
                    board_id = int(board_id_hex, 16)
                    return board_id
            else:
                board_id_file = DOCKER_HWSKU_PATH + "/dpu_board_id"
                board_id_hex = open(board_id_file, "r").read()
                if board_id_hex:
                    board_id = int(board_id_hex, 16)
                    return board_id
            return 0
        except Exception as e:
            print(f"Failed to get board id due to {e}")
            return 0

    def get_board_rev(self):
        cmd = "cpldapp -r 0xA"
        slot_id = 0
        try:
            if self.is_host():
                slot_id = self.run_docker_cmd(cmd)
            else:
                slot_id_file = DOCKER_HWSKU_PATH + "/dpu_slot_id"
                slot_id = open(slot_id_file, "r").read()
            if self.get_board_id() == self.mtfuji_board_id:
                rev = (int(slot_id, 16) >> CPLD_REV_SHIFT) & CPLD_REV_MASK
                if rev == 0x2:
                    return self.mtfuji_rev_v2
                else:
                    return self.mtfuji_rev_v1
            return "N/A"
        except:
            return "N/A"

    def readline_txt_file(self, path):
        try:
            with open(path, 'r') as f:
                output = f.readline()
            return output.strip('\n')
        except Exception:
            pass
        return ''

    def get_slot_id(self):
        cmd = "cpldapp -r 0xA"
        try:
            if self.is_host():
                slot_id = self.run_docker_cmd(cmd)
                return int(slot_id,16)
            else:
                slot_id_file = DOCKER_HWSKU_PATH + "/dpu_slot_id"
                slot_id_hex = open(slot_id_file, "r").read()
                if slot_id_hex:
                    slot_id = int(slot_id_hex, 16)
                    return slot_id
            return -1
        except:
            return -1

