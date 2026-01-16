import sys
import os
from tabulate import tabulate
from test_case import TestCaseCommon
from errcode import *
from function import load_platform_util_module


# gpio test class
class GPIOTC(TestCaseCommon):
    __PLATFORM_SPECIFIC_MODULE_NAME = "fwmgrutil"
    __PLATFORM_SPECIFIC_CLASS_NAME = "FwMgrUtil"

    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "gpio_tc"
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)
        self.gpio_map = None            # default
        self.jtag_map = None            # default
        """case_config.json example
        "gpio":{
            "gpio486":{
                "pin":"GPIO50",
                "net":"CPU_GPIO_JTAG_EN"
            },
            "gpio479":{
                "pin":"GPIO43",
                "net":"CPU_GPIO_JTAG_TCK"
            },
            "gpio450":{
                "pin":"GPIO14",
                "net":"CPU_GPIO_JTAG_TMS"
            },
            "gpio446":{
                "pin":"GPIO10",
                "net":"CPU_GPIO_JTAG_TDO"
            },
            "gpio445":{
                "pin":"GPIO9",
                "net":"CPU_GPIO_JTAG_TDI"
            }
        },
        "jtag":["BASEBOARD_CPLD"]
        """
        try:
            self.platform_conf_path = os.path.dirname(os.path.abspath(self.platform_cfg_file))
            if self.platform_cfg_json and 'gpio' in self.platform_cfg_json.keys():
                self.gpio_map = self.platform_cfg_json['gpio']
            if self.platform_cfg_json and 'jtag' in self.platform_cfg_json.keys():
                self.jtag_map = self.platform_cfg_json['jtag']
        except Exception as e:
            self.logger.log_err(str(e), True)


        fwupgrade_module = load_platform_util_module(self.__PLATFORM_SPECIFIC_MODULE_NAME)
        try:
            platform_util_class = getattr(fwupgrade_module, self.__PLATFORM_SPECIFIC_CLASS_NAME)
            self.firmware_util = platform_util_class()
        except AttributeError as e:
            self.logger.log_err(str(e), True)
            sys.exit(1)


    def _read_gpio_pin_value(self, gpio_value_file):
        value = -1
        try:
            with open(gpio_value_file, "r") as f:
                value = f.read()
        except IOError as e:
            self.logger.log_err(str(e))

        return value


    def gpio_pin_check(self, also_print_console=True):
        if not self.gpio_map:
            err = "gpio conf is null!"
            self.fail_reason.append(err)
            self.logger.log_err(err, also_print_console)
            return E.EEXIST

        self.logger.log_info("[GPIO PIN CHECK]:", also_print_console)
        header = ["GPIO", "Pin", "Net", "Status"]
        status_tbl = []
        ret = E.OK
        for gpio, desc in self.gpio_map.items():
            pin_name = desc['pin']
            net_name = desc['net']
            gpio_value_file = os.path.join("/sys/class/gpio", gpio, "value")
            value = self._read_gpio_pin_value(gpio_value_file)
            if value < 0:
                ret = E.EGPIO13001
                status = "Not_OK"
                err = "{} read failed!".format(gpio)
                self.fail_reason.append(err)
            else:
                status = "OK"
            status_tbl.append([gpio, pin_name, net_name, status])

        if len(status_tbl) > 0:
            self.logger.log_info(tabulate(status_tbl, header, tablefmt="simple"), also_print_console)

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret


    def jtag_upgrade_by_gpio(self, also_print_console=False):
        self.logger.log_info("[GPIO JTAG FUNCTION CHECK]:", also_print_console)
        ret = E.OK
        for jtag in self.jtag_map:
            jtag_upgrade_file = os.path.join(self.platform_conf_path, jtag + '.vme')
            if jtag_upgrade_file and os.path.exists(jtag_upgrade_file):
                status = self.firmware_util.firmware_upgrade("cpld", jtag_upgrade_file, jtag)
                if not status:
                    ret = E.EGPIO13002
                    self.fail_reason.append("ScanJtag failed")
            else:
                ret = E.EEXIST
                self.fail_reason.append("missing vme file")

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret


    def run_test(self, *argv):
        ret = self.gpio_pin_check(True)
        if ret != E.OK:
            return ret
        
        ret = self.jtag_upgrade_by_gpio(True)
        return ret
