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
MAIN_CONF=/usr/share/sonic/device/x86_64-cel_seastone-r0/fancontrol
GPIO_DIR=/sys/class/gpio

init() {
    DIRGPIO_START=15
    BASE_GPIO=$(find $GPIO_DIR | grep gpiochip | grep -o '[[:digit:]]*')
    FANDIR_GPIO_NUMBER=$((DIRGPIO_START + BASE_GPIO))
    FANDIR_VALUE=$(cat ${GPIO_DIR}/gpio${FANDIR_GPIO_NUMBER}/value)
    DIRGPIO_START=$((DIRGPIO_START + 1))
    FANDIR=$([ $FANDIR_VALUE = 1 ] && echo "B2F" || echo "F2B")
    CONF=${MAIN_CONF}-${FANDIR}
    echo $FANDIR > /usr/share/sonic/device/x86_64-cel_seastone-r0/fan_airflow
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
