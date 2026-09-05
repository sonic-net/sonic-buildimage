#############################################################################
# Dell S6100
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
        self.eeprom_path = "/sys/class/i2c-adapter/i2c-2/2-0050/eeprom"
        super(board, self).__init__(self.eeprom_path, 0, '', True)

    def serial_number_str(self, e):
        """Return service tag instead of serial number"""

        (is_valid, results) = self.get_tlv_field(e, self._TLV_CODE_SERVICE_TAG)
        if is_valid == False:
            return "Bad service tag"

        # 'results' is a list containing 3 elements, type (int), length (int),
        # and value (string) of the requested TLV
        return results[2].decode('ascii')
