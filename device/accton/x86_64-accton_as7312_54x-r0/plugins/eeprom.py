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


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/bus/i2c/devices/1-0057/eeprom"
        # Two i2c buses might get flipped order, check them both.
        if not os.path.exists(self.eeprom_path):
            self.eeprom_path = "/sys/bus/i2c/devices/0-0057/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
