#!/bin/bash

# Exit immediately when receiving SIGINT/SIGTERM so that Ctrl-C during
# a build actually stops the process pipeline instead of leaving
# process_log.sh running (and blocking make from exiting).
trap 'exit 130' INT
trap 'exit 143' TERM

add_timestamp=""

while getopts ":t" opt; do
    case $opt in
        t)
            add_timestamp="y"
            ;;
    esac
done

while IFS= read -r line; do
    if [ $add_timestamp ]; then
        printf '[%s] ' "$(date +%T)"
    fi
    printf '%s\n' "$line"
done
