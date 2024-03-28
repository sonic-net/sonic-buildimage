#!/usr/bin/env python

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, name, path, cpld_root, ro):
        #self.eeprom_path = "/etc/sonic/eeprom"
        self.eeprom_path = "/sys/class/i2c-adapter/i2c-0/0-0054/eeprom"
        if not os.path.exists(self.eeprom_path):
            os.system("echo 24c02 0x54 > /sys/class/i2c-adapter/i2c-0/new_device")
        super(board, self).__init__(self.eeprom_path, 0, '', True)
