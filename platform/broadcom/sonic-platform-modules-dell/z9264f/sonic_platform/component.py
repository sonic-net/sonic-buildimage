#!/usr/bin/env python

########################################################################
# DELLEMC Z9264F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, BMC etc.) available in
# the platform
#
########################################################################

try:
    import re
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FIRMWARE_VERSION_FILE="/var/log/firmware_versions"

class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", ("Performs initialization of hardware components during "
                  "booting")],
        ["FPGA", ("Used for managing the system LEDs")],
        ["BMC", ("Platform management controller for on-board temperature "
                 "monitoring, in-chassis power, Fan and LED control")],
        ["System CPLD", ("Used for managing the CPU power sequence and CPU states")],
        ["Slave CPLD 1", ("Used for managing QSFP/QSFP28 port transceivers (1-16)")],
        ["Slave CPLD 2", ("Used for managing QSFP/QSFP28 port transceivers (17-32)")],
        ["Slave CPLD 3", ("Used for managing QSFP/QSFP28 port transceivers (33-48)")],
        ["Slave CPLD 4", ("Used for managing QSFP/QSFP28 port transceivers (49-64) and SFP/SFP28 "
                          "port transceivers (65 and 66)")],
    ]

    def __init__(self, component_index=0):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]

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
        rv = ""
        try:
            fd = open(FIRMWARE_VERSION_FILE,"r")
        except IOError:
            return rv
        version_contents = fd.read()
        fd.close()
        if not version_contents:
            return rv
        if self.index < 8:
            version = re.search(r''+self.CHASSIS_COMPONENTS[self.index][0]+':(.*)',version_contents)
            if version:
                rv = version.group(1).strip()
        return rv

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        return False
