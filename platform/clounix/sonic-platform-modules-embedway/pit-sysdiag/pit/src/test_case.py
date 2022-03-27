import os
import json
import re
import time
from collections import OrderedDict
from errcode import E

CASE_CONFIG_JSON_FILE = "config.json"


TC_TAG_A = "auto"
TC_TAG_M = "manual"
TC_TAG_P = "production"
TC_TAG_D = "delivery"
TC_TAG_R = "rack"
TC_TAG_U = "utilities"
TC_TAG_DEF = "uninitialized"

# Used to validate test case tags
TC_TAG_VALID_LIST = [TC_TAG_A, TC_TAG_M, TC_TAG_P, TC_TAG_D, TC_TAG_R, TC_TAG_U]

class TCBase(object):
    def __init__(self, index, case_cfg_file):
        # self.logger = logger().getlogger()
        self.case_cfg_file = case_cfg_file
        self.config = []
        self.case_index = index
        #self.case_name = case_name
        self.values = []
        #self.success = True
        self.fail_reason = []
        self.__parse_config_json()
        
    def __parse_config_json(self):
        try:
            with open(self.case_cfg_file, 'r') as f:
                self.config = json.load(f, object_pairs_hook=OrderedDict)
        except IOError as e:
            self.logger.log_err(str(e))
            raise SystemExit(1)
    
    def set_index(self, index):
        self.case_index = index

    def get_index(self):
        return self.case_index
    
    def get_tc_type(self):
        """ test case type: auto, manual, utilties"""
        return self.config["type"]

    def get_tc_tags(self):
        """ test case tags: production, delivery, rack """
        return self.config["tags"]
    
    def validate_tags(self):
        return (self.config["tags"] in TC_TAG_VALID_LIST)
    
    def get_tc_name(self):
        """ test case name """
        return self.config["name"]

    def get_tc_description(self):
        """ test case description """
        return self.config["description"]

    def get_tc_expectation(self):
        """ test case expectation """
        return self.config["expectation"]

    def verify_by_regex(self, content, pattern):
        return re.match(content, pattern)
        
    def verify_by_range(self, content, pattern):
        if content in pattern:
            return True
        else:
            return False
    
    def verify_by_string(self, content, pattern):
        if content.find(pattern) >= 0:
            return True
        else:
            return False
    
    def verify(self, content, pattern, type):
        if type == "regex":
            return self.verify_by_regex(content, pattern)
        elif type == "range":
            return self.verify_by_range(content, pattern)
        elif type == "string":
            return self.verify_by_string(content, pattern)
        else:
            return False
    
    def run_test(self, *argv):
        """
        test case process function
        @argv: your input params
        @return: code, 0 for success. else for fail
        """
        pass

    def generate_result(self, code):
        result = "Pass" if code == E.OK else "Fail"
        msg = "Test case {}: [{}] =======> {}".format(self.case_index, self.get_tc_name(), result)
        err = "ErrorCode: {}\nFail reasons: {}".format(code.name, self.fail_reason)

        return msg, err
    
    def generate_result_dict(self, code):
        result = {}
        result['testNo'] = self.case_index
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
    

class TestCaseCommon(TCBase):
    def __init__(self, index, module_name, logger, platform_cfg_file, case_cfg_file=None):
        """
        @index: test case id;
        @module_name: test case name;
        @logger: logger
        @platform_cfg_file: platform config json file, "config/platform/***/case_config.json"
        @case_cfg_file: case config file, "config/cases/${module_name}/config.json"
        """
        if not case_cfg_file:
            current_dir = os.path.dirname(__file__)
            case_cfg_file = os.path.join(current_dir, "../cases", module_name, CASE_CONFIG_JSON_FILE)
        else:
            case_cfg_file = case_cfg_file
        self.module_name = module_name
        self.platform_cfg_file = platform_cfg_file
        self.platform_cfg_json = None               # platform config content, json format
        self.logger = logger
                
        TCBase.__init__(self, index, case_cfg_file)
        self.__read_platform_config()

    def __read_platform_config(self):
        """
        read platform config content
        """
        try:
            with open(self.platform_cfg_file, 'r') as f:
                content = json.load(f, object_pairs_hook=OrderedDict)
                self.platform_cfg_json = content
        except IOError :
            self.platform_cfg_json = None

    def get_module_name(self):
        return self.module_name


