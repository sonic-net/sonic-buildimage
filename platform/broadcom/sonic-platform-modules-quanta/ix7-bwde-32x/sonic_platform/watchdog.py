#!/usr/bin/env python

#############################################################################
# 
# Watchdog contains an implementation of SONiC Platform Base Watchdog API
#
#############################################################################
import fcntl
import os
import array

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

""" ioctl constants """
IO_WRITE = 0x40000000
IO_READ = 0x80000000
IO_READ_WRITE = 0xC0000000
IO_SIZE_INT = 0x00040000
IO_SIZE_40 = 0x00280000
IO_TYPE_WATCHDOG = ord('W') << 8

WDR_INT = IO_READ | IO_SIZE_INT | IO_TYPE_WATCHDOG
WDR_40 = IO_READ | IO_SIZE_40 | IO_TYPE_WATCHDOG
WDWR_INT = IO_READ_WRITE | IO_SIZE_INT | IO_TYPE_WATCHDOG

""" Watchdog ioctl commands """
WDIOC_GETSUPPORT = 0 | WDR_40
WDIOC_GETSTATUS = 1 | WDR_INT
WDIOC_GETBOOTSTATUS = 2 | WDR_INT
WDIOC_GETTEMP = 3 | WDR_INT
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

WDT_COMMON_ERROR = -1
WD_MAIN_IDENTITY = "iTCO_wdt"
WDT_SYSFS_PATH = "/sys/class/watchdog/"

DEFAULT_TIMEOUT=180
watchdog=0

class Watchdog(WatchdogBase):

    def __init__(self):

        self.watchdog, self.wdt_main_dev_name = self._get_wdt()
        if self.wdt_main_dev_name is None:
            raise Exception("Watchdog device is not instantiated")

        self.status_path = "/sys/class/watchdog/%s/status" % self.wdt_main_dev_name
        self.state_path = "/sys/class/watchdog/%s/state" % self.wdt_main_dev_name
        self.timeout_path = "/sys/class/watchdog/%s/timeout" % self.wdt_main_dev_name
        # Set default value
        self._disable()
        self.armed = False
        self.timeout = DEFAULT_TIMEOUT

    def _is_wd_main(self, dev):
        """
        Checks watchdog identity
        """
        identity = self._read_file(
            "{}/{}/identity".format(WDT_SYSFS_PATH, dev))
        return identity == WD_MAIN_IDENTITY

    def _get_wdt(self):
        """
        Retrieves watchdog device
        """
        global watchdog
        wdt_main_dev_list = [dev for dev in os.listdir(
            "/dev/") if dev.startswith("watchdog") and self._is_wd_main(dev)]
        if not wdt_main_dev_list:
            return (None, None)
        wdt_main_dev_name = wdt_main_dev_list[0]
        watchdog_device_path = "/dev/{}".format(wdt_main_dev_name)
        if not watchdog:
           watchdog = os.open(watchdog_device_path, os.O_RDWR)
        return watchdog, wdt_main_dev_name

    def _read_file(self, file_path):
        """
        Read text file
        """
        try:
            with open(file_path, "r") as fd:
                txt = fd.read()
        except IOError:
            return WDT_COMMON_ERROR
        return txt.strip()

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        req = array.array('h', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        req = array.array('h', [WDIOS_DISABLECARD])
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

    def _gettimeout(self, timeout_path):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        req = array.array('I', [0])
        fcntl.ioctl(self.watchdog, WDIOC_GETTIMEOUT, req, True)

        return int(req[0])

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        req = array.array('I', [0])
        fcntl.ioctl(self.watchdog, WDIOC_GETTIMELEFT, req, True)

        return int(req[0])

    #################################################################

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

        ret = WDT_COMMON_ERROR
        if seconds < 0:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.armed:
                self._keepalive()
            else:
                self._settimeout(seconds)
                self._enable()
                self.armed = True
            ret = self.timeout
        except IOError as e:
            pass

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        disarmed = False
        if self.is_armed():
            try:
                self._disable()
                self.armed = False
                disarmed = True
            except IOError:
                pass

        return disarmed

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.
        Returns:
            A boolean, True if watchdog is armed, False if not
        """

        return self.armed

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer
        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """

        timeleft = WDT_COMMON_ERROR

        if self.armed:
            try:
                timeleft = self._gettimeleft()
            except IOError:
                pass

        return timeleft

    def __del__(self):
        """
        Close watchdog
        """

        if self.watchdog is not None :
            os.close(self.watchdog)



