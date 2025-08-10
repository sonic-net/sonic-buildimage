#!/bin/bash

function load_eth0.4088(){
    try_times_remain=5
    state_up=$(ip -d link show eth0.4088 | awk '/state UP/{print $2}')
    while [ -z "$state_up" ] && [ $try_times_remain -ne 0 ]
    do
        ((try_times_remain-=1))
        ip link add link eth0 name eth0.4088 type vlan id 4088 || true
        ip addr add 240.1.1.2/30 brd 240.1.1.3 dev eth0.4088 || true
        ip link set dev eth0.4088 up || true
        state_up=$(ip -d link show eth0.4088 | awk '/state UP/{print $2}')
        sleep 1
    done
}

function unload_eth0.4088(){
    ip link set dev eth0.4088 down
    ip link del eth0.4088
}

if [ "$1" = "start" ];then
    load_eth0.4088
elif [ "$1" = "stop" ];then
    unload_eth0.4088
fi
