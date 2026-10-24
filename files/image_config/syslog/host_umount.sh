#!/bin/bash
# This script is invoked at the closure of syslog socket during reboot
# This will stop journal services, unmount /var/log and delete loop device 
# associated to /host to ensure proper unmount of /host

journal_stop() {
    systemctl stop systemd-journald.service
}

# When /var/log is on tmpfs, the persistent copy lives on the
# disk-backed /var/log.disk mount maintained by ram2disk. Flush the latest
# tmpfs contents to disk after journald stops and before the mounts are torn down.
ram2disk_preshutdown() {
    if mountpoint -q /var/log.disk; then
        /usr/local/bin/ram2disk.sh pre-shutdown
    fi
}

delete_loop_device() {
    umount /var/log
    if [[ $? -ne 0 ]]
    then
        exit 0
    fi
    # When /var/log is on tmpfs, the loop device backs /var/log.disk
    # instead of /var/log, so unmount it too before detaching the loop device.
    if mountpoint -q /var/log.disk; then
        umount /var/log.disk
    fi
    loop_exist=$(losetup -a | grep loop1 | wc -l)
    if [ $loop_exist -ne 0 ]; then
        losetup -d /dev/loop1
    fi
}

case "$1" in
    journal_stop|delete_loop_device|ram2disk_preshutdown)
        $1
        ;;
    *)
        echo "Usage: $0 {journal_stop|delete_loop_device|ram2disk_preshutdown}" >&2
        exit 1
        ;;
esac

