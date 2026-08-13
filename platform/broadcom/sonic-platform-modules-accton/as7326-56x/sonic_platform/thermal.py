#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        
        self.pddf_obj = pddf_data
        self.thermal_obj_name = "TEMP{}".format(self.thermal_index)
        self.thermal_obj = self.pddf_obj.data[self.thermal_obj_name]
        self.min_temperature = None
        self.max_temperature = None

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_name(self):
        if self.is_psu_thermal:
            return "PSU-{0} temp sensor 1".format(self.thermals_psu_index)
        else:
            if 'dev_attr' in self.thermal_obj.keys():
                if 'display_name' in self.thermal_obj['dev_attr']:
                    return str(self.thermal_obj['dev_attr']['display_name'])

            # In case of errors
            return "Temp sensor {0}".format(self.thermal_index)

    def get_status(self):
        get_temp=self.get_temperature()

        if get_temp is not None:
            return True if get_temp else False

    def get_temperature(self):
        current = super().get_temperature()

        if self.min_temperature is None or \
            current < self.min_temperature:
            self.min_temperature = current

        if self.max_temperature is None or \
           current > self.max_temperature:
            self.max_temperature = current

        return current

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

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.min_temperature is None:
            self.get_temperature()

        return self.min_temperature

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.max_temperature is None:
            self.get_temperature()

        return self.max_temperature

