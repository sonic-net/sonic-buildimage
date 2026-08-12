#!/usr/bin/python


import subprocess
import mmap
import requests
import os
import pexpect
import base64
import time
import json
import logging
import ast
import re
from rest.rest import BMCMessage
from rest_api import bmc_restful_logger
from datetime import datetime
from requests.exceptions import ReadTimeout, HTTPError, RequestException

try:
    from sonic_fwmgr.fwgmr_base import FwMgrUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class FwMgrUtil(FwMgrUtilBase):
    BMC_REQ_BASE_URI = "http://240.1.1.1:8080/api"
    ONIE_CFG_FILE = "/host/machine.conf"

    def __init__(self):
        self.platform_name = "AS5148c2g"
        self.bios_bootstatus_uri = "/".join([self.BMC_REQ_BASE_URI, "misc/biosbootstatus"])
        self.bmc_info_uri = "/".join([self.BMC_REQ_BASE_URI, "bmc/info"])
        self.bmc_nextboot_uri = "/".join([self.BMC_REQ_BASE_URI, "bmc/nextboot"])
        self.bmc_reboot_uri = "/".join([self.BMC_REQ_BASE_URI, "bmc/reboot"])
        self.fw_upgrade_uri = "/".join([self.BMC_REQ_BASE_URI, "firmware/upgrade"])
        self.fw_refresh_uri = "/".join([self.BMC_REQ_BASE_URI, "firmware/refresh"])
        self.bios_nextboot_uri = "/".join([self.BMC_REQ_BASE_URI, "firmware/biosnextboot"])
        self.bmc_rawcmd_uri = "/".join([self.BMC_REQ_BASE_URI, "hw/rawcmd"])

        self.fw_upgrade_logger_path = "/var/log/fw_upgrade.log"
        self.fw_refresh_logger_path = "/var/log/fw_refresh.log"

        self.cpld_ver_info = [
                {"name":"CPU_MODULE_CPLD", "gettype":"lpc"},
                {"name":["CPU_BOARD_CPLD"], "attrs_name":["BASE_CPLD"],
                         "gettype":"restful", "url": "/".join([self.BMC_REQ_BASE_URI, "firmware/cpldversion"])},
                {"name":"PORT_BOARD_CPLD", "gettype":"i2c", "bus":5, "devno":0x30},
        ]
        self.bmc_pwd_path = "/usr/local/etc/bmcpwd"
        self.ME_RECOVERY_STATE_CODE = "02"
        self.ME_OPERATIONAL_STATE_CODE = "05"
        self.cpld_type_dict = {"A":["CPU_MODULE_CPLD", "CPU_BOARD_CPLD", "PORT_BOARD_CPLD"],
                               "B":["CPU_MODULE_CPLD", "CPU_BOARD_CPLD2", "PORT_BOARD_CPLD"]}
        self.device_cpld_type = ["A", "B"]

    def __pci_read(self, pcibus, slot,  fn , bar, offset):
        if offset % 4 != 0:
            return None
        filename = "/sys/bus/pci/devices/0000:%02x:%02x.%x/resource%d" % (int(pcibus), int(slot), int(fn), int(bar))
        file = open(filename, "r+")
        size = os.path.getsize(filename)
        data = mmap.mmap(file.fileno(), size)
        result = data[offset: offset + 4]
        s = result[::-1].decode()
        val = 0
        for i in range(0, len(s)):
            val = val << 8  | ord(s[i])
        data.close()
        return val

    def __lpc_cpld_rd(self, reg_addr):
        try:
            regaddr = 0
            if type(reg_addr) == int:
                regaddr = reg_addr
            else:
                regaddr = int(reg_addr, 16)
            devfile = "/dev/lpc_cpld"
            fd = os.open(devfile, os.O_RDWR|os.O_CREAT)
            os.lseek(fd, regaddr, os.SEEK_SET)
            str = os.read(fd, 1)
            return ord(str)
        except ValueError:
            return None
        except Exception as e:
            print(e)
            return None
        finally:
            os.close(fd)
        return None

    def __strtoint(self, str):  # 16 to int ex:"4040"/"0x4040"/"0X4040" = 16448
        value = 0
        rest_v = str.replace("0X", "").replace("0x", "")
        for index in range(len(rest_v)):
            value |= int(rest_v[index], 16) << ((len(rest_v) - index - 1) * 4)
        return value

    def __get_i2c_value(self, bus, devno, address):
        command_line = "i2cget -f -y %d 0x%02x 0x%02x " % (bus, devno, address)
        retrytime = 6
        for i in range(retrytime):
            ret, ret_t = subprocess.getstatusoutput(command_line)
            if ret == 0:
                return True, ret_t
            else:
                print("%d", ret_t)
            time.sleep(0.1)
        return False, 0

    def __retry_cmd(self, cmd, retry=1):
        status = 0
        output = ""
        retry_count = retry
        while(retry_count > 0):
            retry_count -= 1
            status, output = subprocess.getstatusoutput(cmd)
            if status == 0:
                break
            time.sleep(0.1)
        return status, output

    def __wait_me_stable(self):
        for _try in range(30):
            me_status = subprocess.getoutput("setpci -s 00:16.0 0x41.b")
            if len(me_status) != 2:
                print("failed to get me status,{}".format(me_status))
                return
            if int(me_status,16) & 0x02:
                break
            time.sleep(0.3)
        else:
            print("me status is not stable")

    def __get_me_state(self):
        # cmd_list order cannot be adjusted
        self.__wait_me_stable()
        cmd_list = ["dfd_debug io_wr 0xb3 0x4","dfd_debug io_wr 0xb2 0x3e","dfd_debug io_rd 0xb3 1 | grep '0x000000b0' | awk '{print $2}'"]
        for index, cmd in enumerate(cmd_list):
            ret, log = subprocess.getstatusoutput(cmd)
            if ret != 0:
                break
            if index == 2:
                return True,log
        return False, "NA"

    def __set_me_recovery(self):
        self.__wait_me_stable()
        # cmd_list order cannot be adjusted
        cmd_list = ["dfd_debug io_wr 0xb3 0x1","dfd_debug io_wr 0xb2 0x3e"]
        for index, cmd in enumerate(cmd_list):
            ret, log = subprocess.getstatusoutput(cmd)
            self.__wait_me_stable()
            if ret != 0:
                return False
        return True

    def __reset_me(self):
        self.__wait_me_stable()
        # cmd_list order cannot be adjusted
        cmd_list = ["dfd_debug io_wr 0xb3 0x2","dfd_debug io_wr 0xb2 0x3e"]
        for index, cmd in enumerate(cmd_list):
            ret, log = subprocess.getstatusoutput(cmd)
            self.__wait_me_stable()
            if ret != 0:
                return False
        return True

    def __update_fw_upgrade_logger(self, header, message):
        if not os.path.isfile(self.fw_upgrade_logger_path):
            cmd = "sudo touch %s && sudo chmod +x %s" % (
                self.fw_upgrade_logger_path, self.fw_upgrade_logger_path)
            subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        logging.basicConfig(filename=self.fw_upgrade_logger_path,
                            filemode='a',
                            format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                            datefmt='%H:%M:%S',
                            level=logging.INFO)

        log_message = "%s : %s" % (header, message)
        if header != "last_upgrade_result":
            print(log_message)
        return logging.info(log_message)

    def __firmware_program_bmc(self, fw_path, fw_extra=None):
        last_fw_upgrade = ["BMC", str(fw_path), str(fw_extra), "FAILED"]
        self.__update_fw_upgrade_logger(
            "bmc_upgrade", "start BMC upgrade")

        # Determine which flash to upgrade
        fw_extra_str = str(fw_extra).lower()
        flash_list = ["master", "slave", "both", "pingpong"]
        if fw_extra_str not in flash_list:
            print("BMC flash should be master/slave/both/pingpong")
            self.__update_fw_upgrade_logger(
                "bmc_upgrade", "fail, message=BMC flash should be master/slave/both/pingpong %s" % fw_extra_str)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        current_bmc = self.get_bmc_flash()
        if current_bmc not in ["master","slave"]:
            print("Fail, get current BMC flash is %s" % current_bmc)
            self.__update_fw_upgrade_logger(
                "bmc_upgrade", "fail, message=Get current BMC flash is %s" % current_bmc)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        if fw_extra_str == "pingpong":
            flash = "slave" if current_bmc == "master" else "master"
            fw_extra_str = flash

        # Copy BMC image file to BMC
        print("BMC Upgrade")
        print("Uploading image to BMC...")
        if not self.upload_to_bmc(fw_path):
            self.__update_fw_upgrade_logger(
                "bmc_upgrade", "fail, message=BMC Unable to upload BMC image to BMC %s" % fw_path)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        print("Upload bmc image %s to BMC done" % fw_path)

        # Fill json param, "Name", "Path", "Flash"
        image_name = os.path.basename(fw_path)
        json_data = {}
        json_data["Name"] = "bmc"
        json_data["Path"] = "/tmp/%s" % image_name
        json_data["Flash"] = fw_extra_str

        # Send the upgrade request BMC
        if not self.post_to_bmc(self.fw_upgrade_uri, json_data):
            print("Failed to upgrade BMC %s flash" % fw_extra_str)
            self.__update_fw_upgrade_logger(
                "bmc_upgrade", "fail, message=Failed to upgrade BMC %s flash" % fw_extra_str)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            self.rm_file_upload_to_bmc(fw_path)
            return False

        print("Upgrade BMC %s full process done" % fw_extra_str)
        self.__update_fw_upgrade_logger("bmc_upgrade", "done")
        last_fw_upgrade[3] = "DONE"
        self.__update_fw_upgrade_logger(
            "last_upgrade_result", str(last_fw_upgrade))

        self.rm_file_upload_to_bmc(fw_path)

        return True

    def __firmware_program_fpga(self, fw_path, fw_extra=None):
        last_fw_upgrade = ["FPGA", str(fw_path), None, "FAILED"]
        self.__update_fw_upgrade_logger(
            "fpga_upgrade", "start FPGA upgrade")

        if not os.path.isfile(fw_path):
            self.__update_fw_upgrade_logger(
                "fpga_upgrade", "fail, message=FPGA image not found %s" % fw_path)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False

        command = 'firmware_upgrade ' + fw_path +' fpga 0 fpga'
        print("Running command: %s" % command)
        process = subprocess.Popen(
            command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        while True:
            output = process.stdout.readline().decode()
            if output == '' and process.poll() is not None:
                break

        rc = process.returncode
        if rc != 0:
            self.__update_fw_upgrade_logger(
                "fw_upgrade", "fail, message=Unable to install FPGA")
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False

        self.__update_fw_upgrade_logger("fpga_upgrade", "done")
        last_fw_upgrade[3] = "DONE"
        self.__update_fw_upgrade_logger(
            "last_upgrade_result", str(last_fw_upgrade))
        return True

    def __switch_jtag_channel(self,switch):
        if switch in "open":
            wr_byte = "0x01"
        else:
            wr_byte = "0x00"
        cmd = "dfd_debug io_wr 0x90e %s" %wr_byte
        ret, ret_t = self.__retry_cmd(cmd,3)
        if ret == 0 and "success" in ret_t:
            # check JTAG channel
            cmd = "dfd_debug io_rd 0x90e 1"
            ret, ret_t = self.__retry_cmd(cmd,3)
            if ret != 0 or ret_t.split()[-1] not in wr_byte:
                self.__update_fw_upgrade_logger(
                    "cpld_upgrade", "fail, JTAG channel %s fail, ret:%d, ret_t:%s" %(switch,ret,ret_t))
                return False
        else:
            self.__update_fw_upgrade_logger(
                "cpld_upgrade", "fail, set JTAG channel %s fail, ret:%d, ret_t:%s" %(switch,ret,ret_t))
            return False
        return True

    def __firmware_program_cpld(self, fw_path, fw_extra=None):
        last_fw_upgrade = ["CPLD", str(fw_path), str(fw_extra), "FAILED"]
        self.__update_fw_upgrade_logger(
            "cpld_upgrade", "start CPLD upgrade")

        # Check input
        fw_extra_str = str(fw_extra).upper()
        if ":" in fw_path and ":" in fw_extra_str:
            fw_path_list = fw_path.split(":")
            fw_extra_str_list = fw_extra_str.split(":")
        else:
            fw_path_list = [fw_path]
            fw_extra_str_list = [fw_extra_str]

        if len(fw_path_list) != len(fw_extra_str_list):
            self.__update_fw_upgrade_logger(
                "cpld_upgrade", "fail, message=Invalid input")
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        # nonsupport to set fan full speed
        # nonsupport to restore fan full speed

        device_cpld_type = self.get_device_cpld_type()
        cpld_result_list = []
        data_list = list(zip(fw_path_list, fw_extra_str_list))
        for data in data_list:
            fw_path = data[0]
            fw_extra_str = data[1]

            if fw_extra_str not in self.cpld_type_dict[device_cpld_type]:
                self.__update_fw_upgrade_logger(
                        "cpld_upgrade", "current hardware is not support %s, skip this type program" % fw_extra_str)
                cpld_result_list.append("SKIP")
                continue

            # Set fw_extra
            fw_extra_str = {
                "CPU_BOARD_CPLD": "base_cpld",
                "CPU_BOARD_CPLD2": "base_cpld",
                "CPU_MODULE_CPLD": "cpu_cpld",
                "PORT_BOARD_CPLD": "port_cpld",
            }.get(fw_extra_str, None)

            self.__update_fw_upgrade_logger(
                "cpld_upgrade", "start %s upgrade" % data[1])
            upgrade_result = "FAILED"
            for x in range(1, 4):
                # Set fw_extra
                if x > 1:
                    self.__update_fw_upgrade_logger(
                        "cpld_upgrade", "fail, message=Retry to upgrade %s" % data[1])

                elif fw_extra_str is None:
                    self.__update_fw_upgrade_logger(
                        "cpld_upgrade", "fail, message=Invalid extra information string %s" % data[1])
                    break
                elif not os.path.isfile(os.path.abspath(fw_path)):
                    self.__update_fw_upgrade_logger(
                        "cpld_upgrade", "fail, message=CPLD image not found %s" % fw_path)
                    break

                # Install cpld image via ispvm tool
                channel = {
                    "cpu_cpld":0,
                    "base_cpld":1,
                    "port_cpld":1,
                }.get(fw_extra_str,0)

                if "port_cpld" in fw_extra_str:
                    ret = self.__switch_jtag_channel("open")
                else:
                    ret = self.__switch_jtag_channel("close")
                if ret == False:
                    continue

                print("Installing...")
                command = 'firmware_upgrade %s cpld %d cpld' % (fw_path,channel)
                print("Running command : %s" % command)
                process = subprocess.Popen(
                    command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                while True:
                    output = process.stdout.readline().decode()
                    if output == '' and process.poll() is not None:
                        break

                if "port_cpld" in fw_extra_str:
                    ret = self.__switch_jtag_channel("close")
                    if ret == False:
                        continue

                rc = process.returncode
                if rc != 0:
                    self.__update_fw_upgrade_logger(
                        "cpld_upgrade", "fail, message=Unable to install CPLD")
                    continue

                upgrade_result = "DONE"
                self.__update_fw_upgrade_logger("cpld_upgrade", "done")
                break
            cpld_result_list.append(upgrade_result)

        last_fw_upgrade = ["CPLD", ":".join(
            fw_path_list), ":".join(fw_extra_str_list), ":".join(cpld_result_list)]
        self.__update_fw_upgrade_logger(
            "last_upgrade_result", str(last_fw_upgrade))
        return "FAILED" not in cpld_result_list

    def __firmware_program_bios(self, fw_path, fw_extra=None):
        last_fw_upgrade = ["BIOS", str(fw_path), str(fw_extra), "FAILED"]
        self.__update_fw_upgrade_logger(
            "bios_upgrade", "start BIOS upgrade")
        print("BIOS Upgrade")
        # 10 minutes for snoic to boot up before upgrading bios
        cmd = "awk '{printf \"%.1f\", $0/60;}' /proc/uptime | awk -F '.' '{print $1}'"
        p = subprocess.Popen(
             cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, err = p.communicate()
        output = output.decode()
        uptime_str = output.strip('\n')
        if not self.is_support_me_recovery() and int(uptime_str) <= 10:
            msg = "BIOS upgrade must be done 10 minutes after os startup, current os startup time: %sm" % uptime_str
            print(msg)
            self.__update_fw_upgrade_logger(
                "bios_upgrade", "fail, message=%s" % msg)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False

        if not self.set_me_recovery():
            print("Set me recovery Failed")
            return False

        fw_extra_str = str(fw_extra).lower()
        flash_list = ["master", "slave", "both"]
        if fw_extra_str not in flash_list:
            print("BIOS flash should be master/slave/both")
            self.__update_fw_upgrade_logger(
                "bios_upgrade", "fail, message=BIOS flash should be master/slave/both %s" % fw_extra_str)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        flash = fw_extra_str

        print("Uploading BIOS image %s to BMC..." % fw_path)
        if not self.upload_to_bmc(fw_path):
            print("Failed to upload %s to bmc" % fw_path)
            self.__update_fw_upgrade_logger(
                "bios_upgrade", "Failed to upload %s to bmc" % fw_path)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            return False
        print("Upload BIOS image %s to BMC done" % fw_path)

        image_name = os.path.basename(fw_path)
        json_data = {}
        json_data["Name"] = "bios"
        json_data["Path"] = "/tmp/%s" % image_name
        json_data["Flash"] = flash

        if not self.post_to_bmc(self.fw_upgrade_uri, json_data):
            print("Failed to upgrade %s BIOS" % flash)
            self.__update_fw_upgrade_logger(
                "bios_upgrade", "Failed to upgrade %s BIOS" % flash)
            self.__update_fw_upgrade_logger(
                "last_upgrade_result", str(last_fw_upgrade))
            self.rm_file_upload_to_bmc(fw_path)
            return False

        status,log = subprocess.getstatusoutput("baudrate.sh -S")
        if status:
            print(log)

        if not self.reset_me():
            print("Reset me Failed")

        print("Upgrade BIOS %s full process done" % fw_extra_str)
        self.__update_fw_upgrade_logger("bios_upgrade", "done")
        last_fw_upgrade[3] = "DONE"
        self.__update_fw_upgrade_logger(
            "last_upgrade_result", str(last_fw_upgrade))

        self.rm_file_upload_to_bmc(fw_path)

        return True

    def set_me_recovery(self):
        if not self.is_support_me_recovery():
            return True

        ret, me_state = self.__get_me_state()
        if ret == False:
            self.__update_fw_upgrade_logger("bios_upgrade", "Get me state failed")
            return False

        if self.ME_OPERATIONAL_STATE_CODE in me_state:
            if not self.__set_me_recovery():
                self.__update_fw_upgrade_logger("bios_upgrade", "Set me recovery Failed")
                return
            time.sleep(1)
            ret, me_state = self.__get_me_state()
            if ret == False:
                self.__update_fw_upgrade_logger("bios_upgrade", "Get me state failed , after set me recovery")
                return False

        if self.ME_RECOVERY_STATE_CODE not in me_state:
            self.__update_fw_upgrade_logger("bios_upgrade", "Me state %s error" % me_state)
            return False

        return True

    def is_support_me_recovery(self):
        # Supported from 3CNHU016.bin
        cmd = "dmidecode -t bios | grep Version"
        ret, log = subprocess.getstatusoutput(cmd)
        if ret == 0:
            version = re.sub(".*CNHU(\d+).*","\\1",log)
            if "0101" not in version and int(version) >= 16:
                return True

        return False

    def reset_me(self):
        if not self.is_support_me_recovery():
            return True

        if not self.__reset_me():
            self.__update_fw_upgrade_logger("bios_upgrade", "Reset me recovery Failed")
            return False

        retrytime = 10
        while retrytime > 0:
            ret, me_state = self.__get_me_state()
            if ret == True and self.ME_OPERATIONAL_STATE_CODE in me_state:
                return True
            retrytime -= 1
            time.sleep(1)
        self.__update_fw_upgrade_logger("bios_upgrade", "Get me state(%s %s) failed , after reset me recovery" % (ret, me_state))
        return False

    def get_device_cpld_type(self):
        # board cpld B matching CODEID:0x612bd043
        _, log = subprocess.getstatusoutput("firmware_upgrade cpld getidcode 1")
        if "0x612bd043" in log:
            return self.device_cpld_type[1]

        return self.device_cpld_type[0]

    def get_bmc_pass(self):
        platformpath = os.path.abspath(os.path.dirname(__file__))
        rjconf_filename = platformpath + "/rj.conf"
        if not os.path.isfile(rjconf_filename):
            return None
        with open(rjconf_filename) as rjconf_file:
            for line in rjconf_file:
                tokens = line.split('=')
                if len(tokens) < 2:
                    continue
                if tokens[0] == "OPENBMC_PASSWORD":
                    return tokens[1].strip()
        return None

    def get_from_bmc(self, uri, maytimeout=30):
        resp = None
        for retry in range(3):
            try:
                resp = requests.get(uri, timeout=maytimeout)
                break
            except Exception as e:
                bmc_restful_logger("get_from_bmc:%s,Exception:%s" %(uri,str(e)))

        if not resp:
            return None

        data = resp.json()
        if not data or "data" not in data or "status" not in data:
            return None

        if data["status"] != "OK":
            return None

        return data["data"]

    def get_bmc_version(self):
        bmc_ver = "N/A"
        data = self.get_from_bmc(self.bmc_info_uri)
        if not data or "Version" not in data:
            return bmc_ver

        return data["Version"]

    def get_bmc_flash(self):
        flash = "N/A"
        data = self.get_from_bmc(self.bmc_info_uri)
        if not data or "Flash" not in data:
            return flash

        return data["Flash"]

    def post_to_bmc(self, uri, data, resp_required=True):
        resp = None
        for retry in range(3):
            try:
                if resp_required:
                    tmout = (30, 900)
                else:
                    tmout = (30, 30)
                resp = requests.post(uri, json=data, timeout=tmout)
                break
            except Exception as e:
                bmc_restful_logger("data:%s,post_to_bmc:%s,Exception:%s" %(data,uri,str(e)))
                if not resp_required:
                    if 'Connection aborted' in str(e) or 'Read timed out' in str(e):
                        return True
                if 'ConnectTimeoutError' in str(e):
                    continue
                return False

        if not resp_required:
            return True
        elif not resp:
            print("No response")
            return False

        data = resp.json()
        if "status" not in data:
            print("status not in data")
            return False

        if data["status"] != "OK":
            print("status <%s> is not in OK" % data["status"])
            return False

        return True

    def upload_to_bmc_scp(self, fw_path):
        scp_command = 'sudo scp -o StrictHostKeyChecking=no -o ' \
                      'UserKnownHostsFile=/dev/null -r %s root@240.1.1.1:/tmp/' \
                      % os.path.abspath(fw_path)
        print(scp_command)
        child = pexpect.spawn(scp_command, timeout=120)
        expect_list = [pexpect.EOF, pexpect.TIMEOUT, "'s password:", "No route to host"]
        i = child.expect(expect_list, timeout=120)
        bmc_pwd = self.get_bmc_pass()
        if i == 2 and bmc_pwd != None:
            child.sendline(bmc_pwd)
            data = child.read().decode()
            print(data)
            child.close()
            return os.path.isfile(fw_path)
        elif i == 0:
            return True
        else:
            print("Failed to scp %s to BMC, index %d" % (fw_path, i))

        return False

    def upload_to_bmc(self, fw_path):
        ret = False
        trytime = 3
        for m in range(trytime):
            try:
                ret = self.upload_to_bmc_scp(fw_path)
            except Exception as e:
                print(e)
                ret = False
                time.sleep(5)
            if (ret == True):
                break
            self.__update_fw_upgrade_logger(
                "fw_upgrade", "fail, message=scp timeout ")
        return ret

    def rm_file_for_bmc_scp(self, fw_path):
        image_name = os.path.basename(fw_path)
        image_path_bmc = "/tmp/%s" % image_name
        scp_command = 'sudo ssh -o StrictHostKeyChecking=no -o ' \
                      'UserKnownHostsFile=/dev/null root@240.1.1.1 /bin/rm %s' \
                      % image_path_bmc
        print(scp_command)
        child = pexpect.spawn(scp_command, timeout=120)
        expect_list = [pexpect.EOF, pexpect.TIMEOUT, "'s password:", "No route to host"]
        i = child.expect(expect_list, timeout=120)
        bmc_pwd = self.get_bmc_pass()
        if i == 2 and bmc_pwd != None:
            child.sendline(bmc_pwd)
            data = child.read().decode()
            print(data)
            child.close()
            return os.path.isfile(fw_path)
        elif i == 0:
            return True
        else:
            print("Failed to rm %s on BMC, index %d" % (image_path_bmc, i))

        return False

    def rm_file_upload_to_bmc(self,fw_path):
        if not self.rm_file_for_bmc_scp(fw_path):
            print("Failed to rm /tmp/%s on BMC" % os.path.basename(fw_path))
            return False
        else:
            print("rm /tmp/%s on BMC done" % os.path.basename(fw_path))
            return True

    def get_cpld_version(self):
        result = {}
        for cpld in self.cpld_ver_info:
            gettype = cpld.get("gettype",None)
            bus = cpld.get("bus",None)
            devno = cpld.get("devno",None)
            url = cpld.get("url",None)
            data = [0, 0, 0, 0]
            t = True
            ret = None
            if gettype == "lpc":
                for i in range(4):
                    ret = self.__lpc_cpld_rd(i)
                    if ret == None:
                        t = False
                        break
                    data[i] = ret
            elif gettype == "restful":
                data = self.get_from_bmc(url)
                if not data:
                    continue
                names = cpld.get('name', None)
                attrs_names = cpld.get('attrs_name', None)
                for i in range(len(attrs_names)):
                    result[names[i]] = str(data[attrs_names[i]]).strip()
                continue
            elif gettype == "i2c":
                for i in range(4):
                    ind, ret = self.__get_i2c_value(bus, devno, i)
                    if ind == False:
                        t = False
                        break
                    data[i] = self.__strtoint(ret)
            else:
                continue
            result[cpld.get('name', " ")] = "%02x%02x%02x%02x" %(data[1] ,data[2] ,data[3], data[0])
        return result

    def get_bios_version(self):
        bios_version = None

        p = subprocess.Popen(
            ["sudo", "dmidecode", "-s", "bios-version"], stdout=subprocess.PIPE)
        raw_data = p.communicate()[0].decode()
        if raw_data == '':
            return str(None)
        raw_data_list = raw_data.split("\n")
        bios_version = raw_data_list[0] if len(
            raw_data_list) == 1 else raw_data_list[-2]

        return str(bios_version)

    def get_onie_version(self):
        if not os.path.isfile(self.ONIE_CFG_FILE):
            return None
        machine_vars = {}
        with open(self.ONIE_CFG_FILE) as machine_file:
            for line in machine_file:
                tokens = line.split('=')
                if len(tokens) < 2:
                    continue
                machine_vars[tokens[0]] = tokens[1].strip()
        return  "%s_%s" % (machine_vars.get("onie_version"),machine_vars.get("onie_build_date"))

    def get_pcie_version(self):
        pcie_version = dict()
        pcie_version["PCIE_FW_LOADER"] = 'None'
        pcie_version["PCIE_FW"] = 'None'
        return pcie_version


    def get_fpga_version(self):
        version = self.__pci_read(1,0,0,0,0)
        datetime = self.__pci_read(1,0,0,0,4)
        return "%08x-%08x"%(version,datetime)

    def get_fpga_flash(self):
        version1 = self.__pci_read(1,0,0,0,0)
        version2 = self.__pci_read(1,0,0,0,12)
        if version1 == version2:
            return 'golden'
        else:
            return 'primary'

    def get_cpld_flash(self):
        ret, log = subprocess.getstatusoutput("hw_test.bin lpc_cpld_rd8 0x00010006")
        log = re.sub("^\s*|\s*$","",log)
        if ret == 0 and log in ["01","02"]:
            if log == "01":
                return "on-chip"
            if log == "02":
                return "off-chip"
        else:
            return "can't get cpld info"


    def upgrade_logger(self, upgrade_list):
        try:
            with open(self.fw_upgrade_logger_path, 'w') as filetowrite:
                json.dump(upgrade_list, filetowrite)
        except Exception as e:
            pass

    # Get booting flash of running BMC.
    # @return a string, "master" or "slave"
    def get_running_bmc(self):
        return self.get_bmc_flash()

    def firmware_upgrade(self, fw_type, fw_path, fw_extra=None):
        """
            @fw_type MANDATORY, firmware type, should be one of the strings: 'cpld', 'fpga', 'bios', 'bmc'
            @fw_path MANDATORY, target firmware file
            @fw_extra OPTIONAL, extra information string,

            for fw_type 'cpld' and 'fpga': it can be used to indicate specific cpld, such as 'cpld1', 'cpld2', ...
                or 'cpld_fan_come_board', etc. If None, upgrade all CPLD/FPGA firmware. for fw_type 'bios' and 'bmc',
                 value should be one of 'master' or 'slave' or 'both'
        """
        if fw_type == 'bmc':
            if not self.__firmware_program_bmc(fw_path, fw_extra):
                return False

            fw_extra_str = str(fw_extra).lower()
            flash_list = ["master", "slave", "both"]
            if fw_extra_str not in flash_list:
                if fw_extra_str != "pingpong":
                    print("BMC flash should be master/slave/both/pingpong")
                    return False

            current_bmc = self.get_bmc_flash()
            if current_bmc not in ["master","slave"]:
                print("Fail, get current BMC flash is %s" % current_bmc)
                return False

            if fw_extra_str == "pingpong":
                flash = "slave" if current_bmc == "master" else "master"
            else:
                flash = fw_extra_str

            # Change boot flash if required
            if current_bmc != flash:
                # Set desired boot flash
                print("Current BMC boot flash %s, user requested %s" % (current_bmc, flash))
                flash_boot = current_bmc if flash == "both" else flash
                json_data = {}
                json_data["Flash"] = flash_boot
                if not self.post_to_bmc(self.bmc_nextboot_uri, json_data):
                    print("Failed to set BMC next boot to %s" % flash_boot)
                    return False

            if not self.reboot_bmc():
                print("Failed to reboot BMC after upgrade")
                return False
            print("Reboot BMC after upgrade done")
            return True
        elif fw_type == 'fpga':
            return self.__firmware_program_fpga(fw_path, fw_extra)
        elif fw_type == 'bios':
            return self.__firmware_program_bios(fw_path, fw_extra)
        else:
            return False

    def get_last_upgrade_result(self):
        """
            Get last firmware upgrade information, inlcudes:
            1) FwType: cpld/fpga/bios/bmc(passed by method 'firmware_upgrade'), string
            2) FwPath: path and file name of firmware(passed by method 'firmware_upgrade'), string
            3) FwExtra: designated string, econdings of this string is determined by vendor(passed by method 'firmware_program')
            4) Result: indicates whether the upgrade action is performed and success/failure status if performed. Values should be one of: "DONE"/"FAILED"/"NOT_PERFORMED".
            list of object:
            [
                {
                    "FwType": "cpld",
                    "FwPath": "cpu_cpld.vme"
                    "FwExtra":"CPU_CPLD"
                    "Result": "DONE"
                },
                {
                    "FwType": "cpld",
                    "FwPath": "fan_cpld.vme"
                    "FwExtra": "FAN_CPLD"
                    "Result": "FAILED"
                }
            ]
        """
        last_update_list = []
        try:
            self.firmware_refresh_result_get()
        except Exception as e:
            print(e)
            return last_update_list

        if os.path.exists(self.fw_upgrade_logger_path):
            with open(self.fw_upgrade_logger_path, 'r') as file:
                lines = file.read().splitlines()

            upgrade_txt = [i for i in reversed(
                lines) if "last_upgrade_result" in i]
            if len(upgrade_txt) > 0:
                last_upgrade_txt = upgrade_txt[0].split(
                    "last_upgrade_result : ")
                last_upgrade_list = ast.literal_eval(last_upgrade_txt[1])
                for x in range(0, len(last_upgrade_list[1].split(":"))):
                    upgrade_dict = {}
                    upgrade_dict["FwType"] = last_upgrade_list[0].lower()
                    upgrade_dict["FwPath"] = last_upgrade_list[1].split(":")[x]
                    upgrade_dict["FwExtra"] = last_upgrade_list[2].split(":")[
                        x] if last_upgrade_list[2] else "None"
                    upgrade_dict["Result"] = last_upgrade_list[3].split(":")[x]
                    last_update_list.append(upgrade_dict)

        return last_update_list

    def firmware_program(self, fw_type, fw_path, fw_extra=None):
        """
            Program FPGA and/or CPLD firmware only, but do not refresh them

            @param fw_type value can be: FPGA, CPLD
            @param fw_path a string of firmware file path, seperated by ':', it should
                        match the sequence of param @fw_type
            @param fw_extra a string of firmware subtype, i.e CPU_CPLD, BOARD_CPLD,
                            FAN_CPLD, LC_CPLD, etc. Subtypes are seperated by ':'
            @return True when all required firmware is program succefully,
                    False otherwise.

            Example:
                self.firmware_program("CPLD", "/cpu_cpld.vme:/lc_cpld", \
                                    "CPU_CPLD:LC_CPLD")
                or
                self.firmware_program("FPGA", "/fpga.bin")
        """
        fw_type = fw_type.lower()
        bmc_pwd = self.get_bmc_pass()
        if not bmc_pwd and fw_type != "fpga":
            self.__update_fw_upgrade_logger(
                "fw_upgrade", "fail, message=BMC credential not found")
            return False

        if fw_type == 'fpga':
            return self.__firmware_program_fpga(fw_path, fw_extra)
        elif 'cpld' in fw_type:
            return self.__firmware_program_cpld(fw_path, fw_extra)
        elif 'bmc' in fw_type:
            return self.__firmware_program_bmc(fw_path, fw_extra)
        elif 'bios' in fw_type:
            return self.__firmware_program_bios(fw_path, fw_extra)
        else:
            self.__update_fw_upgrade_logger(
                "fw_upgrade", "fail, message=Invalid firmware type")
            return False

        return True

    def firmware_refresh_log_save(self, fw_names, fw_paths):
        if not os.path.isfile(self.fw_upgrade_logger_path):
            cmd = "sudo touch %s && sudo chmod +x %s" % (
                self.fw_upgrade_logger_path, self.fw_upgrade_logger_path)
            subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        try:
            fw_dict = {}
            for index, name in  enumerate(fw_names):
                fw_dict[name.upper()] = {'FwType' : name.upper(), 'FwExtra': 'None', 'FwPath' : fw_paths[index], 'Result' : 'None'}
            with open(self.fw_refresh_logger_path,"w") as f:
                json.dump(fw_dict, f)
            subprocess.getstatusoutput("sudo sync")
        except:
            pass

    def firmware_refresh_result_get(self):
        try:
            if not os.path.exists(self.fw_refresh_logger_path):
                return

            fw_dict = {}
            with open(self.fw_refresh_logger_path, "r") as f:
                fw_dict = json.load(f)

            data_post = {"Command": "/usr/local/bin/rj_rawcmd_cli get last_upgrade_result"}
            ret = BMCMessage().bmcpost(self.bmc_rawcmd_uri, data_post)
            if isinstance(ret, dict):
                for key,value in list(dict(ret).items()):
                    if key in fw_dict:
                        fw_dict[key]['Result'] = value
            elif ret == None:
                raise RequestException("Failed to getlastupgrade. Please check the network status and bmc restful service status or try again.")
            # u'FwExtra' --> 'FwExtra'
            fw_dict = eval(json.dumps(fw_dict))
            result_list = []
            path_list = []
            extra_list = []
            for key, value in list(dict(fw_dict).items()):
                extra_list.append(value['FwType'])
                path_list.append(value['FwPath'])
                result_list.append(value['Result'])

            last_fw_upgrade = ["fw_refresh", ":".join(path_list), ":".join(extra_list), ":".join(result_list)]
            self.__update_fw_upgrade_logger("last_upgrade_result", str(last_fw_upgrade))
        except RequestException as e:
            raise Exception("%s"%e)
        except:
            pass

        subprocess.getstatusoutput("sudo rm %s" % self.fw_refresh_logger_path)
        subprocess.getstatusoutput("sudo sync")

    def firmware_refresh(self, fpga_list, cpld_list, fw_extra=None):
        """
            Refresh firmware and take extra action when necessary.
            @param fpga_list a list of FPGA names
            @param cpld_list a list of CPLD names
            @return True if refresh succefully and no power cycle action is taken.

            @Note extra action should be: power cycle the whole system(except BMC) when
                                        CPU_CPLD or BOARD_CPLD or FPGA is refreshed.
                                        No operation if the power cycle is not needed.

            Example:
            self.firmware_refresh(
                ["FPGA"], ["BASE_CPLD", "LC_CPLD"],"/tmp/fw/refresh.vme")
            or
            self.firmware_refresh(["FPGA"], None, None)
            or
            self.firmware_refresh(None, ["FAN_CPLD", "LC1_CPLD", "BASE_CPLD"],
                                  "/tmp/fw/fan_refresh.vme:none:/tmp/fw/base_refresh.vme")
        """
        fw_names = []
        fw_files = []
        fw_paths = []
        # FPGA list may contain FPGA and BIOS
        if fpga_list:
            for name in fpga_list:
                if name == "FPGA" or name == "BIOS":
                    fw_names.append(name.lower())
                    fw_files.append("none")
                    fw_paths.append("None")
                else:
                    return False

        if cpld_list:
            for name in cpld_list:
                fw_names.append(name.lower())

        if fw_extra:
            fw_extra_fpath = fw_extra.split(":")
            for fpath in fw_extra_fpath:
                if fpath == "none":
                    continue

                fw_paths.append(fpath)
                fname = os.path.basename(fpath)
                bmc_fpath = "/tmp/%s" % fname
                fw_files.append(bmc_fpath)

                if os.path.exists(fpath) and os.path.isfile(fpath):
                    # upload refresh file to bmc
                    if not self.upload_to_bmc(fpath):
                        return False

        device_cpld_type = self.get_device_cpld_type()
        fw_extra_list = self.cpld_type_dict[device_cpld_type] + ["FPGA", "BIOS"]
        fw_list = list(zip(fw_names, fw_paths, fw_files))
        for fw_name, fw_patch, fw_file in fw_list:
            if fw_name.upper() not in fw_extra_list:
                self.__update_fw_upgrade_logger(
                        "cpld_refresh", "current hardware is not support %s, skip this type refresh" % fw_name.upper())
                fw_names.remove(fw_name)
                fw_paths.remove(fw_patch)
                fw_files.remove(fw_file)

        self.firmware_refresh_log_save(fw_names, fw_paths)

        data = {}
        data["Names"] = fw_names
        data["Paths"] = fw_files
        # j = json.dumps(data, sort_keys=True, indent=4, separators=(',', ': '))
        # print j
        if not self.post_to_bmc(self.fw_refresh_uri, data):
            print("Failed to refresh firmware")
            return False

        return True

    def set_bmc_boot_flash(self, flash):
        """
            Set booting flash of BMC
            @param flash should be "master" or "slave"
        """
        if flash.lower() not in ["master", "slave"]:
            return False
        data = {}
        data["Flash"] = flash.lower()
        if not self.post_to_bmc(self.bmc_nextboot_uri, data):
            print("Failed Set booting flash of BMC %s"%(flash.lower()))
            return False
        return True

    def reboot_bmc(self):
        """
            Reboot BMC
        """
        if not self.post_to_bmc(self.bmc_reboot_uri, {}, resp_required=False):
            return False

        return True

    def get_current_bios(self):
        """
            # Get booting bios image of current running host OS
            # @return a string, "master" or "slave"
        """
        flash = "N/A"
        data = self.get_from_bmc(self.bios_bootstatus_uri)
        if not data or "Flash" not in data:
            return flash

        return data["Flash"]

    def get_bios_next_boot(self):
        data = self.get_from_bmc(self.bios_nextboot_uri)
        if not data or "Flash" not in data:
            return "N/A"

        return data["Flash"]

    def set_bios_next_boot(self, flash):
        if flash.lower() not in ["master", "slave"]:
            return False

        json_data = {}
        json_data["Flash"] = flash.lower()
        if not self.post_to_bmc(self.bios_nextboot_uri, json_data):
            return False

        return True

    def get_bmc_servicestatus(self, service_name):
        bmc_raw_cmd = "/usr/local/bin/rj_rawcmd_cli service status %s" % service_name
        data_post = {"Command": bmc_raw_cmd}
        try:
            ret = BMCMessage().bmcpost(self.bmc_rawcmd_uri, data_post, 30)
        except Exception as e:
            print("Failed to get %s status: %s" % (service_name, str(e)))
            return False
        if ret is None or len(ret) == 0:
            print("Failed to get %s status" % service_name)
            return False
        print(ret)
        return True

    def set_bmc_servicestatus(self, action, service_name):
        bmc_raw_cmd = "/usr/local/bin/rj_rawcmd_cli service %s %s" % (action, service_name)
        data_post = {"Command": bmc_raw_cmd}
        try:
            ret = BMCMessage().bmcpost(self.bmc_rawcmd_uri, data_post, 60)
        except Exception as e:
            print("Failed to %s %s: %s" % (action, service_name, str(e)))
            return False
        if ret is None:
            print("Failed to %s %s" % (action, service_name))
            return False
        return True
