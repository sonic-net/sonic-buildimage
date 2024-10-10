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
    from .helper import DeviceThreshold, APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

THERMAL_COUNT_PER_PSU = 3

NOT_AVAILABLE = DeviceThreshold.NOT_AVAILABLE
HIGH_THRESHOLD = DeviceThreshold.HIGH_THRESHOLD
LOW_THRESHOLD = DeviceThreshold.LOW_THRESHOLD
HIGH_CRIT_THRESHOLD = DeviceThreshold.HIGH_CRIT_THRESHOLD
LOW_CRIT_THRESHOLD = DeviceThreshold.LOW_CRIT_THRESHOLD

# Default thresholds
DEFAULT_THRESHOLD = {
    'MB_FrontCenter_temp(0x48)' : {
        HIGH_THRESHOLD : '70.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '85.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'MB_FrontRight_temp(0x49)' : {
        HIGH_THRESHOLD : '70.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '85.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'MB_RearCenter_temp(0x4A)' : {
        HIGH_THRESHOLD : '70.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '85.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'MB_RearLeft_temp(0x4C)' : {
        HIGH_THRESHOLD : '70.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '85.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'MB_RearCenter_temp(0x4F)' : {
        HIGH_THRESHOLD : '70.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '85.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Package_temp' : {
        HIGH_THRESHOLD : '90.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '100.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_0_temp' : {
        HIGH_THRESHOLD : '90.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '100.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_1_temp' : {
        HIGH_THRESHOLD : '90.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '100.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_2_temp' : {
        HIGH_THRESHOLD : '90.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '100.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_3_temp' : {
        HIGH_THRESHOLD : '90.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '100.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-1 temp sensor 1' : {
        HIGH_THRESHOLD : '77.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '80.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-1 temp sensor 2' : {
        HIGH_THRESHOLD : '110.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '113.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-1 temp sensor 3' : {
        HIGH_THRESHOLD : '114.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '117.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-2 temp sensor 1' : {
        HIGH_THRESHOLD : '77.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '80.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-2 temp sensor 2' : {
        HIGH_THRESHOLD : '110.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '113.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-2 temp sensor 3' : {
        HIGH_THRESHOLD : '114.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '117.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    }
}

HWMON_PATH = {
    True: {
            0: {
                0: {"hwmon_path":"as9737_32db_psu.0/hwmon/hwmon*/psu1", "ss_index":1},
                1: {"hwmon_path":"as9737_32db_psu.0/hwmon/hwmon*/psu1", "ss_index":2},
                2: {"hwmon_path":"as9737_32db_psu.0/hwmon/hwmon*/psu1", "ss_index":3},
            },
            1: {
                0: {"hwmon_path":"as9737_32db_psu.1/hwmon/hwmon*/psu2", "ss_index":1},
                1: {"hwmon_path":"as9737_32db_psu.1/hwmon/hwmon*/psu2", "ss_index":2},
                2: {"hwmon_path":"as9737_32db_psu.1/hwmon/hwmon*/psu2", "ss_index":3},
            }
    },
    False: {
            0: {"hwmon_path":"as9737_32db_thermal/hwmon/hwmon*/", "ss_index":1},
            1: {"hwmon_path":"as9737_32db_thermal/hwmon/hwmon*/", "ss_index":2},
            2: {"hwmon_path":"as9737_32db_thermal/hwmon/hwmon*/", "ss_index":3},
            3: {"hwmon_path":"as9737_32db_thermal/hwmon/hwmon*/", "ss_index":4},
            4: {"hwmon_path":"as9737_32db_thermal/hwmon/hwmon*/", "ss_index":5},
            5: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":1},
            6: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":2},
            7: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":3},
            8: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":4},
            9: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":5}
    }
}

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    THERMAL_NAME_LIST = []
    PSU_THERMAL_NAME_LIST = []
    SYSFS_PATH_PREFIX = "/sys/devices/platform"

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0):
        self._api_helper = APIHelper()
        self.index = thermal_index
        self.is_psu = is_psu
        self.is_cpu = False
        self.psu_index = psu_index
        self.hwmon_path = None
        self.ss_index = 0
        self.min_temperature = None
        self.max_temperature = None

        # Add thermal name
        for thermal_name in DEFAULT_THRESHOLD.keys():
            if "PSU" in thermal_name:
                self.PSU_THERMAL_NAME_LIST.append(thermal_name)
            else:
                self.THERMAL_NAME_LIST.append(thermal_name)

        # Threshold Configuration
        self.__conf = DeviceThreshold(self.get_name())
        # Default threshold.
        self.__default_threshold = DEFAULT_THRESHOLD[self.get_name()]

        sysfs_path = None
        hwmon_path = HWMON_PATH.get(self.is_psu, None)
        if self.is_psu:
            # Set hwmon path
            psu_hwmon_path = hwmon_path.get(self.psu_index, None)
            if psu_hwmon_path is not None:
                sysfs_path = psu_hwmon_path.get(self.index, None)
        else :
            # Set hwmon path
            sysfs_path = hwmon_path.get(self.index, None)
            if self.index in range(5,10):
                self.is_cpu = True

        if sysfs_path is not None:
            self.hwmon_path = "{}/{}".format(self.SYSFS_PATH_PREFIX, 
                                             sysfs_path["hwmon_path"])
            self.ss_index = sysfs_path["ss_index"]

    def __get_temp(self, temp_file):
        if not self.is_psu:
            temp_file_path = os.path.join(self.hwmon_path, temp_file)
        else:
            temp_file_path = temp_file

        raw_temp = self._api_helper.glob_read_txt_file(temp_file_path)
        if raw_temp is not None:
            return float(raw_temp) / 1000
        else:
            return 0.0

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
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return 0.0

            temp_file = self.hwmon_path + "_temp{}_input".format(self.ss_index)

        current = self.__get_temp(temp_file)

        if self.min_temperature is None or \
            current < self.min_temperature:
            self.min_temperature = current

        if self.max_temperature is None or \
           current > self.max_temperature:
            self.max_temperature = current

        return current

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        value = self.__conf.get_high_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[HIGH_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        try:
            value = float(temperature)
        except Exception:
            return False

        # The new value can not be more than the default value.
        default_value = self.__default_threshold[HIGH_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            if value > float(default_value):
                return False

        try:
            self.__conf.set_high_threshold(str(value))
        except Exception:
            return False

        return True

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal by 1-based index
        Actions should be taken immediately if the temperature becomes higher than the high critical
        threshold otherwise the device will be damaged.

        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        value = self.__conf.get_high_critical_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[HIGH_CRIT_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        try:
            value = float(temperature)
        except Exception:
            return False

        # The new value can not be more than the default value.
        default_value = self.__default_threshold[HIGH_CRIT_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            if value > float(default_value):
                return False

        try:
            self.__conf.set_high_critical_threshold(str(value))
        except Exception:
            return False

        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        if self.is_psu:
            return self.PSU_THERMAL_NAME_LIST[(self.psu_index * THERMAL_COUNT_PER_PSU) + self.index]
        else:
            return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        if self.is_cpu:
            return True

        if self.is_psu:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + "_present")
            if val is not None:
                return int(val, 10) == 1
            else:
                return False

        temp_file = "temp{}_input".format(self.ss_index)
        temp_file_path = os.path.join(self.hwmon_path, temp_file)
        raw_txt = self._api_helper.glob_read_txt_file(temp_file_path)
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
        if self.is_cpu:
            return True

        if self.is_psu:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return False

            temp_file = self.hwmon_path + "_temp_fault"
            psu_temp_fault = self._api_helper.glob_read_txt_file(temp_file)
            if psu_temp_fault is None:
                psu_temp_fault = '1'
            return self.get_presence() and (not int(psu_temp_fault))

        file_str = "temp{}_input".format(self.ss_index)
        file_path = os.path.join(self.hwmon_path, file_str)

        raw_txt = self._api_helper.glob_read_txt_file(file_path)
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
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index+1

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.is_psu:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return None

        if self.min_temperature is None:
            self.get_temperature()

        return self.min_temperature

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.is_psu:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return None

        if self.max_temperature is None:
            self.get_temperature()

        return self.max_temperature
