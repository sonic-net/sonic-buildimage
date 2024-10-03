#!/bin/bash
#set -x

# Check if a port name is provided
if [ -z "$1" ]; then
    echo "Usage: $0 EthernetXX EthernetYY"
    exit 1
fi


# Assign the first argument to the port variable
lport1="$1"
psleep="$2"

if ! [[ "$psleep" =~ ^-?[0-9]+$ ]]; then
	psleep=0
fi

# Execute the command and extract the port number for the specified Ethernet port
bcmport1=$(bcmcmd 'bsh -c "knet netif info"' | grep -A5 -E "$lport1\b" | awk '/Port:/ {print $2}' | tr -cd '0-9')

echo "SONiC logical port=$lport1 bcmport=$bcmport1"

# Check if port_number is empty
if [ -z "$bcmport1" ]; then
    echo "BCM Port number not found for $lport1."
    exit 1
fi

portn1=$(echo $lport1 | grep -o '[0-9]\+')

start_lane1=$(($portn1 % 8))
end_lane1=$(($start_lane1 + 1))

function start_prbs() 
{
    local bcmport=$1
    local start_lane=$2
    local end_lane=$3

    echo "Enabling PRBS on ports $bcmport lanes=$start_lane,$end_lane"
    CMD="phydiag $bcmport1 prbs set p=p31 lane=${start_lane},${end_lane}"
    echo ${CMD}
    bcmcmd "dsh -c 'phydiag $bcmport prbs set p=p31 lane=${start_lane},${end_lane}'"
    sleep 2
    echo ""
}

function get_prbs_status()
{
    local bcmport=$1
    local start_lane=$1
    local end_lane=$2
    echo "Checking PRBS status on ports $bcmport"
    CMD="phydiag $bcmport prbs get lane=$start_lane,$end_lane"
    echo $CMD
    bcmcmd "dsh -c 'phydiag $bcmport prbs get lane=$start_lane,$end_lane'"
    echo ""
}

function show_dsc_ber()
{
    local lport=$1
    local bcmport=$2
    echo "BER on $lport"
    CMD="phy diag $bcmport dsc"
    echo $CMD
    bcmcmd  "dsh -c 'phy diag $bcmport dsc'" | grep -A4 "BER"
    echo ""
}

function clear_prbs()
{
    local bcmport=$1
    local start_lane=$2
    local end_lane=$3
    CMD="phydiag $bcmport prbs clear lane=$start_lane,$end_lane"
    echo $CMD
    bcmcmd  "dsh -c 'phydiag $bcmport prbs clear lane=$start_lane,$end_lane'"
    echo ""
}

start_prbs "$bcmport1" "$start_lane1" "$end_lane1"
sleep 2

get_prbs_status "$bcmport1" "$start_lane1" "$end_lane1"

sleep 10

show_dsc_ber "$lport1" "$bcmport1"

while [ "$psleep" -ne 0 ]; do
    show_dsc_ber "$lport1" "$bcmport1"
    psleep=$(($psleep / 2))
    sleep 10
done
    
clear_prbs "$bcmport1" "$start_lane1" "$end_lane1"
