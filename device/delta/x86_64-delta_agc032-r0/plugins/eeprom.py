try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):
        self.eeprom_path = "/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/0-0053/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
