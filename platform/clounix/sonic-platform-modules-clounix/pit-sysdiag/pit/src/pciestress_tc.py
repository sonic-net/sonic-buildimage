import re
import random
from errcode import *
from tabulate import tabulate
from test_case import TestCaseCommon
from pit_util_common import run_command, load_platform_util_module


class PCIESTRESSTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "pciestress_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.pcie_devices = None
        self.fpga_devices = None
        self.pciestress_info = None
        try:
            if self.platform_cfg_json and "pcie" in self.platform_cfg_json.keys():
                self.pcie_devices = self.platform_cfg_json["pcie"]
            if self.platform_cfg_json and "fpga" in self.platform_cfg_json.keys():
                self.fpga_devices = self.platform_cfg_json["fpga"]
            if self.platform_cfg_json and "pciestress_info" in self.platform_cfg_json.keys():
                self.pciestress_info = self.platform_cfg_json["pciestress_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def _get_bus_dev_func(self, device):
        bus_dev_func = []
        ret, out = run_command("lspci | grep {}".format(device))
        for line in out.splitlines():
            if re.search("[0-9a-f]{2}\\:[0-9a-f]{2}\\.[0-9a-f]{1}", line):
                bus_dev_func.append(
                    re.findall(
                        "[0-9a-f]{2}\\:[0-9a-f]{2}\\.[0-9a-f]{1}",
                        line)[0])
            else:
                bus_dev_func.append(None)
        return bus_dev_func

    def _get_device_conf(self, busid):
        ret, output = run_command(
            "lspci -s {} -vvv | grep -i LnkSta | grep Speed".format(busid))
        if ret or output == "":
            return "", ""
        else:
            speed = output.strip(" \t\n").split(
                ",")[0].split("Speed")[1].strip()
            width = output.strip(" \t\n").split(
                ",")[1].split("Width")[1].strip()
            return speed, width

    def check_pcie_device(self, also_print_console=False):
        self.logger.log_info("[PCIE DEVICE CHECK]:", also_print_console)
        header = ["Device", "Bus", "Width", "Speed"]
        ret = E.EFAIL
        status_tbl = []
        if self.pcie_devices:
            for device in self.pcie_devices:
                conf_width = self.pcie_devices[device]["width"]
                conf_speed = self.pcie_devices[device]["speed"]
                dev_num = int(self.pcie_devices[device]["num"])
                busid_list = self._get_bus_dev_func(device)
                if not busid_list:
                    self.fail_reason.append("{} not found".format(device))
                    ret = E.EPCI9001
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
                if len(busid_list) != dev_num:
                    self.logger.log_err("{} number expect {}, real {}".format(
                        device, dev_num, len(busid_list)), also_print_console)
                    self.fail_reason.append(
                        "{} number mismatch".format(device))
                    ret = E.EPCI9001
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
                for busid in busid_list:
                    speed, width = self._get_device_conf(busid)
                    if conf_width != width:
                        self.fail_reason.append(
                            "{} width not matched".format(device))
                        ret = E.EPCI9005
                        self.logger.log_err("FAIL!", also_print_console)
                        return ret
                    if conf_speed != speed:
                        self.fail_reason.append(
                            "{} speed not matched".format(device))
                        ret = E.EPCI9004
                        self.logger.log_err("FAIL!", also_print_console)
                        return ret
                    status_tbl.append([device, busid, width, speed])
                    ret = E.OK
        else:
            ret = E.EEXIST
            self.fail_reason.append("pcie config NULL")
        if len(status_tbl) > 0:
            self.logger.log_info(
                tabulate(
                    status_tbl,
                    header,
                    tablefmt="simple"),
                also_print_console)
        self.logger.log_info("PASS.", also_print_console)
        return ret

    def fpga_device_rw_test(self, also_print_console=False):
        self.logger.log_info("[FPGA STRESS TEST]:", also_print_console)
        ret = E.EFAIL

        if self.fpga_devices:
            for device in self.fpga_devices.keys():
                device_num = self.fpga_devices[device]["num"]
                if device_num == "":
                    ret = E.EPCI9007
                    self.fail_reason.append(
                        "invalid device : {}".format(device))
                    break
                random_num = "0x{:x}".format(random.randint(0x0000, 0xffff))
                pcie = load_platform_util_module("com_utils")
                pcie.write_fpga_reg(device_num, random_num)
                read_num = eval(pcie.read_fpga_reg(
                    device_num).strip().split()[-1])
                ret = E.OK
                if random_num != hex(read_num):
                    ret = E.EPCI9003
                    self.fail_reason.append("{} stress fail".format(device))
                    self.logger.log_err("FAIL!", also_print_console)
                    return ret
        else:
            self.fail_reason.append("fpga config NULL")

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        test_times = self.pciestress_info["stress_test_time"]
        for index in range(1, test_times + 1):
            ret = self.check_pcie_device(True)
            if ret != E.OK:
                self.logger.log_err(
                    "check pcie devices {} time failed!".format(index), True)
                return ret
            ret = self.fpga_device_rw_test(True)
            if ret != E.OK:
                self.logger.log_err(
                    "check pcie FPGA r/w {} time failed!".format(index), True)
                return ret
        return ret
