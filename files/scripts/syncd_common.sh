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
    /usr/bin/logger $1
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
    # Wait for redis server start before database clean
    until [[ $($SONIC_DB_CLI PING | grep -c PONG) -gt 0 ]]; do
      sleep 1;
    done

    # Wait for configDB initialization
    until [[ $($SONIC_DB_CLI CONFIG_DB GET "CONFIG_DB_INITIALIZED") -eq 1 ]];
        do sleep 1;
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

    # Create directories if they don't exist
    mkdir -p /host/warmboot
    mkdir -p /host/warmboot.bak
	
    if [ -n "$(ls -A /host/warmboot)" ]; then
	echo "/host/warmboot has contents."
	echo "Moving them to /host/warmboot.bak."
	mv /host/warmboot/* /host/warmboot.bak
    else
	echo "/host/warmboot is empty."
    fi

    # Check if /host/warmboot is already mounted on tmpfs
    if [[ "$(findmnt /host/warmboot -o FSTYPE -n)" != "tmpfs" ]]; then
	mount -t tmpfs tmpfs /host/warmboot
    fi

    # Check if a specific file exists in /pmem/
    if [ -f "/pmem/example_file.txt" ]; then
	echo "Found example_file.txt in /pmem/. Copying from /pmem/ to /host/warmboot."
	cp -r /pmem/* "/host/warmboot/."
    elif [ -n "$(ls -A /host/warmboot.bak)" ]; then
	echo "No example_file.txt found in /pmem/. Copying from /host/warmboot.bak/ to /host/warmboot."
	cp -r /host/warmboot.bak/* "/host/warmboot/."
    else
	echo "Neither example_file.txt found in /pmem/ nor /host/warmboot.bak/ has contents."
	echo "Nothing to copy to /host/warmboot."
    fi


    # case 1. logic correctly determine whether we are taking contents /pmem/ or /host/warmboot.bak, old state should not be restored
    # case 2. logic to determine which one to correctly use /pmem/ or /host/warmboot.bak if both contents
    # both the copies should not run in an ideal case, only 1 should be run 

    wait_for_database_service
    check_warm_boot

    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."

    if [[ x"$WARM_BOOT" == x"true" ]]; then
        # Leave a mark for syncd scripts running inside docker.
        touch /host/warmboot/warm-starting
    else
        rm -f /host/warmboot/warm-starting
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
