#############################################################################
# Ingrasys S8810-32Q
#
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, name, path, cpld_root, ro):
        i2c_bus = "12"
        i2c_addr = "0056"
        self.eeprom_path = "/sys/class/i2c-adapter/i2c-" + i2c_bus + "/" + i2c_bus + "-" + i2c_addr + "/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)
