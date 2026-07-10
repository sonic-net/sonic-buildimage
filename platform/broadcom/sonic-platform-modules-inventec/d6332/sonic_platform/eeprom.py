#
# Name: eeprom.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

EEPROM_TOTAL_LEN_HIGH_OFFSET = 9
EEPROM_TOTAL_LEN_LOW_OFFSET  = 10
EEPROM_TLV_TYPE_OFFSET       = 0
EEPROM_TLV_LEN_OFFSET        = 1
EEPROM_TLV_VALUE_OFFSET      = 2

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self):
        self.__eeprom_path = "/sys/class/i2c-adapter/i2c-0/0-0055/eeprom"
        super(Eeprom, self).__init__(self.__eeprom_path, 0, '', True)
        self.__eeprom_tlv_dict = dict()
        try:
            self.__eeprom_data = self.read_eeprom()
        except:
            self.__eeprom_data = "N/A"
            raise RuntimeError("Eeprom is not Programmed")
        else:
            eeprom = self.__eeprom_data

            if not self.is_valid_tlvinfo_header(eeprom):
                return

            total_length = (ord(eeprom[EEPROM_TOTAL_LEN_HIGH_OFFSET]) << 8) | ord(eeprom[EEPROM_TOTAL_LEN_LOW_OFFSET])
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + EEPROM_TLV_VALUE_OFFSET) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + EEPROM_TLV_VALUE_OFFSET
                             + ord(eeprom[tlv_index + EEPROM_TLV_LEN_OFFSET])]
                code = "0x%02X" % (ord(tlv[EEPROM_TLV_TYPE_OFFSET]))

                if ord(tlv[EEPROM_TLV_TYPE_OFFSET]) == self._TLV_CODE_VENDOR_EXT:
                    value = str((ord(tlv[EEPROM_TLV_VALUE_OFFSET]) << 24) | (ord(tlv[EEPROM_TLV_VALUE_OFFSET+1]) << 16) |
                                (ord(tlv[EEPROM_TLV_VALUE_OFFSET+2]) << 8) | ord(tlv[EEPROM_TLV_VALUE_OFFSET+3]))
                    value += str(tlv[6:6 + ord(tlv[EEPROM_TLV_LEN_OFFSET])])
                else:
                    name, value = self.decoder(None, tlv)

                self.__eeprom_tlv_dict[code] = value
                if ord(eeprom[tlv_index]) == self._TLV_CODE_CRC_32:
                    break

                tlv_index += ord(eeprom[tlv_index+EEPROM_TLV_LEN_OFFSET]) + EEPROM_TLV_VALUE_OFFSET

    def part_number_str(self):
        (is_valid, results) = self.get_tlv_field(
                    self.__eeprom_data, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2]

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.__eeprom_tlv_dict

    def get_eeprom_data(self):
        return self.__eeprom_data
