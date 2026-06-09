#!/usr/bin/env python3
"""
Hardware watchdog manager daemon for Aspeed platforms.

The Linux watchdog framework allows only a single open of /dev/watchdog0 at a
time.  This daemon is the sole owner of the device: it opens it, pets it
periodically while armed, and exposes an IPC interface over a Unix domain socket
so that the SONiC watchdogutil platform API (sonic_platform/watchdog.py) can
arm, disarm and query the watchdog without contending for the device file.

Arming policy is decoupled from the daemon.  Whether the watchdog should be
armed is recorded in a small JSON "intent" file; the daemon does not arm
unconditionally at boot.  On startup it arms iff the persisted intent says so or
the hardware is already counting (e.g. the daemon crashed and was restarted
while the watchdog was live).  This keeps watchdogutil the driver of policy and
makes the arm state survive daemon restarts and upgrades correctly.
"""

import array
import fcntl
import json
import os
import select
import signal
import socket
import sys
import syslog
import time

# Watchdog ioctl commands (from Linux kernel watchdog.h)
WDIOC_SETOPTIONS = 0x80045704
WDIOC_KEEPALIVE = 0x80045705
WDIOC_SETTIMEOUT = 0xc0045706

WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002

WATCHDOG_DEVICE = "/dev/watchdog0"
WATCHDOG_SYSFS_PATH = "/sys/class/watchdog/watchdog0/"

# IPC socket shared with the platform API (sonic_platform/watchdog.py).
SOCKET_PATH = "/run/hw-watchdog-mgrd.sock"

# Persisted arming policy.  JSON: {"armed": bool, "timeout": int}.  Written on
# arm/disarm and read on startup.  The installer may seed this file to arm the
# watchdog by default on first boot.  Lives on persistent storage so the intent
# survives reboots and upgrades.
INTENT_FILE = "/host/bmc/hw-watchdog-mgrd.json"

# Syslog identity/facility.  Logs are emitted via syslog so that the standard
# rsyslog/logrotate/log-export infrastructure handles persistence and rotation.
# This must match the $programname filter in the rsyslog drop-in
# (etc/rsyslog.d/10-hw-watchdog-mgrd.conf).
SYSLOG_IDENT = "hw-watchdog-mgrd"
SYSLOG_FACILITY = syslog.LOG_DAEMON

KEEPALIVE_INTERVAL = 60        # seconds between pings
DEFAULT_TIMEOUT = 180          # default hw timeout (seconds)
MAX_TIMEOUT = 300
KEEPALIVE_LOG_INTERVAL = 3600  # log heartbeat status hourly


class WatchdogManager:
    """Owns the hardware watchdog and serves watchdog requests over IPC."""

    def __init__(self):
        self.fd = None
        self.armed = False
        self.timeout = 0
        self.last_ping = 0.0
        # Set whenever the watchdog is (re-)armed so the maintenance loop logs
        # the first keepalive it sends, confirming the petting loop is alive.
        self.first_pet_pending = False

    # ---------------------------------------------------------------- logging
    def log(self, msg, level=syslog.LOG_INFO):
        syslog.syslog(level, msg)

    # ----------------------------------------------------------- device access
    def _open_device(self):
        if self.fd is None:
            try:
                self.fd = os.open(WATCHDOG_DEVICE, os.O_WRONLY)
            except OSError:
                return False
        return True

    def _read_sysfs_str(self, filename):
        try:
            with open(os.path.join(WATCHDOG_SYSFS_PATH, filename)) as f:
                return f.read().strip()
        except OSError:
            return ""

    def _read_sysfs_int(self, filename):
        try:
            return int(self._read_sysfs_str(filename).split()[0])
        except (ValueError, IndexError):
            return -1

    def _hw_is_armed(self):
        return self._read_sysfs_str("state") == "active"

    def _settimeout(self, seconds):
        req = array.array('I', [seconds])
        fcntl.ioctl(self.fd, WDIOC_SETTIMEOUT, req, True)
        return int(req[0])

    def _enable(self):
        req = array.array('I', [WDIOS_ENABLECARD])
        fcntl.ioctl(self.fd, WDIOC_SETOPTIONS, req, False)

    def _disable(self):
        req = array.array('I', [WDIOS_DISABLECARD])
        fcntl.ioctl(self.fd, WDIOC_SETOPTIONS, req, False)

    def _keepalive(self):
        fcntl.ioctl(self.fd, WDIOC_KEEPALIVE)

    # ------------------------------------------------------------ intent file
    def _read_intent(self):
        try:
            with open(INTENT_FILE) as f:
                data = json.load(f)
            return (bool(data.get("armed", False)),
                    int(data.get("timeout", DEFAULT_TIMEOUT)))
        except (OSError, ValueError, TypeError):
            return False, DEFAULT_TIMEOUT

    def _write_intent(self, armed, timeout):
        data = {"armed": bool(armed), "timeout": int(timeout)}
        try:
            os.makedirs(os.path.dirname(INTENT_FILE), exist_ok=True)
            tmp = INTENT_FILE + ".tmp"
            with open(tmp, "w") as f:
                json.dump(data, f)
            os.replace(tmp, INTENT_FILE)
        except OSError as e:
            self.log("failed to persist intent: %s" % e, syslog.LOG_ERR)

    # ---------------------------------------------------------- watchdog logic
    def arm(self, seconds):
        if seconds < 0 or seconds > MAX_TIMEOUT:
            return -1
        # Persist the intent before touching hardware so the intent file always
        # wins over hardware state: if we crash between here and the ioctls, the
        # next startup re-applies the intent rather than inheriting a stale or
        # partially-applied hardware state.
        self._write_intent(True, seconds)
        if not self._open_device():
            return -1
        was_armed = self.armed
        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self._hw_is_armed():
                self._keepalive()
            else:
                self._enable()
            self.armed = True
            self.last_ping = time.monotonic()
            self.first_pet_pending = True
        except OSError:
            return -1
        self.log("Hardware watchdog %s (timeout %d s)"
                 % ("re-armed" if was_armed else "armed", self.timeout))
        return self.timeout

    def disarm(self):
        # Persist the intent before touching hardware (see arm()): a crash after
        # this point still leaves the system intending to be disarmed.
        prev_timeout = self.timeout
        self._write_intent(False, prev_timeout)
        if not self._open_device():
            return False
        try:
            self._disable()
            self.timeout = 0
            self.armed = False
            self.first_pet_pending = False
        except OSError:
            return False
        self.log("Hardware watchdog disarmed")
        return True

    def get_remaining_time(self):
        if not self.armed or self.timeout <= 0:
            return -1
        remaining = int(self.timeout - (time.monotonic() - self.last_ping))
        return remaining if remaining > 0 else 0

    # -------------------------------------------------------------------- IPC
    def handle_request(self, req):
        cmd = req.get("cmd")
        if cmd == "arm":
            return {"result": self.arm(int(req.get("seconds", DEFAULT_TIMEOUT)))}
        if cmd == "disarm":
            return {"result": self.disarm()}
        if cmd == "is_armed":
            return {"result": self._hw_is_armed()}
        if cmd == "get_remaining_time":
            return {"result": self.get_remaining_time()}
        if cmd == "get_timeout":
            return {"result": self.timeout}
        return {"error": "unknown command: %s" % cmd}

    def _create_socket(self):
        try:
            os.unlink(SOCKET_PATH)
        except OSError:
            if os.path.exists(SOCKET_PATH):
                raise
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server.bind(SOCKET_PATH)
        os.chmod(SOCKET_PATH, 0o600)
        server.listen(5)
        return server

    def _handle_connection(self, server):
        try:
            conn, _ = server.accept()
        except OSError:
            return
        try:
            conn.settimeout(2.0)
            data = conn.recv(4096)
            if not data:
                return
            try:
                req = json.loads(data.decode().strip())
                resp = self.handle_request(req)
            except ValueError:
                resp = {"error": "invalid request"}
            conn.sendall((json.dumps(resp) + "\n").encode())
        except OSError:
            pass
        finally:
            conn.close()

    # ------------------------------------------------------------------- main
    def cleanup(self, signum=None, frame=None):
        # Disarm the hardware on exit (magic-close 'V') so a stopped daemon
        # never leaves an unpetted watchdog that would reset the box.  The
        # intent file is intentionally left untouched: a deliberate disarm has
        # already cleared it, while a service restart/upgrade must re-arm.
        self.log("Hardware watchdog manager stopping")
        if self.fd is not None:
            try:
                os.write(self.fd, b'V')
                os.close(self.fd)
            except OSError:
                pass
            self.fd = None
        try:
            os.unlink(SOCKET_PATH)
        except OSError:
            pass
        self.log("Hardware watchdog manager stopped")
        sys.exit(0)

    def _startup_arm(self):
        # Decide whether to arm at startup: honour the persisted intent, and
        # adopt an already-running hardware watchdog (crash recovery).
        intent_present = os.path.exists(INTENT_FILE)
        intent_armed, intent_timeout = self._read_intent()
        hw_armed = self._hw_is_armed()
        if not (intent_armed or hw_armed):
            # Nothing to arm.  Materialise the intent file from the current
            # hardware status when it does not exist yet (e.g. first boot), so
            # the on-disk intent always reflects reality.  The armed/adoption
            # branches below persist via arm().
            #
            # While disarmed there is no meaningful running timeout to record,
            # so use DEFAULT_TIMEOUT as a placeholder: it matches what
            # _read_intent() returns for an absent file, so the materialised
            # file reads back identically to the previous no-file state.
            if not intent_present:
                self._write_intent(False, DEFAULT_TIMEOUT)
            self.log("Hardware watchdog idle (not armed); awaiting arm request")
            return
        timeout = intent_timeout
        if hw_armed and not intent_armed:
            sysfs_timeout = self._read_sysfs_int("timeout")
            if sysfs_timeout > 0:
                timeout = sysfs_timeout
            self.log("Adopting already-running hardware watchdog at startup "
                     "(crash recovery, timeout %d s)" % timeout)
        else:
            self.log("Restoring armed watchdog from persisted intent "
                     "(timeout %d s)" % timeout)
        # arm() logs the actual arm event.
        if self.arm(timeout) < 0:
            self.log("Failed to arm hardware watchdog at startup",
                     syslog.LOG_ERR)

    def run(self):
        syslog.openlog(ident=SYSLOG_IDENT, logoption=syslog.LOG_PID,
                       facility=SYSLOG_FACILITY)
        self.log("Hardware watchdog manager starting")
        signal.signal(signal.SIGTERM, self.cleanup)
        signal.signal(signal.SIGINT, self.cleanup)

        self._startup_arm()

        server = self._create_socket()
        keepalive_count = 0
        log_threshold = max(1, KEEPALIVE_LOG_INTERVAL // KEEPALIVE_INTERVAL)
        next_ping = time.monotonic() + KEEPALIVE_INTERVAL

        while True:
            wait = max(0, next_ping - time.monotonic())
            try:
                readable, _, _ = select.select([server], [], [], wait)
            except InterruptedError:
                continue

            # Pet first: keeping the hardware watchdog alive is the
            # safety-critical task, so it takes priority over serving requests.
            now = time.monotonic()
            if now >= next_ping:
                if self.armed:
                    try:
                        self._keepalive()
                        self.last_ping = now
                        keepalive_count += 1
                        if self.first_pet_pending:
                            self.log("Hardware watchdog first keepalive sent "
                                     "(timeout %d s)" % self.timeout)
                            self.first_pet_pending = False
                        if keepalive_count % log_threshold == 0:
                            self.log("Hardware watchdog keepalive active (sent "
                                     "%d keepalives)" % keepalive_count)
                    except OSError as e:
                        self.log("keepalive failed: %s" % e, syslog.LOG_ERR)
                next_ping = now + KEEPALIVE_INTERVAL

            for sock in readable:
                self._handle_connection(sock)


if __name__ == "__main__":
    WatchdogManager().run()
