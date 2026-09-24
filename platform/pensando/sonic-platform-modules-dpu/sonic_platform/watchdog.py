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

WATCHDOG_STATUS = "/sys/class/watchdog/watchdog0/status"
WATCHDOG_TIMEOUT = "/sys/class/watchdog/watchdog0/timeout"
WATCHDOG_ARMED_MASK = 0x8000

class Watchdog(WatchdogBase):

    def __init__(self):
        WatchdogBase.__init__(self)

    def arm(self, seconds):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. it is armed but with default seconds, cannot be changed.
        """
        timeout = open(WATCHDOG_TIMEOUT, "r").read()
        if self.is_armed():
            return timeout
        return -1

    def disarm(self):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. It cannot be disarmed
        """
        return False

    def is_armed(self):
        status_hex = open(WATCHDOG_STATUS, "r").read()
        status = int(status_hex, 16)
        if status & WATCHDOG_ARMED_MASK:
            return True
        return False

    def get_remaining_time(self):
        """
        For Pensando, watchdog gets handled by pensando image running on
        docker:dpu docker. returning -1.
        """
        return -1
