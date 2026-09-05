#!/usr/bin/env python3

########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    import time
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


class Thermal(ThermalBase):

    def __init__(self, interface_obj, index):

        self.restful = interface_obj
        self.temp_id = "temp" + str(index)

    def get_service_tag(self):
        """
        Retrieves the service tag of the device
        Returns:
            string: The service tag of the device
        """
        return 'N/A'

    def get_mfr_id(self):
        """
        Retrieves the manufacturer's name (or ID) of the device
        Returns:
            string: Manufacturer's id of device
        """
        return 'N/A'

    def get_sensor(self):
        temp_info = {}
        info = self.restful.get_temp_sensor_by_id(self.temp_id)
        if info is None:
            return None

        name = info.get("alias", "N/A")
        temp_info['alias'] = info.get('alias', "N/A")
        temp_info['type'] = info.get('type', "N/A")
        value = info.get('value')
        if value is None or value in self.restful.restful_err_str:
            temp_info['value'] = None
        else:
            temp_info['value'] = float(value) / 1000
        max =info.get('max')
        if max is None or max in self.restful.restful_err_str:
            temp_info['max'] = None
        else:
            temp_info['max'] = float(max) / 1000
        min = info.get('min')
        if min is None or min in self.restful.restful_err_str:
            temp_info['min'] = None
        else:
            temp_info['min'] = float(min) / 1000
        return temp_info

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return "N/A"
        name = temp_dict.get("alias", "N/A")
        return name

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return "N/A"

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return False
        if temp_dict.get("value") is None or temp_dict.get("max") is None or temp_dict.get("min") is None:
            return False
        if (temp_dict.get("value", -1) >= temp_dict.get("max", 0)
            ) or (temp_dict.get("value", 0) <= temp_dict.get("min", -1)):
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
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return None
        value = temp_dict.get("value", None)
        if value is None:
            return None
        if value < -200:
            return None
        return round(float(value), 1)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return None
        value = temp_dict.get("max", None)
        if value is None:
            return None
        return round(float(value), 1)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return None
        value = temp_dict.get("min", None)
        if value is None:
            return None
        return round(float(value), 1)

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # not supported
        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        # not supported
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return None
        value = temp_dict.get("max", None)
        if value is None:
            return None
        return round(float(value), 1)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temp_dict = self.get_sensor()
        if temp_dict is None:
            return None
        value = temp_dict.get("min", None)
        if value is None:
            return None
        return round(float(value), 1)

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # not supported
        return False

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # not supported
        return False
