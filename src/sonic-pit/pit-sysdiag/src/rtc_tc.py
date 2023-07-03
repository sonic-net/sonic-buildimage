# -*- coding:utf-8
import time
from test_case import TestCaseCommon
from function import run_command
from errcode import E
import traceback

class RTCTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "rtc_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger,
                                platform_cfg_file, case_cfg_file)
        self.rtc_info_dict = None
        try:
            if self.platform_cfg_json and \
                "rtc_info" in self.platform_cfg_json.keys():
                self.rtc_info_dict = self.platform_cfg_json["rtc_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def read_rtc_test(self, also_print_console=False):
        ret = E.OK
        self.logger.log_info("read_rtc_test start")

        cmd = "hwclock -r"
        code, out = run_command(cmd)
        if code:
            reason = "Failed to exec 'hwclock -r'"
            ret = E.ERTC12001
            self.log_reason(reason)
        self.logger.log_dbg(out, also_print_console)

        if ret != E.OK:
            self.logger.log_info("read_rtc_test done, FAILED.")
        else:
            self.logger.log_info("read_rtc_test done, PASS.")
        return ret

    def rtc_precision_test(self, wait_time=5, also_print_console=False):
        ret = E.OK
        rtc_since_epoch_file = "/sys/class/rtc/rtc0/since_epoch"

        self.logger.log_info("rtc_precision_test start")
        try:
            with open(rtc_since_epoch_file, "r") as f:
                start_sec = int(f.read())
                timeArraystart = time.localtime(start_sec)
                otherStyleTime = time.strftime("%Y-%m-%d %H:%M:%S", timeArraystart)
                log_msg = "rtc time: {}".format(otherStyleTime)
                self.logger.log_info(log_msg, also_print_console)
            log_msg = "system time: {}".format(
                time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
            self.logger.log_info(log_msg, also_print_console)

            self.logger.log_info("time sleep: " + str(wait_time), also_print_console)
            time.sleep(wait_time)

            with open(rtc_since_epoch_file, "r") as f:
                end_sec = int(f.read())
                timeArrayend = time.localtime(end_sec)
                otherStyleTime = time.strftime("%Y-%m-%d %H:%M:%S", timeArrayend)
                log_msg = "rtc time: {}".format(otherStyleTime)
                self.logger.log_info(log_msg, also_print_console)
            log_msg = "system time: {}".format(
                time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
            self.logger.log_info(log_msg, also_print_console)

            timeCompare = end_sec - start_sec
            self.logger.log_info("time difference: " + str(timeCompare),
                                 also_print_console)
            if timeCompare < (wait_time - 1) or timeCompare > (wait_time + 1):
                self.log_reason("{} beyond {}".format(timeCompare, wait_time))
                ret = E.ERTC12002
        except IOError as e:
            self.fail_reason.append(str(e))
            ret = E.ERTC12001

        if ret != E.OK:
            self.logger.log_info("rtc_precision_test done, FAILED.")
        else:
            self.logger.log_info("rtc_precision_test done, PASS.")

        return ret

    def rtc_functional_test(self, also_print_console=False):
        self.logger.log_info("rtc_functional_test start")
        current_secs_before = int(time.time())
        delay_interval = self.rtc_info_dict["delay_time"]
        self.logger.log_info(
            "please waiting {} sec".format(delay_interval), also_print_console)
        time.sleep(delay_interval)
        time_end = time.strftime("%Y-%m-%d %H:%M:%S")
        self.logger.log_info("current time: %s" % time_end, also_print_console)
        current_secs_after = int(time.time())
        delta_interval = current_secs_after - current_secs_before
        if abs(delta_interval - delay_interval) > self.rtc_info_dict["max_time_diff"]:
            self.log_reason("time out of sync")
            ret = E.ERTC12002
        else:
            ret = E.OK

        if ret != E.OK:
            self.logger.log_err("rtc_functional_test FAILED")
        else:
            self.logger.log_info("rtc_functional_test PASS.")

        return ret

    def run_test(self, *argv):
        # RTC functional test
        final_result = E.OK
        try:
            ret = self.rtc_functional_test()
            if ret != E.OK:
                final_result = ret
        except Exception as e:
            reason = "Failed, {} rtc_functional_test exception: {}".format(self.get_tc_name(), str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_result = E.EFAIL

        # RTC read test
        try:
            ret = self.read_rtc_test()
            if ret != E.OK:
                final_result = ret
        except Exception as e:
            reason = "Failed, {} read_rtc_test exception: {}".format(self.get_tc_name(), str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_result = E.EFAIL

        # RTC precision test
        try:
            ret = self.rtc_precision_test(5)
            if ret != E.OK:
                final_result = ret
        except Exception as e:
            reason = "Failed, {} rtc_precision_test exception: {}".format(self.get_tc_name(), str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())
            final_result = E.EFAIL

        return final_result
