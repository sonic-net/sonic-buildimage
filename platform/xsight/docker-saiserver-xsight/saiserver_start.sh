#!/usr/bin/env bash
# Copy from src/sonic-sairedis/syncd/scripts/syncd_start.sh
# Re-use the structure for syncd setup
# Use it to start saiserver
# Script to start syncd using supervisord
#

# Source the file that holds common code for systemd and supervisord
. /usr/bin/syncd_init_common.sh

get_saiserver_param()
{
    IFS=' ' read -r -a array <<< "$CMD_ARGS"
    for index in "${!array[@]}"
    do
        #echo "$index ${array[index]}"
        if [[ "${array[index]}" == *"-p"* ]]; then
            SAI_PROFILE="${array[index+1]}"
        fi
        if [[ "${array[index]}" == *"-m"* ]]; then
            PORT_CONFIG="${array[index+1]}"
        fi
    done
}

ENABLE_SAITHRIFT=1
config_syncd
get_saiserver_param

PLATFORM=$(sonic-cfggen --from-db --var 'DEVICE_METADATA.localhost.platform')

if [[ "${PLATFORM}" =~ "kvm" ]]; then
    PACKET_INTERFACE_CPU_IFNAME="xcpu"
else
    PACKET_INTERFACE_CPU_IFNAME="${xpci_netdev}"
fi

export PACKET_INTERFACE_CPU_IFNAME="${PACKET_INTERFACE_CPU_IFNAME}" 
export PACKET_INTERFACE_ENABLE="true"

echo exec /usr/sbin/saiserver -p ${SAI_PROFILE} -f ${PORT_CONFIG}
exec /usr/sbin/saiserver -p ${SAI_PROFILE} -f ${PORT_CONFIG}
#exec ${CMD} ${CMD_ARGS}
