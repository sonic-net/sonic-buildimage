########################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import os
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"


class Component(ComponentBase):
    def __init__(self, idx, name, descript):
        self.index = idx
        self.name = name
        self.description = descript

    def _get_cpld_register(self, syspath):
        rv = 'ERR'
        if (not os.path.isfile(syspath)):
            return rv
        # noinspection PyBroadException
        try:
            with open(syspath, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'
        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _get_bios_version(self):
        # Retrieves the BIOS firmware version
        try:
            with open(BIOS_VERSION_PATH, 'r') as fd:
                bios_version = fd.read()
                return bios_version.strip()
        except Exception as e:
            return None

    def _get_cpld_version(self, cpld_number):
        cpld_version_reg = {
            1: "/sys/class/hwmon/hwmon2/device/NBA715_SYS/cpld_30_ver",
            2: "master_cpld_ver",
            3: "slave_cpld_ver"
        }

        cpld_version = self._get_cpld_register(cpld_version_reg[cpld_number])

        if cpld_version != 'ERR':
            return cpld_version[-4:]

        return 'NA'

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
        if self.index == 0:
            bios_ver = self._get_bios_version()
            if not bios_ver:
                return 'NA'

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
