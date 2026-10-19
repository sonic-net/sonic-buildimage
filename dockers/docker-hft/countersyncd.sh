#!/usr/bin/env bash
#
# Start countersyncd in the hft container.
#
# countersyncd is started without any option unless one is configured. Options
# come from the following sources, from the lowest to the highest precedence:
#
#   1. The settings file named by HFT_CONFIG_FILE (/etc/sonic/hft/hft.env by
#      default). /etc/sonic is mounted from the host, so this is how the hft
#      container managed by systemd on the switch is configured.
#   2. The environment of the container: "docker run -e/--env-file ...", or the
#      "env"/"envFrom" of a Kubernetes container spec.
#   3. HFT_COUNTERSYNCD_ARGS: raw countersyncd arguments separated by
#      whitespace, set in the environment or in the settings file.
#   4. The arguments given to the container itself (Docker CMD / Kubernetes
#      "args"), which docker_init.sh saves in HFT_CONTAINER_ARGS_FILE.
#
# Sources 1 and 2 are COUNTERSYNCD_<NAME> variables; one set in the environment
# wins over the settings file. Each variable is turned into the countersyncd
# option --<name>, where <name> is NAME in lower case with '_' replaced by '-'.
# A flag is set by "true" and left out by "false" or an empty value. An option
# taking a value is given the value of the variable, or left out if it is empty:
#
#   COUNTERSYNCD_ENABLE_OTEL=true                   -> --enable-otel
#   COUNTERSYNCD_OTEL_ENDPOINT=http://10.0.0.1:4317 -> --otel-endpoint=http://10.0.0.1:4317
#   COUNTERSYNCD_ENABLE_COUNTER_DB=false            -> (no --enable-counter-db)
#
# The options of countersyncd, their short names and whether they take a value
# are read from "countersyncd --help", so options added to countersyncd later
# can be configured without changing this script. Variables which do not name
# an option (typos, Kubernetes service links, ...) are reported and ignored.
#
# countersyncd rejects an option given twice. When an option is given by more
# than one of the COUNTERSYNCD_<NAME> variables, HFT_COUNTERSYNCD_ARGS and the
# container arguments, it is only taken from the one with the highest
# precedence: e.g. "-l debug" in the container arguments overrides
# COUNTERSYNCD_LOG_LEVEL=info. Options are matched by their long and short
# names and passed to countersyncd in their long form, --<name> or
# --<name>=<value>. Other arguments are passed as they are. Within one source,
# the options are passed as given, so an option which can be repeated keeps all
# its occurrences. Note that a flag cannot be cleared by an argument, as
# countersyncd has no option for that: to clear a flag set in the settings file,
# set its COUNTERSYNCD_<NAME> variable to false in the environment.
#
# The following variables control the container itself and are only read from
# the environment:
#   HFT_CONFIG_FILE          settings file (default: /etc/sonic/hft/hft.env)
#   HFT_COUNTERSYNCD_BIN     countersyncd binary (default: /usr/bin/countersyncd)
#   HFT_CONTAINER_ARGS_FILE  file where docker_init.sh saves the container arguments
#
# Everything is evaluated each time countersyncd starts, so a change of the
# settings file takes effect after "supervisorctl restart countersyncd" in the
# container or a restart of the hft service.

DEFAULT_HFT_CONFIG_FILE=/etc/sonic/hft/hft.env

# Sources of countersyncd arguments, by increasing precedence
SOURCE_VARIABLES=1
SOURCE_HFT_COUNTERSYNCD_ARGS=2
SOURCE_CONTAINER_ARGS=3

log()
{
    echo "countersyncd.sh: $*"
}

# Read settings from an environment file. The file is parsed rather than
# sourced, so values are taken literally. Variables which are already set are
# left alone: the environment of the container wins over the file.
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
        if [[ ! "${line}" =~ ^[[:space:]]*(export[[:space:]]+)?([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*=[[:space:]]*(.*)$ ]]; then
            log "Ignoring line ${lineno} of ${file}: expected <NAME>=<value>"
            continue
        fi
        key=${BASH_REMATCH[2]}
        value=${BASH_REMATCH[3]}
        if [[ "${key}" != COUNTERSYNCD_?* && "${key}" != HFT_COUNTERSYNCD_ARGS ]]; then
            log "Ignoring line ${lineno} of ${file}: only COUNTERSYNCD_<NAME> and HFT_COUNTERSYNCD_ARGS can be set in this file"
            continue
        fi
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

# Options of countersyncd, read from its --help output. The keys are prefixed
# with dashes, so that they are never empty:
#   option_kind[--<name>]  "flag", "value" (one value), "optional" (an optional
#                          value) or "multiple" (one or more values)
#   short_option[-<c>]     long name of the short option -<c>
declare -A option_kind=()
declare -A short_option=()
load_countersyncd_options()
{
    # clap starts the line of an option with 2 spaces and its short name, or
    # with 6 spaces if it has none. Descriptions are indented further. The
    # option name is followed by " <VALUE>" if it takes a value, " <VALUE>..."
    # if it takes several, and " [<VALUE>]" if its value is optional.
    local option_re='^(  -([[:alnum:]]), |      )--([[:alnum:]][[:alnum:]-]*)(.*)$'
    local multiple_re='^ <[^>]+>[.][.][.]'
    local value_re='^ <'
    local optional_re='^ \['
    local line short name rest kind

    while IFS= read -r line; do
        [[ "${line}" =~ ${option_re} ]] || continue
        short=${BASH_REMATCH[2]}
        name=${BASH_REMATCH[3]}
        rest=${BASH_REMATCH[4]}
        if [[ "${rest}" =~ ${multiple_re} ]]; then
            kind=multiple
        elif [[ "${rest}" =~ ${value_re} ]]; then
            kind=value
        elif [[ "${rest}" =~ ${optional_re} ]]; then
            kind=optional
        else
            kind=flag
        fi
        option_kind[--${name}]=${kind}
        if [[ -n "${short}" ]]; then
            short_option[-${short}]=${name}
        fi
    done < <("${HFT_COUNTERSYNCD_BIN}" --help 2>/dev/null)
}

# The command line being assembled: each argument, the source it comes from,
# the option it belongs to ("" for other arguments), and its origin for logs.
args=()
arg_source=()
arg_option=()
arg_origin=()

add_arg()   # <source> <origin> <option> <argument>
{
    arg_source+=("$1")
    arg_origin+=("$2")
    arg_option+=("$3")
    args+=("$4")
}

source_description()   # <source>
{
    case "$1" in
        "${SOURCE_HFT_COUNTERSYNCD_ARGS}") echo "HFT_COUNTERSYNCD_ARGS" ;;
        "${SOURCE_CONTAINER_ARGS}") echo "the container arguments" ;;
        *) echo "the COUNTERSYNCD_<NAME> variables" ;;
    esac
}

# Add the options given by the COUNTERSYNCD_<NAME> variables
add_variable_args()
{
    local variable option value

    for variable in $(compgen -A export COUNTERSYNCD_ | LC_ALL=C sort); do
        option=${variable#COUNTERSYNCD_}
        option=${option,,}
        option=${option//_/-}
        value=${!variable}
        if [[ "${option}" == help || "${option}" == version ]]; then
            log "Ignoring ${variable}: --${option} would stop countersyncd"
            continue
        fi
        case "${option_kind[--${option}]}" in
            "")
                if [[ -z "${option}" ]]; then
                    log "Ignoring ${variable}: no option name"
                else
                    log "Ignoring ${variable}: countersyncd has no --${option} option"
                fi
                ;;
            flag)
                case "${value,,}" in
                    true)
                        add_arg "${SOURCE_VARIABLES}" "${variable}" "${option}" "--${option}"
                        ;;
                    false|"")
                        ;;
                    *)
                        log "Ignoring ${variable}: --${option} is a flag, expected true or false"
                        ;;
                esac
                ;;
            *)
                if [[ -n "${value}" ]]; then
                    add_arg "${SOURCE_VARIABLES}" "${variable}" "${option}" "--${option}=${value}"
                fi
                ;;
        esac
    done
}

# Add raw countersyncd arguments. Options of countersyncd are rewritten in
# their long form, so that they can be matched across sources.
add_raw_args()   # <source> <argument>...
{
    local source=$1
    shift
    local -a tokens=("$@")
    local count=${#tokens[@]}
    local origin token name kind cluster short rest
    local i=0 j options_ended=0

    origin=$(source_description "${source}")
    while (( i < count )); do
        token=${tokens[i]}
        i=$((i + 1))
        if (( options_ended )) || [[ "${token}" != -?* ]]; then
            add_arg "${source}" "${origin}" "" "${token}"
            continue
        fi
        if [[ "${token}" == "--" ]]; then
            # Everything after "--" is not an option
            options_ended=1
            add_arg "${source}" "${origin}" "" "${token}"
            continue
        fi

        if [[ "${token}" == --* ]]; then
            # Long option: --name, --name=value or --name value
            name=${token#--}
            name=${name%%=*}
            kind=${option_kind[--${name}]}
            case "${kind}" in
                flag)
                    if [[ "${token}" == *=* ]]; then
                        # Invalid, left for countersyncd to report
                        add_arg "${source}" "${origin}" "" "${token}"
                    else
                        add_arg "${source}" "${origin}" "${name}" "--${name}"
                    fi
                    ;;
                value)
                    if [[ "${token}" == *=* ]]; then
                        add_arg "${source}" "${origin}" "${name}" "--${name}=${token#*=}"
                    elif (( i < count )) && [[ "${tokens[i]}" != -?* ]]; then
                        add_arg "${source}" "${origin}" "${name}" "--${name}=${tokens[i]}"
                        i=$((i + 1))
                    else
                        # Missing value, left for countersyncd to report
                        add_arg "${source}" "${origin}" "${name}" "${token}"
                    fi
                    ;;
                optional|multiple)
                    # Kept as given, together with the values which follow it
                    add_arg "${source}" "${origin}" "${name}" "${token}"
                    if [[ "${token}" != *=* ]]; then
                        while (( i < count )) && [[ "${tokens[i]}" != -?* ]]; do
                            add_arg "${source}" "${origin}" "${name}" "${tokens[i]}"
                            i=$((i + 1))
                            [[ "${kind}" == optional ]] && break
                        done
                    fi
                    ;;
                *)
                    add_arg "${source}" "${origin}" "" "${token}"
                    ;;
            esac
            continue
        fi

        # Short options, possibly grouped and with a value:
        # -o, -l debug, -ldebug, -l=debug, -ol debug
        cluster=${token#-}
        for (( j = 0; j < ${#cluster}; j++ )); do
            short=${cluster:j:1}
            rest=${cluster:j+1}
            name=${short_option[-${short}]}
            kind=${option_kind[--${name}]}
            case "${kind}" in
                flag)
                    add_arg "${source}" "${origin}" "${name}" "--${name}"
                    continue
                    ;;
                value)
                    if [[ -n "${rest}" ]]; then
                        add_arg "${source}" "${origin}" "${name}" "--${name}=${rest#=}"
                    elif (( i < count )) && [[ "${tokens[i]}" != -?* ]]; then
                        add_arg "${source}" "${origin}" "${name}" "--${name}=${tokens[i]}"
                        i=$((i + 1))
                    else
                        # Missing value, left for countersyncd to report
                        add_arg "${source}" "${origin}" "${name}" "--${name}"
                    fi
                    ;;
                optional|multiple)
                    # Kept as given, together with the values which follow it
                    add_arg "${source}" "${origin}" "${name}" "-${short}${rest}"
                    if [[ -z "${rest}" ]]; then
                        while (( i < count )) && [[ "${tokens[i]}" != -?* ]]; do
                            add_arg "${source}" "${origin}" "${name}" "${tokens[i]}"
                            i=$((i + 1))
                            [[ "${kind}" == optional ]] && break
                        done
                    fi
                    ;;
                *)
                    # Not an option of countersyncd, left for it to report
                    add_arg "${source}" "${origin}" "" "-${short}${rest}"
                    ;;
            esac
            break
        done
    done
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

load_countersyncd_options

add_variable_args
if [[ -n "${HFT_COUNTERSYNCD_ARGS}" ]]; then
    read -r -d '' -a extra_args <<< "${HFT_COUNTERSYNCD_ARGS}"
    add_raw_args "${SOURCE_HFT_COUNTERSYNCD_ARGS}" "${extra_args[@]}"
fi
if [[ -s "${HFT_CONTAINER_ARGS_FILE}" ]]; then
    mapfile -d '' -t container_args < "${HFT_CONTAINER_ARGS_FILE}"
    add_raw_args "${SOURCE_CONTAINER_ARGS}" "${container_args[@]}"
fi

# Take every option from the source with the highest precedence which gives it
declare -A option_source=()
for i in "${!args[@]}"; do
    name=${arg_option[i]}
    if [[ -n "${name}" ]] && (( arg_source[i] > ${option_source[--${name}]:-0} )); then
        option_source[--${name}]=${arg_source[i]}
    fi
done

countersyncd_args=()
declare -A reported=()
for i in "${!args[@]}"; do
    name=${arg_option[i]}
    if [[ -n "${name}" ]] && (( arg_source[i] < ${option_source[--${name}]} )); then
        key="${arg_origin[i]} --${name}"
        if [[ -z "${reported[${key}]}" ]]; then
            reported[${key}]=1
            if (( arg_source[i] == SOURCE_VARIABLES )); then
                ignored=${arg_origin[i]}
            else
                ignored="--${name} in ${arg_origin[i]}"
            fi
            log "Ignoring ${ignored}: overridden by --${name} in $(source_description "${option_source[--${name}]}")"
        fi
        continue
    fi
    countersyncd_args+=("${args[i]}")
done

printable=$(printable_args "${countersyncd_args[@]}")
log "Starting ${HFT_COUNTERSYNCD_BIN}${printable:+ ${printable}}"
exec "${HFT_COUNTERSYNCD_BIN}" "${countersyncd_args[@]}"
