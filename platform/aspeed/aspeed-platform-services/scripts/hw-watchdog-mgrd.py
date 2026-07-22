#!/usr/bin/env python3
"""
Hardware watchdog manager daemon for Aspeed platforms.

The Linux watchdog framework allows only a single open of /dev/watchdog0 at a
time.  This daemon is the sole owner of the device: it opens it, pets it
periodically while armed, and exposes an IPC interface over a Unix domain socket
so that the SONiC watchdogutil platform API (sonic_platform/watchdog.py) can
arm, disarm and query the watchdog without contending for the device file.

Boot-time arming policy is driven by platform.json (the "watchdog" section).
On a fresh boot the "boot_arm" flag decides whether the daemon arms the
watchdog, which lets u-boot/ABR hand off a running hardware timer that the
daemon then adopts.  Runtime arm/disarm requests (via watchdogutil) are recorded
in a small JSON "intent" file on tmpfs (/run) so they survive a daemon
restart/crash within a boot session; because /run is wiped on reboot, boot_arm
is authoritative again on the next boot.

The "shutdown_protect" flag arms a second behaviour: during a system
shutdown/reboot the daemon keeps the watchdog armed (closing the device without
the magic 'V' character) so the SoC is reset if the reboot path hangs.  On a
normal daemon stop/restart it instead disarms via magic-close so a stopped
daemon never leaves an unpetted watchdog.
"""

import array
import fcntl
import json
import os
import select
import signal
import socket
import subprocess
import sys
import syslog
import time

try:
    from sonic_py_common.device_info import get_platform_json_data
except Exception:  # pragma: no cover - host may lack sonic_py_common early
    get_platform_json_data = None

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

# Runtime arming intent.  JSON: {"armed": bool, "timeout": int}.  Written on
# arm/disarm and read on startup.  Lives on tmpfs (/run) so it survives a daemon
# restart/crash within a boot session but is wiped on reboot; this keeps the
# platform.json "boot_arm" policy authoritative on every fresh boot while still
# letting a runtime watchdogutil arm/disarm survive a daemon bounce.
INTENT_FILE = "/run/hw-watchdog-mgrd.json"

# Syslog identity/facility.  Logs are emitted via syslog so that the standard
# rsyslog/logrotate/log-export infrastructure handles persistence and rotation.
# This must match the $programname filter in the rsyslog drop-in
# (etc/rsyslog.d/10-hw-watchdog-mgrd.conf).
SYSLOG_IDENT = "hw-watchdog-mgrd"
SYSLOG_FACILITY = syslog.LOG_DAEMON

KEEPALIVE_INTERVAL = 60        # seconds between pings
DEFAULT_TIMEOUT = 180          # default hw timeout (seconds)
# Minimum armable timeout.  The Linux watchdog core treats a timeout of 0 as
# "unknown" (WDIOC_GETTIMEOUT returns -EOPNOTSUPP) rather than "disabled", and
# the Aspeed driver programs a reload value of 0 for it, which resets the SoC
# almost immediately.  Very small timeouts are likewise unsafe once IPC and
# keepalive latency are accounted for.  Floor requests at 30 s, matching the
# Aspeed driver's WDT_DEFAULT_TIMEOUT and staying well above KEEPALIVE_INTERVAL.
MIN_TIMEOUT = 30
MAX_TIMEOUT = 300
KEEPALIVE_LOG_INTERVAL = 3600  # log heartbeat status hourly


class WatchdogManager:
    """Owns the hardware watchdog and serves watchdog requests over IPC."""

    def __init__(self):
        self.fd = None
        self.armed = False
        self.timeout = 0
        self.last_ping = 0.0
        # Keepalive cadence.  pet_interval is derived from the armed timeout so
        # the daemon always pets well inside the hardware window (see
        # _pet_interval_for); next_ping is the monotonic deadline for the next
        # keepalive and is rescheduled on every arm.
        self.pet_interval = KEEPALIVE_INTERVAL
        self.next_ping = 0.0
        # Set whenever the watchdog is (re-)armed so the maintenance loop logs
        # the first keepalive it sends, confirming the petting loop is alive.
        self.first_pet_pending = False
        # Platform policy flags, cached once at startup from platform.json.
        # Default on: arm the watchdog at boot and keep it armed across a
        # system shutdown so the box is protected unless platform.json (or a
        # missing/unreadable config) explicitly opts out.
        self.boot_arm = True
        self.shutdown_protect = True
        # Set by the signal handler so the teardown runs in the main loop
        # (normal context) rather than the signal handler itself.  The wakeup
        # pipe lets a signal break the select() promptly.
        self._stop_requested = False
        self._wakeup_r = None
        self._wakeup_w = None

    # ---------------------------------------------------------------- logging
    def log(self, msg, level=syslog.LOG_INFO):
        syslog.syslog(level, msg)

    # ------------------------------------------------------- platform policy
    def _load_platform_config(self):
        # Read the "watchdog" policy from platform.json once at startup and
        # cache it.  Fail safe: if sonic_py_common is unavailable or the file
        # cannot be read/parsed, keep the protective defaults set in __init__
        # (boot arming on, shutdown protection on).
        if get_platform_json_data is None:
            self.log("sonic_py_common unavailable; using default watchdog "
                     "policy (boot_arm on, shutdown_protect on)",
                     syslog.LOG_WARNING)
            return
        try:
            data = get_platform_json_data() or {}
            wd = data.get("watchdog", {}) or {}
            self.boot_arm = bool(wd.get("boot_arm", True))
            self.shutdown_protect = bool(wd.get("shutdown_protect", True))
        except Exception as e:  # pragma: no cover - defensive
            self.log("failed to load platform watchdog policy: %s" % e,
                     syslog.LOG_ERR)
            return
        self.log("Loaded watchdog policy: boot_arm=%s shutdown_protect=%s"
                 % (self.boot_arm, self.shutdown_protect))

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
    def _pet_interval_for(self, timeout):
        # Keep the keepalive interval comfortably inside the hardware timeout so
        # a single late/missed pet cannot expire the watchdog: pet at half the
        # (hardware-accepted) timeout, capped at KEEPALIVE_INTERVAL for long
        # timeouts.  Without this, a caller-requested timeout shorter than
        # KEEPALIVE_INTERVAL would expire before the next 60 s pet and reset the
        # box.  The floor is half the minimum armable timeout (MIN_TIMEOUT),
        # since arm() never accepts anything smaller.
        return max(MIN_TIMEOUT // 2, min(KEEPALIVE_INTERVAL, timeout // 2))

    def arm(self, seconds):
        if seconds < MIN_TIMEOUT or seconds > MAX_TIMEOUT:
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
            # Recompute the pet cadence from the accepted hardware timeout and
            # reschedule so the next keepalive lands inside the new window.
            self.pet_interval = self._pet_interval_for(self.timeout)
            self.next_ping = self.last_ping + self.pet_interval
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
            # Read timeout matches the client SOCKET_TIMEOUT in
            # sonic_platform/watchdog.py.
            conn.settimeout(5.0)
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
    def _system_is_stopping(self):
        # True while systemd is tearing the system down (shutdown/reboot).  Used
        # to distinguish a system shutdown from an ordinary daemon stop/restart.
        try:
            out = subprocess.run(["systemctl", "is-system-running"],
                                 capture_output=True, text=True, timeout=5)
            return out.stdout.strip() == "stopping"
        except Exception:
            return False

    def _request_stop(self, signum, frame):
        # Signal-context safe: only flag the request.  set_wakeup_fd() writes a
        # byte to the wakeup pipe, which breaks the main loop's select() so the
        # real teardown (cleanup()) runs promptly in normal context.
        self._stop_requested = True

    def cleanup(self):
        self.log("Hardware watchdog manager stopping")
        if self.fd is not None:
            try:
                if (self.armed and self.shutdown_protect
                        and self._system_is_stopping()):
                    # System shutdown/reboot in progress: keep the watchdog
                    # armed so the SoC is reset if the reboot path hangs.  Pet
                    # once for a full timeout window, then close WITHOUT the
                    # magic 'V' so the kernel leaves the hardware running.  Since
                    # the requested timeout fits the hardware's max heartbeat and
                    # WDOG_ACTIVE stays set, the kernel worker is not scheduled
                    # (need_worker is false): nothing pets it, so it counts down.
                    self._keepalive()
                    os.close(self.fd)
                    self.log("Shutdown in progress; left hardware watchdog "
                             "armed (timeout %d s) for reboot protection"
                             % self.timeout)
                else:
                    # Normal stop/restart: disarm via magic-close so a stopped
                    # daemon never leaves an unpetted watchdog that would reset
                    # the box.  The intent file is left untouched (it lives on
                    # tmpfs and is wiped on reboot anyway).
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
        # Decide whether to arm at startup.  The intent file lives on tmpfs, so
        # its presence means a runtime arm/disarm happened earlier in THIS boot
        # session (it survives a daemon restart but not a reboot).  We always
        # write the intent BEFORE touching the hardware (see arm()/disarm()), so
        # the intent is the source of truth and is honoured first.
        intent_present = os.path.exists(INTENT_FILE)
        intent_armed, intent_timeout = self._read_intent()

        if intent_present:
            if intent_armed:
                self.log("Restoring armed watchdog from intent (timeout %d s)"
                         % intent_timeout)
                if self.arm(intent_timeout) < 0:
                    self.log("Failed to arm hardware watchdog at startup",
                             syslog.LOG_ERR)
            else:
                # Honour a disarm intent.  This also completes a disarm that was
                # interrupted by a crash between writing the intent and disabling
                # the hardware: if the hardware is still ACTIVE, disarm() stops
                # it rather than leaving it for us to re-adopt.
                self.log("Intent is disarmed; ensuring hardware watchdog is off")
                self.disarm()
            return

        # No intent recorded this boot.  An ACTIVE hardware watchdog here can
        # only come from a previous daemon that armed it and died WITHOUT
        # recording intent (an intent-write failure, or the file was removed);
        # a u-boot/ABR-armed timer is HW_RUNNING but not ACTIVE, so it does not
        # show up as active.  The kernel does not pet an ACTIVE watchdog once its
        # opener is gone, so we must adopt it now to avoid an unpetted reset;
        # arm() re-creates the intent file.
        if self._hw_is_armed():
            timeout = self._read_sysfs_int("timeout")
            if timeout <= 0:
                timeout = DEFAULT_TIMEOUT
            self.log("Adopting active hardware watchdog with no recorded intent "
                     "(timeout %d s)" % timeout)
            if self.arm(timeout) < 0:
                self.log("Failed to adopt hardware watchdog at startup",
                         syslog.LOG_ERR)
            return

        # Fresh boot: platform policy (boot_arm) is the source of truth.
        # boot_arm should mirror what the bootloader does -- if u-boot/ABR arms
        # the watchdog, set boot_arm=true so the daemon adopts that running
        # timer.  On a MISMATCH (boot_arm=false but the bootloader DID arm
        # u-boot/ABR) the hardware is HW_RUNNING but not ACTIVE (we never opened
        # it), so the kernel's petting thread is feeding it; we honour the
        # policy and actively disarm() so a box configured boot_arm=false is
        # genuinely left with the watchdog off rather than silently armed.
        if self.boot_arm:
            self.log("Arming hardware watchdog at boot per platform policy "
                     "(boot_arm, timeout %d s)" % DEFAULT_TIMEOUT)
            if self.arm(DEFAULT_TIMEOUT) < 0:
                self.log("Failed to arm hardware watchdog at boot",
                         syslog.LOG_ERR)
        else:
            self.log("Boot-arm disabled by platform policy; ensuring hardware "
                     "watchdog is off")
            self.disarm()

    def run(self):
        syslog.openlog(ident=SYSLOG_IDENT, logoption=syslog.LOG_PID,
                       facility=SYSLOG_FACILITY)
        self.log("Hardware watchdog manager starting")
        # Route SIGTERM/SIGINT through a wakeup pipe: the handler only sets a
        # flag, and the byte written by set_wakeup_fd() breaks select() so the
        # teardown runs in the main loop (normal context).  This keeps fork/exec
        # (the systemctl shutdown check) out of the signal handler.
        self._wakeup_r, self._wakeup_w = os.pipe()
        os.set_blocking(self._wakeup_w, False)
        signal.set_wakeup_fd(self._wakeup_w)
        signal.signal(signal.SIGTERM, self._request_stop)
        signal.signal(signal.SIGINT, self._request_stop)

        self._load_platform_config()
        self._startup_arm()

        server = self._create_socket()
        keepalive_count = 0
        log_threshold = max(1, KEEPALIVE_LOG_INTERVAL // KEEPALIVE_INTERVAL)
        # _startup_arm() may already have scheduled next_ping via arm(); only
        # seed it here if nothing armed the watchdog at startup.
        if self.next_ping <= 0:
            self.next_ping = time.monotonic() + self.pet_interval

        while True:
            if self._stop_requested:
                self.cleanup()
            wait = max(0, self.next_ping - time.monotonic())
            try:
                readable, _, _ = select.select(
                    [server, self._wakeup_r], [], [], wait)
            except InterruptedError:
                continue

            if self._stop_requested:
                self.cleanup()

            # Pet first: keeping the hardware watchdog alive is the
            # safety-critical task, so it takes priority over serving requests.
            now = time.monotonic()
            if now >= self.next_ping:
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
                self.next_ping = now + self.pet_interval

            for sock in readable:
                if sock == self._wakeup_r:
                    # Drain the signal wakeup byte(s); the stop flag is checked
                    # at the top of the loop.
                    try:
                        os.read(self._wakeup_r, 4096)
                    except OSError:
                        pass
                    continue
                self._handle_connection(sock)


if __name__ == "__main__":
    WatchdogManager().run()
