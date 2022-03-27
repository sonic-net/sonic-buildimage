from function import run_command, exec_cmd
from test_case_base import TestCaseCommon
from errcode import E
key_list = ['Product Name', 'Part Number', 'Serial Number', 'Manufacturer',
            ]
FRU_TEST = "../config/common/slot_test.bin"
FRU_ORIG = "../config/common/slot_orig.bin"


# SLOT_EEPROM_WRITE_TC test class
class SLOT_EEPROM_WRITE_TC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "slot_eeprom_write_tc"
        self.rec = ''
        self.code = 0
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

    def write_slot_fru_eeprom(self, replaceable_part, console_print=False):
        """
                    STORE&WRITE EEPROM Information by xxx.bin
        """

        self.logger.log_info("[ CHECK SET FRU-EEPROM  ]: ==>{}<==".format(replaceable_part), console_print)
        cmd = 'python fruidutil.py get {}'.format(replaceable_part)
        res_orig = exec_cmd(cmd)
        if not res_orig or 'failed' in res_orig:
            self.fail_reason.append("get orig-fru info fail.")
        else:
            self.logger.log_info(res_orig, console_print)


        eeprom_orig = exec_cmd('cat FRU.bin')
        exec_cmd('mv FRU.bin {}'.format(FRU_ORIG))
        eeprom_test = exec_cmd('cat {}'.format(FRU_TEST))
        set_test_cmd = 'python fruidutil.py set {} -f {}'.format(replaceable_part, FRU_TEST)
        exec_cmd(set_test_cmd)
        res_test_readback = exec_cmd(cmd)
        if not res_test_readback:
            self.fail_reason.append("get test-readback-fru info fail.")
        else:
            self.logger.log_info("------||. test fru-syseeprom after set .||------", console_print)
            self.logger.log_info(res_test_readback, console_print)

        eeprom_test_readback = exec_cmd('cat FRU.bin')

        if eeprom_test != eeprom_test_readback:
            result1 = False
            self.logger.log_info("===== Check test fru-eeprom failed =====", console_print)
        else:
            result1 = True
            self.logger.log_info("===== Set test fru-eeprom Success =====", console_print)

        set_orig_cmd = 'python fruidutil.py set {} -f {}'.format(replaceable_part, FRU_ORIG)
        exec_cmd(set_orig_cmd)
        res_orig_readback = exec_cmd(cmd)
        if not res_orig_readback:
            self.fail_reason.append("get orig-readback-fru info fail.")
        else:
            self.logger.log_info("------||. orig fru-syseeprom after set .||------", console_print)
            self.logger.log_info(res_orig_readback, console_print)

        eeprom_orig_readback = exec_cmd('cat FRU.bin')
        if eeprom_orig_readback != eeprom_orig:
            result2 = False
            self.logger.log_info("===== Check orig-readback fru-eeprom failed =====", console_print)
        else:
            result2 = True
            self.logger.log_info("===== Set orig fru-eeprom Success =====", console_print)

        exec_cmd('rm FRU.bin')

        return result1, result2

    def run_test(self):
        pass_cnt = 0
        fail_cnt = 0
        test_list = []
        slot_nums = exec_cmd("cat /sys/switch/slot/num")
        ret1 = exec_cmd("cat /sys/switch/slot/slot*/status").split("\n")

        for i in range(int(slot_nums)):
            if ret1[i] == '1':
                test_list.append('fan{}'.format(i + 1))

        for k in test_list:
            res1, res2 = self.write_slot_fru_eeprom('{}'.format(k), True)
            pass_cnt += 1 if res1 else 0
            fail_cnt += 1 if not res1 else 0
            pass_cnt += 1 if res2 else 0
            fail_cnt += 1 if not res2 else 0

        result = E.OK if pass_cnt == 2 * len(test_list) else E.EFAIL
        return result

