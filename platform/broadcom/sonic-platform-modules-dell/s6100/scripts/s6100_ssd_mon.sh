#!/bin/bash

SSD_FW_UPGRADE="/host/ssd_fw_upgrade"

if [ -e $SSD_FW_UPGRADE/GPIO7_high ]; then
    iSMART="/usr/local/bin/iSMART_64"
    iSMART_OPTIONS="-d /dev/sda"

    iSMART_CMD=`$iSMART $iSMART_OPTIONS`
    GPIO_STATUS=$(echo "$iSMART_CMD" | grep GPIO | awk '{print $NF}')

    if [ $GPIO_STATUS != "0x01" ];then
        logger -p user.crit -t DELL_S6100_SSD_MON "The SSD on this unit is faulty. Do not power-cycle/reboot this unit!"
        logger -p user.crit -t DELL_S6100_SSD_MON "soft-/fast-/warm-reboot is allowed."
        rm -rf $SSD_FW_UPGRADE/GPIO7_*
        touch $SSD_FW_UPGRADE/GPIO7_low
        systemctl stop s6100-ssd-monitor.timer
    fi
else
    systemctl stop s6100-ssd-monitor.timer
fi
