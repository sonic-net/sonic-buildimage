#!/usr/bin/env python3

########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################
import time


class Dcdc(object):

    def __init__(self, interface_obj, index):
        self.dcdc_dict = {}
        self.int_case = interface_obj
        self.index = index
        self.update_time = 0
        self.dcdc_id = "DCDC" + str(index)

    def dcdc_dict_update(self):
        local_time = time.time()
        if not self.dcdc_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            self.dcdc_dict = self.int_case.get_dcdc_by_id(self.dcdc_id)

    def get_name(self):
        """
        Retrieves the name of the sensor

        Returns:
            string: The name of the sensor
        """
        self.dcdc_dict_update()
        return self.dcdc_dict["Name"]

    def get_value(self):
        """
        Retrieves current value reading from sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Value"]
        if value is None:
            value = 0
        return round(float(value), 3)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["High"]
        if value is None:
            value = 0
        return round(float(value), 3)

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Low"]
        if value is None:
            value = 0
        return round(float(value), 3)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Max"]
        if value is None:
            value = 0
        return round(float(value), 3)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of sensor
        """
        self.dcdc_dict_update()
        value = self.dcdc_dict["Min"]
        if value is None:
            value = 0
        return round(float(value), 3)
