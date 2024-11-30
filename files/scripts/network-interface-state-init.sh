#!/bin/sh

# Get list of Ethernet interfaces excluding Docker interfaces
interfaces=$(ip -o link show | awk -F': ' '$2 ~ /^e/ && $2 !~ /^docker/ {print $2}')

# Loop through each Ethernet interface
for interface in $interfaces; do
    # Check if the interface is already up
    if ! ip link show dev $interface | grep -q 'state UP'; then
        # Bring up the interface if it's not already up
        ip link set dev $interface up || { echo "Failed to bring up interface $interface"; continue; }
    fi

    # Configure the interface to use DHCP
    dhclient $interface || echo "Failed to configure DHCP for interface $interface"
done
