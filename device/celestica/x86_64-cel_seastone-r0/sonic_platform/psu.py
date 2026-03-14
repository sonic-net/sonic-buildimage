#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TLV_ATTR_TYPE_MODEL = 2
TLV_ATTR_TYPE_SERIAL = 5
PSU_EEPROM_PATH = "/sys/bus/i2c/devices/{}-00{}/eeprom"
GREEN_LED_PATH = "/sys/devices/platform/leds_dx010/leds/dx010:green:p-{}/brightness"
HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
GPIO_DIR = "/sys/class/gpio"
GPIO_LABEL = "pca9505"
PSU_NAME_LIST = ["PSU-1", "PSU-2"]
PSU_NUM_FAN = [1, 1]
PSU_I2C_MAPPING = {
    0: {
        "num": 10,
        "addr": "5a",
        "eeprom_addr": "52"
    },
    1: {
        "num": 11,
        "addr": "5b",
        "eeprom_addr": "53"
    },
}


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self._api_helper = APIHelper()
        self.green_led_path = GREEN_LED_PATH.format(self.index + 1)
        self.dx010_psu_gpio = [
            {'base': self.__get_gpio_base()},
            {'prs': 27, 'status': 22},
            {'prs': 28, 'status': 25}
        ]
        self.i2c_num = PSU_I2C_MAPPING[self.index]["num"]
        self.i2c_addr = PSU_I2C_MAPPING[self.index]["addr"]
        self.hwmon_path = HWMON_PATH.format(self.i2c_num, self.i2c_addr)
        self.eeprom_addr = PSU_EEPROM_PATH.format(self.i2c_num, PSU_I2C_MAPPING[self.index]["eeprom_addr"])
        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            self._fan_list.append(fan)

    def __search_file_by_contain(self, directory, search_str, file_start):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name.startswith(file_start) and search_str in self._api_helper.read_txt_file(file_path):
                    return file_path
        return None

    def __get_gpio_base(self):
        for r in os.listdir(GPIO_DIR):
            label_path = os.path.join(GPIO_DIR, r, "device/name")
            if "gpiochip" in r and GPIO_LABEL in self._api_helper.read_txt_file(label_path):
                return int(r[8:], 10)
        return 216  # Reserve

    def __get_gpio_value(self, pinnum):
        gpio_base = self.dx010_psu_gpio[0]['base']
        gpio_dir = GPIO_DIR + '/gpio' + str(gpio_base + pinnum)
        gpio_file = gpio_dir + "/value"
        retval = self._api_helper.read_txt_file(gpio_file)
        return retval.rstrip('\r\n')

    def read_fru(self, path, attr_type):
        content = []
        attr_idx = 0
        attr_length = 0

        if(os.path.exists(path)):
            with open(path, 'r', encoding='unicode_escape') as f:
                content = f.read()
            target_offset = ord(content[4])
            target_offset *= 8  # spec defined: offset are in multiples of 8 bytes

            attr_idx = target_offset + 3
            for i in range(1, attr_type):
                if attr_idx > len(content):
                    raise SyntaxError
                attr_length = (ord(content[attr_idx])) & (0x3f)
                attr_idx += (attr_length + 1)

            attr_length = (ord(content[attr_idx])) & (0x3f)
            attr_idx += 1
        else:
            print("[PSU] Can't find path to eeprom : %s" % path)
            return SyntaxError

        return content[attr_idx:attr_idx + attr_length]

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

        vout_label_path = self.__search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            vout_val = self._api_helper.read_txt_file(vout_path)
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

        curr_label_path = self.__search_file_by_contain(
            self.hwmon_path, current_label, "cur")
        if curr_label_path:
            dir_name = os.path.dirname(curr_label_path)
            basename = os.path.basename(curr_label_path)
            cur_num = ''.join(list(filter(str.isdigit, basename)))
            cur_path = os.path.join(
                dir_name, current_name.format(cur_num))
            cur_val = self._api_helper.read_txt_file(cur_path)
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

        pw_label_path = self.__search_file_by_contain(
            self.hwmon_path, current_label, "power")
        if pw_label_path:
            dir_name = os.path.dirname(pw_label_path)
            basename = os.path.basename(pw_label_path)
            pw_num = ''.join(list(filter(str.isdigit, basename)))
            pw_path = os.path.join(
                dir_name, current_name.format(pw_num))
            pw_val = self._api_helper.read_txt_file(pw_path)
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

        set_status_str = {
            self.STATUS_LED_COLOR_GREEN: '255',
            self.STATUS_LED_COLOR_OFF: '0'
        }.get(color, None)

        if not set_status_str:
            return False

        try:
            with open(self.green_led_path, 'w') as file:
                file.write(set_status_str)
        except IOError:
            return False

        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status = self._api_helper.read_txt_file(self.green_led_path)
        status_str = {
            '255': self.STATUS_LED_COLOR_GREEN,
            '0': self.STATUS_LED_COLOR_OFF
        }.get(status, None)

        return status_str

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

        vout_label_path = self.__search_file_by_contain(
            self.hwmon_path, temperature_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            temp_path = os.path.join(
                dir_name, temperature_name.format(in_num))
            vout_val = self._api_helper.read_txt_file(temp_path)
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

        vout_label_path = self.__search_file_by_contain(
            self.hwmon_path, temperature_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            temp_path = os.path.join(
                dir_name, temperature_name.format(in_num))
            vout_val = self._api_helper.read_txt_file(temp_path)
            psu_temperature = float(vout_val) / 1000

        return psu_temperature

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        voltage_name = "in{}_crit"
        voltage_label = "vout1"

        vout_label_path = self.__search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            vout_val = self._api_helper.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        voltage_name = "in{}_lcrit"
        voltage_label = "vout1"

        vout_label_path = self.__search_file_by_contain(
            self.hwmon_path, voltage_label, "in")
        if vout_label_path:
            dir_name = os.path.dirname(vout_label_path)
            basename = os.path.basename(vout_label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            vout_path = os.path.join(
                dir_name, voltage_name.format(in_num))
            vout_val = self._api_helper.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        psu_power = 0.0
        current_name = "power{}_max"
        current_label = "pout1"

        pw_label_path = self.__search_file_by_contain(
            self.hwmon_path, current_label, "power")
        if pw_label_path:
            dir_name = os.path.dirname(pw_label_path)
            basename = os.path.basename(pw_label_path)
            pw_num = ''.join(list(filter(str.isdigit, basename)))
            pw_path = os.path.join(
                dir_name, current_name.format(pw_num))
            pw_val = self._api_helper.read_txt_file(pw_path)
            psu_power = float(pw_val) / 1000000

        return psu_power

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
        raw = self.__get_gpio_value(self.dx010_psu_gpio[self.index + 1]['prs'])
        return int(raw, 10) == 0

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        raw = self.__get_gpio_value(
            self.dx010_psu_gpio[self.index + 1]['status'])
        return int(raw, 10) == 1

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = self.read_fru(self.eeprom_addr, TLV_ATTR_TYPE_MODEL)
        if not model:
            return "N/A"
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = self.read_fru(self.eeprom_addr, TLV_ATTR_TYPE_SERIAL)
        if not serial:
            return "N/A"
        return serial

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
