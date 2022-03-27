# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re


# CPU test class
class CPU2EXTERNAL(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "cpu2external"
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

    def net_ip_test(self, also_print_console=False):
        self.logger.log_info("[CHECK IP ADDRESS]:", also_print_console)

        ip_cmd = "ifconfig eth0 | grep inet | awk '{print $2}' | head -1"
        ip_status, ip_log = run_command(ip_cmd)
        ip_pattern = self.net_info_dict["ip_pattern"]
        if ip_status != 0 or len(ip_log) <= 0:
            self.fail_reason.append("get ip address fail.")
            ret = E.EIO
        elif not re.match(ip_pattern, ip_log):
            self.fail_reason.append("cpu ip address error.")
            ret = E.EIO
        else:
            ret = E.OK
            self.logger.log_info(
                "ip address: {} ".format(ip_log),
                also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def net_speed_test(self, also_print_console=False):
        self.logger.log_info(
            "[CHECK LINK STATUS AND SPEED]:",
            also_print_console)

        link_sta_cmd = "ifconfig eth0"
        sta_status, sta_log = run_command(link_sta_cmd)
        if sta_status != 0 or len(sta_log) <= 0:
            self.fail_reason.append("get link status fail.")
            ret = E.EIO
        else:
            ret = E.OK
            self.logger.log_info(
                "link status:\n {} ".format(sta_log),
                also_print_console)
        if ret == E.OK:
            link_speed_cmd = "ethtool eth0 | grep Speed | awk '{print $2}'"
            speed_status, speed_log = run_command(link_speed_cmd)
            if speed_status != 0 or len(speed_log) <= 0:
                self.fail_reason.append("get link speed fail.")
                ret = E.EIO
            else:
                link_speed = self.net_info_dict["link_speed"]
                if link_speed != speed_log:
                    ret = E.EMGMT11002
                    self.fail_reason.append("link speed not match.")
                else:
                    ret = E.OK
                    self.logger.log_info(
                        "link speed: {} ".format(speed_log),
                        also_print_console)
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def net_ping_test(self, also_print_console=False):
        self.logger.log_info("[CHECK PING STATUS]:", also_print_console)

        cpu_remote_ip = self.net_info_dict["cpu_remote_ip"]
        self.logger.log_info(
            "remote ip add: {} ".format(cpu_remote_ip),
            also_print_console)
        ping_cmd = "ping {} -c 10 -I eth0".format(cpu_remote_ip)
        ping_status, ping_log = run_command(ping_cmd)
        if ping_status != 0 or len(ping_log) <= 0:
            self.fail_reason.append("ping test fail.")
            ret = E.EIO
        else:
            loss_pattern = self.net_info_dict["loss_pattern"]
            if not re.search(loss_pattern, ping_log):
                ret = E.EMGMT11002
                self.fail_reason.append("ping test loss packet.")
            else:
                ret = E.OK
                self.logger.log_info("ping test ok ", also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        ret = self.net_ip_test(True)
        if ret != E.OK:
            return ret
        ret = self.net_speed_test(True)
        if ret != E.OK:
            return ret
        ret = self.net_ping_test(True)
        if ret != E.OK:
            return ret
        return ret
