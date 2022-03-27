# -*- coding:utf-8 -*-
import re
import pit_util_common

from errcode import E
from test_case import TestCaseCommon
from pit_util_common import run_command, isIpV4AddrLegal


class NETSTRESSTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "netstress_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.netstress_info_dict = None
        try:
            if self.platform_cfg_json and "netstress_info" in self.platform_cfg_json.keys():
                self.netstress_info_dict = self.platform_cfg_json["netstress_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def ping_test(self, ip):
        ping_cmd = "ping {} -c 10".format(ip)
        waiting = pit_util_common.waiting(self.logger, "ping testing...")
        ping_status, ping_log = run_command(ping_cmd)
        waiting.stop("ping test stop!")
        if " 0% packet loss" not in ping_log:
            return False
        else:
            return True

    def show_cpu_ip(self):
        ip_cmd = "ifconfig eth0 | grep inet | awk '{print $2}' | head -1"
        ip_statu, ip_log = run_command(ip_cmd)
        if isIpV4AddrLegal(ip_log):
            return True, ip_log
        else:
            return False, ip_log

    def get_pid_and_remove(self):
        pid_stau, pid_out = run_command(
            "netstat -tunlp | grep iperf | awk '{print $7}'")
        if pid_stau != 0 or len(pid_out) <= 0:
            return False
        else:
            iperf_pid = pid_out.split("/")[0]
            run_command("kill -9 {}".format(iperf_pid))
            return True

    def netstress_test(self, also_print_console=False):
        env_list = self.netstress_info_dict["environment"]
        for env in env_list:
            self.logger.log_info(
                "[CPU TO {} NET STRESS TEST]:".format(
                    str(env).upper()), also_print_console)
            stu, cpu_ip = self.show_cpu_ip()
            if not stu:
                ret = E.ENETSTRESS31001
                self.fail_reason.append("cpu ip not legal.")
                self.logger.log_err("{}".format(cpu_ip), also_print_console)
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            env_ip = self.netstress_info_dict["{}_ip".format(env)]
            env_name = self.netstress_info_dict["{}_name".format(env)]
            env_password = self.netstress_info_dict["{}_password".format(env)]
            self.logger.log_info(
                "cpu ip: {}".format(cpu_ip),
                also_print_console)
            self.logger.log_info(
                "{} ip: {}".format(
                    env, env_ip), also_print_console)
            if self.ping_test(env_ip):
                self.logger.log_info(
                    "ping {} ip success!".format(env),
                    also_print_console)
            else:
                ret = E.ENETSTRESS31002
                self.fail_reason.append(
                    "cpu to {} ip ping test loss packet.".format(env))
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            run_command("iperf -s -D -i 1 -p 3389")
            if "bmc" in env:
                run_command(
                    "ssh-keygen -f \"/root/.ssh/known_hosts\" -R {}".format(
                        self.netstress_info_dict["bmc_ip"]))
            remote = pit_util_common.remote(
                self.logger, self.fail_reason, env_name, env_ip, env_password)
            ret = remote.connect(also_print_console)
            if ret != E.OK:
                return ret
            waiting = pit_util_common.waiting(
                self.logger, "netstress testing...")
            output = remote.command(
                "iperf -c {} -p 3389 -i 1 -t {}".format(
                    cpu_ip, self.netstress_info_dict["duration_value"]), int(
                    self.netstress_info_dict["net_stress_timeout"]))
            waiting.stop("netstress test stop!")
            band = re.findall("\\s([0-9]+.[0-9]+)\\sMbits/sec", output)
            if band == []:
                ret = E.ENETSTRESS31003
                self.fail_reason.append("iperf tool not in {}".format(env))
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            else:
                min_speed = min(map(float, band))
                self.logger.log_info(
                    "iperf min speed: {}Mbits/sec".format(min_speed),
                    also_print_console)
                if min_speed < self.netstress_info_dict["net_standard_speed"]:
                    ret = E.ENETSTRESS31004
                    self.fail_reason.append(
                        "cpu to {} ip iperf test loss packet.".format(env))
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
            remote.disconnect()
            if not self.get_pid_and_remove():
                self.fail_reason.append("get iperf pid error")
                ret = E.EIO
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            else:
                ret = E.OK
        self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        for i in range(self.netstress_info_dict["net_stress_count"]):
            ret = self.netstress_test(True)
            if ret != E.OK:
                return ret
        return ret
