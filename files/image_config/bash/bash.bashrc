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

# if SSH_TARGET_CONSOLE_LINE was set, attach to console line interactive cli directly
if [ -n "$SSH_TARGET_CONSOLE_LINE" ]; then
    if [ $SSH_TARGET_CONSOLE_LINE -eq $SSH_TARGET_CONSOLE_LINE 2>/dev/null ]; then
        # enter the interactive cli
        connect line $SSH_TARGET_CONSOLE_LINE

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
    # Define DB names and alias suffixes
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

    # Run Python once to get all DB configurations
    local python_output
    python_output=$(python3 -c "
import sys
try:
    import swsscommon.swsscommon

    SonicDBConfig = None

    if hasattr(swsscommon.swsscommon, 'SonicDBConfig'):
        SonicDBConfig = swsscommon.swsscommon.SonicDBConfig

    if SonicDBConfig is None:
        print('ERROR:SonicDBConfig not found in swsscommon module', file=sys.stderr)
        sys.exit(1)

    # Initialize config (some versions require this)
    try:
        SonicDBConfig.loadSonicDBConfig()
    except AttributeError:
        pass  # Some versions don't have this method
    except Exception:
        pass  # Ignore if config file is missing

    dbs = ['APPL_DB', 'ASIC_DB', 'COUNTERS_DB', 'LOGLEVEL_DB',
           'CONFIG_DB', 'FLEX_COUNTER_DB', 'STATE_DB', 'APPL_STATE_DB']
    for db in dbs:
        try:
            db_id = SonicDBConfig.getDbId(db)
            db_port = SonicDBConfig.getDbPort(db)
            print(f'{db}:{db_id}:{db_port}')
        except Exception as e:
            print(f'ERROR:Failed to get config for {db}: {e}', file=sys.stderr)
            sys.exit(1)

except ImportError as e:
    print(f'ERROR:swsscommon module not found: {e}', file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f'ERROR:Unexpected error: {e}', file=sys.stderr)
    sys.exit(1)
" 2>&1)

    # Check if Python command succeeded
    if [ $? -ne 0 ]; then
        echo "Error generating Redis aliases: $python_output" >&2
        return 1
    fi

    # Check for Python-level errors in output
    if [[ "$python_output" == ERROR:* ]]; then
        echo "$python_output" >&2
        return 1
    fi

    # Check if redis-cli exists
    if ! command -v redis-cli &> /dev/null; then
        echo "Error: redis-cli command not found" >&2
        return 1
    fi

    # Parse output and create aliases
    while IFS=: read -r db_name db_id db_port; do
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