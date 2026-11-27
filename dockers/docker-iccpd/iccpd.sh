#!/usr/bin/env bash

export platform=$(sonic-cfggen -d -y /etc/sonic/sonic_version.yml --var asic_type)

function start_app {
    rm -f /var/run/iccpd/*
    mclagsyncd &
    iccpd
}

function clean_up {
    pkill -9 mclagsyncd
    pkill -9 iccpd
    exit 0
}

trap clean_up SIGTERM SIGINT
start_app
rc=$?
pkill -9 mclagsyncd 2>/dev/null || true
exit $rc
