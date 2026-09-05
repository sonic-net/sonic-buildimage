#!/bin/bash
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# SYNCD_SOCKET_FILE=/var/run/sswsyncd/sswsyncd.socket

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

wait_syncd() {
    # while true; do
    #     if [ -e ${SYNCD_SOCKET_FILE} ]; then
    #         break
    #     fi
    #     sleep 1
    # done

    # wait until bcm sdk is ready to get a request
    counter=0
    while true; do
        /usr/bin/bcmcmd -t 1 "show unit" | grep BCM >/dev/null 2>&1
        rv=$?
        if [ $rv -eq 0 ]; then
            break
        fi
        counter=$((counter+1))
        if [ $counter -ge 60 ]; then
            echo "syncd is not ready to take commands after $counter re-tries; Exiting!"
            break
        fi
        sleep 1
    done
}

start() {
    debug "Starting cpodaemon service..."
    
    platforms=( \
	        "x86_64-micas_m2-w6940-128x1-fr4-r0" \
            )

    result=$(cat /host/machine.conf | grep onie_platform | cut -d = -f 2)
    echo $result

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
        cpo_daemon
    else
        echo "$result not support cpo"
        exit 0
    fi
}

wait() {
    debug "wait cpodaemon service...  do nothing"
}

stop() {
    debug "Stopping cpodaemon service..."
    pkill -9 cpo_daemon
    debug "Stopped cpodaemon service..."
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
