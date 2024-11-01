#!/bin/bash


PLAT_FILE=/etc/mlnx/platform.json
if [[ ! -f $PLAT_FILE ]]; then
PLATFORM=$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
PLATFORM_JSON=/usr/share/sonic/device/x86_64-nvidia_sn4280-r0/platform.json
ln -s $PLATFORM_JSON $PLAT_FILE
fi


usage(){
    echo "Usage: $0 {dpu2pcie|dpu2rshim|rshim2dpu|pcie2dpu} name"
}
declare -A dpu2pcie

validate_platform(){
    if [[ ! -f $PLAT_FILE ]]; then
        echo "platform.json file not found. Exiting script"
        exit 1
    fi
}


validate_platform
case $1 in
    "dpu2rshim")
	jq_query='.DPUS[$dpu].rshim_info'
	var="dpu"
	;;
    "dpu2pcie")
	jq_query='.DPUS[$dpu].bus_info'
	var="dpu"
	;;
    "pcie2dpu")
	jq_query='.DPUS | to_entries[] | select(.value.bus_info == $bus) | .key'
	var="bus"
	;;
    "pcie2rshim")
	jq_query='.DPUS | to_entries[] | select(.value.bus_info == $bus) | .value.rshim_info'
	var="bus"
	;;
    "rshim2dpu")
	jq_query='.DPUS | to_entries[] | select(.value.rshim_info == $rshim) | .key'
	var="rshim"
	;;
    "rshim2pcie")
	jq_query='.DPUS | to_entries[] | select(.value.rshim_info == $rshim) | .value.bus_info'
	var="rshim"
	;;
    *)
        echo "Invalid usage of script!"
        usage
        exit 1
esac

IFS=',' read -r -a identifier_array <<< "$2"
for identifier in "${identifier_array[@]}"; do
	op=$(jq -r --arg "$var" "$identifier" "$jq_query" "$PLAT_FILE")
	if [[ "$op" != "null" ]]; then
		echo "$op"
	fi
done

