# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################

try:
    import os
    import time
    import threading
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
    import syslog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Watchdog(WatchdogBase):

    def __init__(self):
        WatchdogBase.__init__(self)

    def arm(self, seconds):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. returning -1.
        """
        return -1

    def disarm(self):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. Setting disarm true to avoid watchdog-timer.service errors
        """
        return True

    def is_armed(self):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. returning False
        """
        return False

    def get_remaining_time(self):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. returning -1.
        """
        return -1