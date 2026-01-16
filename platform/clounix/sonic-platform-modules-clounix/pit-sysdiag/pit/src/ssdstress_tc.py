# -*- coding:utf-8 -*-
import re
import pit_util_common
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E


class SSDSTRESSTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "ssdstress_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.ssdstress_info_dict = None
        try:
            if self.platform_cfg_json and "ssdstress_info" in self.platform_cfg_json.keys():
                self.ssdstress_info_dict = self.platform_cfg_json["ssdstress_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def ssd_status_check(self, also_print_console=False):
        dev_pattern = self.ssdstress_info_dict['dev_pattern']
        ssd_totalsize_cmd = "df -h | grep %s | awk '{print $2}'" % dev_pattern
        total_size_status, total_size_log = run_command(ssd_totalsize_cmd)
        self.logger.log_info(
            "ssd total size: {}".format(total_size_log),
            also_print_console)
        if int(
                self.ssdstress_info_dict["total_ssd_value"].split("G")[0]) >= int(
                total_size_log.split("G")[0]):
            return False
        else:
            return True

    def ssd_stress_test(self, also_print_console=False):
        self.logger.log_info("[SSD STRESS TEST]:", also_print_console)
        dev_pattern = self.ssdstress_info_dict['dev_pattern']
        ssd_availsize_cmd = "df -h | grep %s | awk '{print $4}'" % dev_pattern

        if not self.ssd_status_check(True):
            self.fail_reason.append("ssd total size not match")
            ret = E.ESSDSTRESS30001
            self.logger.log_err("FAIL!", also_print_console)
            return ret

        avail_size_status, avail_size_log = run_command(ssd_availsize_cmd)
        if avail_size_status != 0 or len(avail_size_log) <= 0:
            self.fail_reason.append("get disk info fail.")
            ret = E.EIO
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        else:
            ssd_run_size_percentage = float(
                self.ssdstress_info_dict["ssd_run_size_percentage"].replace(
                    "%", "")) / 100
            available_ssd_value = float(avail_size_log.split("G")[0])
            ssd_run_size_value = int(
                available_ssd_value *
                ssd_run_size_percentage)

            if (ssd_run_size_value > 8):
                ssd_run_size_value = 8;

            self.logger.log_info("ssd available size: {}G".format(
                available_ssd_value), also_print_console)
            self.logger.log_info(
                "ssd run size: {}G".format(ssd_run_size_value),
                also_print_console)
            ssd_stress_cmd = "fio -filename={} -direct=1 -iodepth 1 -thread -rw={} -ioengine=libaio -bs={} -size={}G -numjobs={} -group_reporting -name=ssd_stress" \
                .format(self.ssdstress_info_dict["dev_path"], self.ssdstress_info_dict["rw_mode"], self.ssdstress_info_dict["io_size"],
                        ssd_run_size_value, self.ssdstress_info_dict["num_jobs"])
            waiting = pit_util_common.waiting(
                self.logger, "ssdstress testing...")
            status, stress_log = run_command(ssd_stress_cmd)
            run_command("rm -rf /stress.txt")
            waiting.stop("ssdstress test stop!")

            if status != 0 or len(stress_log) <= 0:
                self.fail_reason.append("fio ssd stress test fail.")
                ret = E.ESSDSTRESS30002
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            else:
                iop_pattern = "IOPS=.*, "
                read_iops = re.findall(iop_pattern, stress_log)[0]
                self.logger.log_info(
                    "read iops: {}".format(read_iops),
                    also_print_console)
                read_iops = read_iops.strip("IOPS=")
                if 'k' in read_iops:
                    factors = 1000
                    read_iops = read_iops.split('k')[0]
                elif 'm' in read_iops:
                    factors = 1000000
                    read_iops = read_iops.split('m')[0]
                else:
                    factors = 1
                    read_iops = read_iops.split(',')[0]

                read_iops = float(read_iops) * factors
                min_iops = float(self.ssdstress_info_dict["iops_value_standard"])
                if read_iops < min_iops:
                    ret = E.ESSDSTRESS30003
                    self.fail_reason.append("ssd read counts error: min is {} curr is {}".format(min_iops, read_iops))
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret

            if not self.ssd_status_check(True):
                self.fail_reason.append("ssd total size not match")
                ret = E.ESSDSTRESS30003
                self.logger.log_err("FAIL!", also_print_console)
                return ret

            ret = E.OK
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0
        for i in range(self.ssdstress_info_dict["ssd_test_count"]):
            ret = self.ssd_stress_test(True)
            if ret != E.OK:
                fail_cnt += 1
            else:
                pass_cnt += 1
                     
        result = E.OK if fail_cnt == 0 else E.EFAIL
        return [pass_cnt, fail_cnt, result]
