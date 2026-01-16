# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E


class BIOSMETC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "biosme_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.cmdlist = [
            ["--help"],
            ["lock"],
            ["status", "recovery mode", "BIOS recovery open fail.", E.EBIOSME32001],
            ["unlock"],
            ["status", "normal", "BIOS recovery close fail.", E.EBIOSME32002]
        ]

    def bios_me_test(self, also_print_console=False):
        self.logger.log_info(
            "[CHECK BIOS ME CONTROL TEST]:",
            also_print_console)
        for item in self.cmdlist:
            cmd = "pmonutil me {}".format(item[0])
            status, log = run_command(cmd)
            if status != 0 or len(log) < 0:
                self.fail_reason.append(
                    "pmonutil {} cmd error.".format(item[0]))
                self.logger.log_err("FAIL!", also_print_console)
                ret = E.EIO
                return ret
            else:
                if item[0] == "--help":
                    ret = E.OK
                    self.logger.log_info(log, also_print_console)
                elif item[0] == "status":
                    if item[1] not in log:
                        self.fail_reason.append(item[2])
                        ret = item[3]
                        self.logger.log_err("FAIL!", also_print_console)
                        return ret
                    else:
                        ret = E.OK
                        self.logger.log_info(
                            "current ME status: {}".format(log), also_print_console)
        self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        ret = self.bios_me_test(True)
        if ret != E.OK:
            return ret
        return ret
