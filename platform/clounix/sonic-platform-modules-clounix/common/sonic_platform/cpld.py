########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################

try:
    from sonic_platform_base.device_base import DeviceBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class CPLD(DeviceBase):
    """
    Platform-specific CPLD class
    """
    # Device type definition. Note, this is a constant.
    DEVICE_TYPE = "cpld"

    # List of CPLDBase-derived
    _cpld_list = None

    def __init__(self, cpld_index):
        self._index = cpld_index
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = '/sys/switch/cpld/cpld{}/'.format(self._index)
 
        #self.sysfs = SYSFS(self._cpld_path)

    def get_debug(self):
        """
        Retrieves the debug of sensors

        Returns:
            String, the debug of sensors on this chassis
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'debug')

    def get_loglevel(self):
        """
        Retrieves the loglevel of sensors

        Returns:
            Int, the loglevel of sensors on this chassis
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'loglevel')

    def set_loglevel(self, loglevel):
        """
        Set the loglevel of sensors

        Args:
            loglevel: An integer, the loglevel of sensors

        Returns:
            A boolean, True if loglevel is set successfully, False if not
        """
        return self.__api_helper.write_txt_file(self.__attr_path_prefix + 'loglevel',loglevel)

    def get_name(self):
        """
        Retrieves the name of the CPLD

        Returns:
            String: name of CPLD
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'alias')

    def get_type(self):
        """
        Retrieves the type of the CPLD

        Returns:
            String: type of CPLD
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'type')

    def get_hw_version(self):
        """
        Retrieves the hardware version of the CPLD

        Returns:
            String: hardware version of CPLD
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'hw_version')

    def get_board_version(self):
        """
        Retrieves the board version of the CPLD

        Returns:
            String: board version of CPLD
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'board_version')
