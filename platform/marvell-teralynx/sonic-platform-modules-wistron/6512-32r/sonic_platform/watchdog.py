#!/usr/bin/env python

########################################################################
#
# Abstract base class for implementing a platform-specific class with
# which to interact with a hardware watchdog module in SONiC
#
########################################################################

try:
    import subprocess
    from sonic_platform_base.watchdog_base import WatchdogBase
    from shlex import split
    from collections import namedtuple
    from functools import reduce
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

proc_output = namedtuple('proc_output', 'stdout stderr')

WDT_COMMON_ERROR = -1

IPMI_WDT_EN_KICK_CMD = ["ipmitool", "mc", "watchdog", "reset"]
IPMI_WDT_OFF_CMD = ["ipmitool", "mc", "watchdog", "off"]
IPMI_WDT_SET_TIMEOUT_CMD = ["ipmitool", "raw", "0x6", "0x24", "0x4", "0x0", "0x0", "0x0"]
#IPMI_WDT_GET_TIMEOUT_CMD = "ipmitool mc watchdog get | grep Present | awk '{print $3}'"
#IPMI_WDT_GET_STATUS_CMD = "ipmitool mc watchdog get | grep 'Timer Is' | awk '{printf $4}'"

class Watchdog(WatchdogBase):
    """
    Abstract base class for interfacing with a hardware watchdog module
    """

    def __init__(self):
        # Set default value
        self.armed = self._get_status()
        self.timeout = self._gettimeout()

    def pipeline(self, starter_command, *commands):
        if not commands:
            try:
                starter_command, *commands = starter_command.split('|')
            except AttributeError:
                pass
        starter_command = self._parse(starter_command)
        starter = subprocess.Popen(starter_command, stdout=subprocess.PIPE)
        last_proc = reduce(self._create_pipe, map(self._parse, commands), starter)
        return proc_output(*last_proc.communicate())

    def _create_pipe(self, previous, command):
        proc = subprocess.Popen(command, stdin=previous.stdout, stdout=subprocess.PIPE)
        previous.stdout.close()
        return proc

    def _parse(self, cmd):
        try:
            return split(cmd)
        except Exception:
            return cmd

    def _get_status(self):
        #IPMI_WDT_GET_STATUS_CMD
        out, err = self.pipeline("ipmitool mc watchdog get", "grep 'Timer Is'", "awk '{print $4}'")
        status_str = out.decode().rstrip('\n')

        if "Running" in status_str:
            return True

        return False

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        p = subprocess.Popen(IPMI_WDT_EN_KICK_CMD, stdout=subprocess.PIPE)
        p.communicate()
        return 0

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        p = subprocess.Popen(IPMI_WDT_OFF_CMD, stdout=subprocess.PIPE)
        p.communicate()
        return 0

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        p = subprocess.Popen(IPMI_WDT_EN_KICK_CMD, stdout=subprocess.PIPE)
        p.communicate()
        return 0

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        ipmi_timeout = seconds * 10;
        cmd = ["ipmitool", "raw", "0x6", "0x24", "0x4", "0x0", "0x0", "0x0"]
        cmd.append(str(ipmi_timeout % 256))
        cmd.append(str(int(ipmi_timeout / 256)))
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        p.communicate()

        return seconds

    def _gettimeout(self):
        #IPMI_WDT_GET_TIMEOUT_CMD
        out, err = self.pipeline("ipmitool mc watchdog get", "grep Present", "awk '{print $3}'")
        return int(float(out.decode().rstrip('\n')))

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

        if seconds < 0 or seconds > 500:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)

            if self.armed:
                self._keepalive()
            else:
                self._enable()

            ret = self.timeout
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
            return self._gettimeout()

        return timeleft
