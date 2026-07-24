#!/usr/bin/env python

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/devices/platform/as7535_32xb_sys/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
