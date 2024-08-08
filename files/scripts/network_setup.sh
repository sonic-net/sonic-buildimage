#!/bin/sh

# Function to get the IP address of the eth0 interface
get_eth0_ip() {
    ip addr show eth0 | grep 'inet ' | awk '{print $2}' | cut -d'/' -f1
}

# Function to get the default gateway
get_default_gateway() {
    ip route | grep default | awk '{print $3}'
}

# Wait for the network interface to appear
for i in {1..10}; do
    if ip link show eth0; then
        break
    fi
    sleep 1
done

# Bring up the network interface
ip link set eth0 up

# Use DHCP to obtain IP address and default gateway
dhclient eth0

# Wait a few seconds to ensure the IP is assigned
sleep 6

# Get the IP address and gateway to verify
ETH0_IP=$(get_eth0_ip)
DEFAULT_GW=$(get_default_gateway)