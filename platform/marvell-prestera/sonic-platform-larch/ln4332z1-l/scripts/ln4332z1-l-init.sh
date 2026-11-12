#!/bin/bash

# Platform init script for ln4332z1-l

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment for ln4332z1-l"
    FW_ENV_DEFAULT='/dev/mtd1 0x100000 0x100000 0x40000 4'
    echo $FW_ENV_DEFAULT > /etc/fw_env.config
}

ln4332z1-l_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*ln4332z1-l* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln4332z1-l: Updating switch mac address ${MAC_ADDR}"
}

#setting up uboot environment
fw_uboot_env_cfg

# - Main entry
ln4332z1-l_profile

exit 0
