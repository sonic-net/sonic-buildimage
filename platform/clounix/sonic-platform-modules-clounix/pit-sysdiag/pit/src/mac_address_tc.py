import os
import sys
import json
import re
from test_case import TestCaseCommon
from function import load_platform_util_module
from errcode import E

class MACADDRESSTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "baseutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "BaseUtil"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "mac_address_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.base_util = None
        self.mac_address_dict = None

        try:
            base_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
            platform_util_class = getattr(base_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.base_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        try:
            if self.platform_cfg_json and "mac_address" in self.platform_cfg_json.keys():
                self.mac_address_dict = self.platform_cfg_json["mac_address"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def check_mac_test(self, check_item, base_util_func, offset, base_mac, also_print_console=False):
        """
        check mac test

        @param:
            check_item: test item (cpu or bmc)
            base_util_func: base util get mac func
            offset: check mac offset (eg base_mac +1 or +2)
            base_mac: the mac for device
        @return:
            #(E.OK) for success, other for failure
        """
        ret = E.OK
        self.logger.log_info("[CHECK %s MAC]:" % check_item.upper(), also_print_console)
        item_mac = eval(base_util_func)()
        if item_mac == "N/A":
            ret = E.EMGMT11006
            self.fail_reason.append("get mac error, %s_mac:%s" % (check_item.lower(), item_mac.lower()))
        else:
            self.logger.log_info("%s_mac = %s, base_mac = %s" % (check_item.lower(), item_mac.lower(), base_mac.lower()), also_print_console)
            #check mac
            offset_mac = re.sub("(\w\w)(?=\w)", "\g<1>:", "{:012x}".format(int((int("0x"+base_mac.replace(":", ""), 16) + offset) & 0xffffffffffff)))
            if offset_mac.lower() != item_mac.lower():
                ret = E.EMGMT11008
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        ret = E.OK
        pass_cnt = 0
        fail_cnt = 0
        base_mac = self.base_util.get_base_mac()
        if base_mac == "N/A" or base_mac == None:
            ret = E.EMGMT11006
            self.fail_reason.append("get base_mac error")
            result = E.EFAIL
            fail_cnt += 1
            return [pass_cnt, fail_cnt, result]

        for check_item in self.mac_address_dict.get("check_list"):
            item_offset = "%s_offset" % (check_item)
            offset = self.mac_address_dict.get("offset")[item_offset]
            util_func_name = "self.base_util.get_%s_mac" % (check_item)

            ret = self.check_mac_test(check_item, util_func_name, offset, base_mac, True)
            if ret != E.OK:
                fail_cnt += 1
                break
            else:
                pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
