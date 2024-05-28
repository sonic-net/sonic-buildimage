#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################


try:
    from sonic_platform_pddf_base.pddf_platform import PddfPlatform
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Platform(PddfPlatform):
    """
    PDDF Platform-Specific Platform Class
    """

    def __init__(self):
        PddfPlatform.__init__(self)

    # Provide the functions/variables below for which implementation is to be overwritten
root@sonic:/usr/local/lib/python3.11/dist-packages/sonic_platform# cat ./psu.py 
#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 150

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

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """

        # power is returned in micro watts
        return round(float(self.get_input_voltage()*self.get_input_current()), 2)
