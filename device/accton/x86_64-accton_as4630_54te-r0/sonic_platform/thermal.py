#############################################################################
# Edgecore
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import glob

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from .helper import DeviceThreshold
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"
PSU_I2C_MAPPING = {
    0: {
        "num": 10,
        "addr": "58"
    },
    1: {
        "num": 11,
        "addr": "59"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 10,
        "addr": "50"
    },
    1: {
        "num": 11,
        "addr": "51"
    },
}

NOT_AVAILABLE = DeviceThreshold.NOT_AVAILABLE
HIGH_THRESHOLD = DeviceThreshold.HIGH_THRESHOLD
LOW_THRESHOLD = DeviceThreshold.LOW_THRESHOLD
HIGH_CRIT_THRESHOLD = DeviceThreshold.HIGH_CRIT_THRESHOLD
LOW_CRIT_THRESHOLD = DeviceThreshold.LOW_CRIT_THRESHOLD

DEFAULT_THRESHOLD = {
    'MB_temp(0x48)' : {
        HIGH_THRESHOLD : '80.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CB_temp(0x4b)' : {
        HIGH_THRESHOLD : '80.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'FB_temp(0x4a)' : {
        HIGH_THRESHOLD : '80.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Package_temp' : {
        HIGH_THRESHOLD : '71.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '91.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_0_temp' : {
        HIGH_THRESHOLD : '71.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '91.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_1_temp' : {
        HIGH_THRESHOLD : '71.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '91.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_2_temp' : {
        HIGH_THRESHOLD : '71.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '91.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'CPU_Core_3_temp' : {
        HIGH_THRESHOLD : '71.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : '91.0',
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-1 temp sensor 1' : {
        HIGH_THRESHOLD : '80.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    },
    'PSU-2 temp sensor 1' : {
        HIGH_THRESHOLD : '80.0',
        LOW_THRESHOLD : NOT_AVAILABLE,
        HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
        LOW_CRIT_THRESHOLD : NOT_AVAILABLE
    }
}

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    THERMAL_NAME_LIST = []
    PSU_THERMAL_NAME_LIST = []
    SYSFS_PATH = "/sys/bus/i2c/devices"
    CPU_SYSFS_PATH = "/sys/devices/platform"

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0):
        self.index = thermal_index
        self.is_psu = is_psu
        self.psu_index = psu_index
        self.min_temperature = None
        self.max_temperature = None

        if self.is_psu:
            psu_i2c_bus = PSU_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_I2C_MAPPING[psu_index]["addr"]
            self.psu_hwmon_path = PSU_I2C_PATH.format(psu_i2c_bus,
                                                      psu_i2c_addr)
            psu_i2c_bus = PSU_CPLD_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_CPLD_I2C_MAPPING[psu_index]["addr"]
            self.cpld_path = PSU_I2C_PATH.format(psu_i2c_bus, psu_i2c_addr)
        # Add thermal name
        self.THERMAL_NAME_LIST.append("MB_temp(0x48)")
        self.THERMAL_NAME_LIST.append("CB_temp(0x4b)")
        self.THERMAL_NAME_LIST.append("FB_temp(0x4a)")
        self.THERMAL_NAME_LIST.append("CPU_Package_temp")
        self.THERMAL_NAME_LIST.append("CPU_Core_0_temp")
        self.THERMAL_NAME_LIST.append("CPU_Core_1_temp")
        self.THERMAL_NAME_LIST.append("CPU_Core_2_temp")
        self.THERMAL_NAME_LIST.append("CPU_Core_3_temp")
        self.PSU_THERMAL_NAME_LIST.append("PSU-1 temp sensor 1")
        self.PSU_THERMAL_NAME_LIST.append("PSU-2 temp sensor 1")

        # Threshold Configuration
        self.__conf = DeviceThreshold(self.get_name())
        # Default threshold.
        self.__default_threshold = DEFAULT_THRESHOLD[self.get_name()]

        # Set hwmon path
        i2c_path = {
            0: {"hwmon_path":"14-0048/hwmon/hwmon*/", "ss_index":1},
            1: {"hwmon_path":"24-004b/hwmon/hwmon*/", "ss_index":1},
            2: {"hwmon_path":"25-004a/hwmon/hwmon*/", "ss_index":1},
            3: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":1},
            4: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":4},
            5: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":8},
            6: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":10},
            7: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":14}
        }.get(self.index, None)

        self.is_cpu = False
        if self.index in range(3,8):
            self.is_cpu = True
            self.hwmon_path = "{}/{}".format(self.CPU_SYSFS_PATH, i2c_path["hwmon_path"])
        else:
            self.hwmon_path = "{}/{}".format(self.SYSFS_PATH, i2c_path["hwmon_path"])
        self.ss_key = self.THERMAL_NAME_LIST[self.index]
        self.ss_index = i2c_path["ss_index"]

    def __read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'r') as fd:
                    data =fd.readline().strip()
                    if len(data) > 0:
                        return data
            except IOError as e:
                pass

        return None

    def __get_temp(self, temp_file):
        if not self.is_psu:
            temp_file_path = os.path.join(self.hwmon_path, temp_file)
        else:
            temp_file_path = temp_file
        raw_temp = self.__read_txt_file(temp_file_path)
        if raw_temp is not None:
            return float(raw_temp)/1000
        else:
            return 0

    def __set_threshold(self, file_name, temperature):
        if self.is_psu:
            return True
        temp_file_path = os.path.join(self.hwmon_path, file_name)
        for filename in glob.glob(temp_file_path):
            try:
                with open(filename, 'w') as fd:
                    fd.write(str(temperature))
                return True
            except IOError as e:
                print("IOError")


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
            temp_file = self.psu_hwmon_path + "psu_temp1_input"

        current = self.__get_temp(temp_file)

        if self.min_temperature is None or \
            current < self.min_temperature:
            self.min_temperature = current

        if self.max_temperature is None or \
           current > self.max_temperature:
            self.max_temperature = current

        return current

    def set_high_threshold(self, temperature):
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

    def get_high_threshold(self):
        value = self.__conf.get_high_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[HIGH_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def set_low_threshold(self, temperature):
        try:
            value = float(temperature)
        except Exception:
            return False

        # The new value can not be less than the default value.
        default_value = self.__default_threshold[LOW_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            if value < float(default_value):
                return False

        try:
            self.__conf.set_low_threshold(str(value))
        except Exception:
            return False

        return True

    def get_low_threshold(self):
        value = self.__conf.get_low_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[LOW_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def set_high_critical_threshold(self, temperature):
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

    def get_high_critical_threshold(self):
        value = self.__conf.get_high_critical_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[HIGH_CRIT_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def set_low_critical_threshold(self, temperature):
        try:
            value = float(temperature)
        except Exception:
            return False

        # The new value can not be less than the default value.
        default_value = self.__default_threshold[LOW_CRIT_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            if value < float(default_value):
                return False

        try:
            self.__conf.set_low_critical_threshold(str(value))
        except Exception:
            return False

        return True

    def get_low_critical_threshold(self):
        value = self.__conf.get_low_critical_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.__default_threshold[LOW_CRIT_THRESHOLD]
        if default_value != NOT_AVAILABLE:
            return float(default_value)

        raise NotImplementedError

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        if self.is_psu:
            return self.PSU_THERMAL_NAME_LIST[self.psu_index]
        else:
            return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the Thermal
        Returns:
            bool: True if Thermal is present, False if not
        """
        if self.is_cpu:
            return True

        if self.is_psu:
            val = self.__read_txt_file(self.cpld_path + "psu_present")
            if val is not None:
                return int(val, 10) == 1
            else:
                return False
        temp_file = "temp{}_input".format(self.ss_index)
        temp_file_path = os.path.join(self.hwmon_path, temp_file)
        raw_txt = self.__read_txt_file(temp_file_path)
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
            temp_file = self.psu_hwmon_path + "psu_temp_fault"
            psu_temp_fault = self.__read_txt_file(temp_file)
            if psu_temp_fault is None:
                psu_temp_fault = '1'
            return self.get_presence() and (not int(psu_temp_fault))

        file_str = "temp{}_input".format(self.ss_index)
        file_path = os.path.join(self.hwmon_path, file_str)
        raw_txt = self.__read_txt_file(file_path)
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
        if self.max_temperature is None:
            self.get_temperature()

        return self.max_temperature
