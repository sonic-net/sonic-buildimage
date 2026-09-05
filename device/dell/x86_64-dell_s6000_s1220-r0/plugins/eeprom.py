#############################################################################
# Dell S6000
#
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    from sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_platform.eeprom import EepromS6000
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(object):

    def __new__(cls, name, path, cpld_root, ro):
        eeprom_path = "/sys/class/i2c-adapter/i2c-10/10-0053/eeprom"

        with open("/sys/class/dmi/id/product_name", "r") as fd:
            board_type = fd.read()

        if 'S6000-ON' in board_type:
            return TlvInfoDecoder(eeprom_path, 0, '', True)
        else:
            return EepromS6000(is_plugin=True)
