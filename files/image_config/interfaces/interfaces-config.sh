#!/bin/bash

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

# Read sysctl conf files again
sysctl -p /etc/sysctl.d/90-dhcp6-systcl.conf

systemctl restart networking

# Clean-up created files
rm -f /tmp/ztp_input.json /tmp/ztp_port_data.json
