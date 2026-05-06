#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Gate process: runs phcsync.sh as a child and pauses / resumes when warm reboot is detected.
# Uses swsscommon RestartWaiter (event-driven, no DB polling).
#

import os
import signal
import sys
import syslog
import subprocess

try:
    from swsscommon.swsscommon import RestartWaiter
except ImportError:
    syslog.syslog(syslog.LOG_ERR, "phcsync_warm_reboot_gate: swsscommon not available")
    sys.exit(1)

PHCSYNC_SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "phcsync.sh")
# 24h timeout match RestartWaiter default, omitting this would yield the same behavior.
# Defined here for readability.
WARM_BOOT_STARTED_TIMEOUT_SEC = 86400  # 24h
WARM_BOOT_DONE_TIMEOUT_SEC = 240 # 4m

_child_proc = None


def log(msg, level=syslog.LOG_INFO):
    syslog.syslog(level, msg)


def shutdown(signum, frame):
    global _child_proc
    if _child_proc is not None and _child_proc.pid is not None:
        try:
            _child_proc.terminate()
            _child_proc.wait(timeout=5)
        except (ProcessLookupError, subprocess.TimeoutExpired):
            try:
                _child_proc.kill()
            except ProcessLookupError:
                pass

    # Exit with 128 + signal number to indicate this program terminated due to a signal (SIGINT/SIGTERM).
    sys.exit(128 + (signum if signum in (signal.SIGINT, signal.SIGTERM) else 0))


def main():
    global _child_proc
    syslog.openlog("phcsync-warm-reboot-gate", syslog.LOG_PID)

    if not os.path.isfile(PHCSYNC_SCRIPT) or not os.access(PHCSYNC_SCRIPT, os.X_OK):
        log("phcsync script not found or not executable: %s" % PHCSYNC_SCRIPT, syslog.LOG_ERR)
        return 1

    _child_proc = None

    signal.signal(signal.SIGTERM, shutdown)
    signal.signal(signal.SIGINT, shutdown)

    try:
        _child_proc = subprocess.Popen(
            [PHCSYNC_SCRIPT],
            # Run child in a new process group to avoid receiving signals sent to the parent
            preexec_fn=os.setpgrp if hasattr(os, "setpgrp") else None,
        )
    except Exception as e:
        log("Failed to start %s: %s" % (PHCSYNC_SCRIPT, e), syslog.LOG_ERR)
        return 1

    log("Started phcsync (pid %d), listening for warm reboot" % _child_proc.pid)

    while True:
        # Wait for warm boot to start
        if not RestartWaiter.waitWarmBootStarted(maxWaitSec=WARM_BOOT_STARTED_TIMEOUT_SEC):
            log("waitWarmBootStarted timed out, continuing loop", syslog.LOG_DEBUG)
            continue

        # If warm boot started, check if phcsync process is still running
        pid = _child_proc.pid
        if pid is None:
            break
        try:
             # Check if phcsync process is still running - use WNOHANG to avoid blocking
            _pid, _ = os.waitpid(pid, os.WNOHANG)
            if _pid != 0:
                log("phcsync exited (pid=%d), gate exiting" % pid, syslog.LOG_WARNING)
                return 2
        except ChildProcessError:
            break

        # If _pid =0 phcsync is still running, pause it
        log("Warm reboot started, pausing phcsync")
        try:
            os.kill(pid, signal.SIGSTOP)
        except ProcessLookupError:
            log("phcsync process gone, exiting", syslog.LOG_WARNING)
            return 2

        # Wait for warm boot to complete.
        if not RestartWaiter.waitWarmBootDone(maxWaitSec=WARM_BOOT_DONE_TIMEOUT_SEC):
            # If waitWarmBootDone timed out, resume phcsync and check again for warm boot (via RestartWaiter.waitWarmBootStarted)
            log("waitWarmBootDone timed out, resuming phcsync anyway", syslog.LOG_WARNING)

        # Resume phcsync
        try:
            os.kill(pid, signal.SIGCONT)
        except ProcessLookupError:
            log("phcsync process gone while trying to resume, exiting", syslog.LOG_WARNING)
            return 2
        log("Warm reboot done or timed out, resumed phcsync")

    # The main loop is expected to run indefinitely, reaching here means unexpected termination.
    log("Unexpected termination, exiting", syslog.LOG_WARNING)
    return 2


if __name__ == "__main__":
    sys.exit(main())
