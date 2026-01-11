#!/usr/bin/env bash

HWSKU_DIR=/usr/share/sonic/hwsku

SYNCD_SOCKET_FILE=/var/run/sswsyncd/sswsyncd.socket

# Remove stale files if they exist
rm -f ${SYNCD_SOCKET_FILE}

mkdir -p /etc/sai.d/

# There are two ways to specify the contents of the SAI_INIT_CONFIG_FILE and they are mutually exclusive
# via current method (sai.profile.j2) or new method (config.bcm.j2)
# If delta is large, use sai.profile.j2 which basically require the user to select which config file to use
# If delta is small, use config.bcm.j2 where additional SAI INIT config properties are added
#   based on specific device metadata requirement
#   in this case sai.profile should have been modified to use the path /etc/sai.d/config.bcm
# There is also a possibility that both sai.profile.j2 and config.bcm.j2 are absent.  in that cacse just copy
# sai.profile to the new /etc/said directory.

# Create/Copy the sai.profile to /etc/sai.d/sai.profile
if [ -f $HWSKU_DIR/sai.profile.j2 ]; then
    sonic-cfggen -d -t $HWSKU_DIR/sai.profile.j2 > /etc/sai.d/sai.profile
else
    if [ -f $HWSKU_DIR/config.bcm.j2 ]; then
        sonic-cfggen -d -t $HWSKU_DIR/config.bcm.j2 > /etc/sai.d/config.bcm
    fi
    if [ -f $HWSKU_DIR/sai.profile ]; then
        cp $HWSKU_DIR/sai.profile /etc/sai.d/sai.profile
    fi
fi
