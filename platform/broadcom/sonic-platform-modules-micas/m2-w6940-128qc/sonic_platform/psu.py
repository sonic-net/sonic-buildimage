#!/usr/bin/env python3
########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################


try:
    import time
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e

PSU_NOT_PRESENCE = 0x01
PSU_POWER_LOSS = 0x02
PSU_FAN_FAULT = 0x04
PSU_VOL_FAULT = 0x08
PSU_CUR_FAULT = 0x10
PSU_PWR_FAULT = 0x20
PSU_TEMP_FAULT = 0x40

PSU_ERR_VALUE = -999999

class Psu(PsuBase):
    """Platform-specific PSU class"""

    def __init__(self, interface_obj, index):
        self._fan_list = []
        self._thermal_list = []
        self.restful = interface_obj
        self.index = index
        self.name = "psu" + str(index)

        self._fan_list.append(Fan(self.restful, 1, 1, psu_fan=True, psu_index=index))

        self.led_map = {
            "0": self.STATUS_LED_COLOR_OFF,
            "1": self.STATUS_LED_COLOR_GREEN,
            "2": self.STATUS_LED_COLOR_AMBER,
            "3": self.STATUS_LED_COLOR_RED
        }

    def _get_value(self, value):
        return round(float(value) / 1000, 1)

    def psu_dict_update(self):
        return self.restful.get_psu_info(self.name)

    def psu_status_dict_update(self):
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return None
        status = psu_info.get('status', None)
        psu_status_dict = {}
        psu_status_dict['InputStatus'] = False
        psu_status_dict['FanStatus'] = False
        psu_status_dict['TempStatus'] = False
        psu_status_dict['OutputStatus'] = False
        psu_status_dict['Temperature'] = {}
        psu_status_dict['Temperature']['Value'] = PSU_ERR_VALUE
        psu_status_dict['Temperature']['Max'] = PSU_ERR_VALUE
        psu_status_dict['FanSpeed'] = {}
        psu_status_dict['FanSpeed']['Value'] = 0
        psu_status_dict['FanSpeed']['Ratio'] = 0
        psu_status_dict['FanSpeed']['Max'] = 0
        if status is None or status in self.restful.restful_err_str:
            return psu_status_dict
        status = int(status, 16)
        if status & PSU_POWER_LOSS == 0:
            psu_status_dict['InputStatus'] = True
        if status & PSU_FAN_FAULT == 0:
            psu_status_dict['FanStatus'] = True
            if psu_info.get('fan_speed') not in self.restful.restful_err_str:
                psu_status_dict['FanSpeed']['Value'] = int(psu_info.get('fan_speed'))
            if psu_info.get('fan_ratio') not in self.restful.restful_err_str:
                psu_status_dict['FanSpeed']['Ratio'] = int(psu_info.get('fan_ratio'))
            if psu_info.get('fan_speed_max') not in self.restful.restful_err_str:
                psu_status_dict['FanSpeed']['Max'] = int(psu_info.get('fan_speed_max'))
        if status & PSU_TEMP_FAULT == 0:
            psu_status_dict['TempStatus'] = True
        if status & PSU_VOL_FAULT == 0:
            psu_status_dict['OutputStatus'] = True

        psu_temp = psu_info.get('temp0', None)
        if psu_temp is not None:
            if psu_temp.get('value') not in self.restful.restful_err_str:
                psu_status_dict['Temperature']['Value'] = float(psu_temp.get('value'))
            if psu_temp.get('max') not in self.restful.restful_err_str:
                psu_status_dict['Temperature']['Max'] = float(psu_temp.get('max'))
        return psu_status_dict

    def psu_power_dict_update(self):
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return None
        psu_power_dict = {}
        psu_power_dict['Outputs'] = {}
        psu_power_dict['Outputs']['Voltage'] = {}
        psu_power_dict['Outputs']['Current'] = {}
        psu_power_dict['Outputs']['Power'] = {}
        psu_power_dict['Outputs']['Max_Power'] = {}
        psu_power_dict['Inputs'] = {}
        psu_power_dict['Inputs']['Voltage'] = {}
        psu_power_dict['Inputs']['Current'] = {}
        psu_power_dict['Inputs']['Power'] = {}
        psu_power_dict['Outputs']['Voltage']['Value'] = psu_info.get('out_vol', PSU_ERR_VALUE)
        psu_power_dict['Outputs']['Current']['Value'] = psu_info.get('out_cur', PSU_ERR_VALUE)
        psu_power_dict['Outputs']['Power']['Value'] = psu_info.get('out_power', PSU_ERR_VALUE)
        psu_power_dict['Outputs']['Max_Power']['Value'] = psu_info.get('out_max_power', PSU_ERR_VALUE)
        psu_power_dict['Outputs']['Voltage']['HighAlarm'] = psu_info.get('out_cur', PSU_ERR_VALUE)
        psu_power_dict['Outputs']['Voltage']['LowAlarm'] = psu_info.get('out_cur', PSU_ERR_VALUE)
        psu_power_dict['Inputs']['Voltage']['Value'] = psu_info.get('in_vol', PSU_ERR_VALUE)
        psu_power_dict['Inputs']['Current']['Value'] = psu_info.get('in_cur', PSU_ERR_VALUE)
        psu_power_dict['Inputs']['Power']['Value'] = psu_info.get('in_power', PSU_ERR_VALUE)
        psu_power_dict['Inputs']['Type'] = psu_info.get('type', "N/A")
        return psu_power_dict

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU {}".format(self.index)

    def get_mfr_id(self):
        """
        Retrieves the manufacturer's name (or ID) of the device
        Returns:
            string: Manufacturer's id of device
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return "N/A"
        val = psu_info.get("vendor", "")
        if val == "":
            return "N/A"
        return val.rstrip('#')

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return False
        status = psu_info.get('present', None)
        if status is not None:
            if status in self.restful.restful_err_str:
                return False
            if int(status) == 0:
                return False
            return True
        # present is None, try to read status
        status = psu_info.get('status', None)
        if status is None or status in self.restful.restful_err_str:
            return False
        if int(status, 16) & PSU_NOT_PRESENCE == 0:
            return True
        return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return "N/A"
        val = psu_info.get("model_name", "")
        if val == "":
            return "N/A"
        return val

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return "N/A"
        val = psu_info.get("serial_number", "")
        if val == "":
            return "N/A"
        return val.rstrip('#')

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        if not self.get_presence():
            return False
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return False
        status = psu_info.get('status', None)
        if status is None or status in self.restful.restful_err_str:
            return False
        status = int(status, 16)
        if status != 0:
            return False
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_output = psu_power_dict.get("Outputs", None)
            if cur_psu_output is None:
                return 0
            tmp = cur_psu_output.get("Voltage", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
        return self._get_value(value)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_output = psu_power_dict.get("Outputs", None)
            if cur_psu_output is None:
                return 0
            tmp = cur_psu_output.get("Current", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
        return self._get_value(value)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_output = psu_power_dict.get("Outputs", None)
            if cur_psu_output is None:
                return 0
            tmp = cur_psu_output.get("Power", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
            value = round(float(value) / 1000, 1)
        return self._get_value(value)


    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        if not self.get_presence():
            return False
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return False
        status = psu_info.get('status', None)
        if status is None or status in self.restful.restful_err_str:
            return False
        status = int(status, 16)
        if status != 0:
            return False
        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return self.STATUS_LED_COLOR_OFF
        led_status = psu_info.get('led_status', None)
        if led_status is None:
            return self.STATUS_LED_COLOR_OFF

        return self.led_map.get(led_status, "unknown status %s" % led_status)

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        # not supported
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        cur_psu_temp = psu_status_dict.get("Temperature", None)
        if cur_psu_temp is None:
            return 0
        value = cur_psu_temp.get("Value", None)
        if value is None or value in self.restful.restful_err_str:
            return 0
        return self._get_value(value)

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        cur_psu_temp = psu_status_dict.get("Temperature", None)
        if cur_psu_temp is None:
            return 0
        value = cur_psu_temp.get("Max", None)
        if value is None or value in self.restful.restful_err_str:
            return 0
        return self._get_value(value)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return 13.0

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return 11.8

    def get_input_voltage(self):
        """
        Get the input voltage of the PSU

        Returns:
            A float number, the input voltage in volts,
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_input = psu_power_dict.get("Inputs", None)
            if cur_psu_input is None:
                return 0
            tmp = cur_psu_input.get("Voltage", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
        return self._get_value(value)

    def get_input_current(self):
        """
        Get the input electric current of the PSU

        Returns:
            A float number, the input current in amperes, e.g 220.3
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_input = psu_power_dict.get("Inputs", None)
            if cur_psu_input is None:
                return 0
            tmp = cur_psu_input.get("Current", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
        return self._get_value(value)

    def get_input_power(self):
        """
        Get the input current energy of the PSU

        Returns:
            A float number, the input power in watts, e.g. 302.6
        """
        psu_status_dict = self.psu_status_dict_update()
        if psu_status_dict is None:
            return 0
        if psu_status_dict.get("InputStatus", False) is False:
            value = 0
        else:
            psu_power_dict = self.psu_power_dict_update()
            if psu_power_dict is None:
                return 0
            cur_psu_input = psu_power_dict.get("Inputs", None)
            if cur_psu_input is None:
                return 0
            tmp = cur_psu_input.get("Power", None)
            if tmp is None:
                return 0
            value = tmp.get("Value", 0)
            if value in self.restful.restful_err_str:
                return 0
            value = round(float(value) / 1000, 1)
        return self._get_value(value)

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return "N/A"
        val = psu_info.get("hardware_version", "")
        if val == "":
            return "N/A"
        return val

    def get_vendor(self):
        """
        Retrieves the vendor name of the psu

        Returns:
            string: Vendor name of psu
        """
        psu_info = self.psu_dict_update()
        if psu_info is None:
            return "N/A"
        val = psu_info.get("vendor", "")
        if val == "":
            return "N/A"
        return val.rstrip('#')

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        psu_power_dict = self.psu_power_dict_update()
        if psu_power_dict is None:
            return 0
        cur_psu_output = psu_power_dict.get("Outputs", None)
        if cur_psu_output is None:
            return 0
        tmp = cur_psu_output.get("Max_Power", None)
        if tmp is None:
            return 0
        value = tmp.get("Value", 0)
        if value in self.restful.restful_err_str:
            return 0
        value = round(float(value) / 1000, 1)
        return self._get_value(value)


    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        return False

    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts
        Returns:
            An integer, the capacity of PSU
        """
        return int(self.get_maximum_supplied_power())

    def get_type(self):
        """
         Gets the type of the PSU
         Returns:
             A string, the type of PSU (AC/DC)
        """
        psu_power_dict = self.psu_power_dict_update()
        if psu_power_dict is None:
            return "N/A"
        psu_inputs = psu_power_dict.get("Inputs", None)
        if psu_inputs is None:
            return "N/A"
        tmp = psu_inputs.get("Type", None)
        if tmp is None:
            return "N/A"
        if tmp == "0":
            return "DC"
        if tmp == "1":
            return "AC"
        return "N/A"

    def get_service_tag(self):
        """
        Retrieves the service tag of the device
        Returns:
            string: The service tag of the device
        """
        return 'N/A'

