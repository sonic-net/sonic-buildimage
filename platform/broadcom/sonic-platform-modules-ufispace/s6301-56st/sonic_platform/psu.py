#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from sonic_platform.psu_fru import PsuFru
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 750

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self.psu_fru = PsuFru(self.psu_index)
               
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU (or PSU capacity)
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return float(self.PLATFORM_PSU_CAPACITY)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """

        # power is returned in micro watts
        return round(float(self.get_voltage()*self.get_current()), 2)

    def get_mfr_id(self):
        """
        Retrieves the manufacturer's name (or id) of the device
 
        Returns:
            string: Manufacturer's id of device
        """
        return self.psu_fru.mfr_id.rstrip()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.psu_fru.model.rstrip()

    def get_serial(self):
        """
        Retrieves the serial number of the device
 
        Returns:
            string: Serial number of device
        """
        return self.psu_fru.serial.rstrip()
