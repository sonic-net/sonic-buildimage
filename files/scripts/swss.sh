#!/bin/bash

DEV=$2

SERVICE="swss"
PEER="syncd"
DEBUGLOG="/tmp/swss-syncd-debug$DEV.log"
LOCKFILE="/tmp/swss-syncd-lock$DEV"
NAMESPACE_PREFIX="asic"
ETC_SONIC_PATH="/etc/sonic/"


. /usr/local/bin/asic_status.sh

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

function read_dependent_services()
{
    # Update dependent list based on other packages requirements
    if [[ -f ${ETC_SONIC_PATH}/${SERVICE}_dependent ]]; then
        DEPENDENT="${DEPENDENT} $(cat ${ETC_SONIC_PATH}/${SERVICE}_dependent)"
    fi

    if [[ -f ${ETC_SONIC_PATH}/${SERVICE}_multi_inst_dependent ]]; then
        MULTI_INST_DEPENDENT="${MULTI_INST_DEPENDENT} $(cat ${ETC_SONIC_PATH}/${SERVICE}_multi_inst_dependent)"
    fi
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

function validate_restore_count()
{
    if [[ x"$WARM_BOOT" == x"true" ]]; then
        RESTORE_COUNT=`$SONIC_DB_CLI STATE_DB hget "WARM_RESTART_TABLE|orchagent" restore_count`
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
    until [[ $($SONIC_DB_CLI CONFIG_DB GET "CONFIG_DB_INITIALIZED") -eq 1 ]];
        do sleep 1;
    done
}

# This function cleans up the tables with specific prefixes from the database
# $1 the index of the database
# $2 the string of a list of table prefixes
function clean_up_tables()
{
    $SONIC_DB_CLI $1 EVAL "
    local tables = {$2}
    for i = 1, table.getn(tables) do
        local matches = redis.call('KEYS', tables[i])
        for j,name in ipairs(matches) do
            redis.call('DEL', name)
        end
    end" 0
}

# This function cleans up the chassis db table entries created ONLY by this asic
# This is used to do the clean up operation when the line card / asic reboots
# When the asic/lc is RE-booting, the chassis db server is supposed to be running 
# in the supervisor.  So the clean up is done when only the chassis db connectable. 
# Otherwise no need to do the clean up since both the supervisor and line card may be 
# rebooting (the whole chassis scenario)
# The clean up operation is required to delete only those entries created by
# the asic that is rebooted. Entries from the following tables are deleted in the order
# given below
#   (1) SYSTEM_NEIGH
#   (2) SYSTEM_INTERFACE
#   (3) SYSTEM_LAG_MEMBER_TABLE
#   (4) SYSTEM_LAG_TABLE
#   (5) The corresponding LAG IDs of the entries from SYSTEM_LAG_TABLE
#       SYSTEM_LAG_ID_TABLE and SYSTEM_LAG_ID_SET are adjusted appropriately
function clean_up_chassis_db_tables()
{
    if [[ !($($SONIC_DB_CLI CHASSIS_APP_DB PING | grep -c True) -gt 0) ]]; then
        return
    fi

    lc=`$SONIC_DB_CLI CONFIG_DB  hget 'DEVICE_METADATA|localhost' 'hostname'`
    asic=`$SONIC_DB_CLI CONFIG_DB  hget 'DEVICE_METADATA|localhost' 'asic_name'`
    switch_type=`$SONIC_DB_CLI CONFIG_DB  hget 'DEVICE_METADATA|localhost' 'switch_type'`

    # Run clean up only in swss running for voq switches
    if is_chassis_supervisor || [[ $switch_type != 'voq' ]]; then
        return
    fi

    # First, delete SYSTEM_NEIGH entries
    $SONIC_DB_CLI CHASSIS_APP_DB EVAL "
    local host = string.gsub(ARGV[1], '%-', '%%-')
    local dev = ARGV[2]
    local ps = 'SYSTEM_NEIGH*|' .. host .. '|' .. dev
    local keylist = redis.call('KEYS', 'SYSTEM_NEIGH*')
    for j,key in ipairs(keylist) do
        if string.match(key, ps) ~= nil then
            redis.call('DEL', key)
        end
    end
    return " 0 $lc $asic

    # Wait for some time before deleting system interface so that the system interface's "object in use"
    # is cleared in both orchangent and in syncd. Without this delay, the orchagent clears the refcount
    # but the syncd (meta) still has no-zero refcount. Because of this, orchagent gets "object still in use"
    # error and aborts.

    sleep 30

    # Next, delete SYSTEM_INTERFACE entries
    $SONIC_DB_CLI CHASSIS_APP_DB EVAL "
    local host = string.gsub(ARGV[1], '%-', '%%-')
    local dev = ARGV[2]
    local ps = 'SYSTEM_INTERFACE*|' .. host .. '|' .. dev
    local keylist = redis.call('KEYS', 'SYSTEM_INTERFACE*')
    for j,key in ipairs(keylist) do
        if string.match(key, ps) ~= nil then
            redis.call('DEL', key)
        end
    end
    return " 0 $lc $asic

    # Next, delete SYSTEM_LAG_MEMBER_TABLE entries
    $SONIC_DB_CLI CHASSIS_APP_DB EVAL "
    local host = string.gsub(ARGV[1], '%-', '%%-')
    local dev = ARGV[2]
    local ps = 'SYSTEM_LAG_MEMBER_TABLE*|' .. host .. '|' .. dev
    local keylist = redis.call('KEYS', 'SYSTEM_LAG_MEMBER_TABLE*')
    for j,key in ipairs(keylist) do
        if string.match(key, ps) ~= nil then
            redis.call('DEL', key)
        end
    end
    return " 0 $lc $asic

    # Wait for some time before deleting system lag so that the all the memebers of the
    # system lag will be cleared. 

    sleep 15

    # Finally, delete SYSTEM_LAG_TABLE entries and deallot LAG IDs
    $SONIC_DB_CLI CHASSIS_APP_DB EVAL "
    local host = string.gsub(ARGV[1], '%-', '%%-')
    local dev = ARGV[2]
    local ps = 'SYSTEM_LAG_TABLE*|' .. '(' .. host .. '|' .. dev ..'.*' .. ')'
    local keylist = redis.call('KEYS', 'SYSTEM_LAG_TABLE*')
    for j,key in ipairs(keylist) do
        local lagname = string.match(key, ps)
        if lagname ~= nil then
            redis.call('DEL', key)
            local lagid = redis.call('HGET', 'SYSTEM_LAG_ID_TABLE', lagname)
            redis.call('SREM', 'SYSTEM_LAG_ID_SET', lagid)
            redis.call('HDEL', 'SYSTEM_LAG_ID_TABLE', lagname)
        end
    end
    return " 0 $lc $asic

}

start_peer_and_dependent_services() {
    check_warm_boot

    if [[ x"$WARM_BOOT" != x"true" ]]; then
        for peer in ${PEER}; do
            if [[ ! -z $DEV ]]; then
                /bin/systemctl start ${peer}@$DEV
            else
               /bin/systemctl start ${peer}
            fi
        done
        for dep in ${DEPENDENT}; do
            /bin/systemctl start ${dep}
        done
        for dep in ${MULTI_INST_DEPENDENT}; do
            if [[ ! -z $DEV ]]; then
                /bin/systemctl start ${dep}@$DEV
            else
                /bin/systemctl start ${dep}
            fi
        done
    fi
}

stop_peer_and_dependent_services() {
    # if warm/fast start enabled or peer lock exists, don't stop peer service docker
    if [[ x"$WARM_BOOT" != x"true" ]] && [[ x"$FAST_BOOT" != x"true" ]]; then
        for dep in ${MULTI_INST_DEPENDENT}; do
            if [[ ! -z $DEV ]]; then
                /bin/systemctl stop ${dep}@$DEV
            else
                /bin/systemctl stop ${dep}
            fi
        done
        for dep in ${DEPENDENT}; do
            /bin/systemctl stop ${dep}
        done
        for peer in ${PEER}; do
            if [[ ! -z $DEV ]]; then
                /bin/systemctl stop ${peer}@$DEV
            else
                /bin/systemctl stop ${peer}
            fi
        done
    fi
}

start() {
    debug "Starting ${SERVICE}$DEV service..."

    lock_service_state_change

    wait_for_database_service
    check_warm_boot
    validate_restore_count

    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."

    # Don't flush DB during warm boot
    if [[ x"$WARM_BOOT" != x"true" ]]; then
        debug "Flushing APP, ASIC, COUNTER, CONFIG, and partial STATE databases ..."
        $SONIC_DB_CLI APPL_DB FLUSHDB
        $SONIC_DB_CLI ASIC_DB FLUSHDB
        $SONIC_DB_CLI COUNTERS_DB FLUSHDB
        $SONIC_DB_CLI FLEX_COUNTER_DB FLUSHDB
        $SONIC_DB_CLI GB_ASIC_DB FLUSHDB
        $SONIC_DB_CLI GB_COUNTERS_DB FLUSHDB
        $SONIC_DB_CLI RESTAPI_DB FLUSHDB
        clean_up_tables STATE_DB "'PORT_TABLE*', 'MGMT_PORT_TABLE*', 'VLAN_TABLE*', 'VLAN_MEMBER_TABLE*', 'LAG_TABLE*', 'LAG_MEMBER_TABLE*', 'INTERFACE_TABLE*', 'MIRROR_SESSION*', 'VRF_TABLE*', 'FDB_TABLE*', 'FG_ROUTE_TABLE*', 'BUFFER_POOL*', 'BUFFER_PROFILE*', 'MUX_CABLE_TABLE*', 'ADVERTISE_NETWORK_TABLE*', 'VXLAN_TUNNEL_TABLE*', 'VNET_ROUTE*', 'MACSEC_PORT_TABLE*', 'MACSEC_INGRESS_SA_TABLE*', 'MACSEC_EGRESS_SA_TABLE*', 'MACSEC_INGRESS_SC_TABLE*', 'MACSEC_EGRESS_SC_TABLE*', 'VRF_OBJECT_TABLE*', 'VNET_MONITOR_TABLE*', 'BFD_SESSION_TABLE*'"
        $SONIC_DB_CLI APPL_STATE_DB FLUSHDB
        clean_up_chassis_db_tables
        rm -rf /tmp/cache
    fi

    # On supervisor card, skip starting asic related services here. In wait(),
    # wait until the asic is detected by pmon and published via database.
    if ! is_chassis_supervisor; then
        # start service docker
        /usr/bin/${SERVICE}.sh start $DEV
        debug "Started ${SERVICE}$DEV service..."
    fi

    # Unlock has to happen before reaching out to peer service
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

    start_peer_and_dependent_services

    # Allow some time for peer container to start
    # NOTE: This assumes Docker containers share the same names as their
    # corresponding services
    for SECS in {1..60}; do
        ALL_PEERS_RUNNING=true
        for peer in ${PEER}; do
            if [[ ! -z $DEV ]]; then
                RUNNING=$(docker inspect -f '{{.State.Running}}' ${peer}$DEV)
            else
                RUNNING=$(docker inspect -f '{{.State.Running}}' ${peer})
            fi
            if [[ x"$RUNNING" != x"true" ]]; then
                ALL_PEERS_RUNNING=false
                break
            fi
        done
        ALL_DEPS_RUNNING=true
        for dep in ${MULTI_INST_DEPENDENT}; do
            if [[ ! -z $DEV ]]; then
                DEP_RUNNING=$(docker inspect -f '{{.State.Running}}' ${dep}$DEV)
            else
                DEP_RUNNING=$(docker inspect -f '{{.State.Running}}' ${dep})
            fi
            if [[ x"$DEP_RUNNING" != x"true" ]]; then
                ALL_DEPS_RUNNING=false
                break
            fi
        done

        if [[ x"$ALL_PEERS_RUNNING" == x"true" && x"$ALL_DEPS_RUNNING" == x"true" ]]; then
            break
        else
            sleep 1
        fi
    done

    # NOTE: This assumes Docker containers share the same names as their
    # corresponding services
    for dep in ${MULTI_INST_DEPENDENT}; do
        if [[ ! -z $DEV ]]; then
            ALL_DEPS="$ALL_DEPS ${dep}$DEV"
        else
            ALL_DEPS="$ALL_DEPS ${dep}"
        fi
    done

    if [[ ! -z $DEV ]]; then
        /usr/bin/docker-wait-any -s ${SERVICE}$DEV -d `printf "%s$DEV " ${PEER}` ${ALL_DEPS}
    else
        /usr/bin/docker-wait-any -s ${SERVICE} -d ${PEER} ${ALL_DEPS}
    fi
}

stop() {
    debug "Stopping ${SERVICE}$DEV service..."

    [[ -f ${LOCKFILE} ]] || /usr/bin/touch ${LOCKFILE}

    lock_service_state_change
    check_warm_boot
    debug "Warm boot flag: ${SERVICE}$DEV ${WARM_BOOT}."
    check_fast_boot
    debug "Fast boot flag: ${SERVICE}$DEV ${FAST_BOOT}."

    # For WARM/FAST boot do not perform service stop
    if [[ x"$WARM_BOOT" != x"true" ]] && [[ x"$FAST_BOOT" != x"true" ]]; then
        /usr/bin/${SERVICE}.sh stop $DEV
        debug "Stopped ${SERVICE}$DEV service..."
        $SONIC_DB_CLI APPL_DB DEL PORT_TABLE:PortInitDone
        debug "Cleared PortInitDone from APPL_DB..."
    else
        debug "Killing Docker swss..."
        /usr/bin/docker kill swss &> /dev/null || debug "Docker swss is not running ($?) ..."
    fi

    # Flush FAST_REBOOT table when swss needs to stop. The only
    # time when this would take effect is when fast-reboot
    # encountered error, e.g. syncd crashed. And swss needs to
    # be restarted.
    if [[ x"$FAST_BOOT" != x"true" ]]; then
        debug "Clearing FAST_RESTART_ENABLE_TABLE flag..."
        sonic-db-cli STATE_DB hset "FAST_RESTART_ENABLE_TABLE|system" "enable" "false"
    fi
    # Unlock has to happen before reaching out to peer service
    unlock_service_state_change

    stop_peer_and_dependent_services
}

function check_peer_gbsyncd()
{
    GEARBOX_CONFIG=/usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV/gearbox_config.json

    if [ -f $GEARBOX_CONFIG ]; then
        PEER="$PEER gbsyncd"
    fi
}

function check_macsec()
{
    MACSEC_STATE=`$SONIC_DB_CLI CONFIG_DB hget 'FEATURE|macsec' state`

    if [[ ${MACSEC_STATE} == 'enabled' ]]; then
        if [ "$DEV" ]; then
            DEPENDENT="${DEPENDENT} macsec@${DEV}"
        else
            DEPENDENT="${DEPENDENT} macsec"
        fi
    fi
}

function check_add_bgp_dependency()
{
    if ! is_chassis_supervisor; then
        if [ "$DEV" ]; then
            DEPENDENT="${DEPENDENT} bgp@${DEV}"
        else
            DEPENDENT="${DEPENDENT} bgp"
        fi
    fi
}
function check_ports_present()
{
    PORT_CONFIG_INI=/usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV/port_config.ini
    HWSKU_JSON=/usr/share/sonic/device/$PLATFORM/$HWSKU/$DEV/hwsku.json

    if [[ -f $PORT_CONFIG_INI ]] || [[ -f $HWSKU_JSON ]]; then
         return 0
    fi
    return 1
}

function check_service_exists()
{
    systemctl list-units --full -all 2>/dev/null | grep -Fq $1
    if [[ $? -eq 0 ]]; then
        echo true
        return
    else
        echo false
        return
    fi
}

# DEPENDENT initially contains namespace independent services
# namespace specific services are added later in this script.
DEPENDENT=""
MULTI_INST_DEPENDENT=""

if [[ $(check_service_exists radv) == "true" ]]; then
    DEPENDENT="$DEPENDENT radv"
fi

if [ "$DEV" ]; then
    NET_NS="$NAMESPACE_PREFIX$DEV" #name of the network namespace
    SONIC_DB_CLI="sonic-db-cli -n $NET_NS"
else
    NET_NS=""
    SONIC_DB_CLI="sonic-db-cli"
fi

PLATFORM=`$SONIC_DB_CLI CONFIG_DB hget 'DEVICE_METADATA|localhost' platform`
HWSKU=`$SONIC_DB_CLI CONFIG_DB hget 'DEVICE_METADATA|localhost' hwsku`

check_peer_gbsyncd
check_macsec
check_add_bgp_dependency
check_ports_present
PORTS_PRESENT=$?

if [[ $PORTS_PRESENT == 0 ]] && [[ $(check_service_exists teamd) == "true" ]]; then
    MULTI_INST_DEPENDENT="teamd"
fi

read_dependent_services

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
