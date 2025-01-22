#!/bin/bash

. /usr/local/bin/asic_status.sh

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUG_LOG}
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
        RESTORE_COUNT=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_TABLE|${SERVICE}" restore_count`
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

    # On supervisor card, skip starting asic related services here. In wait(),
    # wait until the asic is detected by pmon and published via database.
    if ! is_chassis_supervisor; then
        # start service docker
        /usr/bin/${SERVICE}.sh start $DEV
        debug "Started ${SERVICE}$DEV service..."
    fi
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

    /usr/bin/${SERVICE}.sh wait $DEV
}

stop() {
    debug "Stopping ${SERVICE}$DEV service..."

    check_warm_boot
    check_fast_boot
    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    debug "Fast boot flag: ${SERVICE}$DEV ${FAST_BOOT}."

    if [[ x"$WARM_BOOT" == x"true" ]]; then
        # Send USR1 signal to all teamd instances to stop them
        # It will prepare teamd for warm-reboot
        # Note: We must send USR1 signal before syncd, because it will send the last packet through CPU port
        docker exec -i ${SERVICE}$DEV pkill -USR1 -f ${TEAMD_CMD} > /dev/null || [ $? == 1 ]
    elif [[ x"$FAST_BOOT" == x"true" ]]; then
        # Kill teamd processes inside of teamd container with SIGUSR2 to allow them to send last LACP frames
        # We call `docker kill teamd` to ensure the container stops as quickly as possible,
        # Note: teamd must be killed before syncd, because it will send the last packet through CPU port
        docker exec -i ${SERVICE}$DEV pkill -USR2 -f ${TEAMD_CMD} || [ $? == 1 ]
    fi

    if [[ x"$WARM_BOOT" == x"true" ]] || [[ x"$FAST_BOOT" == x"true" ]]; then
        while docker exec -i ${SERVICE}$DEV pgrep -f ${TEAMD_CMD} > /dev/null; do
            sleep 0.05
        done
        docker kill ${SERVICE}$DEV  &> /dev/null || debug "Docker ${SERVICE}$DEV is not running ($?) ..."
    else
        /usr/bin/${SERVICE}.sh stop $DEV
    fi

    debug "Stopped ${SERVICE}$DEV service..."
}

DEV=$2

SERVICE="teamd"
TEAMD_CMD="/usr/bin/teamd"
DEBUG_LOG="/tmp/teamd-debug$DEV.log"
NAMESPACE_PREFIX="asic"
if [ "$DEV" ]; then
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
