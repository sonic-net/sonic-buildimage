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
        self.armed = False
    
    def _open_watchdog(self):
        """
        Open the watchdog device file
        
        Returns:
            True if successful, False otherwise
        """
        if self.watchdog_fd is None:
            try:
                self.watchdog_fd = os.open(self.watchdog_device, os.O_WRONLY)
                return True
            except (IOError, OSError):
                return False
        return True
    
    def _close_watchdog(self):
        """
        Close the watchdog device file
        """
        if self.watchdog_fd is not None:
            try:
                # Write magic close character 'V' to properly close watchdog
                os.write(self.watchdog_fd, b'V')
                os.close(self.watchdog_fd)
            except (IOError, OSError):
                pass
            finally:
                self.watchdog_fd = None
    
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
    
    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds
        
        Args:
            seconds: Timeout value in seconds
            
        Returns:
            An integer specifying the actual number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        if seconds < 0:
            return -1
        
        if not self._open_watchdog():
            return -1
        
        try:
            # Set timeout using ioctl
            timeout_value = array.array('I', [seconds])
            fcntl.ioctl(self.watchdog_fd, WDIOC_SETTIMEOUT, timeout_value, True)
            
            # Enable watchdog
            options = array.array('h', [WDIOS_ENABLECARD])
            fcntl.ioctl(self.watchdog_fd, WDIOC_SETOPTIONS, options, False)
            
            self.timeout = int(timeout_value[0])
            self.armed = True
            
            return self.timeout
        except (IOError, OSError):
            return -1
    
    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        if not self._open_watchdog():
            return False

        try:
            # Disable watchdog
            options = array.array('h', [WDIOS_DISABLECARD])
            fcntl.ioctl(self.watchdog_fd, WDIOC_SETOPTIONS, options, False)

            self.armed = False
            self.timeout = 0

            # Close the watchdog device properly
            self._close_watchdog()

            return True
        except (IOError, OSError):
            return False

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        # Check the state from sysfs
        state = self._read_sysfs_str("state")
        if state == "active":
            self.armed = True
            return True
        else:
            self.armed = False
            return False

    def get_remaining_time(self):
        """
        Get the number of seconds remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on the
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        if not self.is_armed():
            return -1

        # Try to read timeleft from sysfs first
        timeleft = self._read_sysfs_int("timeleft")
        if timeleft >= 0:
            return timeleft

        # If timeleft is not available in sysfs, try ioctl
        if not self._open_watchdog():
            return -1

        try:
            timeleft_value = array.array('I', [0])
            fcntl.ioctl(self.watchdog_fd, WDIOC_GETTIMELEFT, timeleft_value, True)
            return int(timeleft_value[0])
        except (IOError, OSError):
            # If ioctl fails, return the configured timeout as an estimate
            return self.timeout if self.armed else -1

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

