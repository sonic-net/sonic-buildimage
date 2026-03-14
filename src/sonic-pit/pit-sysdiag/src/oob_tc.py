import sys
from test_case import TestCaseCommon
from errcode import E
from function import run_command, load_platform_util_module
import traceback


class OOBTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "bmcutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "BmcUtil"
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "oob_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.server_ip = None # external ip
        self.bmc_ip = None # BMC internal ip

        try:
            if self.platform_cfg_json and 'server_ip' in self.platform_cfg_json.keys():
                self.server_ip = self.platform_cfg_json['server_ip']
            if self.platform_cfg_json and 'bmc_ip' in self.platform_cfg_json.keys():
                self.bmc_ip = self.platform_cfg_json['bmc_ip']
        except Exception as e:
            self.logger.log_err(str(e))
            self.logger.log_err(traceback.format_exc())

        bmc_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            bmc_util_class = getattr(bmc_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.bmc_util = bmc_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

    def cpu_ping_bmc_test(self, also_print_console=True):
        self.logger.log_info("cpu ping bmc test start")

        ret = E.OK
        if not self.bmc_ip:
            self.logger.log_dbg("BMC not exist, skip")
        else:
            count = 5
            cmd = "ping %s -c %d -I eth0.4088 | grep received" % (self.bmc_ip, count)
            status, output = run_command(cmd)
            self.logger.log_info(output, also_print_console)

            if output.find(" 0% packet loss") > 0:
                ret = E.OK
            else:
                self.log_reason("cpu ping bmc lost packages")
                ret = E.EMGMT11003

        if ret != E.OK:
            self.logger.log_err("cpu ping bmc test done, FAILED.", also_print_console)
        else:
            self.logger.log_err("cpu ping bmc test done, PASS.", also_print_console)

        return ret

    def cpu_ping_server_test(self, also_print_console=True):
        self.logger.log_info("cpu ping server test start")

        ret = E.OK
        if not self.server_ip:
            self.logger.log_dbg("External server ip not set, skip")
        else:
            count = 5
            ping_cmd = "ping %s -c %d -I eth0 | grep received" % (self.server_ip, count)
            status, output = run_command(ping_cmd)
            self.logger.log_info(output, also_print_console)

            if output.find(" 0% packet loss") > 0:
                ret = E.OK
            else:
                self.log_reason("cpu ping server lost packages")
                ret = E.EMGMT11002

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret

    def bmc_ping_server_test(self, also_print_console=True):
        self.logger.log_info("bmc ping server test start")

        count = 5
        ping_cmd = "ping %s -c %d -I eth0 | grep received" % (self.server_ip, count)
        try:
            status, output = self.bmc_util.exec_raw_cmd(ping_cmd)
            self.logger.log_info(output[0], also_print_console)

            if status and output[0].find(" 0% packet loss") > 0:
                ret = E.OK
            else:
                self.log_reason.append("bmc ping server lost packages")
                ret = E.EMGMT11004
        except Exception as e:
            self.log_reason("bmc ping server exception: {}".format(str(e)))
            self.logger.log_err(traceback.format_exc())
            ret = E.EMGMT11004

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret

    def run_test(self, *argv):
        final_ret = E.OK

        try:
            ret = self.cpu_ping_bmc_test()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "cpu ping bmc test exception: {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret = self.cpu_ping_server_test()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "cpu ping server test exception: {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        try:
            ret =  self.bmc_ping_server_test()
            if ret != E.OK:
                final_ret = ret
        except Exception as e:
            reason = "bmc ping server test exception: {}".format(str(e))
            self.log_reason(reason)
            self.logger.log_err(traceback.format_exc())

        return ret
