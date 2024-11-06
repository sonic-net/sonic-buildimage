#!/bin/bash -e

CURRENT_HOSTNAME=$(hostname)
HOSTNAME=$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' hostname)

if [ -z "$HOSTNAME" ] ; then
       echo "Missing hostname in the config file, setting to default 'sonic'"
       HOSTNAME='sonic'
fi

echo $HOSTNAME > /etc/hostname
hostname -F /etc/hostname

#Don't update the /etc/hosts if hostname is not changed
#This is to prevent intermittent redis_chassis.server reachability issue
if [ $CURRENT_HOSTNAME == $HOSTNAME ] ;  then
    exit 0
fi

# Remove the old hostname entry from hosts file.
# But, 'localhost' entry is used by multiple applications. Don't remove it altogether.
# Edit contents of /etc/hosts and put in /etc/hosts.new
if [ $CURRENT_HOSTNAME  != "localhost" ] ;  then
    sed "/\s$CURRENT_HOSTNAME$/d" /etc/hosts > /etc/hosts.new
else
    cp -f /etc/hosts /etc/hosts.new
fi

echo "127.0.0.1 $HOSTNAME" >> /etc/hosts.new

# Swap file: hosts.new and hosts
mv -f /etc/hosts     /etc/hosts.old
mv -f /etc/hosts.new /etc/hosts
