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
    from sonic_platform.regutil import Reg
    from sonic_platform.logger import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(ComponentBase):
    """ Ragile Platform-specific Component class"""

    def __init__(self, index, config=None):
        self.index = index
        self.name = config.get("name")
        self._reg_fm_ver = Reg(config.get("firmware_version"))
        self.description = config.get("desc")
        self.slot = config.get("slot")

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
        try:
            return self._reg_fm_ver.decode()
        except Exception as e:
            logger.error(str(e))

        return ""

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
 
