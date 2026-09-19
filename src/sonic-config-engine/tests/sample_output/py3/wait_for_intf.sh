#!/usr/bin/env bash

# Maximum time (in seconds) to wait for a single interface to become ready in
# STATE_DB before proceeding with relay startup. A value of 0 means "wait
# indefinitely" (the historical behavior).
#
# The bound is applied only when has_sonic_dhcpv4_relay == 'True'. In that mode
# the relay agents reconcile a late-ready interface at runtime (dhcp4relay and
# dhcp6relay both watch STATE_DB INTERFACE_TABLE; dhcp6relay also re-checks the
# link-local address periodically), so an interface that becomes ready after the
# timeout is still picked up without a restart, and one never-ready interface
# must not block the whole container. The default (300s) covers normal startup
# convergence.
#
# Otherwise (legacy ISC dhcrelay) the default stays 0 (wait forever): that agent
# binds its interfaces at process start and does not reconcile readiness at
# runtime, so shortening the wait could make it permanently miss an interface.
# Operators can still opt into a bound by exporting WAIT_IFACE_READY_TIMEOUT.
WAIT_IFACE_READY_TIMEOUT="${WAIT_IFACE_READY_TIMEOUT:-0}"

function wait_until_iface_ready
{
    IFACE_NAME=$1
    IFACE_CIDR=$2

    echo "Waiting until interface ${IFACE_NAME} is ready..."

    # Wait for the interface to come up
    # (i.e., interface is present in STATE_DB and state is "ok"). When
    # WAIT_IFACE_READY_TIMEOUT > 0 the wait is bounded so a never-ready
    # interface cannot hang startup forever; when it is 0 we wait indefinitely
    # (legacy ISC dhcrelay behavior).
    local waited=0
    while true; do
        RESULT=$(sonic-db-cli STATE_DB HGET "INTERFACE_TABLE|${IFACE_NAME}|${IFACE_CIDR}" "state" 2> /dev/null)
        if [ x"$RESULT" == x"ok" ]; then
            echo "Interface ${IFACE_NAME} is ready!"
            return 0
        fi

        if [ "${WAIT_IFACE_READY_TIMEOUT}" -gt 0 ] && [ "${waited}" -ge "${WAIT_IFACE_READY_TIMEOUT}" ]; then
            echo "WARNING: interface ${IFACE_NAME} (${IFACE_CIDR}) not ready after ${WAIT_IFACE_READY_TIMEOUT}s; proceeding without it (relay agents reconcile it at runtime)"
            return 0
        fi

        sleep 1
        waited=$((waited + 1))
    done
}

# Wait for all interfaces with IPv4 addresses to be up and ready
# dhcp6relay binds to ipv6 addresses configured on these vlan ifaces
# Thus check if they are ready before launching dhcp6relay
wait_until_iface_ready Vlan1000 fc02:2000::2/24
wait_until_iface_ready Vlan1000 192.168.0.1/27
wait_until_iface_ready Vlan2000 192.168.200.1/27
wait_until_iface_ready PortChannel01 10.0.0.56/31
wait_until_iface_ready PortChannel02 10.0.0.58/31
wait_until_iface_ready PortChannel03 10.0.0.60/31
wait_until_iface_ready PortChannel04 10.0.0.62/31

# Wait 10 seconds for the rest of interfaces to get added/populated.
# dhcrelay listens on each of the interfaces (in addition to the port
# channels and vlan interfaces)
sleep 10
