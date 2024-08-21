#!/bin/bash
#Script to control the DPU management traffic forwarding through the SmartSwitch

command_name=$0

usage(){
    echo "Syntax: $command_name -e|--enable -d|--disable"
    echo "Arguments:"
    echo "-e			Enable dpu management traffic forwarding"
    echo "-d			Disable dpu management traffic forwarding"
}

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
    echo $value > /proc/sys/net/ipv4/ip_forward
    echo $value > /proc/sys/net/ipv4/conf/eth0/forwarding
}

ctrl_dpu_forwarding(){
    local op=$1
    control_forwarding $op
    add_rem_valid_iptable $op nat POSTROUTING -o ${mgmt_iface}  -j MASQUERADE
    add_rem_valid_iptable $op filter FORWARD -i ${mgmt_iface} -o ${midplane_iface} -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
    add_rem_valid_iptable $op filter FORWARD -i ${midplane_iface} -o ${mgmt_iface} -j ACCEPT
    if [ "$op" = "enable" ]; then
        echo "Enabled DPU management traffic Forwarding"
    else
        echo "Disabled DPU management traffic Forwarding"
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

case $1 in
    -e|--enable)
        ctrl_dpu_forwarding enable
        ;;
    -d|--disable)
        ctrl_dpu_forwarding disable
        ;;
    *)
        echo "Incorrect Usage!"
        usage
        exit 1
        ;;
esac
