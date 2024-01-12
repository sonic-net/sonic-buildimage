#!/usr/bin/env python

#############################################################################
# Celestica
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

PLATFORM_CPLD_PATH = '/sys/devices/platform/sys_cpld'
GETREG_FILE = 'getreg'

FAN_DIRECTION_REG1 = '0xA141'
FAN_DIRECTION_REG2 = '0xA145'
FAN_DIRECTION_REG3 = '0xA149'
FAN_DIRECTION_REG4 = '0xA14D'


NULL_VAL = "N/A"
I2C_ADAPTER_PATH = "/sys/class/i2c-adapter"
IPMI_SENSOR_TEMP_CMD = "ipmitool sensor | grep degrees"
FANSHOW_CMD = "show platform fan"
THERMAL_INFO = [
    {'name': 'CPU_TEMP', 'temp': 'na'},  # 0
    {'name': 'TEMP_BB_U3', 'temp': 'na'},  # 1
    {'name': 'TEMP_SW_U25', 'temp': 'na'},  # 2
    {'name': 'TEMP_SW_U26', 'temp': 'na'},  # 3
    {'name': 'TEMP_SW_U16', 'temp': 'na'},  # 4
    {'name': 'TEMP_SW_U52', 'temp': 'na'},  # 5
    {'name': 'TEMP_SW_CORE', 'temp': 'na'},  # 6
    {'name': 'PSU1_Temp1', 'temp': 'na'},  # 7
    {'name': 'PSU1_Temp2', 'temp': 'na'},  # 8
    {'name': 'PSU1_Temp3', 'temp': 'na'},  # 9
    {'name': 'PSU2_Temp1', 'temp': 'na'},  # 10
    {'name': 'PSU2_Temp2', 'temp': 'na'},  # 11
    {'name': 'PSU2_Temp3', 'temp': 'na'},  # 12
    {'name': 'XP3R3V_LEFT_T', 'temp': 'na'},  # 13
    {'name': 'XP3R3V_RIGHT_T', 'temp': 'na'},  # 14
    {'name': 'U3P1_AVDD_T', 'temp': 'na'},  # 15
    {'name': 'XP0R8V_VDD_T', 'temp': 'na'},  # 16
    {'name': 'DIMMB0_TEMP', 'temp': 'na'},  # 17
]

thermal_temp_dict = {
    "CPU_TEMP": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL, "max": 103, "high_critical_threshold": 105},
    "TEMP_BB_U3": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 55, "F2B": NULL_VAL}, "high_critical_threshold": {"B2F": 60, "F2B": NULL_VAL}},
    "TEMP_SW_U25": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": NULL_VAL, "high_critical_threshold": NULL_VAL},
    "TEMP_SW_U26": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": NULL_VAL, "high_critical_threshold": NULL_VAL},
    "TEMP_SW_U16": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": {"B2F": NULL_VAL, "F2B": 55},
                    "high_critical_threshold": {"B2F": NULL_VAL, "F2B": 60}},
    "TEMP_SW_U52": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": NULL_VAL, "high_critical_threshold": NULL_VAL},
    "TEMP_SW_CORE": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                     "max": 105, "high_critical_threshold": 110},
    "PSU1_Temp1": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 61, "F2B": 61}, "high_critical_threshold": NULL_VAL},
    "PSU1_Temp2": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 107, "F2B": 108}, "high_critical_threshold": NULL_VAL},
    "PSU1_Temp3": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 106, "F2B": 91}, "high_critical_threshold": NULL_VAL},
    "PSU2_Temp1": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 61, "F2B": 61}, "high_critical_threshold": NULL_VAL},
    "PSU2_Temp2": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 107, "F2B": 108}, "high_critical_threshold": NULL_VAL},
    "PSU2_Temp3": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                   "max": {"B2F": 106, "F2B": 91}, "high_critical_threshold": NULL_VAL},
    "XP3R3V_LEFT_T": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                      "max": {"B2F": 125, "F2B": 125}, "high_critical_threshold": NULL_VAL},
    "XP3R3V_RIGHT_T": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                       "max": {"B2F": 125, "F2B": 125}, "high_critical_threshold": NULL_VAL},
    "U3P1_AVDD_T": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": {"B2F": 125, "F2B": 125}, "high_critical_threshold": NULL_VAL},
    "XP0R8V_VDD_T": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                     "max": {"B2F": 125, "F2B": 125}, "high_critical_threshold": NULL_VAL},
    "DIMMB0_TEMP": {"low_critical_threshold": NULL_VAL, "min": NULL_VAL,
                    "max": {"B2F": 85, "F2B": 85}, "high_critical_threshold": NULL_VAL},
}



temp_result = []
class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    SS_CONFIG_PATH = "/usr/share/sonic/device/x86_64-cel_midstone-100x-r0/sonic_platform/sensors.conf"

    def __init__(self, thermal_index, airflow):

        self.index = thermal_index
        self._api_helper = APIHelper()
        self._airflow = self.__get_fan_direction()
        self._thermal_info = THERMAL_INFO[self.index]
        self.name = self.get_name()
        
    def __get_fan_direction(self):
        """
        Complete other path information according to the corresponding BUS path
        """
        self.getreg_path = os.path.join(PLATFORM_CPLD_PATH, GETREG_FILE)
        b2f  =  f2b = 0
        for i in range(1,5):
            value = self._api_helper.get_cpld_reg_value(
                        self.getreg_path, eval('FAN_DIRECTION_REG'+str(i)))
            if int(value,16) & 0x8 :
                b2f = b2f + 1
            else:
                f2b = f2b + 1

        airflow = "B2F" if b2f > f2b else  "F2B"
        return airflow
       
    def __get_thermal_info(self):
        """
        Complete other path information according to the corresponding BUS path
        """
        global temp_result

        status, temp_result_str = self._api_helper.run_command(IPMI_SENSOR_TEMP_CMD)
        if type(temp_result_str) is bytes:
            temp_result_str = temp_result_str.decode('UTF-8')
        temp_result = temp_result_str.split('\n')

        for i in temp_result:
            if '|' not in i:
                continue
            sigle_list = i.split('|')
            if self._thermal_info["name"] == sigle_list[0].strip():
                self._thermal_info["temp"] = sigle_list[1].strip()
                break
        return True

    def __set_threshold(self, temperature):
        temp_file_path = self._thermal_info.get("max_temp", "N/A")
        try:
            with open(temp_file_path, 'w') as fd:
                fd.write(str(temperature))
            return True
        except IOError:
            return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        self.__get_thermal_info()
        temperature = self._thermal_info.get("temp", "na")
        if temperature != "na":
            temperature = float(temperature)
        else:
            temperature = 0
        return float("{:.3f}".format(temperature))

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_threshold = thermal_temp_dict.get(self.name).get("max")
        if isinstance(high_threshold, dict):
            high_threshold = high_threshold.get(self._airflow)
        if high_threshold != NULL_VAL:
            high_threshold = float("{:.3f}".format(high_threshold))
        return high_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        low_threshold = thermal_temp_dict.get(self.name).get("min")
        if low_threshold != NULL_VAL:
            low_threshold = float("{:.3f}".format(low_threshold))
        return low_threshold

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        #temp_file = "temp1_max"
        #is_set = self.__set_threshold(int(temperature) * 1000)
        #file_set = False
        #if is_set:
        #    try:
        #        with open(self.SS_CONFIG_PATH, 'r+') as f:
        #            content = f.readlines()
        #            f.seek(0)
        #            ss_found = False
        #            for idx, val in enumerate(content):
        #                if self.name in val:
        #                    ss_found = True
        #                elif ss_found and temp_file in val:
        #                    content[idx] = "    set {} {}\n".format(
        #                        temp_file, temperature)
        #                    f.writelines(content)
        #                    file_set = True
        #                    break
        #    except IOError:
        #        file_set = False

        #return is_set & file_set
        # Not Support
        return False

    @staticmethod
    def set_low_threshold(temperature):
        """
        Sets the low threshold temperature of thermal
        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # Not Support
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        high_critical_threshold = thermal_temp_dict.get(self.name).get("high_critical_threshold")
        if isinstance(high_critical_threshold, dict):
            high_critical_threshold = high_critical_threshold.get(str(self._airflow).upper())
        if high_critical_threshold != NULL_VAL:
            high_critical_threshold = float("{:.3f}".format(float(high_critical_threshold)))
        return high_critical_threshold

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        low_critical_threshold = thermal_temp_dict.get(self.name).get("low_critical_threshold")
        if low_critical_threshold != NULL_VAL:
            low_critical_threshold = float("{:.3f}".format(float(low_critical_threshold)))
        return low_critical_threshold

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self._thermal_info.get("name")

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return True if self.get_temperature() >= 0 else False


    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._thermal_info.get("model", NULL_VAL)

    @staticmethod
    def get_serial():
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return NULL_VAL

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False
        return True
