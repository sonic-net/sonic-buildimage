#!/usr/bin/env python3

#############################################################################
#
# Watchdog contains an implementation of SONiC Platform Base Watchdog API
#
#############################################################################
import os
import sys
import socket
import struct

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MACHINE_CONF_FILE = "/host/machine.conf"
HOST_WATCHDOGD_SOCKET_FILE = "/usr/share/sonic/device/{}/watchdogd.socket"
PMON_WATCHDOGD_SOCKET_FILE = "/usr/share/sonic/platform/watchdogd.socket"
MIN_WATCHDOG_TIMER_RESET_VAL = 70
MAX_WATCHDOG_TIMER_RESET_VAL = 180
WDT_COMMON_ERROR = -1
watchdog = 0

class Watchdog(WatchdogBase):
    def __init__(self):
        self._api_helper = APIHelper()
        self.timeout = MAX_WATCHDOG_TIMER_RESET_VAL
        self.watchd_socket_path = PMON_WATCHDOGD_SOCKET_FILE
        if self._api_helper.is_host():
            platform = "None"
            try:
                with open(MACHINE_CONF_FILE, 'r') as file:
                    for line in file:
                        if 'onie_platform=' in line:
                            platform = line.strip().split('=')[1]
                            break
            except IOError as e:
                print(f"Error reading machine config file: {e}")
            self.watchd_socket_path = HOST_WATCHDOGD_SOCKET_FILE.format(platform)


    def _send_command(self, command):
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client_socket:
                client_socket.connect(self.watchd_socket_path)
                client_socket.sendall(command.encode())
                response = client_socket.recv(1024)
                response_code, response_message = response.decode().split(",", 1)

                return int(response_code), response_message
        except Exception as e:
            print(f"Error communicating with watchdog daemon: {e}")
            return WDT_COMMON_ERROR, str(e)

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
        if (seconds < MIN_WATCHDOG_TIMER_RESET_VAL or seconds > MAX_WATCHDOG_TIMER_RESET_VAL):
            return ret

        try:
            status, output = self._send_command("settimeout,{}".format(seconds))
            if status != 0:
                return ret
            self.timeout = int(output)

            if not self.is_armed():
                status, output = self._send_command("enable")
                if status != 0:
                    return ret

            ret = self.timeout
        except IOError:
            pass

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        disarmed = False
        try:
            status, output = self._send_command('disable')
            if status == 0:
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
        status, output = self._send_command('is_armed')
        return status == 0 and int(output) == 1


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
            status, output = self._send_command('get_remaining_time')
            if status == 0:
                timeleft = int(output)

        return timeleft

