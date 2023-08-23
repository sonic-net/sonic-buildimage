#
# Copyright (c) 2019-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""
Mellanox

Module contains an implementation of SONiC Platform Base API and
provides access to hardware watchdog on Mellanox platforms
"""

import os
import fcntl
import array
import time

from sonic_platform_base.watchdog_base import WatchdogBase
from . import utils

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

""" Mellanox main watchdog identity string """
WD_MLNX_MAIN_IDENTITY = "mlx-wdt-main"
""" watchdog sysfs """
WD_SYSFS_PATH = "/sys/class/watchdog/"


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

    def _enablecard(self):
        """
        Turn on the watchdog timer
        """

        req = array.array('h', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.watchdog, WDIOC_SETOPTIONS, req, False)

    def _disablecard(self):
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

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """

        return utils.read_int_from_file('/run/hw-management/watchdog/main/timeout')

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """

        return utils.read_int_from_file('/run/hw-management/watchdog/main/timeleft')

    def arm(self, seconds):
        """
        Implements arm WatchdogBase API
        """

        ret = WD_COMMON_ERROR
        if seconds < 0:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.is_armed():
                self._keepalive()
            else:
                self._enablecard()
            ret = self.timeout
        except IOError:
            pass

        return ret

    def disarm(self):
        """
        Implements disarm WatchdogBase API
        """

        disarmed = False
        if self.is_armed():
            try:
                self._disablecard()
                disarmed = True
            except IOError:
                pass

        return disarmed

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """

        return utils.read_str_from_file('/run/hw-management/watchdog/main/state') == 'active'

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

    def __del__(self):
        """
        Close watchdog
        """

        if self._watchdog is not None:
            os.close(self._watchdog)


class WatchdogType1(WatchdogImplBase):
    """
    Watchdog type 1
    """
    TIMESTAMP_FILE = '/tmp/nvidia/watchdog_timestamp'

    def arm(self, seconds):
        """
        Call arm from WatchdgoImplBase and save the timestamp
        when the watchdog was armed
        """

        ret = WatchdogImplBase.arm(self, seconds)
        # Save the watchdog arm timestamp
        # requiered for get_remaining_time()
        os.makedirs('/tmp/nvidia', exist_ok=True)
        utils.write_file(self.TIMESTAMP_FILE, str(time.time()))

        return ret

    def get_remaining_time(self):
        """
        Watchdog Type 1 does not support timeleft
        operation, we will calculate timeleft based
        on timeout and arm timestamp
        """

        timeleft = WD_COMMON_ERROR

        if self.is_armed():
            arm_timestamp = utils.read_float_from_file(self.TIMESTAMP_FILE)
            timeleft = int(self.timeout - (time.time() - arm_timestamp))

        return timeleft

class WatchdogType2(WatchdogImplBase):
    """
    Watchdog type 2
    """

    pass


def is_mlnx_wd_main(dev):
    """
    Checks if dev is Mellanox main watchdog
    """

    try:
        with open("{}/{}/identity".format(WD_SYSFS_PATH, dev)) as identity_file:
            identity = identity_file.read().strip()
            if identity == WD_MLNX_MAIN_IDENTITY:
                return True
    except IOError:
        pass

    return False


def is_wd_type2(dev):
    """
    Checks if dev is Mellanox type 2 watchdog
    """

    return os.path.exists("{}/{}/timeleft".format(WD_SYSFS_PATH, dev))


def get_watchdog():
    """
    Return WatchdogType1 or WatchdogType2 based on system
    """

    watchdog_main_device_name = None

    for device in os.listdir("/dev/"):
        if device.startswith("watchdog") and is_mlnx_wd_main(device):
            watchdog_main_device_name = device
            break

    if watchdog_main_device_name is None:
        return None

    watchdog_device_path = "/dev/{}".format(watchdog_main_device_name)

    watchdog = None

    if is_wd_type2(watchdog_main_device_name):
        watchdog = WatchdogType2(watchdog_device_path)
    else:
        watchdog = WatchdogType1(watchdog_device_path)

    return watchdog
