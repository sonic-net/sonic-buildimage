#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

import subprocess

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FPGA_VERSION_PATH = "/sys/devices/platform/fpga-board/version"
SYSCPLD_VERSION_PATH = "/sys/devices/platform/sys_cpld/version"
SWCPLD1_VERSION_PATH = "/sys/bus/i2c/devices/i2c-10/10-0030/version"
SWCPLD2_VERSION_PATH = "/sys/bus/i2c/devices/i2c-10/10-0031/version"
SWCPLD3_VERSION_PATH = "/sys/bus/i2c/devices/i2c-10/10-0032/version"
SWCPLD4_VERSION_PATH = "/sys/bus/i2c/devices/i2c-10/10-0033/version"
COMeCPLD_VERSION_cmd= "ipmitool raw 0x3a 0x3e 1 0x1a 1 0xe0"
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
Main_BMC_cmd = "ipmitool raw 0x32 0x8f 0x08 0x01"
Backup_BMC_cmd = "ipmitool raw 0x32 0x8f 0x08 0x02"
#Fan_CPLD_cmd = "ipmitool raw 0x3a 0x64 02 01 00"
COMPONENT_NAME_LIST = ["FPGA", "SYSCPLD", "SWCPLD1", "SWCPLD2", "SWCPLD3","SWCPLD4", "COMeCPLD", "Main_BMC", "Backup_BMC", "Main_BIOS", "Backup_BIOS"]
COMPONENT_DES_LIST = ["Used for managering the CPU and expanding I2C channels", "Used for managing the CPU",
                      "Used for managing QSFP+ ports (1-16)", "Used for managing QSFP+ ports (17-32)", "Used for managing QSFP+ ports (33-48)", "Used for managing QSFP+ ports (49-64)","Used for managing the COMe","Main Baseboard Management Controller", "Backup Baseboard Management Controller", "Main basic Input/Output System", "Backup basic Input/Output System"]


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self._api_helper = APIHelper()
        self.name = self.get_name()

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        status,result = self._api_helper.run_command("ipmitool raw 0x3a 0x64 00 01 0x70")
        if result.strip() == "01":
            if self.name == "Main_BIOS":
                with open(BIOS_VERSION_PATH, 'r') as fd:
                    bios_version = fd.read()
                    return bios_version.strip()
            elif self.name == "Backup_BIOS":
                bios_version = "na"
                return bios_version
                
        elif result.strip() == "03": 
            if self.name == "Backup_BIOS":
                with open(BIOS_VERSION_PATH, 'r') as fd:
                    bios_version = fd.read()
                    return bios_version.strip()
            elif self.name == "Main_BIOS":
                bios_version = "na"
                return bios_version
                
    def get_register_value(self, register):
        # Retrieves the cpld register value
        cmd = "echo {1} > {0}; cat {0}".format(GETREG_PATH, register)
        p = subprocess.Popen(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        raw_data, err = p.communicate()
        if err != '':
            return None
        return raw_data.strip()

    def __get_cpld_version(self):
        if self.name == "SYSCPLD":
            try:
                with open(SYSCPLD_VERSION_PATH, 'r') as fd:
                    syscpld_version = fd.read()
                    return syscpld_version.strip()
            except Exception as e:
                return None
        elif self.name == "SWCPLD1":
            try:
                with open(SWCPLD1_VERSION_PATH, 'r') as fd:
                    swcpld1_version = fd.read()
                    return swcpld1_version.strip()
            except Exception as e:
                return None
        elif self.name == "SWCPLD2":
            try:
                with open(SWCPLD2_VERSION_PATH, 'r') as fd:
                    swcpld2_version = fd.read()
                    return swcpld2_version.strip()
            except Exception as e:
                return None
        elif self.name == "SWCPLD3":
            try:
                with open(SWCPLD3_VERSION_PATH, 'r') as fd:
                    swcpld3_version = fd.read()
                    return swcpld3_version.strip()
            except Exception as e:
                return None
        elif self.name == "SWCPLD4":
            try:
                with open(SWCPLD4_VERSION_PATH, 'r') as fd:
                    swcpld4_version = fd.read()
                    return swcpld4_version.strip()
            except Exception as e:
                return None
        elif self.name == "COMeCPLD":
            status, ver = self._api_helper.run_command(COMeCPLD_VERSION_cmd)
            version1 = int(ver.strip()) / 10
            version2 = int(ver.strip()) % 10
            version = "%s.%s" % (version1, version2)
            return str(version)
        elif self.name == "FANCPLD":
            status,ver = self._api_helper.run_command(Fan_CPLD_cmd)
            version = int(ver.strip(), 16)
            return str(version)
 
    def __get_fpga_version(self):
            # Retrieves the FPGA firmware version
            try:
                with open(FPGA_VERSION_PATH, 'r') as fd:
                    version = fd.read()
                    fpga_version = (version.strip().split("x")[1])
                    return fpga_version.strip()
            except Exception as e:
                return None

    def __get_bmc_version(self):
            # Retrieves the BMC firmware version
            cmd = Main_BMC_cmd if self.name == "Main_BMC" else Backup_BMC_cmd
            stasus, ver = self._api_helper.run_command(cmd)
            return ver.strip()
 
    def get_name(self):
        """
        Retrieves the name of the component
         Returns:
            A string containing the name of the component
        """
        return COMPONENT_NAME_LIST[self.index]

    def get_description(self):
        """
        Retrieves the description of the component
            Returns:
            A string containing the description of the component
        """
        return COMPONENT_DES_LIST[self.index]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = None
 
        if "BIOS" in self.name:
            fw_version = self.__get_bios_version()
        elif "CPLD" in self.name:
            fw_version = self.__get_cpld_version()
        elif self.name == "FPGA":
            fw_version = self.__get_fpga_version()
        elif "BMC" in self.name:
            version = self.__get_bmc_version()
            version_1 = int(version.strip().split(b" ")[0], 16)
            version_2 = int(version.strip().split(b" ")[1], 16)
            fw_version = "%s.%s" % (version_1, version_2)

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """

        #if not os.path.isfile(image_path):
        #    return False

        #op_cmd = "ipmitool raw 0x32 0xaa 0x00"
        #cl_cmd = "ipmitool raw 0x32 0xaa 0x01"
        #if "FPGA" in self.name:

        #    """img_name = os.path.basename(image_path)
        #    root, ext = os.path.splitext(img_name)
        #    ext = ".vme" if ext == "" else ext
        #    new_image_path = os.path.join("/tmp", (root.lower() + ext))
        #    shutil.copy(image_path, new_image_path)"""
        #    install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/fpga_prog /sys/bus/pci/devices/0000:0b:00.0/resource0 %s" % image_path
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Programing is complete' in result:
        #        print("Update success")
        #    else:
        #        print("Update failed")

        #elif self.name == "Main_BIOS":
        #    self._api_helper.run_command(op_cmd)
        #            time.sleep(5)
        #            install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 2 -mse 1 %s " % image_path
        #            print("Updating now...Plz wait...")
        #            status, result = self._api_helper.run_command(install_command)
        #            if status == False:
        #                print("Running install command error")
        #            if 'Beginning to Deactive flashMode...end' in result:
        #                print("Update success")
        #            else:
        #               print("Update failed")
        #           time.sleep(10)
        #            self._api_helper.run_command(cl_cmd)
                     
        #        elif self.name == "Backup_BIOS":
        #            self._api_helper.run_command(op_cmd)
        #            time.sleep(5)
        #            install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 2 -mse 2 %s " % image_path
        #            print("Updating now...Plz wait...")
        #            status, result = self._api_helper.run_command(install_command)
        #            if status == False:
        #                print("Running install command error")
        #            if 'Beginning to Deactive flashMode...end' in result:
        #                print("Update success")
        #            else:
        #                print("Update failed")
        #            time.sleep(10)
        #            self._api_helper.run_command(cl_cmd)
                  
        #        elif self.name == "Main_BMC":
        #            self._api_helper.run_command(op_cmd)
        #            time.sleep(5)
        #            install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -fb -d 1 -mse 1 %s" % image_path
        #            print("Updating now...Plz wait...")
        #            status, result = self._api_helper.run_command(install_command)
        #            if status == False:
        #                print("Running install command error")
        #            if 'Beginning to Deactive flashMode...end' in result:
        #                print("Update success")
        #                print("BMC is rebooting now...Plz wait for about 180s")
        #            else:
        #                print("Update failed")
            
        #        elif self.name == "Backup_BMC":
        #            self._api_helper.run_command(op_cmd)
        #            time.sleep(5)
        #            install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -fb -d 1 -mse 2 %s" % image_path
        #            print("Updating now...Plz wait...")
        #            status, result = self._api_helper.run_command(install_command)
        #            if status == False:
        #                print("Running install command error")
        #            if 'Beginning to Deactive flashMode...end' in result:
        #                print("Update success")
        #                print("BMC is rebooting now...")
        #            else:
        #                print("Update failed")
                    
        #        elif "CPLD" in self.name:
        #            self._api_helper.run_command(op_cmd)
        #            time.sleep(5)
        #            install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 4 %s " % image_path
        #            print("Updating now...Plz wait...")
        #            status, result = self._api_helper.run_command(install_command)
        #            if status == False:
        #                print("Running install command error")
        #            if 'Beginning to Deactive flashMode...end' in result:
        #                print("Update success")
        #            else:
        #                print("Update failed")
        #            time.sleep(10)
        #            self._api_helper.run_command(cl_cmd)

        #not support because firmare update need 3rd tools that can't be submitted to community
        return False

    def update_firmware(self, image_path):
        #if not os.path.isfile(image_path):
        #    return False

        #op_cmd = "ipmitool raw 0x32 0xaa 0x00"
        #cl_cmd = "ipmitool raw 0x32 0xaa 0x01"
        #if "FPGA" in self.name:

        #    """img_name = os.path.basename(image_path)
        #    root, ext = os.path.splitext(img_name)
        #    ext = ".vme" if ext == "" else ext
        #    new_image_path = os.path.join("/tmp", (root.lower() + ext))
        #    shutil.copy(image_path, new_image_path)"""
        #    install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/fpga_prog /sys/bus/pci/devices/0000:0b:00.0/resource0 %s" % image_path
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Programing is complete' in result:
        #        print("Update success")
        #    else:
        #        print("Update failed")

        #elif self.name == "Main_BIOS":
        #    self._api_helper.run_command(op_cmd)
        #    time.sleep(5)
        #    install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 2 -mse 1 %s " % image_path
        #    print("Updating now...Plz wait...")
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Beginning to Deactive flashMode...end' in result:
        #        print("Update success")
        #    else:
        #        print("Update failed")
        #    time.sleep(10)
        #    self._api_helper.run_command(cl_cmd)
            
        #elif self.name == "Backup_BIOS":
        #    self._api_helper.run_command(op_cmd)
        #    time.sleep(5)
        #    install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 2 -mse 2 %s " % image_path
        #    print("Updating now...Plz wait...")
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Beginning to Deactive flashMode...end' in result:
        #        print("Update success")
        #    else:
        #        print("Update failed")
        #    time.sleep(10)
        #    self._api_helper.run_command(cl_cmd)
         
        #elif self.name == "Main_BMC":
        #    self._api_helper.run_command(op_cmd)
        #    time.sleep(5)
        #    install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -fb -d 1 -mse 1 %s" % image_path
        #    print("Updating now...Please wait...")
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Beginning to Deactive flashMode...end' in result:
        #        print("Update success")
        #        print("BMC is rebooting now...Please wait for about 180s")
        #    else:
        #        print("Update failed")
            
        #elif self.name == "Backup_BMC":
        #    self._api_helper.run_command(op_cmd)
        #    time.sleep(5)
        #    install_command = "/usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -fb -d 1 -mse 2 %s" % image_path
        #    print("Updating now...Please wait...")
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Beginning to Deactive flashMode...end' in result:
        #        print("Update success")
        #        print("BMC is rebooting now...")
        #    else:
        #        print("Update failed")
            
        #elif "CPLD" in self.name:
        #    self._api_helper.run_command(op_cmd)
        #    time.sleep(5)
        #    install_command = "echo y | /usr/local/lib/firmware/x86_64-cel_silverstone-x-r0/CFUFLASH -cd -d 4 %s " % image_path
        #    print("Updating now...Please wait...")
        #    status, result = self._api_helper.run_command(install_command)
        #    if status == False:
        #        print("Running install command error")
        #    if 'Beginning to Deactive flashMode...end' in result:
        #        print("Update success")
        #    else:
        #        print("Update failed")
        #    time.sleep(10)
        #    self._api_helper.run_command(cl_cmd)
        #not support firmware update because 3rd tools can't be uploaded to community
        return False

