########################################################################
# Ragile RA-B6510-32c
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    from sonic_platform_base.component_base import ComponentBase
    import sonic_platform.hwaccess as hwaccess
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

def get_cpld_version(bus, i2caddr):
    return '{}{}{}{}'.format(hwaccess.i2c_get(bus, i2caddr, 1),
                          hwaccess.i2c_get(bus, i2caddr, 2),
                          hwaccess.i2c_get(bus, i2caddr, 3),
                          hwaccess.i2c_get(bus, i2caddr, 0)
                          )

def get_cpu_cpld_version():
    return get_cpld_version(6, 0x0d)

def get_fan_cpld_version():
    return get_cpld_version(2, 0x0d)

def get_cpld1_version():
    return get_cpld_version(8, 0x30)

def get_cpld2_version():
    return get_cpld_version(8, 0x31)

def get_fpga_version():
    version = hwaccess.pci_get_value('/sys/bus/pci/devices/0000:08:00.0/resource0', 0)
    datetime = hwaccess.pci_get_value('/sys/bus/pci/devices/0000:08:00.0/resource0',4)
    return "%08x-%08x"%(version,datetime)

COMPONENT_LIST= [
        ['CPU CPLD',
         'cpu board',
         get_cpu_cpld_version
         ],

        ['CPU FAN CPLD',
         'cpu fan',
         get_fan_cpld_version
         ],

        ['MAC1 CPLD',
         'mac1 board',
         get_cpld1_version
         ],

        ['MAC2 CPLD',
         'mac2 board',
         get_cpld2_version
         ],
        ['FPGA',
         'fpga version',
         get_fpga_version
         ]
    ]

class Component(ComponentBase):
    """ Ragile Platform-specific Component class"""

    def __init__(self, component_index=0):
        ComponentBase.__init__(self)
        self.index = component_index

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return COMPONENT_LIST[self.index][0]

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return COMPONENT_LIST[self.index][1]

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        return COMPONENT_LIST[self.index][2]()

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        try:
            successtips = "CPLD Upgrade succeeded!"
            status, output = getstatusoutput_noshell(["which", "firmware_upgrade"])
            if status or len(output) <= 0:
                logger.error("no upgrade tool.")
                return False
            cmdstr = [output, image_path, "cpld", str(self.slot), "cpld"]
            ret, log = getstatusoutput_noshell(cmdstr)
            if ret == 0 and successtips in log:
                return True
            logger.error("upgrade failed. ret:%d, log:\n%s" % (ret, log))
        except Exception as e:
            logger.error(str(e))
        return False
        
