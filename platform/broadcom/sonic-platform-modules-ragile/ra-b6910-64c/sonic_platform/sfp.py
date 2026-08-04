#!/usr/bin/env python
import time
try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

EEPROM_RETRY = 5
EEPROM_RETRY_BREAK_SEC = 0.2

class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self._xcvr_api = self.get_xcvr_api()

    def get_eeprom_path(self):
        return self.eeprom_path

    def read_eeprom(self, offset, num_bytes):
        eeprom_raw = None
        try:
            for i in range(EEPROM_RETRY):
                eeprom_raw = PddfSfp.read_eeprom(self, offset, num_bytes)
                if eeprom_raw is None:
                    time.sleep(EEPROM_RETRY_BREAK_SEC)
                    continue
                break
        except Exception as e:
            print("Error: Unable to read eeprom_path: %s" % (str(e)))
            return None

        return eeprom_raw

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            for i in range(EEPROM_RETRY):
                ret = PddfSfp.write_eeprom(self, offset, num_bytes, write_buffer)
                if ret is False:
                    time.sleep(EEPROM_RETRY_BREAK_SEC)
                    continue
                break
        except Exception as e:
            print("Error: Unable to write eeprom_path: %s" % (str(e)))
            return None

        return ret

    def get_power_set(self):
        if not self._xcvr_api.get_lpmode_support():
            return False
        return self._xcvr_api.get_power_set()

    def get_power_override(self):
        if not self._xcvr_api.get_power_override_support() or not self._xcvr_api.get_lpmode_support():
            return False
        return self._xcvr_api.get_power_override()
