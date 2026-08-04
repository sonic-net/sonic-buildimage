#############################################################################
# Celestica
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################

try:
    import os
    import time
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


PLATFORM_CPLD_PATH = '/sys/devices/platform/e1031.smc/'
SETREG_FILE = 'setreg'
GETREG_FILE = 'getreg'
WDT_COMMON_ERROR = -1
MMC_VERSION_REG = "0x100"

# watchdog infomation for cpld v06
V06_MMC_VERSION = 0x05
V06_WDT_WIDTH = '0x110'
V06_WDT_WIDTH_SELECTOR = {
    30: '0x1',
    60: '0x2',
    180: '0x3'
}

V06_CPLD_WDT_INFO = {
    'wdt_en_reg': '0x111',
    'wdt_en_cmd': '0x0',
    'wdt_dis_cmd': '0x1'
}

# watchdog infomation
WDT_TIMER_L_BIT_REG = '0x117'
WDT_TIMER_M_BIT_REG = '0x118'
WDT_TIMER_H_BIT_REG = '0x119'
WDT_KEEP_ALVIVE_REG = '0x11a'

CPLD_WDT_INFO = {
    'wdt_en_reg': '0x116',
    'wdt_en_cmd': '0x1',
    'wdt_dis_cmd': '0x0'
}


class Watchdog(WatchdogBase):

    def __init__(self):
        # Init api_common
        self._api_common = Common()

        # Init cpld reg path
        self.setreg_path = os.path.join(PLATFORM_CPLD_PATH, SETREG_FILE)
        self.getreg_path = os.path.join(PLATFORM_CPLD_PATH, GETREG_FILE)

        self.mmc_v = self._get_mmc_version()
        self.cpld_info = V06_CPLD_WDT_INFO if self.mmc_v <= V06_MMC_VERSION else CPLD_WDT_INFO

        # Set default value
        self._disable()
        self.armed = False
        self.timeout = 0

    def _get_mmc_version(self):
        hex_str_v = self._api_common.get_reg(self.getreg_path, MMC_VERSION_REG)
        return int(hex_str_v, 16)

    def _get_level_hex(self, sub_hex):
        sub_hex_str = sub_hex.replace("x", "0")
        return hex(int(sub_hex_str, 16))

    def _seconds_to_lmh_hex(self, seconds):
        ms = seconds*1000  # calculate timeout in ms format
        hex_str = hex(ms)
        l = self._get_level_hex(hex_str[-2:])
        m = self._get_level_hex(hex_str[-4:-2])
        h = self._get_level_hex(hex_str[-6:-4])
        return (l, m, h)

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        return self._api_common.set_reg(self.setreg_path, self.cpld_info['wdt_en_reg'], self.cpld_info['wdt_en_cmd'])

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        return self._api_common.set_reg(self.setreg_path, self.cpld_info['wdt_en_reg'], self.cpld_info['wdt_dis_cmd'])

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        if self.mmc_v <= V06_MMC_VERSION:
            self._disable()
            self._enable()

        else:
            self._api_common.set_reg(
                self.setreg_path, WDT_KEEP_ALVIVE_REG, self.cpld_info['wdt_en_cmd'])

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """

        if self.mmc_v <= V06_MMC_VERSION:
            timeout_hex = V06_WDT_WIDTH_SELECTOR.get(seconds, '0x2')
            seconds = 60 if timeout_hex == '0x2' else seconds
            self._api_common.set_reg(
                self.setreg_path, V06_WDT_WIDTH, timeout_hex)

        else:
            (l, m, h) = self._seconds_to_lmh_hex(seconds)
            self._api_common.set_reg(
                self.setreg_path, WDT_TIMER_H_BIT_REG, h)  # set high bit
            self._api_common.set_reg(
                self.setreg_path, WDT_TIMER_M_BIT_REG, m)  # set med bit
            self._api_common.set_reg(
                self.setreg_path, WDT_TIMER_L_BIT_REG, l)  # set low bit

        return seconds

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

        if seconds < 0 or seconds > 180:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)

            if self.armed:
                self._keepalive()
            else:
                self._enable()
                self.armed = True

            ret = self.timeout
            self.arm_timestamp = time.time()
        except IOError as e:
            print("Error: unable to enable wdt due to : {}".format(e))

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        disarmed = False
        try:
            self._disable()
            self.armed = False
            disarmed = True
        except IOError as e:
            print("Error: unable to disable wdt due to : {}".format(e))
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
            timeleft = int(self.timeout - (time.time() - self.arm_timestamp))

        return timeleft
