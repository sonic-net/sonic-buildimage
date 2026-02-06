#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the watchdog management function
#
#############################################################################

try:
    from sonic_platform_base.watchdog_base import WatchdogBase 
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Watchdog(WatchdogBase):

    def __init__(self):
        self.device = "/dev/watchdog1"
        self.sysfs = "/sys/devices/platform/cpld_wdt/watchdog/watchdog1"

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """

        with open("%s/state" % self.sysfs, "r") as fd:
            txt = fd.read()
        state = txt.strip()
        self.armed = True if state == "active" else False

        return self.armed

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """

        with open(self.device, "w") as fd:
            fd.write("V")

        if self.is_armed():
            return False
        else:
            return True

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.
        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """

        with open("%s/settimeout" % self.sysfs, "w") as fd:
            fd.write("%d" % seconds)
    
        with open("%s/settimeout" % self.sysfs, "r") as fd:
            timeout = int(fd.read().strip())

        if self.is_armed() == False:
            with open(self.device, "w") as fd:
                fd.write("k")

        if self.is_armed() == False:
            return -1
        else:
            return timeout
  
    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer
        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """

        if self.is_armed():
            with open("%s/timeleft" % self.sysfs, "r") as fd:
                timeleft = int(fd.read().strip())
            return timeleft
        else:
            return -1
