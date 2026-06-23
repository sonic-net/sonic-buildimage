#!/bin/sh

# Docker may be socket-activated very early during first boot. On some systems,
# the IPv6 iptables/nf_tables path may not be ready yet, which can cause dockerd
# initialization to fail and cascade dependency failures to database/config
# services.
#
# This check is only needed during early boot. Skip it for normal docker restarts
# to avoid expensive or blocking ip6tables probes.

uptime_seconds=$(awk '{printf "%d", $1}' /proc/uptime)
if [ "$uptime_seconds" -gt 120 ] 2>/dev/null; then
    exit 0
fi

MAX_ATTEMPTS=30
CALL_TIMEOUT=5

for i in $(seq 1 $MAX_ATTEMPTS); do
    if timeout $CALL_TIMEOUT ip6tables -w 1 -n -t filter -L >/dev/null 2>&1 && \
       timeout $CALL_TIMEOUT ip6tables -w 1 -n -t nat -L >/dev/null 2>&1; then
        logger "docker-netfilter-ready: ip6tables ready after ${i} attempt(s)"
        exit 0
    fi
    sleep 1
done

logger "docker-netfilter-ready: ip6tables not ready after $MAX_ATTEMPTS attempts"
echo "Docker netfilter/ip6tables path is not ready" >&2
exit 0
