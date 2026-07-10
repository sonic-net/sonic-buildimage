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
    import syslog
    from struct import *
    from array import *
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "eeprom.py"
EEPROM_PATH = "/sys/bus/i2c/devices/0-0056/eeprom"


def log_error(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):

        if not os.path.exists(EEPROM_PATH):
            log_error("Cannot find system eeprom")
            raise RuntimeError("No syseeprom found")

        self.eeprom_path = EEPROM_PATH
        super(board, self).__init__(self.eeprom_path, 0, '', True)
