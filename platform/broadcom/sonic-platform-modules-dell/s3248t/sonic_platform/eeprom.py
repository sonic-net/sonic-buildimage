"""
    Dell S3248T

    Platform and model specific eeprom subclass, inherits from the base class,
    and provides the followings:
    - the eeprom format definition
    - specific encoder/decoder if there is special need
"""

try:
    import os.path
    import syslog
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    """ Platform-specific EEPROM class """
    _TLV_DISPLAY_VENDOR_EXT = True

    def __init__(self):
        self.eeprom_path = None
        file_path = '/sys/class/i2c-adapter/i2c-2/2-0050/eeprom'
        if not os.path.exists(file_path):
            return
        self.eeprom_path = file_path
        super(Eeprom, self).__init__(self.eeprom_path, 0, '', True)
        self.eeprom_tlv_dict = dict()
        try:
            self.eeprom_data = self.read_eeprom()
        except:
            self.eeprom_data = "N/A"
            raise RuntimeError("EEPROM is not Programmed")

        eeprom = self.eeprom_data

        if not self.is_valid_tlvinfo_header(eeprom):
            syslog.syslog(syslog.LOG_ERR, "Not valid tlvinfo header")
            return

        total_length = (eeprom[9] << 8) | eeprom[10]
        tlv_index = self._TLV_INFO_HDR_LEN
        tlv_end = self._TLV_INFO_HDR_LEN + total_length

        while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
            if not self.is_valid_tlv(eeprom[tlv_index:]):
                break

            tlv = eeprom[tlv_index:tlv_index + 2 + eeprom[tlv_index + 1]]
            code = "0x%02X" % (tlv[0])
            name, value = self.decoder(None, tlv)

            self.eeprom_tlv_dict[code] = value
            if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                break

            tlv_index += eeprom[tlv_index+1] + 2

    def serial_number_str(self):
        """
        Returns the serial number
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_SERIAL_NUMBER)
        if not is_valid:
            return "N/A"
        return results[2].decode('ascii')

    def base_mac_addr(self, eeprom):
        """
        Returns the base mac address found in the system EEPROM
        """
        (is_valid, tlv) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_MAC_BASE)
        if not is_valid or tlv[1] != 6:
            return super(eeprom_tlvinfo.TlvInfoDecoder, self).switchaddrstr(tlv)

        return ":".join([f"{T:02x}" for T in tlv[2]]).upper()

    def modelstr(self):
        """
        Returns the Model name
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_PRODUCT_NAME)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def part_number_str(self):
        """
        Returns the part number
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def serial_str(self):
        """
        Returns the servicetag number
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_SERIAL_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def revision_str(self):
        """
        Returns the device revision
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_LABEL_REVISION)
        if not is_valid:
            return "N/A"

        return results[2].decode('ascii')

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
