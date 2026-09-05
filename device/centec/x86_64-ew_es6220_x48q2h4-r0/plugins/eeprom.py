#############################################################################
# EmbedWay
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

    def __init__(self, name, path, cpld_root, ro):
        #output = os.popen('i2cdetect -l | grep CP')
        # a=output.read()
        # b=a[4]
        self.eeprom_path = "/home/admin/eeprom.bin"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
