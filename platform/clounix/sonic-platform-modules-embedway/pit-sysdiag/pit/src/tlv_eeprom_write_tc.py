import json
import os
import re
from function import run_command, convert_unicode
from test_case_base import TestCaseCommon
from errcode import E
import time

key_list = ['Product Name', 'Part Number', 'Serial Number', 'Base MAC Address',
            'MAC Addresses', 'Platform Name', 'Vendor Name', 'Manufacturer']

SYSEEPROM_TEST = "../config/common/syseeprom_test.json"
SYSEEPROM_ORIG = "../config/common/syseeprom_orig.json"


# TLV_EEPROM_WRITE test class
class TLV_EEPROM_WRITE_TC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "tlv_eeprom_write_tc"
        self.rec = ''
        self.code = 0
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)


    def parse_syseeprom(self, console_print=False):
        #cmd = 'show platform syseeprom'
        cmd = 'decode-syseeprom'
        res = run_command(cmd)

        if res[0] != 0:
            self.fail_reason.append("get syseeprom info fail.")
            return False
        else:
            eeprom_list = res[1].split("\n")
            eeprom_result = {}
            for lines in eeprom_list:
                if "0x" in lines and "CRC" not in lines:
                    key = lines.split("  ")[0]
                    val_find = re.findall("0x.......(.*)", lines)
                    if val_find:
                        eeprom_result[key] = val_find[0]
                    else:
                        self.logger.log_info("===== Parse tlv-eeprom Failed! =====", console_print)
                        return False

        return eeprom_result


    def write_eeprom_info(self, console_print=False):
        """
                    STORE&SET EEPROM Information
        """
        cmd = 'decode-syseeprom'
        pass_cnt = 0
        fail_cnt = 0
        current_dir = os.path.dirname(os.path.abspath(__file__))
        self.logger.log_info("[RECORD EEPROM INFO]:", console_print)

        # STORE Orig tlv-eeprom
        syseeprom_orig = self.parse_syseeprom()
        os.system("echo '' > " + SYSEEPROM_ORIG)
        with open(os.path.join(current_dir, SYSEEPROM_ORIG), 'w') as f:
            json.dump(syseeprom_orig, f, indent=4, separators=(',', ': '))

        os.system("python syseeprom-write.py {}".format(SYSEEPROM_TEST))

        os.system("rm /var/cache/sonic/decode-syseeprom/syseeprom_cache")
        code, rec = run_command(cmd)
        self.logger.log_info("------||. test tlv-syseeprom after set .||------", console_print)
        self.logger.log_info(rec, console_print)
        syseeprom_test_readback = self.parse_syseeprom()
        with open(os.path.join(current_dir, SYSEEPROM_TEST), 'r') as f:
            syseeprom_test = json.load(f)

        syseeprom_test = convert_unicode(syseeprom_test)
        syseeprom_test_readback = convert_unicode(syseeprom_test_readback)
        if syseeprom_test_readback != syseeprom_test:
            result1 = False
            self.logger.log_info("===== Check test tlv-eeprom failed =====", console_print)
        else:
            result1 = True
            self.logger.log_info("===== Set test tlv-eeprom Success =====", console_print)

        os.system("python syseeprom-write.py {}".format(SYSEEPROM_ORIG))

        os.system("rm /var/cache/sonic/decode-syseeprom/syseeprom_cache")
        code, rec = run_command(cmd)
        self.logger.log_info("------||. orig tlv-syseeprom after set .||------", console_print)
        self.logger.log_info(rec, console_print)
        syseeprom_orig_readback = self.parse_syseeprom()
        if syseeprom_orig_readback != syseeprom_orig:
            result2 = False
            self.logger.log_info("===== Check orig tlv-eeprom failed =====", console_print)
        else:
            result2 = True
            self.logger.log_info("===== Set orig tlv-eeprom Success =====", console_print)

        if result1:
            pass_cnt += 1
        else:
            fail_cnt += 1

        if result2:
            pass_cnt += 1
        else:
            fail_cnt += 1

        result = E.OK if result1 and result2 else E.EFAIL

        return pass_cnt, fail_cnt, result


    def run_test(self, *argv):
        pass_cnt, fail_cnt, result = self.write_eeprom_info(True)
        return [pass_cnt, fail_cnt, result]

