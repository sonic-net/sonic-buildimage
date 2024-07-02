try:
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(PddfEeprom):

    _TLV_DISPLAY_VENDOR_EXT = True
    _TLV_INFO_MAX_LEN = 256
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data

        # system EEPROM always has device name EEPROM1
        self.eeprom_path = self.pddf_obj.get_path("EEPROM1", "eeprom")
        if self.eeprom_path is None:
            return

        super(PddfEeprom, self).__init__(self.eeprom_path, 0, '', True)
        #super().__init__(self.pddf_obj, self.plugin_data)
        self.eeprom_tlv_dict = dict()
        try:
            self.eeprom_data = self.read_eeprom()
        except Exception as e:
            self.eeprom_data = "N/A"
            raise RuntimeError("PddfEeprom is not Programmed - Error: {}".format(str(e)))
        else:
            eeprom = self.eeprom_data

            if not self.is_valid_tlvinfo_header(eeprom):
                return

            total_length = ((eeprom[9]) << 8) | (eeprom[10])
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < self._TLV_INFO_MAX_LEN and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + (eeprom[tlv_index + 1])]
                code = "0x%02X" % ((tlv[0]))

                if (tlv[0]) == self._TLV_CODE_VENDOR_EXT:
                    name = "Vendor Extension" #lgtm [py/multiple-definition]
                    value = ""
                    if self._TLV_DISPLAY_VENDOR_EXT:
                       for c in tlv[2:2 + tlv[1]]:
                           value += "0x%02X " % c
                else:
                    name, value = self.decoder(None, tlv)

                self.eeprom_tlv_dict[code] = value.strip()
                if (eeprom[tlv_index]) == self._TLV_CODE_CRC_32:
                    break

                tlv_index += (eeprom[tlv_index+1]) + 2

    def get_vendor_extn(self):
        return self.eeprom_tlv_dict.get('0xFD', 'N/A')
