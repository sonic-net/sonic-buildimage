#!/bin/bash

# Define the interface
INTERFACE="int_mnic0"

# Delete any preexisting IP addresses on the interface
ip addr flush dev $INTERFACE

dhclient $INTERFACE
