from function import exec_cmd
from function import run_command
from test_case_base import TestCaseCommon
from errcode import E
import re
import ast
import os
import time

key_list = ['Product Name', 'Part Number', 'Serial Number', 'Manufacturer',
            # "AaAa", "BbBb", "CcCc"
            ]
FRU_TEST = "../config/common/test.bin"
FRU_ORIG = "../config/common/orig.bin"
CMD_SHOW_PLATFORM = "show platform"
CMD_FRUIDUTIL_GET = "fruidutil get"
CMD_FRUIDUTIL_SET = "fruidutil set"
WRITE_PROTECT_ENABLE = "1"
WRITE_PROTECT_DISABLE = "0"

# REPLACEABLE_EEPROM_WRITE test class
class REPLACEABLE_EEPROM_WRITE_TC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "replaceable_eeprom_write_tc"
        self.rec = ''
        self.code = 0
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.fan_syseeprom_test = {
            "-m":"ClounixTest",
            "-p":"CL8000-IOBTest",
            "-n":"CL8000-IOBTest",
            "-v":"02",
            "-s":"CLX0000002Test",
            "-t":"CLXTest",
        }
        self.fan_syseeprom_cmd = {
            "productManufacturer":"-m",
            "productName":"-p",
            "productPartModelName":"-n",
            "productVersion":"-v",
            "productSerialNumber":"-s",
            "productAssetTag":"-t",
        }

    def write_replaceable_frueeprom(self, replaceable_part, console_print=False):
        """
                    STORE&SET EEPROM Information by xxx.bin
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

    def fan_eeprom_write_protect(self, value):
        cmd = " ".join(["echo", value, "> /sys_switch/fan/eepromwp"])
        output = exec_cmd(cmd)

    def set_fan_syseeprom(self, syseeprom_info):
        self.fan_eeprom_write_protect(WRITE_PROTECT_DISABLE)
        for key, value in syseeprom_info.items():
            cmd = " ".join([CMD_FRUIDUTIL_SET, "fanboard", key, value])
            print(cmd)
            err, output = run_command(cmd)
        self.fan_eeprom_write_protect(WRITE_PROTECT_ENABLE)

    def read_fan_syseeprom(self):
        cmd = " ".join([CMD_FRUIDUTIL_GET, "fanboard"])
        output = exec_cmd(cmd)
        regex_int = re.compile(r'(.+?)\s*:\s*(.*)')
        tlv_ignore = ["version", "length", "language", "fruFileId"]
        syseeprom_content = {}
        output = output.decode()
        for line in output.splitlines():
            t1 = regex_int.match(line)
            if t1:
                    tlv_name = t1.group(1).strip()
                    if(tlv_name in tlv_ignore):
                        continue
                    tlv_cmd = self.fan_syseeprom_cmd[tlv_name]
                    syseeprom_content[tlv_cmd] = t1.group(2).strip()
                    #print("Name: %s; Value: %s" % (tlv_name, syseeprom_content[tlv_cmd]))

        return syseeprom_content

    def test_fan_syseeprom_write(self, console_print=False):
        fan_syseeprom_orig = {}
        fan_syseeprom_test_readback = {}
        fan_syseeprom_orig_readback = {}
        fan_syseeprom_orig = self.read_fan_syseeprom()

        self.set_fan_syseeprom(self.fan_syseeprom_test)
        time.sleep(3)
        fan_syseeprom_test_readback = self.read_fan_syseeprom()
        if ast.literal_eval(str(self.fan_syseeprom_test)) != fan_syseeprom_test_readback:
            result1 = False
            self.logger.log_info("===== Check test fru-eeprom failed =====", console_print)
        else:
            result1 = True
            self.logger.log_info("===== Set test fru-eeprom Success =====", console_print)

        self.set_fan_syseeprom(fan_syseeprom_orig)
        fan_syseeprom_orig_readback = self.read_fan_syseeprom()

        if fan_syseeprom_orig_readback != fan_syseeprom_orig:
            result2 = False
            self.logger.log_info("===== Check orig-readback fru-eeprom failed =====", console_print)
        else:
            result2 = True
            self.logger.log_info("===== Set orig fru-eeprom Success =====", console_print)

        return result1,result2

    def run_test(self):
        pass_cnt = 0
        fail_cnt = 0
        test_list = []
        fan_nums = exec_cmd("cat /sys_switch/fan/number")
        psu_nums = exec_cmd("psuutil numpsus")
        ret1 = exec_cmd("cat /sys_switch/fan/fan*/status").replace(b'\n', b'').decode()
        ret1 = list(ret1)
        ret2 = exec_cmd("cat /sys_switch/psu/psu*/out_status").replace(b'\n', b'').decode()
        ret2 = list(ret2)

        for i in range(int(fan_nums)):
            if ret1[i] == '1':
                test_list.append('fan{}'.format(i + 1))

        for i in range(int(psu_nums)):
            if ret2[i] == '1':
                test_list.append('psu{}'.format(i + 1))

        # for k in test_list:
        #     res1, res2 = self.write_replaceable_frueeprom('{}'.format(k), True)
        #     pass_cnt += 1 if res1 else 0
        #     fail_cnt += 1 if not res1 else 0
        #     pass_cnt += 1 if res2 else 0
        #     fail_cnt += 1 if not res2 else 0
        
        res1, res2  = self.test_fan_syseeprom_write(True)
        pass_cnt += 1 if res1 else 0
        fail_cnt += 1 if not res1 else 0
        pass_cnt += 1 if res2 else 0
        fail_cnt += 1 if not res2 else 0

        result = E.OK if fail_cnt==0 else E.EFAIL
        return [pass_cnt, fail_cnt, result]
