"""
Module contains an implementation of SONiC Platform Base API and
provides access to hardware watchdog
"""

import os
import fcntl
import array
import time

from sonic_platform_base.watchdog_base import WatchdogBase
from sonic_py_common import logger

WD_COMMON_ERROR = -1

sonic_logger = logger.Logger()

# CPLD Watchdog Timer CTRL register bits
WDT_CTRL_INTR_EN_BIT = 0x80
WDT_CTRL_RSET_EN_BIT = 0x40
WDT_CTRL_CNTR_EN_BIT = 0x20

WDT_CTRL_FREQ_SEL_16HZ   = 0x00
WDT_CTRL_FREQ_SEL_4HZ    = 0x01
WDT_CTRL_FREQ_SEL_1HZ    = 0x02
WDT_CTRL_FREQ_SEL_0_25HZ = 0x03

#When clk use WDT_CTRL_FREQ_SEL_0_25HZ
#WDT_MAX_TIMEOUT = 867

#When clk use WDT_CTRL_FREQ_SEL_1HZ
WDT_MAX_TIMEOUT = 255

G3748CPLD_I2C_DIR = "/sys/bus/i2c/devices/0-0066/"
WDT_TIMER_REG_PATH = "/sys/bus/i2c/devices/0-0066/wdt_timer"
WDT_CTRL_REG_PATH  = "/sys/bus/i2c/devices/0-0066/wdt_ctrl"
WDT_INTR_CNT_MASK_REG_PATH = "/sys/bus/i2c/devices/0-0066/wdt_intr_cnt_mas"
WDT_INTR_FLAG_REG_PATH = "/sys/bus/i2c/devices/0-0066/wdt_intr_flag"
WDT_TIMEOUT_PATH = "/sys/bus/i2c/devices/0-0066/wdt_timeout"

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
        self.watchdog_path = wd_device_path
        self.armed = self.is_armed()
        self.timeout = self._gettimeout()

    def _read_cpld_reg(self, reg_file):
        """
        On successful read, returns the value read from given
        reg_name and on failure returns 'ERR'
        """
        rv = 'ERR'
        if (not os.path.isfile(reg_file)):
            return rv
        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'
        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_cpld_reg(self, reg_file, value):
        """
        On successful write, the value read will be written on
        reg_name and on failure returns 'ERR'
        """
        rv = 'ERR'
        if (not os.path.isfile(reg_file)):
            return rv
        try:
            with open(reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'
        # Ensure that the write operation has succeeded
        if (int(self._read_cpld_reg(reg_file)) != value ):
            time.sleep(3)
            if (int(self._read_cpld_reg(reg_file)) != value ):
                rv = 'ERR'
        return rv

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """
        reg = self._read_cpld_reg(WDT_CTRL_REG_PATH)
        # Set reg bit(7-5) to 0 to disable timer
        reg_val = int(reg)
        reg_val = reg_val & ~(WDT_CTRL_INTR_EN_BIT | WDT_CTRL_RSET_EN_BIT | WDT_CTRL_CNTR_EN_BIT)
        self._write_cpld_reg(WDT_CTRL_REG_PATH, reg_val)

    def _enablewatchdog(self):
        """
        Turn on the watchdog timer
        """
        reg = self._read_cpld_reg(WDT_CTRL_REG_PATH)
        # Set reg bit(5) to 1 to enable timer
        reg_val = int(reg) | (WDT_CTRL_INTR_EN_BIT | WDT_CTRL_RSET_EN_BIT | WDT_CTRL_CNTR_EN_BIT)
        self._write_cpld_reg(WDT_CTRL_REG_PATH, reg_val)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        1. The watchdog timer can be set to an initial value from 0 through 0xff.
           Writing 0xff would generate an immediate watchdog reset.
        2. The watchdog timer will increment from the initial value after enabling 
           the watchdog timer reset function or the watchdog interrupt function.
        3. Writing a value to the watchdog timer register will restart the timer count.
        4. The watchdog timer will generate an interrupt when the watchdog timer
           interrupt function is enabled by software and the current timer value 
           reaches watchdog interrupt count mask register value (power on default = 0X7f).
        5. The PLD will reset the whole system when watchdog timer reset function is enabled
           by software and the current timer value is 0XFF.

        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        if (seconds > 255 or seconds < 0):
            sonic_logger.log_warning("Error: WDT timeout value is out of range.")
            return WD_COMMON_ERROR

        # Save the timeout (seconds) value
        self._write_cpld_reg(WDT_TIMEOUT_PATH, seconds)
        
        # Configure the initial timer value to CPLD registor
        if (seconds >= 0 and seconds <= 255):
            # Use clock 1 Hz for WDT_CTRL_REG
            count = seconds
            clkval = WDT_CTRL_FREQ_SEL_1HZ

        init_timer = int(255 - count)
        if (init_timer > 1):
            init_timer = init_timer - 1
        # Set initial timer count
        self._write_cpld_reg(WDT_TIMER_REG_PATH, init_timer)
        # Set clock frequency selector
        # If Bit7 - Bit5 of fval has one bit '1', the timer will be restarted 
        reg = self._read_cpld_reg(WDT_CTRL_REG_PATH)
        fval = (int(reg) & 0xF0) | clkval
        self._write_cpld_reg(WDT_CTRL_REG_PATH, fval)
        
        return seconds

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        timeout = self._read_cpld_reg(WDT_TIMEOUT_PATH)
        return int(timeout)

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        timer_cnt = int(self._read_cpld_reg(WDT_TIMER_REG_PATH))
        if (timer_cnt < 255):
            timer_cnt = timer_cnt + 1
        freq_sel = int(self._read_cpld_reg(WDT_CTRL_REG_PATH)) & 0x03
        timer_cnt_left = 0xFF - int(timer_cnt)
        if (freq_sel == WDT_CTRL_FREQ_SEL_16HZ):
            timeleft = timer_cnt_left / 16
        elif (freq_sel == WDT_CTRL_FREQ_SEL_4HZ):
            timeleft = timer_cnt_left / 4
        elif (freq_sel == WDT_CTRL_FREQ_SEL_1HZ):
            # Change from 1 sec to 0.85 sec to be more closer to real time.
            #timeleft = timer_cnt_left * 0.85
            timeleft = timer_cnt_left
        elif (freq_sel == WDT_CTRL_FREQ_SEL_0_25HZ):
            #timeleft = timer_cnt_left * 4
            # Change from 4 sec to 3.4 sec to be more closer to real time.
            timeleft = timer_cnt_left * 3.4

        return int(timeleft)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        self._disablewatchdog()
        timeout = self._gettimeout()
        self._settimeout(timeout)
        self._enablewatchdog()

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
        ret = WD_COMMON_ERROR
        if seconds < 0:
            return WD_COMMON_ERROR
        if seconds > 255:
            return WD_COMMON_ERROR

        try:
            self._disablewatchdog()
            #if self._gettimeout() != seconds:
            self.timeout = self._settimeout(seconds)
            self._enablewatchdog()
            self.armed = True
            ret = self.timeout
        except IOError as e:
            pass

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        disarmed = False

        if self.is_armed():
            try:
                self._disablewatchdog()
                self.armed = False
                disarmed = True
                #self.timeout = 0
                #self._write_cpld_reg(WDT_TIMEOUT_PATH, self.timeout)
            except IOError:
                pass

        return disarmed

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """
        reg = self._read_cpld_reg(WDT_CTRL_REG_PATH)
        state = (int(reg) & 0x20) >> 5
        if (state == 0):
            return False    # Disabled
        return True         # Enabled

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


