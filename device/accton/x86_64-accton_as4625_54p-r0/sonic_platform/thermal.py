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
    from .helper import APIHelper
    from .helper import DeviceThreshold
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

THERMAL_COUNT_PER_PSU = 2
THERMAL_I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/hwmon/hwmon*/"
CPU_SYSFS_PATH = "/sys/devices/platform/coretemp.0/hwmon/hwmon*/"

I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"
PSU_PMBUS_I2C_MAPPING = {
    0: {
        "num": 8,
        "addr": "58"
    },
    1: {
        "num": 9,
        "addr": "59"
    }
}

PSU_EEPROM_I2C_MAPPING = {
    0: {
        "num": 8,
        "addr": "50"
    },
    1: {
        "num": 9,
        "addr": "51"
    }
}

THERMAL_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "4a"
    },
    1: {
        "num": 3,
        "addr": "4b"
    },
    2: {
        "num": 3,
        "addr": "4d"
    },
    3: {
        "num": 3,
        "addr": "4e"
    },
    4: {
        "num": 3,
        "addr": "4f"
    }
}

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""
    THERMAL_NAME_LIST = [
        "MB_Temp(0x4a)",
        "MB_Temp(0x4b)",
        "MB_Temp(0x4d)",
        "MB_Temp(0x4e)",
        "MB_Temp(0x4f)",
        "CPU Package Temp",
        "CPU Core 0 Temp",
        "CPU Core 1 Temp",
        "CPU Core 2 Temp",
        "CPU Core 3 Temp"
    ]

    PSU_THERMAL_NAME_LIST = [
        "PSU-1 temp sensor 1",
        "PSU-1 temp sensor 2",
        "PSU-2 temp sensor 1",
        "PSU-2 temp sensor 2"
    ]

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0):
        self.index = thermal_index
        self.is_psu = is_psu
        self.psu_index = psu_index
        self.is_cpu = False
        self.min_temperature = None
        self.max_temperature = None
        self.pcb_id = self.__get_pcb_id()

        self.conf = DeviceThreshold(self.get_name())
        # Default thresholds
        self.default_threshold = {
            0: { # PCB_ID 0: AS4625-54P
                self.THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '67.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '72.0' },
                self.THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '67.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '72.0' },
                self.THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '73.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '78.0' },
                self.THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '80.0' },
                self.THERMAL_NAME_LIST[4] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '69.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '74.0' },
                self.THERMAL_NAME_LIST[5] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[6] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[7] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[8] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[9] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.PSU_THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '70.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '90.0' },
                self.PSU_THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '70.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '101.0' },
                self.PSU_THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '70.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '90.0' },
                self.PSU_THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '70.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '101.0' }
            },
            1: { # PCB_ID 1: AS4625-54T-F2B
                self.THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '70.0' },
                self.THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '70.0' },
                self.THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '62.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '67.0' },
                self.THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '70.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '75.0' },
                self.THERMAL_NAME_LIST[4] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '70.0' },
                self.THERMAL_NAME_LIST[5] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[6] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[7] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[8] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[9] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.PSU_THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '83.0' },
                self.PSU_THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '77.0' },
                self.PSU_THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '83.0' },
                self.PSU_THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '77.0' }
            },
            2: { # PCB_ID 2: AS4625-54T-B2F
                self.THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '70.0' },
                self.THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '67.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '72.0' },
                self.THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '66.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '71.0' },
                self.THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '73.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '78.0' },
                self.THERMAL_NAME_LIST[4] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '66.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '71.0' },
                self.THERMAL_NAME_LIST[5] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[6] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[7] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[8] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.THERMAL_NAME_LIST[9] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '75.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '95.0' },
                self.PSU_THERMAL_NAME_LIST[0] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '83.0' },
                self.PSU_THERMAL_NAME_LIST[1] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '77.0' },
                self.PSU_THERMAL_NAME_LIST[2] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '83.0' },
                self.PSU_THERMAL_NAME_LIST[3] : {
                    self.conf.HIGH_THRESHOLD_FIELD : '65.0',
                    self.conf.HIGH_CRIT_THRESHOLD_FIELD : '77.0' }
            },
        }

        if self.index in range(5,10):
            self.is_cpu = True

        if self.is_psu:
            self.i2c_num = PSU_PMBUS_I2C_MAPPING[self.psu_index]["num"]
            self.i2c_addr = PSU_PMBUS_I2C_MAPPING[self.psu_index]["addr"]
            self.pmbus_path = I2C_PATH.format(self.i2c_num, self.i2c_addr)

            self.i2c_num = PSU_EEPROM_I2C_MAPPING[self.psu_index]["num"]
            self.i2c_addr = PSU_EEPROM_I2C_MAPPING[self.psu_index]["addr"]
            self.eeprom_path = I2C_PATH.format(self.i2c_num, self.i2c_addr)
        elif self.is_cpu:
            self.ss_index = 0
            coretemp_list = self.__get_coretemp_list()
            if coretemp_list is not None and len(coretemp_list) >= 5:
                self.ss_index = coretemp_list[self.index-5]
            self.hwmon_path = CPU_SYSFS_PATH
        else:
            # Set sysfs path
            self.i2c_num = THERMAL_I2C_MAPPING[self.index]["num"]
            self.i2c_addr = THERMAL_I2C_MAPPING[self.index]["addr"]
            self.hwmon_path = THERMAL_I2C_PATH.format(self.i2c_num, self.i2c_addr)

    def __get_coretemp_list(self):
        coretemp_path = "{}{}".format(CPU_SYSFS_PATH, "temp*_input")
        coretemp_files = glob.glob(coretemp_path)

        for (idx, file) in enumerate(coretemp_files):
            file = file[file.rfind("/")+1:] # Discard its directory, keep file name
            file = file.strip("temp").strip("_input") # Discard temp_input, keep the index string
            coretemp_files[idx] = int(file)

        coretemp_files.sort()
        return coretemp_files

    def __read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'r') as fd:
                    data = fd.readline().rstrip()
                    if len(data) > 0:
                        return data
            except IOError as e:
                pass

        return None

    def __get_temp(self, path, temp_file):
        file_path = os.path.join(path, temp_file)
        raw_temp = self.__read_txt_file(file_path)
        if raw_temp is not None:
            return float(raw_temp) / 1000
        else:
            return None

    def __set_threshold(self, path, file_name, temperature):
        if self.is_psu:
            return True

        file_path = os.path.join(path, file_name)
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'w') as fd:
                    fd.write(str(temperature))
                return True
            except IOError as e:
                print("IOError")

    def __get_pcb_id(self):
        cpld_path = I2C_PATH.format('0', '64') + 'pcb_id'
        pcb_id = self.__read_txt_file(cpld_path)
        if pcb_id is not None:
            return int(pcb_id)

        return None

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.is_cpu:
            ss_file = "temp{}_input".format(self.ss_index)
            current = self.__get_temp(self.hwmon_path, ss_file)
        elif self.is_psu:
            if not self.get_presence():
                return None

            power_path = "{}{}".format(self.eeprom_path, 'psu_power_good')
            val = self.__read_txt_file(power_path)
            if val is not None:
                if int(val, 10) != 1:
                    return None

            # The base index start from 2 since temp1_input is not used by the PSU
            ss_file = "temp{}_input".format(self.index + 2)
            current = self.__get_temp(self.pmbus_path, ss_file)
        else:
            current = self.__get_temp(self.hwmon_path, "temp1_input")

        if current is None:
            return current

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
        if value != self.conf.NOT_AVAILABLE:
            return float(value)

        default_value = self.default_threshold[self.pcb_id][self.get_name()][self.conf.HIGH_CRIT_THRESHOLD_FIELD]
        if default_value != self.conf.NOT_AVAILABLE:
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
        if value != self.conf.NOT_AVAILABLE:
            return float(value)

        default_value = self.default_threshold[self.pcb_id][self.get_name()][self.conf.HIGH_THRESHOLD_FIELD]
        if default_value != self.conf.NOT_AVAILABLE:
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
            return self.PSU_THERMAL_NAME_LIST[self.index + self.psu_index * THERMAL_COUNT_PER_PSU]
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
        elif self.is_psu:
            path = "{}{}".format(self.eeprom_path, "psu_present")
            val = self.__read_txt_file(path)
            return int(val, 10) == 1
        else:
            path = "{}{}".format(self.hwmon_path, "temp1_input")
            val = self.__read_txt_file(path)
            if val is not None:
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
        elif self.is_psu:
            if not self.get_presence():
                return None

            path = "{}{}".format(self.pmbus_path, "psu_temp_fault")
            val = self.__read_txt_file(path)
            if val is None:
                return False
            else:
                return int(val, 10) == 0
        else:
            if self.get_temperature() is None:
                return False
            else:
                return True

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
