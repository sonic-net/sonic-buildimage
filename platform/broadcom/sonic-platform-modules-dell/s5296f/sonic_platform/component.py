#!/usr/bin/env python

########################################################################
# DELLEMC S5296F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, BMC etc.) available in
# the platform
#
########################################################################


try:
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
    import sonic_platform.hwaccess as hwaccess
    from sonic_py_common.general import check_output_pipe

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def get_bios_version():
    return subprocess.check_output(['dmidecode', '-s',
        'system-version']).decode('utf-8').strip()

def get_fpga_version():
    val = hwaccess.pci_get_value('/sys/bus/pci/devices/0000:04:00.0/resource0', 0)
    return '{}.{}'.format((val >> 8) & 0xff, val & 0xff)

def get_bmc_version():
    """ Returns BMC Version """
    bmc_ver = ''
    try:
        bmc_ver = check_output_pipe(["ipmitool", "mc", "info"], ["awk", "/Firmware Revision/ { print $NF }"])
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass
    return bmc_ver

def get_cpld_version(bus, i2caddr):
    """ Return CPLD version """
    major = hwaccess.i2c_get(bus, i2caddr, 1)
    minor = hwaccess.i2c_get(bus, i2caddr, 0)
    if major != -1 and minor != -1:
        return '{}.{}'.format(major, minor)
    return ''

def get_cpld0_version():
    return get_cpld_version(601, 0x31)

def get_cpld1_version():
    return get_cpld_version(600, 0x30)

def get_cpld2_version():
    return get_cpld_version(600, 0x31)

def get_cpld3_version():
    return get_cpld_version(600, 0x32)

def get_cpld4_version():
    return get_cpld_version(600, 0x33)


class Component(ComponentBase):
    """DellEMC Platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ['BIOS',
         'Performs initialization of hardware components during booting',
         get_bios_version
         ],

        ['FPGA',
         'Used for managing the system LEDs',
         get_fpga_version
         ],

        ['BMC',
         'Platform management controller for on-board temperature monitoring, in-chassis power, Fan and LED control',
         get_bmc_version
         ],

        ['System CPLD',
         'Used for managing the CPU power sequence and CPU states',
         get_cpld0_version
         ],

        ['Secondary CPLD 1',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 1-24, QSFP28 1-4)',
         get_cpld1_version
         ],

        ['Secondary CPLD 2',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 25-48, QSFP28 5-8)',
         get_cpld2_version
         ],

        ['Secondary CPLD 3',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 49-72)',
         get_cpld3_version
         ],

        ['Secondary CPLD 4',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 73-96)',
         get_cpld4_version
         ]
    ]

    def __init__(self, component_index = 0):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.version = None

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
        if self.version == None:
            self.version = self.CHASSIS_COMPONENTS[self.index][2]()
        return self.version

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

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        return False

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component
        Note: the firmware version will be read from image
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"

    def get_firmware_update_notification(self, image_path):
        """
        Retrieves a notification on what should be done in order to complete
        the component firmware update
        Args:
            image_path: A string, path to firmware image
        Returns:
            A string containing the component firmware update notification if required.
            By default 'None' value will be used, which indicates that no actions are required
        """
        return "None"

    def update_firmware(self, image_path):
        """
        Updates firmware of the component
        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API
        Args:
            image_path: A string, path to firmware image
        Raises:
            RuntimeError: update failed
        """
        return False
