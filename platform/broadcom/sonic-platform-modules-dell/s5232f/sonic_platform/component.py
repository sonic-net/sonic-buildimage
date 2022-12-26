#!/usr/bin/env python

########################################################################
# DELLEMC S5232F
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

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def get_bios_version():
    return subprocess.check_output(['dmidecode', '-s', 'system-version']).decode('utf-8').strip()

def get_fpga_version():
    val = hwaccess.pci_get_value('/sys/bus/pci/devices/0000:04:00.0/resource0', 0)
    return '{}.{}'.format((val >> 8) & 0xff, val & 0xff)

def get_bmc_version():
    return subprocess.check_output(
        ['cat', '/sys/class/ipmi/ipmi0/device/bmc/firmware_revision']
        ).decode('utf-8').strip()

def get_cpld_version(bus, i2caddr):
    return '{}.{}'.format(hwaccess.i2c_get(bus, i2caddr, 1),
                          hwaccess.i2c_get(bus, i2caddr, 0)
                          )

def get_cpld0_version():
    return get_cpld_version(601, 0x31)

def get_cpld1_version():
    return get_cpld_version(600, 0x30)

def get_cpld2_version():
    return get_cpld_version(600, 0x31)


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

        ['Slave CPLD 1',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 1-24, QSFP28 1-4)',
         get_cpld1_version
         ],

        ['Slave CPLD 2',
         'Used for managing SFP28/QSFP28 port transceivers (SFP28 25-48, QSFP28 5-8)',
         get_cpld2_version
         ]

    ]

    def __init__(self, component_index = 0):
        ComponentBase.__init__(self)
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

    def install_firmware(self, image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        return False
