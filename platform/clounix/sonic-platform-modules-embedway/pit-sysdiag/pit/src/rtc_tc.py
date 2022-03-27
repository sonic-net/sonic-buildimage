# -*- coding:utf-8
import time
from test_case import TestCaseCommon
from function import run_command
from errcode import E

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

    def read_rtc(self, also_print_console=False):
        ret = E.OK
        self.logger.log_info("[RTC READ CHECK]:", also_print_console)

        cmd = "hwclock -r"
        code, out = run_command(cmd)
        if code:
            ret = E.ERTC12001
            self.fail_reason.append(out)
        self.logger.log_info(out, also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def rtc_precision_test(self, wait_time=5, also_print_console=False):
        ret = E.OK
        rtc_since_epoch_file = "/sys/class/rtc/rtc0/since_epoch"

        self.logger.log_info("[RTC PRECISION CHECK]:", also_print_console)
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
                self.fail_reason.append("{} beyond {}".format(timeCompare, wait_time))
                ret = E.ERTC12002
        except IOError as e:
            self.fail_reason.append(str(e))
            ret = E.ERTC12001

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def rtc_functional_test(self, also_print_console=False):
        self.logger.log_info("[RTC READ CHECK]:", also_print_console)
        time_start = time.strftime("%Y-%m-%d %H:%M:%S")
        self.logger.log_info(
            "current time: %s" %
            time_start, also_print_console)
        current_secs_before = int(time.time())
        delay_interval = self.rtc_info_dict["delay_time"]
        self.logger.log_info(
            "please waiting {} sec".format(delay_interval),
            also_print_console)
        time.sleep(delay_interval)
        time_end = time.strftime("%Y-%m-%d %H:%M:%S")
        self.logger.log_info("current time: %s" % time_end, also_print_console)
        current_secs_after = int(time.time())
        delta_interval = current_secs_after - current_secs_before
        if abs(
                delta_interval -
                delay_interval) > self.rtc_info_dict["max_time_diff"]:
            self.fail_reason.append("time out of sync")
            ret = E.ERTC12002
        else:
            ret = E.OK

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        # RTC functional test
        res1 = self.rtc_functional_test(True)
        if res1 != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        # RTC read test
        res2 = self.read_rtc(True)
        if res2 != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        # RTC precision test
        res3 = self.rtc_precision_test(5, True)
        if res3 != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if res1==E.OK and res2==E.OK and res3==E.OK else E.EFAIL
        return [pass_cnt, fail_cnt, result]
