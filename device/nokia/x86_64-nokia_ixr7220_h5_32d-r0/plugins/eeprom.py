try:
    import os
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/bus/i2c/devices/4-0054/eeprom"
        if not os.path.exists(self.eeprom_path):
            file = "/sys/bus/i2c/devices/i2c-4/new_device"
            with open(file, 'w') as f:
                f.write('24c02 0x54\n')
        super(board, self).__init__(self.eeprom_path, 0, '', True)
