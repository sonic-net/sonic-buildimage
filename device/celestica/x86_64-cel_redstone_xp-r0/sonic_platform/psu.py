import os

try:
    from sonic_platform_base.psu_base import PsuBase
    from .fan import Fan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_MAPPING = {
    0: {
        "name": "PSU-1",
        "adapter": 11,
        "address": "59",
        "eeprom": "51",
        "presence": "28",
        "status": "25"
    },
    1: {
        "name": "PSU-2",
        "adapter": 10,
        "address": "58",
        "eeprom": "50",
        "presence": "27",
        "status": "22"
    },
}


class Psu(PsuBase):
    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self._api_helper = APIHelper()

        self.__psu_map = PSU_MAPPING[self.index]

        self.psu_name = self.__psu_map.get("name", "PSU-{}".format(self.index + 1))

        i2c_adapter = self.__psu_map.get('adapter', 0)
        i2c_address = self.__psu_map.get('address', 0)
        i2c_eeprom = self.__psu_map.get('eeprom', 0)

        self.led_path = "/sys/devices/platform/redstone_xp_cpld/psu{}_led".format(self.index + 1)
        self.hwmon_path = self._api_helper.get_hwmon_path(i2c_adapter, i2c_address)
        self.eeprom_path = "/sys/bus/i2c/devices/{}-00{}/eeprom".format(i2c_adapter, i2c_eeprom)

        self._fan_list.append(Fan(self.index + 4, 0))

    @property
    def presence(self):
        gpio = self.__psu_map.get('presence')
        return int(self._api_helper.get_gpio(gpio), 10) == 0

    @property
    def status(self):
        gpio = self.__psu_map.get('status')
        return int(self._api_helper.get_gpio(gpio), 10) == 1

    def __read_fru(self, path, attr_type):
        content = []
        attr_idx = 0
        attr_length = 0

        if (os.path.exists(path)):
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
        psu_voltage = 0.0
        voltage_metric = "in2_input"

        vout_path = os.path.normpath("{}/{}".format(self.hwmon_path, voltage_metric))
        if os.path.isfile(vout_path):
            vout_val = self._api_helper.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000

        return psu_voltage

    def get_current(self):
        psu_current = 0.0
        current_metric = "curr2_input"

        curr_path = os.path.normpath("{}/{}".format(self.hwmon_path, current_metric))
        if os.path.isfile(curr_path):
            cur_val = self._api_helper.read_txt_file(curr_path)
            psu_current = float(cur_val) / 1000

        return psu_current

    def get_power(self):
        psu_power = 0.0
        power_metric = "power2_input"

        pw_path = os.path.normpath("{}/{}".format(self.hwmon_path, power_metric))
        if os.path.isfile(pw_path):
            pw_val = self._api_helper.read_txt_file(pw_path)
            psu_power = float(pw_val) / 1000000

        return psu_power

    def get_powergood_status(self):
        return all((self.presence, self.status))

    def set_status_led(self, color):
        set_status = 1 if color == self.STATUS_LED_COLOR_GREEN else 0
        self._api_helper.write_txt_file(self.led_path, set_status)

        return int(self._api_helper.read_txt_file(self.led_path), 10) == set_status

    def get_status_led(self):
        status = self._api_helper.read_txt_file(self.led_path)
        status_str = {
            '1': self.STATUS_LED_COLOR_GREEN,
            '0': self.STATUS_LED_COLOR_OFF
        }.get(status, "Error")

        return status_str.title()

    def get_temperature(self):
        psu_temperature = None
        temperature_metric = "temp2_input"

        temp_path = os.path.normpath("{}/{}".format(self.hwmon_path, temperature_metric))
        if os.path.isfile(temp_path):
            vout_val = self._api_helper.read_txt_file(temp_path)
            psu_temperature = float(vout_val) / 1000

        return psu_temperature

    def get_temperature_high_threshold(self):
        psu_temperature = None
        temperature_metric = "temp2_max"

        temp_path = os.path.normpath("{}/{}".format(self.hwmon_path, temperature_metric))
        if os.path.isfile(temp_path):
            vout_val = self._api_helper.read_txt_file(temp_path)
            psu_temperature = float(vout_val) / 1000
        return psu_temperature

    def get_voltage_high_threshold(self):
        psu_voltage = 0.0
        voltage_metric = "in2_rated_max"

        vout_path = os.path.normpath("{}/{}".format(self.hwmon_path, voltage_metric))
        if os.path.isfile(vout_path):
            vout_val = self._api_helper.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000
        return psu_voltage

    def get_voltage_low_threshold(self):
        psu_voltage = 0.0
        voltage_metric = "in2_lcrit"

        vout_path = os.path.normpath("{}/{}".format(self.hwmon_path, voltage_metric))
        if os.path.isfile(vout_path):
            vout_val = self._api_helper.read_txt_file(vout_path)
            psu_voltage = float(vout_val) / 1000
        return psu_voltage

    def get_maximum_supplied_power(self):
        psu_power = 0.0
        pw_metric = "power2_max"

        pw_path = os.path.normpath("{}/{}".format(self.hwmon_path, pw_metric))
        if os.path.isfile(pw_path):
            pw_val = self._api_helper.read_txt_file(pw_path)
            psu_power = float(pw_val) / 1000000

        return psu_power

    def get_name(self):
        return self.psu_name

    def get_presence(self):
        return self.presence

    def get_status(self):
        return self.status

    def get_model(self):
        tlv_value = 2  # TLV_ATTR_TYPE_MODEL
        model = self.__read_fru(self.eeprom_path, tlv_value)
        if not model:
            return "N/A"
        return model

    def get_serial(self):
        tlv_value = 5  # TLV_ATTR_TYPE_SERIAL
        serial = self.__read_fru(self.eeprom_path, tlv_value)
        if not serial:
            return "N/A"
        return serial

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return True
