#!/usr/bin/env python

try:
    from sonic_eeprom import eeprom_tlvinfo
    import binascii
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    """Platform-specific Eeprom class"""

    def __init__(self):
        eeprom_path = "/sys/bus/i2c/devices/1-0056/eeprom"
        if eeprom_path is None:
            raise ValueError("get eeprom path failed")

        super(Eeprom, self).__init__(eeprom_path, 0, "", True)


    def modelnumber(self, e):
        '''
        Returns the value field of the model(part) number TLV as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return super(Eeprom, self).part_number_str(e)

        return t[2].decode("ascii")

    def deviceversion(self, e):
        '''
        Returns the value field of the Device Version as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return str(ord(t[2]))
