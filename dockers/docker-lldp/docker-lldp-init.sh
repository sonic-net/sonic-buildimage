#!/usr/bin/env bash
#Generate supervisord.conf based on device metadata
mkdir -p /etc/supervisor/conf.d/
sonic-cfggen -d -a "{\"namespace_id\":\"$NAMESPACE_ID\"}" -t /usr/share/sonic/templates/supervisord.conf.j2 > /etc/supervisor/conf.d/supervisord.conf
exec /usr/local/bin/supervisord
