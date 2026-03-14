#!/bin/bash
resolvconf_updates=true

function wait_networking_service_done() {
    local -i _WDOG_CNT="1"
    local -ir _WDOG_MAX="30"

    local -r _TIMEOUT="1s"

    while [[ "${_WDOG_CNT}" -le "${_WDOG_MAX}" ]]; do
        networking_status="$(systemctl is-active networking 2>&1)"

        if [[ "${networking_status}" == active || "${networking_status}" == inactive || "${networking_status}" == failed ]] ; then
            return
        fi

        echo "interfaces-config: networking service is running, wait for it done"

        let "_WDOG_CNT++"
        sleep "${_TIMEOUT}"
    done

    echo "interfaces-config: networking service is still running after 30 seconds, killing it"
    systemctl kill networking 2>&1
}

function resolvconf_updates_disable() {
    resolvconf --updates-are-enabled
    if [[ $? -ne 0 ]]; then
        resolvconf_updates=false
    fi
    resolvconf --disable-updates
}

function resolvconf_updates_restore() {
    if [[ $resolvconf_updates == true ]]; then
        resolvconf --enable-updates
    fi
}

# Do not run DNS configuration update during the shutdowning of the management interface. 
# This operation is redundant as there will be an update after the start of the interface.
resolvconf_updates_disable

if [[ $(ifquery --running eth0) ]]; then
    wait_networking_service_done
    ifdown --force eth0
fi

# Check if ZTP DHCP policy has been installed
if [[ -e /etc/network/ifupdown2/policy.d/ztp_dhcp.json ]]; then
    # Obtain port operational state information
    redis-dump -d 0 -k "PORT_TABLE:Ethernet*"  -y > /tmp/ztp_port_data.json

    if [[ $? -ne 0 || ! -e /tmp/ztp_port_data.json || "$(cat /tmp/ztp_port_data.json)" = "" ]]; then
        echo "{}" > /tmp/ztp_port_data.json
    fi

    # Create an input file with ztp input information
    echo "{ \"PORT_DATA\" : $(cat /tmp/ztp_port_data.json) }" > \
          /tmp/ztp_input.json
else
    echo "{ \"ZTP_DHCP_DISABLED\" : \"true\" }" > /tmp/ztp_input.json
fi

# Create /e/n/i file for existing and active interfaces, dhcp6 sytcl.conf and dhclient.conf
CFGGEN_PARAMS=" \
    -d -j /tmp/ztp_input.json \
    -t /usr/share/sonic/templates/interfaces.j2,/etc/network/interfaces \
    -t /usr/share/sonic/templates/90-dhcp6-systcl.conf.j2,/etc/sysctl.d/90-dhcp6-systcl.conf \
    -t /usr/share/sonic/templates/dhclient.conf.j2,/etc/dhcp/dhclient.conf \
"
sonic-cfggen $CFGGEN_PARAMS

[[ -f /var/run/dhclient.eth0.pid ]] && kill `cat /var/run/dhclient.eth0.pid` && rm -f /var/run/dhclient.eth0.pid
[[ -f /var/run/dhclient6.eth0.pid ]] && kill `cat /var/run/dhclient6.eth0.pid` && rm -f /var/run/dhclient6.eth0.pid

for intf_pid in $(ls -1 /var/run/dhclient*.Ethernet*.pid 2> /dev/null); do
    [[ -f ${intf_pid} ]] && kill `cat ${intf_pid}` && rm -f ${intf_pid}
done

/usr/bin/resolv-config.sh cleanup
# Restore DNS configuration update to the previous state.
resolvconf_updates_restore

# Read sysctl conf files again
sysctl -p /etc/sysctl.d/90-dhcp6-systcl.conf

MAX_RETRIES=5
RETRY_DELAY=2
for ((i=1; i<=MAX_RETRIES; i++)); do
    LOG_MARK=$(date '+%Y-%m-%d %H:%M:%S')
    if systemctl restart networking; then
        if journalctl -u networking --since "$LOG_MARK" | grep -q "error.*already running"; then
            echo "interfaces-config: error during networking restart in attempt $i. Retrying in ${RETRY_DELAY} seconds..."
            sleep "${RETRY_DELAY}"
        else
            echo "interfaces-config: systemctl restart networking succeeded on attempt $i"
            break
        fi
    else
        echo "interfaces-config: Attempt $i to restart networking failed. Retrying in ${RETRY_DELAY} seconds..."
        sleep "${RETRY_DELAY}"
    fi
done

# Clean-up created files
rm -f /tmp/ztp_input.json /tmp/ztp_port_data.json
