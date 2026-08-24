#!/bin/bash

wait_syncd() {
    # wait until bcm sdk is ready to get a request
    while true; do
        /usr/bin/bcmcmd -t 1 "show unit" | grep BCM >/dev/null 2>&1
        rv=$?
        if [ $rv -eq 0 ]; then
            break
        fi
        sleep 1
    done
}

start() {
    echo "Starting cpo_daemon_process..."
    
    platforms=( \
	        "x86_64-micas_m2-w6940-128x1-fr4-r0" \
            )

    result=$(cat /host/machine.conf | grep onie_platform | cut -d = -f 2)
    echo "platform: $result"

    cpo_device=0
    for i in ${platforms[*]}; do
        if [ $result == $i ];
        then
            cpo_device=1
            break
        fi
    done

    if [ $cpo_device -eq 1 ];
    then
        wait_syncd
        cpo_daemon_process
    else
        echo "$result not support cpo_daemon_process"
        exit 0
    fi
}

wait() {
    echo "wait cpo_daemon_process...  do nothing"
}

stop() {
    echo "Stopping cpo_daemon_process..."
    kill -9 `ps -ef | grep python3 | grep /usr/local/bin/cpo_daemon_process | awk '{print $2}'`
    echo "Stopped cpo_daemon_process..."
    exit 0
}

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
