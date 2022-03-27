import os
import re
import subprocess
from test_case import TestCaseCommon
from errcode import *
from pit_util_common import run_command


class DACPINTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "dac_pin_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.dac_info_dict = None
        try:
            if self.platform_cfg_json and "dac_info" in self.platform_cfg_json.keys():
                self.dac_info_dict = self.platform_cfg_json["dac_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def is_presence(self, port):
        presence_cmd = "sfputil show presence -p 'Ethernet{}' | grep -a 'Ethernet'".format(port)
        status_presence, log_presence = run_command(presence_cmd)
        if status_presence != 0 or len(log_presence) <= 0:
            return False
        else:
            if 'Not' in log_presence:
                return False
            else:
                return True

        return False

    def dac_eeprom_test(self, port_list, also_print_console):
        self.logger.log_info("[CHECK DAC EEPROM DEVICE]:", also_print_console)
        dac_type = self.dac_info_dict["dac_type"]
        qsfp_eeprom_dict = self.dac_info_dict[dac_type]
        for speed in qsfp_eeprom_dict["dac_port"]:
            port_num = qsfp_eeprom_dict[speed]["port_num"]
            port_setp = port_num[2]
            for number in range(port_num[0], port_num[1], port_setp):
                if number not in port_list:
                    continue
                self.logger.log_info("Test Ethernet{}".format(number), also_print_console)
                eeprom_cmd = "sfputil show eeprom -p Ethernet{}".format(number) + " | grep -a -E 'Identifier:|Vendor Name:|Vendor PN:|Vendor SN:' | grep -a -v 'Extended'| awk -F ':' '{print $2}'"
                status_eeprom, log_eeprom = run_command(eeprom_cmd)
                log_eeprom_list = log_eeprom.splitlines()
                if status_eeprom != 0 or len(log_eeprom_list) != 4:
                    self.fail_reason.append("port{} get dac eeprom fail.".format(number))
                    ret = E.EIO
                    return ret
                else:
                    ret = E.OK
                    match = 0
                    for identifier in qsfp_eeprom_dict[speed]["Identifier"]:
                        if identifier in log_eeprom_list[0]:
                            match = 1
                    if match == 0:
                        self.fail_reason.append("Ethernet{} eeprom fail: error Identifier".format(number))
                        ret = E.ESFP18008
                        return ret

                    match = 0
                    for vendor_name in qsfp_eeprom_dict[speed]["Vendor_Name"]:
                        if vendor_name in log_eeprom_list[1]:
                            match = 1
                    if match == 0:
                        print("Warning: DAC Vendor name not match on Ethernet{}".format(number))

                    if len(log_eeprom_list[2]) <= qsfp_eeprom_dict[speed]["Vendor_PN_len"][0] or len(
                               log_eeprom_list[2]) >= qsfp_eeprom_dict[speed]["Vendor_PN_len"][1]:
                        self.fail_reason.append(
                            "Ethernet{} eeprom fail: Vendor PN".format(number))
                        ret = E.ESFP18008
                        return ret

                    if len(log_eeprom_list[3]) < qsfp_eeprom_dict[speed]["Vendor_SN_len"][0] or len(
                               log_eeprom_list[3]) >= qsfp_eeprom_dict[speed]["Vendor_SN_len"][1]:
                        self.fail_reason.append(
                            "Ethernet{} eeprom fail: error Vendor SN".format(number))
                        ret = E.ESFP18008
                        return ret
        return E.OK

    def dac_reset_sfp_port_test(self, port_list, also_print_console=False):
        self.logger.log_info("[CHECK DAC RESET SWITCH DEVICE]:", also_print_console)
        dac_type = self.dac_info_dict["dac_type"]
        qsfp_eeprom_dict = self.dac_info_dict[dac_type]
        ret = E.OK
        for speed in qsfp_eeprom_dict["dac_port"]:
            port_num = qsfp_eeprom_dict[speed]["port_num"]
            port_setp = port_num[2]
            for number in range(port_num[0], port_num[1], port_setp):
                if number in port_list:
                    self.logger.log_info("Reset Test Ethernet{}".format(number), also_print_console)
                    reset_switch_cmd = "sfputil reset Ethernet{} ".format(number)
                    status_reset_port, log_reset_port = run_command(reset_switch_cmd)
                    if status_reset_port != 0 or len(log_reset_port) <= 0:
                        self.fail_reason.append("dac reset fail.")
                        ret = E.EIO
                    else:
                        if "OK" not in log_reset_port:
                            self.logger.log_info("Ethernet{} reset Fail".format(number),also_print_console)
                            ret = E.EIO
                        else:
                            if self.is_presence(number) != True:
                                ret = E.EIO
                                self.logger.log_info("Ethernet{} Presence err".format(number),also_print_console)
                            else:
                                self.logger.log_info("Ethernet{} reset OK".format(number),also_print_console)

        return ret

    def generate_presence_list(self, also_print_console):
        active_port_list = []
        self.logger.log_info("[CHECK SWITCH PORT]:", also_print_console)
        dac_type = self.dac_info_dict["dac_type"]
        qsfp_eeprom_dict = self.dac_info_dict[dac_type]
        for speed in qsfp_eeprom_dict["dac_port"]:
            port_num = qsfp_eeprom_dict[speed]["port_num"]
            port_setp = port_num[2]
            for number in range(port_num[0], port_num[1], port_setp):
                if self.is_presence(number) == True:
                    active_port_list.append(number)
                else:
                    self.logger.log_info("dac status: Ethernet{} Not Present, Skip".format(number),
                    also_print_console)
        return active_port_list

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        port_list = self.generate_presence_list(True)

        dac_type = self.dac_info_dict["dac_type"]
        if dac_type == "QSFP":
            ret = self.dac_eeprom_test(port_list, True)
            if ret != E.OK:
                fail_cnt += 1
            else:
                pass_cnt += 1

            ret = self.dac_reset_sfp_port_test(port_list, True)
            if ret != E.OK:
                fail_cnt += 1
            else:
                pass_cnt += 1
        elif dac_type == "SFP":
            pass
        else:
            pass

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
