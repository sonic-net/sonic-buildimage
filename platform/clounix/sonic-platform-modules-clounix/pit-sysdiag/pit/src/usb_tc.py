import os
import subprocess
from function import run_command
from test_case import TestCaseCommon
from errcode import *

class USBTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "usb_tc"  # this param specified the case config dirictory
        TestCaseCommon.__init__(self, index, MODULE_NAME, logger, platform_cfg_file, case_cfg_file)

    def search_dir_by_name(self, name, dir):
        result = []
        try:
            files = os.listdir(dir)
            for file in files:
                if name in file:
                    result.append(os.path.join(dir, file))
        except Exception as e:
            self.logger.log_err(str(e), True)
        return result

    def get_usb_location(self):
        dir = "/sys/block/"
        spect = "sd"
        usbpath = []
        result = self.search_dir_by_name(spect, dir)
        if len(result) <= 0:
            return E.EUSB1000, usbpath

        for item in result:
            with open(os.path.join(item, "removable"), 'r') as fd:
                value = fd.read()
                if value.strip() == "1":  # found Udisk
                    usbpath.append(item)
        if not usbpath:  # not found Udisk
            self.logger.log_err("no usb found")
            return E.EUSB1000, usbpath
        return E.OK, usbpath

    def get_usb_info(self, path):
        size_file = os.path.join(path, "size")
        try:
            with open(size_file, "r") as f:
                value = f.read()
            return E.OK, {"dev": os.path.basename(path), "size": round(float(value) * 512 / 1024 / 1024 / 1024, 1)}
        except IOError as e:
            return E.EIO, str(e)

    def usb_rw_test(self, usb_dev, also_print_console=False):
        totalret = E.OK
        cmd = "fdisk -l |grep '%s'|grep 'Disk' -v|awk '{print $1}'" % usb_dev
        ret, usb = run_command(cmd)
        usb_list = usb.splitlines()
        if not usb_list:
            usb_list.append("/dev/" + usb_dev)

        for usb_disk in usb_list:
            if not os.path.exists("/sys/block/{}/{}".format(usb_dev, usb_disk)):
                self.logger.log_info("usb fs error, partion:{} can't get type".format(usb_disk), also_print_console)
                self.logger.log_info("enter in easy test mode", also_print_console)
                usb_list = ["/dev/" + usb_dev]
                break;

        for usb_disk in usb_list:
            totalret = E.OK
            self.logger.log_info("[USB READ/WRITE TEST]:", also_print_console)
            
            ret, log = run_command("dd if=/dev/urandom of=/tmp/usbtest.txt bs=1M count=10")

            ret, log = run_command("mkdir -p /tmp/usb_test")
            if ret:
                self.logger.log_err("mkdir failed", also_print_console)
                return E.ERR

            ret, log = run_command("mount %s /tmp/usb_test/" % usb_disk)
            if ret:
                self.logger.log_err("mount usb failed", also_print_console)
                self.fail_reason.append("mount usb failed")
                totalret = E.EUSB1001
                continue
            
            ret, log = run_command("cp -rf /tmp/usbtest.txt /tmp/usb_test/usbtest.txt")
            if ret:
                self.logger.log_err("copy file failed!", also_print_console)
                self.fail_reason.append("copy file failed!")
                totalret = E.EUSB1003
            else:
                self.logger.log_info("copy file success.", also_print_console)
            
            ret, log = run_command("diff /tmp/usbtest.txt /tmp/usb_test/usbtest.txt")
            if ret:
                self.logger.log_err("verify file failed!", also_print_console)
                self.fail_reason.append("verify file failed!")
                totalret = E.EUSB1002
            else:
                self.logger.log_info("verify file success.", also_print_console)
            
            ret, log = run_command("rm -rf /tmp/usb_test/usbtest.txt")
            if ret:
                self.logger.log_err("delete file failed!", also_print_console)
                self.fail_reason.append("delete file failed!")
                totalret = E.EUSB1003
            else:
                self.logger.log_info("delete file success.", also_print_console)
            
            run_command("umount /tmp/usb_test")
            run_command("rm -rf /tmp/usb_test")
            run_command("rm -f /tmp/usbtest.txt")
            
            if totalret == E.OK:
                break

        if totalret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return totalret

    def run_test(self, *argv):
        fail_cnt = 0
        pass_cnt = 0
        ret, path_list = self.get_usb_location()
        if ret != E.OK:
            self.fail_reason.append("usb not found!")
            fail_cnt += 1
            result = E.OK if fail_cnt == 0  else E.EFAIL
            return [pass_cnt, fail_cnt, result]

        for path in path_list:
            errocode, info = self.get_usb_info(path)
            if errocode != E.OK or info["size"] <= 0:
                continue
            self.logger.log_info("found usb device: {}, size={}GB".format(info["dev"], info["size"]), True)
            ret = self.usb_rw_test(info["dev"], True)
            if ret == E.OK:
                pass_cnt += 1
            else:
                fail_cnt += 1
                
            result = E.OK if fail_cnt == 0  else E.EFAIL
            return [pass_cnt, fail_cnt, result]
