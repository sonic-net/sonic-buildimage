#!/bin/bash
# Wait for dhcpservd to signal readiness by creating a flag file.
# This ensures kea-dhcp4 only starts after dhcpservd has registered
# its SIGUSR1 handler, preventing a race condition where kea sends
# SIGUSR1 before the handler is set up (default disposition: terminate).

DHCPSERVD_READY_FLAG="/tmp/dhcpservd_ready"
TIMEOUT=120

for i in $(seq 1 $TIMEOUT); do
    if [ -f "$DHCPSERVD_READY_FLAG" ]; then
        rm -f "$DHCPSERVD_READY_FLAG"
        logger -p daemon.info "wait_for_dhcpservd: dhcpservd is ready (flag file found and removed after ${i}s)"
        exit 0
    fi
    sleep 1
done

logger -p daemon.error "wait_for_dhcpservd: timed out waiting for dhcpservd readiness after ${TIMEOUT}s"
exit 1
