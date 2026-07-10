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

echo exec /usr/sbin/saiserver -p ${SAI_PROFILE} -f ${PORT_CONFIG}
exec /usr/sbin/saiserver -p ${SAI_PROFILE} -f ${PORT_CONFIG}
#exec ${CMD} ${CMD_ARGS}
