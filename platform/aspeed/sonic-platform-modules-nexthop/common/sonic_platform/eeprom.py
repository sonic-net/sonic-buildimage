from __future__ import annotations

import os
import time
from nexthop_utils import eeprom_utils

BMC_EEPROM_PATH = "/sys/bus/i2c/devices/4-0050/eeprom"

class Eeprom(eeprom_utils.Eeprom):
    """
    BMC SONiC EEPROM handler for the BMC IDPROM on Chipmunk.

    - Uses Nexthop Eeprom subclass so Vendor Extension TLVs
      (IANA 63074 + custom field codes) map to friendly names like
      "Switch-Host Serial Number".
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


    def system_eeprom_info(self):
        """
        Return the full system EEPROM contents as a dictionary keyed by the
        ONIE TlvInfo type code (e.g. "0x21") with decoded values.

        Backs Chassis.get_system_eeprom_info(). Returns an empty dict if the
        EEPROM cannot be read or has no valid TlvInfo header.
        """
        info = {}
        try:
            e = self.read_eeprom()
        except Exception:
            return info

        if not self.is_valid_tlvinfo_header(e):
            return info

        total_len = (e[9] << 8) | e[10]
        idx = self._TLV_INFO_HDR_LEN
        end = self._TLV_INFO_HDR_LEN + total_len

        while idx + 2 <= len(e) and idx < end:
            t_code = e[idx]
            length = e[idx + 1]
            v_start = idx + 2
            v_end = v_start + length
            if v_end > len(e):
                break

            code_str = "0x%02X" % t_code
            if t_code == self._TLV_CODE_VENDOR_EXT:
                formatted_hex = " ".join("0x%02X" % b for b in e[v_start:v_end])
                info.setdefault(code_str, []).append(formatted_hex)
            else:
                _, decoded = self.decoder(None, e[idx:v_end])
                info[code_str] = decoded

            if t_code == self._TLV_CODE_CRC_32:
                break

            idx = v_end

        return info
