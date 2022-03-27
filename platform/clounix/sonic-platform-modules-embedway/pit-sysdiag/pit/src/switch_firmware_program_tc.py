# coding:utf-8
import json
import os
import sys

from errcode import *
from function import load_platform_util_module

# from click._compat import raw_input
from test_case import TestCaseCommon

MASTER_ORIG_FW_CFG = "switch_master_firmware_baseline.json"
SLAVE_ORIG_FW_CFG = "switch_slave_firmware_baseline.json"


class SWITCHFIRMWAREPROGRAMTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "fwmgrutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "FwMgrUtil"
    get_version_func = "self.firmware_util.get_fw_version"
    firmware_upgrade_func = "self.firmware_util.firmware_upgrade"
    firmware_refresh_func = "self.firmware_util.firmware_refresh"
    slave_upgrade = False
    fw_extra = "master"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        # this param specified the case config directory
        MODULE_NAME = "switch_firmware_program_tc"
        TestCaseCommon.__init__(
            self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file
        )
        self.firmware_util = None

        try:
            firmware_module = load_platform_util_module(
                self.__PLATFORM_SPECIFIC_MODULE_NAME
            )
            platform_util_class = getattr(
                firmware_module, self.__PLATFORM_SPECIFIC_CLASS_NAME
            )
            self.firmware_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

        try:
            self.firmware_type = self.platform_cfg_json["switch_firmware_type"]
            self.test_firmware_list = self.platform_cfg_json["switch_firmware_test"]
            self.baseline_firmware_list = self.platform_cfg_json["switch_firmware_baseline"]
            self.platform_path = os.path.dirname(self.platform_cfg_file)
        except Exception as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)

    def get_upgrade_path(self, firmware_list, item):
        """

        @param firmware_list: firmware list
        @param item: firmware type
        @return: get path success:bool,skip upgrade:bool,firmware path:string
        """
        support_slave, skip_upgrade = self.check_item_support_slave(item)
        if "absolute_path" not in firmware_list[item]:
            firmware_list[item]["absolute_path"] = True
            # absolute_path字段不存在时默认值为True
        if skip_upgrade is True:
            return False, True, ""
        elif (
            skip_upgrade is False
            and support_slave is True
            and "same_with_master" in firmware_list[item]
            and firmware_list[item]["same_with_master"] is False
        ):
            # 判断备区升级文件是否存在
            if (
                "slave_file_path" not in firmware_list[item]
                or firmware_list[item]["slave_file_path"] == ""
            ):
                self.fail_reason.append(
                    "there are no upgrade firmware file in item %s" % item
                )
                return False, False, ""
            else:
                fw_path = (
                    (self.platform_path + "/")
                    if firmware_list[item]["absolute_path"] is not True
                    else ""
                )
                fw_path += firmware_list[item]["slave_file_path"]
                if os.path.exists(fw_path) is False:
                    self.fail_reason.append(
                        "there are no upgrade firmware file in item %s" % item
                    )
                    return False, False, ""
                return True, False, fw_path
            # 获取备区升级固件所在路径，“same_with_master”字段省略默认为False,备区固件路径和主区一样则直接使用firmware_list[item]["file_path"]
            # 不一样则使用firmware_list[item]["slave_file_path"]
        else:
            if (
                "file_path" not in firmware_list[item]
                or firmware_list[item]["file_path"] == ""
            ):
                self.fail_reason.append(
                    "there are no upgrade firmware file in item %s" % item
                )
                return False, False, ""
            else:
                fw_path = (
                    (self.platform_path + "/")
                    if firmware_list[item]["absolute_path"] is not True
                    else ""
                )
                fw_path += firmware_list[item]["file_path"]
                if os.path.exists(fw_path) is False:
                    self.fail_reason.append(
                        "there are no upgrade firmware file in item %s" % item
                    )
                    return False, False, ""
                return True, False, fw_path

    def upgrade_firmware_refresh(self, firmware_list, test_type="test"):
        """

        @param firmware_list: firmware list
        @param test_type: sting to print to console
        @return: upgrade and refresh firmware success:bool
        """
        ret = True
        try:
            for item in self.firmware_type:
                result, skip_upgrade, fw_path = self.get_upgrade_path(
                    firmware_list, item
                )
                if skip_upgrade:
                    self.logger.log_info(
                        "there are no slave_upgrade attr in %s  skip upgrade" % item,
                        True,
                    )
                    continue
                if not result:
                    self.fail_reason.append(
                        "upgrade %s %s %s firmware fail"
                        % (item, self.fw_extra, test_type)
                    )
                    ret = False
                    break
                result = eval(self.firmware_upgrade_func)(item, fw_path, self.fw_extra)
                if not result:
                    self.fail_reason.append(
                        "upgrade %s %s %s firmware fail"
                        % (item, self.fw_extra, test_type)
                    )
                    ret = False
                    break
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append(str(e))
            ret = False
        return ret

    def save_baseline_and_upgrade(self):
        """

        @return: save baseline version and upgrade success:bool
        """
        try:

            version_running_read_back = eval(self.get_version_func)(
                self.firmware_type, self.fw_extra
            )
            with open(
                self.platform_path
                + "/"
                + (MASTER_ORIG_FW_CFG if not self.slave_upgrade else SLAVE_ORIG_FW_CFG),
                "w",
            ) as f:
                json.dump(
                    version_running_read_back, f, indent=4, separators=(",", ": ")
                )
            return self.upgrade_firmware_refresh(
                self.test_firmware_list, test_type="test"
            )
        except Exception as e:
            self.fail_reason.append(str(e))
            return False

    def check_all_items_support_slave(self):
        item = ""
        try:
            for item in self.firmware_type:
                support_slave, skip = self.check_item_support_slave(item)
                if support_slave is True:
                    self.logger.log_info(
                        "there are slave_upgrade attr in item %s" % item, True
                    )
                    return True
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append(str(e))
            raise Exception("check %s support slave failed" % item)
        return False

    def check_item_support_slave(self, item):
        """

        @param item: firmware type
        @return: is support slave upgrade:bool,skip upgrade
        """
        if self.slave_upgrade:
            if (
                "slave_upgrade" not in self.test_firmware_list[item]
                or "slave_upgrade" not in self.baseline_firmware_list[item]
            ):
                if (
                    "slave_upgrade" not in self.test_firmware_list[item]
                    and "slave_upgrade" not in self.baseline_firmware_list[item]
                ):
                    # 不存在“slave_upgrade”字段,默认为false
                    return False, True
                else:
                    raise Exception(
                        'firmware_test[%s]["slave_upgrade"]  must be equal'
                        ' firmware_baseline[%s]["slave_upgrade"].' % (item, item)
                    )
                    # test fw "slave_upgrade"字段和baseline fw不一致，抛出异常，JSON文件存在问题
            elif (
                self.test_firmware_list[item]["slave_upgrade"] is False
                and self.baseline_firmware_list[item]["slave_upgrade"] is False
            ):
                return False, True
            # “slave_upgrade”字段为False，不支持备区升级
            elif (
                self.test_firmware_list[item]["slave_upgrade"]
                != self.baseline_firmware_list[item]["slave_upgrade"]
            ):
                raise Exception(
                    'firmware_test[%s]["slave_upgrade"]  must be equal'
                    ' firmware_baseline[%s]["slave_upgrade"].' % (item, item)
                )
            # test fw "slave_upgrade"字段和baseline fw不一致，抛出异常，JSON文件存在问题
            elif self.test_firmware_list[item]["slave_upgrade"] is True:
                return True, False
        else:
            return False, False

    def check_firmware_version(self, firmware_cfg, test_type="test"):
        """

        @param firmware_cfg: firmware config file content
        @param test_type: string to print，distinguish the two modes
        @return:check firmware version success:bool
        """
        version_read_back = eval(self.get_version_func)(
            self.firmware_type, self.fw_extra
        )
        ret = True
        try:
            for item in self.firmware_type:
                support_slave, skip = self.check_item_support_slave(item)
                if skip is True:
                    self.logger.log_info(
                        "there are no slave_upgrade attr in %s skip check version"
                        % item,
                        True,
                    )
                    continue
                #     不支持备区升级直接跳过检测
                if skip is False and support_slave is True:
                    if (
                        test_type is "test"
                        and "same_with_master" not in firmware_cfg[item]
                        or firmware_cfg[item]["same_with_master"] is False
                    ):
                        version_config_file = firmware_cfg[item]["slave_version"]
                    #     比对备区test版本固件版本号，“same_with_master”字段省略默认为False,备区版本号和主区一样则直接使用firmware_cfg[item]["version"]
                    #     不一样则存放在firmware_cfg[item]["slave_version"]
                    else:
                        version_config_file = firmware_cfg[item]["version"]
                else:
                    version_config_file = firmware_cfg[item]["version"]
                if version_config_file == version_read_back[item]["version"]:
                    self.logger.log_info(
                        "%s %s firmware check success" % (item, test_type), True
                    )
                else:
                    self.logger.log_err(
                        "%s %s firmware check fail,config file version:[%s],device version:[%s]"
                        % (
                            item,
                            test_type,
                            version_config_file,
                            version_read_back[item]["version"],
                        ),
                        True,
                    )
                    self.fail_reason.append(
                        "%s %s firmware check fail" % (item, test_type)
                    )
                    ret = False
                    break
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append(str(e))
        return ret

    def check_and_recover_firmware(self):
        """

        @return: check and recover firmware to baseline success:bool
        """
        try:
            version_test_check_result = self.check_firmware_version(
                self.test_firmware_list, test_type="test"
            )
            if version_test_check_result is not True:
                return False
            else:
                return self.upgrade_firmware_refresh(
                    self.baseline_firmware_list, test_type="baseline"
                )
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append(str(e))
            return False

    def check_baseline_firmware(self):
        """

        @return: check baseline firmware version success:bool
        """
        ret = True
        try:
            with open(
                self.platform_path
                + "/"
                + (MASTER_ORIG_FW_CFG if not self.slave_upgrade else SLAVE_ORIG_FW_CFG),
                "r",
            ) as f:
                version_running_read_back_list = json.load(f)
            version_running_check_result = self.check_firmware_version(
                version_running_read_back_list, test_type="baseline"
            )
            if version_running_check_result is not True:
                ret = False
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append(str(e))
            ret = False
        return ret

    test_case_list = [
        "save_baseline_and_upgrade",
        "check_and_recover_firmware",
        "check_baseline_firmware"
    ]
    test_help_list = [
        "1.save master running firmware version and program test firmware",
        "2.check master test firmware version and program baseline firmware",
        "3.check master baseline firmware version"
    ]

    def run_test(self, *argv):
        ret = E.OK
        also_print_console = True
        for item in self.test_help_list:
            self.logger.log_info(item, True)

        choice = input("Please enter your choice (0 to quit):")
        try:
            if int(choice) not in range(0, (len(self.test_case_list) + 1)):
                self.fail_reason.append("input invalid param:%s" % choice)
                ret = E.EFAIL
            else:
                if int(choice) is 0:
                    # self.fail_reason.append("user cancelled input")
                    self.logger.log_info("user cancelled input", also_print_console)
                    ret = E.OK
                else:
                    index = int(choice)
                    # continue test item while check is success
                    test_item = "self.%s" % self.test_case_list[index - 1]
                    self.slave_upgrade = True if index > 3 else False
                    self.fw_extra = "master" if not self.slave_upgrade else "slave"
                    self.logger.log_info(
                        "[%s]:" % self.test_help_list[index - 1], also_print_console
                    )
                    if (
                        self.slave_upgrade is False
                        or self.check_all_items_support_slave() is True
                    ):
                        # skip test item while there are no "slave_upgrade" in all "firmware_type" item
                        result = eval(test_item)()
                        if result is not True:
                            self.logger.log_info(
                                "item %s =======> test fail"
                                % self.test_help_list[index - 1],
                                also_print_console,
                            )
                            return E.EUPG25001 + index - 1
                        elif index == 1 or index == 2 or index == 4 or index == 5:
                            self.logger.log_info(
                                "item %s =======> test success"
                                % self.test_help_list[index - 1],
                                also_print_console,
                            )
                            # system reboot may be required in test item 1,2,4 or 5
                            # break out of the loop to end the test item
                    else:
                        self.logger.log_info(
                            "there are no slave_upgrade attr, item %s =======> test skip"
                            % self.test_help_list[index - 1],
                            True,
                        )
        except Exception as e:
            self.logger.log_err(str(e), True)
            self.fail_reason.append("input invalid param:%s" % choice)
            ret = E.EFAIL
        return ret
