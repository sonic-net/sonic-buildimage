#!/bin/bash

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment..."
    MACH_FILE="/host/machine.conf"
    PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' $MACH_FILE`
}

es2227_54ts_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*es2227_54ts* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "es2227_54ts: Updating switch mac address ${MAC_ADDR}"
}

update_modulelist()
{
    MODULE_FILE="/etc/modules-load.d/marvell.conf"

    echo "# Module list to load during the boot" > $MODULE_FILE
    echo "mvcpss" >> $MODULE_FILE
    echo "psample" >> $MODULE_FILE
}

main()
{
    fw_uboot_env_cfg
    es2227_54ts_profile
    update_modulelist
}

main $@
