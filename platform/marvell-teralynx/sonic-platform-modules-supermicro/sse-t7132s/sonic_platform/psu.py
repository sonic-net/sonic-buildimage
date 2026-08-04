#############################################################################
# SuperMicro SSE-T7132S
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import re

try:
    from sonic_platform_base.psu_base import PsuBase
    from .helper import APIHelper
    from sonic_platform.fan import Fan
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NAME_LIST = ["PSU_1", "PSU_2"]
PSU_NUM_FAN = [1, 1]
PSU_NUM_TERMAL = [2, 2]

IPMI_SENSOR_NETFN = "0x04"
IPMI_OEM_NETFN = "0x30"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_OEM_CMD = "0x89 {}"
IPMI_PSU_TYPE_CMD = "0x1 {}"
IPMI_GET_PSU_LED_MODE_CMD = "0x2 0x1"
IPMI_SET_PSU_LED_MODE_CMD = "0x2 0x2 {}"
IPMI_GET_PSU_LED_PATTERN_CMD = "0x3 0x0 {}"
IPMI_SET_PSU_LED_PATTERN_CMD = "0x3 0x1 {} {}"
IPMI_PSU_INFO_CMD= "0x4 {} {}"

PSU_LED_OFF_CMD = "0x00"
PSU_LED_GREEN_CMD = "0x01"
PSU_LED_AMBER_CMD = "0x02"

PSU_SERIAL_CMD = "0x00"
PSU_MODEL_CMD = "0x01"
PSU_VOUT_CMD = "0x02"
PSU_COUT_CMD = "0x03"
PSU_POUT_CMD = "0x04"
PSU_VIN_CMD = "0x05"
PSU_CIN_CMD = "0x06"
PSU_PIN_CMD = "0x07"
PSU_FAN1_CMD = "0x08"
PSU_FAN2_CMD = "0x09"
PSU_TEMP1_CMD = "0x0A"
PSU_TEMP2_CMD = "0x0B"
PSU_MAX_TEMP1_CMD = "0x0C"
PSU_MAX_TEMP2_CMD = "0x0D"
PSU_MAX_POUT_CMD = "0x0E"

PSU1_STATUS_REG = "0xC4"
PSU2_STATUS_REG = "0xC5"

PSU1_FRU_ID = 3

PSU_OUT_VOLTAGE = 12

SS_READ_OFFSET = 0
OEM_READ_OFFSET = 0


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(0, fan_index, is_psu_fan=True, psu=self)
            self._fan_list.append(fan)
        for thermal_index in range(0, PSU_NUM_TERMAL[self.index]):
            self._thermal_list.append(PsuThermal(self, thermal_index))
        self._api_helper = APIHelper()

    def find_value(self, in_string):
        result = re.search("^.+ ([0-9a-f]{2}) .+$", in_string)
        return result.group(1) if result else result

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        psu_vout_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_VOUT_CMD)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_vout_key))

        if raw_oem_read:
            # Formula: R/10
            psu_voltage = int("".join(raw_oem_read.split()[::-1]), 16) / 10

        return psu_voltage

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        # Formula: PSU_OUT_VOLTAGEx11/10
        psu_voltage = PSU_OUT_VOLTAGE * 11 / 10

        return psu_voltage

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        # Formula: PSU_OUT_VOLTAGEx9/10
        psu_voltage = PSU_OUT_VOLTAGE * 9 / 10

        return psu_voltage

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = 0.0
        psu_cout_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_COUT_CMD)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_cout_key))

        if raw_oem_read:
            # Formula: R/1000
            psu_current = int("".join(raw_oem_read.split()[::-1]), 16) / 1000

        return psu_current

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = 0.0
        psu_pout_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_POUT_CMD)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_pout_key))

        if raw_oem_read:
            # Formula: R
            psu_power = int("".join(raw_oem_read.split()[::-1]), 16)
        return psu_power

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        psu_power = 0.0
        psu_pout_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_MAX_POUT_CMD)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_pout_key))

        if raw_oem_read:
            # Formula: R
            psu_power = int("".join(raw_oem_read.split()[::-1]), 16)
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
        Note
        """
        led_cmd = {
            self.STATUS_LED_COLOR_GREEN: PSU_LED_GREEN_CMD,
            "orange": PSU_LED_AMBER_CMD,
            self.STATUS_LED_COLOR_OFF: PSU_LED_OFF_CMD 
        }.get(color)

        psu_led_key = IPMI_SET_PSU_LED_PATTERN_CMD.format(self.index+2,led_cmd)
        status, set_led = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_led_key))
        set_status_led = False if not status else True

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        psu_led_key = IPMI_GET_PSU_LED_PATTERN_CMD.format(self.index+2)
        status, hx_color = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_led_key))

        status_led = {
            "00": self.STATUS_LED_COLOR_OFF,
            "01": self.STATUS_LED_COLOR_GREEN,
            "02": "orange",
        }.get(hx_color, self.STATUS_LED_COLOR_OFF)

        return status_led

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
            there are three temp sensors , we choose one of them
        """
        # Need implement after BMC function ready
        psu_temperature = None

        return psu_temperature

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
            there are three temp sensors , we choose one of them
        """
        # Need implement after BMC function ready
        psu_temperature = None

        return psu_temperature

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
        psu_presence = False
        psu_pstatus_key = globals()['PSU{}_STATUS_REG'.format(self.index+1)]
        status, raw_status_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pstatus_key))
        status_byte = self.find_value(raw_status_read)

        if status:
            presence_int = (int(status_byte, 16) >> 0) & 1
            psu_presence = True if presence_int else False

        return psu_presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = "Unknown"
        psu_model_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_MODEL_CMD)
        status, raw_model = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_model_key))

        model_raw_list = raw_model.split()
        if len(model_raw_list) > 0:
            model="".join(map(chr,map(lambda x: int(x, 16), model_raw_list)))
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = "Unknown"
        psu_serial_key = IPMI_PSU_INFO_CMD.format(self.index+1, PSU_SERIAL_CMD)
        status, raw_serial = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_serial_key))

        serial_raw_list = raw_serial.split()
        if len(serial_raw_list) > 0:
            serial="".join(map(chr,map(lambda x: int(x, 16), serial_raw_list)))

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        psu_status = False
        psu_pstatus_key = globals()['PSU{}_STATUS_REG'.format(self.index+1)]
        status, raw_status_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pstatus_key))
        status_byte = self.find_value(raw_status_read)

        if status:
            failure_detected = (int(status_byte, 16) >> 1) & 1
            input_lost = (int(status_byte, 16) >> 3) & 1
            psu_status = False if (input_lost or failure_detected) else True

        return psu_status

    def get_type(self):
        """
        Retrives the Power Type of PSU

        Returns :
            A string, PSU power type
        """
        psu_type = [None, 'AC', 'AC', 'DC']
        psu_type_key = IPMI_PSU_TYPE_CMD.format(self.index+1)
        status, raw_type_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_type_key))
        if status:
            raw_type = raw_type_read.split()[OEM_READ_OFFSET]
            type_index = int(raw_type, 16)
            if type_index < 4:
                return psu_type[type_index]
        return None

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this PSU is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True


    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this PSU

        Returns:
            An integer, the number of thermals available on this PSU
        """
        return len(self._thermal_list)

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this PSU

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this PSU
        """
        return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object derived from ThermalBase representing the specified thermal
        """
        thermal = None

        try:
            thermal = self._thermal_list[index]
        except IndexError:
            sys.stderr.write("THERMAL index {} out of range (0-{})\n".format(
                             index, len(self._thermal_list)-1))

        return thermal

class PsuThermal(ThermalBase):
    """Platform-specific Thermal class for PSU """

    def __init__(self, psu, index):
        self._api_helper = APIHelper()
        self.psu = psu
        self.index = index
        self.minimum_thermal = 999
        self.maximum_thermal = 0

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if self.psu.get_presence() != True:
            return None

        psu_temperature = 0.0
        psu_temp_cmd_key = globals()['PSU_TEMP{}_CMD'.format(self.index+1)]
        psu_temp_key = IPMI_PSU_INFO_CMD.format(self.psu.index+1, psu_temp_cmd_key)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_temp_key))

        if raw_oem_read:
            # Formula: R
            psu_temperature = int("".join(raw_oem_read.split()[::-1]), 16)

        if psu_temperature > self.maximum_thermal:
            self.maximum_thermal = psu_temperature
        if psu_temperature < self.minimum_thermal:
            self.minimum_thermal = psu_temperature

        return psu_temperature

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # ignore max_temp 1 to aviod false alarm because PSU fan control is not ready
        if self.index == 0:
            return None

        if self.get_presence() != True:
            return None

        psu_temper_high = 0.0
        psu_temp_cmd_key = globals()['PSU_MAX_TEMP{}_CMD'.format(self.index+1)]
        psu_temp_key = IPMI_PSU_INFO_CMD.format(self.psu.index+1, psu_temp_cmd_key)
        status, raw_oem_read = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_OEM_CMD.format(psu_temp_key))

        if raw_oem_read:
            # Formula: R
            psu_temper_high = int("".join(raw_oem_read.split()[::-1]), 16)

        return psu_temper_high

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.minimum_thermal;

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.maximum_thermal

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return '{}_TEMP_{}'.format(self.psu.get_name(), self.index+1)

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return self.psu.get_presence()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self.psu.get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self.psu.get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.psu.get_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return (self.index + 1)

    def is_replaceable(self):
        """
        Indicate whether this Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

