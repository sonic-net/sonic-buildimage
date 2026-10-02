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

# Safely introspect a Click command for the positional arguments it still
# expects, WITHOUT running the command.  Click's shell-completion only suggests
# subcommands and choice-typed arguments; it returns nothing for free-form
# positional arguments (e.g. an interface name or IP address), so '?' would
# otherwise show no useful help for commands like "config interface ip add".
#
# Resolution uses Click's resilient-parsing context resolver (the same path the
# completion protocol uses), which parses the command tree but never invokes the
# group/command callbacks -- important because callbacks such as the "config"
# group load the DB config, require root, etc.  Output mirrors the Click 8
# zsh_complete protocol (type / value / help triples, "_" for no help) so the
# caller can format it with the same loop used for real completions.
_cli_help_introspect() {
    python3 -I - "$@" <<'PYEOF'
import sys


def emit(pairs):
    out = []
    for value, help_text in pairs:
        value = " ".join(str(value).split())
        if not value:
            continue
        help_text = " ".join(str(help_text).split()) or "_"
        # Click 8 zsh_complete protocol: type / value / help per item.
        out.append("plain")
        out.append(value)
        out.append(help_text)
    if out:
        sys.stdout.write("\n".join(out) + "\n")


def run():
    if len(sys.argv) < 2:
        return
    name = sys.argv[1]
    args = sys.argv[2:]
    # Map the supported command names to their Click entry-point objects.
    entries = {
        "show": ("show.main", "cli"),
        "config": ("config.main", "config"),
        "sonic-clear": ("clear.main", "cli"),
    }
    spec = entries.get(name)
    if spec is None:
        return
    try:
        import importlib
        import click
        from click.shell_completion import _resolve_context
        module = importlib.import_module(spec[0])
        cli = getattr(module, spec[1])
        # resilient_parsing context: walks the tree, never runs callbacks.
        ctx = _resolve_context(cli, {}, name, args)
    except Exception:
        return
    if ctx is None:
        return
    command = ctx.command
    # Collect, in one pass, the positional arguments not yet provided and the
    # options the command accepts.  Options stay relevant even after every
    # positional is supplied (e.g. "show interfaces status Ethernet0 ?" should
    # still list --display, --namespace, ...).  Track whether any *required*
    # parameter (positional argument or option) is still missing -- that decides
    # if the command can run.
    arg_pairs, opt_pairs = [], []
    missing_required = False
    for p in command.get_params(ctx):
        if isinstance(p, click.Argument):
            if ctx.params.get(p.name) not in (None, (), [], ""):
                continue
            missing_required = missing_required or p.required
            # Only the first unsatisfied positional is a valid next token;
            # later positionals can't be entered yet, so don't list them.
            if arg_pairs:
                continue
            try:
                metavar = p.make_metavar()
            except Exception:
                metavar = (p.name or "").upper()
            arg_pairs.append((metavar, ""))
        elif isinstance(p, click.Option):
            if p.required and ctx.params.get(p.name) in (None, (), [], ""):
                missing_required = True
            names = list(p.opts) + list(p.secondary_opts)
            if names:
                opt_pairs.append((", ".join(names), p.help or ""))
    emit(arg_pairs + opt_pairs)
    # The command is runnable as typed when no required parameter (argument or
    # option) is missing and it isn't a group still awaiting a subcommand.
    # Signal that to the caller via the exit code so it can append the
    # <CR>/redirect/pipe hints.
    is_group = (
        isinstance(command, click.MultiCommand)
        and not command.invoke_without_command
    )
    return not missing_required and not is_group


sys.exit(0 if run() else 1)
PYEOF
}

# IOS-style '?' inline help for SONiC CLI commands.
# Pressing ? while typing a supported command (show, config, sonic-clear)
# displays available completions immediately (like Cisco IOS) with descriptions,
# without inserting the character.  For all other commands, ? inserts literally
# as usual.
# Uses Click's zsh_complete protocol to get help strings alongside values.
_cli_question_mark() {
    # local - scopes shell-option changes to this function (restored on return).
    # -f (noglob): completion/introspection output may contain [, ?, * which must
    # not glob against cwd files.  +e: those commands intentionally exit nonzero
    # (e.g. missing required arg) and must not abort the shell under errexit.
    local -
    set -f +e

    # Grab the current command line text and cursor position from readline.
    # READLINE_LINE = full text the user has typed so far
    # READLINE_POINT = cursor position (character offset from start)
    local line="$READLINE_LINE" point="$READLINE_POINT"

    # Text typed up to the cursor, with leading whitespace removed.
    local before="${line:0:point}"
    local segment="${before#"${before%%[![:space:]]*}"}"

    # First word of the segment = the command the user is typing.
    local cmd="${segment%% *}"

    # Only trigger '?' help when the cursor is still in the FIRST command
    # segment and that segment is one of the supported Click commands.  Once the
    # line contains a command separator (| ; & < >) the cursor is in a later
    # segment, so insert a literal '?' — e.g. "show ... | grep ?" types '?' for
    # grep.
    if [[ "$before" == *[\|\;\&\<\>]* || " show config sonic-clear " != *" $cmd "* ]]; then
        READLINE_LINE="${line:0:point}?${line:point}"
        ((READLINE_POINT++))
        return
    fi

    # --- From here on, the cursor is inside a supported command segment ---

    # Split the segment into words, e.g. "show bgp " -> words=("show" "bgp").
    local -a words
    read -ra words <<< "$segment"
    # cword = index of the word being completed (Click needs this).
    #   - If there's a trailing space, user is starting a NEW word
    #   - If no trailing space, user is still typing the LAST word
    local cword=${#words[@]}
    [[ "$segment" != *" " ]] && ((cword--))

    # Ask Click for completions using the zsh_complete protocol, which
    # returns 3 lines per completion item:
    #   line 1: type  ("plain", "dir", or "file")
    #   line 2: value (the completion text, e.g. "neighbors")
    #   line 3: help  (description string, or "_" if none)
    # We pass COMP_WORDS (space-separated) and COMP_CWORD (index) as
    # environment variables — this is how Click knows what to complete.
    # Click derives the activation variable from the command name by uppercasing
    # and replacing hyphens with underscores, e.g. "show" -> _SHOW_COMPLETE,
    # "config" -> _CONFIG_COMPLETE, "sonic-clear" -> _SONIC_CLEAR_COMPLETE.
    # Build it dynamically and export it (local -x) so the subshell command sees
    # it.  Capture comp_words before changing IFS so words join with spaces.
    local comp_words="${words[*]}"
    local cmd_var="${cmd^^}"
    local -x "_${cmd_var//-/_}_COMPLETE=zsh_complete"
    local IFS=$'\n'
    local -a lines
    lines=($(COMP_WORDS="$comp_words" COMP_CWORD=$cword \
             "$cmd" 2>/dev/null))

    # Click returned no completions.  This commonly means the command expects a
    # free-form positional argument (e.g. "config interface ip add <name>"),
    # which Click can't suggest.  Safely introspect the command for the
    # arguments it still expects so '?' shows them instead of nothing.
    local runnable=0
    if (( ${#lines[@]} == 0 )); then
        lines=($(_cli_help_introspect "$cmd" "${words[@]:1:cword-1}" 2>/dev/null))
        # _cli_help_introspect exits 0 when the command can be executed as
        # typed, so we know whether to offer the <CR>/redirect/pipe hints below.
        (( $? == 0 )) && runnable=1
    fi

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
    printf '%s%s?\n' "$prompt" "$before"

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
    fi

    # IOS-style trailing hints: show them when the command can be executed as
    # typed (runnable), or when there was nothing else to display at all.
    if (( runnable || ${#names[@]} == 0 )); then
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
