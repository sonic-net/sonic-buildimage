#!/bin/bash

# Platform init script for s6500-32x

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment for cn9130"
    FW_ENV_DEFAULT='/dev/mtd1 0x140000 0x40000 0x40000'
    echo $FW_ENV_DEFAULT > /etc/fw_env.config
}

s6500-32x_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*s6500_32x* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "s6500_32x: Updating switch mac address ${MAC_ADDR}"
}

#setting up uboot environment
fw_uboot_env_cfg

# - Main entry
s6500-32x_profile

exit 0
