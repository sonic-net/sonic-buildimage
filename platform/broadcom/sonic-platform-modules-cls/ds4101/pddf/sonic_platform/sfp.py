#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    from sonic_platform.cls_sfp import ClsPddfSfp as PddfSfp
except ImportError as e:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_port_or_cage_type(self):
        if self.port_index >= 1 and self.port_index <= 32:
            return self.SFP_CAGE_TYPE_OSFP
        elif self.port_index == 33 or self.port_index == 34:
            return self.SFP_CAGE_TYPE_SFP
        else:
            return "N/A"
