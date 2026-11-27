#!/usr/bin/env bash

# Maximum time (in seconds) to wait for a single interface to become ready in
# STATE_DB before proceeding with relay startup. A value of 0 means "wait
# indefinitely" (the historical behavior). Two bounds are used, one per agent:
#
# WAIT_IFACE_READY_TIMEOUT governs the IPv4 interface waits (INTERFACE, VLAN and
# PortChannel v4 addresses). It is bounded (300s) only when has_sonic_dhcpv4_relay
# == 'True': the sonic dhcp4relay watches STATE_DB INTERFACE_TABLE and reconciles
# a late-ready interface at runtime, so proceeding without one and picking it up
# later is safe. For the legacy ISC dhcrelay it stays 0 (wait forever): that agent
# binds its interfaces once at process start and never reconciles, so shortening
# the wait could make it permanently miss an interface.
#
# WAIT_IFACE_READY_TIMEOUT_V6 governs the IPv6 VLAN-interface waits, which are for
# dhcp6relay. dhcp6relay is always the native v6 agent and always reconciles at
# runtime (STATE_DB INTERFACE_TABLE + periodic link-local check), so these are
# always bounded (300s) regardless of the v4 relay type -- a never-ready v6 VLAN
# interface must not block container startup. Operators can override either bound
# by exporting the corresponding variable.
WAIT_IFACE_READY_TIMEOUT="${WAIT_IFACE_READY_TIMEOUT:-300}"
# dhcp6relay always reconciles late-ready interfaces at runtime, so the v6
# VLAN-interface waits below are always bounded, independent of the v4 relay type.
WAIT_IFACE_READY_TIMEOUT_V6="${WAIT_IFACE_READY_TIMEOUT_V6:-300}"

function wait_until_iface_ready
{
    IFACE_NAME=$1
    IFACE_CIDR=$2
    # Optional per-call timeout; defaults to WAIT_IFACE_READY_TIMEOUT (the v4/ISC
    # bound). The IPv6 VLAN waits pass WAIT_IFACE_READY_TIMEOUT_V6 instead.
    local timeout="${3:-${WAIT_IFACE_READY_TIMEOUT}}"

    echo "Waiting until interface ${IFACE_NAME} is ready..."

    # Wait for the interface to come up
    # (i.e., interface is present in STATE_DB and state is "ok"). When
    # timeout > 0 the wait is bounded so a never-ready interface cannot hang
    # startup forever; when it is 0 we wait indefinitely (legacy ISC dhcrelay
    # behavior).
    local waited=0
    while true; do
        RESULT=$(sonic-db-cli STATE_DB HGET "INTERFACE_TABLE|${IFACE_NAME}|${IFACE_CIDR}" "state" 2> /dev/null)
        if [ x"$RESULT" == x"ok" ]; then
            echo "Interface ${IFACE_NAME} is ready!"
            return 0
        fi

        if [ "${timeout}" -gt 0 ] && [ "${waited}" -ge "${timeout}" ]; then
            echo "WARNING: interface ${IFACE_NAME} (${IFACE_CIDR}) not ready after ${timeout}s; proceeding without it (relay agents reconcile it at runtime)"
            return 0
        fi

        sleep 1
        waited=$((waited + 1))
    done
}

# Wait for all interfaces with IPv4 addresses to be up and ready
# dhcp6relay binds to ipv6 addresses configured on these vlan ifaces
# Thus check if they are ready before launching dhcp6relay
wait_until_iface_ready Vlan1000 fc02:2000::2/24 "${WAIT_IFACE_READY_TIMEOUT_V6}"
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
