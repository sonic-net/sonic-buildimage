#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides access to hardware watchdog
#
#############################################################################

import os
import fcntl
import array
import time
from sonic_platform_base.watchdog_base import WatchdogBase
from sonic_py_common import logger
from . import utils

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
WD_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"

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
        super(WatchdogImplBase, self).__init__()

        self.watchdog_path = wd_device_path
        self._watchdog = None
        self.timeout = self._gettimeout()

    @property
    def watchdog(self):
        if self._watchdog is None:
            self._watchdog = self.open_handle()
        return self._watchdog

    def open_handle(self):
        return os.open(self.watchdog_path, os.O_WRONLY)

    def __del__(self):
        """
        Close watchdog
        """

        if self._watchdog is not None:
            os.close(self._watchdog)

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        sonic_logger.log_info(" Debug disarm watchdog ")

        if self.is_armed():
            os.popen("systemctl stop watchdog-control.service")
            time.sleep(2)

            try:
                self._disablewatchdog()
                self.timeout = 0
            except IOError:
                return False

        return True

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """

        req = array.array('h', [WDIOS_DISABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)

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

        file_path = WD_SYSFS_PATH + 'timeout'
        return utils.fread_int(file_path)

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """

        file_path = WD_SYSFS_PATH + 'timeleft'
        return utils.fread_int(file_path)


    def arm(self, seconds):
        """
        Implements arm WatchdogBase API
        """
        sonic_logger.log_info(" Debug arm watchdog4 ")
        ret = WD_COMMON_ERROR
        if seconds < 0:
            return ret
        if seconds > 340:
            return ret

        # Stop the watchdog service to gain access of watchdog file pointer
        if self.is_armed():
            os.popen("systemctl stop watchdog-control.service")
            time.sleep(2)

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

        return ret

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

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """

        file_path = WD_SYSFS_PATH + 'state'
        return utils.fread_str(file_path) == 'active'

    def get_remaining_time(self):
        """
        Implements get_remaining_time WatchdogBase API
        """
        if self.is_armed():
            return self._gettimeleft()
        else:
            return -1

