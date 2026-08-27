#!/usr/bin/env bash

start=$(date +%s.%N)
timeout 5s bash -c -- 'until vtysh -d zebra -c "show version" >/dev/null 2>&1; do sleep 0.1; done'
if [ "$?" != "0" ]; then
    logger -p error "Error: zebra is not ready on its vtysh socket"
else
    timespan=$(awk "BEGIN {print $(date +%s.%N)-$start; exit}")
    logger -p info "It took ${timespan} seconds to wait for zebra's vtysh socket"
fi

exit 0
