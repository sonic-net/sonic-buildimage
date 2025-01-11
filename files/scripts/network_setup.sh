#!/bin/sh

# Read USE_KDUMP variable from /etc/default/kdump-tools
if [ -f /etc/default/kdump-tools ]; then
    . /etc/default/kdump-tools
else
    USE_KDUMP=0  # Default to 0 if the file doesn't exist
fi

# Check if USE_KDUMP is enabled or disabled.
if [ "$USE_KDUMP" -ne 1 ]; then
    echo "KDUMP is not enabled. Skipping network setup."
    exit 0
fi

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
ETH0_IP=$(get_eth0_ip)
DEFAULT_GW=$(get_default_gateway)

if [ -z "$ETH0_IP" ] || [ -z "$DEFAULT_GW" ]; then
    echo "DHCP failed to assign IP. Please enter a static IP and gateway."

    read -p "Enter static IP address: " STATIC_IP
    read -p "Enter default gateway: " STATIC_GW

    # Set the static IP and gateway
    ip addr add $STATIC_IP/24 dev eth0
    ip route add default via $STATIC_GW
else

    echo "DHCP succeeded. IP: $ETH0_IP, Gateway: $DEFAULT_GW"
fi