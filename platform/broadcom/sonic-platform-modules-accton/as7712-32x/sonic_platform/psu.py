#!/usr/bin/env python
#


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_name(self):
        return "PSU-{}".format(self.psu_index)

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return 'N/A'

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        threshold = super().get_temperature_high_threshold()

        for psu_thermal_idx in range(self.num_psu_thermals):
            try:
                tmp = self._thermal_list[psu_thermal_idx].get_high_threshold()
                if threshold > tmp or threshold == 0.0:
                    threshold = tmp
            except Exception:
                pass

        return threshold

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = super().get_model()
        if model and model.strip() == "":
            return None

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = super().get_serial()
        if serial and serial.strip() == "":
            return None

        return serial

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if self.get_status() is not True:
            return 0.0

        return super().get_voltage()

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        if self.get_status() is not True:
            return 0.0

        return super().get_current()

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        if self.get_status() is not True:
            return 0.0

        return super().get_power()
