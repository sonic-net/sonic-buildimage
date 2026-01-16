# -*- coding:utf-8
import pit_util_common
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re


# BMC test class
class BMC2CPU(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "bmc2cpu"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.net_info_dict = None
        try:
            if self.platform_cfg_json and "net_info" in self.platform_cfg_json.keys():
                self.net_info_dict = self.platform_cfg_json["net_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)
        self.bmc_usb0_ip = self.net_info_dict["bmc_usb0_ip"]
        self.bmc_username = self.net_info_dict["bmc_username"]
        self.bmc_password = self.net_info_dict["bmc_password"]
        # self.SshBmc = pit_util_common.remote(self.logger, self.fail_reason,
        #     self.bmc_username, self.bmc_usb0_ip, self.bmc_password)

    def check_cpu_ip_test(self, also_print_console=False):
        self.logger.log_info("[CHECK IP ADDRESS]:", also_print_console)

        ip_cmd = "ifconfig eth0 | grep inet | awk '{print $2}' | head -1"
        ip_status, ip_log = run_command(ip_cmd)
        ip_pattern = self.net_info_dict["ip_pattern"]
        ip_result = re.findall(ip_pattern, ip_log)[0]
        if ip_status != 0 or len(ip_log) <= 0:
            self.fail_reason.append("get cpu ip address fail.")
            ret = E.EIO
        elif ip_result != ip_log:
            self.fail_reason.append("cpu ip address error.")
            ret = E.EIO
        else:
            ret = E.OK
            self.cpu_eth0_ip = ip_result
            self.logger.log_info(
                "cpu ip address: {} ".format(ip_result),
                also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret

    def check_bmc_ip_test(self, also_print_console=False):
        self.logger.log_info("[CHECK LINK STATUS]:", also_print_console)
        run_command(
            "ssh-keygen -f \"/root/.ssh/known_hosts\" -R {}".format(
                self.net_info_dict["bmc_usb0_ip"]))
        bmc_ip_cmd = "ifconfig eth0 | grep inet | awk '{print $2}' | awk -F ':' '{print $2}'| head -1"
        # self.SshBmc.connect(also_print_console)
        ret, bmc_ip_log = pit_util_common.ssh_command(self,bmc_ip_cmd,self.bmc_username, self.bmc_usb0_ip, self.bmc_password)
        if ret == E.OK:
            ip_pattern = self.net_info_dict["ip_pattern"]
            ip_result = re.findall(ip_pattern, bmc_ip_log.strip())[0]
            if len(bmc_ip_log) <= 0:
                self.fail_reason.append("get bmc ip address fail.")
                ret = E.EIO
            elif ip_result != bmc_ip_log.strip():
                self.fail_reason.append("cpu ip address error.")
                ret = E.EIO
            else:
                ret = E.OK
                self.logger.log_info(
                    "bmc ip address: {} ".format(ip_result),
                    also_print_console)
        if ret != E.OK:
            self.fail_reason.append("get link status fail.")
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        # self.SshBmc.disconnect()
        return ret

    def bmc_ping_cpu_test(self, also_print_console=False):
        self.logger.log_info("[CHECK PING STATUS]:", also_print_console)

        cpu_eth0_ip = self.cpu_eth0_ip
        self.logger.log_info(
            "cpu ip add: {} ".format(cpu_eth0_ip),
            also_print_console)
        # self.SshBmc.connect()
        ping_cmd = "ping {} -c 10 -I eth0".format(cpu_eth0_ip)
        ret, ping_log = pit_util_common.ssh_command(self,ping_cmd,self.bmc_username, self.bmc_usb0_ip, self.bmc_password,time=15)
        if ret == E.OK:
            if len(ping_log) <= 0:
                self.fail_reason.append("bmc ping cpu test fail.")
                ret = E.EIO
            else:
                ret = E.OK
                loss_pattern = self.net_info_dict["loss_pattern"]
                if not re.search(loss_pattern, ping_log):
                    ret = E.EMGMT11002
                    self.fail_reason.append("ping test loss packet.")
                else:
                    self.logger.log_info("ping test ok ", also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        # self.SshBmc.disconnect()
        return ret

    def run_test(self, *argv):
        ret = self.check_cpu_ip_test(True)
        if ret != E.OK:
            return ret
        ret = self.check_bmc_ip_test(True)
        if ret != E.OK:
            return ret
        ret = self.bmc_ping_cpu_test(True)
        if ret != E.OK:
            return ret
        return ret
