#!/bin/bash

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
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

function validate_restore_count()
{
    if [[ x"$WARM_BOOT" == x"true" ]]; then
        RESTORE_COUNT=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_TABLE|bgp" restore_count`
        # We have to make sure db data has not been flushed.
        if [[ -z "$RESTORE_COUNT" ]]; then
            WARM_BOOT="false"
        fi
    fi
}

function check_fast_boot ()
{
    SYSTEM_FAST_REBOOT=`sonic-db-cli STATE_DB hget "FAST_RESTART_ENABLE_TABLE|system" enable`
    if [[ x"${SYSTEM_FAST_REBOOT}" == x"true" ]]; then
        FAST_BOOT="true"
    else
        FAST_BOOT="false"
    fi
}

start() {
    debug "Starting ${SERVICE}$DEV service..."

    check_warm_boot
    validate_restore_count

    check_fast_boot

    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    debug "Fast boot flag: ${SERVICE}$DEV ${Fast_BOOT}."

    # start service docker
    /usr/bin/${SERVICE}.sh start $DEV
    debug "Started ${SERVICE}$DEV service..."

}

wait() {
    /usr/bin/${SERVICE}.sh wait $DEV
}

stop() {
    debug "Stopping ${SERVICE}$DEV service..."

    check_warm_boot
    check_fast_boot
    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    debug "Fast boot flag: ${SERVICE}$DEV ${FAST_BOOT}."

    # Kill bgpd to start the bgp graceful restart procedure
    if [[ x"$WARM_BOOT" == x"true" ]] || [[ x"$FAST_BOOT" == x"true" ]]; then
        debug "Kill zebra first"
        /usr/bin/docker exec -i bgp pkill -9 zebra || [ $? == 1 ]
        /usr/bin/docker exec -i bgp pkill -9 bgpd || [ $? == 1 ]
    fi

    /usr/bin/${SERVICE}.sh stop $DEV
    debug "Stopped ${SERVICE}$DEV service..."

}

DEV=$2

SERVICE="bgp"
DEBUGLOG="/tmp/bgp-debug$DEV.log"
NAMESPACE_PREFIX="asic"
if [ "$DEV" ]; then
    NET_NS="$NAMESPACE_PREFIX$DEV" #name of the network namespace
    SONIC_DB_CLI="sonic-db-cli -n $NET_NS"
else
    NET_NS=""
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
