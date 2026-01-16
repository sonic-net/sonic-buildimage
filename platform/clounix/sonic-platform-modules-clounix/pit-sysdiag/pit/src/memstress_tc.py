# -*- coding:utf-8 -*-
import pit_util_common

from errcode import E
from test_case import TestCaseCommon
from pit_util_common import run_command

# memstress test class


class MEMSTRESS(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "memstress_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        try:
            self.memstress_info_dict = None
            if self.platform_cfg_json and "memstress_info" in self.platform_cfg_json.keys():
                self.memstress_info_dict = self.platform_cfg_json["memstress_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def test_memory_by_memtester(self, also_print_console=False):
        ret = E.EFAIL
        self.logger.log_info("[MEMORY MEMTESTER]:", also_print_console)
        available_value_cmd = "free -m | grep Mem | awk '{print $7}'"
        available_status, available_value = run_command(available_value_cmd)
        if available_status != 0 or len(available_value) <= 0:
            self.fail_reason.append("get available mem size fail.")
            ret = E.EIO
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        else:
            available_mem_value = int(available_value)
            mem_run_size_percentage = float(
                self.memstress_info_dict["mem_run_size_percentage"].replace(
                    "%", "")) / 100
            mem_run_size_value = int(
                available_mem_value *
                mem_run_size_percentage)
            mem_test_count = self.memstress_info_dict["mem_test_count"]
            self.logger.log_info("available memmory size: {}M".format(
                available_mem_value), also_print_console)
            self.logger.log_info(
                "memmory run size: {}M".format(mem_run_size_value),
                also_print_console)
            memstress_test_cmd = "memtester {}M {}".format(
                mem_run_size_value, mem_test_count)
            waiting = pit_util_common.waiting(
                self.logger, "memstress testing...")
            status, out = run_command(memstress_test_cmd)
            waiting.stop("memstress test stop!")
            if status != 0 or len(out) <= 0:
                self.fail_reason.append("memtester cmd error.")
                ret = E.EMEM4002
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            else:
                self.logger.log_info(out, also_print_console)
                for value in out.split("\n"):
                    if ":" in value and value.split(":")[-1] != "":
                        if "ok" not in value.split(":")[-1]:
                            self.fail_reason.append(
                                "memmory stress test error.")
                            ret = E.EMEM4005
                            self.logger.log_err("FAIL!", also_print_console)
                            return ret
                        else:
                            ret = E.OK
        self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        for i in range(self.memstress_info_dict["memstress_test_time"]):
            ret = self.test_memory_by_memtester(True)
            if ret != E.OK:
                fail_cnt += 1
                break
            else:
                pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
