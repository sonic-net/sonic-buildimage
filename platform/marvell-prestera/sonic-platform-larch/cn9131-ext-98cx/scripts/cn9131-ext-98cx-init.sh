#!/bin/bash

# Platform init script for cn9131-ext-98cx

cn9131-ext-98cx_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*cn9131-ext-98cx* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "cn9131-ext-98cx: Updating switch mac address ${MAC_ADDR}"
}

# - Main entry
cn9131-ext-98cx_profile

exit 0
