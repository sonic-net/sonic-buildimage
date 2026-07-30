"""
SONiC Platform API - Watchdog class for Aspeed BMC

This module provides the Watchdog class for the Aspeed AST2700 BMC platform.

The hardware watchdog device (/dev/watchdog0) can only be opened by a single
process at a time.  On this platform the hw-watchdog-mgrd daemon owns the
device and pets it periodically.  This class therefore does not access the
device directly; instead it talks to the daemon over a Unix domain socket so
that watchdogutil arm/disarm/status work while the daemon keeps the watchdog
alive.
"""

import json
import os
import socket
import syslog

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


# IPC socket served by the hw-watchdog-mgrd daemon.  This must match
# SOCKET_PATH in
# platform/aspeed/aspeed-platform-services/scripts/hw-watchdog-mgrd.py
SOCKET_PATH = "/run/hw-watchdog-mgrd.sock"
SOCKET_TIMEOUT = 5  # seconds

# Read-only sysfs view of the hardware watchdog state.  Used only as a fallback
# for is_armed() when the daemon is unreachable; reading sysfs does not open
# /dev/watchdog0 and so does not contend with the daemon.
WATCHDOG_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"


class Watchdog(WatchdogBase):
    """
    Watchdog class for the Aspeed AST2700 BMC platform.

    Acts as an IPC client to the hw-watchdog-mgrd daemon, which is the sole
    owner of the hardware watchdog device.
    """

    def __init__(self):
        """
        Initialize the Watchdog object
        """
        self.socket_path = SOCKET_PATH

    def _request(self, cmd, **kwargs):
        """
        Send a request to the hw-watchdog-mgrd daemon and return the response dict.

        Returns None on any communication failure.
        """
        req = {"cmd": cmd}
        req.update(kwargs)
        try:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(SOCKET_TIMEOUT)
            sock.connect(self.socket_path)
            sock.sendall((json.dumps(req) + "\n").encode())
            data = sock.recv(4096)
            sock.close()
        except (OSError, socket.timeout) as e:
            self._log_daemon_unreachable(e)
            return None
        if not data:
            return None
        try:
            return json.loads(data.decode().strip())
        except ValueError:
            return None

    def _log_daemon_unreachable(self, err):
        """Emit a clear diagnostic when the hw-watchdog-mgrd daemon cannot be reached."""
        syslog.syslog(
            syslog.LOG_WARNING,
            "watchdog: hw-watchdog-mgrd daemon not reachable at %s (%s); "
            "is the hw-watchdog-mgrd service running?"
            % (self.socket_path, err))

    def _sysfs_is_armed(self):
        """
        Read the hardware watchdog state directly from sysfs.

        Fallback for is_armed() when the daemon is unreachable so that status
        still reflects the hardware (e.g. the daemon crashed while the watchdog
        was armed and is now counting down to a reset).

        This fallback is meaningful because of how the aspeed_wdt driver
        behaves (drivers/watchdog/aspeed_wdt.c):
          * It advertises WDIOF_MAGICCLOSE and the kernel runs with nowayout=0,
            so a clean stop needs the magic 'V' close (which the daemon writes
            on SIGTERM) but an *unclean* close (a daemon crash) leaves the
            watchdog running and counting down.
          * Its max_hw_heartbeat_ms (~71 min) is far larger than the timeouts
            we use, so the watchdog core does not start its own keepalive
            worker; after a crash nothing re-pets the device and it really does
            reset the box.
        In that crash window /sys/class/watchdog/watchdog0/state still reads
        "active", so this is the only way is_armed() can report the truth
        instead of a dangerous False until systemd restarts the daemon (which
        then re-adopts the live watchdog).  Reading sysfs does not open
        /dev/watchdog0, so it never contends with the daemon.
        """
        try:
            with open(os.path.join(WATCHDOG_SYSFS_PATH, "state")) as f:
                return f.read().strip() == "active"
        except OSError:
            return False

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        resp = self._request("is_armed")
        if resp is None:
            # Daemon unreachable; fall back to the read-only sysfs state.
            return self._sysfs_is_armed()
        return bool(resp.get("result", False))

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds

        Args:
            seconds: Timeout value in seconds

        Returns:
            An integer specifying the actual number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        resp = self._request("arm", seconds=seconds)
        if resp is None or "result" not in resp:
            return -1
        return int(resp["result"])

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        resp = self._request("disarm")
        if resp is None:
            return False
        return bool(resp.get("result", False))

    def get_remaining_time(self):
        """
        Get the number of seconds remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on the
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        resp = self._request("get_remaining_time")
        if resp is None or "result" not in resp:
            return -1
        return int(resp["result"])

