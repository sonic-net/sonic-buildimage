# -*- coding:utf-8
import time
import re
import sys
import os

from posixpath import abspath
from errcode import E
from typing import Pattern
from pit_util_common import run_command, load_platform_util_module
from test_case import TestCaseCommon


class BMCUPGRADETC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "fwmgrutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "FwMgrUtil"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "bmcupgrade_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME,
                                logger, platform_cfg_file, case_cfg_file)
        fw_upgrade_module = load_platform_util_module(
            self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(
                fw_upgrade_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.bmc = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)
        self.bmcupgrade_info_dict = None
        try:
            if self.platform_cfg_json and "bmcupgrade_info" in self.platform_cfg_json.keys():
                self.bmcupgrade_info_dict = self.platform_cfg_json["bmcupgrade_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def bmc_upgrade_test(self, also_print_console=False):
        self.logger.log_info("[UPGRADE BMC TEST]:", also_print_console)
        if not os.path.exists(self.bmcupgrade_info_dict['flash_path']):
            self.fail_reason.append("get bmc upgrade file fail")
            ret = E.EFWBMCUPGRADE29010
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        bmc_version_running = self.bmc.get_bmc_version()
        bmc_flash_running = self.bmc.get_bmc_flash()
        if bmc_version_running == "N/A":
            self.fail_reason.append("get bmc version fail")
            ret = E.EFWBMCUPGRADE29007
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        elif bmc_flash_running == "N/A":
            self.fail_reason.append("get bmc flash fail")
            ret = E.EFWBMCUPGRADE29008
            self.logger.log_err("FAIL!", also_print_console)
            return ret
        else:
            self.logger.log_info("current version:  {}    {}".format(
                bmc_flash_running, bmc_version_running), also_print_console)
            if "master" in bmc_flash_running:
                upgrade = "slave"
            else:
                upgrade = "master"
            ret = self.bmc_update(upgrade)
            if ret == E.OK:
                time.sleep(90)
                bmc_version_test_readback = self.bmc.get_bmc_version()
                bmc_flash_test_readback = self.bmc.get_bmc_flash()
                self.logger.log_info(
                    "Updated version: {}   {}".format(
                        bmc_flash_test_readback,
                        bmc_version_test_readback),
                    also_print_console)
                if bmc_flash_running != bmc_flash_test_readback:
                    self.logger.log_info(
                        "bmc flash should be not match", also_print_console)
                    self.logger.log_info(
                        "bmc_flash_running:{} after_change ---> bmc_flash_test_readback:{}".format(
                            bmc_flash_running, bmc_flash_test_readback), also_print_console)
                    pattern = self.bmcupgrade_info_dict["bmc_pattern"]
                    bmc_version_test_readback = re.findall(
                        pattern, bmc_version_test_readback)[0]
                    self.logger.log_info("bmc_version_test_readback: {} ".format(
                        bmc_version_test_readback), also_print_console)
                    self.logger.log_info(
                        "bmcupgrade_info_dict[version_test]:  {} ".format(
                            self.bmcupgrade_info_dict['version_test']),
                        also_print_console)
                    if bmc_version_test_readback != self.bmcupgrade_info_dict['version_test']:
                        self.fail_reason.append("bmc version not match.")
                        ret = E.EFWBMCUPGRADE29005
                        self.logger.log_err("FAIL!", also_print_console)
                        return ret
                    else:
                        self.logger.log_info(
                            "bmc version match", also_print_console)
                else:
                    self.fail_reason.append("bmc flash match")
                    ret = E.EFWBMCUPGRADE29006
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
            else:
                self.logger.log_err("FAIL!", also_print_console)
                return ret
        self.logger.log_info("PASS.", also_print_console)
        return ret

    def bmc_update(self, upgrade, also_print_console=True):
        self.logger.log_info("upgrade standby flash [{}]".format(
            upgrade), also_print_console)
        if self.bmc.firmware_upgrade(
                "bmc",
                "{}".format(
                    self.bmcupgrade_info_dict['flash_path']),
                "{}".format(upgrade)):
            if self.bmc.set_bmc_boot_flash("{}".format(upgrade)):
                time.sleep(10)
                self.logger.log_info(
                    "set boot flash to [{}] success, please wait for reboot! \n".format(upgrade),
                    also_print_console)
                if not self.ping_bmc(True):
                    self.fail_reason.append("ping bmc fail,time out")
                    ret = E.EFWBMCUPGRADE29009
                    return ret
                else:
                    ret = E.OK
                    self.logger.log_info("reboot successfully! \n",also_print_console)
            else:
                self.fail_reason.append(
                    "set boot flash to [{}] fail".format(upgrade))
                ret = E.EFWBMCUPGRADE29003
                return ret
        else:
            self.fail_reason.append("bmc upgrade [{}] fail".format(upgrade))
            ret = E.EFWBMCUPGRADE29001
            return ret
        return ret

    def ping_bmc(self, also_print_console=False):
        ping_cmd = "ping {} -c 3".format(self.bmcupgrade_info_dict['ip'])
        turn = 12 * 5
        test_time = 0
        while turn > 0:
            self.logger.log_info(
                "start to ping {}, this is {} count".format(
                    self.bmcupgrade_info_dict['ip'],
                    test_time),
                also_print_console)
            status, content = run_command(ping_cmd)
            self.logger.log_info(content, also_print_console)
            time.sleep(5)
            if " 0%" in content:
                self.logger.log_info(
                    "******ping pass: {} connect  ******".format(
                        self.bmcupgrade_info_dict['ip']),
                    also_print_console)
                self.logger.log_info(
                    "set boot flash success", also_print_console)
                return True
            else:
                self.logger.log_info("{} not connect, waiting! this is {} turn".format(
                    self.bmcupgrade_info_dict['ip'], test_time), also_print_console)
                turn -= 1
                test_time += 1
                continue
        self.fail_reason.append("time out , {} not connect".format(
            self.bmcupgrade_info_dict['ip']))
        return False

    def run_test(self, *argv):
        for i in range(2):
            ret = self.bmc_upgrade_test(True)
            if ret != E.OK:
                return ret
        return ret
