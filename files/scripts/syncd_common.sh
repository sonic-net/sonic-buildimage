#!/bin/bash

#
# common functions used by "syncd" scipts (syncd.sh, gbsyncd.sh, etc..)
# scripts using this must provide implementations of the following functions:
#
# startplatform
# waitplatform
# stopplatform1 and stopplatform2
#
# For examples of these, see gbsyncd.sh and syncd.sh.
#

. /usr/local/bin/asic_status.sh

function debug()
{
    # Use --id=$$ so all messages from this script share the parent shell's PID,
    # preventing rsyslog imuxsock ratelimiter memory growth.
    /usr/bin/logger --id=$$ -- "$1"
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

function lock_service_state_change()
{
    debug "Locking ${LOCKFILE} from ${SERVICE}$DEV service"

    exec {LOCKFD}>${LOCKFILE}
    /usr/bin/flock -x ${LOCKFD}
    trap "/usr/bin/flock -u ${LOCKFD}" EXIT

    debug "Locked ${LOCKFILE} (${LOCKFD}) from ${SERVICE}$DEV service"
}

function unlock_service_state_change()
{
    debug "Unlocking ${LOCKFILE} (${LOCKFD}) from ${SERVICE}$DEV service"
    /usr/bin/flock -u ${LOCKFD}
}

function check_warm_boot()
{
    SYSTEM_WARM_START=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
    SERVICE_WARM_START=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_ENABLE_TABLE|${SERVICE}" enable`
    # SYSTEM_WARM_START could be empty, always make WARM_BOOT meaningful.
    if [[ x"$SYSTEM_WARM_START" == x"true" ]] || [[ x"$SERVICE_WARM_START" == x"true" ]]; then
        WARM_BOOT="true"
    else
        WARM_BOOT="false"
    fi
}

function check_fast_boot()
{
    SYSTEM_FAST_REBOOT=`sonic-db-cli STATE_DB hget "FAST_RESTART_ENABLE_TABLE|system" enable`
    if [[ x"${SYSTEM_FAST_REBOOT}" == x"true" ]]; then
        FAST_BOOT="true"
    else
        FAST_BOOT="false"
    fi
}

function wait_for_database_service()
{
    local timeout_sec=${DB_READY_TIMEOUT_SEC:-120}
    local poll_sec=${DB_READY_POLL_INTERVAL_SEC:-1}
    local elapsed=0

    [[ "$timeout_sec" =~ ^[0-9]+$ ]] || timeout_sec=120
    [[ "$poll_sec" =~ ^[0-9]+$ ]] || poll_sec=1
    if (( poll_sec <= 0 )); then
        poll_sec=1
    fi

    # Wait for redis server start before database clean
    while [[ $($SONIC_DB_CLI PING | grep -c PONG) -le 0 ]]; do
        if (( elapsed >= timeout_sec )); then
            debug "Timed out waiting for redis PING after ${elapsed}s (timeout=${timeout_sec}s)"
            return 1
        fi
        sleep "$poll_sec"
        elapsed=$((elapsed + poll_sec))
    done

    elapsed=0
    # Wait for configDB initialization
    while [[ $($SONIC_DB_CLI CONFIG_DB GET "CONFIG_DB_INITIALIZED") -ne 1 ]]; do
        if (( elapsed >= timeout_sec )); then
            debug "Timed out waiting for CONFIG_DB_INITIALIZED after ${elapsed}s (timeout=${timeout_sec}s)"
            return 1
        fi
        sleep "$poll_sec"
        elapsed=$((elapsed + poll_sec))
    done
}

function getBootType()
{
    # same code snippet in files/build_templates/docker_image_ctl.j2
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
        # check that the key exists
        SYSTEM_FAST_REBOOT=`sonic-db-cli STATE_DB hget "FAST_RESTART_ENABLE_TABLE|system" enable`
        if [[ x"${SYSTEM_FAST_REBOOT}" == x"true" ]]; then
            TYPE='fast'
        else
            TYPE='cold'
        fi
        ;;
    *)
        TYPE='cold'
    esac
    echo "${TYPE}"
}

start() {
    debug "Starting ${SERVICE}$DEV service..."

    lock_service_state_change

    mkdir -p /host/warmboot$DEV

    if ! wait_for_database_service; then
        debug "Database readiness check failed for ${SERVICE}$DEV"
        unlock_service_state_change
        return 1
    fi
    check_warm_boot

    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."

    if [[ x"$WARM_BOOT" == x"true" ]]; then
        # Leave a mark for syncd scripts running inside docker.
        touch /host/warmboot$DEV/warm-starting
    else
        rm -f /host/warmboot$DEV/warm-starting
    fi

    startplatform

    # On supervisor card, skip starting asic related services here. In wait(),
    # wait until the asic is detected by pmon and published via database.
    if ! is_chassis_supervisor; then
        # start service docker
        /usr/bin/${SERVICE}.sh start $DEV
        debug "Started ${SERVICE}$DEV service..."
    fi

    unlock_service_state_change
}

wait() {
    # On supervisor card, wait for asic to be online before starting the docker.
    if is_chassis_supervisor; then
        check_asic_status
        ASIC_STATUS=$?

        # start service docker
        if [[ $ASIC_STATUS == 0 ]]; then
            /usr/bin/${SERVICE}.sh start $DEV
            debug "Started ${SERVICE}$DEV service..."
        fi
    fi

    waitplatform

    /usr/bin/${SERVICE}.sh wait $DEV
}

stop() {
    debug "Stopping ${SERVICE}$DEV service..."

    lock_service_state_change
    check_warm_boot
    check_fast_boot
    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    debug "Fast boot flag: ${SERVICE}$DEV ${FAST_BOOT}."

    if [[ x"$WARM_BOOT" == x"true" ]]; then
        TYPE=warm
    elif [[ x"$FAST_BOOT" == x"true" ]]; then
        TYPE=fast
    else
        TYPE=cold
    fi

    stopplatform1

    /usr/bin/${SERVICE}.sh stop $DEV
    debug "Stopped ${SERVICE}$DEV service..."

    stopplatform2

    unlock_service_state_change
}
