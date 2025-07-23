#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Watchdog information
#
#############################################################################

import os

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
#WD_MAIN_IDENTITY = "iTCO_wdt"
WD_MAIN_IDENTITY = "t7132s_wdt"
WDT_SYSFS_PATH = "/sys/class/watchdog/"
DEV_STATE_PATH = "/sys/devices/platform/switchboard/CPLD1/dev_state"
WDT_MAX_PATH   = "/sys/devices/platform/switchboard/CPLD1/wdt_max"
WDT_COUNT_PATH = "/sys/devices/platform/switchboard/CPLD1/wdt_count"


class Watchdog(WatchdogBase):

    def __init__(self):
        self.watchdog = None
        self.wdt_main_dev_name = None
        self.armed = self.is_armed()
        self.timeout = self._gettimeout()

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
        if self.watchdog is not None:
            return

        wdt_main_dev_list = [dev for dev in os.listdir(
            "/dev/") if dev.startswith("watchdog") and self._is_wd_main(dev)]
        if not wdt_main_dev_list:
            self.wdt_main_dev_name = None
            return
        self.wdt_main_dev_name = wdt_main_dev_list[0]

        watchdog_device_path = "/dev/{}".format(self.wdt_main_dev_name)
        try:
            self.watchdog = os.open(watchdog_device_path, os.O_RDWR)
        except (FileNotFoundError, IOError, OSError):
            self.watchdog = None
            self.wdt_main_dev_name = None
        except SystemExit:
            pass

        return

    def _put_wdt(self):
        """
        Release watchdog device
        """
        if self.watchdog is not None:
            os.close(self.watchdog)
            self.watchdog = None
            self.wdt_main_dev_name = None

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
        try:
            with open(DEV_STATE_PATH, "r+") as reg_file:
                content = reg_file.readline().strip()
                reg_value = int(content, 16)
                bit_enable = 0x100
                reg_value_new = reg_value | bit_enable
                reg_file.write(hex(reg_value_new))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        try:
            with open(DEV_STATE_PATH, "r+") as reg_file:
                content = reg_file.readline().strip()
                reg_value = int(content, 16)
                bit_enable = 0x100
                reg_value_new = reg_value & ~bit_enable
                reg_file.write(hex(reg_value_new))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        self._disable()
        self._enable()

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        if seconds > 65535:
            seconds = 65535

        try:
            with open(WDT_MAX_PATH, "r+") as reg_file:
                reg_value = seconds
                reg_file.write(hex(reg_value))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        return seconds

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        seconds = 0
        try:
            with open(WDT_MAX_PATH, "r+") as reg_file:
                content = reg_file.readline().strip()
                reg_value = int(content, 16)
                seconds = reg_value
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        return seconds

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        try:
            with open(WDT_MAX_PATH, "r+") as reg_file:
                content = reg_file.readline().strip()
                reg_value = int(content, 16)
                state_seconds = reg_value
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        try:
            with open(WDT_COUNT_PATH, "r+") as reg_file:
                content = reg_file.readline().strip()
                reg_value = int(content, 16)
                count_seconds = reg_value
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        timeleft = state_seconds - count_seconds
        if timeleft < 0:
            timeleft = 0
            print("Error: incorrect counter: state={} count={}".
                  format(state_seconds, count_seconds))
        elif timeleft > 65535:
            timeleft = 65535
            print("Error: incorrect counter: state={} count={}".
                  format(state_seconds, count_seconds))

        return timeleft

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
        if seconds > 65535:
            return ret

        try:
            self._disable()
            if self._gettimeout() != seconds:
                self.timeout = self._settimeout(seconds)
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
        """
        We always get the HW status because all new instance have
        it's own self.armed. And only the instance had called arm()
        has self.armed = True if self.armed is a class variable.
        """
        # Read status
        try:
            with open(DEV_STATE_PATH) as reg_file:
                content = reg_file.readline().rstrip()
                reg_value = int(content, 16)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        bit_enable = 0x100
        if reg_value & bit_enable:
            return True

        return False

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer
        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        timeleft = WDT_COMMON_ERROR

        if self.is_armed():
            try:
                timeleft = self._gettimeleft()
            except IOError:
                pass

        return timeleft

    def __del__(self):
        """
        Close watchdog
        """
        if self.watchdog is not None:
            os.close(self.watchdog)
