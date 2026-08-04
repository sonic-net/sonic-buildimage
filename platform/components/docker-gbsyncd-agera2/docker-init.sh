#! /bin/sh

GB_CONFIG=/usr/share/sonic/hwsku/gearbox_config.json

if [ ! -f $GB_CONFIG ]; then
   exit 0
fi

exec /usr/local/bin/supervisord
