#!/bin/bash

RETRY=0
while [ $RETRY -lt 5 ]; do
    let RETRY=$RETRY+1

    /bin/systemctl --quiet is-active serial-getty@ttyS1.service
    status=$?
    if [ $status == 0 ]; then
        exit 0
    fi

    # when serial-getty not running, recheck later, beause systemd will restart serial-getty automatically.
    sleep 1
done

exit 1