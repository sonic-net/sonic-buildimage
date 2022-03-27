# -*- coding: utf-8 -*-
import os
import json
import re
import time
from errcode import *
from collections import OrderedDict

TC_TAG_A = "auto"
TC_TAG_M = "manual"
TC_TAG_P = "production"
TC_TAG_D = "delivery"
TC_TAG_R = "rack"
TC_TAG_U = "utilities"
TC_TAG_DEF = "uninitialized"

# Used to validate test case tags
TC_TAG_VALID_LIST = [TC_TAG_A, TC_TAG_M, TC_TAG_P, TC_TAG_D, TC_TAG_R, TC_TAG_U]


# class TestCaseBase
class TestCaseBase(object):
    def __init__(self, index, case_cfg_json):
        # intermediate result should be placed here in the derived classes
        self.tags = []
        self.config = []
        self.index = index
        self.case_cfg_json = case_cfg_json
        self.__parse_config_json()
        pass

    def __parse_config_json(self):
        try:
            with open(self.case_cfg_json, 'r') as f:
                self.config = json.load(f, object_pairs_hook=OrderedDict)
                self.tags = self.config["tags"]
        except IOError as e:
            self.logger.log_err(str(e))
            raise SystemExit(1)

    def set_index(self, index):
        self.index = index

    def get_index(self):
        return self.index

    def get_tc_type(self):
        """ test case type: auto, manual, utilties"""
        return self.config["type"]

    def get_tc_tags(self):
        """ test case tags: production, delivery, rack """
        return self.config["tags"]

    def validate_tags(self):
        return self.config["tags"] in TC_TAG_VALID_LIST

    def get_tc_name(self):
        """ test case name """
        return self.config["name"]

    def get_tc_description(self):
        """ test case description """
        return self.config["description"]

    def get_tc_expectation(self):
        """ test case expectation """
        return self.config["expectation"]

    def run(self):
        # Should run derived class logic
        pass

    def generate_result(self, code):
        result = "Pass" if code == E.OK else "Fail"
        msg = "Test case {}: [{}] =======> {}".format(self.index, self.get_tc_name(), result)
        err = "ErrorCode: {}\nFail reasons: {}".format(code.name, self.fail_reason)
        
        return msg, err

    def generate_result_dict(self, code):
        result = {}
        result['testNo'] = self.index
        result['testItem'] = self.get_tc_name()
        result['startTime'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(self.start_time))
        result['endTime'] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(self.stop_time))
        result['duration'] = str(round(self.stop_time - self.start_time, 2)) + 's'
        result['result'] = "Pass" if code == E.OK else "Fail"
        result['errorCode'] = code.name
        result['failReason'] = self.fail_reason
        return result

    def record_result_dict_to_file(self, result_dict, json_file):
        with open(json_file, 'w') as f:
            json.dump(result_dict, f, indent=4, separators=(',', ': '))

    def get_start_time(self):
        self.fail_reason = []
        self.start_time = time.time()

    def get_stop_time(self):
        self.stop_time = time.time()

class TestCaseCommon(TestCaseBase):
    def __init__(self, index, module_name, logger, platform_case_cfg, case_cfg_json):
        """
        :param index: test case id
        :param module_name: test case name
        :param logger: logger
        :param platform_case_cfg: json file from "config/platform/--*Platform*--/case_config.json"
        """
        if not case_cfg_json:
            current_dir = os.path.dirname(__file__)
            case_cfg_file = os.path.join(current_dir, "../cases", module_name, "case_config.json")       # 需要创建测试case对应config.json
        else:
            case_cfg_file = case_cfg_json
        self.module_name = module_name
        self.platform_case_cfg = platform_case_cfg
        self.platform_cfg_json = None               # platform config content, json format
        self.logger = logger
        TestCaseBase.__init__(self, index, case_cfg_file)
        self.__read_platform_config()

    def __read_platform_config(self):
        """
                read platform config content
        """
        try:
            with open(self.platform_case_cfg, 'r') as f:
                content = json.load(f, object_pairs_hook=OrderedDict)
                self.platform_cfg_json = content
        except IOError:
            self.platform_cfg_json = None

    def get_module_name(self):
        return self.module_name
