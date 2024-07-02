#############################################################################
# Edgecore
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    SYSFS_PATH = "/sys/devices/platform/as7535_28xb_thermal/"
    SYSFS_PATH_PSU = "/sys/devices/platform/as7535_28xb_psu/"

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0):
        self._api_helper = APIHelper()
        self.index = thermal_index
        self.is_psu = is_psu
        self.psu_index = psu_index

        if self.is_psu:
            self.hwmon_path = self.SYSFS_PATH_PSU
        else:
            self.hwmon_path = self.SYSFS_PATH

        self.ss_index = self.index + 1

    def __get_temp(self, temp_file):
        temp_file_path = os.path.join(self.hwmon_path, temp_file)

        raw_temp = self._api_helper.read_txt_file(temp_file_path)
        if raw_temp is not None:
            return float(raw_temp)/1000
        else:
            return 0

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_input".format(self.ss_index)
        else:
            if not self.get_presence():
                return None
            temp_file = "psu{}_temp{}_input".format(self.psu_index+1, self.ss_index)

        return self.__get_temp(temp_file)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_min".format(self.ss_index)
        else:
            if not self.get_presence():
                return None
            temp_file = "psu{}_temp{}_input_min".format(self.psu_index+1, self.ss_index)

        return self.__get_temp(temp_file)

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal by 1-based index
        Actions should be taken immediately if the temperature becomes lower than the low critical
        threshold otherwise the device will be damaged.

        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: A float number, the low critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_min_alarm".format(self.ss_index)
        else:
            if not self.get_presence():
                return None
            temp_file = "psu{}_temp{}_input_min".format(self.psu_index+1, self.ss_index)

        return self.__get_temp(temp_file)

    def set_low_critical_threshold(self, temperature):
        """
        Sets the critical low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_max".format(self.ss_index)
        else:
            if not self.get_presence():
                return None
            temp_file = "psu{}_temp{}_input_max".format(self.psu_index+1, self.ss_index)

        return self.__get_temp(temp_file)

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_max_alarm".format(self.ss_index)
        else:
            if not self.get_presence():
                return None
            temp_file = "psu{}_temp{}_input_max".format(self.psu_index+1, self.ss_index)

        return self.__get_temp(temp_file)

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        if self.is_psu:
            return "PSU-{} temp sensor {}".format(self.psu_index+1, self.ss_index)
        else:
            return "Temp sensor {}".format(self.ss_index)

    def get_presence(self):
        """
        Retrieves the presence of the Thermal
        Returns:
            bool: True if Thermal is present, False if not
        """
        if self.is_psu:
            temp_file = self.hwmon_path + "psu{}_present".format(self.psu_index+1)
            val = self._api_helper.read_txt_file(temp_file)
            return int(val, 10) == 1

        temp_file = "temp{}_input".format(self.ss_index)
        temp_file_path = os.path.join(self.hwmon_path, temp_file)
        raw_txt = self._api_helper.read_txt_file(temp_file_path)
        if raw_txt is not None:
            return True
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu:
            if not self.get_presence():
                return False
            return (self.get_temperature() <= self.get_high_threshold())

        file_str = "temp{}_input".format(self.ss_index)
        file_path = os.path.join(self.hwmon_path, file_str)
        raw_txt = self._api_helper.read_txt_file(file_path)
        if raw_txt is None:
            return False
        else:
            return int(raw_txt) != 0

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """

        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "N/A"

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
        return self.index+1

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False
