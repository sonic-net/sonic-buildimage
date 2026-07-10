#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/7/24 9:34
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Load pddf_custom_lpc_basecpld.ko, after confirming the BMC is in place,
#            load different configuration files, and finally remove the driver.

import subprocess
import os
import os.path

from sonic_py_common import device_info
(platform_name, _) = device_info.get_platform_and_hwsku()

class PrePddfInit(object):
    def __init__(self, platform):
        self.ker_path = "/usr/lib/modules/{}/extra"
        self.lpc_basecpld_name = "pddf_custom_lpc_basecpld"
        self.lpc_basecpld_ko = "pddf_custom_lpc_basecpld.ko"
        self.bmc_exist_cmd = "/sys/bus/platform/devices/sys_cpld/bmc_present"
        self.platform_name = platform
        self.bmc_present = False

    @staticmethod
    def run_command(cmd):
        status = True
        result = ""
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = False
        else:
            result = data

        return status, result

    def get_kernel_path(self):
        """
        get the kernel object complete path
        :return:
        """
        sta_, res_ = self.run_command("uname -r")
        if sta_:
            return self.ker_path.format(res_)
        else:
            return None

    def install_lpc_basecpld(self):
        """
        install lpc basecpld driver
        """
        ker_path = self.get_kernel_path()
        if ker_path:
            self.run_command("insmod %s/%s" % (ker_path, self.lpc_basecpld_ko))

    def uninstall_lpc_basecpld(self):
        self.run_command("rmmod %s" % self.lpc_basecpld_name)

    def get_bmc_status(self):
        """
        get bmc status
        """
        self.install_lpc_basecpld()
        if os.path.exists(self.bmc_exist_cmd):
            # "1": "absent", "0": "present"
            sta, res = self.run_command("cat %s" % self.bmc_exist_cmd)
            self.bmc_present = False if res == "1" else True
        self.uninstall_lpc_basecpld()

    def choose_pddf_device_json(self):
        """
        Depending on the state of the BMC, different pddf-device.json file configurations will be used:
        1.BMC exist: cp pddf-device-bmc.json pddf-device.json
        2.None BMC : cp pddf-device-nonbmc.json pddf-device.json
        """
        device_name = "pddf-device-bmc.json" if self.bmc_present else "pddf-device-nonbmc.json"
        device_path = "/usr/share/sonic/device/%s/pddf/" % self.platform_name
        self.run_command("cp %s%s %spddf-device.json" % (device_path, device_name, device_path))

    def choose_platform_components(self):
        """
        Depending on the state of the BMC, different platform_components.json file configurations will be used:
        1.BMC exist: cp platform_components-bmc.json platform_components.json
        2.None BMC : cp platform_components-nonbmc.json platform_components.json
        """
        device_name = "platform_components-bmc.json" if self.bmc_present else "platform_components-nonbmc.json"
        device_path = "/usr/share/sonic/device/%s/" % self.platform_name
        self.run_command("cp %s%s %splatform_components.json" % (device_path, device_name, device_path))

    def main(self):
        self.get_bmc_status()
        self.choose_pddf_device_json()
        self.choose_platform_components()
        with open("/usr/share/sonic/device/%s/bmc_status" % self.platform_name, 'w') as fp:
            fp.write(str(self.bmc_present))


if __name__ == '__main__':
    if not os.path.isfile(f"/usr/share/sonic/device/{platform_name}/bmc_status"):
        pre_init = PrePddfInit(platform_name)
        pre_init.main()
