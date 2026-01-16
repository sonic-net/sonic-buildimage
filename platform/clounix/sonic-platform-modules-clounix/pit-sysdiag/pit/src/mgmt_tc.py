import sys
from function import *
from test_case import TestCaseCommon
from errcode import *

PLATFORM_SPECIFIC_MODULE_NAME = "mgmtport"
PLATFORM_SPECIFIC_CLASS_NAME = "MgmtPortUtil"

class MGMTTC(TestCaseCommon):

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "mgmt_tc"
        self.mgmtport_util = None
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.server_ip = None        # external ip
        mgmtport_module = load_platform_util_module(PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            mgmtport_class = getattr(mgmtport_module, PLATFORM_SPECIFIC_CLASS_NAME)
            self.mgmtport_util = mgmtport_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)
        try:
            if self.platform_cfg_json and 'server_ip' in self.platform_cfg_json.keys():
                self.server_ip = self.platform_cfg_json['server_ip']
        except Exception as e:
            self.logger.log_err(str(e), True)


    def check_mgmt_status(self, port_status, also_print_console=True):
        ret = E.OK
        
        self.logger.log_info("[MGMT PORT STATUS CHECK]:", also_print_console)
        
        # check OS MGMT link status
        if not port_status:
            ret = E.EMGMT11001
            self.fail_reason.append("get eth0 status failed")
        else:
            for key, value in port_status.items():
                self.logger.log_info("{}: {}".format(key, value), also_print_console)
                if key == "Speed":
                    if value != "1000":
                        ret = E.EMGMT11001
                        self.fail_reason.append("speed is {}".format(value))
                
                if key == "Duplex":
                    if value != "full":
                        ret = E.EMGMT11001
                        self.fail_reason.append("duplex is {}".format(value))
                
                if key == "AN":
                    if value != True:
                        ret = E.EMGMT11001
                        self.fail_reason.append("auto-neg is {}".format(value))
                
                if key == "Link":
                    if value != True:
                        ret = E.EMGMT11001
                        self.fail_reason.append("link detected {}".format(value))
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret


    def mgmt_ping_test(self, also_print_console=True):
        ret = E.OK
        self.logger.log_info("[MGMT PORT PING CHECK]:", also_print_console)
        
        if self.server_ip == None:
            self.fail_reason.append("no server ip specified")
            return E.EEXIST
        else:
            # OS ping server
            count = '5'
            ping_cmd = "ping %s -c %s -I eth0 | grep received | awk '{print $4}'" % (self.server_ip, count)
            status, result = run_command(ping_cmd)
            self.logger.log_info("send {} {} packets, revice {} packets".format(self.server_ip, count, result), 
                                 also_print_console)
            # Compare the packages transmitted count and received count, failed if mismatch
            if status != 0 or result != count:
                self.fail_reason.append("ping {} lost packets".format(self.server_ip))
                ret = E.EMGMT11002
        
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret


    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        port_status = self.mgmtport_util.get_all()
        # If any test-case failed, do not continue
        ret = self.check_mgmt_status(port_status)
        if ret != NO_ERR:
            fail_cnt += 1
        else:
            pass_cnt += 1

        ret = self.mgmt_ping_test()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]