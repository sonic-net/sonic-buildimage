"""
SONiC Platform API - Watchdog stub for the Aspeed AST2700 reference platform.

This module is the build-time default sonic_platform on Aspeed BMC images and
is replaced on first boot by the card-specific platform wheel.

On every Aspeed BMC image the hardware watchdog (/dev/watchdog0) is owned
exclusively by the hw-watchdog-mgrd daemon, which opens the device once and
pets it.  The Linux watchdog framework allows only a single open of the
device, so any other process that opens it either steals the device from the
daemon or fails with EBUSY.  This stub therefore never touches the device:
every API is a safe no-op.  Arm/disarm/status must go through the daemon's
IPC socket, which the card-specific wheel implements.
"""

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Watchdog(WatchdogBase):
    """
    No-op watchdog stub for the Aspeed AST2700 reference platform.

    The real hardware watchdog is owned by the hw-watchdog-mgrd daemon; this
    stub never opens /dev/watchdog0 so it cannot contend with the daemon.
    """

    def __init__(self):
        """
        Initialize the Watchdog object
        """
        self.timeout = 0

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog

        Returns:
            A boolean, always False: this stub does not manage the device.
        """
        return False

    def arm(self, seconds):
        """
        Arm the hardware watchdog

        Returns:
            An integer, always -1: this stub does not manage the device.
        """
        return -1

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, always False: this stub does not manage the device.
        """
        return False

    def get_remaining_time(self):
        """
        Get the number of seconds remaining on the watchdog timer

        Returns:
            An integer, always -1: this stub does not manage the device.
        """
        return -1

