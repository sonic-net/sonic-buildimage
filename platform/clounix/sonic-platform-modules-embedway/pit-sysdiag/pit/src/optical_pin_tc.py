import os
import re
import subprocess
from test_case import TestCaseCommon
from errcode import *
from pit_util_common import run_command


class OPTICALPINTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "optical_pin_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.optical_info_dict = None
        try:
            if self.platform_cfg_json and "optical_info" in self.platform_cfg_json.keys():
                self.optical_info_dict = self.platform_cfg_json["optical_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def check_presence(self, also_print_console):
        presence_reset = []
        self.logger.log_info(
            "[CHECK OPTICAL PRESENCE DEVICE]:",
            also_print_console)
        presence_cmd = "transceiverutil show presence | grep -a 'Ethernet' | awk '{print $2}'"
        status_presence, log_presence = run_command(presence_cmd)
        if status_presence != 0 or len(log_presence) <= 0:
            self.fail_reason.append("get optical fail.")
            ret = E.EIO
        else:
            ret = E.OK
            presence_list = log_presence.splitlines()
            for index, presence in enumerate(presence_list):
                if presence == 'Not':
                    presence_reset.append(index)
        return ret, presence_reset

    def optical_presence_test(self, also_print_console):
        all_presence_continue = False
        ret, presence_reset = self.check_presence(also_print_console)
        if ret != E.OK:
            return all_presence_continue, ret
        if len(presence_reset) != 0:
            self.logger.log_info(
                "[RESET OPTICAL PRESENCE]:",
                also_print_console)
            for presence_reset_index in presence_reset:
                reset_cmd = "transceiverutil reset Ethernet{}".format(
                    presence_reset_index)
                status_reset, log_reset = run_command(reset_cmd)
                if status_reset != 0 or len(log_reset) <= 0:
                    self.fail_reason.append("reset optical fail.")
                    ret = E.EIO
                    return all_presence_continue, ret
                else:
                    self.logger.log_info(log_reset, also_print_console)

            ret, presence_reset = self.check_presence(also_print_console)
            if len(presence_reset) != 0:
                ret = E.ESFP18001
                self.fail_reason.append(
                    "please check number of:{} optical status ".format(presence_reset))
                return all_presence_continue, ret
            else:
                ret = E.OK
                all_presence_continue = True
                self.logger.log_info(
                    "optical status: Present",
                    also_print_console)

        else:
            ret = E.OK
            all_presence_continue = True
            self.logger.log_info("optical status: Present", also_print_console)
        return all_presence_continue, ret

    def optical_eeprom_test(self, also_print_console=False):
        self.logger.log_info(
            "[CHECK OPTICAL EEPROM DEVICE]:",
            also_print_console)

        optical_type = self.optical_info_dict["optical_type"]
        qsfp_eeprom_dict = self.optical_info_dict[optical_type]

        optical_port = qsfp_eeprom_dict["optical_port"][0]
        port_num = qsfp_eeprom_dict[optical_port]["port_num"]
        for number in range(port_num[0], port_num[1]):
            eeprom_cmd = "transceiverutil show eeprom | grep -a -A 20 'Ethernet{}:'".format(
                number) + " | grep -a -E 'Identifier:|Vendor Name:|Vendor PN:|Vendor SN:' | grep -a -v 'Extended'| awk -F ': ' '{print $2}'"
            status_eeprom, log_eeprom = run_command(eeprom_cmd)
            log_eeprom_list = log_eeprom.splitlines()
            if status_eeprom != 0 or len(log_eeprom_list) != 4:
                self.fail_reason.append("port{} get optical eeprom fail.".format(number))
                ret = E.EIO
                return ret
            else:
                ret = E.OK
                if log_eeprom_list[0].strip(
                ) not in qsfp_eeprom_dict[optical_port]["Identifier"]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Identifier".format(number))
                    ret = E.ESFP18008
                    return ret
                if log_eeprom_list[1].strip(
                ) not in qsfp_eeprom_dict[optical_port]["Vendor_Name"]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: Vendor Name".format(number))
                    ret = E.ESFP18008
                    return ret
                if len(log_eeprom_list[2].strip()) <= qsfp_eeprom_dict[optical_port]["Vendor_PN_len"][0] or len(
                        log_eeprom_list[2].strip()) >= qsfp_eeprom_dict[optical_port]["Vendor_PN_len"][1]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: Vendor PN".format(number))
                    ret = E.ESFP18008
                    return ret
                if len(log_eeprom_list[3].strip()) <= qsfp_eeprom_dict[optical_port]["Vendor_SN_len"][0] or len(
                        log_eeprom_list[3].strip()) >= qsfp_eeprom_dict[optical_port]["Vendor_SN_len"][1]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Vendor SN".format(number))
                    ret = E.ESFP18008
                    return ret

        optical_port = qsfp_eeprom_dict["optical_port"][1]
        port_num = qsfp_eeprom_dict[optical_port]["port_num"]
        for number in range(port_num[0], port_num[1]):
            eeprom_cmd = "transceiverutil show eeprom | grep -a -A 10 'Ethernet{}:'".format(
                number) + " | grep -a -E 'Identifier:|Vendor Name:|Vendor Part Number|Vendor Serial Number:'|awk -F ': ' '{print $2}'"
            status_eeprom, log_eeprom = run_command(eeprom_cmd)
            log_eeprom_list = log_eeprom.splitlines()
            if status_eeprom != 0 or len(log_eeprom_list) != 4:
                self.fail_reason.append("get optical eeprom fail.")
                ret = E.EIO
            else:
                ret = E.OK
                if log_eeprom_list[0].strip(
                ) not in qsfp_eeprom_dict[optical_port]["Identifier"]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Identifier".format(number))
                    ret = E.ESFP18008
                    return ret
                if log_eeprom_list[1].strip(
                ) not in qsfp_eeprom_dict[optical_port]["Vendor_Name"]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Vendor Name".format(number))
                    ret = E.ESFP18008
                    return ret
                if len(log_eeprom_list[2].strip()) <= qsfp_eeprom_dict[optical_port]["Vendor_Part_Number_len"][0] or len(
                        log_eeprom_list[2].strip()) >= qsfp_eeprom_dict[optical_port]["Vendor_Part_Number_len"][1]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Vendor Part Number".format(number))
                    ret = E.ESFP18008
                    return ret
                if len(log_eeprom_list[3].strip()) <= qsfp_eeprom_dict[optical_port]["Vendor_Serial_Number_len"][0] or len(
                        log_eeprom_list[3].strip()) >= qsfp_eeprom_dict[optical_port]["Vendor_Serial_Number_len"][1]:
                    self.fail_reason.append(
                        "Ethernet{} eeprom fail: error Vendor Serial Number".format(number))
                    ret = E.ESFP18008
                    return ret
        self.logger.log_info("optical eeprom check: Pass", also_print_console)
        return ret

    def optical_tx_switch_test(self, also_print_console=False):
        self.logger.log_info(
            "[CHECK OPTICAL TX SWITCH DEVICE]:",
            also_print_console)
        intr_cmd = "transceiverutil show intr"
        status_intr, log_intr = run_command(intr_cmd)
        if status_intr != 0 or len(log_intr) <= 0:
            self.fail_reason.append("get optical fail.")
            ret = E.EIO
        else:
            ret = E.OK
            self.logger.log_info("read optical intr OK", also_print_console)
        if ret == E.OK:
            modsel_cmd = "transceiverutil show modsel"
            status_modsel, log_modsel = run_command(modsel_cmd)
            if status_modsel != 0 or len(log_modsel) <= 0:
                self.fail_reason.append("get optical fail.")
                ret = E.EIO
            else:
                ret = E.OK
                self.logger.log_info("read optical modsel OK", also_print_console)
            if ret == E.OK:
                lpmode_switch_control = ['off', 'on']
                lpmode_switch_status = ['On', 'Off']
                optical_type = self.optical_info_dict["optical_type"]
                qsfp_eeprom_dict = self.optical_info_dict[optical_type]
                total_portnum = qsfp_eeprom_dict["total_portnum"]
                for control in lpmode_switch_control:
                    for number in range(0, int(total_portnum)):
                        lpmode_switch_cmd = "transceiverutil lpmode {} Ethernet{}".format(
                            control, number)
                        status_lpmode, log_lpmode = run_command(lpmode_switch_cmd)
                        if status_lpmode != 0 or len(log_lpmode) <= 0:
                            self.fail_reason.append("lpmode switch fail.")
                            ret = E.EIO
                        else:
                            lpmode_switch_readback_cmd = "transceiverutil show lpmode | grep -a 'Ethernet{} '".format(
                                number) + "|awk '{print $2}'"
                            status_lpmode_switch, log_lpmode_switch = run_command(
                                lpmode_switch_readback_cmd)
                            if status_lpmode_switch != 0 or len(
                                    log_lpmode_switch) <= 0:
                                self.fail_reason.append("lpmode switch fail.")
                                ret = E.EIO
                            else:
                                if log_lpmode_switch != lpmode_switch_status[lpmode_switch_control.index(
                                        control)]:
                                    ret = E.ESFP18015
                                    self.fail_reason.append(
                                        "Ethernet{} lpmode switch fail.".format(number))
                                    return ret
                                else:
                                    ret = E.OK
        self.logger.log_info("lpmode switch OK", also_print_console)
        return ret

    def optical_reset_switch_test(self, also_print_console=False):
        self.logger.log_info(
            "[CHECK OPTICAL RESET SWITCH DEVICE]:",
            also_print_console)
        optical_type = self.optical_info_dict["optical_type"]
        qsfp_eeprom_dict = self.optical_info_dict[optical_type]
        total_portnum = qsfp_eeprom_dict["total_portnum"]
        for number in range(0, int(total_portnum)):
            reset_switch_cmd = "transceiverutil reset Ethernet{}".format(
                number)
            status_reset_switch, log_rest_switch = run_command(
                reset_switch_cmd)
            if status_reset_switch != 0 or len(log_rest_switch) <= 0:
                self.fail_reason.append("optical reset fail.")
                ret = E.EIO
            else:
                ret = E.OK
                self.logger.log_info(
                    "Ethernet{} reset OK".format(number),
                    also_print_console)
        if ret == E.OK:
            reset_cmd = "transceiverutil show reset"
            status_reset, log_rest = run_command(reset_cmd)
            if status_reset != 0 or len(log_rest) <= 0:
                self.fail_reason.append("reset optical fail.")
                ret = E.EIO
            else:
                if "Not" in log_rest:
                    ret = E.ESFP18016
                    self.fail_reason.append("optical reset switch fail.")
                else:
                    ret = E.OK
                    self.logger.log_info("reset switch OK", also_print_console)
        return ret

    def run_test(self, *argv):
        all_presence_continue, ret = self.optical_presence_test(True)
        if ret != E.OK:
            return ret
        elif all_presence_continue:
            optical_type = self.optical_info_dict["optical_type"]
            if optical_type == "QSFP":
                ret = self.optical_eeprom_test(True)
                if ret != E.OK:
                    return ret
                ret = self.optical_tx_switch_test(True)
                if ret != E.OK:
                    return ret
                ret = self.optical_reset_switch_test(True)
                if ret != E.OK:
                    return ret
                ret = self.optical_eeprom_test(True)
                if ret != E.OK:
                    return ret
            elif optical_type == "SFP":
                pass
            else:
                pass
        return ret
