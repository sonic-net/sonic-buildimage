#!/bin/bash

#platform init script for Dell S6100

if [[ "$1" == "init" ]]; then
    depmod -a
    case "$(cat /proc/cmdline)" in
        *SONIC_BOOT_TYPE=warm*)
            TYPE='warm'
            ;;
        *SONIC_BOOT_TYPE=fastfast*)
            TYPE='fastfast'
            ;;
        *SONIC_BOOT_TYPE=fast*|*fast-reboot*)
            TYPE='fast'
            ;;
        *SONIC_BOOT_TYPE=soft*)
            TYPE='soft'
            ;;
        *)
            TYPE='cold'
    esac

    if [[ "$TYPE" == "cold" ]]; then
        /usr/local/bin/iom_power_on.sh
    fi

    systemctl enable s6100-lpc-monitor.service
    systemctl start --no-block s6100-lpc-monitor.service

    pericom="/sys/bus/pci/devices/0000:08:00.0"
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe dell_ich
    modprobe dell_s6100_iom_cpld
    modprobe dell_s6100_lpc
    modprobe nvram
    systemctl start s6100-reboot-cause.service

    # Disable pericom/xilinx
    echo 1 > /sys/bus/pci/devices/0000:02:00.0/remove
    [ -d $pericom ] &&  echo 1 > $pericom/remove

    # Disable Watchdog Timer
    if [[ -e /usr/local/bin/platform_watchdog_disable.sh ]]; then
        /usr/local/bin/platform_watchdog_disable.sh
    fi

    systemctl start --no-block s6100-ssd-upgrade-status.service

    if [[ "$TYPE" == "cold" ]]; then
        systemctl start s6100-platform-startup.service
    else
        systemctl start --no-block s6100-platform-startup.service
    fi

elif [[ "$1" == "deinit" ]]; then
    /usr/local/bin/s6100_platform_startup.sh deinit

    modprobe -r dell_s6100_lpc
    modprobe -r dell_s6100_iom_cpld
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    modprobe -r dell_ich
    modprobe -r nvram
else
     echo "s6100_platform : Invalid option !"
fi
