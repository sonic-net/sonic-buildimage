#!/bin/bash

sonic-cfggen -d --redis-unix-socket -t  /usr/share/sonic/templates/rsyslog.j2 -a "{\"var_log_kb\":$(df -k /var/log | sed -n 2p | awk '{ print $2 }') }" > /etc/logrotate.d/rsyslog

