"""
Module contains an implementation of SONiC Platform Base API and
provides access to hardware watchdog
"""

import os
import fcntl
import array

from sonic_platform_base.watchdog_base import WatchdogBase
from sonic_py_common import logger

""" ioctl constants """
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
WDIOC_GETTIMELEFT = 10 | WDR_INT

""" Watchdog status constants """
WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002

""" watchdog sysfs """
WD_SYSFS_PATH = "/sys/class/watchdog/{}/"
WD_DEV_PATH = "/dev/{}"

WD_COMMON_ERROR = -1

sonic_logger = logger.Logger()


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

        self.watchdog_path = WD_DEV_PATH.format(wd_device_path)
        self.watchdog_sysfs_path = WD_SYSFS_PATH.format(wd_device_path)

        # Opening a watchdog descriptor starts
        # watchdog timer; by default it should be stopped
        #self._disablewatchdog()

        self.timeout = self._gettimeout()

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        sonic_logger.log_info(" Debug disarm watchdog ")

        try:
            self._disablewatchdog()
        except IOError:
            return False

        return True

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """

        watchdog = os.open(self.watchdog_path, os.O_WRONLY)
        req = array.array('h', [WDIOS_DISABLECARD])
        fcntl.ioctl(watchdog, WDIOC_SETOPTIONS, req, False)
        os.close(watchdog)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """

        watchdog = os.open(self.watchdog_path, os.O_WRONLY)
        req = array.array('I', [seconds])
        fcntl.ioctl(watchdog, WDIOC_SETTIMEOUT, req, True)
        os.close(watchdog)

        return int(req[0])

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """

        timeout_path = self.watchdog_sysfs_path + "timeout"
        timeout = self.__read_txt_file(timeout_path)
        return int(timeout)

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """

        timeleft_path = self.watchdog_sysfs_path + "timeleft"
        timeleft = self.__read_txt_file(timeleft_path)
        return int(timeleft)


    def arm(self, seconds):
        """
        Implements arm WatchdogBase API
        """
        sonic_logger.log_info(" Debug arm watchdog4 ")
        ret = WD_COMMON_ERROR
        if seconds < 0:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.is_armed():
                self._keepalive()
            else:
                sonic_logger.log_info(" Debug arm watchdog5 ")
                self._enablewatchdog()
                self._keepalive()
            ret = self.timeout
        except IOError:
            pass

        return ret

    def _enablewatchdog(self):
        """
        Turn on the watchdog timer
        """

        watchdog = os.open(self.watchdog_path, os.O_WRONLY)
        req = array.array('h', [WDIOS_ENABLECARD])
        fcntl.ioctl(watchdog, WDIOC_SETOPTIONS, req, False)
        os.close(watchdog)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """

        watchdog = os.open(self.watchdog_path, os.O_WRONLY)
        fcntl.ioctl(watchdog, WDIOC_KEEPALIVE)
        os.close(watchdog)

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """
        state_path = self.watchdog_sysfs_path + "state"
        state = self.__read_txt_file(state_path)
        return state == "active"

    def get_remaining_time(self):
        """
        Implements get_remaining_time WatchdogBase API
        """

        timeleft = WD_COMMON_ERROR

        if self.is_armed():
            try:
                timeleft = self._gettimeleft()
            except IOError:
                pass

        return timeleft

