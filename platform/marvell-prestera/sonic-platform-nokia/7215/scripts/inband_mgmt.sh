#!/bin/sh

#inband_mgmt

inband_mgmt(){
 # In this platform, one of the network ports is used as mgmt interface.
 # This script periodically monitors inband management port eth0 and
 # assigns IP address to eth0 if needed.
 if [ ! -f /host/machine.conf ]; then
     exit 0
 fi
 #wait for n/w port init to complete
 sleep 60
 while :; do
   ip -br link show eth0 2> /dev/null
   if [ $? -eq 0 ]; then
       ip address show eth0 | grep -qw "inet" 2>/dev/null
       if [ $? -ne 0 ]; then
           ifconfig eth0 down
           systemctl restart networking
       fi
       sleep 120
   else
     sleep 3
   fi
 done
}
(inband_mgmt > /dev/null)&
