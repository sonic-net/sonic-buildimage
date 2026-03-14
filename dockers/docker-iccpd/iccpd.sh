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
    exit
}

trap clean_up SIGTERM SIGKILL
start_app
read
