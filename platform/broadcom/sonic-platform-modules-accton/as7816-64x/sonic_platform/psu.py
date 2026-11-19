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
    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU (or PSU capacity)
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return PddfPsu.get_maximum_supplied_power(self)

    def get_type(self):
        """
        Gets the type of the PSU
        Returns:
        A string, the type of PSU (AC/DC)
        """
        return "AC"

    def get_input_voltage(self):
        """Input telemetry is unavailable on the inspected YM-2851F supply."""
        raise NotImplementedError

    def get_input_current(self):
        """Input telemetry is unavailable on the inspected YM-2851F supply."""
        raise NotImplementedError

    def get_input_power(self):
        """Input telemetry is unavailable on the inspected YM-2851F supply."""
        raise NotImplementedError
