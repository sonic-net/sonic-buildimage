"""
SONiC Platform API - Watchdog class for Aspeed BMC

This module provides the Watchdog class for Aspeed AST2700 BMC platform.
"""

import os
import fcntl
import array
import ctypes

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


# Watchdog ioctl commands (from Linux kernel watchdog.h)
WDIOC_GETSUPPORT = 0x80285700
WDIOC_GETSTATUS = 0x80045701
WDIOC_GETBOOTSTATUS = 0x80045702
WDIOC_GETTEMP = 0x80045703
WDIOC_SETOPTIONS = 0x80045704
WDIOC_KEEPALIVE = 0x80045705
WDIOC_SETTIMEOUT = 0xc0045706
WDIOC_GETTIMEOUT = 0x80045707
WDIOC_SETPRETIMEOUT = 0xc0045708
WDIOC_GETPRETIMEOUT = 0x80045709
WDIOC_GETTIMELEFT = 0x8004570a

# Watchdog options
WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002

# Default watchdog device
WATCHDOG_DEVICE = "/dev/watchdog0"

# Sysfs paths for AST2700 watchdog
WATCHDOG_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"


class Watchdog(WatchdogBase):
    """
    Watchdog class for Aspeed AST2700 BMC platform
    
    Provides hardware watchdog functionality using the aspeed_wdt driver.
    """
    
    def __init__(self):
        """
        Initialize the Watchdog object
        """
        self.watchdog_device = WATCHDOG_DEVICE
        self.watchdog_fd = None
        self.timeout = 0
    
    def _read_sysfs_int(self, filename):
        """
        Read an integer value from sysfs

        Args:
            filename: Name of the file in WATCHDOG_SYSFS_PATH

        Returns:
            Integer value, or -1 on error
        """
        try:
            with open(os.path.join(WATCHDOG_SYSFS_PATH, filename), 'r') as f:
                return int(f.read().strip())
        except (IOError, OSError, ValueError):
            return -1

    def _read_sysfs_str(self, filename):
        """
        Read a string value from sysfs

        Args:
            filename: Name of the file in WATCHDOG_SYSFS_PATH

        Returns:
            String value, or empty string on error
        """
        try:
            with open(os.path.join(WATCHDOG_SYSFS_PATH, filename), 'r') as f:
                return f.read().strip()
        except (IOError, OSError):
            return ""

    def _open_watchdog(self):
        """
        Open the watchdog device file

        Returns:
            True if successful, False otherwise
        """
        if self.watchdog_fd is None:
            try:
                self.watchdog_fd = os.open(self.watchdog_device, os.O_WRONLY)
            except (IOError, OSError):
                return False
        return True

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """
        req = array.array('I', [WDIOS_DISABLECARD])
        fcntl.ioctl(self.watchdog_fd, WDIOC_SETOPTIONS, req, False)

    def _enablewatchdog(self):
        """
        Turn on the watchdog timer
        """
        req = array.array('I', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.watchdog_fd, WDIOC_SETOPTIONS, req, False)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        fcntl.ioctl(self.watchdog_fd, WDIOC_KEEPALIVE)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        req = array.array('I', [seconds])
        fcntl.ioctl(self.watchdog_fd, WDIOC_SETTIMEOUT, req, True)

        return int(req[0])

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        return self._read_sysfs_int("timeout")

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        # Check the state from sysfs
        state = self._read_sysfs_str("state")
        if (state != 'inactive'):
            return True
        else:
            return False

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds

        Args:
            seconds: Timeout value in seconds

        Returns:
            An integer specifying the actual number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        if (seconds < 0 or seconds > 300 ):
            return -1

        if not self._open_watchdog():
            return -1
        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.is_armed():
                self._keepalive()
            else:
                self._enablewatchdog()
        except (IOError, OSError):
            return -1

        return self.timeout

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        if not self._open_watchdog():
            return False

        try:
            self._disablewatchdog()
            self.timeout = 0

        except (IOError, OSError):
            return False

        return True

    def get_remaining_time(self):
        """
        Get the number of seconds remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on the
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        return -1

