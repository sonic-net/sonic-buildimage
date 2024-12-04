#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    from sonic_py_common.logger import Logger
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


logger = Logger("thermal")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, index, name, sysfile_path, is_bmc, support_mask=0x1, ext_sysfile_list=None):
        # index is used to indicate the temp{} under sffile_path
        # support_mask:  1:support  0:not support
        #   bit 0 : temperature (always 1)
        #   bit 1 : high threshold
        #   bit 2 : low threshold
        #   bit 3 : high critical threshold
        #   bit 4 : low critical threshold
        #   bit 7 : cpu internal sensor
        # ext_sysfile_list: each specified path of each supported function,
        #   which not follows the general naming rule

        self.index = index
        self.name = name
        self.filepath = sysfile_path
        self.support_mask = support_mask
        self.is_bmc = is_bmc

        self.temperature_file = None
        self.high_thershold_file = None
        self.low_threshold_file = None
        self.high_critical_file = None
        self.low_critical_file = None

        if sysfile_path is None:
            return

        if self.is_bmc == False or support_mask & 0x80 == 0x80:
            if support_mask & 0x1:
                self.temperature_file = \
                    sysfile_path + "/temp{}_input".format(self.index)
            if support_mask & 0x2:
                self.high_thershold_file = \
                    sysfile_path + "/temp{}_max".format(self.index)
            if support_mask & 0x4:
                self.low_threshold_file = \
                    sysfile_path + "/temp{}_min".format(self.index)
            if support_mask & 0x8:
                self.high_critical_file = \
                    sysfile_path + "/temp{}_crit".format(self.index)
            if support_mask & 0x10:
                self.low_critical_file = \
                    sysfile_path + "/temp{}_lcrit".format(self.index)
        elif self.is_bmc and ext_sysfile_list is not None:
            if support_mask & 0x1:
                self.temperature_file = \
                    sysfile_path + ext_sysfile_list[self.index][0]
            if support_mask & 0x2:
                self.high_thershold_file = \
                    sysfile_path + ext_sysfile_list[self.index][1]
            if support_mask & 0x4:
                self.low_threshold_file = \
                    sysfile_path + ext_sysfile_list[self.index][2]
            if support_mask & 0x8:
                self.high_critical_file = \
                    sysfile_path + ext_sysfile_list[self.index][3]
            if support_mask & 0x10:
                self.low_critical_file = \
                    sysfile_path + ext_sysfile_list[self.index][4]

    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath, 'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except Exception as ex:
            logger.log_error(
                "Unable to open {} due to {}".format(filepath, repr(ex)))

        return None

    def get_name(self):
        return self.name

    def get_presence(self):
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        if self.temperature_file is not None:
            temp = self.__read_attr_file(self.temperature_file)
            if temp is not None:
                return float(temp) / 1000

        return None

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_thershold_file is not None:
            temp = self.__read_attr_file(self.high_thershold_file)
            if temp is not None:
                return float(temp) / 1000

        return None

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.low_threshold_file is not None:
            temp = self.__read_attr_file(self.low_threshold_file)
            if temp is not None:
                return float(temp) / 1000

        return None

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius, 
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_critical_file is not None:
            temp = self.__read_attr_file(self.high_critical_file)
            if temp is not None:
                return float(temp) / 1000

        return None

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.low_critical_file is not None:
            temp = self.__read_attr_file(self.low_critical_file)
            if temp is not None:
                return float(temp) / 1000

        return None
