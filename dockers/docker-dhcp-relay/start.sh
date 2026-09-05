#!/usr/bin/env bash

if [ "${RUNTIME_OWNER}" == "" ]; then
    RUNTIME_OWNER="kube"
fi

CTR_SCRIPT="/usr/share/sonic/scripts/container_startup.py"
if test -f ${CTR_SCRIPT}
then
    ${CTR_SCRIPT} -f dhcp_relay -o ${RUNTIME_OWNER} -v ${IMAGE_VERSION}
fi

keys=$(sonic-db-cli COUNTERS_DB keys "DHCPV4_COUNTER_TABLE:*")
for key in $keys; do
    sonic-db-cli COUNTERS_DB del "$key"
done

# If our supervisor config has entries in the "dhcp-relay" group...
if [ $(supervisorctl status | grep -c "^dhcp-relay:") -gt 0 ]; then
    # Wait for all interfaces to come up and be assigned IPv4 addresses before
    # starting the DHCP relay agent(s). If an interface the relay should listen
    # on is down, the relay agent will not start. If an interface the relay
    # should listen on is up but does not have an IP address assigned when the
    # relay agent starts, it will not listen or send on that interface for the
    # lifetime of the process.
    /usr/bin/wait_for_intf.sh
fi
