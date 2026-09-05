#!/usr/bin/env bash

addr="127.0.0.1"
port=2601

function help()
{
    echo "This script aims to ensure zebra is ready to accept connections"
    echo "Usage: $0 [options]"
    echo "Options:"
    echo " -a   Zebra address"
    echo " -o   Zebra port"
    exit 1
}

while getopts ":a:o:h" opt; do
    case "${opt}" in
        h) help
            ;;
        a) addr=${OPTARG}
            ;;
        o) port=${OPTARG}
            ;;
    esac
done

start=$(date +%s.%N)
timeout 5s bash -c -- "until </dev/tcp/${addr}/${port}; do sleep 0.1;done"
if [ "$?" != "0" ]; then
    logger -p error "Error: zebra is not ready to accept connections"
else
    timespan=$(awk "BEGIN {print $(date +%s.%N)-$start; exit}")
    logger -p info "It took ${timespan} seconds to wait for zebra to be ready to accept connections"
fi

exit 0
