#! /bin/bash

VERBOSE=no
WATCHDOG_UTIL="/usr/local/bin/watchdogutil"
WATCHDOG_SERVICE="watchdog.service"
WATCHDOG_DEFAULT="/etc/default/watchdog"
WATCHDOG_CONFIG="/etc/watchdog.conf"
WATCHDOG_TABLE="WATCHDOG|config"
DEFAULT_WATCHDOG_DEVICE="/dev/watchdog0"
DEFAULT_WATCHDOG_TIMEOUT="600"
DEFAULT_WATCHDOG_PRETIMEOUT="300"
WATCHDOG_CONFIG_OVERRIDE=""

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

    if [[ -n "${WATCHDOG_CONFIG_OVERRIDE}" ]]; then
        jq -r --arg field "${field}" '.[$field] // empty' <<< "${WATCHDOG_CONFIG_OVERRIDE}"
        return
    fi

    sonic-db-cli CONFIG_DB HGET "${WATCHDOG_TABLE}" "${field}" 2> /dev/null || true
}

function config_error()
{
    debug "$1"
    echo "$1" >&2
    return 1
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

    if [[ "${timeout}" =~ ^[0-9]{1,10}$ ]] && (( 10#${timeout} > 0 && 10#${timeout} <= 2147483647 )); then
        echo "$((10#${timeout}))"
        return 0
    fi

    config_error "Invalid watchdog timeout ${timeout}: expected a positive integer"
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

function watchdog_supports_pretimeout()
{
    local watchdog_sysfs_path="$1"
    local options
    local governors

    [[ -e "${watchdog_sysfs_path}/pretimeout" ]] || return 1
    options="$(cat "${watchdog_sysfs_path}/options" 2> /dev/null || true)"
    # Older kernels expose pretimeout even when the driver lacks the capability.
    if [[ "${options}" =~ ^0x[0-9a-fA-F]{1,8}$ ]] && (( options & 0x0200 )); then
        return 0
    fi
    # A kernel software pretimeout governor can also provide this capability.
    governors="$(cat "${watchdog_sysfs_path}/pretimeout_available_governors" 2> /dev/null || true)"
    [[ -n "${governors//[[:space:]]/}" ]]
}

function get_watchdog_pretimeout()
{
    local watchdog_sysfs_path="$1"
    local pretimeout

    pretimeout="$(get_watchdog_config pretimeout)"
    if [[ -z "${pretimeout}" ]]; then
        if watchdog_supports_pretimeout "${watchdog_sysfs_path}"; then
            pretimeout="${DEFAULT_WATCHDOG_PRETIMEOUT}"
        else
            pretimeout=0
        fi
    fi

    if [[ "${pretimeout}" =~ ^[0-9]{1,10}$ ]] && (( 10#${pretimeout} <= 2147483647 )); then
        echo "$((10#${pretimeout}))"
        return 0
    fi

    config_error "Invalid watchdog pretimeout ${pretimeout}: expected a nonnegative integer"
}

function validate_watchdog_settings()
{
    local limit

    watchdog_device="$(select_watchdog_device)" || {
        config_error "Unable to select the configured watchdog device"
        return 1
    }
    watchdog_sysfs_path="$(get_watchdog_sysfs_path "${watchdog_device}")" || watchdog_sysfs_path=""
    watchdog_timeout="$(get_watchdog_timeout)" || return 1
    watchdog_pretimeout="$(get_watchdog_pretimeout "${watchdog_sysfs_path}")" || return 1

    if (( watchdog_pretimeout >= watchdog_timeout )); then
        config_error "Watchdog pretimeout must be less than timeout"
        return 1
    fi

    if (( watchdog_pretimeout > 0 )) && ! watchdog_supports_pretimeout "${watchdog_sysfs_path}"; then
        config_error "Watchdog ${watchdog_device} does not support pretimeout"
        return 1
    fi

    for limit in min_timeout max_timeout; do
        local value
        value="$(cat "${watchdog_sysfs_path}/${limit}" 2> /dev/null || true)"
        if [[ "${value}" =~ ^[0-9]{1,10}$ ]] && (( 10#${value} > 0 )); then
            if { [[ "${limit}" == min_timeout ]] && (( watchdog_timeout < 10#${value} )); } ||
               { [[ "${limit}" == max_timeout ]] && (( watchdog_timeout > 10#${value} )); }; then
                config_error "Watchdog timeout ${watchdog_timeout} is outside device ${limit} ${value}"
                return 1
            fi
        fi
    done
}

function configure_watchdog_timeouts()
{
    local watchdog_device="$1"
    local watchdog_timeout="$2"
    local watchdog_pretimeout="$3"
    local watchdog_sysfs_path="$4"
    local pretimeout_file="${watchdog_sysfs_path}/pretimeout"
    local timeout_file="${watchdog_sysfs_path}/timeout"

    if command -v wdctl > /dev/null; then
        if ! watchdog_supports_pretimeout "${watchdog_sysfs_path}"; then
            wdctl --settimeout "${watchdog_timeout}" "${watchdog_device}" > /dev/null 2>&1 && return 0
        # Clear the previous pretimeout before lowering the total timeout.
        elif wdctl --setpretimeout 0 "${watchdog_device}" > /dev/null 2>&1 &&
             wdctl --settimeout "${watchdog_timeout}" --setpretimeout "${watchdog_pretimeout}" "${watchdog_device}" > /dev/null 2>&1; then
            return 0
        fi
    fi

    # Some vendor drivers expose writable timeout attributes. Standard Linux
    # watchdog sysfs attributes are read-only and must be set through wdctl.
    if [[ -w "${timeout_file}" ]]; then
        if ! watchdog_supports_pretimeout "${watchdog_sysfs_path}"; then
            echo "${watchdog_timeout}" > "${timeout_file}" && return 0
        elif [[ -w "${pretimeout_file}" ]]; then
            if echo 0 > "${pretimeout_file}" &&
               echo "${watchdog_timeout}" > "${timeout_file}" &&
               echo "${watchdog_pretimeout}" > "${pretimeout_file}"; then
                return 0
            fi
        fi
    fi

    config_error "Failed to configure watchdog timeout ${watchdog_timeout}, pretimeout ${watchdog_pretimeout} for ${watchdog_device}"
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
    local watchdog_sysfs_path
    local watchdog_timeout
    local watchdog_pretimeout

    if ! systemctl list-unit-files "${WATCHDOG_SERVICE}" > /dev/null 2>&1; then
        debug "${WATCHDOG_SERVICE} is not installed"
        return 1
    fi

    # Validate the complete pair and device capability before stopping keepalives.
    validate_watchdog_settings || return 1
    if [[ ! -f "${WATCHDOG_CONFIG}" ]]; then
        config_error "${WATCHDOG_CONFIG} does not exist"
        return 1
    fi
    systemctl stop "${WATCHDOG_SERVICE}" || return 1
    configure_watchdog_timeouts "${watchdog_device}" "${watchdog_timeout}" "${watchdog_pretimeout}" "${watchdog_sysfs_path}" || return 1
    configure_watchdog "${watchdog_device}" "${watchdog_timeout}" || return 1
    configure_watchdog_defaults

    debug "Starting Linux watchdog service using ${watchdog_device}, timeout ${watchdog_timeout}, pretimeout ${watchdog_pretimeout}"
    systemctl unmask "${WATCHDOG_SERVICE}" || true
    systemctl enable "${WATCHDOG_SERVICE}" || true
    systemctl reset-failed "${WATCHDOG_SERVICE}" || true
    systemctl restart --no-block "${WATCHDOG_SERVICE}"
}

function main()
{
    if (( $# > 0 )); then
        if [[ $# -ne 2 || "$1" != --validate ]] ||
           ! jq -e 'type == "object"' > /dev/null 2>&1 <<< "$2"; then
            config_error "Usage: $0 [--validate <watchdog-config-json>]"
            return 1
        fi
        WATCHDOG_CONFIG_OVERRIDE="$2"
        validate_watchdog_settings
        return
    fi

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
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
