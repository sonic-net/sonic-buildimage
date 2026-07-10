#!/usr/bin/env python
#
# Name: eeprom.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    from sonic_eeprom import eeprom_tlvinfo    
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self):
        self.__eeprom_path = "/sys/bus/i2c/devices/3-0054/eeprom"
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

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % tlv[0]

                if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                    value = str((tlv[2] << 24) | (tlv[3] << 16) |
                                (tlv[4] << 8) | tlv[5])
                    value += tlv[6:6 + tlv[1]].decode('ascii')
                else:
                    value = self.decoder(None, tlv)[30:]

                self.__eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

    def serial_number_str(self):
        (is_valid, results) = self.get_tlv_field(
                         self.__eeprom_data, self._TLV_CODE_SERIAL_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def base_mac_addr(self):
        (is_valid, results) = self.get_tlv_field(
                          self.__eeprom_data, self._TLV_CODE_MAC_BASE)
        if not is_valid or results[1] != 6:
            return super(TlvInfoDecoder, self).switchaddrstr(e)

        return ":".join(["{:02x}".format(T) for T in results[2]]).upper()

    def modelstr(self):
        (is_valid, results) = self.get_tlv_field(
                        self.__eeprom_data, self._TLV_CODE_PRODUCT_NAME)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def part_number_str(self):
        (is_valid, results) = self.get_tlv_field(
                    self.__eeprom_data, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def serial_tag_str(self):
        (is_valid, results) = self.get_tlv_field(
                    self.__eeprom_data, self._TLV_CODE_SERVICE_TAG)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def revision_str(self):
        (is_valid, results) = self.get_tlv_field(
                    self.__eeprom_data, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.__eeprom_tlv_dict

