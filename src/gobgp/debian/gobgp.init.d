#!/bin/bash
#
### BEGIN INIT INFO
# Provides: gobgp
# Required-Start: $local_fs $network $remote_fs $syslog
# Required-Stop: $local_fs $network $remote_fs $syslog
# Default-Start:  2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: start and stop the gobgpd
# Description: gobgpd is an implementation of bgp daemon in Go
### END INIT INFO
#

PATH=/bin:/usr/bin:/sbin:/usr/sbin
D_PATH=/usr/sbin
C_PATH=/etc/gobgp

. /lib/lsb/init-functions

#########################################################
#               Main program                            #
#########################################################

case "$1" in
    start)
        if [ -f /etc/gobgp/gobgpd.conf ]
        then
          /usr/sbin/gobgpd -f /etc/gobgp/gobgpd.conf -r
          echo $! > /var/run/gobgpd.pid
        else
          echo /etc/gobgp/gobgpd.conf not found
        fi
        ;;

    stop)
        kill -9 $(echo /var/run/gobgpd.pid)
        ;;

    restart|force-reload)
        $0 stop $2
        sleep 1
        $0 start $2
        ;;
    *)
        echo "Usage: /etc/init.d/gobgp {start|stop|restart|force-reload}"
        exit 1
        ;;
esac

exit 0
