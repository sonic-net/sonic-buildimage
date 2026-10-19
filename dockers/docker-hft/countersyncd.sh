#!/usr/bin/env bash
#
# Start countersyncd in the hft container.
#
# countersyncd is started without any option unless one is configured. Its
# command line is assembled from the following sources, from the lowest to the
# highest precedence:
#
#   1. The settings file named by HFT_CONFIG_FILE (/etc/sonic/hft/hft.env by
#      default). /etc/sonic is mounted from the host, so this is how the hft
#      container managed by systemd on the switch is configured.
#   2. The environment of the container: "docker run -e/--env-file ...", or the
#      "env"/"envFrom" of a Kubernetes container spec.
#   3. Raw countersyncd arguments: HFT_COUNTERSYNCD_ARGS, followed by the
#      arguments given to the container itself (Docker CMD / Kubernetes
#      "args"), which docker_init.sh saves in HFT_CONTAINER_ARGS_FILE.
#
# Sources 1 and 2 are environment variables. Every COUNTERSYNCD_<NAME> variable
# is turned into the countersyncd option --<name>, where <name> is NAME in lower
# case with '_' replaced by '-'. The value "true" sets a flag, "false" or an
# empty value leaves the option out, and any other value is used as the value
# of the option:
#
#   COUNTERSYNCD_ENABLE_OTEL=true                   -> --enable-otel
#   COUNTERSYNCD_OTEL_ENDPOINT=http://10.0.0.1:4317 -> --otel-endpoint=http://10.0.0.1:4317
#   COUNTERSYNCD_ENABLE_COUNTER_DB=false            -> (no --enable-counter-db)
#
# Only the options listed by "countersyncd --help" are accepted, so options
# added to countersyncd later are configurable without changing this script,
# while unrelated variables (typos, Kubernetes service links, ...) are reported
# and ignored. An option that is also given as a raw argument (source 3) is
# taken from the raw arguments only.
#
# Variables controlling the container itself use the HFT_ prefix:
#   HFT_CONFIG_FILE          settings file (default: /etc/sonic/hft/hft.env)
#   HFT_COUNTERSYNCD_ARGS    extra raw countersyncd arguments, split on whitespace
#   HFT_COUNTERSYNCD_BIN     countersyncd binary (default: /usr/bin/countersyncd)
#   HFT_CONTAINER_ARGS_FILE  file where docker_init.sh saves the container arguments
#   HFT_RMEM_MAX             minimum net.core.rmem_max in bytes (default: 134217728,
#                            i.e. 128MB; 0 or empty leaves the kernel setting alone)
#
# net.core.rmem_max caps the socket receive buffer countersyncd can get with
# --netlink-rcvbuf. It is a global setting that can only be changed from the
# host network namespace, so it is raised to HFT_RMEM_MAX before countersyncd
# starts, and never lowered.
#
# Everything is evaluated each time countersyncd starts, so a change of the
# settings file takes effect after "supervisorctl restart countersyncd" in the
# container or a restart of the hft service.

DEFAULT_HFT_CONFIG_FILE=/etc/sonic/hft/hft.env
DEFAULT_HFT_RMEM_MAX=134217728

# Root of the sysctl tree, overridden by the unit tests
HFT_SYSCTL_DIR=${HFT_SYSCTL_DIR:-/proc/sys}

log()
{
    echo "countersyncd.sh: $*"
}

# Read KEY=VALUE settings from an environment file. The file is parsed rather
# than sourced, so values are taken literally. Variables which are already set
# are left alone: the environment of the container wins over the file.
load_settings_file()
{
    local file=$1
    local line key value
    local lineno=0

    while IFS= read -r line || [[ -n "${line}" ]]; do
        lineno=$((lineno + 1))
        if [[ "${line}" =~ ^[[:space:]]*(#.*)?$ ]]; then
            continue
        fi
        if [[ ! "${line}" =~ ^[[:space:]]*(export[[:space:]]+)?((COUNTERSYNCD|HFT)_[A-Za-z0-9_]+)[[:space:]]*=[[:space:]]*(.*)$ ]]; then
            log "Ignoring line ${lineno} of ${file}: expected COUNTERSYNCD_<NAME>=<value> or HFT_<NAME>=<value>"
            continue
        fi
        key=${BASH_REMATCH[2]}
        value=${BASH_REMATCH[4]}
        # Strip trailing whitespace and one pair of surrounding quotes
        value=${value%"${value##*[![:space:]]}"}
        if [[ "${value}" =~ ^\"(.*)\"$ || "${value}" =~ ^\'(.*)\'$ ]]; then
            value=${BASH_REMATCH[1]}
        fi
        if [[ -v ${key} ]]; then
            continue
        fi
        export "${key}=${value}"
    done < "${file}"
}

# Long options supported by countersyncd, taken from its --help output, except
# --help and --version which would make countersyncd exit right away.
list_countersyncd_options()
{
    "${HFT_COUNTERSYNCD_BIN}" --help 2>/dev/null | \
        sed -nE 's/^[[:space:]]*(-[[:alnum:]],[[:space:]]+)?--([[:alnum:]][[:alnum:]-]*).*/\2/p' | \
        grep -vxE 'help|version'
}

# Format arguments for the log, hiding credentials: the values of options whose
# name suggests a secret, and the user information of URLs.
printable_args()
{
    local arg name
    local hide_next=0
    local -a out=()

    for arg in "$@"; do
        if (( hide_next )); then
            hide_next=0
            if [[ "${arg}" != -* ]]; then
                out+=("<hidden>")
                continue
            fi
        fi
        name=${arg%%=*}
        if [[ "${arg}" == --* && "${name,,}" =~ (password|passwd|secret|token|credential|auth|header|key) ]]; then
            if [[ "${arg}" == *=* ]]; then
                out+=("${name}=<hidden>")
                continue
            fi
            hide_next=1
        fi
        if [[ "${arg}" =~ ^(.*://)[^/@]*@(.*)$ ]]; then
            arg="${BASH_REMATCH[1]}<hidden>@${BASH_REMATCH[2]}"
        fi
        out+=("${arg}")
    done
    echo "${out[*]}"
}

# Raise net.core.rmem_max to at least the given number of bytes. It is never
# lowered, as other applications on the host may rely on a higher limit.
raise_rmem_max()
{
    local target=$1
    local file="${HFT_SYSCTL_DIR}/net/core/rmem_max"
    local current

    if [[ -z "${target}" || "${target}" == "0" ]]; then
        return 0
    fi
    if [[ ! "${target}" =~ ^[1-9][0-9]*$ ]]; then
        log "Ignoring HFT_RMEM_MAX=${target}: expected a number of bytes"
        return 0
    fi
    if ! current=$(cat "${file}" 2>/dev/null) || [[ ! "${current}" =~ ^[0-9]+$ ]]; then
        log "Cannot read ${file}, leaving net.core.rmem_max unchanged"
        return 0
    fi
    if (( 10#${current} >= target )); then
        return 0
    fi
    if { echo "${target}" > "${file}"; } 2>/dev/null; then
        log "Raised net.core.rmem_max from ${current} to ${target}"
    else
        log "Cannot raise net.core.rmem_max from ${current} to ${target}" \
            "(this needs the host network namespace and a writable /proc/sys);" \
            "set it on the host if countersyncd needs a larger receive buffer"
    fi
}

# The settings file must not override the environment of the container
settings_file=${HFT_CONFIG_FILE:-${DEFAULT_HFT_CONFIG_FILE}}
if [[ -r "${settings_file}" ]]; then
    log "Loading settings from ${settings_file}"
    load_settings_file "${settings_file}"
elif [[ -n "${HFT_CONFIG_FILE}" ]]; then
    log "Settings file ${HFT_CONFIG_FILE} is not readable, ignoring it"
fi

HFT_COUNTERSYNCD_BIN=${HFT_COUNTERSYNCD_BIN:-/usr/bin/countersyncd}
HFT_CONTAINER_ARGS_FILE=${HFT_CONTAINER_ARGS_FILE:-/var/run/hft/container_args}

if [[ ! -x "${HFT_COUNTERSYNCD_BIN}" ]]; then
    log "${HFT_COUNTERSYNCD_BIN} is not executable"
    exit 1
fi

raise_rmem_max "${HFT_RMEM_MAX-${DEFAULT_HFT_RMEM_MAX}}"

# Raw countersyncd arguments
raw_args=()
if [[ -n "${HFT_COUNTERSYNCD_ARGS}" ]]; then
    read -r -d '' -a raw_args <<< "${HFT_COUNTERSYNCD_ARGS}"
fi
if [[ -s "${HFT_CONTAINER_ARGS_FILE}" ]]; then
    mapfile -d '' -t container_args < "${HFT_CONTAINER_ARGS_FILE}"
    raw_args+=("${container_args[@]}")
fi

declare -A raw_options=()
for arg in "${raw_args[@]}"; do
    if [[ "${arg}" == --?* ]]; then
        raw_options["${arg%%=*}"]=1
    fi
done

# COUNTERSYNCD_* variables, from the environment or the settings file
declare -A known_options=()
while IFS= read -r option; do
    known_options["${option}"]=1
done < <(list_countersyncd_options)

env_args=()
for name in $(compgen -A export COUNTERSYNCD_ | LC_ALL=C sort); do
    option=${name#COUNTERSYNCD_}
    option=${option,,}
    option=${option//_/-}
    if [[ -z "${option}" || -z "${known_options[${option}]}" ]]; then
        log "Ignoring ${name}: countersyncd has no --${option} option"
        continue
    fi
    if [[ -n "${raw_options[--${option}]}" ]]; then
        log "Ignoring ${name}: --${option} is given as a countersyncd argument"
        continue
    fi
    value=${!name}
    case "${value,,}" in
        true)
            env_args+=("--${option}")
            ;;
        false|"")
            ;;
        *)
            env_args+=("--${option}=${value}")
            ;;
    esac
done

countersyncd_args=("${env_args[@]}" "${raw_args[@]}")

printable=$(printable_args "${countersyncd_args[@]}")
log "Starting ${HFT_COUNTERSYNCD_BIN}${printable:+ ${printable}}"
exec "${HFT_COUNTERSYNCD_BIN}" "${countersyncd_args[@]}"
