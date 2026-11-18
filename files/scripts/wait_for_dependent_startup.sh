#!/bin/bash
# Quiescence only: a terminal or unqueryable helper must not block teardown.
while :; do
    state=$(supervisorctl status dependent-startup 2>/dev/null |
        awk '$1 == "dependent-startup" { print $2 }')
    case "$state" in
        STARTING|RUNNING|BACKOFF) sleep 1 ;;
        *) exit 0 ;;
    esac
done
