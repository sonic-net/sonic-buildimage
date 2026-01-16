# -*- coding:utf-8
from pit_util_common import run_command
from test_case import TestCaseCommon
from errcode import E
import re

class SVIDTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "svid_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.svid_info_dict = None
        try:
            if self.platform_cfg_json and "svid_info" in self.platform_cfg_json.keys():
                self.svid_info_dict = self.platform_cfg_json["svid_info"]
            if self.platform_cfg_json and "voltage_margin_device" in self.platform_cfg_json.keys():
                self.voltage_margin_device_dict = self.platform_cfg_json["voltage_margin_device"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    # def svid_test(self, also_print_console=False):
    #     ret = E.EFAIL
    #     self.logger.log_info("[SVID HIGH VOLTAGE TEST]:", also_print_console)
    #     self.logger.log_info("setting voltage...\n", also_print_console)
    #     get_cmd = "pmonutil volget"
    #     cmd_list = [
    #         "pmonutil volset high",
    #         "pmonutil volset low",
    #         "pmonutil volset normal"
    #     ]
    #     for cmd in cmd_list:
    #         self.logger.log_info(
    #             "set svid {} voltage".format(
    #                 cmd.split()[2]),
    #             also_print_console)
    #         status, out = run_command(cmd)
    #         if status != 0 or len(out) < 0:
    #             self.fail_reason.append("set svid fail.")
    #             ret = E.EIO
    #             self.logger.log_err("FAIL!", also_print_console)
    #             return ret
    #         else:
    #             self.logger.log_info("set svid success!", also_print_console)
    #             get_status, get_log = run_command(get_cmd)
    #             if get_status != 0 or len(get_log) <= 0:
    #                 self.fail_reason.append("get svid fail.")
    #                 ret = E.EIO
    #                 self.logger.log_err("FAIL!", also_print_console)
    #                 return ret
    #             else:
    #                 switch_vol_test_readback = round(
    #                     int(get_log) / float(1000), 1)
    #                 if switch_vol_test_readback != self.svid_info_dict["{}_voltage".format(
    #                         format(cmd.split()[2]))]:
    #                     if cmd.split()[2] == "high":
    #                         ret = E.ESVID33001
    #                     elif cmd.split()[2] == "low":
    #                         ret = E.ESVID33002
    #                     elif cmd.split()[2] == "normal":
    #                         ret = E.ESVID33003
    #                     self.fail_reason.append(
    #                         "{} voltage not match.".format(
    #                             cmd.split()[2]))
    #                     self.logger.log_err("FAIL!", also_print_console)
    #                     return ret
    #                 else:
    #                     ret = E.OK
    #                     self.logger.log_info("current svid voltage: {}V\n".format(
    #                         switch_vol_test_readback), also_print_console)
    #     self.logger.log_info("PASS.", also_print_console)
    #     return ret

    def svid_level_test(self, device, volset_level, also_print_console=False):
        get_cmd = "margin-vol get {}".format(device) + " | awk '{print $3}'"
        set_high_val = self.svid_info_dict["{}".format(device)]['high_margin_val'] if "high_margin_val" in self.svid_info_dict["{}".format(device)].keys() else 2
        set_low_val = self.svid_info_dict["{}".format(device)]['low_margin_val'] if "low_margin_val" in self.svid_info_dict["{}".format(device)].keys() else -2
        if volset_level == "high":
            cmd = "margin-vol set -p {} {}".format(set_high_val, device)
        elif  volset_level == "low":
            cmd = "margin-vol set -p {} {}".format(set_low_val, device)
        elif volset_level == "normal":
            cmd = "margin-vol set -p 0 {}".format(device)
        else:
            return E.EFAIL

        status, out = run_command(cmd)
        if status != 0 or len(out) < 0:
            self.fail_reason.append("set svid fail.")
            ret = E.EIO
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        else:
            self.logger.log_info("set svid success!", also_print_console)
            get_status, get_log = run_command(get_cmd)
            if get_status != 0 or len(get_log) <= 0:
                self.fail_reason.append("get svid fail.")
                ret = E.EIO
                self.logger.log_err("FAIL!", also_print_console)
                return ret
            else:
                t1 =  re.match(r"(\d+)\s*[mv]*", get_log)
                if t1:
                    switch_vol_test_readback = int(t1.group(1).strip())
        
                self.logger.log_info("deivce:{},{}_volage read:{}".format(device, volset_level, switch_vol_test_readback), also_print_console)
                if switch_vol_test_readback > self.svid_info_dict["{}".format(device)]["{}_voltage".format(
                            format(volset_level))] + 10 or switch_vol_test_readback < self.svid_info_dict["{}".format(device)]["{}_voltage".format(
                            format(volset_level))] - 10:
                    if volset_level == "high":
                        ret = E.ESVID33001
                    elif volset_level == "low":
                        ret = E.ESVID33002
                    elif volset_level == "normal":
                        ret = E.ESVID33003
                    self.fail_reason.append(
                        "{} voltage not match.".format(
                            volset_level))
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret 
                else:
                    ret = E.OK
                    self.logger.log_info("current svid voltage: {}mV\n".format(
                            switch_vol_test_readback), also_print_console) 

        self.logger.log_info("PASS.", also_print_console)
        return ret     

    def svid_test(self, device, also_print_console=False):
        ret = E.OK
        self.logger.log_info("[SVID HIGH VOLTAGE TEST]:", also_print_console)
        self.logger.log_info("setting voltage...\n", also_print_console)
        
        cmd_list = [
            "high",
            "low",
            "normal"
        ]
        for cmd in cmd_list:
            test_ret = self.svid_level_test(device, cmd, True)
            if test_ret != E.OK:
                ret = test_ret 

        return ret

    def run_test(self, *argv):
        ret = E.EFAIL
        pass_cnt = 0
        fail_cnt = 0
        for key in self.voltage_margin_device_dict:
            ret = self.svid_test(key, True)
            if ret != E.OK:
                fail_cnt += 1
                break
            else:
                pass_cnt += 1
                   
        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
