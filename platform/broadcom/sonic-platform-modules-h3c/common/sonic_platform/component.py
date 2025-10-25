#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available
the platform
"""
try:
    import re
    import os
    import subprocess
    import time
    from pathlib2 import Path
    from sonic_platform_base.component_base import ComponentBase
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as import_error:
    raise ImportError(str(import_error) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -t 11"


class Component(ComponentBase):
    """Platform-specific Component class"""

    def __init__(self, component_index):
        self.index = component_index
        self.name = Devcfg.CHASSIS_COMPONENTS[self.index][0]
        self.description = Devcfg.CHASSIS_COMPONENTS[self.index][1]
        super(Component, self).__init__()

    def _get_file_path(self, main_dir, sub_dir):
        path = main_dir + sub_dir
        temp_value = 0

        try:
            with open(path, 'r') as temp_read:
                temp_value = temp_read.read().strip('\n')
        except Exception as error:
            self.log_error(str(error))
            return "N/A"
        return temp_value

    def _get_command_result(self, cmdline):

        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.rstrip('\n')
        except Exception as e:
            self.log_error(str(e))
            return None
        return result

    def get_name(self):
        """
        Retrieves the name of the component
        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component
        Returns:
            A string containing the description of the component
        """
        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component
        Returns:
            A string containing the firmware version of the component
        """
        if "BIOS" in self.name:
            bios_ver = self._get_command_result(BIOS_QUERY_VERSION_COMMAND)
            if not bios_ver:
                ver_str = 'N/A'
            else:
                pattern = r"BIOS Version *([^\n]+)"
                matchobj = re.search(pattern, bios_ver)
                ver_str = matchobj.group(1) if matchobj is not None else "N/A"
            return ver_str
        elif 'FPGA' in self.name:
            fpga_version = self._get_file_path("/sys/switch/fpga/fpga1/", "hw_version")
            if fpga_version is None:
                return 'N/A'
            return fpga_version
        elif 'ALL_BOARD' in self.name:
            fw_version = "v"
            for i in range(Devcfg.COMPONENT_NUM - 4):
                fw_version += self._get_file_path(Devcfg.CPLD_DIR + "/cpld{}".format(i + 2) + "/", "hw_version")
            return fw_version
        else:
            fw_version = self._get_file_path(Devcfg.CPLD_DIR + "/cpld" + str(self.index) + "/", "hw_version")
            return fw_version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install was successful, False if not
        """
        upgrade_file_name = ''
        kernel_ver_name = ''
        upgrade_file_list = list()

        if 'BIOS' in self.name:
            os.system("mount /dev/sda1 {}".format(Devcfg.MNT_DIR))
            time.sleep(0.1)
            if not Path(Devcfg.EFI_DIR).is_dir():
                return False

            if not Path(Devcfg.EFI_BOOT_DIR).is_dir():
                os.system("mkdir -p {}".format(Devcfg.EFI_BOOT_DIR))
                time.sleep(0.1)
                if not Path(Devcfg.EFI_BOOT_DIR).is_dir():
                    return False
            else:
                os.system("rm -rf {}*".format(Devcfg.EFI_BOOT_DIR))

            os.system("cp -rf {} {}".format(image_path, Devcfg.EFI_BOOT_DIR))
            upgrade_file_list = os.listdir(Devcfg.EFI_BOOT_DIR)

            if upgrade_file_list:
                upgrade_file_name = upgrade_file_list[0]
            else:
                return False

            os.system("mv {efi_boot}{upgrade_file} {efi_boot}bdeplatcome.btw".format(
                efi_boot=Devcfg.EFI_BOOT_DIR, upgrade_file=upgrade_file_name))

            if not os.path.exists("{}bdeplatcome.btw".format(Devcfg.EFI_BOOT_DIR)):
                return False

            os.system("reboot")

            return True

        elif 'CPU-CPLD' in self.name:
            kernel_ver_name = os.popen('uname -a').readlines()[0].split()[2]

            drv_exist = os.popen('lsmod | grep drv_cpld').readlines()
            if not drv_exist:
                os.system('insmod /lib/modules/%s/extra/drv_cpld.ko' %
                          kernel_ver_name)
            os.system('chmod 777 {}'.format(Devcfg.VMECPU_DIR))
            time.sleep(0.1)
            os.system('vmecpu {}'.format(image_path))
        elif 'ALL_BOARD' in self.name:
            drv_exist = os.popen('lsmod | grep wishbone').readlines()
            if not drv_exist:
                os.system("insmod /lib/modules/$(uname -r)/extra/wishbone.ko")
            os.system('service pmon stop; kill -15 $(ps x | grep \"h3c_hw_mon\" | grep -v grep | awk \'{print $1}\'); sleep 5')
            for i in range(Devcfg.COMPONENT_NUM - 4):
                if not os.path.exists('/root/CPLD_{}.bin'.format(i + 1)):
                    self.log_error("CPLD_{}.bin not exits".format(i + 1))
                    return False
                bit = 1 << int(i)
                cmd = "echo {} > /sys/switch/wishbone".format(hex(bit))
                try:
                    subprocess.check_call(cmd, shell=True)
                except subprocess.CalledProcessError:
                    self.log_error("cpld update failed")
                    os.system('rmmod wishbone; service platform-modules restart; service pmon start; sleep 5')
                    return False
            os.system('rmmod wishbone; service platform-modules restart; service pmon start; sleep 5')
            return True
        elif 'BOARD-CPLD' in self.name:
            drv_exist = os.popen('lsmod | grep wishbone').readlines()
            if not drv_exist:
                os.system("insmod /lib/modules/$(uname -r)/extra/wishbone.ko")
            cpld_index = int(self.name[self.name.find("CPLD-") + 5:]
                             ) if self.name.startswith('MAIN') else int(self.name[self.name.find("CPLD-") + 5:]) + 2
            if not os.path.exists('/root/CPLD_{}.bin'.format(cpld_index)):
                self.log_error("CPLD_{}.bin not exits".format(cpld_index))
                return False
            os.system('service pmon stop; kill -15 $(ps x | grep \"h3c_hw_mon\" | grep -v grep | awk \'{print $1}\'); sleep 5')
            cmd = "echo {} > /sys/switch/wishbone".format(hex(1 << int(cpld_index - 1)))
            try:
                subprocess.check_call(cmd, shell=True)
            except subprocess.CalledProcessError:
                self.log_error("cpld update failed")
                os.system('rmmod wishbone; service platform-modules restart; service pmon start; sleep 5')
                return False
            os.system('rmmod wishbone; service platform-modules restart; service pmon start; sleep 5')
            return True
        elif 'FPGA' in self.name:
            try:
                cmd = "dom-upgrade.sh  {}".format(image_path)
                subprocess.check_call(cmd, shell=True)
            except subprocess.CalledProcessError:
                self.log_error("dom-paga update failed")
                return False
        return True
