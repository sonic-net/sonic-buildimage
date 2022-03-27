import re
from function import run_command, exec_cmd
from test_case_base import TestCaseCommon
from errcode import E
key_list = ['Board Product Name', 'Board Part Number', 'Board Serial Number', 'Board Manufacturer', 'Board Part number', 'Mfg. Date/Time'
            ]

# SLOT_EEPROM_READ test class
class SLOT_EEPROM_READ_TC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "slot_eeprom_read_tc"
        self.rec = ''
        self.code = 0
        self.info_dic = {}
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)


    def check_eeprom_info(self, replaceable_part, console_print=False):
        """
                    Read&Check FRU-EEPROM Information
        """
        self.logger.log_info("[ CHECK FRU-EEPROM INFO ]: ==>{}<==".format(replaceable_part), console_print)

        cmd = 'python fruidutil.py get {}'.format(replaceable_part)
        self.code, self.rec = run_command(cmd)

        if self.code != 0:
            self.fail_reason.append("get eeprom info fail.")
            return False
        elif 'failed' not in self.rec:
            self.logger.log_info("Details:", console_print)
            self.logger.log_info(self.rec, console_print)


        for key in key_list:
            if key not in self.rec:
                self.logger.log_info('Key word {} is missing'.format(key), console_print)
            else:
                val = re.findall('{}+ +?: ([\S ]+)'.format(key), self.rec, re.IGNORECASE)
                if not val:
                    self.logger.log_info('{} info is missing'.format(key), console_print)
                else:
                    self.info_dic[key] = val[0]

        if len(self.info_dic) != len(key_list):
            self.logger.log_info('FAILED', console_print)
            return False
        else:
            self.logger.log_info('PASS', console_print)
            return True

    def check_value(self, console_print=False):
        """
                    Check Mac Information
        """
        if self.code != 0:
            return False
        self.logger.log_info('[ CHECK FRU EEPROM VALUE ]', console_print)


        if not self.info_dic:
            self.fail_reason.append('Missing Fru Keyword')
            self.logger.log_info("Missing Fru Keyword", console_print)
            return False
        for key in self.info_dic:
            val = self.info_dic[key]
            if len(set(val)) < 2:
                self.fail_reason.append("{} Value is not conform: {}".format(key, val))
                self.logger.log_info("{} Value is not conform: {}".format(key, val), console_print)
                return False
        self.logger.log_info("PASS",  console_print)
        return True


    def run_test(self):
        pass_cnt = 0
        fail_cnt = 0
        test_list = []
        slot_nums = exec_cmd("cat /sys/switch/slot/num")
        ret1 = exec_cmd("cat /sys/switch/slot/slot*/status").split("\n")

        for i in range(int(slot_nums)):
            if ret1[i] == '1':
                test_list.append('slot{}'.format(i + 1))

        for k in test_list:
            self.info_dic = {}
            res1 = self.check_eeprom_info('{}'.format(k), True)
            res2 = self.check_value(True)
            pass_cnt += 1 if res1 else 0
            fail_cnt += 1 if not res1 else 0
            pass_cnt += 1 if res2 else 0
            fail_cnt += 1 if not res2 else 0




        result = E.OK if pass_cnt == 2 * len(test_list) else E.EFAIL
        return result

