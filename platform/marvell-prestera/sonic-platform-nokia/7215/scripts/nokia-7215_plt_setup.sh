#!/bin/bash

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment..."

    MACH_FILE="/host/machine.conf"
    PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' $MACH_FILE`

    if [ "$PLATFORM" = "armhf-nokia_ixs7215_52x-r0" ]; then
	# Ixs7215 / IPD6448M board Uboot ENV offset
        FW_ENV_DEFAULT='/dev/mtd0 0x00100000 0x10000 0x10000'
    else
        FW_ENV_DEFAULT='/dev/mtd0 0x00500000 0x80000 0x100000 8'
    fi

    echo "Using pre-configured uboot env"
    echo $FW_ENV_DEFAULT > /etc/fw_env.config

}


main()
{
    fw_uboot_env_cfg
    echo "Nokia-IXS7215: /dev/mtd0 FW_ENV_DEFAULT"

    /bin/sh /etc/inband_mgmt.sh
}

main $@
