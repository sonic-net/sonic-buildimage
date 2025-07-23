#! /bin/bash

VERBOSE=no
WATCHDOG_UTIL="/usr/local/bin/watchdogutil"

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment
PLATFORM=${PLATFORM:-$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)}
PLATFORM_JSON="/usr/share/sonic/device/$PLATFORM/platform.json"

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

function is_smart_switch_dpu()
{
    if [[ ! -f "$PLATFORM_JSON" ]]; then
        return 1
    fi

    if jq -e 'has("DPU")' "$PLATFORM_JSON" > /dev/null; then
        return 0
    else
        return 1
    fi
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

function enable_watchdog()
{
    if [[ -x ${WATCHDOG_UTIL} ]]; then
        debug "Enabling Watchdog"
        ${WATCHDOG_UTIL} arm
    fi
}

if is_smart_switch_dpu; then
    # Keep watchdog enabled for smart switch DPUs
    # Smart switch DPUs uses ARM SBSA Generic Watchdog,
    # which is capable of monitoring the system in runtime.
    enable_watchdog
else
    # Disable watchdog for all other platforms
    disable_watchdog
fi
