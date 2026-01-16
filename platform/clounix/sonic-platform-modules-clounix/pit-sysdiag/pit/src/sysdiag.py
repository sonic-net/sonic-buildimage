#!/usr/bin/python
# -*- coding: UTF-8 -*-

from function import get_platform_and_hwsku, run_command
import imp
import inspect
import os
import signal
import sys
import time
import json
import getopt
import argparse
from optparse import OptionParser
import threading
from tabulate import tabulate
sys.path.append(os.path.dirname(__file__))
import syslogger
from errcode import *
from banner import banner_string
from test_case import TestCaseCommon
import traceback
from pit_util_common import *
import pit_util_common


THIS_MODULE = "SYSDIAG"
VERSION = 1.1
TITLE_PRINT = True
# Diag tags
DIAG_TAG_DELIVERY = "delivery"
DIAG_TAG_MANUFACTURE = "manufacture"
DIAG_TAG_USER = "user"
DIAG_TAG_EMC = "emc"
DIAG_TAG_POWER = "power"
DIAG_TAG_PA = "pa"
DIAG_TAG_STRESS = "stress"
DIAG_TAG_SMOKE = "smoke"
DIAG_TAG_DEFAULT = DIAG_TAG_DELIVERY
DIAG_TAGS = [
    DIAG_TAG_DELIVERY,
    DIAG_TAG_MANUFACTURE,
    DIAG_TAG_EMC,
    DIAG_TAG_POWER,
    DIAG_TAG_PA,
    DIAG_TAG_STRESS,
    DIAG_TAG_SMOKE]

# TestCases list
DELIVERY_TEST_CASE_LIST = []
MANUFACTURE_TEST_CASE_LIST = []
USER_TEST_CASE_LIST = []

CMD_DIAG_TEST = "diag"
CMD_SINGLE_TEST = "single"
CMD_LIST_TEST = "list"
CMD_FAILED_CASE = "failed"
CMD_DEFAULT = CMD_DIAG_TEST

PERFORM_TEST_ARGS = ["start", "stop"]

# case types
CASE_TYPE_AUTO = "auto"
CASE_TYPE_MANUAL = "manual"
CASE_TYPE_UTILITY = "utility"
CASE_TYPE_DEFAULT = CASE_TYPE_AUTO
CASE_TYPES = [CASE_TYPE_AUTO, CASE_TYPE_MANUAL, CASE_TYPE_UTILITY]

LOG_VERBOSE = False
LOG_QUIET = False
LOG_LANG = "en"

# global param, signal "Ctrl C"
is_sigint_up = False

# find classes define from .py
def get_classes(arg):
    def _valid_cls(attr):
        if inspect.isclass(attr) and \
            attr.__bases__[0].__name__ == TestCaseCommon.__name__:
            return True
        else:
            return False
    classes = []
    clsmembers = inspect.getmembers(arg, _valid_cls)
    for (name, _) in clsmembers:
        classes.append(name)
    return classes


# Diag types
CMD_DEFAULT = CMD_DIAG_TEST
DIAG_TYPE_DELIVERY = "delivery"
DIAG_TYPE_MANUFACTURE = "manufacture"
DIAG_TAG_EMC = "emc"
DIAG_TAG_POWER = "power"
DIAG_TAG_PA = "pa"
DIAG_TYPE_USER = "user"

# Psydo type, list all cases
DIAG_TYPE_DEFAULT = DIAG_TYPE_DELIVERY
DIAG_TYPES = [DIAG_TYPE_DELIVERY, DIAG_TYPE_MANUFACTURE, DIAG_TAG_EMC, DIAG_TAG_POWER, DIAG_TAG_PA]

SYSDIAG_PLATFORM_CFG_FILE = "platform_config.json"
SYSDIAG_PLATFORM_CASE_CFG_FILE = "case_config.json"
SYSDIAG_PLATFORM_PRE_SCRIPT = "pre_handle.sh"

SYSDIAG_PLATFORM_CASE_CFG_FILE_MAP = {
    "D-1539"    : "case_config_d1539.json",
    "D-1627"    : "case_config_d1627.json",
    "D-1732"    : "case_config_d1732.json",
    "D-1735"    : "case_config_d1735.json",
    "D-1712"    : "case_config_d1712.json",
    "D-1715"    : "case_config_d1715.json",
    "KX-U6780A" : "case_config_kxu6780.json",
}

# case types
CASE_TYPE_AUTO = "auto"
CASE_TYPE_MANUAL = "manual"
CASE_TYPE_UTILITY = "utility"
CASE_TYPE_DEFAULT =  CASE_TYPE_AUTO
CASE_TYPES = [CASE_TYPE_AUTO, CASE_TYPE_MANUAL, CASE_TYPE_UTILITY]
SPLITER = "================================================================="
BANNER = """
  ____ ___ _____           ____            ____  _             
 |  _ \_ _|_   _|         / ___| _   _ ___|  _ \(_) __ _  __ _ 
 | |_) | |  | |    _____  \___ \| | | / __| | | | |/ _` |/ _` |
 |  __/| |  | |   |_____|  ___) | |_| \__ \ |_| | | (_| | (_| |
 |_|  |___| |_|           |____/ \__, |___/____/|_|\__,_|\__, |
                                 |___/                   |___/ 
"""

# Global vars
#############################################################
g_lang = "EN"

USAGE = '''
sysdiag

    System diagnostic test utility

Usage:
    sysdiag [FUNCTION] [OPTIONS...]

FUNCTION:
    -v, --version                      Show sysdiag version
    -d, --diag [-g <tag>] [-t <type>]  Run diagnostic test, default tag is delivery
    -s, --single <casefile>            Run single test, test case file should be given
    -l, --list [-g <tag>]              Show all possible cases, default tag is delivery

OPTIONS:
    -h, --help                         Print sysdiag user guide
    -c, --chinese                      Test outputs in Chinese
    -q, --quiet                        don't print status messages to stdout")
    -t, --type                         <auto|manual|utility>    Specify type of test cases, default type is auto
    -g, --tag                          <delivery|manufacture|emc|power|pa|smoke>  Specify tag of test cases, default tag is delivery
    -i, --interval                     <min>    Set test interval when loop test, unit minute
    -p, --pre handle                   pre handle
'''
# global param, signal "Ctrl C"
is_sigint_up = False

# class SysDiag
class SysDiag(object):
    def __init__(self, logger):
        self.log_json_file = "/var/log/"
        self.filename = "/var/log/sysdiag_result.json"
        self.log = logger
        # command default config
        self.tag = DIAG_TAG_DEFAULT
        self.type = CASE_TYPE_DEFAULT
        self.cmd = CMD_DEFAULT
        self.signal_test_case = None
        # test case list and result
        self.test_case_list = []
        self.test_case_files = []
        self.test_result = None
        self.pass_case_cnt = 0
        self.pass_case_dict = {}
        self.fail_case_cnt = 0
        self.fail_case_dict = {}
        self.cases_result = {}
        self.diag_run_flag = False
        self.run_case_list = []
        self.run_case_flag = True
        self.run_fail_list = []
        # console print on-off
        self.console_print_flag = True
        # run interval, unit second
        self.interval = 0
        # pre handle
        self.pre_handle_scrip = ''
        self.pre_handle = False
        # case stress test times, eg. cpld r/w test 10000 times.
        self.case_test_count = 1
        self.init_options()

    def show_banner(self):
        banner = banner_string
        self.log.log_info(banner, True)

    def init_options(self):
        self.arg_parser = argparse.ArgumentParser(add_help=False)
        self.opt_parser = OptionParser(add_help_option=False)
        self.opt_parser.add_option("-v", "--version", action="store_true", dest="version", help="Show sysdiag version")
        self.opt_parser.add_option("-d", "--diag", action="store_false", dest="diag",help="Run diagnostic test, default tag is delivery")
        self.opt_parser.add_option("-s", "--single", action="store_true", dest="single", help="Run single test, test case file should be given")
        self.opt_parser.add_option("-l", "--list", action="store_true", dest="list", help="Show all possible cases, default tag is delivery")
        self.opt_parser.add_option("-h", "--help", action="store_true", dest="help", help="Print sysdiag user guide")
        self.opt_parser.add_option("-c", "--chinese", action="store_false", dest="chinese", help="Test outputs in Chinese")
        self.opt_parser.add_option("-q", "--quiet", action="store_true", dest="quiet", help="don't print status messages to stdout")
        self.opt_parser.add_option("-t", "--type", action="store_true", dest="type", help="<auto|manual|utility>   Specify type of test cases, default type is auto")
        self.opt_parser.add_option("-g", "--tag", action="store_true", dest="tag", help="<delivery|manufacture|emc|power|pa|smoke>  Specify tag of test cases, default tag is delivery")
        self.opt_parser.add_option("-i", "--interval", action="store_true", dest="interval", help="<second> Set test interval when loop test, unit second")
        self.opt_parser.add_option("-p", "--pre", action="store_true", dest="pre", help="pre handle")
        self.opt_parser.add_option("-n", "--count", action="store_true", dest="count", help="set case run loop times")

    def parse_opts_and_args(self):
        global LOG_LANG

        try:
            opts, args = getopt.getopt(sys.argv[1:], "vhdcqs:lpt:g:i:f:n:",
                                       ["version", "help", "diag", "chinese",
                                        "quiet", "single=", "list", "pre",
                                        "type=", "tag=", "interval=", "failed",
                                        "count="])
        except getopt.GetoptError:
            print("\nYour inputs is invalid!")
            sys.exit(IO_ERR)

        self.log.log_info(
            "Parse input options and arguments ...",
            self.console_print_flag)
        for opt, opt_val in opts:
            if opt in ("-h", "--help"):
                sysdiag_usage(NO_ERR)
            if opt in ('-v', '--version'):
                self.log.log_info(
                    "sysdiag version: {0}".format(VERSION),
                    self.console_print_flag)
                sys.exit(0)
            if opt in ('-d', "--diag"):
                self.cmd = CMD_DIAG_TEST
            elif opt in ('-f', '--failed'):
                self.run_case_flag = False
            elif opt in ('-c', "--chinese"):
                LOG_LANG = "cn"
            # add tag config, to support emc/ac-power/pa test
            elif opt in ("-g", "--tag"):
                self.tag = opt_val
                if opt_val not in DIAG_TAGS:
                    self.log.log_err(
                        "Invalid input, case tag must be {}".format(DIAG_TAGS),
                        self.console_print_flag)
                    sys.exit(IO_ERR)
            elif opt in ("-q", "--quiet"):
                self.console_print_flag = False
                self.set_logger(self.console_print_flag)
            elif opt in ("-s", "--single"):
                self.cmd = CMD_SINGLE_TEST
                self.signal_test_case = opt_val
            elif opt in ("-l", "--list"):
                self.cmd = CMD_LIST_TEST
            elif opt in ("-t", "--type"):
                if opt_val in CASE_TYPES:
                    self.type = opt_val
                else:
                    self.log.log_err(
                        "Invalid type, must be {}".format(CASE_TYPES),
                        self.console_print_flag)
                    sys.exit(IO_ERR)
            elif opt in ("-i", "--interval"):
                self.interval = int(opt_val)
            elif opt in ("-p", "--pre"):
                self.pre_handle = True
            elif opt in ("-n", "--count"):
                self.case_test_count = int(opt_val)
            else:
                sysdiag_usage(IO_ERR)

        return 0

    def get_cpu_model_name(self):
        ret, out = run_command("lscpu |grep 'Model name'")
        #self.log.log_info(out, self.console_print_flag)
        return out
    
    def get_platform_case_cfg(self):
        model_name = self.get_cpu_model_name()
        for key, value in SYSDIAG_PLATFORM_CASE_CFG_FILE_MAP.items():
            if key in model_name:
                return value

        self.log.log_err("PIT Not Support Platform ...", self.console_print_flag)
        sys.exit(IO_ERR)

    def generate_test_case_list(self):
        (platform, hwsku) = get_platform_and_hwsku()
        SYSDIAG_PLATFORM_CASE_CFG_FILE = self.get_platform_case_cfg()
        try:
            current_dir = os.path.dirname(os.path.abspath(__file__))
            platform_case_config = os.path.join(
                current_dir,
                "../config/platform",
                platform,
                SYSDIAG_PLATFORM_CFG_FILE)
            platform_config = os.path.join(
                current_dir,
                "../config/platform",
                platform,
                SYSDIAG_PLATFORM_CASE_CFG_FILE)
            self.pre_handle_scrip = os.path.join(
                current_dir,
                "../config/platform",
                platform,
                SYSDIAG_PLATFORM_PRE_SCRIPT)
            with open(platform_case_config, 'r') as f:
                diag_cfg_content = json.load(f)

        except IOError as e:
            self.log.log_err(str(e), self.console_print_flag)
            self.log.log_err(traceback.format_exc(), self.console_print_flag)
            raise SystemExit(1)

        self.log.log_info(
            "Generate test case list ...",
            self.console_print_flag)
        index = 1

        if self.signal_test_case:
            diag_case_list = [self.signal_test_case]
        else:
            diag_case_list = diag_cfg_content['test_cases']

        for case_file_name in diag_case_list:
            try:
                case_path = os.path.join(
                    os.path.dirname(__file__),
                    case_file_name + '.py')
                case_module = imp.load_source(case_file_name, case_path)
            except Exception as e:
                self.log.log_err(str(e), self.console_print_flag)
                self.log.log_err(
                    traceback.format_exc(),
                    self.console_print_flag)
                raise SystemExit(IO_ERR)

            try:
                case_class_name = get_classes(case_module)[0]
                case_class = getattr(case_module, case_class_name)
                case = case_class(index, self.log, platform_config)
            except Exception as e:
                self.log.log_err(str(e), self.console_print_flag)
                self.log.log_err(
                    traceback.format_exc(),
                    self.console_print_flag)
                raise SystemExit(IO_ERR)

            self.test_case_list.append(case)
            self.test_case_files.append(case_file_name)
            index = index + 1
        return

    def run_one_case(self, tc):
        case_name = tc.get_tc_name()

        ret = E.EFAIL
        try:
            self.log.log_info(
                "====================  {}  ====================".format(case_name),
                self.console_print_flag)
            tc.get_start_time()
            ret = tc.run_test()
            tc.get_stop_time()

            self.diag_run_flag = True
            msg, err = tc.generate_result(ret[-1])
            self.log.log_info(msg, self.console_print_flag)

            case_result = {}
            tc_file = self.test_case_files[self.test_case_list.index(tc)]
            tc_file_path = os.path.join("/var/log", tc_file + '.json')

            os.system("rm -f {}".format(tc_file_path))
            result_dict = tc.generate_result_dict(ret[-1])
            tc.record_result_dict_to_file(result_dict, tc_file_path)
            case_result[tc.get_tc_name()] = result_dict
            self.cases_result.update(case_result)

            if ret[-1] != E.OK:
                self.fail_case_cnt += 1
                self.pass_case_dict[case_name] = ret[0]
                self.fail_case_dict[case_name] = ret[1]
                self.log.log_err(err, self.console_print_flag)
            else:
                self.pass_case_cnt += 1
                self.pass_case_dict[case_name] = ret[0]
                self.fail_case_dict[case_name] = ret[1]
            # record run test_case name
            if case_name not in self.run_case_list:
                self.run_case_list.append(case_name)

        except Exception as e:
            self.log.log_err(str(e), self.console_print_flag)
            self.fail_case_cnt += 1
            if self.fail_case_dict.get(case_name):
                self.fail_case_dict[case_name] = self.fail_case_dict[case_name] + 1
            else:
                self.fail_case_dict[case_name] = 1
            # record run test_case name
            if case_name not in self.run_case_list:
                self.run_case_list.append(case_name)
        return

    def run(self):
        self.parse_opts_and_args()
        if self.run_case_flag:
            self.generate_test_case_list()
        else:
            self.fail_case_test()

        if not self.test_case_list:
            self.log.log_warn(
                "No test cases generated.",
                self.console_print_flag)
            return

        if self.cmd == CMD_LIST_TEST:
            self.print_case_list(self.tag)
            return

        self.log.log_info("Start switch diag ...", self.console_print_flag)
        # pre handle
        if self.pre_handle:
            if os.path.exists(self.pre_handle_scrip):
                ret, out = run_command(self.pre_handle_scrip)
                self.log.log_info(out, self.console_print_flag)
            else:
                self.log.log_warn("miss pre handle script", self.console_print_flag)

        self.log.log_info("Start switch diag ...", self.console_print_flag)

        # pre handle
        if self.pre_handle:
            if os.path.exists(self.pre_handle_scrip):
                ret, out = run_command(self.pre_handle_scrip)
                self.log.log_info(out, self.console_print_flag)
            else:
                self.log.log_info("=======> Miss pre handle script <=======",
                                  self.console_print_flag)

        self.log.log_info("Start switch diag ...", self.console_print_flag)
        # pre handle
        if self.pre_handle:
            if os.path.exists(self.pre_handle_scrip):
                ret, out = run_command(self.pre_handle_scrip)
                self.log.log_info(out, self.console_print_flag)
            else:
                self.log.log_warn(
                    "miss pre handle script",
                    self.console_print_flag)

        # main loop
        while True:
            for tc in self.test_case_list:
                case_tags = tc.get_tc_tags()
                case_name = tc.get_tc_name()

                # if Ctrl C catched, set interval to 0
                if is_sigint_up:
                    self.interval = 0
                    break

                # single case test
                if self.cmd == CMD_SINGLE_TEST:
                    self.run_one_case(tc)
                    break
                # diag test
                elif self.cmd in [CMD_DIAG_TEST]:
                    if self.tag in case_tags:
                        # auto test
                        if self.type == tc.get_tc_type():
                            self.run_one_case(tc)
                        else:
                            # manual/utility test
                            pass
                else:
                    pass

            if self.interval:
                time.sleep(self.interval * 60)
            else:
                break

        return

    def generate_final_result(self):
        if not self.diag_run_flag:
            return NORUN_ERR

        self.log.log_info(
            "=============================================",
            self.console_print_flag)
        self.log.log_info(
            "Generate final test result...",
            self.console_print_flag)

        if self.fail_case_cnt > 0:
            self.test_result = 'fail'
            ret = TEST_FAIL_ERR
        else:
            self.test_result = 'pass'
            ret = NO_ERR

        self.log.log_info(
            "System diagnostic summary: tag {}, result {}, total {}, failed {}".format(
                self.tag,
                self.test_result,
                self.pass_case_cnt +
                self.fail_case_cnt,
                self.fail_case_cnt),
            self.console_print_flag)

        self.log.log_info("Details: ", self.console_print_flag)

        header = ["CaseName", 'PassCnt', 'FailCnt', 'Result']
        detail_table = []
        for case_name in self.run_case_list:
            pass_cnt = self.pass_case_dict.get(case_name, 0)
            fail_cnt = self.fail_case_dict.get(case_name, 0)
            result = "FAIL" if fail_cnt else "PASS"
            detail_table.append(
                [case_name, str(pass_cnt), str(fail_cnt), result])
        if len(detail_table) > 0:
            self.log.log_info(
                tabulate(
                    detail_table,
                    header,
                    tablefmt="grid"),
                self.console_print_flag)

        # record result to jsonfile
        self.print_final_result_to_json()
        return ret

    def print_final_result_to_json(self):
        result = {}
        info = {}
        info['final_result'] = self.test_result
        info['total_count'] = self.pass_case_cnt + self.fail_case_cnt
        info['failed_count'] = self.fail_case_cnt
        info['detailed_result'] = {}
        info['detailed_result']['pass_case'] = list(self.pass_case_dict.keys())[0]
        info['detailed_result']['failed_case'] = list(self.fail_case_dict.keys())[0]
        result["result"] = info
        result["details"] = self.cases_result
        search_cmd = "ls {} | grep sysdiag_result | wc -l".format(
            self.log_json_file)
        search_status, search_log = run_command(search_cmd)
        if search_status != 0 or int(search_log) < 0:
            self.log.log_info("find filename error!", self.console_print_flag)
        else:
            if int(search_log) == 0:
                log_name = self.log_json_file + "sysdiag_result.json"
            else:
                log_name = self.log_json_file + \
                    "sysdiag_result{}.json".format(int(search_log))
        with open(log_name, "w") as f:
            json.dump(result, f, indent=4, separators=(',', ': '))
        json_list = []
        for files in os.listdir(self.log_json_file):
            if "sysdiag_result" in files:
                json_list.append(files)
        sorting = pit_util_common.sort_list()
        log_list = sorting.sort_humanly(json_list)
        if int(search_log) >= 1:
            for index in range(len(log_list) - 1, 0, -1):
                oldfile = os.path.join(
                    self.log_json_file,
                    "sysdiag_result{}.json".format(index))
                newfile = os.path.join(
                    self.log_json_file,
                    "sysdiag_result{}.json".format(
                        index + 1))
                os.rename(oldfile, newfile)
            run_command(
                "mv {} {}".format(
                    "{}sysdiag_result.json".format(
                        self.log_json_file), "{}sysdiag_result1.json".format(
                            self.log_json_file)))
            oldfile = os.path.join(
                self.log_json_file,
                "sysdiag_result{}.json".format(
                    len(log_list)))
            newfile = os.path.join(self.log_json_file, "sysdiag_result.json")
            os.rename(oldfile, newfile)

    def print_case_list(self, case_tag):
        self.log.log_info(
            "==================== TestCase List ====================",
            self.console_print_flag)
        header = ["Index", "CaseFile", "CaseName", "Type", "Description"]
        tc_list = []
        for tc in self.test_case_list:
            tc_tags = tc.get_tc_tags()
            if case_tag in tc_tags or case_tag == '':
                id = tc.get_index()
                tc_name = tc.get_tc_name()
                tc_type = tc.get_tc_type()
                tc_desc = tc.get_tc_description()
                tc_file = self.test_case_files[self.test_case_list.index(tc)]
                tc_list.append([id, tc_file, tc_name, tc_type, tc_desc])
        if len(tc_list) > 0:
            self.log.log_info(
                tabulate(
                    tc_list,
                    header,
                    tablefmt="simple"),
                self.console_print_flag)

    def fail_case_test(self):
        with open(self.filename) as f:
            data = json.load(f)
            self.fail_run_case_list = data["result"]["detailed_result"]["failed_case"]
            for case in self.fail_run_case_list:
                self.run_fail_list.append(case.replace("-test", "_tc"))
        index = 1
        (platform, hwsku) = get_platform_and_hwsku()
        current_dir = os.path.dirname(os.path.abspath(__file__))
        platform_case_config = os.path.join(
            current_dir,
            "../config/platform",
            platform,
            SYSDIAG_PLATFORM_CFG_FILE)
        for case_fail_name in self.run_fail_list:
            try:
                case_path = os.path.join(
                    os.path.dirname(__file__),
                    case_fail_name + '.py')
                case_module = imp.load_source(case_fail_name, case_path)
            except Exception as e:
                self.log.log_err(str(e), self.console_print_flag)
                self.log.log_err(
                    traceback.format_exc(),
                    self.console_print_flag)
                raise SystemExit(IO_ERR)
            try:
                case_class_name = get_classes(case_module)[0]
                case_class = getattr(case_module, case_class_name)
                case = case_class(index, self.log, platform_case_config)
            except Exception as e:
                self.log.log_err(str(e), self.console_print_flag)
                self.log.log_err(
                    traceback.format_exc(),
                    self.console_print_flag)
                raise SystemExit(IO_ERR)
            self.test_case_list.append(case)
            self.test_case_files.append(case_fail_name)
            index = index + 1
        return

    def set_logger(self, flag):
        self.log.set_logger_also_print_console(flag)


############# common function ##############

def sysdiag_usage(exit_code):
    print("")
    print("sysdiag")
    print("")
    print("    System diagnostic test utility")
    print("")
    print("Usage:")
    print("    sysdiag [FUNCTION] [OPTIONS...]")
    print("")
    print("FUNCTION:")
    print("    -v, --version                      Show sysdiag version")
    print("    -d, --diag [-g <tag>] [-t <type>]  Run diagnostic test, default tag is delivery")
    print("    -s, --single <casefile>            Run single test, test case file should be given")
    print("    -l, --list [-g <tag>]              Show all possible cases, default tag is delivery")
    print("    -f, --failed                       Test last failed case again")
    print("")
    print("OPTIONS:")
    print("    -h, --help                         Print sysdiag user guide")
    print("    -c, --chinese                      Test outputs in Chinese")
    print("    -q, --quiet                        Do not show outputs")
    print("    -t, --type <auto|manual|utility>   Specify type of test cases, default type is auto")
    print("    -g, --tag <delivery|manufacture|emc|power|pa|smoke>   Specify tag of test cases, default tag is delivery")
    print("    -i, --interval <min>               Set test interval when loop test, unit minute")
    print("    -p, --pre handle                   pre handle")
    print("    -n, --count <times>                stress test times for one case")
    sys.exit(exit_code)


def signal_handler(signum, frame):
    global is_sigint_up
    is_sigint_up = True
    print('\033[1;42mCatched interrupt signal! Waiting for test exit ...\033[0m')


def sysdiag_main():
    logger = None
    diag = None
    try:
        logger = syslogger.sys_log_init_logger(THIS_MODULE)
    except Exception as e:
        err_msg = "Failed to init syslogger"
        logger.log_err(err_msg)
        raise IOError(err_msg)

    # Create SysDiag
    try:
        diag = SysDiag(logger)
    except Exception as e:
        err_msg = "Create SysDiag instance failed: %s" % str(e)
        logger.log_err(err_msg)
        return IO_ERR

    signal.signal(signal.SIGINT, signal_handler)
    time_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(time.time()))
    if TITLE_PRINT:
        spliter = "================================================================="
        diag.log.log_info(spliter, True)
        diag.show_banner()
        diag.log.log_info(spliter, True)
        print("\n         {}\n".format(time_str))
    diag.log.log_info("Start to Diagnose Switch Hardware ...", True)
    diag.run()
    ret = diag.generate_final_result()
    return ret


if __name__ == "__main__":
    ret = sysdiag_main()
    exit(ret)
