#!/bin/bash
# This script would run once kea-dhcp4 lease change (defined in kea-dhcp4.conf), 
# it is to find running process dhcpservd.py, and send SIGUSR1 signal to this
# process to inform it to update lease table in state_db (defined in dhcpservd.py)

pid=`ps aux | grep 'dhcpservd' | grep -nv 'grep' | awk '{print $2}'`
if [ -z "$pid" ]; then
    logger -p daemon.error Cannot find running dhcpservd.py.
else
    # Send SIGUSR1 signal to dhcpservd.py
    kill -s 10 ${pid}
fi
