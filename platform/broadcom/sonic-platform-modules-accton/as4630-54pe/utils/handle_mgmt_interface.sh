#!/bin/bash

#Due to the hardware design, as4630-54pe use "eth2" instead of "eth0" as management interface.
#Rename netdev "eth0" and "eth2" to swap original "eth2" to "eth0".
 
ifconfig eth0 down
ip link set eth0 name eth3
ip link set eth2 name eth0
ifconfig eth0 up
