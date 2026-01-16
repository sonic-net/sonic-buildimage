import os
import pit_util_common
from errcode import E
from pit_util_common import run_command
from test_case import TestCaseCommon


class USBSTRESSTC(TestCaseCommon):
    def __init__(self, index, logger, platform_cfg_file, case_cfg_file=None):
        MODULE_NAME = "usbstress_tc"
        TestCaseCommon.__init__(
            self,
            index,
            MODULE_NAME,
            logger,
            platform_cfg_file,
            case_cfg_file)
        self.usb_info_dict = None
        try:
            if self.platform_cfg_json and "usbstress_info" in self.platform_cfg_json.keys():
                self.usb_info_dict = self.platform_cfg_json["usbstress_info"]
        except Exception as e:
            self.logger.log_err(str(e), True)

    def search_dir_by_name(self, name, dir):
        result = []
        try:
            files = os.listdir(dir)
            for file in files:
                if name in file:
                    result.append(os.path.join(dir, file))
        except Exception as e:
            self.logger.append(str(e))
        return result

    def get_usb_location(self):
        dir = "/sys/block/"
        spect = "sd"
        usbpath = []
        result = self.search_dir_by_name(spect, dir)
        if len(result) <= 0:
            self.fail_reason.append("no usb found")
            return E.EUSB1000, usbpath
        for item in result:
            with open(os.path.join(item, "removable")) as fd:
                value = fd.read()
                if value.strip() == "1":
                    usbpath.append(item)
        if not usbpath:
            self.fail_reason.append("no usb found")
            return E.EUSB1000, usbpath
        return E.OK, usbpath

    def get_usb_info(self, path):
        size_file = os.path.join(path, "size")
        try:
            with open(size_file) as f:
                value = f.read()
            return E.OK, {
                "dev": os.path.basename(path), "size": round(
                    float(value) * 512 / 1024 / 1024 / 1024, 1)}
        except IOError as e:
            return E.EIO, str(e)

    def usb_stress(self, usb_dev, also_print_console=False):
        ret = E.EFAIL
        cmd = "fdisk -l |grep %s|grep 'Disk' -v|awk '{print $1}'" % usb_dev
        fdisk_ret, fdisk_log = run_command(cmd)
        usb_list = fdisk_log.splitlines()
        if not usb_list:
            usb_list.append("/dev/" + usb_dev)

        for usb_disk in usb_list:
            if not os.path.exists("/sys/block/{}/{}".format(usb_dev, usb_disk)):
                self.logger.log_info("usb fs error, partion:{} can't get type".format(usb_disk), also_print_console)
                self.logger.log_info("enter in easy test mode", also_print_console)
                usb_list = ["/dev/" + usb_dev]
                break;

        for usb_disk in usb_list:
            self.logger.log_info("[USB STRESS TEST]:", also_print_console)
            ret, log = run_command("mount -v | grep /mnt/usb")
            if log != '':
                run_command("umount /mnt/usb/")
                run_command("rm -rf /mnt/usb/")
            mount_ret, mount_log = run_command(
                "mkdir -p /mnt/usb && mount {} /mnt/usb/".format(usb_disk))
            if mount_ret:
                self.fail_reason.append("mount usb failed")
                self.logger.log_err("FAIL!", also_print_console)
                ret = E.EUSB1001
                run_command("rm -rf /mnt/usb/")
                return ret
            else:
                test_stress_cmd = "dd if={} of={}4G.txt bs=64k count=1024".format(
                    usb_disk, "/mnt/usb/")
                waiting = pit_util_common.waiting(self.logger,"usbstress testing...")
                stress_ret, stress_log = run_command(test_stress_cmd)
                waiting.stop("usbstress test stop!")
                if stress_ret:
                    self.fail_reason.append("stress test failed!")
                    ret = E.EUSB1006
                else:
                    self.logger.log_info(
                        "stress test success.", also_print_console)
                    ret = E.OK
                run_command("rm -rf /mnt/usb/4G.txt")
                run_command("umount /mnt/usb/")
                run_command("rm -rf /mnt/usb/")

        if ret != E.OK:
            self.logger.log_err("FAIL!", also_print_console)
        else:
            self.logger.log_info("PASS.", also_print_console)
        return ret

    def run_test(self, *argv):
        pass_cnt = 0
        fail_cnt = 0
        for index in range(1, self.usb_info_dict['stress_test_time'] + 1):
            ret, path_list = self.get_usb_location()
            if ret != E.OK:
                self.fail_reason.append("usb not found!")
                fail_cnt += 1
                result = E.OK if fail_cnt == 0  else E.EFAIL
                return [pass_cnt, fail_cnt, result]
            for path in path_list:
                errocode, info = self.get_usb_info(path)
                if errocode != E.OK or info["size"] <= 0:
                    self.fail_reason.append("USB device cannot be used")
                    fail_cnt += 1
                    result = E.OK if fail_cnt == 0  else E.EFAIL
                    return [pass_cnt, fail_cnt, result]
                self.logger.log_info(
                    "found usb device: {}, size={}GB".format(
                        info["dev"], info["size"]), True)
                ret = self.usb_stress(info["dev"], True)
                if ret != E.OK:
                    self.logger.log_err(
                        "usb stress {} failed!".format(index), True)
                    fail_cnt += 1
                else:
                    pass_cnt += 1

        result = E.OK if fail_cnt == 0  else E.EFAIL
        return [pass_cnt, fail_cnt, result]
