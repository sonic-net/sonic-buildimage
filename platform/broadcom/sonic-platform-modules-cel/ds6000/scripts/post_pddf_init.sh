#!/usr/bin/env bash

# Function: wait until syncd has created the socket for bcmcmd to connect to
wait_syncd() {
    while true; do
        service syncd status | grep 'active (running)' >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            break
        fi
        sleep 1
    done

}

wait_syncd
# wait until fpga driver is ready
exit 0