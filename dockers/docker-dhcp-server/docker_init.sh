#!/usr/bin/env bash


# Generate supervisord config file
mkdir -p /etc/supervisor/conf.d/
# Generate kea folder
mkdir -p /etc/kea/
udp_server_ip=$(ip -j -4 addr list lo scope host | jq -r -M '.[0].addr_info[0].local')
hostname=$(hostname)
# Generate the following files from templates:
# port-to-alias name map
sonic-cfggen -d -t /usr/share/sonic/templates/rsyslog.conf.j2 \
    -a "{\"udp_server_ip\": \"$udp_server_ip\", \"hostname\": \"$hostname\"}" \
    > /etc/rsyslog.conf
sonic-cfggen -d -t /usr/share/sonic/templates/port-name-alias-map.txt.j2,/tmp/port-name-alias-map.txt

# Make the script that waits for all interfaces to come up executable
chmod +x /etc/kea/lease_update.sh /usr/bin/start.sh
# The docker container should start this script as PID 1, so now that supervisord is
# properly configured, we exec /usr/local/bin/supervisord so that it runs as PID 1 for the
# duration of the container's lifetime
exec /usr/local/bin/supervisord
