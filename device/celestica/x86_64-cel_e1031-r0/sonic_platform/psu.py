#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os.path

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from .common import Common
    from .eeprom import DeviceEEPROM
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_E1031_SPEED_PATH = "/sys/class/hwmon/hwmon{}/fan1_input"
PSU_E1031_STAT_PATH = "/sys/devices/platform/e1031.smc/"
HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
FAN_MAX_RPM = 11000
PSU_NAME_LIST = ["PSU-R", "PSU-L"]
PSU_NUM_FAN = [1, 1]
PSU_I2C_MAPPING = {
    0: {
        "num": 13,
        "addr": "5b",
        "eeprom_addr": "53"
    },
    1: {
        "num": 12,
        "addr": "5a",
        "eeprom_addr": "52"
    },
}
PSU_EEPROM_PATH = "/sys/bus/i2c/devices/{}-00{}/eeprom"
PSU_EEPROM_FORMAT = [
    ('Serial Number', 's', 16), ('burn', 'x', 16),
    ('Model', 's', 16),
    ('Part Number', 's', 16),
]
PSU_EEPROM_START_OFFSET = 48


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)

        self._api_common = Common()
        self.index = psu_index
        self.psu_presence = "psu{}_prs"
        self.psu_oper_status = "psu{}_status"
        self.i2c_num = PSU_I2C_MAPPING[self.index]["num"]
        self.i2c_addr = PSU_I2C_MAPPING[self.index]["addr"]
        self.hwmon_path = HWMON_PATH.format(self.i2c_num, self.i2c_addr)
        self.eeprom_addr = PSU_EEPROM_PATH.format(
            self.i2c_num, PSU_I2C_MAPPING[self.index]["eeprom_addr"])

        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            self._fan_list.append(fan)

    def _search_file_by_contain(self, directory, search_str, file_start):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name.startswith(file_start) and search_str in self._api_common.read_txt_file(file_path):
                    return file_path
        return None

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        voltage_name = "in{}_input"
        voltage_label = "vout1"

        vout_label_path = self._search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            vout_val = self._api_common.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = 0.0
        current_name = "curr{}_input"
        current_label = "iout1"

        curr_label_path = self._search_file_by_contain(
            self.hwmon_path, current_label, "cur")
        if curr_label_path:
            dir_name = os.path.dirname(curr_label_path)
            basename = os.path.basename(curr_label_path)
            cur_num = ''.join(list(filter(str.isdigit, basename)))
            cur_path = os.path.join(
                dir_name, current_name.format(cur_num))
            cur_val = self._api_common.read_txt_file(cur_path)
            psu_current = float(cur_val) / 1000

        return psu_current

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = 0.0
        current_name = "power{}_input"
        current_label = "pout1"

        pw_label_path = self._search_file_by_contain(
            self.hwmon_path, current_label, "power")
        if pw_label_path:
            dir_name = os.path.dirname(pw_label_path)
            basename = os.path.basename(pw_label_path)
            pw_num = ''.join(list(filter(str.isdigit, basename)))
            pw_path = os.path.join(
                dir_name, current_name.format(pw_num))
            pw_val = self._api_common.read_txt_file(pw_path)
            psu_power = float(pw_val) / 1000000

        return psu_power

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # Hardware not supported
        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # Hardware not supported
        return self.STATUS_LED_COLOR_OFF

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        psu_location = ["R", "L"]
        presences_status = self._api_common.read_txt_file(
            PSU_E1031_STAT_PATH + self.psu_presence.format(psu_location[self.index])) or 0

        return int(presences_status) == 1

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        psu_location = ["R", "L"]
        power_status = self._api_common.read_txt_file(
            PSU_E1031_STAT_PATH + self.psu_oper_status.format(psu_location[self.index])) or 0

        return int(power_status) == 1

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        self._eeprom = DeviceEEPROM(
            self.eeprom_addr, PSU_EEPROM_FORMAT, PSU_EEPROM_START_OFFSET)
        return self._eeprom.model_str

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        self._eeprom = DeviceEEPROM(
            self.eeprom_addr, PSU_EEPROM_FORMAT, PSU_EEPROM_START_OFFSET)
        return self._eeprom.serial_number

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index + 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
            there are three temp sensors , we choose one of them
        """
        psu_temperature = None
        temperature_name = "temp{}_input"
        temperature_label = "vout1"

        vout_label_path = self._search_file_by_contain(
            self.hwmon_path, temperature_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            temp_path = os.path.join(
                dir_name, temperature_name.format(in_num))
            vout_val = self._api_common.read_txt_file(temp_path)
            psu_temperature = float(vout_val) / 1000

        return psu_temperature

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
            there are three temp sensors , we choose one of them
        """
        psu_temperature = None
        temperature_name = "temp{}_max"
        temperature_label = "vout1"

        vout_label_path = self._search_file_by_contain(
            self.hwmon_path, temperature_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            temp_path = os.path.join(
                dir_name, temperature_name.format(in_num))
            vout_val = self._api_common.read_txt_file(temp_path)
            psu_temperature = float(vout_val) / 1000

        return psu_temperature

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 12.6
        voltage_name = "in{}_crit"
        voltage_label = "vout1"

        vout_label_path = self._search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            if os.path.exists(vout_path):
                vout_val = self._api_common.read_txt_file(vout_path)
                psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 11.4
        voltage_name = "in{}_lcrit"
        voltage_label = "vout1"

        vout_label_path = self._search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            if os.path.exists(vout_path):
                vout_val = self._api_common.read_txt_file(vout_path)
                psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return 200.0
