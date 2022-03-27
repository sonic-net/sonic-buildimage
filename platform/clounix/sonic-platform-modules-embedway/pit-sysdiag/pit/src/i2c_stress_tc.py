from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import E
from function import run_command
import sys
import os
import time
import threading

STRESS_TIME = "loop_time"
STRESS_THREADS = "run_threads"
STRESS_CFG = "slave_dict"
i2c_stress_target_dicts = []

def test_thread(num):
    global i2c_stress_target_dicts
    status = 0
    while True:
        for slave_node in i2c_stress_target_dicts:
            master = slave_node['master']
            slave = slave_node['slave']
            addr = slave_node['addr']
            cmd = "i2cget -f -y {} {} {} b > /dev/null".format(master, slave, addr)
            status = os.system(cmd)
            if status is not 0:
                return status

class I2CTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "i2c_stress_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        try:
            if self.platform_cfg_json and 'i2c_stress' in self.platform_cfg_json.keys():
                self.i2c_stress_cfg = self.platform_cfg_json['i2c_stress']
        except Exception as e:
            self.logger.log_err(str(e), True)

    def run_test(self, *argv):
        global i2c_stress_target_dicts
        pass_cnt = 0
        fail_cnt = 0
        threads = []
        if STRESS_CFG in self.i2c_stress_cfg:
            i2c_stress_target_dicts = self.i2c_stress_cfg[STRESS_CFG]
            if len(i2c_stress_target_dicts) == 0:
                fail_cnt += 1
                self.logger.log_err("i2c_stress cfg is empty!", True)
            else:
                for i in range(0, self.i2c_stress_cfg[STRESS_THREADS]):
                    thread = threading.Thread(target = test_thread, name = "thread{}".format(i+1), args = (i,))
                    threads.append(thread)
                    thread.start()
                    self.logger.log_info("test thread:{} start running.".format(thread.getName()), True)

                for sec in range(0, self.i2c_stress_cfg[STRESS_TIME]):
                    for thread in threads:
                        if thread.is_alive() != True:
                            self.logger.log_err("test thread:{} fail!".format(thread.getName()), True)
                            fail_cnt += 1
                    if fail_cnt != 0:
                        break
                    time.sleep(1)
                    self.logger.log_info("left times: {} secs, ctrl+c to exit.".format(self.i2c_stress_cfg[STRESS_TIME] - sec), True)

                for thread in threads:
                    if thread.is_alive() == True:
                        thread._Thread__stop()
                    thread.join()
                    pass_cnt += 1
        else:
            fail_cnt += 1
            self.logger.log_err("i2c_stress cfg load failed!", True)

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
