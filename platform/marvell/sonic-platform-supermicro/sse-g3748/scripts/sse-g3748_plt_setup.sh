#!/bin/bash

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment..."
    MACH_FILE="/host/machine.conf"
    PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' $MACH_FILE`

    FW_ENV_DEFAULT='/dev/mtd1   0  0x10000  0x10000'

    echo $FW_ENV_DEFAULT
    echo $FW_ENV_DEFAULT > /etc/fw_env.config
}

sse_g3748_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*sse_g3748* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "SSE-G3748 Updating switch mac address ${MAC_ADDR}"
}

haveged_service_cfg()
{
    # Workaround for failing haveged.service
    sed -i 's/^DAEMON_ARGS=.*/DAEMON_ARGS="-w 1024 -d 16"/' /etc/default/haveged
}

main()
{
    echo "Executing sse-g3748_plt_setup.sh ....."
    fw_uboot_env_cfg
    sse_g3748_profile
    haveged_service_cfg
}

main $@
