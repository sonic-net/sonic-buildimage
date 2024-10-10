#!/bin/bash

# Define the interface
INTERFACE="int_mnic0"

# Delete any preexisting IP addresses on the interface
ip addr flush dev $INTERFACE

dhclient $INTERFACE
# Infinite loop to ping every 10 seconds
while true; do
    dhclient $INTERFACE
    sleep 180
done

