#!/usr/bin/env python3
"""
syncd.service ExecStopPost hook for Nexthop platforms.

When syncd stops, power-cycle the ASIC via /usr/local/bin/asic_init.sh to
recover from any errors potentially.Skip the power-cycle if
warm-reboot or fast-reboot orchestration is in progress — in that case
the ASIC must be left intact for the upcoming kexec.

Wired in via a per-platform syncd.service.d/*.conf override:
    [Service]
    ExecStopPost=/usr/local/bin/syncd_post_stop.py
"""

import os
import subprocess
import sys
import syslog

ASIC_INIT = "/usr/local/bin/asic_init.sh"
WARMBOOT_DUMP_RDB = "/host/warmboot/dump.rdb"
SONIC_DB_CLI = "sonic-db-cli"
DB_CLI_TIMEOUT_S = 5

WARM_RESTART_ENABLE_KEY = ("WARM_RESTART_ENABLE_TABLE|system", "enable")
FAST_RESTART_ENABLE_KEY = ("FAST_RESTART_ENABLE_TABLE|system", "enable")


def _state_db_hget(key, field):
    try:
        result = subprocess.run(
            [SONIC_DB_CLI, "STATE_DB", "hget", key, field],
            capture_output=True,
            text=True,
            timeout=DB_CLI_TIMEOUT_S,
        )
    except (FileNotFoundError, subprocess.TimeoutExpired) as e:
        syslog.syslog(syslog.LOG_WARNING, f"sonic-db-cli STATE_DB hget {key} {field} failed: {e}")
        return None
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def is_warm_or_fast_reboot_in_progress():
    for key, field in (WARM_RESTART_ENABLE_KEY, FAST_RESTART_ENABLE_KEY):
        if _state_db_hget(key, field) == "true":
            syslog.syslog(syslog.LOG_INFO, f"{key} {field}=true in STATE_DB")
            return True

    if os.path.isfile(WARMBOOT_DUMP_RDB):
        syslog.syslog(syslog.LOG_INFO, f"{WARMBOOT_DUMP_RDB} present")
        return True

    return False


def main():
    syslog.openlog("syncd_post_stop")

    if is_warm_or_fast_reboot_in_progress():
        syslog.syslog(syslog.LOG_INFO, "syncd stop during warm/fast reboot: leaving ASIC up for kexec")
        return 0

    syslog.syslog(syslog.LOG_INFO, "syncd stop without warm/fast reboot: power-cycling ASIC to reinitialize")
    os.execv(ASIC_INIT, [ASIC_INIT])


if __name__ == "__main__":
    sys.exit(main())
