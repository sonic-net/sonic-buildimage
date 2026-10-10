#! /bin/bash

VERBOSE=no
WATCHDOG_UTIL="/usr/local/bin/watchdogutil"
WATCHDOG_SERVICE="watchdog.service"
WATCHDOG_DEFAULT="/etc/default/watchdog"
WATCHDOG_CONFIG="/etc/watchdog.conf"
WATCHDOG_TABLE="WATCHDOG|config"
DEFAULT_WATCHDOG_DEVICE="/dev/watchdog0"
DEFAULT_WATCHDOG_TIMEOUT="30"
DEFAULT_WATCHDOG_TIMEOUT_MARGIN="300"

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment
PLATFORM=${PLATFORM:-$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)}
PLATFORM_JSON="/usr/share/sonic/device/$PLATFORM/platform.json"

function debug()
{
    /usr/bin/logger "$0 : $1"
    if [[ x"${VERBOSE}" == x"yes" ]]; then
        echo "$(date) $0: $1"
    fi
}


function getBootType()
{
    # same code snippet in files/scripts/syncd.sh
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
        TYPE='fast'
        ;;
    *)
        TYPE='cold'
    esac
    echo "${TYPE}"
}

function get_watchdog_config()
{
    local field="$1"

    sonic-db-cli CONFIG_DB HGET "${WATCHDOG_TABLE}" "${field}" 2> /dev/null || true
}

function is_smart_switch_dpu()
{
    if [[ ! -f "$PLATFORM_JSON" ]]; then
        return 1
    fi

    if jq -e 'has("DPU")' "$PLATFORM_JSON" > /dev/null; then
        return 0
    else
        return 1
    fi
}

function disable_watchdog()
{
    if systemctl list-unit-files "${WATCHDOG_SERVICE}" > /dev/null 2>&1; then
        debug "Stopping Linux watchdog service"
        systemctl stop "${WATCHDOG_SERVICE}" || true
        systemctl disable "${WATCHDOG_SERVICE}" || true
    fi

    # Obtain boot type from kernel arguments
    BOOT_TYPE=`getBootType`
    if [[ -x ${WATCHDOG_UTIL} ]]; then
        debug "Disabling Watchdog during bootup after $BOOT_TYPE"
        ${WATCHDOG_UTIL} disarm
    fi
}

function enable_watchdog()
{
    if [[ -x ${WATCHDOG_UTIL} ]]; then
        debug "Enabling Watchdog"
        ${WATCHDOG_UTIL} arm
    fi
}

function watchdog_enabled()
{
    case "$(get_watchdog_config enabled | tr '[:upper:]' '[:lower:]')" in
        true|yes|1|on)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

function select_watchdog_device()
{
    local configured_identity
    local wdt
    local identity
    local device

    configured_identity="$(get_watchdog_config identity)"
    if [[ -z "${configured_identity}" ]]; then
        if [[ -e "${DEFAULT_WATCHDOG_DEVICE}" ]]; then
            echo "${DEFAULT_WATCHDOG_DEVICE}"
            return 0
        fi

        debug "Default watchdog device ${DEFAULT_WATCHDOG_DEVICE} does not exist"
        return 1
    fi

    for wdt in /sys/class/watchdog/watchdog*; do
        [[ -e "${wdt}/identity" ]] || continue

        identity="$(cat "${wdt}/identity")"
        if [[ "${identity}" == "${configured_identity}" ]]; then
            device="/dev/$(basename "${wdt}")"
            if [[ -e "${device}" ]]; then
                echo "${device}"
                return 0
            fi

            debug "Watchdog identity ${configured_identity} matched ${wdt}, but ${device} does not exist"
            return 1
        fi
    done

    debug "No watchdog device found with identity ${configured_identity}"
    return 1
}

function get_watchdog_timeout()
{
    local timeout

    timeout="$(get_watchdog_config timeout)"
    if [[ -z "${timeout}" ]]; then
        echo "${DEFAULT_WATCHDOG_TIMEOUT}"
        return 0
    fi

    if [[ "${timeout}" =~ ^[0-9]+$ ]] && (( timeout > 0 )); then
        echo "${timeout}"
        return 0
    fi

    debug "Invalid watchdog timeout ${timeout}"
    return 1
}

function get_watchdog_sysfs_path()
{
    local watchdog_device="$1"
    local watchdog_name
    local watchdog_sysfs_path

    watchdog_name="$(basename "$(readlink -f "${watchdog_device}")")"
    watchdog_sysfs_path="/sys/class/watchdog/${watchdog_name}"

    if [[ -d "${watchdog_sysfs_path}" ]]; then
        echo "${watchdog_sysfs_path}"
        return 0
    fi

    debug "Watchdog sysfs path ${watchdog_sysfs_path} does not exist"
    return 1
}

function get_watchdog_timeout_margin()
{
    local watchdog_sysfs_path="$1"
    local timeout
    local pretimeout
    local timeout_margin

    timeout="$(cat "${watchdog_sysfs_path}/timeout" 2> /dev/null || true)"
    pretimeout="$(cat "${watchdog_sysfs_path}/pretimeout" 2> /dev/null || true)"

    if [[ "${timeout}" =~ ^[0-9]+$ && "${pretimeout}" =~ ^[0-9]+$ ]] && (( timeout > pretimeout )); then
        timeout_margin=$((timeout - pretimeout))
        if (( timeout_margin > 0 )); then
            echo "${timeout_margin}"
            return 0
        fi
    fi

    echo "${DEFAULT_WATCHDOG_TIMEOUT_MARGIN}"
}

function configure_watchdog_pretimeout()
{
    local watchdog_device="$1"
    local pretimeout="$2"
    local watchdog_sysfs_path
    local pretimeout_file
    local timeout_file
    local timeout_margin
    local total_timeout

    watchdog_sysfs_path="$(get_watchdog_sysfs_path "${watchdog_device}")" || {
        echo "${pretimeout}"
        return 0
    }

    pretimeout_file="${watchdog_sysfs_path}/pretimeout"
    timeout_file="${watchdog_sysfs_path}/timeout"
    if [[ ! -e "${pretimeout_file}" ]]; then
        echo "${pretimeout}"
        return 0
    fi

    timeout_margin="$(get_watchdog_timeout_margin "${watchdog_sysfs_path}")"
    total_timeout=$((pretimeout + timeout_margin))

    if command -v wdctl > /dev/null; then
        if wdctl --settimeout "${total_timeout}" "${watchdog_device}" > /dev/null 2>&1 &&
           wdctl --setpretimeout "${pretimeout}" "${watchdog_device}" > /dev/null 2>&1; then
            debug "Configured watchdog pretimeout ${pretimeout}, timeout margin ${timeout_margin}, timeout ${total_timeout} for ${watchdog_device}"
            echo "${total_timeout}"
            return 0
        fi

        debug "Failed to configure watchdog pretimeout using wdctl for ${watchdog_device}"
    fi

    if [[ -w "${pretimeout_file}" && -w "${timeout_file}" ]]; then
        if ! echo "${total_timeout}" > "${timeout_file}"; then
            debug "Failed to set watchdog timeout ${total_timeout} for ${watchdog_device}"
            return 1
        fi

        if ! echo "${pretimeout}" > "${pretimeout_file}"; then
            debug "Failed to set watchdog pretimeout ${pretimeout} for ${watchdog_device}"
            return 1
        fi
    else
        debug "Watchdog pretimeout is not configurable for ${watchdog_device}"
        return 1
    fi

    debug "Configured watchdog pretimeout ${pretimeout}, timeout margin ${timeout_margin}, timeout ${total_timeout} for ${watchdog_device}"
    echo "${total_timeout}"
}

function configure_watchdog_defaults()
{
    if [[ ! -f "${WATCHDOG_DEFAULT}" ]]; then
        return 0
    fi

    if grep -q '^run_wd_keepalive=' "${WATCHDOG_DEFAULT}"; then
        sed -i 's/^run_wd_keepalive=.*/run_wd_keepalive=0/' "${WATCHDOG_DEFAULT}"
    else
        echo 'run_wd_keepalive=0' >> "${WATCHDOG_DEFAULT}"
    fi

    if grep -q '^run_watchdog=' "${WATCHDOG_DEFAULT}"; then
        sed -i 's/^run_watchdog=.*/run_watchdog=1/' "${WATCHDOG_DEFAULT}"
    else
        echo 'run_watchdog=1' >> "${WATCHDOG_DEFAULT}"
    fi

    if grep -q '^watchdog_options=' "${WATCHDOG_DEFAULT}"; then
        sed -i "s|^watchdog_options=.*|watchdog_options=\"--config-file ${WATCHDOG_CONFIG}\"|" "${WATCHDOG_DEFAULT}"
    else
        echo "watchdog_options=\"--config-file ${WATCHDOG_CONFIG}\"" >> "${WATCHDOG_DEFAULT}"
    fi
}

function set_watchdog_config_value()
{
    local key="$1"
    local value="$2"

    sed -i "/^[[:space:]]*${key}[[:space:]]*=.*/d" "${WATCHDOG_CONFIG}"
    echo "${key} = ${value}" >> "${WATCHDOG_CONFIG}"
}

function configure_watchdog()
{
    local watchdog_device="$1"
    local watchdog_timeout="$2"

    if [[ ! -f "${WATCHDOG_CONFIG}" ]]; then
        debug "${WATCHDOG_CONFIG} does not exist"
        return 1
    fi

    set_watchdog_config_value "watchdog-device" "${watchdog_device}"
    set_watchdog_config_value "watchdog-timeout" "${watchdog_timeout}"
}

function enable_linux_watchdog_service()
{
    local watchdog_device
    local configured_timeout
    local watchdog_timeout

    if ! systemctl list-unit-files "${WATCHDOG_SERVICE}" > /dev/null 2>&1; then
        debug "${WATCHDOG_SERVICE} is not installed"
        return 1
    fi

    watchdog_device="$(select_watchdog_device)" || return 1
    configured_timeout="$(get_watchdog_timeout)" || return 1
    systemctl stop "${WATCHDOG_SERVICE}" || true
    watchdog_timeout="$(configure_watchdog_pretimeout "${watchdog_device}" "${configured_timeout}")" || return 1
    configure_watchdog "${watchdog_device}" "${watchdog_timeout}" || return 1
    configure_watchdog_defaults

    debug "Starting Linux watchdog service using ${watchdog_device}, timeout ${watchdog_timeout}"
    systemctl unmask "${WATCHDOG_SERVICE}" || true
    systemctl enable "${WATCHDOG_SERVICE}" || true
    systemctl reset-failed "${WATCHDOG_SERVICE}" || true
    systemctl restart --no-block "${WATCHDOG_SERVICE}"
}

if is_smart_switch_dpu; then
    # Keep watchdog enabled for smart switch DPUs
    # Smart switch DPUs uses ARM SBSA Generic Watchdog,
    # which is capable of monitoring the system in runtime.
    enable_watchdog
elif watchdog_enabled; then
    enable_linux_watchdog_service
else
    disable_watchdog
fi
