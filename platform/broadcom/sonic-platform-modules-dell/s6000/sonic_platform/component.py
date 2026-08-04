#!/usr/bin/env python

########################################################################
# DELLEMC S6000
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import os
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_QUERY_VERSION_COMMAND = "dmidecode -s system-version"


class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CPLD_DIR = "/sys/devices/platform/dell-s6000-cpld.0"

    CHASSIS_COMPONENTS = [
        ["BIOS", ("Performs initialization of hardware components during "
                  "booting")],
        ["System-CPLD", "Used for managing CPU board devices and power"],
        ["Master-CPLD", ("Used for managing Fan, PSU, system LEDs, QSFP "
                         "modules (17-32)")],
        ["Slave-CPLD", "Used for managing QSFP modules (1-16)"]
    ]

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]

    def _get_cpld_register(self, reg_name):
        rv = 'ERR'
        mb_reg_file = self.CPLD_DIR+'/'+reg_name

        if (not os.path.isfile(mb_reg_file)):
            return rv

        try:
            with open(mb_reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.decode('utf-8').rstrip('\n')
        except OSError:
            result = None

        return result

    def _get_cpld_version(self, cpld_number):
        cpld_version_reg = {
            1: "system_cpld_ver",
            2: "master_cpld_ver",
            3: "slave_cpld_ver"
        }

        cpld_version = self._get_cpld_register(cpld_version_reg[cpld_number])

        if cpld_version != 'ERR':
            return str(int(cpld_version, 16))
        else:
            return 'NA'

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_model(self):
        """
        Retrieves the part number of the component
        Returns:
            string: Part number of component
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the component
        Returns:
            string: Serial number of component
        """
        return 'NA'

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the component
        Returns:
            bool: True if component is operating properly, False if not
        """
        return True

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
        Indicate whether component is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

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
        if self.index == 0:
            bios_ver = self._get_command_result(BIOS_QUERY_VERSION_COMMAND)

            if not bios_ver:
                return 'NA'
            else:
                return bios_ver

        elif self.index <= 3:
            return self._get_cpld_version(self.index)

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False
