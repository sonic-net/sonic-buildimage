#/bin/bash

DIR=$(dirname $0)

# include files
source ${DIR}/funcs.sh

WATCHDOG_PRINT=0

while [ 1 ]
do
    if [ `dpkg -l |grep -c "watchdog "` -eq "0" ]; then
        dpkg -i /opt/debs/watchdog_5.14-3_amd64.deb
        if [ "$?" -ne "0" ]; then
            if [ $WATCHDOG_PRINT -eq 0 ]; then
                WATCHDOG_PRINT=1
                log_msg "Wait for watchdog package install."
            fi
            sleep 1
            continue
        else
            log_msg "Install watchdog package success."
        fi
    fi
    break
done

ln -sf /opt/watchdog/watchdog.conf /etc/watchdog.conf

/usr/sbin/watchdog -c /etc/watchdog.conf

exit 0;
