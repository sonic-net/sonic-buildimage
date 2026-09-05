#!/bin/bash
# This script is invoked at the closure of syslog socket during reboot
# This will stop journal services, unmount /var/log and delete loop device 
# associated to /host to ensure proper unmount of /host

journal_stop() {
    systemctl stop systemd-journald.service
}

delete_loop_device() {
    umount /var/log
    if [[ $? -ne 0 ]]
    then
        exit 0
    fi
    loop_exist=$(losetup -a | grep loop1 | wc -l)
    if [ $loop_exist -ne 0 ]; then
        losetup -d /dev/loop1
    fi
}

case "$1" in
    journal_stop|delete_loop_device)
        $1
        ;;
    *)
        echo "Usage: $0 {journal_stop|delete_loop_device}" >&2
        exit 1
        ;;
esac

