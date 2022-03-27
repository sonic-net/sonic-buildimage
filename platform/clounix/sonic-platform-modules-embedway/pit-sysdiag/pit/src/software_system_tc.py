# coding:utf-8
import sys
import re
from function import run_command
from errcode import *
from test_case import TestCaseCommon
from sysdiag import VERSION

SYSDIAG_VERSION_DESC = "sysdiag version"
SONIC_VERSION_DESC = "SONiC Software Version:"


def find_spec(spec, subject):
    match = re.search(spec + r"\s(.*)", subject)
    if match:
        result = match.group(1)
        return True, result
    else:
        result = ""
        return False, result


class SOFTWARESYSTEMTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "software_system_tc"
        TestCaseCommon.__init__(
            self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file
        )
        try:
            if (
                self.platform_cfg_json
                and "software_system" in self.platform_cfg_json.keys()
            ):
                self.software_system_cfg = self.platform_cfg_json["software_system"]

        except Exception as e:
            self.logger.log_err(str(e), True)

    def check_sysdiag_version(self, also_print_console=True):
        ret = E.OK
        if "sysdiag_version" not in self.software_system_cfg:
            return E.EFAIL
        sysdiag_version = self.software_system_cfg["sysdiag_version"]
        self.logger.log_info("[SYSDIAG VERSION CHECK]:", also_print_console)

        # cmd = "python3 ./sysdiag.py -v" if sys.version > '3' else "python2 ./sysdiag.py -v"
        # code, out = run_command(cmd)
        # if code:
        #     ret = E.EFAIL
        #     self.fail_reason.append(out)
        #     return ret

        # code,VERSION = find_spec(SYSDIAG_VERSION_DESC,out)
        # if code and VERSION == sysdiag_version:
        if sysdiag_version == str(VERSION):
            self.logger.log_info("sysdiag version check success.", also_print_console)
        else:
            self.logger.log_err(
                "sysdiag version check failed. local version is %s , check version is %s"
                % (VERSION, sysdiag_version),
                also_print_console,
            )
            self.fail_reason.append("sysdiag version check failed")
            ret = E.EFAIL
        return ret

    def check_system_timezone(self, also_print_console=True):
        ret = E.OK
        if "timezone" not in self.software_system_cfg:
            return E.EFAIL
        timezone = self.software_system_cfg["timezone"]
        self.logger.log_info("[SYSTEM TIMEZONE CHECK]:", also_print_console)
        cmd = "cat /etc/timezone"
        code, out = run_command(cmd)
        if code:
            ret = E.EFAIL
            self.fail_reason.append(out)
            return ret
        if out == timezone:
            self.logger.log_info("system timezone check success.", also_print_console)
        else:
            self.logger.log_err(
                "system timezone check failed. local timezone is %s , check timezone is %s"
                % (out, timezone),
                also_print_console,
            )
            self.fail_reason.append("system timezone check failed")
            ret = E.EFAIL
        return ret

    def check_sonic_version(self, also_print_console=True):
        ret = E.OK
        if "sonic_version" not in self.software_system_cfg:
            return E.EFAIL
        sonic_version = self.software_system_cfg["sonic_version"]
        self.logger.log_info("[SONIC VERSION CHECK]:", also_print_console)

        cmd = "show version"
        code, out = run_command(cmd)
        if code:
            ret = E.EFAIL
            self.fail_reason.append(out)
            return ret

        code, result = find_spec(SONIC_VERSION_DESC, out)
        if code and sonic_version in result:
            self.logger.log_info("sonic version check success.", also_print_console)
        else:
            self.logger.log_err(
                "sonic version check failed. local version is %s , check version is %s"
                % (result, sonic_version),
                also_print_console,
            )
            self.fail_reason.append("sonic version check failed")
            ret = E.EFAIL

        return ret

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        ret = self.check_sysdiag_version()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        ret = self.check_system_timezone()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        ret = self.check_sonic_version()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0 else E.EFAIL
        return [pass_cnt, fail_cnt, result]
