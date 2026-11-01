#!/usr/bin/env python3
"""Clear restored restart state before SWSS on the first fast boot."""

import shlex
import subprocess
import syslog
import time
from pathlib import Path


def db(*args: str) -> str:
    command = ["sonic-db-cli", *args]
    syslog.syslog(f"Running command: {shlex.join(command)}")
    return subprocess.check_output(command, text=True).strip()


def is_fast_boot() -> bool:
    fast_boot = "SONIC_BOOT_TYPE=fast-reboot" in Path("/proc/cmdline").read_text().split()
    syslog.syslog(f"is_fast_boot={fast_boot}")
    return fast_boot


def get_fast_pending() -> bool:
    fast_pending = db("STATE_DB", "hget", "FAST_RESTART_ENABLE_TABLE|system", "enable") == "true"
    syslog.syslog(f"get_fast_pending={fast_pending}")
    return fast_pending


def wait_for_database() -> None:
    syslog.syslog("Waiting for database (Redis PING)")
    while True:
        try:
            if "PONG" in db("PING"):
                syslog.syslog("Database is ready (Redis PING returned PONG)")
                return
        except subprocess.CalledProcessError:
            pass
        time.sleep(1)


def main() -> None:
    if not is_fast_boot():
        return

    wait_for_database()

    # The cmdline persists for this boot; FAST is consumed only once.
    if not get_fast_pending():
        return

    # Clear FAST last so an earlier failure leaves cleanup pending for retry.
    db("STATE_DB", "hset", "WARM_RESTART_ENABLE_TABLE|system", "enable", "false")
    db("STATE_DB", "del", "WARM_RESTART_TABLE|orchagent", "WARM_RESTART_TABLE|syncd")
    db("STATE_DB", "hset", "FAST_RESTART_ENABLE_TABLE|system", "enable", "false")
    syslog.syslog("fast-reboot post-kexec: cleared FAST/WARM restart flags so swss/syncd cold-init")


if __name__ == "__main__":
    syslog.openlog("clear_fast_reboot_state")
    try:
        main()
    except (OSError, subprocess.SubprocessError) as error:
        # Cleanup errors must not block SWSS activation through ExecCondition.
        syslog.syslog(syslog.LOG_ERR, f"fast-reboot post-kexec: restart-state cleanup failed: {error}")
