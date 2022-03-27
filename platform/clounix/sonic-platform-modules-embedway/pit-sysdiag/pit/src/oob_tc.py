import sys
from test_case import TestCaseCommon
from errcode import E
from function import run_command, load_platform_util_module


class OOBTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "bmcutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "BmcUtil"
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "oob_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.server_ip = None # external ip
        try:
            if self.platform_cfg_json and 'server_ip' in self.platform_cfg_json.keys():
                self.server_ip = self.platform_cfg_json['server_ip']
        except Exception as e:
            self.logger.log_err(str(e))
        
        bmc_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            bmc_util_class = getattr(bmc_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.bmc_util = bmc_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)


    def cpu_ping_bmc_test(self, also_print_console=True):
        self.logger.log_info("[CPU PING BMC TEST]:", also_print_console)
        
        count = 5
        cmd = "ping 240.1.1.1 -c %d -I eth0.4088 | grep received" % count
        status, output = run_command(cmd)
        self.logger.log_info(output, also_print_console)
        
        if output.find(" 0% packet loss") > 0:
            ret = E.OK
        else:
            self.fail_reason.append("cpu ping bmc lost packages")
            ret = E.EMGMT11003
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret


    def cpu_ping_server_test(self, also_print_console=True):
        self.logger.log_info("[CPU PING SERVER TEST]:", also_print_console)
        
        count = 5
        ping_cmd = "ping %s -c %d -I eth0 | grep received" % (self.server_ip, count)
        status, output = run_command(ping_cmd)
        self.logger.log_info(output, also_print_console)
        
        if output.find(" 0% packet loss") > 0:
            ret = E.OK
        else:
            self.fail_reason.append("cpu ping server lost packages")
            ret = E.EMGMT11002

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret
    
    
    def bmc_ping_server_test(self, also_print_console=True):
        self.logger.log_info("[BMC PING SERVER TEST]:", also_print_console)
        
        count = 5
        ping_cmd = "ping %s -c %d -I eth0 | grep received" % (self.server_ip, count)
        try:
            status, output = self.bmc_util.exec_raw_cmd(ping_cmd)
            self.logger.log_info(output[0], also_print_console)
            
            if status and output[0].find(" 0% packet loss") > 0:
                ret = E.OK
            else:
                self.fail_reason.append("bmc ping server lost packages")
                ret = E.EMGMT11004
        except Exception as e:
            self.fail_reason.append("bmc ping server fail: {}".format(str(e)))
            ret = E.EMGMT11004
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret


    def run_test(self, *argv):
        ret = self.cpu_ping_bmc_test()
        if ret != E.OK:
            return ret
        
        if not self.server_ip:
            self.fail_reason.append("server IP is null")
            ret = E.EEXIST
        else:
            ret = self.cpu_ping_server_test()
            if ret != E.OK:
                return ret
            ret =  self.bmc_ping_server_test()
        
        return ret
