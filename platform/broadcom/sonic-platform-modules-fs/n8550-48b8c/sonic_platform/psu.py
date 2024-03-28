#!/usr/bin/env python
#


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 1200

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
        return float(self.PLATFORM_PSU_CAPACITY)

    def get_type(self):
        """
        Gets the type of the PSU
        Returns:
        A string, the type of PSU (AC/DC)
        """
        return "DC"
