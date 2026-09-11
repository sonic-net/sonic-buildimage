#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(PddfEeprom):

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfEeprom.__init__(self, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def platform_name_str(self):
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_PLATFORM_NAME)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')