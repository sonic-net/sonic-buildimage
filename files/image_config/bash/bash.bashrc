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

# IOS-style '?' inline help for SONiC show commands.
# Pressing ? while typing a show command displays available completions
# immediately (like Cisco IOS) with descriptions, without inserting the
# character.  For non-show commands, ? inserts literally as usual.
# Uses Click's zsh_complete protocol to get help strings alongside values.
_cli_question_mark() {
    # Grab the current command line text and cursor position from readline.
    # READLINE_LINE = full text the user has typed so far
    # READLINE_POINT = cursor position (character offset from start)
    local line="$READLINE_LINE" point="$READLINE_POINT"

    # If the command being typed is NOT "show", just insert a literal '?'
    # at the cursor position and advance the cursor — normal typing behavior.
    if [[ "${line%% *}" != "show" ]]; then
        READLINE_LINE="${line:0:point}?${line:point}"
        ((READLINE_POINT++))
        return
    fi

    # --- From here on, the user is typing a "show" command ---

    # Take the text from the start up to the cursor (ignore anything after).
    # Split it into words, e.g. "show bgp " -> words=("show" "bgp").
    local partial="${line:0:point}"
    local -a words
    read -ra words <<< "$partial"
    # cword = index of the word being completed (Click needs this).
    #   - If there's a trailing space, user is starting a NEW word
    #   - If no trailing space, user is still typing the LAST word
    local cword=${#words[@]}
    [[ "$partial" != *" " ]] && ((cword--))

    # Ask Click for completions using the zsh_complete protocol, which
    # returns 3 lines per completion item:
    #   line 1: type  ("plain", "dir", or "file")
    #   line 2: value (the completion text, e.g. "neighbors")
    #   line 3: help  (description string, or "_" if none)
    # We pass COMP_WORDS (space-separated) and COMP_CWORD (index) as
    # environment variables — this is how Click knows what to complete.
    # Capture comp_words before changing IFS so words join with spaces.
    local comp_words="${words[*]}"
    local IFS=$'\n'
    local -a lines
    lines=($(COMP_WORDS="$comp_words" COMP_CWORD=$cword \
             _SHOW_COMPLETE=zsh_complete show 2>/dev/null))

    # Parse the triplets into parallel arrays: names[] and descs[].
    # Also track the longest name for column alignment.
    local -a names=() descs=()
    local i maxlen=0
    for (( i=0; i < ${#lines[@]}; i+=3 )); do
        local ctype="${lines[i]}" cvalue="${lines[i+1]}" chelp="${lines[i+2]}"
        [[ "$ctype" != "plain" && "$ctype" != "dir" && "$ctype" != "file" ]] && continue
        [[ -z "$cvalue" ]] && continue
        names+=("$cvalue")
        [[ "$chelp" == "_" ]] && chelp=""
        descs+=("$chelp")
        (( ${#cvalue} > maxlen )) && maxlen=${#cvalue}
    done

    # Print a header that looks like the current prompt + what the user typed,
    # e.g. "admin@sonic:~$ show bgp ?"
    # PS1@P expands the prompt string, then we strip \x01/\x02 markers that
    # readline uses for color codes — they'd show as garbled characters.
    local prompt="${PS1@P}"
    prompt="${prompt//[$'\x01'$'\x02']/}"
    printf '%s%s?\n' "$prompt" "$partial"

    # Print completions in IOS style: name and description in aligned columns.
    #   neighbors  LLDP neighbor entries
    #   table      LLDP neighbor table
    if (( ${#names[@]} )); then
        for (( i=0; i < ${#names[@]}; i++ )); do
            if [[ -n "${descs[i]}" ]]; then
                printf "  %-${maxlen}s  %s\n" "${names[i]}" "${descs[i]}"
            else
                printf "  %s\n" "${names[i]}"
            fi
        done
    else
        printf '  <CR>      \n'
        printf '  >         Redirect it to a file\n'
        printf '  >>        Redirect it to a file in append mode\n'
        printf '  |         Pipe command output to filter\n'
    fi
}
# Bind the '?' key to call our function instead of inserting the character.
bind -x '"?": _cli_question_mark'

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
