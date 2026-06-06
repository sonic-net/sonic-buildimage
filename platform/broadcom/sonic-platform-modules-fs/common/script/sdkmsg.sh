#!/bin/bash

function debug()
{
    /usr/bin/logger $1
    # /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

start() {
    debug "Starting sdkmsg service..."

    platforms=( \
            "x86_64-ruijie_b6990-128qc-r0" \
            "x86_64-tencent_tcs9500-r0" \
            "x86_64-micas_m2-w6940-128x1-fr4-r0" \
            "x86_64-micas_m2-w6940-64oc-r0" \
            "x86_64-micas_m2-w6931-64qc-r0" \
            "x86_64-ruijie_b6990-128qc2xs-r0" \
            "x86_64-ruijie_b9510-40qc-r0" \
            "x86_64-ruijie_b6940-64qc-d16-r0" \
            "x86_64-ruijie_ws9851-32dq-r0" \
            "x86_64-ruijie_tc20r0t400-r0" \
            "x86_64-ruijie_tc20r1t400-r0" \
            )

    result=$(grep onie_platform /host/machine.conf | cut -d = -f 2)
    echo $result

    sdkmsg=0
    for i in ${platforms[*]}; do
        if [ $result == $i ];
        then
            sdkmsg=1
            break
        fi
    done

    if [ $sdkmsg -eq 0 ];
    then
        result2=$(grep sdkmsg /usr/share/sonic/device/$result/platform_env_fac.conf | cut -d = -f 2)
        if [ $result2 == 1 ];
        then
            sdkmsg=1
        fi
    fi

    if [ $sdkmsg -eq 1 ];
    then
        if [ -e /sonic_baresdk/baresdk_running.sh ];
        then
            /sonic_baresdk/baresdk_running.sh sdkmsg
            debug "sdkmsg service exit"
        else
            echo "sctipt baresdk_running.sh miss"
            exit 0
        fi
    else
        echo "$result not support, sdkmsg=0"
        exit 0
    fi

}

wait() {
    debug "wait sdkmsg service...  do nothing"
}

stop() {
    debug "Stopping sdkmsg service..."
    pkill -9 bcm.user
    sleep 0.1
    for ((i=0; i<30; i++)); do
        if pgrep -x "bcm.user" > /dev/null; then
            debug "Found bcm.user process, killing it"
            pkill -9 bcm.user
            sleep 1
        else
            debug "bcm.user process not found, exiting loop"
            break
        fi
    done
    # 查找 baresdk_running.sh 进程号
    PIDS=$(ps -elf | grep baresdk_running.sh | grep -v grep | awk '{print $4}')

    # 如果找到进程，终止它们
    if [ -n "$PIDS" ]; then
        debug "Terminating baresdk_running.sh and its subprocesses: $PIDS"
        for PID in $PIDS; do
            kill -9 $PID
        done
    else
        debug "No baresdk_running.sh process found"
    fi
    debug "Stopped sdkmsg service..."
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
