import os
import re
import subprocess
from test_case import TestCaseCommon
from errcode import *
from function import run_command
import traceback


class SSDTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "ssd_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.test_size = 1               # unit: MBytes, default
        self.ssd_bom_list = None         # default

        try:
            if self.platform_cfg_json and 'ssd_test_size' in self.platform_cfg_json.keys():
                size = self.platform_cfg_json['ssd_test_size']
                if size.endswith("m") or size.endswith("M"):
                    self.test_size = int(size.strip("mM"))
                else:
                    self.test_size = int(size)
            if self.platform_cfg_json and 'ssd_bom' in self.platform_cfg_json.keys():
                self.ssd_bom_list = self.platform_cfg_json['ssd_bom']
        except Exception as e:
            self.logger.log_err(str(e))

    def search_dir_by_name(self, name, dir):
        result = []
        try:
            files = os.listdir(dir)
            for file in files:
                if name in file:
                    result.append(os.path.join(dir, file))
        except Exception as e:
            pass
        return result

    def get_ssd_location(self):
        ret = NO_ERR
        dir = "/sys/block/"
        spect = "sd"
        ssdpath = []
        result = self.search_dir_by_name(spect, dir)
        if len(result) <= 0:
            ret = ABSENT_ERR
        else:
            for item in result:
                with open(os.path.join(item, "removable"), 'r') as fd:
                    value = fd.read()
                    if value.strip() == "0":  # found ssd
                        ssd_disk = "/dev/" + os.path.basename(item)
                        ssdpath.append(ssd_disk)
            if not ssdpath:  # not found ssd
                self.logger.log_err("no ssd found")
                ret = ABSENT_ERR

        if ret:
            self.log_reason("ssd not found!")

        return ret, ssdpath

    def test_ssd_info(self, ssdpath):
        ret = E.OK
        ssd = {}
        self.logger.log_info("test ssd info start")
        for path in ssdpath:
            status, out = run_command("smartctl -i {}".format(path))
            self.logger.log_info(out)
            if status:
                err = "Read ssd {} info failed!".format(path)
                self.log_reason(err)
                ret = E.ESSD2001
            else:
                if self.ssd_bom_list:
                    matched = False
                    model_match = False
                    size_match = False
                    lines = out.splitlines()
                    for ssd_bom in self.ssd_bom_list:
                        expected_model = ssd_bom["model"]
                        expected_size = ssd_bom["size"]
                        for line in lines:
                            if line.startswith("Device Model:"):
                                if line.find(expected_model) != -1:
                                    model_match = True
                                    continue
                            elif line.startswith("User Capacity"):
                                if line.find(expected_size) != -1:
                                    size_match = True
                                    continue
                        if model_match and size_match:
                            matched = True
                            break

                    # Does not match any of expected BOM
                    if not matched:
                        ret = E.ESSD2001
                        self.fail_reason.append("SSD model/size not match")

        if ret != E.OK:
            self.logger.log_err("test ssd info done, FAILED.")
        else:
            self.logger.log_err("test ssd info done, PASS.")

        return ret

    def ssd_health_check(self, ssdpath):
        """
             SSD SMART overall-health self-assessment test
        """
        ret = E.OK
        self.logger.log_info("ssd health check start")
        for path in ssdpath:
            status, out = run_command("smartctl -H {} | grep result".format(path))
            self.logger.log_info(out)

            if out.find("PASSED") == -1:
                reason = "ssd {} health check failed!".format(path)
                ret = E.ESSD2004
                self.log_reason(err)

        if ret != E.OK:
            self.logger.log_err("ssd health check done, FAILED.")
        else:
            self.logger.log_err("ssd health check done, PASS.")

        return ret

    def ssd_read_test(self):
        self.logger.log_info("ssd read test start")

        bs_count = self.test_size * 64
        cmd = "dd if=/dev/sda of=/dev/null bs=16k count=%d iflag=direct,nonblock" % bs_count
        self.logger.log_dbg(cmd)
        status, out = run_command(cmd)
        if status:
            err = "[{}] read test failed!".format(self.module_name)
            self.log_reason.append(err)
        else:
            self.logger.log_info(out)

        if status:
            self.logger.log_err("ssd read test done, FAILED.")
            ret = E.ESSD2002
        else:
            self.logger.log_info("ssd read test done, PASS.")
            ret = E.OK

        return ret

    def ssd_write_test(self):
        self.logger.log_info("ssd write test start")

        bs_count = self.test_size * 64
        cmd = "dd if=/dev/urandom of=/tmp/txtfile_ssd bs=16k count=%d oflag=direct,nonblock" % bs_count
        self.logger.log_info(cmd)
        status, out = run_command(cmd)
        if status:
            err = "[{}] write test failed!".format(self.module_name)
            self.log_reason.append(err)
        else:
            self.logger.log_info(out)
        os.remove("/tmp/txtfile_ssd")

        if status:
            self.logger.log_err("ssd write test done, FAILED.")
            ret = E.ESSD2003
        else:
            self.logger.log_info("ssd write test done, PASS.")
            ret = E.OK

        return ret

    def run_test(self, *argv):
        final_ret = E.OK

        try:
            status, ssdpath = self.get_ssd_location()
            if status:
                final_ret = E.ESSD2001
        except Exception as e:
            reason = "get ssd location exception {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret = self.test_ssd_info(ssdpath)
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "ssd info check exception {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret = self.ssd_health_check(ssdpath)
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "ssd health check exception {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret = self.ssd_read_test()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "ssd read test exception {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret = self.ssd_write_test()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "ssd write test exception {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        return final_ret
