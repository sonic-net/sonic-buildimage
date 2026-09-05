#!/usr/bin/env bash

function clean_up {
    service syncd stop
    exit
}

trap clean_up SIGTERM SIGKILL

service syncd start

read
