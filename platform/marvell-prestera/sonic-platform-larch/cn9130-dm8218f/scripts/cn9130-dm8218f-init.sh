#!/bin/bash

# Platform init script for cn9130-dm8218f

cn9130-dm8218f_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*cn9130-dm8218f* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "cn9130-dm8218f: Updating switch mac address ${MAC_ADDR}"
    sudo ifconfig eth0 hw ether $MAC_ADDR
}

# - Main entry
cn9130-dm8218f_profile

exit 0