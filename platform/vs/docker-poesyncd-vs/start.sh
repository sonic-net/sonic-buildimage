#!/usr/bin/env bash

HWSKU_DIR=/usr/share/sonic/hwsku

mkdir -p /etc/sai.d/

# Create/Copy the poe.profile to /etc/sai.d/poe.profile
if [ -f $HWSKU_DIR/poe.profile.j2 ]; then
    sonic-cfggen -d -t $HWSKU_DIR/poe.profile.j2 > /etc/sai.d/poe.profile
else
    if [ -f $HWSKU_DIR/poe.profile ]; then
        cp $HWSKU_DIR/poe.profile /etc/sai.d/poe.profile
    fi
fi
