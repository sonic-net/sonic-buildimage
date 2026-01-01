#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_status(self):
        """
        Retrieves the operational status of the thermal
        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal
        Returns:
            string: Model/part number of Thermal
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal
        Returns:
            string: Serial number of Thermal
        """
        return 'NA'
