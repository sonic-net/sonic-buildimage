#############################################################################
# Centec E582-48X6Q
#
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    import binascii
    import time
    import optparse
    import warnings
    import os
    import sys
    import subprocess
    from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/class/i2c-adapter/i2c-0/0-0057/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
