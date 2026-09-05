#!/bin/bash
#Script to control the DPU management traffic forwarding through the SmartSwitch

command_name=$0

usage(){
    echo "Syntax: $command_name inbound/outbound -e|--enable -d|--disable [--dpus,--ports,--nofwctrl]"
    echo "Arguments:"
    echo "inbound		Control DPU Inbound traffic forwarding"
    echo "outbound		Control DPU Outbound traffic forwarding"
    echo "-e			Enable dpu management traffic forwarding in a specific direction"
    echo "-d			Disable dpu management traffic forwarding in a specific direction"
    echo "--dpus		Selection of dpus for which the inbound traffic has to be controlled (all can be specified)"
    echo "--ports		Selection of ports for which the inbound traffic has to be controlled"
    echo "--nofwctrl	Disable changing the general ipv4 forwarding (Could be useful if inbound is enabled and outbound is disabled)"
}

dpu_l=()
declare -A midplane_dict
declare -A midplane_ip_dict
fw_change="enable"

add_rem_valid_iptable(){
    local op=$1
    local table=$2
    local chain=$3
    shift 3
    local rule="$@"
    iptables -t $table -C $chain $rule &>/dev/null
    local exit_status=$?
    local exec_cond=0
    if [ "$op" = "enable" ]; then
        exec_command="iptables -t $table -A $chain $rule"
        [ "$exit_status" -eq 0 ] || exec_cond=1 # Execute if rule is currently not present
    else
        exec_command="iptables -t $table -D $chain $rule"
        [ "$exit_status" -ne 0 ] || exec_cond=1 # Execute if rule is currently present
    fi
    if [ "$exec_cond" -eq 1 ]; then
        eval "$exec_command"
    else
        echo "$exec_command not requried, will not be executed"
    fi
}

control_forwarding(){
    local op=$1
    local value=0
    if [ "$op" = "enable" ]; then
        value=1
    fi
    if [ "$fw_change" = "enable" ]; then
        echo $value > /proc/sys/net/ipv4/ip_forward
        echo $value > /proc/sys/net/ipv4/conf/eth0/forwarding
    fi
}

validate_dpus(){
    local provided_list=("$@")
    for item1 in "${provided_list[@]}"; do
        local found=0
        for item2 in "${dpu_l[@]}"; do
            if [[ "$item1" = "$item2" ]]; then
                found=1
                break
            fi
        done
        if [[ $found -eq 0 ]]; then
            echo "$item1 is not detected! Please provide proper dpu names list!"
            exit 1
        fi
    done
}

general_validation(){
    if [ -z "$direction" ]; then
        echo "Please provide the direction argument (inbound or outboud)"
        usage
        exit 1
    fi

    if [ -z "$operation" ]; then
        echo "Please provide the operation option (-e or -d)"
        usage
        exit 1
    fi
}

port_use_validation(){
	local port_l=("$@")
	for port in "${port_l[@]}"; do
		if (( port >= 0 && port <= 1023 )); then
			echo "Provided port $port in range 0-1023, Please execute with a different port"
			exit 1
		fi
		if netstat -tuln | awk '{print $4}' | grep -q ":$port\$"; then
			echo "Provided port $port is in use by another process, Please execute with a different port"
			exit 1
		fi
	done
}




inbound_validation(){
    #DPU Validation
    while IFS= read -r line; do
        dpu_name=$( echo "$line" | sed 's/.*|//;s/"//g')
        dpu_l+=("$dpu_name")
    done < <(redis-cli -n 4 keys DPUS*)
    len1=${#dpu_name[@]}
    if [ "$len1" -eq 0 ]; then
        echo "No dpus detected on device!"
        exit 1
    fi
    sorted_dpu_l=($(for item in "${dpu_l[@]}"; do
        echo "$item"
    done | sort))
    if [ -z "$arg_dpu_names" ]; then
            echo "No DPUs provided!"
            usage
            exit 1
        else
            if [ "$arg_dpu_names" = "all" ]; then
                sel_dpu_names=("${sorted_dpu_l[@]}")
                echo "${#sorted_dpu_l[@]} DPUs detected:"
                echo "${sorted_dpu_l[@]}"
            else
                IFS=',' read -ra sel_dpu_names <<< "$arg_dpu_names"
                validate_dpus ${sel_dpu_names[@]}
            fi
    fi
    #Port validation
    IFS=',' read -ra provided_ports <<< "$arg_port_list"
    len1=${#sel_dpu_names[@]}
    len2=${#provided_ports[@]}
    if [ "$len1" -ne "$len2" ]; then
        echo "Length of ${sel_dpu_names[@]} does not match provided port length ${provided_ports[@]}"
        usage
        exit 1
    fi
    port_use_validation ${provided_ports[@]}
    for dpu in "${sel_dpu_names[@]}"; do
        midplane_int_name=$(redis-cli -n 4 hget "DPUS|$dpu" "midplane_interface")
        if [ -z "$midplane_int_name" ]; then
            echo "Cannot obtain midplane interface for $dpu"
            exit 1
        fi
        midplane_dict["$dpu"]="$midplane_int_name"
    done

    for dpu in "${!midplane_dict[@]}"; do
        midplane_ip=$(redis-cli -n 4 hget "DHCP_SERVER_IPV4_PORT|$midplane_iface|${midplane_dict[$dpu]}" "ips@")
        if [ -z "$midplane_ip" ]; then
            echo "Cannot obtain midplane ip for $dpu"
            exit 1
        fi
        midplane_ip_dict["$dpu"]="$midplane_ip"
    done
}

# Outbound Traffice forwarding control function
ctrl_dpu_ob_forwarding(){
    local op=$1
    control_forwarding $op
    add_rem_valid_iptable $op nat POSTROUTING -o ${mgmt_iface}  -j MASQUERADE
    add_rem_valid_iptable $op filter FORWARD -i ${mgmt_iface} -o ${midplane_iface} -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
    add_rem_valid_iptable $op filter FORWARD -i ${midplane_iface} -o ${mgmt_iface} -j ACCEPT
    if [ "$op" = "enable" ]; then
        echo "Enabled DPU management outbound traffic Forwarding"
    else
        echo "Disabled DPU management outbound traffic Forwarding"
    fi
}

ctrl_dpu_ib_forwarding(){
    local op=$1
    local dest_port=22
    control_forwarding $op
    add_rem_valid_iptable $op filter FORWARD -i ${midplane_iface} -o ${mgmt_iface} -p tcp -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
    add_rem_valid_iptable $op filter FORWARD -i ${mgmt_iface} -o ${midplane_iface} -p tcp --dport $dest_port -j ACCEPT
    for index in ${!sel_dpu_names[@]}; do
        dpu_name="${sel_dpu_names[$index]}"
        dpu_midplane_ip="${midplane_ip_dict[$dpu_name]}"
        switch_port="${provided_ports[$index]}"
        add_rem_valid_iptable $op nat POSTROUTING -p tcp -d $dpu_midplane_ip --dport $dest_port -j SNAT --to-source $midplane_gateway
        add_rem_valid_iptable $op nat PREROUTING -i ${mgmt_iface}  -p tcp --dport $switch_port -j DNAT --to-destination $dpu_midplane_ip:$dest_port
    done
    if [ "$op" = "enable" ]; then
        echo "Enabled DPU management inbound traffic Forwarding"
    else
        echo "Disabled DPU management inbound traffic Forwarding"
    fi
}

mgmt_iface=eth0
midplane_iface=bridge-midplane

if [ "$EUID" -ne 0 ]
    then
    echo "Permission denied: Please run the script with elevated privileges using sudo"
    exit 1
fi

if ! ifconfig "$midplane_iface" > /dev/null 2>&1; then
    echo "$midplane_iface doesn't exist! Please run on smart switch system"
    exit 1
fi

midplane_gateway=$(ifconfig "$midplane_iface" 2>/dev/null | \
             grep 'inet ' | \
             awk '{print $2}' | \
             tr -d 'addr:')
if [ -z "$midplane_gateway" ]; then
    echo "Failed to get IP address for $midplane_iface"
    exit 1
fi

operation=""
direction=""

invalid_arg(){
    echo "Invalid arguments $1"
    usage
    exit 1
}


case $1 in
    inbound)
        direction="inbound"
        shift
        while [ "$1" != "--" ]  && [ -n "$1" ]; do
            case $1 in
                -e|--enable)
                    operation="enable"
                ;;
                -d|--disable)
                    operation="disable"
                ;;
                --dpus)
                    shift;
                    arg_dpu_names=$1
                ;;
                --ports)
                    shift;
                    arg_port_list=$1
                ;;
                --nofwctrl)
                    fw_change="disable"
                ;;
                *)
                    invalid_arg $1
                ;;
            esac
        shift
        done
    ;;
    outbound)
        direction="outbound"
        shift
        while [ "$1" != "--" ]  && [ -n "$1" ]; do
            case $1 in
                -e|--enable)
                    operation="enable"
                ;;
                -d|--disable)
                    operation="disable"
                ;;
                --nofwctrl)
                    fw_change="disable"
                ;;
                *)
                    invalid_arg $1
                ;;
            esac
        shift
        done
    ;;
    *)
        invalid_arg $1
    ;;
esac

general_validation


case $direction in
    outbound)
        ctrl_dpu_ob_forwarding $operation
        ;;
    inbound)
        inbound_validation
        ctrl_dpu_ib_forwarding $operation
        ;;
esac
