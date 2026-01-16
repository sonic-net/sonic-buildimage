import os
import time
import subprocess
from pit_util_common import run_command
from test_case import TestCaseCommon
from function import restful_command
from errcode import *

class WATCHDOGTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "watchdog_tc"  # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

    def read_sysfs(self,file):        
        with open(file,'r') as f:
            return f.read().strip()
    
    def write_sysfs(self,file,v):        
        with open(file,'w') as f:
            return f.write(v)

    def get_watchdog_node_property(self, node):
        node_path = "/sys_switch/watchdog/{}".format(node)
        val = self.read_sysfs(node_path)
        return val
    
    def set_watchdog_node_property(self, node, v):
        node_path = "/sys_switch/watchdog/{}".format(node, v)
        val = self.write_sysfs(node_path, v)
        return val
    
    def test_watchdog_identify(self):
        ret = E.OK
        val = self.get_watchdog_node_property("identify")
        if val == "clounix_fpga_wdt":
            self.logger.log_info("watchdog recognize success !", True)
            return E.OK
        else:
            self.logger.log_err("watchdog recognize fail !", True)
            return E.EFAIL 
    
    def test_watchdog_enable(self):
        ret = E.OK
        val = self.set_watchdog_node_property("enable", str(1))
        val = self.get_watchdog_node_property("enable")
        if int(val) == 1:
            self.logger.log_info("watchdog set enable success !", True)
            ret &= E.OK
        else:
            self.logger.log_err("watchdog set enable fail !", True)
            ret &= E.EFAIL

        val = self.set_watchdog_node_property("enable", str(0))
        val = self.get_watchdog_node_property("enable")
        if int(val) == 0:
            self.logger.log_info("watchdog set disable success !", True)
            ret &= E.OK
        else:
            self.logger.log_err("watchdog set disable fail !", True)
            ret &= E.EFAIL  
    
        return ret  
    
    def test_watchdog_state(self):
        ret = E.OK
        val = self.set_watchdog_node_property("enable", str(1))
        val = self.get_watchdog_node_property("state")
        if val == "active":
            self.logger.log_info("watchdog set state active success !", True)
            ret &= E.OK
        else:
            self.logger.log_err("watchdog set state active success fail !", True)
            ret &= E.EFAIL  
        
        val = self.set_watchdog_node_property("enable", str(0))
        val = self.get_watchdog_node_property("state")
        if val == "inactive":
            self.logger.log_info("watchdog set state inactive success !", True)
            ret &= E.OK
        else:
            self.logger.log_err("watchdog set state inactive fail !", True)
            ret &= E.EFAIL 
        return ret 

    def test_watchdog_timeout(self):
        ret = E.OK
        val_vec = [1, 128, 255]
        for timeout_val in val_vec:
            val = self.set_watchdog_node_property("timeout", str(timeout_val))
            val = self.get_watchdog_node_property("timeout")
            if int(val) == timeout_val:
                self.logger.log_info("watchdog set timeout {} success !".format(timeout_val), True)
                ret &= E.OK
            else:
                self.logger.log_err("watchdog set timeout {} fail !".format(timeout_val), True)
                ret &= E.EFAIL 

        return ret 
    
    def test_watchdog_timeleft(self):
        timeout_val = 128
        self.set_watchdog_node_property("enable", str(0))
        self.set_watchdog_node_property("timeout", str(timeout_val))
        self.set_watchdog_node_property("enable", str(1))

        left_val1 = self.get_watchdog_node_property("timeleft")
        time.sleep(1)
        left_val2 = self.get_watchdog_node_property("timeleft")

        if int(left_val1) > int(left_val2):
            self.logger.log_info("watchdog timeleft auto decrement success !", True)
            return E.OK
        else:
            self.logger.log_err("watchdog timeleft auto decrement fail !", True)
            return E.EFAIL 

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0

        ret = self.test_watchdog_identify()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        ret = self.test_watchdog_enable()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1

        ret = self.test_watchdog_state()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.test_watchdog_timeout()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        ret = self.test_watchdog_timeleft()
        if ret != E.OK:
            fail_cnt += 1
        else:
            pass_cnt += 1
        
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
