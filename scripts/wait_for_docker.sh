#!/bin/bash

total_time=$1
cnt=0
while [ $cnt -le $total_time ]; do
    docker info &>/dev/null
    rv=$?
    if [ $rv -eq 0 ]; then
        exit 0
    fi
    sleep 1
    cnt=$((cnt+1))
done

echo 'Timed out waiting for internal docker daemon to start' > /dev/stderr
echo '==== START OF /var/log/docker.log ===='
cat /var/log/docker.log
echo '==== END OF /var/log/docker.log ===='

exit 1
