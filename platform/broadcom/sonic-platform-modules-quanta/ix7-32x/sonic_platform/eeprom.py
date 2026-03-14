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

    def decoder(self, s, t):
        '''
        Return a string representing the contents of the TLV field. The format of
        the string is:
            1. The name of the field left justified in 20 characters
            2. The type code in hex right justified in 5 characters
            3. The length in decimal right justified in 4 characters
            4. The value, left justified in however many characters it takes
        The vailidity of EEPROM contents and the TLV field has been verified
        prior to calling this function. The 's' parameter is unused
        '''
        if t[0] == self._TLV_CODE_PRODUCT_NAME:
            name  = "Product Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_PART_NUMBER:
            name = "Part Number"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_SERIAL_NUMBER:
            name  = "Serial Number"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MAC_BASE:
            name = "Base MAC Address"
            value = ":".join(["{:02x}".format(T) for T in t[2:8]]).upper()
        elif t[0] == self._TLV_CODE_MANUF_DATE:
            name = "Manufacture Date"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_DEVICE_VERSION:
            name  = "Device Version"
            value = str(t[2])
        elif t[0] == self._TLV_CODE_LABEL_REVISION:
            name  = "Label Revision"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_PLATFORM_NAME:
            name  = "Platform Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_ONIE_VERSION:
            name  = "ONIE Version"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MAC_SIZE:
            name = "MAC Addresses"
            value = str((t[2] << 8) | t[3])
        elif t[0] == self._TLV_CODE_MANUF_NAME:
            name = "Manufacturer"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MANUF_COUNTRY:
            name = "Manufacture Country"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_VENDOR_NAME:
            name = "Vendor Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_DIAG_VERSION:
            name = "Diag Version"
            # Quanta legacy format of diag version
            if t[1] == 4:
                value = "{}.{}.{}.{}".format('{:02x}'.format(t[2])[0], '{:02x}'.format(t[2])[1],
                                             '{:02x}'.format(t[3])[0], '{:02x}'.format(t[3])[1])
            else:
                value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_SERVICE_TAG:
            name = "Service Tag"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_VENDOR_EXT:
            name = "Vendor Extension"
            value = ""
            if self._TLV_DISPLAY_VENDOR_EXT:
                for c in t[2:2 + t[1]]:
                    value += "0x%02X " % c
        elif t[0] == self._TLV_CODE_CRC_32 and len(t) == 6:
            name = "CRC-32"
            value = "0x%08X" % ((t[2] << 24) | (t[3] << 16) | (t[4] << 8) | t[5])
        # Quanta specific codes below here.
        # These decodes are lifted from their U-Boot codes
        elif t[0] == self._TLV_CODE_QUANTA_MAGIC and len(t) == 3:
            name  = "Magic Number"
            value = "0x%02X" % t[2]
        elif t[0] == self._TLV_CODE_QUANTA_CRC and len(t) == 4:
            name = "QUANTA-CRC"
            value = "0x%04X" % ((t[2] << 8) + t[3])
        elif t[0] == self._TLV_CODE_QUANTA_CARD_TYPE and len(t) == 6:
            name = "Card Type"
            value = "0x%08X" % ((t[2] << 24) | (t[3] << 16) | (t[4] << 8) | t[5])
        elif t[0] == self._TLV_CODE_QUANTA_HW_VERSION and len(t) == 6:
            name = "Hardware Version"
            value = "%d.%d" % (t[2], t[3])
        elif t[0] == self._TLV_CODE_QUANTA_SW_VERSION and len(t) == 6:
            name = "Software Version"
            value = "%d.%d.%d.%d" % ((t[2] >> 4), (t[2] & 0xF), (t[3] >> 4), (t[3] & 0xF))
        elif t[0] == self._TLV_CODE_QUANTA_MANUF_DATE and len(t) == 6:
            name = "Manufacture Date"
            value = "%04d/%d/%d" % (((t[2] << 8) | t[3]), t[4], t[5])
        elif t[0] == self._TLV_CODE_QUANTA_MODEL_NAME:
            name  = "Model Name"
            value = t[2:2 + t[1]].decode("ascii")
        else:
            name = "Unknown"
            value = ""
            for c in t[2:2 + t[1]]:
                value += "0x%02X " % c
        return name, value