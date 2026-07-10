#!/usr/bin/env bash

mkdir -p /etc/supervisor/conf.d

# Generate supervisord router advertiser config, /etc/radvd.conf config file, and
# the script that waits for pertinent interfaces to come up and make it executable
CFGGEN_PARAMS=" \
    -d \
    -t /usr/share/sonic/templates/docker-router-advertiser.supervisord.conf.j2,/etc/supervisor/conf.d/supervisord.conf \
    -t /usr/share/sonic/templates/radvd.conf.j2,/etc/radvd.conf \
    -t /usr/share/sonic/templates/wait_for_link.sh.j2,/usr/bin/wait_for_link.sh \
"
sonic-cfggen $CFGGEN_PARAMS

chmod +x /usr/bin/wait_for_link.sh

exec /usr/local/bin/supervisord
