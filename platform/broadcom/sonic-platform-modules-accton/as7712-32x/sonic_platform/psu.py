#!/usr/bin/env python
#


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 650

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
        # Currently the platform supports only AC type of PSUs
        try:
            import sonic_platform.platform
            ch=sonic_platform.platform.Platform().get_chassis()
            e=ch.sys_eeprom.read_eeprom()
            ret, prod_name = ch.sys_eeprom.get_tlv_field(e,0x21)
            if ret:
                prod_name = prod_name[2].decode('ascii')
                #print "Product name is {}".format(prod_name)
                if '48V' in prod_name:
                    ptype = 'DC'
        except Exception as e:
            print("Error while trying to read syseeprom to get PSU type - {}".format(repr(e)))

        return ptype
