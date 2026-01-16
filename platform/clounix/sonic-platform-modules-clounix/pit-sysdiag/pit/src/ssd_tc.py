import os
import re
from function import run_command
from test_case import TestCaseCommon
from errcode import *

class SSDTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "ssd_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.test_size = 1               # unit: MBytes, default
        self.ssd_bom_list = None         # default
        '''
        "ssd_test_size":"100M",
        "ssd_bom":[
            {
                "model": "INTEL SSDSCKKB240G8",
                "size": "240 GB"
            },
            {
                "model": "MTFDDAV240TDS",
                "size": "240 GB"
            }
        ],
        '''
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
        ssdpath = []

        spect = "sd"
        result = self.search_dir_by_name(spect, dir)
        spect = "nvme"
        result = result + self.search_dir_by_name(spect, dir)

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
            self.fail_reason.append("ssd not found!")
        return ret, ssdpath


    def ssd_info_check(self, ssdpath, also_print_console=False):
        """
            Read SSD Information
        """
        RET = E.OK
        ssd = {}
        self.logger.log_info("[SSD INFO CHECK]:", also_print_console)
        for path in ssdpath:
            cmd = "smartctl -i {}".format(path)
            print(cmd)
            status, out = run_command(cmd)
            self.logger.log_info(out, also_print_console)
            if status:
                err = "Read ssd {} info failed!".format(path)
                self.fail_reason.append(err)
                RET = E.ESSD2001
            else:
                if self.ssd_bom_list:
                    matched = False
                    for ssd_bom in self.ssd_bom_list:
                        if out.find(ssd_bom["model"]) != -1 and out.find(ssd_bom["size"]) != -1:
                            matched = True
                            break
                    if not matched:
                        RET = E.ESSD2001
                        self.fail_reason.append("ssd info not match")

        if RET != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return RET


    def ssd_health_check(self, ssdpath, also_print_console=False):
        """
             SSD SMART overall-health self-assessment test
        """
        ret = E.OK
        self.logger.log_info("[SSD HEALTH CHECK]:", also_print_console)
        for path in ssdpath:
            status, out = run_command("smartctl -H {} | grep result".format(path))
            self.logger.log_info(out, also_print_console)
            
            if out.find("PASSED") == -1:
                err = "ssd {} health check failed!".format(path)
                ret = E.ESSD2004
                self.fail_reason.append(err) 

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret


    def ssd_read_test(self, ssdpath, also_print_console=False):
        self.logger.log_info("[SSD READ TEST]:", also_print_console)

        bs_count = self.test_size * 64
        for path in ssdpath:
            cmd = "dd if=%s of=/dev/null bs=16k count=%d iflag=direct,nonblock" % (path, bs_count)
            self.logger.log_info(cmd, also_print_console)
            status, out = run_command(cmd)
            if status:
                err = "[{}] read test failed!".format(self.module_name)
                self.fail_reason.append(err)
            else:
                self.logger.log_info(out, also_print_console)

            if status:
                self.logger.log_err("FAIL!", also_print_console)
                ret = E.ESSD2002
                break
            else:
                self.logger.log_info("PASS.", also_print_console)
                ret = E.OK

        return ret


    def ssd_write_test(self, also_print_console=False):
        self.logger.log_info("[SSD WRITE TEST]:", also_print_console)

        bs_count = self.test_size * 64
        cmd = "dd if=/dev/urandom of=/tmp/txtfile_ssd bs=16k count=%d oflag=direct,nonblock" % bs_count
        self.logger.log_info(cmd, also_print_console)
        status, out = run_command(cmd)
        if status:
            err = "[{}] write test failed!".format(self.module_name)
            self.fail_reason.append(err)
        else:
            self.logger.log_info(out, also_print_console)
        os.remove("/tmp/txtfile_ssd")

        if status:
            self.logger.log_err("FAIL!", also_print_console)
            ret = E.ESSD2003
        else:
            self.logger.log_info("PASS.", also_print_console)
            ret = E.OK
        return ret


    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        status, ssdpath = self.get_ssd_location()
        if status:
            self.fail_reason.append("get ssd location error, E.ESSD2001")
            fail_cnt += 1
            result=E.EFAIL
            return [pass_cnt, fail_cnt, result]
        
        ret = self.ssd_info_check(ssdpath, True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.ssd_health_check(ssdpath, True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.ssd_read_test(ssdpath, True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.ssd_write_test(True)
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
