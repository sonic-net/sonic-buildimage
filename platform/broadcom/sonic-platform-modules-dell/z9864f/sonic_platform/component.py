#!/usr/bin/env python
"""
########################################################################
# DELL Z9864F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, BMC etc.) available in
# the platform
#
########################################################################
"""

try:
    import re
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
    import sonic_platform.hwaccess as hwaccess

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


def get_bios_version():
    """ Returns BIOS Version """
    bios_ver = ''
    try:
        bios_ver =  subprocess.check_output(['dmidecode', '-s', 'system-version']).strip()
        if type(bios_ver) == bytes:
            bios_ver = bios_ver.decode()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass
    return bios_ver


def get_fpga_version():
    """ Returns FPGA Version """
    val = hwaccess.pci_get_value(hwaccess.get_fpga_buspath(), 4)
    return '{}.{}'.format((val & 0xff),0)

def get_bmc_version():
    """ Returns BMC Version """
    bmc_ver = ''
    try:
        bmc_ver = subprocess.check_output(
            "ipmitool mc info | awk '/Firmware Revision/ { print $NF }'",
            shell=True, text=True).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass
    return bmc_ver

def get_cpld_version(bus, i2caddr, regaddr):
    """ Returns CPLD Major.Minor Version at Byte1.Byte0 @ given i2c address """
    major = hwaccess.i2c_get(bus, i2caddr, regaddr)
    if major != -1:
        return '{}.{}'.format(major, 0)
    return ''

def get_cpld0_version():
    """ Returns System CPLD Version """
    return get_cpld_version(hwaccess.get_smbus_i801_devnum(), 0x3d,1)

def get_cpld1_version():
    """ Returns Secondary CPLD1 Version """
    return get_cpld_version(600, 0x41,0)

def get_cpld2_version():
    """ Returns Secondary CPLD2 Version """
    return get_cpld_version(600, 0x45,0)

def _get_pcie_fw_version():
    """ Returns PCIe Firmware Version """
    pcie_ver = ''
    try:
        cmd_out =  subprocess.check_output(['bcmcmd', "dsh -c \"pciephy fwinfo\""], text=True).strip()
        result = re.search(r'(?<=PCIe FW loader version:).*', cmd_out)
        if result is not None:
            pcie_ver = result.group(0).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass

    return pcie_ver

class Component(ComponentBase):
    """DELL Platform-specific Component class"""

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
         'Platform management controller for on-board temperature '
         'monitoring, in-chassis power, Fan and LED control',
         get_bmc_version
        ],

        ['System CPLD',
         'Used for managing the CPU power sequence and CPU states',
         get_cpld0_version
        ],

        ['Secondary CPLD 1',
         'Used for managing OSFP112 port transceivers (OSFP112 1-32)',
         get_cpld1_version
        ],

        ['Secondary CPLD 2',
         'Used for managing OSFP112 port transceivers (OSFP112 33-64)',
         get_cpld2_version
        ],
        ['PCIe',
         'ASIC PCIe firmware',
         _get_pcie_fw_version
        ]
    ]

    def __init__(self, component_index=0):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.version = self.CHASSIS_COMPONENTS[self.index][2]()

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

    @staticmethod
    def install_firmware(image_path):
        """
        Installs firmware to the component
        Args:
        image_path: A string, path to firmware image
        Returns:
        A boolean, True if install was successful, False if not
        """
        del image_path
        return False
