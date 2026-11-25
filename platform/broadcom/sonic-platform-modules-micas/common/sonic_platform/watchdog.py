#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import fcntl
import logging
import os
import array

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error


# ioctl constants
IO_WRITE = 0x40000000
IO_READ = 0x80000000
IO_READ_WRITE = 0xC0000000
IO_SIZE_INT = 0x00040000
IO_SIZE_40 = 0x00280000
IO_TYPE_WATCHDOG = ord('W') << 8

WDR_INT = IO_READ | IO_SIZE_INT | IO_TYPE_WATCHDOG
WDR_40 = IO_READ | IO_SIZE_40 | IO_TYPE_WATCHDOG
WDWR_INT = IO_READ_WRITE | IO_SIZE_INT | IO_TYPE_WATCHDOG

# Watchdog ioctl command
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

# Watchdog status constants
WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002

WDT_COMMON_ERROR = -1
WDT_IDENTITY = ["iTCO_wdt"]
WDT_SYSFS_PATH = "/sys/class/watchdog/"

DEFAULT_TIMEOUT = 180


class Watchdog(WatchdogBase):
    """
    Abstract base class for interfacing with a hardware watchdog module
    """

    def __init__(self):
        # Opening /dev/watchdog starts or refreshes the timer. Query commands
        # must use sysfs, including when each command creates a new object.
        self.watchdog = None
        self.wdt_main_dev_name = None
        try:
            devices = [dev for dev in os.listdir("/dev/")
                       if dev.startswith("watchdog") and self._is_wd_main(dev)]
            if devices:
                self.wdt_main_dev_name = devices[0]
        except OSError:
            pass
        base = "{}/{}".format(WDT_SYSFS_PATH.rstrip('/'), self.wdt_main_dev_name)
        self.state_path = base + "/state"
        self.timeout_path = base + "/timeout"
        self.timeleft_path = base + "/timeleft"

    def _open_watchdog(self):
        """Open the device only when intentionally changing the watchdog."""
        if self.watchdog is None:
            device = self._get_wdt()
            if device is None:
                raise OSError("Watchdog device not found")
            self.watchdog, self.wdt_main_dev_name = device

    def _is_wd_main(self, dev):
        """
        Checks watchdog identity
        """
        identity = self._read_file("{}/{}/identity".format(WDT_SYSFS_PATH, dev))
        return identity in WDT_IDENTITY

    def _get_wdt(self):
        """
        Retrieves watchdog device
        """
        wdt_main_dev_list = [dev for dev in os.listdir(
            "/dev/") if dev.startswith("watchdog") and self._is_wd_main(dev)]
        if not wdt_main_dev_list:
            return None
        wdt_main_dev_name = wdt_main_dev_list[0]
        watchdog_device_path = "/dev/{}".format(wdt_main_dev_name)
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
        req = array.array('i', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        req = array.array('i', [WDIOS_DISABLECARD])
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

    def arm(self, seconds):
        """Arm or refresh the timer; return the actual timeout, or -1."""
        if (isinstance(seconds, bool) or not isinstance(seconds, int)):
            return WDT_COMMON_ERROR

        # Read state before opening: opening an inactive device starts it.
        state = self._read_file(self.state_path)
        if state not in ("active", "inactive"):
            return WDT_COMMON_ERROR
        was_armed = state == "active"
        try:
            # Opening the device is part of the arm operation. Do not disable
            # it before configuring the requested timeout.
            self._open_watchdog()
            timeout = self._settimeout(seconds)
            # The API tests require the actual timeout to meet the request.
            # Do not report success if the driver silently shortens it.
            if timeout < seconds:
                raise ValueError("Actual watchdog timeout is less than requested")
            self._enable()
            self._keepalive()
            if not self.is_armed():
                raise OSError("Watchdog is not active after enabling")
            return timeout
        except (OSError, ValueError, OverflowError):
            if not was_armed and self.watchdog is not None:
                # Also cover failures during enable/keepalive, not just timeout.
                try:
                    self._disable()
                except OSError:
                    logging.getLogger(__name__).exception(
                        "Failed to disable watchdog after arm failure; "
                        "watchdog may still be armed")
            return WDT_COMMON_ERROR

    def disarm(self):
        """Disable the hardware watchdog; return whether it is disarmed."""
        # Opening an inactive device would itself start the watchdog.
        if self._read_file(self.state_path) == "inactive":
            return True
        try:
            self._open_watchdog()
            self._disable()
            # A successful ioctl alone is not proof of the resulting state.
            return self._read_file(self.state_path) == "inactive"
        except OSError:
            return False

    def is_armed(self):
        """Read live state without starting or refreshing the watchdog."""
        return self._read_file(self.state_path) == "active"

    def get_remaining_time(self):
        """Return remaining seconds, or -1 when inactive or unavailable."""
        if not self.is_armed():
            return WDT_COMMON_ERROR
        try:
            remaining = int(self._read_file(self.timeleft_path))
        except (TypeError, ValueError):
            remaining = WDT_COMMON_ERROR
        if remaining >= 0:
            return remaining

        # Some drivers expose GETTIMELEFT but no usable sysfs timeleft.
        # Reuse an existing descriptor only; opening here would refresh time.
        if self.watchdog is not None:
            try:
                remaining = self._gettimeleft()
                return remaining if remaining >= 0 else WDT_COMMON_ERROR
            except OSError:
                pass
        return WDT_COMMON_ERROR

    def __del__(self):
        """Release our descriptor without sending a magic-close character."""
        watchdog = getattr(self, "watchdog", None)
        if watchdog is not None:
            try:
                os.close(watchdog)
            except OSError:
                pass
            self.watchdog = None

