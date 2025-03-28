#############################################################################
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

try:
    import subprocess
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform_pddf_base import pddfapi
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CPLD_SYSFS = {
    "CPLD1": {"major": "cpld1_major_ver", "minor": "cpld1_minor_ver", "build": "cpld1_build"},
    "CPLD2": {"major": "cpld2_major_ver", "minor": "cpld2_minor_ver", "build": "cpld2_build"},
}

BMC_CMDS = {
    "VER1": "ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1",
    "VER2": "ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2",
    "VER3": "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'` + 0))",
}

BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
COMPONENT_LIST= [
   ("CPLD1", "CPLD 1"),
   ("CPLD2", "CPLD 2"),
   ("BIOS", "Basic Input/Output System"),
   ("BMC", "BMC"),
]

class Component(ComponentBase):
    """Platform-specific Component class"""

    DEVICE_TYPE = "component"

    def __init__(self, component_index=0):
        self.pddf_obj = pddfapi.PddfApi()
        self.index = component_index
        self.name = self.get_name()

    def _get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def _get_cpld_version(self):
        # Retrieves the CPLD firmware version
        cpld_version = dict()
        for cpld_name, elem in CPLD_SYSFS.items():
            device = "SYSSTATUS"
            major = self.pddf_obj.get_attr_name_output(device, elem["major"])
            minor = self.pddf_obj.get_attr_name_output(device, elem["minor"])
            build = self.pddf_obj.get_attr_name_output(device, elem["build"])
            if major and minor and build:
                major = int(major['status'].rstrip(),0)
                minor = int(minor['status'].rstrip(),0)
                build = int(build['status'].rstrip(),0)
                cpld_version[cpld_name] = "{}.{:02d}.{:03d}".format(major, minor, build)
            else:
                cpld_version[cpld_name] = "N/A"
        return cpld_version

    def _get_bmc_version(self):
        # Retrieves the BMC firmware version
        bmc_ver = dict()
        for ver in BMC_CMDS:
            status, value = subprocess.getstatusoutput(BMC_CMDS[ver])
            if not status:
                bmc_ver[ver] = int(value.rstrip())
            else:
                return None

        bmc_version = "{}.{}.{}".format(bmc_ver["VER1"], bmc_ver["VER2"], bmc_ver["VER3"])

        return bmc_version

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
            fw_version = self._get_bios_version()
        elif "CPLD" in self.name:
            cpld_version = self._get_cpld_version()
            fw_version = cpld_version.get(self.name)
        elif self.name == "BMC":
            fw_version = self._get_bmc_version()
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
