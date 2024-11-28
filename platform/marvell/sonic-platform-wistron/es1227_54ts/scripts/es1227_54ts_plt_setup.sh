#!/bin/bash

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment..."
    MACH_FILE="/host/machine.conf"
    PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' $MACH_FILE`
}

es1227_54ts_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*es1227_54ts* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "es1227_54ts: Updating switch mac address ${MAC_ADDR}"
}

update_modulelist()
{
    MODULE_FILE="/etc/modules-load.d/marvell.conf"

    echo "# Module list to load during the boot" > $MODULE_FILE
    echo "mvcpss" >> $MODULE_FILE
    echo "psample" >> $MODULE_FILE
}

haveged_service_cfg()
{
    # Workaround for failing haveged.service
    sed -i 's/^DAEMON_ARGS=.*/DAEMON_ARGS="-w 1024 -d 16"/' /etc/default/haveged
}

main()
{
    fw_uboot_env_cfg
    es1227_54ts_profile
    update_modulelist
    haveged_service_cfg
}

main $@
