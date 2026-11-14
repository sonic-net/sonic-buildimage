#!/bin/bash
set -euo pipefail

# The guard applies only to plugin-enabled Bash builds.
if ! grep -q '^#define BASH_SHELL_EXECVE_PLUGIN 1$' ../config.h; then
    exit 0
fi

for command in \
    'enable -f /nonexistent.so unsafe' \
    'builtin enable -f /nonexistent.so unsafe'; do
    if output=$(../bash --noprofile --norc -c "$command" 2>&1); then
        echo "unexpectedly loaded a dynamic builtin: $command" >&2
        exit 1
    fi
    case "$output" in
        *'dynamic builtin loading disabled while exec authorization is enabled'*) ;;
        *) echo "unexpected rejection: $output" >&2; exit 1 ;;
    esac
done

../bash --noprofile --norc -c 'enable echo'
