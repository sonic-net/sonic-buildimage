#!/usr/bin/env bash

HWSKU_DIR=/usr/share/sonic/hwsku

mkdir -p /etc/sai.d/

# Create/Copy the psai.profile to /etc/sai.d/psai.profile
if [ -f $HWSKU_DIR/psai.profile.j2 ]; then
    sonic-cfggen -d -t $HWSKU_DIR/psai.profile.j2 > /etc/sai.d/psai.profile
else
    if [ -f $HWSKU_DIR/psai.profile ]; then
        cp $HWSKU_DIR/psai.profile /etc/sai.d/psai.profile
    fi
fi
