#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of platform specific watchdog API's
#
#############################################################################

try:
    from sonic_platform_base.linux_watchdog import LinuxWatchdog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Watchdog(LinuxWatchdog):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self):
        LinuxWatchdog.__init__(self)
        self.timeout= 180

    # Provide the functions/variables below for which implementation is to be overwritten
