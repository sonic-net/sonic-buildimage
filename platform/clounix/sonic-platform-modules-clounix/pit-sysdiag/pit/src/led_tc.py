import os

from errcode import *
from test_case import TestCaseCommon
from function import run_command

DEV_LED_PATH="/sys_switch/sysled/"
LED_NODE_LIST=["psu_led_status", "sys_led_status"]

class LEDTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "led_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        org_val_list = []
        for node in LED_NODE_LIST:
            ret, org = run_command("cat " + DEV_LED_PATH + node)
            if ret != 0:
                fail_cnt += 1
            else:
                org_val_list.append(org)
        
        for index in range(len(org_val_list)):
            # if int(org_val_list[index]) > 0:
            #     val = '0'
            # else:
            #     val = '1'
            val = '1'

            ret, org = run_command("echo " + val + " > " + DEV_LED_PATH + LED_NODE_LIST[index])
            if ret != 0:
                fail_cnt += 1
                continue

            ret, org = run_command("cat " + DEV_LED_PATH + LED_NODE_LIST[index])
            if ret != 0 or org != val:
                fail_cnt += 1
                continue
            
            ret, org = run_command("echo " + org_val_list[index] + " > " + DEV_LED_PATH + LED_NODE_LIST[index])
            if ret != 0:
                fail_cnt += 1
                continue
            
            ret, org = run_command("cat " + DEV_LED_PATH + LED_NODE_LIST[index])
            if ret != 0 or org != org_val_list[index]:
                fail_cnt += 1
                continue
            else:
                pass_cnt += 1
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
