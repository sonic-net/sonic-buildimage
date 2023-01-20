try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 550

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts

        Returns:
            An integer, the capacity of PSU
        """
        return (self.PLATFORM_PSU_CAPACITY)

    def get_type(self):
        """
        Gets the type of the PSU

        Returns:
            A string, the type of PSU (AC/DC)
        """
        ptype = "AC"

        # This platform supports AC PSU
        return ptype

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves the psu index number
        """
        return self.psu_index

    def get_revision(self):
        return "N/A"

    def temperature(self):
        return self.get_temperature()

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return 12.6

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return 11.4
