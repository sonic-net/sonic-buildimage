try:
    import os
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256
    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/0-0052/eeprom"
        #Two i2c buses might get flipped order, check them both.
        if not os.path.exists(self.eeprom_path):
            self.eeprom_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-1/1-0052/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
