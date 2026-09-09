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

class FPGA(DeviceBase):
    """
    Platform-specific FPAGclass
    """
    # Device type definition. Note, this is a constant.
    DEVICE_TYPE = "fpga"

    # List of FPGABase-derived
    _fpga_list = None

    def __init__(self, fpga_index):
        self._index = fpga_index
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = '/sys/switch/fpga/fpga{}/'.format(self._index)

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
        Retrieves the name of the FPGA

        Returns:
            String: name of FPGA
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'alias')

    def get_type(self):
        """
        Retrieves the type of the FPGA

        Returns:
            String: type of FPGA
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'type')

    def get_hw_version(self):
        """
        Retrieves the hardware version of the FPGA

        Returns:
            String: hardware version of FPGA
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'hw_version')

    def get_board_version(self):
        """
        Retrieves the board version of the FPGA

        Returns:
            String: board version of FPGA
        """
        return self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'board_version')
