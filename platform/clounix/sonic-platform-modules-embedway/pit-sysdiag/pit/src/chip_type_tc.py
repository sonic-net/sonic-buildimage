import os
import sys
import json
import re
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import load_platform_util_module

class CHIPTYPETC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "chiptypeutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "ChipTypeUtil"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "chip_type_tc" # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.chip_type_util = None
        self.chip_type_cfg = None

        try:
            chip_type_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
            platform_util_class = getattr(chip_type_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.chip_type_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        try:
            if self.platform_cfg_json and "chip_type" in self.platform_cfg_json.keys():
                self.chip_type_cfg = self.platform_cfg_json["chip_type"]
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

    def deal_chip_type_check_test(self, chip_item, chiptype_util_func, util_return_json, also_print_console=False):
        """
        deal firmware version check test

        @param:
            chip_item: chip test item (board or fpga or cpld ...)
            chiptype_util_func: chiptype util get chiptype version func
            util_return_json: chiptype util func return vlaue is json format (True or False)
        @return:
            #(E.OK) for success, other for failure
        """
        ret = E.OK
        self.logger.log_info("[{} CHECK]:".format(chip_item.upper().replace("_", " ")), also_print_console)
        config_type = self.get_case_config_file_value(chip_item, also_print_console)
        if config_type == None:
            ret = E.EFW17007
        else:
            chip_type = eval(chiptype_util_func)()
            if chip_type == None or str(chip_type) in "N/A" or (util_return_json == True and type(chip_type) != dict):
                self.fail_reason.append("get chip type failed")
                ret = E.EFW17001
            elif util_return_json == False:
                self.logger.log_info("read type is: %s" % (chip_type), also_print_console)
            else:
                header = ["NAME","TYPE"]
                status_table = []
                for key,value in chip_type.items():
                    status_table.append([key,value])
                if len(status_table) > 0:
                    self.logger.log_info("chip type is:", also_print_console)
                    self.logger.log_info(tabulate(status_table, header, tablefmt="simple"), also_print_console)
            if config_type != chip_type:
                self.fail_reason.append("check chip type is wrong! config_type:{}, chip_type:{}".format(config_type, chip_type))
                ret = E.EFW17002
        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        """
        util_return_dict_map
        chip type test item    util return vlaue is josn(str or dict)
        "cpld":         True,
        """
        util_return_dict_map = {
            "board":         False,
            "cpld":         True,
            "fpga":         False
        }
        ret = E.OK
        pass_cnt = 0
        fail_cnt = 0
        for chip in self.chip_type_cfg:
            util_return_json = util_return_dict_map.get(chip, None)
            if util_return_json != None:
                test_item = "%s_type" % (chip)
                util_func_name = "self.chip_type_util.get_%s_type" % (chip)
                ret = self.deal_chip_type_check_test(test_item, util_func_name, util_return_json, True)
                if ret != E.OK:
                    fail_cnt += 1
                    break
                else:
                    pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
