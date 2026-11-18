#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from sonic_platform_base.thermal_base import ThermalBase
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        temp = super().get_temperature()
        self.minimum_thermal = temp
        self.maximum_thermal = temp

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_temperature(self):
        temperature = super().get_temperature()
        # Record maximum
        if temperature > self.maximum_thermal:
            self.maximum_thermal = temperature
        # Record minimum
        if temperature < self.minimum_thermal:
            self.minimum_thermal = temperature
        return temperature

    def get_high_threshold(self):
        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000
        else:
            return super().get_high_threshold()

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu_thermal:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_crit_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            return (attr_value/float(1000))
        else:
            raise NotImplementedError

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.maximum_thermal

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return super().get_presence()

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
        Get Thermal HW Revision
        return: Thermal Revision 'N/A'
        """
        return "N/A"

class ThermalMon(ThermalBase):
    def __init__(self, index, name):
        self.thermal_index = index + 1
        self.thermal_name = name
