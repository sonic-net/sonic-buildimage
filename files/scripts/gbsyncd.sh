#!/bin/bash

. /usr/local/bin/syncd_common.sh

function startplatform() {

    declare -A DbCliArray=([0]=$SONIC_GLOBAL_DB_CLI [1]=$SONIC_DB_CLI)
    for DB_CLI in "${DbCliArray[@]}"; do
        # Add gbsyncd to FEATURE table, if not in. It did have same config as syncd.
        if [ -z $($DB_CLI CONFIG_DB HGET 'FEATURE|gbsyncd' state) ]; then
            local CMD="local r=redis.call('DUMP', KEYS[1]); redis.call('RESTORE', KEYS[2], 0, r)"
            $DB_CLI CONFIG_DB EVAL "$CMD" 2 'FEATURE|syncd' 'FEATURE|gbsyncd'
            $DB_CLI CONFIG_DB EVAL "$CMD" 2 'SYSLOG_CONFIG_FEATURE|syncd' 'SYSLOG_CONFIG_FEATURE|gbsyncd'
        fi
    done
}

function waitplatform() {
    :
}

function stopplatform1() {
    if ! docker top gbsyncd$DEV | grep -q /usr/bin/syncd; then
        debug "syncd process in container gbsyncd$DEV is not running"
        return
    fi

    # Invoke platform specific pre shutdown routine.
    PLATFORM=`$SONIC_DB_CLI CONFIG_DB hget 'DEVICE_METADATA|localhost' platform`
    PLATFORM_PRE_SHUTDOWN="/usr/share/sonic/device/$PLATFORM/plugins/gbsyncd_request_pre_shutdown"
    [ -f $PLATFORM_PRE_SHUTDOWN ] && \
        /usr/bin/docker exec -i gbsyncd$DEV /usr/share/sonic/platform/plugins/gbsyncd_request_pre_shutdown --${TYPE}

    debug "${TYPE} shutdown syncd process in container gbsyncd$DEV ..."
    /usr/bin/docker exec -i gbsyncd$DEV /usr/bin/syncd_request_shutdown -g 1 -x /usr/share/sonic/hwsku/context_config.json --${TYPE}

    # wait until syncd quits gracefully or force syncd to exit after
    # waiting for 20 seconds
    start_in_secs=${SECONDS}
    end_in_secs=${SECONDS}
    timer_threshold=20
    while docker top gbsyncd$DEV | grep -q /usr/bin/syncd \
            && [[ $((end_in_secs - start_in_secs)) -le $timer_threshold ]]; do
        sleep 0.1
        end_in_secs=${SECONDS}
    done

    if [[ $((end_in_secs - start_in_secs)) -gt $timer_threshold ]]; then
        debug "syncd process in container gbsyncd$DEV did not exit gracefully"
    fi

    /usr/bin/docker exec -i gbsyncd$DEV /bin/sync
    debug "Finished ${TYPE} shutdown syncd process in container gbsyncd$DEV ..."
}

function stopplatform2() {
    :
}

OP=$1
DEV=$2

SERVICE="$gbsyncd_platform"
PEER="swss"
DEBUGLOG="/tmp/swss-$SERVICE-debug$DEV.log"
LOCKFILE="/tmp/swss-$SERVICE-lock$DEV"
NAMESPACE_PREFIX="asic"
SONIC_GLOBAL_DB_CLI="sonic-db-cli"
SONIC_DB_CLI="sonic-db-cli"
if [ "$DEV" ]; then
    NET_NS="$NAMESPACE_PREFIX$DEV" #name of the network namespace
    SONIC_DB_CLI="sonic-db-cli -n $NET_NS"
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
