#!/bin/bash

# Platform init script for ln3516xgf-4z

ln3516xgf-4z_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*ln3516xgf-4z* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln3516xgf-4z: Updating switch mac address ${MAC_ADDR}"
    sudo ifconfig eth0 hw ether $MAC_ADDR

    sudo modprobe optoe
}

# - Main entry
ln3516xgf-4z_profile

exit 0
