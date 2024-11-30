#!/bin/bash

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

function lock_service_state_change()
{
    LOCKFILE=$1
    SERVICE=$2
    DEV=$3

    debug "Locking ${LOCKFILE} from ${SERVICE}$DEV service"

    exec {LOCKFD}>${LOCKFILE}
    /usr/bin/flock -x ${LOCKFD}
    trap "/usr/bin/flock -u ${LOCKFD}" 0 2 3 15

    debug "Locked ${LOCKFILE} (${LOCKFD}) from ${SERVICE}$DEV service"
}

function unlock_service_state_change()
{
    LOCKFILE=$1
    SERVICE=$2
    DEV=$3

    debug "Unlocking ${LOCKFILE} (${LOCKFD}) from ${SERVICE}$DEV service"
    /usr/bin/flock -u ${LOCKFD}
}

function check_warm_boot()
{
    SERVICE=$1
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
    PROCESS=$1
    if [[ x"$WARM_BOOT" == x"true" ]]; then
        RESTORE_COUNT=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_TABLE|${PROCESS}" restore_count`
        # We have to make sure db data has not been flushed.
        if [[ -z "$RESTORE_COUNT" ]]; then
            WARM_BOOT="false"
        fi
    fi
}

function wait_for_database_service()
{
    # Wait for redis server start before database clean
    until [[ $($SONIC_DB_CLI PING | grep -c PONG) -gt 0 ]]; do
      sleep 1;
    done

    # Wait for configDB initialization
    until [[ $($SONIC_DB_CLI CONFIG_DB GET "CONFIG_DB_INITIALIZED") ]];
        do sleep 1;
    done
}
