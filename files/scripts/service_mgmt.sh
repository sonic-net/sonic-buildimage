#!/bin/bash

function debug()
{
    /usr/bin/logger $1
}

function check_warm_boot()
{
    SYSTEM_WARM_START=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
    SERVICE_WARM_START=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_ENABLE_TABLE|${SERVICE}" enable`
    if [[ x"$SYSTEM_WARM_START" == x"true" ]] || [[ x"$SERVICE_WARM_START" == x"true" ]]; then
        WARM_BOOT="true"
    else
        WARM_BOOT="false"
    fi
}

function check_fast_boot ()
{
    SYSTEM_FAST_REBOOT=`$SONIC_DB_CLI STATE_DB hget "FAST_RESTART_ENABLE_TABLE|system" enable`
    if [[ x"${SYSTEM_FAST_REBOOT}" == x"true" ]]; then
        FAST_BOOT="true"
    else
        FAST_BOOT="false"
    fi
}

function check_redundant_type()
{
    DEVICE_SUBTYPE=`$SONIC_DB_CLI CONFIG_DB hget "DEVICE_METADATA|localhost" subtype`
    if [[ x"$DEVICE_SUBTYPE" == x"DualToR" ]]; then
        MUX_CONFIG=`show muxcable config`
        if [[ $MUX_CONFIG =~ .*active-active.* ]]; then 
            ACTIVE_ACTIVE="true"
        else
            ACTIVE_ACTIVE="false"
        fi
    else
        ACTIVE_ACTIVE="false"
    fi
    CONFIG_KNOB=`$SONIC_DB_CLI CONFIG_DB hget "MUX_LINKMGR|SERVICE_MGMT" kill_radv`
    if [[ x"$CONFIG_KNOB" == x"False" ]]; then
        ACTIVE_ACTIVE='false'
    fi 
    debug "DEVICE_SUBTYPE: ${DEVICE_SUBTYPE}, CONFIG_KNOB: ${CONFIG_KNOB}"
}

function handle_interrupted_start() {
    # If this wrapper is killed (e.g. `systemctl stop sonic.target` during a
    # `config reload`) while still inside start(), the unit never reaches "active"
    # and systemd will not run ExecStop - so stop() below is skipped even though
    # `/usr/bin/${SERVICE}.sh start` may have already left the container running.
    # Stop it here instead of leaving an orphaned, unmanaged container (and its
    # main process) running.
    debug "Start of ${SERVICE}$DEV interrupted by signal, checking for an orphaned container..."
    if [ -n "$(docker ps -q --filter "name=^/${SERVICE}${DEV}$" --filter "status=running")" ]; then
        debug "${SERVICE}$DEV container is running after interrupted start, stopping it..."
        stop
    fi
    # Conventional shell exit code for death-by-signal: 128 + signal number.
    exit $((128 + $(kill -l TERM)))
}

start() {
    debug "Starting ${SERVICE}$DEV service..."

    trap handle_interrupted_start TERM

    # start service docker
    /usr/bin/${SERVICE}.sh start $DEV
    debug "Started ${SERVICE}$DEV service..."

    trap - TERM
}

wait() {
    /usr/bin/${SERVICE}.sh wait $DEV
}

stop() {
    debug "Stopping ${SERVICE}$DEV service..."

    check_warm_boot
    check_fast_boot
    check_redundant_type
    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    debug "Fast boot flag: ${SERVICE}$DEV ${FAST_BOOT}."

    # For WARM/FAST boot do not perform service stop
    if [[ x"$WARM_BOOT" != x"true" ]] && [[ x"$FAST_BOOT" != x"true" ]]; then
        if [[ x"$SERVICE" == x"radv" ]] && [[ x"$ACTIVE_ACTIVE" == x"true" ]]; then
            debug "Killing Docker ${SERVICE}${DEV} for active-active dualtor device..."
            /usr/bin/${SERVICE}.sh kill $DEV
        else
            /usr/bin/${SERVICE}.sh stop $DEV
            debug "Stopped ${SERVICE}$DEV service..."
        fi
    else
        debug "Killing Docker ${SERVICE}${DEV}..."
        /usr/bin/${SERVICE}.sh kill $DEV
    fi
}

DEV=$2

SCRIPT_NAME=$(basename -- "$0")
SERVICE="${SCRIPT_NAME%.*}"
NAMESPACE_PREFIX="asic"
if [[ "$DEV" && "$DEV" != *"dpu"* ]]; then
    NET_NS="$NAMESPACE_PREFIX$DEV" #name of the network namespace
    SONIC_DB_CLI="sonic-db-cli -n $NET_NS"
else
    SONIC_DB_CLI="sonic-db-cli"
fi

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
