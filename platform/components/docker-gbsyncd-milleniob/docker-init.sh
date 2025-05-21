#! /bin/sh

GB_CONFIG=/usr/share/sonic/hwsku/gearbox_config.json

if [ ! -f $GB_CONFIG ]; then
   exit 0
fi

CFGGEN_ARG="-j $GB_CONFIG"

mkdir -p /etc/supervisor/conf.d/

sonic-cfggen $CFGGEN_ARG -t /usr/share/sonic/templates/supervisord.conf.j2 > /etc/supervisor/conf.d/supervisord.conf
sonic-cfggen $CFGGEN_ARG -t /usr/share/sonic/templates/critical_processes.j2 > /etc/supervisor/critical_processes

exec /usr/local/bin/supervisord
