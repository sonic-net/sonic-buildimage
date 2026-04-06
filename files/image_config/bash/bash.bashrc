# System-wide .bashrc file for interactive bash(1) shells.

# To enable the settings / commands in this file for login shells as well,
# this file has to be sourced in /etc/profile.

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# set variable identifying the chroot you work in (used in the prompt below)
if [ -z "${debian_chroot:-}" ] && [ -r /etc/debian_chroot ]; then
    debian_chroot=$(cat /etc/debian_chroot)
fi

# set a fancy prompt (non-color, overwrite the one in /etc/profile)
PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '

# Commented out, don't overwrite xterm -T "title" -n "icontitle" by default.
# If this is an xterm set the title to user@host:dir
#case "$TERM" in
#xterm*|rxvt*)
#    PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME}: ${PWD}\007"'
#    ;;
#*)
#    ;;
#esac

# enable bash completion in interactive shells
if ! shopt -oq posix; then
    if [ -f /usr/share/bash-completion/bash_completion ]; then
      . /usr/share/bash-completion/bash_completion
    elif [ -f /etc/bash_completion ]; then
      . /etc/bash_completion
    fi
fi

# if the command-not-found package is installed, use it
if [ -x /usr/lib/command-not-found -o -x /usr/share/command-not-found/command-not-found ]; then
    function command_not_found_handle {
        # check because c-n-f could've been removed in the meantime
        if [ -x /usr/lib/command-not-found ]; then
            /usr/lib/command-not-found -- "$1"
            return $?
        elif [ -x /usr/share/command-not-found/command-not-found ]; then
           /usr/share/command-not-found/command-not-found -- "$1"
           return $?
        else
           printf "%s: command not found\n" "$1" >&2
           return 127
        fi
    }
fi

# Automatically log out console ttyS* sessions after 15 minutes of inactivity
tty | egrep -q '^/dev/ttyS[[:digit:]]+$' && TMOUT=900

console_state_set() {
    local line_num="$1"
    local state="$2"
    local pid="${3:-}"
    local start_time="${4:-}"
    local key="CONSOLE_PORT|${line_num}"

    sonic-db-cli STATE_DB hset "$key" state "$state" > /dev/null 2>&1 || true
    sonic-db-cli STATE_DB hset "$key" pid "$pid" > /dev/null 2>&1 || true
    sonic-db-cli STATE_DB hset "$key" start_time "$start_time" > /dev/null 2>&1 || true
}

console_find_active_session() {
    local console_device="$1"

    ps -eo pid=,lstart=,args= | awk -v dev="$console_device" '
        $0 ~ /(mini|pico)com/ && index($0, dev) {
            print $1 "|" $2 " " $3 " " $4 " " $5 " " $6
            exit
        }
    '
}

# if SSH_TARGET_CONSOLE_LINE was set, attach to console line interactive cli directly
if [ -n "$SSH_TARGET_CONSOLE_LINE" ]; then
    if [ $SSH_TARGET_CONSOLE_LINE -eq $SSH_TARGET_CONSOLE_LINE 2>/dev/null ]; then
        # enter the interactive cli
        SONIC_PLATFORM=$(sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" platform 2>/dev/null)
        UDEV_PREFIX_CONF="/usr/share/sonic/device/${SONIC_PLATFORM}/udevprefix.conf"
        DEVICE_PREFIX=""
        if [ -r "$UDEV_PREFIX_CONF" ]; then
            read -r DEVICE_PREFIX < "$UDEV_PREFIX_CONF"
        fi
        if [ "$DEVICE_PREFIX" = "ttyCO" ]; then
            CONSOLE_SWITCH_ENABLED=$(sonic-db-cli CONFIG_DB hget "CONSOLE_SWITCH|console_mgmt" enabled 2>/dev/null)
            if [ -z "$CONSOLE_SWITCH_ENABLED" ] || [ "$CONSOLE_SWITCH_ENABLED" = "no" ]; then
                echo "Console switch feature is disabled"
                exit 1
            fi
            CONSOLE_BAUD=$(sonic-db-cli CONFIG_DB hget "CONSOLE_PORT|${SSH_TARGET_CONSOLE_LINE}" baud_rate)
            CONSOLE_FLOW=$(sonic-db-cli CONFIG_DB hget "CONSOLE_PORT|${SSH_TARGET_CONSOLE_LINE}" flow_control)
            CONSOLE_DEVICE=/dev/"${DEVICE_PREFIX}${SSH_TARGET_CONSOLE_LINE}"
            if [ -z "$CONSOLE_BAUD" ]; then
                echo "Cannot connect: line [${SSH_TARGET_CONSOLE_LINE}] has no baud rate"
                exit 4
            fi
            if [ "$CONSOLE_FLOW" = "1" ]; then
                FLOW_FLAG="h"
            else
                FLOW_FLAG="n"
            fi

            ACTIVE_SESSION=$(console_find_active_session "$CONSOLE_DEVICE")
            if [ -n "$ACTIVE_SESSION" ]; then
                ACTIVE_PID=${ACTIVE_SESSION%%|*}
                ACTIVE_START=${ACTIVE_SESSION#*|}
                console_state_set "$SSH_TARGET_CONSOLE_LINE" busy "$ACTIVE_PID" "$ACTIVE_START"
                echo "Cannot connect: line [${SSH_TARGET_CONSOLE_LINE}] is busy"
                exit 5
            fi

            console_state_set "$SSH_TARGET_CONSOLE_LINE" busy "$$" "$(ps -p $$ -o lstart= | sed 's/^ *//')"
            trap 'console_state_set "$SSH_TARGET_CONSOLE_LINE" idle "" ""' EXIT HUP INT TERM
            printf 'Successful connection to line [%s]\nPress ^A ^X to disconnect\n' "$SSH_TARGET_CONSOLE_LINE"
            picocom --quiet -b "$CONSOLE_BAUD" -f "$FLOW_FLAG" "$CONSOLE_DEVICE"
        else
            connect line $SSH_TARGET_CONSOLE_LINE
        fi
        # test exit code, 1 means the console switch feature not enabled
        if [ $? -ne 1 ]; then
            # exit after console session ended
            exit
        fi
    else
        # exit directly when target console line variable is invalid
        exit
    fi
fi

# Helper function to generate all redis-cli aliases at once
generate_sonic_redis_aliases() {
    # Define DB names and alias suffixes (Single Source of Truth)
    local -A SONIC_DBS=(
        ["APPL_DB"]="appdb"
        ["ASIC_DB"]="asicdb"
        ["COUNTERS_DB"]="counterdb"
        ["LOGLEVEL_DB"]="logleveldb"
        ["CONFIG_DB"]="configdb"
        ["FLEX_COUNTER_DB"]="flexcounterdb"
        ["STATE_DB"]="statedb"
        ["APPL_STATE_DB"]="appstatedb"
    )

    # Extract DB names (keys) from the associative array to pass to Python
    local db_keys=("${!SONIC_DBS[@]}")

    # Run the external Python script, passing DB names as arguments
    local python_output
    python_output=$(/usr/local/bin/sonic-db-aliases.py "${db_keys[@]}" 2>&1)
    local python_exit_code=$?

    # Check if Python command failed catastrophically (e.g., module not found)
    if [ $python_exit_code -ne 0 ]; then
        echo "Error generating Redis aliases: $python_output" >&2
        return 1
    fi

    # Check if redis-cli exists
    if ! command -v redis-cli &> /dev/null; then
        echo "Error: redis-cli command not found" >&2
        return 1
    fi

    # Parse output and create aliases
    while IFS=: read -r db_name db_id db_port; do
        # Skip lines that contain errors printed by the python script
        if [[ "$db_name" == "ERROR" ]]; then
            echo "$db_name:$db_id:$db_port" >&2
            continue
        fi

        if [ -n "$db_name" ] && [ -n "$db_id" ] && [ -n "$db_port" ]; then
            local alias_name="redis-${SONIC_DBS[$db_name]}"
            if [ -n "$alias_name" ]; then
                eval "alias $alias_name='redis-cli -n $db_id -p $db_port'"
            fi
        fi
    done <<< "$python_output"
}

# Generate all aliases at shell startup
generate_sonic_redis_aliases