#!/usr/bin/env python

########################################################################
# Clounix
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import os
    import re
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -s bios-version"


class Component(ComponentBase):
    """Clounix Platform-specific Component class"""
    def __init__(self, component_index,component_conf):
        self.__index = component_index
        self.__conf = component_conf
        self.__api_helper = APIHelper()

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.__conf[self.__index]['name']

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.__conf[self.__index]['description']

    def get_version_path(self):
        """
        Retrieves the version path of the component

        Returns:
            A string containing the version path of the component
        """
        return self.__conf[self.__index]['firmware_version_path']

    def get_hw_version_path(self):
        """
        Retrieves the version path of the component

        Returns:
            A string containing the version path of the component
        """
        return self.__conf[self.__index]['hw_version_path']
    """
    def get_hw_version(self):
        hw_version = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.get_hw_version_path())
        if attr_rv != None:
            hw_version = attr_rv
        else:
            if self.get_name() == 'BIOS':
                ret,bios_ver = self.__api_helper.run_command(BIOS_QUERY_VERSION_COMMAND)
                if ret:
                    if bios_ver:
                        hw_version = re.findall('\d',bios_ver)[0]
        return hw_version
    """


    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        firmware_version = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.get_version_path())
        if attr_rv != None:
            firmware_version = attr_rv
        else:
            if self.get_name() == 'BIOS':
                ret,bios_ver = self.__api_helper.run_command(BIOS_QUERY_VERSION_COMMAND)
                if ret:
                    if bios_ver:
                        firmware_version = bios_ver
        return firmware_version

    def get_hw_version(self):
        return self.get_firmware_version()

    def check_file_validity(self, image_path):
        """
        Check installation file validation

        Returns:
            bool: True if installation file is valid, False if not
        """
 
        if not os.path.isfile(image_path):
            print("ERROR: File {} doesn't exist or is not a file".format(image_path))
            return False
        file_ext_dist = {'MCU-MB':'.efm8', 'MCU-FB':'.efm8', 'CPLD-A':'.vme', 'CPLD-B':'.vme','CPLD-C':'.vme','CPLD-D':'.vme','FPGA':'.bin', 'BIOS':'.bin'}
        name_list = os.path.splitext(image_path)
        image_ext_name = file_ext_dist[self.get_name()] 
        if image_ext_name is not None:
            if name_list[1] != image_ext_name:
                print("ERROR: Extend name of file {} is wrong. Image for {} should have extend name {}".format(image_path, self.get_name(), image_ext_name))
                return False
        file_name_dist = {'MCU-MB':'mcu_mb', 'MCU-FB':'mcu_fb', 'CPLD-A':'cpld_mb', 'CPLD-B':'cpld_mb','CPLD-C':'cpld_mb','CPLD-D':'cpld_cpu', 'FPGA':'fpga_mb', 'BIOS':'bios_cpu'}
        image_key_name = file_name_dist[self.get_name()] 
        if image_key_name is not None:
            if not image_key_name in image_path:
                print("ERROR: File name of file {} is wrong. Image for {} should have key name {}".format(image_path, self.get_name(), image_key_name))
                return False

        return True

    def version_calc_value(self, version):
        """
        transfer version string to value, eg 1.2.3.4 tranfer to 1 * 100 + 2 *1 + 3 * 0.01 + 4 * 0.0001

        Returns:
            version value
        """
        version_split = version.split('.')
        sum = 0
        factor=100
        for val in version_split:
            sum += float(val) * factor
            factor = factor * 0.01
        return sum

    def check_version_validity(self, image_path):
        """
        Check installation version validation

        Returns:
            bool: True if installation version is valid, False if not
        """

        run_version = self.get_firmware_version()

        install_version = re.findall(r'v(.+?)\.', image_path)
        if install_version:
            version = ''.join(install_version).replace('_', '.')
        else:
            print("please make sure installation version is from released version like as _V0_10.xxx")
            return False

        version_val = self.version_calc_value(version)
        run_version_val = self.version_calc_value(run_version)
        if version_val == run_version_val:
            print("skip installation version %.2f for runing version %.2f." % (version_val, run_version_val))
            return False
 
        return True

    def check_image_validity(self, image_path):
        """
        Check installation image validation

        Returns:
            bool: True if installation is valid, False if not
        """
 
        if not self.check_file_validity(image_path):
            return False

        if not self.check_version_validity(image_path):
            return False

        return True

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        if not self.check_image_validity(image_path):
            return False

        if self.get_name() == 'BIOS':
            cmd = "/usr/local/bin/install_firmware.sh bios "+ image_path
        elif self.get_name() == 'MCU-MB':
            cmd = "/usr/local/bin/install_firmware.sh mcu-mb "+ image_path
        elif self.get_name() == 'MCU-FB':
            cmd = "/usr/local/bin/install_firmware.sh mcu-fb "+ image_path
        elif self.get_name() == 'CPLD-A' or self.get_name() == 'CPLD-B' or self.get_name() == 'CPLD-C':
            cmd = "/usr/local/bin/install_firmware.sh cpld-mb "+ image_path
        elif self.get_name() == 'CPLD-D':
            cmd = "/usr/local/bin/install_firmware.sh cpld-cpu "+ image_path
        elif self.get_name() == 'FPGA':
            cmd = "/usr/local/bin/install_firmware.sh fpga "+ image_path
        else:
            print(self.get_name() + 'frimware upgrade did not implement')

        os.system(cmd)
        #start service
        os.system("/usr/local/bin/install_firmware_ext.sh start")
        return True

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = 'N/A'
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = 'N/A'
        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
