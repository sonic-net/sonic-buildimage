"""
Module contains an implementation of SONiC Platform Base API and
provides access to hardware watchdog
"""

import os
import fcntl
import array
import time
from sonic_platform_base.watchdog_base import WatchdogBase

""" ioctl constants """
IO_WRITE = 0x40000000
IO_READ = 0x80000000
IO_SIZE_INT = 0x00040000
IO_READ_WRITE = 0xC0000000
IO_TYPE_WATCHDOG = ord('W') << 8

WDR_INT = IO_READ | IO_SIZE_INT | IO_TYPE_WATCHDOG
WDWR_INT = IO_READ_WRITE | IO_SIZE_INT | IO_TYPE_WATCHDOG

""" Watchdog ioctl commands """
WDIOC_SETOPTIONS = 4 | WDR_INT
WDIOC_KEEPALIVE = 5 | WDR_INT
WDIOC_SETTIMEOUT = 6 | WDWR_INT
WDIOC_GETTIMEOUT = 7 | WDR_INT
WDIOC_SETPRETIMEOUT = 8 | WDWR_INT
WDIOC_GETPRETIMEOUT = 9 | WDR_INT
WDIOC_GETTIMELEFT = 10 | WDR_INT

""" Watchdog status constants """
WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002

""" watchdog sysfs """
WD_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"
WD_GPIO_PATH = "/sys/class/gpio/gpio41/value"
CPLD_DIR = "/sys/bus/i2c/devices/0-0041/"

WD_COMMON_ERROR = -1

class WatchdogImplBase(WatchdogBase):
    """
    Base class that implements common logic for interacting
    with watchdog using ioctl commands
    """

    def __init__(self, wd_device_path):
        """
        Open a watchdog handle
        @param wd_device_path Path to watchdog device
        """
        super(WatchdogImplBase, self).__init__()
        
        self.watchdog=""
        self.watchdog_path = wd_device_path
        self.watchdog_gpio_reg = WD_GPIO_PATH
        self.wd_state_reg = WD_SYSFS_PATH+"state"
        self.wd_timeout_reg = WD_SYSFS_PATH+"timeout"
        self.wd_timeleft_reg = WD_SYSFS_PATH+"timeleft"
    
        self.timeout = self._gettimeout()

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if (int(self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if (int(self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """

        req = array.array('h', [WDIOS_DISABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)
        self._write_sysfs_file(self.watchdog_gpio_reg, 1)
        self._read_sysfs_file(CPLD_DIR+"last_reset_cause")
        
    def _enablewatchdog(self):
        """
        Turn on the watchdog timer
        """

        req = array.array('h', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """

        fcntl.ioctl(self.watchdog, WDIOC_KEEPALIVE)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """

        req = array.array('I', [seconds])
        fcntl.ioctl(self.watchdog, WDIOC_SETTIMEOUT, req, True)

        return int(req[0])

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        timeout=0
        timeout=self._read_sysfs_file(self.wd_timeout_reg)

        return timeout

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """

        req = array.array('I', [0])
        fcntl.ioctl(self.watchdog, WDIOC_GETTIMELEFT, req, True)

        return int(req[0])

    def arm(self, seconds):
        """
        Implements arm WatchdogBase API
        """

        ret = WD_COMMON_ERROR
        if (seconds < 0 or seconds > 340 ):
            return ret
        # Stop the watchdog service to gain access of watchdog file pointer
        if self.is_armed():
            os.popen("systemctl stop cpu_wdt.service")
            time.sleep(2)
        if not self.watchdog:
            self.watchdog = os.open(self.watchdog_path, os.O_WRONLY)
        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.is_armed():
                self._keepalive()
            else:
                self._enablewatchdog()
            ret = self.timeout
        except IOError:
            pass
        if(ret == seconds):
            self._write_sysfs_file(self.watchdog_gpio_reg, 0)
        return ret

    def disarm(self):
        """
        Implements disarm WatchdogBase API

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        
        if self.is_armed():
            os.popen("systemctl stop cpu_wdt.service")
            time.sleep(2)
            if not self.watchdog:
                self.watchdog = os.open(self.watchdog_path, os.O_WRONLY)
            try:
                self._disablewatchdog()
                self.timeout = 0
            except IOError:
                return False    
        return True

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """
        status = False

        state = self._read_sysfs_file(self.wd_state_reg)
        if (state != 'inactive'):
            status = True

        return status

    def get_remaining_time(self):
        """
        Implements get_remaining_time WatchdogBase API
        """

        timeleft = WD_COMMON_ERROR

        if self.is_armed():
            timeleft=self._read_sysfs_file(self.wd_timeleft_reg)

        return int(timeleft)
