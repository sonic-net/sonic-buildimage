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

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_position_in_parent(self):
        """Retrieves 1-based relative physical position in parent device."""
        return self.port_index
