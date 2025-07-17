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

NOT_AVAILABLE = DeviceThreshold.NOT_AVAILABLE
HIGH_THRESHOLD = DeviceThreshold.HIGH_THRESHOLD
LOW_THRESHOLD = DeviceThreshold.LOW_THRESHOLD
HIGH_CRIT_THRESHOLD = DeviceThreshold.HIGH_CRIT_THRESHOLD
LOW_CRIT_THRESHOLD = DeviceThreshold.LOW_CRIT_THRESHOLD

PSU_I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"

PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 9,
        "addr": "58"
    },
    1: {
        "num": 9,
        "addr": "59"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 9,
        "addr": "50"
    },
    1: {
        "num": 9,
        "addr": "51"
    },
}

THERMAL_NAME_LIST = ["MB_FrontMiddle_temp(0x48)", "MB_RightCenter_temp(0x49)", "MB_LeftCenter_temp(0x4A)",
                     "CB_temp(0x4B)", "OCXO_temp(0x4C)", "MB_RearRight_temp(0x4F)",
                     "CPU_Package_temp", "CPU_Core_0_temp", "CPU_Core_1_temp",
                     "CPU_Core_2_temp", "CPU_Core_3_temp"]

PSU_THERMAL_NAME_LIST = ["PSU-1 temp sensor 1", "PSU-2 temp sensor 1"]

SYSFS_PATH = "/sys/bus/i2c/devices"
CPU_SYSFS_PATH = "/sys/devices/platform"

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0, fan_dir=1):
        self.index = thermal_index
        self.is_psu = is_psu
        self.psu_index = psu_index
        self.min_temperature = None
        self.max_temperature = None

        if self.is_psu:
            psu_i2c_bus = PSU_HWMON_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_HWMON_I2C_MAPPING[psu_index]["addr"]
            self.psu_hwmon_path = PSU_I2C_PATH.format(psu_i2c_bus,
                                                      psu_i2c_addr)
            psu_i2c_bus = PSU_CPLD_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_CPLD_I2C_MAPPING[psu_index]["addr"]
            self.cpld_path = PSU_I2C_PATH.format(psu_i2c_bus, psu_i2c_addr)

        self.conf = DeviceThreshold(self.get_name())
        # Default thresholds
        self.default_threshold = {
            THERMAL_NAME_LIST[0] : {
                HIGH_THRESHOLD : '80.0',
                LOW_THRESHOLD : NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD : NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD : NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[1] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[2] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[3] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[4] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[5] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[6] : {
                HIGH_THRESHOLD: '82.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: '104.0',
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[7] : {
                HIGH_THRESHOLD: '82.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: '104.0',
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[8] : {
                HIGH_THRESHOLD: '82.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: '104.0',
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[9] : {
                HIGH_THRESHOLD: '82.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: '104.0',
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            THERMAL_NAME_LIST[10] : {
                HIGH_THRESHOLD: '82.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: '104.0',
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            PSU_THERMAL_NAME_LIST[0] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            },
            PSU_THERMAL_NAME_LIST[1] : {
                HIGH_THRESHOLD: '80.0',
                LOW_THRESHOLD: NOT_AVAILABLE,
                HIGH_CRIT_THRESHOLD: NOT_AVAILABLE,
                LOW_CRIT_THRESHOLD: NOT_AVAILABLE
            }
        }

        # The thermal policy configuration table is referenced from the accton_as9726_32d_monitor.py file
        # and is used for different fan directions (airflow modes).
        #
        # The table is organized by fan_dir values (0 and 1), where:
        # - fan_dir = 0: Front-to-back (F2B) airflow direction
        # - fan_dir = 1: Back-to-front (B2F) airflow direction
        #
        # Each sensor is mapped to a tuple (HIGH_THRESHOLD, HIGH_CRIT_THRESHOLD) representing:
        # - HIGH_THRESHOLD: The temperature at which a red alarm warning is triggered
        # - HIGH_CRIT_THRESHOLD: The temperature at which a system shutdown is triggered
        thermal_policy_config_table = {
            0: {
                THERMAL_NAME_LIST[0]: ('72.0', '77.0'),
                THERMAL_NAME_LIST[1]: ('70.0', '75.0'),
                THERMAL_NAME_LIST[2]: ('69.0', '74.0'),
                THERMAL_NAME_LIST[3]: ('72.0', '77.0'),
                THERMAL_NAME_LIST[4]: ('67.0', '72.0'),
                THERMAL_NAME_LIST[5]: ('69.0', '74.0'),
                THERMAL_NAME_LIST[6]: ('78.0', '83.0'),
                THERMAL_NAME_LIST[7]: ('78.0', '83.0'),
                THERMAL_NAME_LIST[8]: ('78.0', '83.0'),
                THERMAL_NAME_LIST[9]: ('78.0', '83.0'),
                THERMAL_NAME_LIST[10]: ('78.0', '83.0')
            },
            1: {
                THERMAL_NAME_LIST[0]: ('62.9', '67.9'),
                THERMAL_NAME_LIST[1]: ('56.9', '61.9'),
                THERMAL_NAME_LIST[2]: ('53.9', '58.9'),
                THERMAL_NAME_LIST[3]: ('46.8', '51.8'),
                THERMAL_NAME_LIST[4]: ('58.9', '63.9'),
                THERMAL_NAME_LIST[5]: ('53.5', '58.5'),
                THERMAL_NAME_LIST[6]: ('57.0', '62.0'),
                THERMAL_NAME_LIST[7]: ('57.0', '62.0'),
                THERMAL_NAME_LIST[8]: ('57.0', '62.0'),
                THERMAL_NAME_LIST[9]: ('57.0', '62.0'),
                THERMAL_NAME_LIST[10]: ('57.0', '62.0')
            }
        }
        for key, (ht, hct) in thermal_policy_config_table[fan_dir].items():
            if key in self.default_threshold:
                self.default_threshold[key][HIGH_THRESHOLD] = ht
                self.default_threshold[key][HIGH_CRIT_THRESHOLD] = hct

        # Set hwmon path
        i2c_path = {
            0: {"hwmon_path":"15-0048/hwmon/hwmon*/", "ss_index":1},
            1: {"hwmon_path":"15-0049/hwmon/hwmon*/", "ss_index":1},
            2: {"hwmon_path":"15-004a/hwmon/hwmon*/", "ss_index":1},
            3: {"hwmon_path":"15-004b/hwmon/hwmon*/", "ss_index":1},
            4: {"hwmon_path":"15-004c/hwmon/hwmon*/", "ss_index":1},
            5: {"hwmon_path":"15-004f/hwmon/hwmon*/", "ss_index":1},
            6: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":1},
            7: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":2},
            8: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":3},
            9: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":4},
            10: {"hwmon_path":"coretemp.0/hwmon/hwmon*/", "ss_index":5}
        }.get(self.index, None)

        self.is_cpu = False
        if self.index in range(6,11):
            self.is_cpu = True
            self.hwmon_path = "{}/{}".format(CPU_SYSFS_PATH, i2c_path["hwmon_path"])
        else:
            self.hwmon_path = "{}/{}".format(SYSFS_PATH, i2c_path["hwmon_path"])
        self.ss_key = THERMAL_NAME_LIST[self.index]
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
                return False


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
        if self.min_temperature is None or current < self.min_temperature:
            self.min_temperature = current

        if self.max_temperature is None or current > self.max_temperature:
            self.max_temperature = current

        return current

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        value = self.conf.get_high_critical_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.default_threshold[self.get_name()][HIGH_CRIT_THRESHOLD]
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
        except:
            return False

        try:
            self.conf.set_high_critical_threshold(str(value))
        except:
            return False

        return True

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        value = self.conf.get_high_threshold()
        if value != NOT_AVAILABLE:
            return float(value)

        default_value = self.default_threshold[self.get_name()][HIGH_THRESHOLD]
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
        except:
            return False

        try:
            self.conf.set_high_threshold(str(value))
        except:
            return False

        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        if self.is_psu:
            return PSU_THERMAL_NAME_LIST[self.psu_index]
        else:
            return THERMAL_NAME_LIST[self.index]

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
        """ Retrieves the minimum recorded temperature of thermal
        Returns: A float number, the minimum recorded temperature of thermal in Celsius
        up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.min_temperature is None:
            self.get_temperature()

        return self.min_temperature

    def get_maximum_recorded(self):
        """ Retrieves the maximum recorded temperature of thermal
        Returns: A float number, the maximum recorded temperature of thermal in Celsius
        up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.max_temperature is None:
            self.get_temperature()

        return self.max_temperature
