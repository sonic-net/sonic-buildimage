import os
import subprocess
from test_case import TestCaseCommon
from errcode import E

def run_command(cmd):
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode == 0:
        if err:
            out += err
    return proc.returncode, out.rstrip('\n')

class EMMCTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "emmc_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.test_size = 1             # unit: MBytes
        self.emmc_bom_list = None      # default
        '''
        "emmc_test_size":"100M",
        "emmc_bom":[
            {
                "manufacturer":"Toshiba",
                "pn":"064G02"
            },
        ],
        '''
        if self.platform_cfg_json and 'emmc_test_size' in self.platform_cfg_json.keys():
            size = self.platform_cfg_json['emmc_test_size']
            if size.endswith("m") or size.endswith("M"):
                self.test_size = int(size.strip("mM"))
            else:
                self.test_size = int(size)
        if self.platform_cfg_json and 'emmc_bom' in self.platform_cfg_json.keys():
                self.emmc_bom_list = self.platform_cfg_json['emmc_bom']
            

    def emmc_info_check(self, also_print_console=False):
        """
        get emmc CID data
        """
        ret = E.OK
        self.logger.log_info( "[EMMC INFO CHECK]:", also_print_console)
        
        status, out = run_command("mmc cid read /sys/block/mmcblk0/device")
        if status:
            err = "Read eMMC info failed!"
            self.fail_reason.append(err)
            ret = E.ESSD2001
        else:
            with open("/sys/block/mmcblk0/size", 'r') as f:
                value = f.read()
                size = round(float(value) * 512 / 1024 / 1024 / 1024, 1)

            self.logger.log_info(out, also_print_console)
            self.logger.log_info("size: {}GB".format(size), also_print_console)
        
            if self.emmc_bom_list:
                matched = False
                for emmc_bom in self.emmc_bom_list:
                    if out.find(emmc_bom["manufacturer"]) != -1 and out.find(emmc_bom["pn"]) != -1:
                        matched = True
                        break
                if not matched:
                    ret = E.ESSD2001
                    self.fail_reason.append("emmc info not match")
        
        if ret != E.OK:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret


    def emmc_read_test(self, also_print_console=False):
        self.logger.log_info( "[EMMC READ TEST]:", also_print_console)
        
        bs_count = self.test_size * 64
        cmd = "dd if=/dev/mmcblk0 of=/dev/null bs=16k count=%d iflag=direct,nonblock" % bs_count
        self.logger.log_info(cmd, also_print_console)
        status, out = run_command(cmd)
        if status:
            err = "emmc read test failed!"
            self.fail_reason.append(err)
            ret = E.ESSD2002
        else:
            self.logger.log_info(out, also_print_console)
            ret = E.OK
        
        if ret != E.OK:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        
        return ret
    
    
    def emmc_write_test(self, also_print_console=False):
        self.logger.log_info( "[EMMC WRITE TEST]:", also_print_console)
        
        bs_count = self.test_size * 64
        cmd = "dd if=/dev/urandom of=/tmp/txtfile bs=16k count=%d oflag=direct,nonblock" % bs_count
        self.logger.log_info(cmd, also_print_console)
        status, out = run_command(cmd)
        if status:
            err = "emmc write test failed!"
            self.fail_reason.append(err)
            ret = E.ESSD2003
        else:
            self.logger.log_info(out, also_print_console)
            ret = E.OK
        os.remove("/tmp/txtfile")
        
        if ret != E.OK:
            self.logger.log_info("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret
    
    
    def run_test(self, *argv):
        ret = self.emmc_info_check(True)
        if ret != E.OK:
            return ret
        
        loop = int(argv[0])
        for i in range(1, loop + 1):
            ret = self.emmc_read_test(True)
            if ret != E.OK:
                self.logger.log_err("emmc r/w loop {} failed!".format(i), True)
                break
            ret = self.emmc_write_test(True)
            if ret != E.OK:
                self.logger.log_err("emmc r/w loop {} failed!".format(i), True)
                break
        return ret
        