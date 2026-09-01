try:
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(PddfEeprom):

    _TLV_DISPLAY_VENDOR_EXT = True

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfEeprom.__init__(self, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
