import re
from function import run_command
from test_case_base import TestCaseCommon
from errcode import E
key_list = ['Product Name', 'Part Number', 'Serial Number', 'Base MAC Address',
            'MAC Addresses', 'Platform Name', 'Vendor Name', 'Manufacturer',
            ]

# tlv_eeprom_read_tc test class
class TLV_EEPROM_READ_TC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "tlv_eeprom_read_tc"
        self.rec = ''
        self.code = 0
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)


    def check_eeprom_info(self, console_print=False):
        """
                    Read EEPROM Information
        """
        self.logger.log_info("[CHECK EEPROM INFO]:", console_print)

        cmd = 'decode-syseeprom'
        self.code, self.rec = run_command(cmd)

        flag = 0
        miss_list = []
        if self.code != 0:
            self.fail_reason.append("get syseeprom info fail.")
            result1 = False
        else:
            self.logger.log_info("Details:", console_print)
            self.logger.log_info(self.rec, console_print)
            for key in key_list:
                if key in str(self.rec):
                    pass
                else:
                    miss_list.append(key)
                    flag = 1
            if flag == 1:
                self.logger.log_info('Key:{} is Missing'.format(miss_list), console_print)
                return False
            else:
                result1 = True
        return result1

    def check_mac(self, console_print=False):
        """
                    Check Mac Information
        """
        if self.code != 0:
            return False
        self.logger.log_info('[CHECK EEPROM MAC:]', console_print)

        find_mac = re.findall(
            'Base MAC Address.+([\dabcdef]{2}:[\dabcdef]{2}:[\dabcdef]{2}:[\dabcdef]{2}:[\dabcdef]{2}:[\dabcdef]{2})',
            self.rec, re.IGNORECASE)
        if not find_mac:
            result2 = False
            self.fail_reason.append("find mac failed.")
            self.logger.log_info('FAIL', console_print)
            return result2
        else:
            mac_add = find_mac[0]
            self.logger.log_info("Mac address: {}".format(mac_add), console_print)
            check_mac1 = mac_add.count('0') != 12 and mac_add.upper().count('F') != 12
            if not check_mac1:
                self.fail_reason.append("invalid mac")
                self.logger.log_info('FAIL', console_print)
            check_mac2 = bin(int(mac_add[:2], 16))[-1] != '1'
            if not check_mac2:
                self.fail_reason.append("multicast mac")
                self.logger.log_info('FAIL', console_print)
            result2 = True if check_mac1 and check_mac2 else False
            self.logger.log_info('PASS', console_print)

        return result2

    def run_test(self):
        pass_cnt = 0
        fail_cnt = 0
        res1 = self.check_eeprom_info(True)
        res2 = self.check_mac(True)
        if res1:
            pass_cnt += 1
        else:
            fail_cnt += 1

        if res2:
            pass_cnt += 1
        else:
            fail_cnt += 1

        result = E.OK if res1 and res2 else E.EFAIL
        return [pass_cnt, fail_cnt, result]

