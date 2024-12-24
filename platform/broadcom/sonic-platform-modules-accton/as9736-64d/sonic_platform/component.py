#############################################################################
# Accton
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

try:
    from sonic_platform_base.component_base import ComponentBase
    from .helper import APIHelper
    from sonic_platform_base.sonic_storage.ssd import SsdUtil
    from sonic_py_common.general import getstatusoutput_noshell_pipe
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_ADDR_MAPPING = {
    "SYSTEM CPLD": "6-0060",
    "FCM CPLD": "25-0033",
    "PSU CPLD": "36-0060",
    "SCM CPLD": "51-0035"
}

FPGA_VERSION_MAPPING = {
    "UDB FPGA": "udb_version",
    "LDB FPGA": "ldb_version",
    "SMB FPGA": "smb_version",
    "UDB CPLD1": "udb_cpld1_version",
    "UDB CPLD2": "udb_cpld2_version",
    "LDB CPLD1": "ldb_cpld1_version",
    "LDB CPLD2": "ldb_cpld2_version",
}

SYSFS_PATH = "/sys/bus/i2c/devices/"
FPGA_PATH = "/sys/devices/platform/as9736_64d_fpga/"
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"

SSD_BASE_DEVICE = ["lsblk", "-l", "-o", "NAME,TYPE,MOUNTPOINT", "-p"]
SSD_GREP_HOST   = ["grep", "-w", "host"]
SSD_GREP_DOCKER = ["grep", "-w", "share"]

COMPONENT_LIST= [
   ("SYSTEM CPLD", "SYSTEM CPLD"),
   ("FCM CPLD", "Fan Control Module CPLD"),
   ("PSU CPLD", "Power Supply Unit CPLD"),
   ("SCM CPLD", "Switch Control Module CPLD"),
   ("UDB CPLD1", "Upper Daughter Board CPLD1"),
   ("UDB CPLD2", "Upper Daughter Board CPLD2"),
   ("LDB CPLD1", "Lower Daughter Board CPLD1"),
   ("LDB CPLD2", "Lower Daughter Board CPLD2"),
   ("UDB FPGA", "Upper Daughter Board FPGA"),
   ("LDB FPGA", "Lower Daughter Board FPGA"),
   ("SMB FPGA", "Switch Main Board FPGA"),
   ("BIOS", "Basic Input/Output System"),
   ("SSD", "Solid State Drive")
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        self._api_helper=APIHelper()
        ComponentBase.__init__(self)
        self.index = component_index
        self.name = self.get_name()
        self.is_host = self._api_helper.is_host()

    def __get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def __get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        for cpld_name in CPLD_ADDR_MAPPING:
            try:
                cpld_path = "{}{}{}".format(SYSFS_PATH, CPLD_ADDR_MAPPING[cpld_name], '/version')
                cpld_version_raw= self._api_helper.read_txt_file(cpld_path)
                cpld_version[cpld_name] = "{}".format(cpld_version_raw)
            except Exception as e:
                print('Get exception when read cpld')
                cpld_version[cpld_name] = 'None'

        return cpld_version

    def __get_fpga_version(self):
        # Retrieves the FPGA firmware version
        fpga_version = dict()
        for fpga_name in FPGA_VERSION_MAPPING:
            try:
                fpga_path = "{}{}".format(FPGA_PATH, FPGA_VERSION_MAPPING[fpga_name])
                fpga_version_raw= self._api_helper.read_txt_file(fpga_path)
                fpga_version[fpga_name] = "{}".format(fpga_version_raw)
            except Exception as e:
                print('Get exception when read fpga')
                fpga_version[fpga_name] = 'None'

        return fpga_version

    def __get_ssd_version(self):
        if self.is_host:
            status, ssd_info = getstatusoutput_noshell_pipe(SSD_BASE_DEVICE, SSD_GREP_HOST)
        else:
            status, ssd_info = getstatusoutput_noshell_pipe(SSD_BASE_DEVICE, SSD_GREP_DOCKER)

        if status[0] == 0 and status[1] == 0:
            base_device = ssd_info.strip().split()[0]
            ssd_version = SsdUtil(base_device).get_firmware()
        else:
            ssd_version = 'None'

        return ssd_version

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
        fw_version = None

        if self.name == "BIOS":
            fw_version = self.__get_bios_version()
        elif any(key in self.name for key in CPLD_ADDR_MAPPING):
            cpld_version = self.__get_cpld_version()
            fw_version = cpld_version.get(self.name)
        elif any(key in self.name for key in FPGA_VERSION_MAPPING):
            fpga_version = self.__get_fpga_version()
            fw_version = fpga_version.get(self.name)
        elif self.name == "SSD":
            fw_version = self.__get_ssd_version()

        return fw_version

    def install_firmware(self, image_path):
        """
        Install firmware to module
        Args:
            image_path: A string, path to firmware image
        Returns:
            A boolean, True if install successfully, False if not
        """
        raise NotImplementedError

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
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
