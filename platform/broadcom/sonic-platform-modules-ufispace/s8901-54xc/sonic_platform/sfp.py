#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self.index = index

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_lpmode(self):
        if self.sfp_type == "QSFP28":
            return super().get_lpmode()
        else:
            return False

    def set_lpmode(self, lpmode):
        if self.sfp_type == "QSFP28":
            return super().set_lpmode(lpmode)
        else:
            return False

    def reset(self):
        if self.sfp_type == "QSFP28":
            return super().reset()
        else:
            return False

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED

        return self.SFP_STATUS_OK
