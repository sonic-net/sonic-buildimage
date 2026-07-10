#############################################################################
# SuperMicro SSE-T7132S
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_LIST = [
    ("BIOS",        "Basic Input/Output System"),
    ("BMC",         "Baseboard Management Controller"),
    ("SWITCH_CPLD1", "Switch board CPLD1"),
    ("SWITCH_CPLD2", "Switch board CPLD2")
]
SW_CPLD1_VER_PATH = "/sys/devices/platform/switchboard/CPLD1/ver_bmc_i2c"
SW_CPLD2_VER_PATH = "/sys/devices/platform/switchboard/CPLD2/ver_bmc_i2c"
CPLD_UPGRADE_OPT = 4
BIOS_VER_PATH = "/sys/class/dmi/id/bios_version"
BIOS__UPGRADE_OPT = 2
BMC_VER_CMD = "ipmitool mc info | grep 'Firmware Revision'"
IPMI_BMC_VER_NETFN = "0x6"
IPMI_BMC_VER_CMD = "0x1"
BMC_VER_MAJOR_OFFSET = 2
BMC_VER_MINOR_OFFSET = 3
BMC_VER_AUX_OFFSET = 11
BMC_UPGRADE_OPT = 1
CFUFLASH_FW_UPGRADE_CMD = "CFUFLASH -cd -d {} -mse 3 {}"
MEM_PCI_RESOURCE = "/sys/bus/pci/devices/0000:09:00.0/resource0"
FPGA_VER_MEM_OFFSET = 0


class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index):
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self._api_helper = APIHelper()

    def __get_bmc_ver(self):
        bmc_ver = "Unknown"
        status, raw_bmc_data = self._api_helper.ipmi_raw(
            IPMI_BMC_VER_NETFN, IPMI_BMC_VER_CMD)
        if status:
            bmc_ver_data_list = raw_bmc_data.split()
            bmc_ver = '{}.{}.{}'.format(bmc_ver_data_list[BMC_VER_MAJOR_OFFSET],
                                        bmc_ver_data_list[BMC_VER_MINOR_OFFSET],
                                        bmc_ver_data_list[BMC_VER_AUX_OFFSET])
        return bmc_ver

    def __get_cpld_ver(self,path):
        cpld_data = self._api_helper.read_txt_file(path)
        cpld_ver = cpld_data[-2:]

        return cpld_ver

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
        Retrieves the firmware version of module
        Returns:
            string: The firmware versions of the module
        """
        fw_version = {
            "BIOS": self._api_helper.read_txt_file(BIOS_VER_PATH),
            "BMC": self.__get_bmc_ver(),
            "SWITCH_CPLD1": self.__get_cpld_ver(SW_CPLD1_VER_PATH),
            "SWITCH_CPLD2": self.__get_cpld_ver(SW_CPLD2_VER_PATH),
        }.get(self.name, "Unknown")

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        """Not Implement"""
        return False
