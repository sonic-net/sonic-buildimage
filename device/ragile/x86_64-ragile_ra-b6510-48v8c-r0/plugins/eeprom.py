try:
    import os
    import sys
    import json
    sys.path.append('/usr/share/sonic/platform/plugins')
    import pddfparse
    #from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):
        global pddf_obj
        global plugin_data
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)) + '/../pddf/pd-plugin.json')) as pd:
            plugin_data = json.load(pd)

        pddf_obj = pddfparse.PddfParse()
        # system EEPROM always has device name EEPROM1
        self.eeprom_path = pddf_obj.get_path("EEPROM1", "eeprom")
        super(board, self).__init__(self.eeprom_path, 0, '', True)
