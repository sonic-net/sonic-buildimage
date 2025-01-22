#!/usr/bin/env python

try:
    import os
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

def eeprom_check():
    filepath="/sys/bus/i2c/devices/0-0055/eeprom"
    if os.path.isfile(filepath):
        return 1 #now board, 0x56
    else:
        return 0 #now board, 0x57

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        ret=eeprom_check()
        if ret==1:
            self.eeprom_path = "/sys/bus/i2c/devices/0-0055/eeprom"
        else:
            pass

        super(board, self).__init__(self.eeprom_path, 0, '', True)

