import time

from test_case import TestCaseCommon
from errcode import E
from function import run_command


class INTERNALUSBTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "internal_usb_tc"  # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

    def get_internal_usb_dev(self, also_print_console=False):
        self.logger.log_info("[CHECK INTERNAL USB DEVICE]:", also_print_console)
        cmd = "ifconfig -a | grep usb"
        status, device = run_command(cmd)
        if not status and device:
            self.logger.log_info("get usb network device.", also_print_console)
            ret = E.OK
        else:
            ret = E.EEXIST
            self.fail_reason.append("usb network device not found!")

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def cpu_ping_bmc_test(self, also_print_console=True):
        self.logger.log_info("[CPU BMC LINK TEST]:", also_print_console)
        cmd = "ifconfig usb0 up "
        status, output = run_command(cmd)
        cmd = "ifconfig usb0 192.168.1.100 "
        status, output = run_command(cmd)
        time.sleep(10)
        count = 5
        ping_cmd = "ping 192.168.1.101 -c %d -I usb0 | grep received" % count
        status, output = run_command(ping_cmd)
        self.logger.log_info(output, also_print_console)

        if output.find(" 0% packet loss") > 0:
            ret = E.OK
        else:
            self.fail_reason.append("cpu ping server lost packages")
            ret = E.EUSB1005

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)

        return ret

    def run_test(self, *argv):
        ret = self.get_internal_usb_dev(True)
        if ret != E.OK:
            return ret
        ret = self.cpu_ping_bmc_test(True)
        return ret
