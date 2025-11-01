#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the Watchdog' information which are available in the platform
"""
try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as import_error:
    raise ImportError(str(import_error) + "- required module not found")


class Watchdog(WatchdogBase):
    """
    Abstract base class for interfacing with a hardware watchdog module
    """

    def __init__(self):
        self.watchdog_path = Devcfg.WATCHDOG_DIR
        super(Watchdog, self).__init__()

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
        w_file = None
        val_file = None
        file_path = self.watchdog_path + "arm"

        try:
            with open(file_path, "w") as w_file:
                w_file.write(str(seconds))
        except Exception as error:
            self.log_error(str(error))
            return False

        try:
            with open(file_path, 'r') as val_file:
                seconds = int(val_file.read())
        except Exception as error:
            self.log_error(str(error))
            return False
        return seconds

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
        A boolean, True if watchdog is disarmed successfully, False
        if not
        """
        val_file = None
        file_path = self.watchdog_path + "disarm"

        try:
            with open(file_path, "w") as val_file:
                val_file.write("1")
        except Exception as error:
            self.log_error(str(error))
            return False

        return True

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.
        Returns:
        A boolean, True if watchdog is armed, False if not
        """
        armd = 0
        val_file = None
        file_path = self.watchdog_path + "is_armed"

        try:
            with open(file_path, 'r') as val_file:
                armd = int(val_file.read())
        except Exception as error:
            self.log_error(str(error))
            return False

        return armd == 1

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds
        remaining on the watchdog timer
        Returns:
        An integer specifying the number of seconds remaining on
        their watchdog timer. If the watchdog is not armed, returns
        -1.
        """
        if not self.is_armed():
            return -1
        val_file = None
        timeleft = 0
        file_path = self.watchdog_path + "timeleft"

        try:
            with open(file_path, 'r') as val_file:
                timeleft = int(val_file.read())
        except Exception as error:
            self.log_error(str(error))
            return 0
        return timeleft
