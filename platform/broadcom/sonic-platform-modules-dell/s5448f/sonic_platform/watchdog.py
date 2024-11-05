#!/usr/bin/env python3

#########################################################################################
#
# This module implements the Watchdog APIs for Dell Platforms in SONiC
#
# A Dell Watchdog is instantiated in one of the 3 modes:
# MODE_I2C where the WD register is accessed Directly over I2C
#   (Z9XXX/S52XX Programmable logic Design Doc Section 3.1.2.8 SYS_WD: Offset: 0x07)
# MODE_CPLD where the WD register is accessed via a CPLD driver
#   (N16XX/N22XX/N32XX CPLD Register Specification Section 3.1.1.8 SYS_WD: Offset: 0x07)
# MODE_PCIE where the WD register is accessed over PCIe IO mapped registers
#   (Z9332 Base Board CPLD Design Specification : WD registers Sections 7.2.40-43)
#
# Chassis init for a platform (which typically instantiates the Watchdog), is required to
# provide a watchdog_spec that contain common and mode specific keys:
#  Common Keys:
#    timers : A list of supported timers on the Platform in seconds (float)
#    check_cpld_version : check a minimum CPLD version that supports a Watchdog (boolean)
#    mode : MODE_I2C/MODE_CPLD/MODE_PCIE
#  Keys specific to MODE_I2C:
#    i2c_bus : the i2c bus on which the CPLD (with WD register) is to be found (int)
#    i2c_addr : the i2c address of the CPLD (with WD register) (hex)
#    wd_reg : The WD register address (hex)
#  Keys specific to MODE_CPLD:
#    cpld_dir : The sysfs path to the CPLD registers (string)
#    cpld_reg_name : The WD CPLD register name (string)
#  Keys specific to MODE_PCIE:
#    io_resource : The dev path to the IO mapped resource (string)
#    io_wd_timer_offset_reg : WD timer register address (hex)
#    io_wd_status_offset_reg : WD status register address (hex)
#    io_wd_timer_punch_offset_reg : WD punch register address (hex)
#
#########################################################################################

try:
    import ctypes
    import subprocess
    import syslog
    import logging
    import logging.handlers
    from logging import DEBUG, INFO, WARNING, ERROR
    import time
    import inspect
    from multiprocessing import Lock
    import sonic_platform.component as Component
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

try:
    from sonic_platform.hwaccess import io_reg_read, io_reg_write
except ImportError as err:
    pass

MODE_INVALID = 0
MODE_I2C = 1
MODE_CPLD = 2
MODE_PCIE = 3

MAX_WD_SOCK_BUFSIZE = 1024
SET_CONFIRM_DELAY = 0.01   # in seconds

WATCHDOG_LOG_FILE = "/var/log/watchdog.log"
WATCHDOG_LOG_PUNCH_INTERVAL = 6 # log every 30 secs

class _timespec(ctypes.Structure):
    _fields_ = [
        ('tv_sec', ctypes.c_long),
        ('tv_nsec', ctypes.c_long)
    ]

# Functions to get the Watchdog timer, enable and punch fields from a single SYS_WD register
# Supports access directly over I2C (MODE_I2C) or thru a CPLD (MODE_CPLD)

class HwAccessFields():
    mode = MODE_INVALID
    # MODE_I2C access specification:
    i2c_get_cmd = ""
    i2c_set_cmd = ""
    # MODE_CPLD access specification:
    cpld_reg_file = ""

    def __init__(self, watchdog_spec):
        if watchdog_spec['mode'] == 'MODE_I2C':
            self.mode = MODE_I2C
            self.i2c_get_cmd = "/usr/sbin/i2cget -y {0} {1} {2}".format(watchdog_spec['i2c_bus'], hex(watchdog_spec['i2c_addr']), hex(watchdog_spec['wd_reg']))
            self.i2c_set_cmd = "/usr/sbin/i2cset -y {0} {1} {2} %s".format(watchdog_spec['i2c_bus'], hex(watchdog_spec['i2c_addr']), hex(watchdog_spec['wd_reg']))
        elif watchdog_spec['mode'] == 'MODE_CPLD':
            self.mode = MODE_CPLD
            self.cpld_reg_file = watchdog_spec['cpld_dir'] + '/' + watchdog_spec['cpld_reg_name']

    def _get_i2c_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, universal_newlines=True)
            stdout = proc.communicate()[0]
            proc.wait()
            return stdout.rstrip('\n')
        except OSError:
            return -1

    def _get_cpld_register(self, reg_file):
        try:
            with open(reg_file, 'r') as file_desc:
                ret_val = file_desc.read()
        except IOError:
            return -1

        return ret_val.strip('\r\n').lstrip(' ')

    def _get_reg_val(self):
        if self.mode == MODE_I2C:
            value = self._get_i2c_command_result(self.i2c_get_cmd)
        elif self.mode == MODE_CPLD:
            value = self._get_cpld_register(self.cpld_reg_file).strip()

        return int(value, 16)

    def get_wd_timer(self):
        ret = self._get_reg_val()
        return -1 if ret == -1 else (ret >> 4) & 0xf

    def get_wd_en(self):
        ret = self._get_reg_val()
        return -1 if ret == -1 else (ret >> 3) & 1

    def _set_cpld_register(self, reg_file, value):
        try:
            with open(reg_file, 'w') as file_desc:
                return file_desc.write(str(value))
        except IOError:
            return -1

    def _set_reg_val(self, val):
        if self.mode == MODE_I2C:
            return self._get_i2c_command_result(self.i2c_set_cmd % (val))

        if self.mode == MODE_CPLD:
            return self._set_cpld_register(self.cpld_reg_file, val)

        return -1

    def set_wd_timer(self, timer):
        reg = self._get_reg_val()
        return self._set_reg_val((reg & 0xF) | ((timer & 0xF) << 4))

    def set_wd_en(self, enable):
        reg = self._get_reg_val()
        return self._set_reg_val((reg & 0xF7) | ((enable & 0x1) << 3))

    def set_wd_punch(self, punch):
        reg = self._get_reg_val()
        return self._set_reg_val((reg & 0xFE) | (punch & 0x1))

    def dump(self):
        resp = "mode: {}\n".format(self.mode)
        if self.mode == MODE_I2C:
            resp += "i2c_get_cmd: {}\n".format(self.i2c_get_cmd)
            resp += "i2c_set_cmd: {}\n".format(self.i2c_set_cmd)
        elif self.mode == MODE_CPLD:
            resp += "cpld_reg_file: {}\n".format(self.cpld_reg_file)

        resp += "wd reg: {}\n".format(bin(self._get_reg_val()))
        return resp

# Functions to get the Watchdog timer, enable and punch from individual watchdog function registers
# Supports register access over PCIe io mapped registers (MODE_PCIE)

class HwAccessRegisters():
    # MODE_PCIE access specification:
    io_resource = ""
    io_wd_timer_offset_reg = 0x0
    io_wd_status_offset_reg = 0x0
    io_wd_timer_punch_offset_reg = 0x0

    def __init__(self, watchdog_spec):
        self.mode = MODE_PCIE
        self.io_resource = watchdog_spec['io_resource']
        self.io_wd_timer_offset_reg = watchdog_spec['io_wd_timer_offset_reg']
        self.io_wd_status_offset_reg = watchdog_spec['io_wd_status_offset_reg']
        self.io_wd_timer_punch_offset_reg = watchdog_spec['io_wd_timer_punch_offset_reg']

    def get_wd_timer(self):
        return io_reg_read(self.io_resource, self.io_wd_timer_offset_reg)

    def get_wd_en(self):
        return io_reg_read(self.io_resource, self.io_wd_status_offset_reg)

    def set_wd_timer(self, timer):
        ret = io_reg_write(self.io_resource, self.io_wd_timer_offset_reg, timer & 0xF)
        return -1 if ret == False else ret

    def set_wd_en(self, enable):
        ret = io_reg_write(self.io_resource, self.io_wd_status_offset_reg, enable & 0x1)
        return -1 if ret == False else ret

    def set_wd_punch(self, punch):
        ret = io_reg_write(self.io_resource, self.io_wd_timer_punch_offset_reg, punch & 0x1)
        return -1 if ret == False else ret

    def dump(self):
        resp = "mode: {}\n".format(self.mode)
        resp += "io_resource: {}\n".format(self.io_resource)
        resp += "io_wd_timer_offset_reg: {} io_wd_status_offset_reg: {}\n".format(hex(self.io_wd_timer_offset_reg), hex(self.io_wd_status_offset_reg))
        resp += "io_wd_timer_punch_offset_reg: {}\n".format(hex(self.io_wd_timer_punch_offset_reg))
        resp += "wd timer ({}): {}\n".format(hex(self.io_wd_timer_offset_reg), bin(io_reg_read(self.io_resource, self.io_wd_timer_offset_reg)))
        resp += "wd en ({}): {}\n".format(hex(self.io_wd_status_offset_reg), bin(io_reg_read(self.io_resource, self.io_wd_status_offset_reg)))
        resp += "wd timer punch ({}): {}\n".format(hex(self.io_wd_timer_punch_offset_reg), bin(io_reg_read(self.io_resource, self.io_wd_timer_punch_offset_reg)))
        return resp

# Logger to track if Watchdog is active
class WdLogger():

    logger = None
    log_file = None

    def __init__(self):
        self.logger = logging.getLogger('watchdog')
        self.logger.setLevel(logging.DEBUG)
        self.logger.propagate = False
        self.log_file = logging.FileHandler(WATCHDOG_LOG_FILE)
        self.log_file.setFormatter(logging.Formatter('%(asctime)s %(levelname)s: %(message)s'))
        self.log_file.setLevel(DEBUG)
        self.logger.addHandler(self.log_file)

    def log(self, level, msg, *args, **kwargs):
        self.logger.log(level, msg, *args, **kwargs)

    def debug(self, msg, *args, **kwargs):
        self.log(DEBUG, msg, *args, **kwargs)

    def info(self, msg, *args, **kwargs):
        self.log(INFO, msg, *args, **kwargs)

    def warning(self, msg, *args, **kwargs):
        self.log(WARNING, msg, *args, **kwargs)

    def error(self, msg, *args, **kwargs):
        self.log(ERROR, msg, *args, **kwargs)

class Watchdog(WatchdogBase):
    """Dell Platform-specific Watchdog class"""

    timers = []
    # some platforms need a minimum CPLD version:
    check_cpld_version = False
    wd_capable_cpld_version = True
    wd_lock = Lock()

    # Counters to track wd register writes:
    wd_set_timer_count = []    # A counter for each timer offset
    wd_set_enable_count = 0    # Arms
    wd_set_disable_count = 0   # Disarms
    wd_set_punch_count = 0     # Punches
    wd_set_timer_failed = []   # A failed counter for each timer offset
    wd_set_enable_failed = 0   # Arms failed
    wd_set_disable_failed = 0  # Disarms failed
    wd_set_punch_failed = 0    # Punches failed

    armed_time = 0
    timeout = 0
    CLOCK_MONOTONIC = 1
    _errno = 0

    def __init__(self, watchdog_spec):
        self._librt = ctypes.CDLL('librt.so.1', use_errno=True)
        self._clock_gettime = self._librt.clock_gettime
        self._clock_gettime.argtypes = [ctypes.c_int, ctypes.POINTER(_timespec)]

        self.timers = watchdog_spec['timers']
        self.wd_set_timer_count = [0] * len(self.timers)
        self.wd_set_timer_failed = [0] * len(self.timers)

        self.check_cpld_version = watchdog_spec['check_cpld_version']
        if self.check_cpld_version:
            cpld_version = Component.get_cpld0_version()
            syslog.syslog(syslog.LOG_ERR, "Watchdog: cpld version is {}\n".format(cpld_version))
            wd_enabled_version = "0.8"

            if cpld_version < wd_enabled_version:
                syslog.syslog(syslog.LOG_ERR, 'Watchdog: Older System CPLD ver, Update to 0.8 to support watchdog ')
                self.wd_capable_cpld_version = False

        if (watchdog_spec['mode'] == 'MODE_I2C' or watchdog_spec['mode'] == 'MODE_CPLD'):
            self.hwaccess = HwAccessFields(watchdog_spec)
        elif watchdog_spec['mode'] == 'MODE_PCIE':
            self.hwaccess = HwAccessRegisters(watchdog_spec)
        else:
            syslog.syslog(syslog.LOG_ERR, "Watchdog: watchdog_spec not set! \n")

        self.logger = WdLogger()

# Functions to set the Watchdog timer, enable and punch registers/fields

    def _set_confirm_wd_timer(self, timer):
        """
        This function will set the wd timer, wait for a specific time and confirm if the set was successful
        """
        caller_frame = inspect.stack()[1][0]
        frame_info = inspect.getframeinfo(caller_frame)

        self.wd_lock.acquire()
        ret = self.hwaccess.set_wd_timer(timer & 0xF)
        time.sleep(SET_CONFIRM_DELAY)
        curr_timer = self.hwaccess.get_wd_timer()
        self.wd_lock.release()

        if((ret == -1) or (timer != curr_timer)):
            syslog.syslog(syslog.LOG_ERR, "Watchdog: [{}:{}] set {} failed for WD_TIMER ret {} read back {} \n".format(frame_info.function, frame_info.lineno, timer, ret, curr_timer))
            self.wd_set_timer_failed[timer & 0xF] += 1
            return -1

        self.wd_set_timer_count[timer & 0xF] += 1
        return 0

    def _set_confirm_wd_en(self, enable):
        """
        This function will set the wd enable, wait for a specific time and confirm if the set was successful
        """
        caller_frame = inspect.stack()[1][0]
        frame_info = inspect.getframeinfo(caller_frame)

        self.wd_lock.acquire()
        ret = self.hwaccess.set_wd_en(enable & 0x1)
        time.sleep(SET_CONFIRM_DELAY)
        curr_enable = self.hwaccess.get_wd_en()
        self.wd_lock.release()

        if((ret == -1) or (enable != curr_enable)):
            syslog.syslog(syslog.LOG_ERR, "Watchdog: [{}:{}] set {} failed for WD_EN ret {} read back {} \n".format(frame_info.function, frame_info.lineno, enable, ret, curr_enable))
            if enable == 1:
                self.wd_set_enable_failed += 1
            else:
                self.wd_set_disable_failed += 1
            return -1

        if enable == 1:
            self.wd_set_enable_count += 1
        else:
            self.wd_set_disable_count += 1
        return 0

    def set_wd_punch(self, punch):
        """
        This function will set the punch
        """

        self.wd_lock.acquire()
        ret = self.hwaccess.set_wd_punch(punch & 0x1)
        self.wd_lock.release()

        if ret == -1:
            syslog.syslog(syslog.LOG_ERR, "Watchdog: set {} failed for WD_PUNCH \n".format(punch))
            self.wd_set_punch_failed += 1
            return -1

        self.wd_set_punch_count += 1
        return 0

# Helper functions

    def _get_time(self):
        """
        To get clock monotonic time
        """
        time_spec = _timespec()
        if self._clock_gettime(self.CLOCK_MONOTONIC, ctypes.pointer(time_spec)) != 0:
            self._errno = ctypes.get_errno()
            return 0
        return time_spec.tv_sec + time_spec.tv_nsec * 1e-9

# Watchdog APIs

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater*
        available value.

        Returns:
            An integer specifying the *actual* number of seconds the
            watchdog was armed with. On failure returns -1.
        """
        timer = -1
        for key, timer_seconds in enumerate(self.timers):
            if 0 < seconds <= timer_seconds:
                timer = key
                seconds = timer_seconds
                break

        if timer == -1:
            syslog.syslog(syslog.LOG_ERR, "Watchdog: platform does not support watchdog for {} seconds\n".format(seconds))
            return -1

        if not self.wd_capable_cpld_version:
            syslog.syslog(syslog.LOG_ERR, 'Watchdog: Older System CPLD ver, Update to 0.8 to support watchdog')
            return -1

        curr_timer = self.hwaccess.get_wd_timer()
        if curr_timer != timer:
            self.disarm()
            ret = self._set_confirm_wd_timer(timer)
            if ret == -1:
                return -1

        if self.is_armed():
            # Setting WD Timer punch
            ret = self.set_wd_punch(0)
            if ret == -1:
                self.logger.info("Arm FAILED")
                return -1
            self.armed_time = self._get_time()
            if ((self.wd_set_punch_count + self.wd_set_punch_failed) % WATCHDOG_LOG_PUNCH_INTERVAL) == 0:
                self.logger.info("Arm OK")
            self.timeout = seconds
            return seconds

        ret = self._set_confirm_wd_en(1)
        if ret == -1:
            return -1
        self.armed_time = self._get_time()
        self.timeout = seconds
        return seconds

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        if self.is_armed():
            ret = self._set_confirm_wd_en(0)
            if ret == -1:
                return -1
            self.armed_time = 0
            self.timeout = 0
            return True

        return False

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """

        return bool(self.hwaccess.get_wd_en())

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds
        remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on
            their watchdog timer. If the watchdog is not armed, returns
            -1.

            Currently, we do not have hardware support to show remaining time.
            Due to this limitation, this API is implemented in software.
            This API would return correct software time difference if it
            is called from the process which armed the watchdog timer.
            If this API called from any other process, it would return
            0. If the watchdog is not armed, this API would return -1.
        """
        if not self.is_armed():
            return -1

        if self.armed_time > 0 and self.timeout != 0:
            cur_time = self._get_time()

            if cur_time <= 0:
                return 0

            diff_time = int(cur_time - self.armed_time)

            if diff_time > self.timeout:
                return self.timeout

            return self.timeout - diff_time

        return 0

    def dump(self):
        """
        Dump the access specs for retrieving the Watchdog registers, the registers as well as the counters
        """
        resp = "timers: {}\n".format(self.timers)
        resp += "check_cpld_version: {}\n".format(self.check_cpld_version)

        # Get the mode specific dump
        resp += self.hwaccess.dump()

        resp += "wd set counts : timer {}\n".format(self.wd_set_timer_count)
        resp += "wd set failed : timer {}\n".format(self.wd_set_timer_failed)
        resp += "wd set counts : enable {} disable {} punch {}\n".format(self.wd_set_enable_count, self.wd_set_disable_count, self.wd_set_punch_count)
        resp += "wd set failed : enable {} disable {} punch {}".format(self.wd_set_enable_failed, self.wd_set_disable_failed, self.wd_set_punch_failed)

        if len(resp) > MAX_WD_SOCK_BUFSIZE:
            return "dump output too long! Maximum is {} bytes".format(MAX_WD_SOCK_BUFSIZE)

        return resp
