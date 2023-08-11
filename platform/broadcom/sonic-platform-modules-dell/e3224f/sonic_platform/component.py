#!/usr/bin/env python

########################################################################
# DELLEMC E3224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, BMC etc.) available in
# the platform
#
########################################################################

try:
    import subprocess
    from sonic_platform_base.component_base import ComponentBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
def get_bios_version():
    return subprocess.check_output(['dmidecode', '-s', 'system-version']).strip().decode()

def get_cpld_version(cpld):
    mjr_ver_path = '/sys/devices/platform/dell-e3224f-cpld.0/' + cpld + '_mjr_ver'
    mnr_ver_path = '/sys/devices/platform/dell-e3224f-cpld.0/' + cpld + '_mnr_ver'
    mjr_ver = subprocess.check_output(['cat', mjr_ver_path]).strip()[2:].decode()
    mnr_ver = subprocess.check_output(['cat', mnr_ver_path]).strip()[2:].decode()
    return (str(mjr_ver) + '.' + str(mnr_ver))

class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ['BIOS',
         'Performs initialization of hardware components during booting',
         get_bios_version()
        ],
        ['CPU CPLD',
         'Used for managing the CPU power sequence and CPU states',
         get_cpld_version('cpu_cpld')
        ],
        ['SYS CPLD',
         'Used for managing FAN, PSU, SFP+ modules (25-28) and QSFP modules (29-30)',
         get_cpld_version('sys_cpld')
        ],
        ['PORT CPLD',
         'Used for managing SFP modules (1-24)',
         get_cpld_version('port_cpld')
        ]
    ]

    def __init__(self, component_index=0):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.version = self.CHASSIS_COMPONENTS[self.index][2]

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
        return self.version

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        return False

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        return True

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
