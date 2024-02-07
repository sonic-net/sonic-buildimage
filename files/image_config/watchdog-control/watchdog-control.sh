#! /bin/bash

VERBOSE=no
WATCHDOG_UTIL="/usr/local/bin/watchdogutil"

function debug()
{
    /usr/bin/logger "$0 : $1"
    if [[ x"${VERBOSE}" == x"yes" ]]; then
        echo "$(date) $0: $1"
    fi
}


function getBootType()
{
    # same code snippet in files/scripts/syncd.sh
    case "$(cat /proc/cmdline)" in
    *SONIC_BOOT_TYPE=warm*)
        TYPE='warm'
        ;;
    *SONIC_BOOT_TYPE=fastfast*)
        TYPE='fastfast'
        ;;
    *SONIC_BOOT_TYPE=express*)
        TYPE='express'
        ;;
    *SONIC_BOOT_TYPE=fast*|*fast-reboot*)
        TYPE='fast'
        ;;
    *)
        TYPE='cold'
    esac
    echo "${TYPE}"
}

function disable_watchdog()
{
    # Obtain boot type from kernel arguments
    BOOT_TYPE=`getBootType`
    if [[ -x ${WATCHDOG_UTIL} ]]; then
        debug "Disabling Watchdog during bootup after $BOOT_TYPE"
        ${WATCHDOG_UTIL} disarm
    fi
}

disable_watchdog
