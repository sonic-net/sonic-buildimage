#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(PddfEeprom):

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfEeprom.__init__(self, pddf_data, pddf_plugin_data)

        eeprom = self.eeprom_data

        try:
            self.update_cache(eeprom)
        except:
            pass

        if not self.is_valid_tlvinfo_header(eeprom):
            return

        total_length = ((eeprom[9]) << 8) | (eeprom[10])
        tlv_index = self._TLV_INFO_HDR_LEN
        tlv_end = self._TLV_INFO_HDR_LEN + total_length

        while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
            if not self.is_valid_tlv(eeprom[tlv_index:]):
                break

            tlv = eeprom[tlv_index:tlv_index + 2
                            + (eeprom[tlv_index + 1])]
            code = "0x%02X" % ((tlv[0]))

            if (tlv[0]) == self._TLV_CODE_VENDOR_EXT:
                name, value = self.decoder(None, tlv)
                self.eeprom_tlv_dict[code] = value

            if (eeprom[tlv_index]) == self._TLV_CODE_CRC_32:
                break

            tlv_index += (eeprom[tlv_index+1]) + 2

    # Provide the functions/variables below for which implementation is to be overwritten
