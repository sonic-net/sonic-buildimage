#!/bin/bash

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment..."
    MACH_FILE="/host/machine.conf"
    PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' $MACH_FILE`

    FW_ENV_DEFAULT='/dev/mtd0 0x400000 0x10000 0x10000'

    echo $FW_ENV_DEFAULT > /etc/fw_env.config
}

update_modulelist()
{
    MODULE_FILE="/etc/modules-load.d/marvell.conf"
    if grep -Fxq "mvMbusDrv" $MODULE_FILE
    then
        echo "Module list up to date"
    else
        echo "mvMbusDrv" >> $MODULE_FILE
        modprobe mvMbusDrv
    fi

}

main()
{
    fw_uboot_env_cfg
    update_modulelist
}

main $@
