#!/bin/bash

# Re-install the igb and ixgbe again to make the NIC sequence follow the udev rule
modprobe -r igb
modprobe -r ixgbe
modprobe igb
modprobe ixgbe
