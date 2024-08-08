#!/usr/bin/env bash

mkdir -p /etc/supervisor/conf.d/

SUPERVISOR_CONF_TEMPLATE="/usr/share/sonic/templates/docker-snmp.supervisord.conf.j2"
SUPERVISOR_CONF_FILE="/etc/supervisor/conf.d/supervisord.conf"
SNMP_DAEMON_CONTROL_FILE="/usr/share/sonic/platform/snmp_daemon_control.json"

if [ -e $SNMP_DAEMON_CONTROL_FILE ];
then
    sonic-cfggen -j $SNMP_DAEMON_CONTROL_FILE -t $SUPERVISOR_CONF_TEMPLATE > $SUPERVISOR_CONF_FILE
else
    sonic-cfggen -t $SUPERVISOR_CONF_TEMPLATE > $SUPERVISOR_CONF_FILE
fi

exec /usr/local/bin/supervisord
