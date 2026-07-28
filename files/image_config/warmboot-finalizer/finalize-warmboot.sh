#! /bin/bash

VERBOSE=${VERBOSE:-no}

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment
PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}

[ -f /usr/share/sonic/device/$PLATFORM/asic.conf ] && . /usr/share/sonic/device/$PLATFORM/asic.conf
NUM_ASIC=${NUM_ASIC:-1}

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
    local message="$1"
    if [[ -n $DEV ]]; then
        message="asic$DEV: $message"
    fi
    if [[ x"${VERBOSE}" == x"yes" ]]; then
        echo `date` "- $message"
    fi
    /usr/bin/logger "WARMBOOT_FINALIZER : $message"
}

# Determines if a given service has per-ASIC scope.
function is_asic_service()
{
    local -r service=$1
    local -r has_per_asic_scope=$(sonic-db-cli CONFIG_DB hget "FEATURE|$service" has_per_asic_scope)
    [[ x"${has_per_asic_scope,,}" == x"true" ]] && echo 1 || echo 0
}

# Determines if a given service has global scope.
function is_global_service()
{
    local -r service=$1
    local -r has_global_scope=$(sonic-db-cli CONFIG_DB hget "FEATURE|$service" has_global_scope)
    [[ x"${has_global_scope,,}" == x"true" ]] && echo 1 || echo 0
}

# Outputs a list of services that have per-ASIC scope.
function get_asic_service_list()
{
    local asic_service_list=""
    for service in ${!RECONCILE_COMPONENTS[@]}; do
        is_asic_service=$(is_asic_service $service)
        if (( is_asic_service )); then
            asic_service_list="${asic_service_list} ${service}"
        fi
    done
    echo ${asic_service_list}
}

# Outputs a list of services that have global scope.
function get_global_service_list()
{
    local global_service_list=""
    for service in ${!RECONCILE_COMPONENTS[@]}; do
        is_global_service=$(is_global_service $service)
        if (( is_global_service )); then
            global_service_list="${global_service_list} ${service}"
        fi
    done
    echo ${global_service_list}
}

ASIC_SERVICE_LIST="$(get_asic_service_list)"
GLOBAL_SERVICE_LIST="$(get_global_service_list)"

function get_component_list()
{
    SVC_LIST="$1"
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
    WARM_BOOT=`sonic-db-cli -n "$NETNS" STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
}

function check_fast_reboot()
{
    debug "Checking if fast-reboot is enabled..."
    FAST_REBOOT=`sonic-db-cli -n "$NETNS" STATE_DB hget "FAST_RESTART_ENABLE_TABLE|system" enable`
    if [[ x"${FAST_REBOOT}" == x"true" ]]; then
       debug "Fast-reboot is enabled..."
    else
       debug "Fast-reboot is disabled..."
    fi
}


function wait_for_database_service()
{
    debug "Wait for database to become ready..."

    # Wait for redis server start before database clean
    until [[ $(sonic-db-cli -n "$NETNS" PING | grep -c PONG) -gt 0 ]]; do
      sleep 1;
    done

    # Wait for configDB initialization
    until [[ $(sonic-db-cli -n "$NETNS" CONFIG_DB GET "CONFIG_DB_INITIALIZED") -eq 1 ]];
        do sleep 1;
    done

    debug "Database is ready..."
}


function get_component_state()
{
    sonic-db-cli -n "$NETNS" STATE_DB hget "WARM_RESTART_TABLE|$1" state
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

function set_cpufreq_governor() {
    local -r governor="$1"
    echo "$governor" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 1> /dev/null \
        && debug "Set CPUFreq scaling governor to $governor" \
        || debug "Failed to set CPUFreq scaling governor to $governor"
}

function finalize_warm_boot()
{
    debug "Finalizing warmboot..."
    sudo config warm_restart disable -n "$NETNS"
}

function finalize_fast_reboot()
{
    debug "Finalizing fast-reboot..."
    sonic-db-cli -n "$NETNS" STATE_DB hset "FAST_RESTART_ENABLE_TABLE|system" "enable" "false" &>/dev/null
    sonic-db-cli -n "$NETNS" CONFIG_DB DEL "WARM_RESTART|teamd" &>/dev/null
}

# Finalize warm/fast boot in an ASIC namespace.
# This function is called for each ASIC namespace on a Multi-ASIC device.
# For Single-ASIC devices, this function is called as well in global namespace context.
function finalize_boot()
{
    if [ x"${FAST_REBOOT}" == x"true" ]; then
        finalize_fast_reboot
    fi

    if [ x"${WARM_BOOT}" == x"true" ]; then
        finalize_warm_boot
    fi
}

# Finalize warm/fast boot in global namespace.
function finalize_global() {
    local -r asic_type=${ASIC_TYPE:-`sonic-cfggen -y /etc/sonic/sonic_version.yml -v asic_type`}

    finalize_boot

    if [[ "$asic_type" == "mellanox" ]]; then
        # Read default governor from kernel config
        local -r default_governor=$(cat "/boot/config-$(uname -r)" | grep -E 'CONFIG_CPU_FREQ_DEFAULT_GOV_.*=y' | sed -E 's/CONFIG_CPU_FREQ_DEFAULT_GOV_(.*)=y/\1/')
        set_cpufreq_governor "$default_governor"
    fi
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

function check_warm_boot_and_fast_boot_or_exit()
{
    check_fast_reboot
    check_warm_boot
    if [[ x"${WARM_BOOT}" != x"true" ]]; then
        debug "warmboot is not enabled ..."
        if [[ x"${FAST_REBOOT}" != x"true" ]]; then
            debug "fastboot is not enabled ..."
            exit 0
        fi
    fi
}

# Wait until a given list of components reach the reconciled state or timeout after 5 minutes.
function wait_for_components_to_reconcile() {
    local -r service_list="$1"
    get_component_list "${service_list}"

    debug "Waiting for components: '${COMPONENT_LIST}' to reconcile ..."

    list=${COMPONENT_LIST}

    # Wait up to 5 minutes
    for i in `seq 60`; do
        list=`check_list ${list}`
        debug "Waiting for components to reconcile: ${list}"
        if [[ -z "${list}" ]]; then
            break
        fi
        sleep 5
    done

    if [[ -n "${list}" ]]; then
        debug "Some components didn't finish reconcile: ${list} ..."
    fi
}

wait_for_database_service
check_warm_boot_and_fast_boot_or_exit

if [[ (x"${WARM_BOOT}" == x"true") && (x"${FAST_REBOOT}" != x"true") ]]; then
    restore_counters_folder
fi

# Wait for asic services to reconcile.
# Spawns a separate process for each ASIC to run finalization in parallel.
for dev in `seq 0 $((NUM_ASIC - 1))`; do
    (
        if [[ $NUM_ASIC -gt 1 ]]; then
            export DEV=$dev
            export NETNS=asic$dev
        fi

        # For multi-ASIC devices, wait for the namespace database to be ready
        # and check if warm/fast boot is enabled.
        if [[ -n $NETNS ]]; then
            wait_for_database_service
            check_warm_boot_and_fast_boot_or_exit
        fi

        wait_for_components_to_reconcile "${ASIC_SERVICE_LIST}"

        # For multi-ASIC devices, finalize the warm/fast boot flags in the namespace database.
        if [[ -n $NETNS ]]; then
            finalize_boot
        fi
    ) &
done

# Wait for global services to reconcile.
# Spawns a separate process to run finalization in parallel.
(
    wait_for_components_to_reconcile "${GLOBAL_SERVICE_LIST}"
) &

# Block till all processes complete.
wait

finalize_global

if [[ (x"${WARM_BOOT}" == x"true") && (x"${FAST_REBOOT}" != x"true") ]]; then
    stop_control_plane_assistant
fi

# Save DB after stopped control plane assistant to avoid extra entries
debug "Save in-memory database after warm/fast reboot ..."
config save -y
