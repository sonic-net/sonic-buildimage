try:
    import binascii
    import time
    import optparse
    import warnings
    import os
    import sys
    from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def eeprom_check():
    filepath = "/sys/bus/i2c/devices/0-0057/eeprom"
    if os.path.isfile(filepath):
        return 1  # now board, 0x57
    else:
        return 0  # now board, 0x56


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):
        ret = eeprom_check()
        if ret == 1:
            self.eeprom_path = "/sys/bus/i2c/devices/0-0057/eeprom"
        else:
            self.eeprom_path = "/sys/bus/i2c/devices/0-0056/eeprom"

        super(board, self).__init__(self.eeprom_path, 0, '', True)
