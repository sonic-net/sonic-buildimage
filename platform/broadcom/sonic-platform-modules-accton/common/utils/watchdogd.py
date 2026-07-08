#!/usr/bin/env python3
#
# Copyright (C) 2024 Accton Networks, Inc.
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

import socket
import os
import fcntl
import array
import threading
import time
import json
import logging
from logging.handlers import SysLogHandler
import signal
import argparse

logger = None

# ioctl constants (used to construct ioctl request codes)
IO_WRITE        = 0x40000000        # For write-only ioctl (not used here)
IO_READ         = 0x80000000        # For read-only ioctl
IO_READ_WRITE   = 0xC0000000        # For read/write ioctl
IO_SIZE_INT     = 0x00040000        # Data size = sizeof(int)
IO_SIZE_40      = 0x00280000        # Data size = 0x28 (40 bytes, not used here)
IO_TYPE_WATCHDOG = ord('W') << 8    # Watchdog magic number (0x5700)

# Composite ioctl codes used for specific watchdog calls
WDR_INT   = IO_READ       | IO_SIZE_INT | IO_TYPE_WATCHDOG  # Read int (e.g. status, timeout, etc.)
WDR_40    = IO_READ       | IO_SIZE_40  | IO_TYPE_WATCHDOG  # Read 40-byte struct (not used here)
WDWR_INT  = IO_READ_WRITE | IO_SIZE_INT | IO_TYPE_WATCHDOG  # Read/write int (e.g. set timeout)

# Watchdog ioctl commands
WDIOC_GETSUPPORT      = 0  | WDR_40   # Not used
WDIOC_GETSTATUS       = 1  | WDR_INT  # Not used
WDIOC_GETBOOTSTATUS   = 2  | WDR_INT  # Not used
WDIOC_GETTEMP         = 3  | WDR_INT  # Not used
WDIOC_SETOPTIONS      = 4  | WDR_INT  # Used by enable()/disable()
WDIOC_KEEPALIVE       = 5  | WDR_INT  # Used by keepalive()
WDIOC_SETTIMEOUT      = 6  | WDWR_INT # Used by settimeout()
WDIOC_GETTIMEOUT      = 7  | WDR_INT  # Not used
WDIOC_SETPRETIMEOUT   = 8  | WDWR_INT # Not used
WDIOC_GETPRETIMEOUT   = 9  | WDR_INT  # Not used
WDIOC_GETTIMELEFT     = 10 | WDR_INT  # Used by _get_remaining_time()

# Watchdog status constants (used with WDIOC_SETOPTIONS)
WDIOS_DISABLECARD = 0x0001  # Disable the watchdog timer
WDIOS_ENABLECARD  = 0x0002  # Enable the watchdog timer

DEFAULT_WATCHDOG_STATUS = True
DEFAULT_WATCHDOG_RESET_TIME_VAL = 180
WATCHDOG_KICK_INTERVAL = 60
MACHINE_CONF_FILE = "/host/machine.conf"
HOST_WATCHDOGD_SOCKET_FILE = "/usr/share/sonic/device/{}/watchdogd.socket"
HOST_WATCHDOGD_CONF_FILE = "/etc/sonic/watchdog.conf"

stop_event = threading.Event()

def log_print(message, should_print=True, log_level='INFO'):
    global logger

    log_func = getattr(logger, log_level.lower(), logger.info)
    log_func(message)
    if should_print:
        print(f"{log_level}: {message}")

class WatchdogConfig:
    def __init__(self, platform):
        self.file_path = HOST_WATCHDOGD_CONF_FILE.format(platform)
        self.config = {'enable': DEFAULT_WATCHDOG_STATUS, 'timeout': DEFAULT_WATCHDOG_RESET_TIME_VAL}
        self.read_conf_file()

    def read_conf_file(self):
        try:
            with open(self.file_path, "r") as file:
                self.config = json.load(file)
        except FileNotFoundError:
            log_print(f"File not found: {self.file_path}", should_print=False, log_level='INFO')
        except json.JSONDecodeError as e:
            log_print(f"Invalid JSON format in file: {self.file_path}, error: {e}", should_print=False, log_level='ERROR')

    def write_conf_file(self):
        try:
            with open(self.file_path, "w") as file:
                json.dump(self.config, file, indent=4)
                file.flush()
        except IOError as e:
            log_print(f"Error writing to {self.file_path}: {e}", should_print=False, log_level='ERROR')

class Watchdog:
    def __init__(self, fd, config):
        self.fd = fd
        self.config = config

    def enable(self):
        req = array.array('h', [WDIOS_ENABLECARD])
        try:
            fcntl.ioctl(self.fd, WDIOC_SETOPTIONS, req, False)
            self.config.config['enable'] = True
            return "0,Watchdog enabled."
        except Exception as e:
            log_print(f"Failed to enable watchdog: {e}", should_print=False, log_level='ERROR')
            return f"-1,Failed to enable watchdog: {e}"

    def disable(self):
        req = array.array('h', [WDIOS_DISABLECARD])
        try:
            fcntl.ioctl(self.fd, WDIOC_SETOPTIONS, req, False)
            self.config.config['enable'] = False
            return "0,Watchdog disabled."
        except Exception as e:
            log_print(f"Failed to disable watchdog: {e}", should_print=False, log_level='ERROR')
            return f"-1,Failed to disable watchdog: {e}"

    def settimeout(self, seconds):
        req = array.array('I', [int(seconds)])
        try:
            fcntl.ioctl(self.fd, WDIOC_SETTIMEOUT, req, True)
            self.config.config['timeout'] = int(seconds)
            return "0,{}".format(int(req[0]))
        except Exception as e:
            log_print(f"Failed to set timeout: {e}", should_print=False, log_level='ERROR')
            return f"-1,Failed to set timeout: {e}"

    def _get_remaining_time(self):
        try:
            req = array.array('I', [0])
            fcntl.ioctl(self.fd, WDIOC_GETTIMELEFT, req, True)
            return int(req[0])
        except Exception as e:
            log_print(f"Failed to get remaining time: {e}", should_print=False, log_level='ERROR')
            return -1

    def get_remaining_time(self):
        timeleft = self._get_remaining_time()
        if timeleft < 0:
            return f"-1,Failed to get remaining time"

        return "0,{}".format(timeleft)

    def keepalive(self):
        try:
            fcntl.ioctl(self.fd, WDIOC_KEEPALIVE)
            return "0,Watchdog keepalive."
        except Exception as e:
            log_print(f"Failed to keep alive: {e}", should_print=False, log_level='ERROR')
            return f"-1,Failed to keep alive: {e}"

    def is_armed(self):
        armed = 1 if self.config.config['enable'] is True else 0
        return f"0, {armed}"

def handle_command(raw_command, watchdog):
    parts = raw_command.split(',')
    command = parts[0]
    args = parts[1:]

    commands = {
        "enable": watchdog.enable,
        "disable": watchdog.disable,
        "settimeout": lambda: watchdog.settimeout(int(args[0])) if len(args) == 1 and args[0].isdigit() else "-1,Invalid arguments for settimeout.",
        "get_remaining_time": watchdog.get_remaining_time,
        "keepalive": watchdog.keepalive,
        "is_armed": watchdog.is_armed
    }

    func = commands.get(command)
    if func:
        try:
            return func()
        except Exception as e:
            log_print(f"Error executing command '{command}': {e}", should_print=False, log_level='ERROR')
            return f"-1,Error executing command '{command}': {e}"
    return "-1,Invalid command"

def watchdogd(watchdog):
    try:
        watchdog.settimeout(watchdog.config.config['timeout'])

        if watchdog.config.config['enable']:
            watchdog.enable()
        else:
            watchdog.disable()

        interval = 1.0 # Check every second
        while not stop_event.is_set():
            time.sleep(interval)

            if watchdog.config.config['enable']:
                timeleft = watchdog._get_remaining_time()
                elapsed = watchdog.config.config['timeout'] - timeleft

                if elapsed >= WATCHDOG_KICK_INTERVAL:
                    watchdog.keepalive()

    except KeyboardInterrupt:
        log_print("KeyboardInterrupt caught in thread, exiting now.")
    finally:
        log_print("Cleaning up thread...")

def setup_logger():
    global logger

    syslog_handler = SysLogHandler(facility=SysLogHandler.LOG_SYSLOG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    syslog_handler.setFormatter(formatter)

    logger = logging.getLogger('WatchdogDaemon')
    logger.setLevel(logging.INFO)
    logger.addHandler(syslog_handler)

def handle_signal(signum, frame):
    log_print(f"Received signal: {signum}")
    stop_event.set()

def get_platform():
    platform = None

    with open(MACHINE_CONF_FILE, 'r') as file:
        for line in file:
            if 'onie_platform=' in line:
                platform = line.strip().split('=')[1]
                break

    return platform

def start_watchdog_service():
    try:
        fd = os.open("/dev/watchdog0", os.O_RDWR)
    except Exception as e:
        log_print(f"Failed to open watchdog device: {e}", log_level='ERROR')
        return

    platform = get_platform()
    watchd_socket_path = HOST_WATCHDOGD_SOCKET_FILE.format(platform)
    if os.path.exists(watchd_socket_path):
        os.unlink(watchd_socket_path)

    config = WatchdogConfig(platform)
    watchdog = Watchdog(fd, config)

    thread = threading.Thread(target=watchdogd, args=(watchdog, ))
    thread.start()

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as server_socket:
        try:
            if os.path.exists(watchd_socket_path):
                os.unlink(watchd_socket_path)
            server_socket.bind(watchd_socket_path)
            server_socket.listen(1)

            log_print("Watchdog daemon is running...")

            while not stop_event.is_set():
                server_socket.settimeout(1.0)
                try:
                    conn, addr = server_socket.accept()
                    with conn:
                        while not stop_event.is_set():
                            data = conn.recv(1024)
                            if not data:
                                break
                            command = data.decode().strip()
                            response = handle_command(command, watchdog)
                            conn.sendall(response.encode())
                except socket.timeout:
                    continue
        except KeyboardInterrupt:
            log_print("Ctrl+C pressed in main program")
        except Exception as e:
            log_print(f"An error occurred: {e}", should_print=False, log_level='ERROR')
        finally:
            stop_event.set()
            thread.join()
            os.close(fd)
            if os.path.exists(watchd_socket_path):
                os.unlink(watchd_socket_path)
            log_print("Final cleanup if necessary")

def main():
    signal.signal(signal.SIGTERM, handle_signal)
    setup_logger()

    parser = argparse.ArgumentParser(description="Control the Watchdog service")
    parser.add_argument('--enable', action='store_true', help="Enable watchdog at startup")
    parser.add_argument('--disable', action='store_true', help="Disable watchdog at startup")
    parser.add_argument('--timeout', type=int, help="Set the watchdog timeout in seconds")
    args = parser.parse_args()

    # Start watchdog service if no arguments were passed
    if not any(vars(args).values()):
        start_watchdog_service()
    else:
        config = WatchdogConfig(get_platform())

        if args.enable:
            config.config['enable'] = True
        elif args.disable:
            config.config['enable'] = False

        # Set the watchdog timeout if provided
        if args.timeout is not None:
            config.config['timeout'] = args.timeout

        config.write_conf_file()
        log_print("Watchdog config saved")


if __name__ == '__main__':
    main()
