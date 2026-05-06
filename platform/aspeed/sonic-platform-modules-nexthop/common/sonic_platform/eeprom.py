from __future__ import annotations

import os
import time
from sonic_platform import eeprom_utils

BMC_EEPROM_PATH = "/sys/bus/i2c/devices/4-0050/eeprom"

class Eeprom(eeprom_utils.Eeprom):
    """
    BMC SONiC EEPROM handler for the BMC IDPROM on Chipmunk.

    - Uses Nexthop Eeprom subclass so Vendor Extension TLVs
      (IANA 63074 + custom field codes) map to friendly names like
      "Switch Host Serial Number".
    """

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

