#############################################################################
# Mellanox
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
    from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):

    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-2/2-0053/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
