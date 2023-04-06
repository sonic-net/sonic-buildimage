#! /bin/bash

VERBOSE=no

# Define components that needs to reconcile during warm
# boot:
#       The key is the name of the service that the components belong to.
#       The value is list of component names that will reconcile.
declare -A RECONCILE_COMPONENTS=( \
                        ["swss"]="orchagent neighsyncd" \
                        ["bgp"]="bgp"                   \
                        ["nat"]="natsyncd"              \
                        ["mux"]="linkmgrd"              \
                       )

for reconcile_file in $(find /etc/sonic/ -iname '*_reconcile' -type f); do
    file_basename=$(basename $reconcile_file)
    docker_container_name=${file_basename%_reconcile}
    RECONCILE_COMPONENTS[$docker_container_name]=$(cat $reconcile_file)
done

EXP_STATE="reconciled"

ASSISTANT_SCRIPT="/usr/local/bin/neighbor_advertiser"


function debug()
{
    /usr/bin/logger "WARMBOOT_FINALIZER : $1"
    if [[ x"${VERBOSE}" == x"yes" ]]; then
        echo `date` "- $1"
    fi
}


function get_component_list()
{
    SVC_LIST=${!RECONCILE_COMPONENTS[@]}
    COMPONENT_LIST=""
    for service in ${SVC_LIST}; do
        components=${RECONCILE_COMPONENTS[${service}]}
        status=$(sonic-db-cli CONFIG_DB HGET "FEATURE|${service}" state)
        if [[ x"${status}" == x"enabled" || x"${status}" == x"always_enabled" ]]; then
            COMPONENT_LIST="${COMPONENT_LIST} ${components}"
        fi
    done
}


function check_warm_boot()
{
    WARM_BOOT=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
}


function wait_for_database_service()
{
    debug "Wait for database to become ready..."

    # Wait for redis server start before database clean
    until [[ $(sonic-db-cli PING | grep -c PONG) -gt 0 ]]; do
      sleep 1;
    done

    # Wait for configDB initialization
    until [[ $(sonic-db-cli CONFIG_DB GET "CONFIG_DB_INITIALIZED") ]];
        do sleep 1;
    done

    debug "Database is ready..."
}


function get_component_state()
{
    sonic-db-cli STATE_DB hget "WARM_RESTART_TABLE|$1" state
}


function check_list()
{
    RET_LIST=''
    for comp in $@; do
        state=`get_component_state ${comp}`
        if [[ x"${state}" != x"${EXP_STATE}" ]]; then
            RET_LIST="${RET_LIST} ${comp}"
        fi
    done

    echo ${RET_LIST}
}


function finalize_warm_boot()
{
    debug "Finalizing warmboot..."
    sudo config warm_restart disable
}

function stop_control_plane_assistant()
{
    if [[ -x ${ASSISTANT_SCRIPT} ]]; then
        debug "Tearing down control plane assistant ..."
        ${ASSISTANT_SCRIPT} -m reset
    fi
}

function restore_counters_folder()
{
    debug "Restoring counters folder after warmboot..."

    cache_counters_folder="/host/counters"
    if [[ -d $cache_counters_folder ]]; then
        mv $cache_counters_folder /tmp/cache
        chown -R admin:admin /tmp/cache
    fi
}


wait_for_database_service

check_warm_boot

if [[ x"${WARM_BOOT}" != x"true" ]]; then
    debug "warmboot is not enabled ..."
    exit 0
fi

restore_counters_folder

get_component_list

debug "Waiting for components: '${COMPONENT_LIST}' to reconcile ..."

list=${COMPONENT_LIST}

# Wait up to 5 minutes
for i in `seq 60`; do
    list=`check_list ${list}`
    if [[ -z "${list}" ]]; then
        break
    fi
    sleep 5
done

stop_control_plane_assistant

# Save DB after stopped control plane assistant to avoid extra entries
debug "Save in-memory database after warm reboot ..."
config save -y

if [[ -n "${list}" ]]; then
    debug "Some components didn't finish reconcile: ${list} ..."
fi

finalize_warm_boot
