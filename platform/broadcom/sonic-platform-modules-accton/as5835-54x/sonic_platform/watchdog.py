#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of platform specific watchdog API's
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_watchdog import PddfWatchdog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Watchdog(PddfWatchdog):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self):
        PddfWatchdog.__init__(self)
        self.timeout= 180

    # Provide the functions/variables below for which implementation is to be overwritten
