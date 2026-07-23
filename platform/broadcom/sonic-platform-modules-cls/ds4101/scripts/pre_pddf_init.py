#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/7/24 9:34
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Load custom_lpc_basecpld.ko, after confirming the BMC is in place,
#            load different configuration files, and finally remove the driver.

import subprocess
import os
from sonic_py_common import device_info


class PrePddfInit(object):
    def __init__(self):
        self.ker_path = "/usr/lib/modules/{}/extra"
        self.lpc_basecpld_name = "custom_lpc_basecpld"
        self.lpc_basecpld_ko = "custom_lpc_basecpld.ko"
        self.bmc_exist_cmd = "/sys/bus/platform/devices/sys_cpld/bmc_present"
        (self.platform_name, _) = device_info.get_platform_and_hwsku()
        self.bmc_present = False

    @staticmethod
    def run_command(cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err.decode("utf-8") == "":
                result = raw_data.decode("utf-8").strip()
        except Exception:
            status = False
        return status, result

    def get_kernel_path(self):
        """
        get the kernel object complete path
        :return:
        """
        sta_, res_ = self.run_command(["uname", "-r"])
        if sta_:
            return self.ker_path.format(res_)
        else:
            print("Can't get the kernel extra path!")
            return None

    def install_lpc_basecpld(self):
        """
        install lpc basecpld driver
        """
        ker_path = self.get_kernel_path()
        lpc_basecpld_path = f"{ker_path}/{self.lpc_basecpld_ko}" 
        if ker_path:
            self.run_command(["insmod", lpc_basecpld_path])
            print("Has install %s" % self.lpc_basecpld_ko)
        else:
            print("Install %s error!" % self.lpc_basecpld_ko)

    def get_bmc_status(self):
        """
        get bmc status
        """
        self.install_lpc_basecpld()
        if os.path.exists(self.bmc_exist_cmd):
            # "1": "absent", "0": "present"
            sta, res = self.run_command(["cat", self.bmc_exist_cmd])
            self.bmc_present = False if res == "1" else True

    def choose_pddf_device_json(self):
        """
        Depending on the state of the BMC, different pddf-device.json file configurations will be used:
        1.BMC exist: cp pddf-device-bmc.json pddf-device.json
        2.None BMC : cp pddf-device-nonebmc.json pddf-device.json
        """
        device_name = "pddf-device-bmc.json" if self.bmc_present else "pddf-device-nonebmc.json"
        device_path = "/usr/share/sonic/device/%s/pddf/" % self.platform_name
        origin_file = "%s%s" % (device_path, device_name)
        target_file = "%spddf-device.json" % device_path
        self.run_command(["cp", origin_file, target_file])
        print("The selection of the %s file has been completed" % device_name)

    def choose_platform_components(self):
        """
        Depending on the state of the BMC, different platform_components.json file configurations will be used:
        1.BMC exist: cp platform_components-bmc.json platform_components.json
        2.None BMC : cp platform_components-nonebmc.json platform_components.json
        """

        device_name = "platform_components-bmc.json" if self.bmc_present else "platform_components-nonebmc.json"
        device_path = "/usr/share/sonic/device/%s/" % self.platform_name
        origin_file = "%s%s" % (device_path, device_name)
        target_file = "%splatform_components.json" % device_path
        self.run_command(["cp", origin_file, target_file])
        print("The selection of the %s file has been completed" % device_name)

    def main(self):
        self.get_bmc_status()
        self.choose_pddf_device_json()
        self.choose_platform_components()


if __name__ == '__main__':
    pre_init = PrePddfInit()
    pre_init.main()
