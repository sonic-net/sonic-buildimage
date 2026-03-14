#!/usr/bin/env bash

# Render supervisord config from Jinja2 template
sonic-cfggen -d -t /usr/share/sonic/templates/supervisord.conf.j2 > /etc/supervisor/conf.d/supervisord.conf

exec /usr/local/bin/supervisord
