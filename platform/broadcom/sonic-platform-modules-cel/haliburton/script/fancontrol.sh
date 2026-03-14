#! /bin/sh

### BEGIN INIT INFO
# Provides:          fancontrol
# Required-Start:    $remote_fs
# Required-Stop:     $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:
# Short-Description: fancontrol
# Description:       fancontrol configuration selector
### END INIT INFO

. /lib/lsb/init-functions

[ -f /etc/default/rcS ] && . /etc/default/rcS
MAIN_CONF=/usr/share/sonic/device/x86_64-cel_e1031-r0/fancontrol
PLATFORM_PATH=/sys/devices/platform/e1031.smc

init() {
    FANDIR=$(cat ${PLATFORM_PATH}/fan1_dir)
    CONF=${MAIN_CONF}-${FANDIR}
    echo $FANDIR > /usr/share/sonic/device/x86_64-cel_e1031-r0/fan_airflow
}

install() {
    find /var/lib/docker/overlay*/ -path */sbin/fancontrol -exec cp /usr/local/bin/fancontrol {} \;
}

case "$1" in
start)
    init
    cp $CONF $MAIN_CONF
    ;;
install)
    install
    ;;
*)
    log_success_msg "Usage: /etc/init.d/fancontrol {start} | {install}"
    exit 1
    ;;
esac

exit 0
