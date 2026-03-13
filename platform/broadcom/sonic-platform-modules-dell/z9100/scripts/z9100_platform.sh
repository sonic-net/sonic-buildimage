#!/bin/bash

#platform init script for Dell Z9100

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

    systemctl enable z9100-lpc-monitor.service
    systemctl start --no-block z9100-lpc-monitor.service

  
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe dell_ich
    modprobe dell_mailbox
    modprobe dell_z9100_cpld
    systemctl start z9100-reboot-cause.service



    # Disable Watchdog Timer
    if [[ -e /usr/local/bin/platform_watchdog_disable.sh ]]; then
        /usr/local/bin/platform_watchdog_disable.sh
    fi

    systemctl start --no-block z9100-ssd-upgrade-status.service

    if [[ "$TYPE" == "cold" ]]; then
        systemctl start z9100-platform-startup.service
    else
        systemctl start --no-block z9100-platform-startup.service
    fi

elif [[ "$1" == "deinit" ]]; then
    /usr/local/bin/z9100_platform_startup.sh deinit

    modprobe -r dell_z9100_cpld
    modprobe -r dell_mailbox
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    modprobe -r dell_ich
else
     echo "z9100_platform : Invalid option !"
fi