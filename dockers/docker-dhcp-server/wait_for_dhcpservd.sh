#!/bin/bash
# Wait for dhcpservd to signal readiness by creating a flag file.
# This ensures kea-dhcp4 only starts after dhcpservd has registered
# its SIGUSR1 handler, preventing a race condition where kea sends
# SIGUSR1 before the handler is set up (default disposition: terminate).

DHCPSERVD_READY_FLAG="/tmp/dhcpservd_ready"
TIMEOUT=120

# Do NOT remove the flag: the dependent-startup plugin may launch this
# checker more than once (the program has startsecs=0 and exits quickly).
# dhcpservd does not restart within a container, and docker_init.sh clears the stale
# flag on container start, so leaving it in place keeps re-runs idempotent.
for i in $(seq 1 $TIMEOUT); do
    if [ -f "$DHCPSERVD_READY_FLAG" ]; then
        logger -p daemon.info "wait_for_dhcpservd: dhcpservd is ready (flag file found after ${i}s)"
        exit 0
    fi
    sleep 1
done

logger -p daemon.error "wait_for_dhcpservd: timed out waiting for dhcpservd readiness after ${TIMEOUT}s"
exit 1
