#!/bin/bash
#
# ciena-8140-bmc-rtc-bootstrap.sh
#
# Launcher for obmc_rtc_updater (runs as the service's ExecStart).
#
# obmc_rtc_updater informs the BMC on every system-clock *step*, and does one
# unconditional host->BMC sync at startup.
# SONiC deliberately configures chrony to slew (never step) so as not to disturb
# protocols/PMs/traffic.  That creates two problems this launcher solves before
# it hands off (via exec) to the daemon:
#
#   1. Never push un-synced time to the BMC.  We only exec the daemon once chrony
#      has a real source AND the residual correction has actually been applied,
#      so the daemon's initial sync writes genuinely NTP-disciplined time.
#   2.  once synced we issue one on-demand 'chronyc makestep' and confirm (via 'chronyc
#      waitsync') that the offset has collapsed, i.e. the step has landed.  This
#      does not change chrony's steady-state slew-only behaviour.
#
set -u

DAEMON=/usr/local/bin/obmc_rtc_updater

# ---------------------------------------------------------------------------
# Prerequisite: /dev/ipmi0 (the BMC IPMI KCS char device).
#
#  loaded at boot by systemd-modules-load.service from
# /etc/modules-load.d/ciena-8140.conf, 
# ---------------------------------------------------------------------------
if [ ! -e /dev/ipmi0 ]; then
    # modules-load.d should already have loaded these.
    modprobe ipmi_si ipmi_devintf >/dev/null 2>&1 || true
fi
attempt=0
while [ ! -e /dev/ipmi0 ]; do
    attempt=$((attempt + 1))
    if [ "$attempt" -eq 1 ] || [ "$((attempt % 12))" -eq 0 ]; then
        echo "bmc-rtc-bootstrap: waiting for /dev/ipmi0 (ipmi_si/ipmi_devintf via /etc/modules-load.d/ciena-8140.conf)" >&2
    fi
    sleep 5
done

# Without chrony we cannot verify sync; let the daemon do its best-effort sync
# rather than blocking BMC updates forever.
command -v chronyc >/dev/null 2>&1 || exec "$DAEMON"

is_synced() {
    local t leap refid
    t=$(timeout 10 chronyc -n tracking 2>/dev/null) || return 1
    leap=$(echo "$t"  | awk -F': *' '/Leap status/  {print $2; exit}')
    refid=$(echo "$t" | awk -F': *' '/Reference ID/ {print $2; exit}')
    [ "$leap" = "Normal" ] && [ -n "$refid" ] && ! echo "$refid" | grep -qi '00000000'
}

# Wait (in the background of the already-active unit) until NTP is synchronized,
# then exec the daemon.  We never exit non-zero for the expected "not synced yet"
# case, so systemd never marks the unit FAILED.
attempt=0
while : ; do
    if is_synced; then
        # Force the corrective step now, then confirm the residual correction
        # has collapsed (step landed) before handing off to the daemon.  waitsync:
        # max-tries=3, max-correction=0.5s, max-skew=0 (ignored), interval=2s.
        timeout 10 chronyc -a makestep >/dev/null 2>&1 || true
        if timeout 20 chronyc waitsync 3 0.5 0.0 2 >/dev/null 2>&1; then
            exec "$DAEMON"
        fi
    else
        # Not yet synced: nudge chrony to close a large offset early.
        timeout 10 chronyc -a makestep >/dev/null 2>&1 || true
    fi
    # Log occasionally (every ~60s) so the wait is visible in the journal
    # without spamming the console.
    attempt=$((attempt + 1))
    if [ "$((attempt % 12))" -eq 1 ]; then
        echo "bmc-rtc-bootstrap: waiting for host clock to NTP-synchronize before updating BMC RTC" >&2
    fi
    sleep 5
done
