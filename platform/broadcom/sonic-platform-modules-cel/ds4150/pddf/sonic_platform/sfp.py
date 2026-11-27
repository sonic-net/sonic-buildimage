#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    from sonic_platform.cls_sfp import ClsPddfSfp
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Sfp(ClsPddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        ClsPddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_port_or_cage_type(self):
        if self.port_index >= 5 and self.port_index <= 28:
            return self.SFP_CAGE_TYPE_QSFP
        elif self.port_index == 33 or self.port_index == 34:
            return self.SFP_CAGE_TYPE_SFP
        elif (self.port_index >= 1 and self.port_index <= 4) or (self.port_index >= 29 and self.port_index <= 32):
            return self.SFP_CAGE_TYPE_OSFP
        else:
            return "N/A"
        
    def get_lpmode(self):
        if self.get_port_or_cage_type() == self.SFP_CAGE_TYPE_SFP:
            return False
                
        return super().get_lpmode(True)

    def set_lpmode(self, lpmode):
        return super().set_lpmode(lpmode, True)
 

