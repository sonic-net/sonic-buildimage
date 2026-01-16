import os
import sys
import json
import re
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import load_platform_util_module

class FIRMWAREVERSIONTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "fwmgrutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "FwMgrUtil"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "firmware_version_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.firmware_util = None
        self.firmware_version_cfg = None

        try:
            firmware_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
            platform_util_class = getattr(firmware_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.firmware_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        try:
            if self.platform_cfg_json and "firmware_version" in self.platform_cfg_json.keys():
                self.firmware_version_cfg = self.platform_cfg_json["firmware_version"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def get_case_config_file_value(self, key, also_print_console=False):
        config_value = None
        try:
            config_value = self.platform_cfg_json[key]
        except KeyError:
            err = "no {} in case_config.json file".format(key)
            self.fail_reason.append(err)
        except Exception as e:
            self.fail_reason.append(str(e))

        return config_value

    def deal_firmware_version_check_test(self, firmware_item, firmware_util_func, util_return_json, also_print_console=False):
        """
        deal firmware version check test

        @param:
            firmware_item: firmware test item (bmc or bios or cpld ...)
            firmware_util_func: firmware util get firmware version func
            util_return_json: firmware util func return vlaue is json format (True or False)
        @return:
            #(E.OK) for success, other for failure
        """
        ret = E.OK
        self.logger.log_info("[{} CHECK]:".format(firmware_item.upper().replace("_", " ")), also_print_console)
        config_version = self.get_case_config_file_value(firmware_item, also_print_console)
        if config_version == None:
            ret = E.EFW17007
        else:
            firmware_version = eval(firmware_util_func)()
            if firmware_version == None or str(firmware_version) in "N/A" or (util_return_json == True and type(firmware_version) != dict):
                self.fail_reason.append("get firmware version failed")
                ret = E.EFW17001
            elif util_return_json == False:
                self.logger.log_info("read version is: %s" % (firmware_version), also_print_console)
            else:
                header = ["NAME","VERSION"]
                status_table = []
                for key,value in firmware_version.items():
                    status_table.append([key,value])
                if len(status_table) > 0:
                    self.logger.log_info("read version is:", also_print_console)
                    self.logger.log_info(tabulate(status_table, header, tablefmt="simple"), also_print_console)
                config_version = json.dumps(config_version)
                firmware_version = json.dumps(firmware_version)
            print(config_version)
            print(firmware_version)
            if isinstance(firmware_version,int):
                if firmware_version < int(config_version,16):
                    self.fail_reason.append("check firmware version is wrong! config_version:{}, firmware_version:{}".format(config_version, firmware_version))
                    ret = E.EFW17002     
            else:
               if config_version not in firmware_version:
                    self.fail_reason.append("check firmware version is wrong! config_version:{}, firmware_version:{}".format(config_version, firmware_version))
                    ret = E.EFW17002
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        """
        util_return_dict_map
        firmware test item    util return vlaue is josn(str or dict)
            "onie":                     False
        """
        util_return_dict_map = {
            "onie":         False,
            "bmc":          False,
            "cpld":         True,
            "bios":         False,
            "fpga":         False,
            "sdk":          False,
            "switch_pcie":  True,
            "uboot":        False
        }
        ret = E.OK
        pass_cnt = 0
        fail_cnt = 0
        for version in self.firmware_version_cfg:
            util_return_json = util_return_dict_map.get(version, None)
            if util_return_json != None:
                test_item = "%s_version" % (version)
                util_func_name = "self.firmware_util.get_%s_version" % (version)
                ret = self.deal_firmware_version_check_test(test_item, util_func_name, util_return_json, True)
                if ret != E.OK:
                    fail_cnt += 1
                    break
                else:
                    pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
