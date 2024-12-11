#!/usr/bin/env bash

# Generate supervisord config file
mkdir -p /etc/supervisor/conf.d/

# Generate the following files from templates:
# 1. supervisord configuration 
# 2. wait_for_intf.sh, which waits for all interfaces to come up
# 3. port-to-alias name map
CFGGEN_PARAMS=" \
    -d \
    -t /usr/share/sonic/templates/docker-dhcp-relay.supervisord.conf.j2,/etc/supervisor/conf.d/docker-dhcp-relay.supervisord.conf \
    -t /usr/share/sonic/templates/wait_for_intf.sh.j2,/usr/bin/wait_for_intf.sh \
    -t /usr/share/sonic/templates/port-name-alias-map.txt.j2,/tmp/port-name-alias-map.txt \
"
sonic-cfggen $CFGGEN_PARAMS

# Make the script that waits for all interfaces to come up executable
chmod +x /usr/bin/wait_for_intf.sh

# The docker container should start this script as PID 1, so now that supervisord is
# properly configured, we exec /usr/local/bin/supervisord so that it runs as PID 1 for the
# duration of the container's lifetime
exec /usr/local/bin/supervisord
