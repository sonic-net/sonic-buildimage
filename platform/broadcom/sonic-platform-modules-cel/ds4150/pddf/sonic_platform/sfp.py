#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    import sys
    import syslog
    import time
    from multiprocessing import Lock
    from sonic_platform.cls_sfp import ClsPddfSfp
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(ClsPddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        ClsPddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self.eeprom_lock = Lock()

    def get_port_or_cage_type(self):
        if self.port_index >= 1 and self.port_index <= 16:
            return self.SFP_CAGE_TYPE_OSFP
        elif self.port_index == 17 or self.port_index == 18:
            return self.SFP_CAGE_TYPE_SFP
        else:
            return "N/A"
