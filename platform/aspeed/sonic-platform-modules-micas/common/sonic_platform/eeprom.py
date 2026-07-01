from __future__ import annotations

import os
import time
try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error

BMC_EEPROM_PATH = "/sys/bus/i2c/devices/2-0057/eeprom"

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        # Nexthop Eeprom ctor matches TlvInfoDecoder(path, start, status, ro)
        if not os.path.exists(BMC_EEPROM_PATH):
            raise RuntimeError(f"EEPROM device not found at {BMC_EEPROM_PATH}")
        super(Eeprom, self).__init__(BMC_EEPROM_PATH, start=0, status="", ro=True)

    def get_eeprom(self):
        """
        Read EEPROM, update Redis, and return raw bytes.
        syseepromd calls this.
        """
        e = self.read_eeprom()
        # Populate STATE_DB: EEPROM_INFO|* keys
        self.update_eeprom_db(e)
        return e

    def read_eeprom(self):
        # Just delegate to base class
        return super(Eeprom, self).read_eeprom()

    def modelnumber(self, e):
        '''
        Returns the value field of the model(part) number TLV as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return super().part_number_str(e)

        return t[2].decode("ascii")

    def deviceversion(self, e):
        '''
        Returns the value field of the Device Version as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return str(ord(t[2]))